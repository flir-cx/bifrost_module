/* Copyright (c) 2013 FLIR Systems AB. All rights reserved. */
/*
 * i2c_usim.c - A I2C user-space simulator driver
 *
 * How to use the driver...
 *
 * ...
 * fd = open(name, O_RDONLY);
 * ...
 * fds[0].fd = fd;
 * fds[0].events = POLLIN;
 * fds[0].revents = 0;
 * ...
 * for (;;) {
 *         v = poll(fds, ARRAY_SIZE(fds), -1);
 *         if (v < 0)
 *                 <error>
 *         if (v == 0)
 *                 <timeout>
 *         for (n = 0; n < ARRAY_SIZE(fds); n++) {
 *                 if (fds[n].revents & POLLIN) {
 *                         ioctl(fd, I2C_USIM_IOC_GET_EVENT, &arg);
 *                         <do something with the event>
 *                         ioctl(fd, I2C_USIM_IOC_PUT_EVENT, &arg);
 *                 }
 *         }
 * }
 * ...
 */
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/wait.h>

#include "i2c_usim.h"

MODULE_LICENSE("GPL");

#define DRIVER_NAME "animal-i2c-usim"

#define STATE_READY 0
#define STATE_IN_PROGRESS 1
#define STATE_DONE 2

struct event {
        struct event_queue *queue; /* Added to this queue */
        struct list_head node;
        atomic_t refcnt;
        unsigned int ticket;

        /*
         * The event state-machine:
         *
         * - state     an event can be in one of three states, 0=ready,
         *             1=in-progress or 2=done. See below...
         * - status    signals if the event execution was OK or not,
         *             0 indicates success while a negative errno is set on
         *             error.
         * - waitq     used to block or wake-up processes involved in
         *             the event handling. Don't mistake it for the wait-queue
         *             that is used by the event-queue, which is used in
         *             the poll system call handling.
         *
         * An event's state is set to 0 when it's posted, i.e. added to
         * the event queue. Posting an event wakes up the receiver (in
         * user-space), which can retrieve the event with an ioctl system
         * call. This changes the event's state to 1. When the receiver has
         * finished processing the event, it is passed back with another
         * ioctl call, which changes the state to 2. The process that posted
         * the event waits (blocks) until the state is 2.
         *
         * If the receiver is killed (ctrl-c), any event in state 1 must be
         * completed, otherwise the sender will hang until it is terminated
         * (ctrl-c) even if the simulator is restarted. If an event is in
         * state 0 when the simulator is killed, it will be processed once
         * the simulator is restarted.
         */
        int status;
        int state;
        wait_queue_head_t waitq;

        /*
         * Some words about the I2C event data:
         *
         * - type     access type, e.g. i2c read/write, smbus read/write, etc.
         * - bus      bus or adapter number
         * - addr     slave address
         * - len      number of bytes in data area
         * - data     data area for slave read/write
         *
         * Note that the command in a SMBUS write/read is stored in the first
         * data byte, i.e. data[0]. In case of a SMBUS read, the command
         * shall be overwritten, i.e. always copy data to &data[0]...
         */
        u32 type;
        u32 bus;
        u32 addr;
        u32 len;
        u8 data[512];
};

struct event_queue {
        struct list_head list; /* Event list */
        spinlock_t lock; /* Protects event list */
        wait_queue_head_t waitq; /* Syscall poll blocks on this */
};

static struct event_queue i2c_eventq;

struct event *alloc_event(void)
{
        static atomic_t __ticket__ = ATOMIC_INIT(0);
        struct event *e;

        e = kzalloc(sizeof(*e), GFP_KERNEL);
        if (e != NULL) {
                e->queue = &i2c_eventq;
                e->ticket = atomic_add_return(1, &__ticket__);
                atomic_set(&e->refcnt, 1);
                init_waitqueue_head(&e->waitq);
        }
        return e;
}

static void post_event(struct event *e)
{
        spin_lock(&e->queue->lock);
        e->state = STATE_READY;
        list_add_tail(&e->node, &e->queue->list);
        spin_unlock(&e->queue->lock);

        wake_up(&e->queue->waitq);
}

static int wait_event_completion(struct event *e)
{
        int v;

        v = wait_event_interruptible(e->waitq, e->state == STATE_DONE);

        spin_lock(&e->queue->lock);
        list_del(&e->node);
        spin_unlock(&e->queue->lock);

        return (e->status == 0) ? v : e->status;
}

static void get_event(struct event *e)
{
        atomic_inc(&e->refcnt);
}

static void put_event(struct event *e)
{
        if (atomic_dec_and_test(&e->refcnt))
                kfree(e);
}

static struct event *get_1st_ready_event(void)
{
        struct event *e;
        struct list_head *h;

        spin_lock(&i2c_eventq.lock);
        list_for_each(h, &i2c_eventq.list) {
                e = list_entry(h, struct event, node);
                if (e->state == STATE_READY) {
                        e->state = STATE_IN_PROGRESS;
                        get_event(e);
                        spin_unlock(&i2c_eventq.lock);
                        return e;
                }
        }
        spin_unlock(&i2c_eventq.lock);

        return NULL;
}

static struct event *lookup_event_w_ticket(unsigned int ticket)
{
        struct event *e;
        struct list_head *h;

        spin_lock(&i2c_eventq.lock);
        list_for_each(h, &i2c_eventq.list) {
                e = list_entry(h, struct event, node);
                /* Also match pid?!?*/
                if (e->ticket == ticket) {
                        spin_unlock(&i2c_eventq.lock);
                        return e;
                }
        }
        spin_unlock(&i2c_eventq.lock);

        return NULL;
}

static void complete_event(struct event *e, int status)
{
        e->status = status;
        e->state = STATE_DONE;
        wake_up(&e->waitq);
        put_event(e);
}

static void init_event_queue(struct event_queue *q)
{
        INIT_LIST_HEAD(&q->list);
        spin_lock_init(&q->lock);
        init_waitqueue_head(&q->waitq);
}

/*
 * Character device driver part...
 */
static int i2c_usim_major;
static atomic_t i2c_usim_users = ATOMIC_INIT(0);

static int i2c_usim_open(struct inode *inode, struct file *file)
{
        (void)inode;
        (void)file;

        /* Only allow one user to open this device */
        return atomic_add_unless(&i2c_usim_users, 1, 1) ? 0 : -EBUSY;
}

static int i2c_usim_release(struct inode *inode, struct file *file)
{
        struct list_head *h;
        struct event *e;

        (void)inode;
        (void)file;

        /*
         * Complete any in-progess events, e.g. simulator has done ioctl-get,
         * receives a ctrl-c before it can do ioctl-put.
         */
        spin_lock(&i2c_eventq.lock);
        list_for_each(h, &i2c_eventq.list) {
                e = list_entry(h, struct event, node);
                if (e->state == STATE_IN_PROGRESS) {
                        /* Complete orphan events */
                        complete_event(e, -EIO);
                }
        }
        spin_unlock(&i2c_eventq.lock);

        atomic_dec(&i2c_usim_users);
        return 0;
}

unsigned int i2c_usim_poll(struct file *file, struct poll_table_struct *ptab)
{
        struct list_head *h;
        struct event *e;

        poll_wait(file, &i2c_eventq.waitq, ptab);

        /* Check if there are any "ready" events */
        spin_lock(&i2c_eventq.lock);
        list_for_each(h, &i2c_eventq.list) {
                e = list_entry(h, struct event, node);
                if (e->state == STATE_READY) {
                        spin_unlock(&i2c_eventq.lock);
                        return (POLLIN | POLLRDNORM);
                }
        }
        spin_unlock(&i2c_eventq.lock);

        return 0;
}

static int handle_i2c_usim_ioctl(struct file *file, unsigned int cmd,
                                 unsigned long arg)
{
        struct i2c_usim_ioc_event __user *p;
        struct event *e;
        unsigned int ticket;
        unsigned int len;

        switch (cmd) {
        case I2C_USIM_IOC_GET_EVENT:
                e = get_1st_ready_event();
                if (e == NULL)
                        return -EINVAL;

                p = (struct i2c_usim_ioc_event __user *)arg;
                if (put_user(e->ticket, &p->ticket) != 0)
                        goto out;
                if (put_user(e->type, &p->type) != 0)
                        goto out;
                if (put_user(e->bus, &p->bus) != 0)
                        goto out;
                if (put_user(e->addr, &p->addr) != 0)
                        goto out;
                if (put_user(e->len, &p->len) != 0)
                        goto out;
                if (ACCESS_TYPE(e->type) & WRITE_ACCESS)
                        len = e->len;
                else
                        len = 1; /* Command required by SMB protocol */
                if (copy_to_user(p->data, e->data, len) != 0)
                        goto out;
                return 0;
        case I2C_USIM_IOC_PUT_EVENT:
                p = (struct i2c_usim_ioc_event __user *)arg;
                if (get_user(ticket, &p->ticket) != 0)
                        return -EFAULT;

                e = lookup_event_w_ticket(ticket);
                if (e == NULL)
                        return -EINVAL;

                if (ACCESS_TYPE(e->type) & READ_ACCESS) {
                        if (get_user(e->len, &p->len) != 0)
                                goto out;
                        if (e->len > ARRAY_SIZE(e->data))
                                goto out;
                        if (copy_from_user(e->data, p->data, e->len) != 0)
                                goto out;
                }

                complete_event(e, 0);
                return 0;
        default:
                return -ENOTTY;
        }
  out:
        complete_event(e, -EIO);
        return -EFAULT;
}

#ifdef HAVE_UNLOCKED_IOCTL
static long i2c_usim_ioctl(struct file *file, unsigned int cmd,
                           unsigned long arg)
{
        return (long)handle_i2c_usim_ioctl(file, cmd, arg);
}
#else
static int i2c_usim_ioctl(struct inode *inode, struct file *file,
                          unsigned int cmd, unsigned long arg)
{
        (void)inode;
        return handle_i2c_usim_ioctl(file, cmd, arg);
}
#endif

static struct file_operations i2c_usim_fops =
{
        .owner = THIS_MODULE,
        .open = i2c_usim_open,
        .release = i2c_usim_release,
        .poll = i2c_usim_poll,
#ifdef HAVE_UNLOCKED_IOCTL
        .unlocked_ioctl = i2c_usim_ioctl,
#else
        .ioctl = i2c_usim_ioctl,
#endif
#ifdef HAVE_COMPAT_IOCTL /* No 32/64 conversion needed or... */
        .compat_ioctl = i2c_usim_ioctl,
#endif
};

static const struct i2c_algorithm i2c_usim_algo;

static struct i2c_adapter i2c_usim_adapter = {
        .owner = THIS_MODULE,
        .name = "i2c usim bus",
        .algo = &i2c_usim_algo,
};

static int i2c_usim_proc_read(char *buf, char **start, off_t offset, int count,
                              int *eof, void *data)
{
        struct i2c_adapter *adap;
        int len;

        adap = data;
        len = snprintf(buf, count, "%d\n", adap->nr);
        *eof = 1;

        return len;
}

static int __init i2c_usim_init(void)
{
        int v;

        v = i2c_add_adapter(&i2c_usim_adapter);
        if (v < 0) {
                printk(KERN_ERR DRIVER_NAME ": i2c_add_adapter failed with "
                       "%d\n", v);
                return v;
        }
        printk(KERN_INFO DRIVER_NAME ": registered i2c-adapter %d\n",
               i2c_usim_adapter.nr);

        if (create_proc_read_entry(DRIVER_NAME, 0, NULL, i2c_usim_proc_read,
                                   &i2c_usim_adapter) == NULL) {
                i2c_del_adapter(&i2c_usim_adapter);
                printk(KERN_ERR DRIVER_NAME ": create_proc_read_entry "
                       "failed\n");
                return -EINVAL;
        }

        init_event_queue(&i2c_eventq);

        v = register_chrdev(0, DRIVER_NAME, &i2c_usim_fops);
        if (v < 0) {
                remove_proc_entry(DRIVER_NAME, NULL);
                i2c_del_adapter(&i2c_usim_adapter);
                printk(KERN_ERR DRIVER_NAME ": register_chrdev failed with "
                       "%d\n", v);
                return v;
        }
        i2c_usim_major = v;

        return 0;
}
module_init(i2c_usim_init);

static void __exit i2c_usim_exit(void)
{
        unregister_chrdev(i2c_usim_major, DRIVER_NAME);
        remove_proc_entry(DRIVER_NAME, NULL);
        i2c_del_adapter(&i2c_usim_adapter);
}
module_exit(i2c_usim_exit);

/*
 * Simulate low-level HW driver functions...
 */
static struct event *alloc_i2c_event(int type, int bus, u32 adr, u32 len)
{
        struct event *e;

        e = alloc_event();
        if (e != NULL) {
                e->type = type;
                e->bus = bus;
                e->addr = adr;
                e->len = len;
        }
        return e;
}

static int usim_smb_read(int bus, u16 addr, u8 command, int size,
                         union i2c_smbus_data *data)
{
        struct event *e;
        int v, len;
        void *buf;

        switch (size) {
	case I2C_SMBUS_BYTE:		
        case I2C_SMBUS_BYTE_DATA:
                len = 1;
                buf = &data->byte;
                break;
        case I2C_SMBUS_WORD_DATA:
                len = 2;
                buf = &data->word;
                break;
        case I2C_SMBUS_BLOCK_DATA:
                len = 2 + I2C_SMBUS_BLOCK_MAX;
                buf = &data->block[1];
                break;
        default:
                return -EINVAL;
        }

        e = alloc_i2c_event(I2C_USIM_SMB_READ, bus, addr, len);
        if (e == NULL)
                return -ENOMEM;

        e->data[0] = command;
        memcpy(e->data, &command, 1);
        post_event(e);

        v = wait_event_completion(e);
        if (v == 0) {
                if (size == I2C_SMBUS_BLOCK_DATA) {
                        if ((e->len > 0) && (e->len <= I2C_SMBUS_BLOCK_MAX)) {
                                data->block[0] = (u8)e->len;
                                len = e->len;
                        } else {
                                v = -EPROTO;
                                len = 0;
                        }
                }
                memcpy(buf, e->data, len);
        }
        put_event(e);
        return v;
}

static int usim_smb_write(int bus, u16 addr, u8 command, int size,
                          union i2c_smbus_data *data)
{
        struct event *e;
        int v, len;
        void *buf;

        switch (size) {
	case I2C_SMBUS_BYTE:
                len = 0;
		break;
        case I2C_SMBUS_BYTE_DATA:
                len = 1;
                buf = &data->byte;
                break;
        case I2C_SMBUS_WORD_DATA:
                len = 2;
                buf = &data->word;
                break;
        case I2C_SMBUS_BLOCK_DATA:
                len = data->block[0];
                if ((len == 0) || (len > I2C_SMBUS_BLOCK_MAX))
                        return -EINVAL;
                buf = &data->block[1];
                break;
        default:
                return -EINVAL;
        }

        /* Add one extra byte for the command */
        e = alloc_i2c_event(I2C_USIM_SMB_WRITE, bus, addr, 1 + len);
        if (e == NULL)
                return -ENOMEM;

        memcpy(e->data, &command, 1);
        memcpy(e->data + 1, buf, len);
        post_event(e);

        v = wait_event_completion(e);
        put_event(e);
        return v;
}

static int smb_usim_xfer(struct i2c_adapter *adap, u16 addr,
                         unsigned short flags, char read_write, u8 command,
                         int size, union i2c_smbus_data *data)
{
        switch (read_write) {
        case I2C_SMBUS_READ:
                return usim_smb_read(adap->nr, addr, command, size, data);
        case I2C_SMBUS_WRITE:
                return usim_smb_write(adap->nr, addr, command, size, data);
        default:
                return -EINVAL;
        }
}

static int usim_i2c_read(int bus, struct i2c_msg *msg)
{
        struct event *e;
        int v;

        e = alloc_i2c_event(I2C_USIM_I2C_READ, bus, msg->addr, msg->len);
        if (e == NULL)
                return -ENOMEM;

        post_event(e);

        v = wait_event_completion(e);
        if (v == 0)
                memcpy(msg->buf, e->data, msg->len);
        put_event(e);

        return v;
}

static int usim_i2c_write(int bus, struct i2c_msg *msg)
{
        struct event *e;
        int v;

        e = alloc_i2c_event(I2C_USIM_I2C_WRITE, bus, msg->addr, msg->len);
        if (e == NULL)
                return -ENOMEM;

        memcpy(e->data, msg->buf, msg->len);
        post_event(e);

        v = wait_event_completion(e);
        put_event(e);

        return v;
}

static int i2c_usim_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
                         int num)
{
        int n, count;

        count = 0;
        for (n = 0; n < num; n++) {
                if ((msgs + n)->flags & I2C_M_RD) {
                        if (usim_i2c_read(adap->nr, msgs + n) == 0)
                                count++; /* OK */
                } else {
                        if (usim_i2c_write(adap->nr, msgs + n) == 0)
                                count++; /* OK */
                }
        }

        return count;
}

static u32 i2c_usim_functionality(struct i2c_adapter *adap)
{
	return (I2C_FUNC_SMBUS_BYTE | I2C_FUNC_SMBUS_BYTE_DATA |
		I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_BLOCK_DATA);
}

static const struct i2c_algorithm i2c_usim_algo = {
        .master_xfer = i2c_usim_xfer,
        .smbus_xfer = smb_usim_xfer,
	.functionality = i2c_usim_functionality,
};
