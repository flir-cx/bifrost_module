/*
 * Copyright (c) FLIR Systems AB.
 *
 * bifrost_cdev.c
 *
 *  Created on: Mar 1, 2010
 *      Author: Jonas Romfelt <jonas.romfelt@flir.se>
 *              Tommy Karlsson
 *
 * Character device parts of driver.
 *
 */

#include <linux/module.h>
#include <linux/jiffies.h>

#include "bifrost.h"
#include "bifrost_dma.h"
#include "valhalla_dma.h"
#include "valhalla_msi.h"
#include "bifrost_platform.h"

static struct file_operations bifrost_fops;
static dev_t bifrost_dev_no;

/**
 * Initialize character device support of driver
 *
 * IMPORTANT! Do not call this function before the driver is ready to handle
 * all operations on the device. It will go "live" after this function
 * returns.
 *
 * @param dev The handle to bifrost device instance.
 * @return 0 on success.
 */
int bifrost_cdev_init(struct bifrost_device *dev)
{
        int rc;

        INFO("\n");

        /*
         * Register device number series and name, makes it appear
         * in /proc/devices
         */
        rc = alloc_chrdev_region(&bifrost_dev_no, 0, 1, BIFROST_DEVICE_NAME);
        if (rc < 0) {
                ALERT("register_chrdev_region() failed: %d\n", rc);
                return rc;
        }

        /* Init char device structure */
        cdev_init(&dev->cdev, &bifrost_fops);
        dev->cdev.owner = THIS_MODULE;

        /* Register char device to kernel, and go "live" */
        rc = cdev_add(&dev->cdev, bifrost_dev_no, 1);
        if (rc) {
                unregister_chrdev_region(bifrost_dev_no, 1);
                ALERT("cdev_add() failed: %d\n", rc);
                return rc;
        }

        dev->cdev_initialized = 1;

        return 0;
}

/**
 * Clean up character device parts from driver
 *
 * @param dev The handle to bifrost device instance.
 */
void __exit bifrost_cdev_exit(struct bifrost_device *dev)
{
        INFO("cdev_initialized=%d\n", dev->cdev_initialized);
        if (dev->cdev_initialized == 0)
                return;

        cdev_del(&dev->cdev);
        unregister_chrdev_region(bifrost_dev_no, 1);
}

/**
 * Handler for file operation open().
 *
 * @param inode
 * @param file
 * @return 0 on success.
 */
static int bifrost_open(struct inode *inode, struct file *file)
{
        struct bifrost_device *dev;
        struct bifrost_user_handle *hnd;

        INFO("\n");

        /* get struct that contains this cdev */
        dev = container_of(inode->i_cdev, struct bifrost_device, cdev);

        hnd = kzalloc(sizeof(struct bifrost_user_handle), GFP_KERNEL);
        if (hnd == NULL) {
                ALERT("Unable to allocate user handle struct\n");
                return -ENOMEM;
        }

        /* initialize struct members */
        hnd->dev = dev;
        INIT_LIST_HEAD(&hnd->event_list);
        spin_lock_init(&hnd->event_list_lock);
        hnd->event_list_count = 0;
        init_waitqueue_head(&hnd->waitq);

        /*
         * Add this handle _first_ to list of user handles. Lock necessary
         * since the list may be used by interrupt handler
         */
        spin_lock(&dev->lock_list);
        list_add(&hnd->node, &dev->list);
        spin_unlock(&dev->lock_list);

        /* allow access to user handle via file struct data pointer */
        file->private_data = hnd;

        return 0;
}

/**
 * Handler for file operation close(). Will remove the user handle.
 *
 * @param inode
 * @param file
 * @return 0 on success.
 */
static int bifrost_release(struct inode *inode, struct file *file)
{
        struct bifrost_user_handle *hnd = file->private_data;
        struct bifrost_device *dev = hnd->dev;
        struct bifrost_event_cont *p;
        struct list_head *pos, *tmp;

        INFO("\n");

        /*
         * Remove this handle from list of user handles. Lock necessary
         * since the list may be used by interrupt handler
         */
        spin_lock(&dev->lock_list);
        list_del(&hnd->node);
        spin_unlock(&dev->lock_list);

        spin_lock(&hnd->event_list_lock);
        list_for_each_safe(pos, tmp, &hnd->event_list) {
                p = list_entry(pos, struct bifrost_event_cont, node);
                list_del(&p->node);
                kfree(p);
        }
        spin_unlock(&hnd->event_list_lock);

        /*
         * TODO move to simulator
         *
         * De-register simulator memory if it was this handle that was the
         * responsible
         */
        if (dev->simulator.ram.hnd == hnd)
        {
                dev->simulator.ram.hnd = NULL;
                dev->simulator.ram.size = 0;
                dev->simulator.ram.address = 0;
        }

        kfree(hnd);
        return 0;
}

/**
 * Handler for file operation poll()/select(). Used to signal that events are available.
 *
 * @param file
 * @param wait
 * @return poll mask.
 */
static unsigned int bifrost_poll(struct file *file, poll_table *wait)
{
        struct bifrost_user_handle *hnd = file->private_data;
        unsigned int v;

        poll_wait(file, &hnd->waitq, wait);

        spin_lock(&hnd->event_list_lock);
        v = list_empty(&hnd->event_list) ? 0 : (POLLIN | POLLRDNORM);
        spin_unlock(&hnd->event_list_lock);

        return v;
}

/**
 * Handler for file operation mmap(). Will map common DMA'able scratch buffer.
 *
 * @param file
 * @param vma
 * @return 0 on success.
 */
static int bifrost_mmap(struct file *file, struct vm_area_struct *vma)
{
        struct bifrost_user_handle *hnd = file->private_data;
        struct bifrost_device *dev = hnd->dev;

        return dev->ops->remap_pfn_range(dev, vma);
}

static struct bifrost_event_cont *dequeue_event(struct bifrost_user_handle *h)
{
        struct bifrost_event_cont *p;

        spin_lock(&h->event_list_lock);
        if (list_empty(&h->event_list)) {
                spin_unlock(&h->event_list_lock);
                return NULL;
        }
        p = list_first_entry(&h->event_list, struct bifrost_event_cont, node);
        list_del(&p->node);
        h->event_list_count--;
        spin_unlock(&h->event_list_lock);

        return p;
}

static int enqueue_event(struct bifrost_user_handle *h,
                         struct bifrost_event_cont *p)
{
        spin_lock(&h->event_list_lock);
        if (h->event_list_count >= BIFROST_EVENT_BUFFER_SIZE) {
                spin_unlock(&h->event_list_lock);
                kfree(p);
                return -ENOSPC;
        }
        list_add_tail(&p->node, &h->event_list);
        h->event_list_count++;
        spin_unlock(&h->event_list_lock);

        return 0;
}

static int enqueue_on_this_handle(struct bifrost_user_handle *h,
                                  struct bifrost_event *e)
{
        if (!(e->type & h->event_enable_mask)) {
                return 0; /* Nothing to do for this user */
        }
        if (e->type == BIFROST_EVENT_TYPE_IRQ) {
                if (!(h->irq_forwarding_mask & e->data.irq_source))
                        return 0; /* Nothing to do for this user */
        } else if (e->type == BIFROST_EVENT_TYPE_DMA_DONE) {
                if (h != (void*)(unsigned long)e->data.dma.cookie)
                        return 0; /* Nothing to do for this user */
        }

        return 1;
}

static struct bifrost_event_cont *duplicate_event(struct bifrost_event *e,
                                                  gfp_t flags)
{
        struct bifrost_event_cont *p;

        p = kmalloc(sizeof(*p), flags);
        if (p == NULL)
                return NULL;

        memcpy(&p->event, e, sizeof(*e));
        return p;
}

/**
 * Create events to user handle subscribers.
 *
 * TODO for read/write REGB events add filter so that producer of event
 * does not receive an event!
 *
 * @param dev The device handle.
 * @param event The event data.
 */
void bifrost_create_event(struct bifrost_device *dev,
                          struct bifrost_event *event)
{
        struct bifrost_user_handle *hnd;
        struct list_head *pos, *tmp;
        struct bifrost_event_cont *p;

        do_gettimeofday(&event->timestamp.received);

        spin_lock(&dev->lock_list);
        list_for_each_safe(pos, tmp, &dev->list) {
                hnd = list_entry(pos, struct bifrost_user_handle, node);
                if (enqueue_on_this_handle(hnd, event)) {
                        p = duplicate_event(event, GFP_ATOMIC);
                        if (p != NULL) {
                                enqueue_event(hnd, p);
                                wake_up_interruptible(&hnd->waitq);
                        } else {
                                ALERT("dropped event type=%d", event->type);
                        }
                }
        }
        spin_unlock(&dev->lock_list);
}


int finish_dma_buffer(struct dma_usr_req *usr_req)
{

    dma_addr_t bus_addr = usr_req->bus_addr;
    __u32 size = usr_req->size;
    struct bifrost_user_handle *hnd = usr_req->cookie;
    struct pci_dev *dev = hnd->dev->pdev;
    void *  dev_buff = usr_req->dev_buff;

    switch(usr_req->up_down)
    {
    case BIFROST_DMA_DIRECTION_DOWN:
        pci_unmap_single(dev, bus_addr, size, DMA_TO_DEVICE);
        break;

    case BIFROST_DMA_DIRECTION_UP:
        pci_unmap_single(dev, bus_addr, size, DMA_FROM_DEVICE);
        if(copy_to_user(usr_req->usr_buff,dev_buff,size))
            goto e_exit;
        break;
    }

e_exit:
    kfree(dev_buff);

    return 0;
}


int prepare_dma_buffer( struct bifrost_dma_transfer *xfer,struct dma_req *req,
                        int up_down,struct  dma_usr_req * usr_req)
{
    __u32 size = usr_req->size = xfer->size;
    struct bifrost_user_handle *hnd = usr_req->cookie= req->cookie;
    struct pci_dev *dev = hnd->dev->pdev;
    void *dev_buff;
    dma_addr_t bus_addr=0;

    init_completion(&usr_req->work);

    req->pwork = &usr_req->work;             // save completion in request
    usr_req->usr_buff = (void*)xfer->system; // save usr ptr  in usr request
    usr_req->up_down = up_down;

    dev_buff = usr_req->dev_buff = kmalloc(size, GFP_KERNEL | __GFP_DMA); // allocate dma buffer
    if(dev_buff ==NULL)
           goto e_exit;

    switch (up_down)
    {
        case BIFROST_DMA_DIRECTION_DOWN: /* system memory -> FPGA memory */
            if(copy_from_user(dev_buff, usr_req->usr_buff,size)) // copy user buffer to dma buffer
                goto e_exit;
            bus_addr =   pci_map_single(dev, dev_buff, size, DMA_TO_DEVICE); // prepare buffer for dma transfer
            break;
        case BIFROST_DMA_DIRECTION_UP: /* FPGA memory -> system memory */
            bus_addr =   pci_map_single(dev, dev_buff, size, DMA_FROM_DEVICE); // prepare buffer for dma transfer
            break;
    }

    if(!bus_addr)
       goto e_exit;

     xfer->system = (unsigned long)bus_addr;       // update req with new dma buffer
     usr_req->bus_addr = (unsigned long)bus_addr;


     INFO("Starting DMA transfer %s using usrp %p bus %x devp %p with size %x \n",
          (usr_req->up_down ? "to FPGA":"from FPGA"),
         usr_req->usr_buff,usr_req->bus_addr,usr_req->dev_buff, usr_req->size);


    return 0;

e_exit:
    kfree(dev_buff);
    return -ENOMEM;
}


void wait_for_done(struct dma_usr_req *usr_req)
{

    int ret = wait_for_completion_interruptible_timeout(&usr_req->work, msecs_to_jiffies(1000));

    if(ret==0)
          INFO("BIFROST_DMA_TRANSFER timed out");

    finish_dma_buffer(usr_req);
}

static int do_dma_start_xfer(struct dma_ctl *ctl,
                             struct bifrost_dma_transfer *xfer,
                             int up_down,
                             void *cookie)
{
        struct dma_req *req;
        unsigned int ticket;
        struct dma_usr_req usr_req;
        req = alloc_dma_req(&ticket, cookie, GFP_KERNEL);
        if (req == NULL)
                return -ENOMEM;

        if(xfer->flags & BIFROST_DMA_USER_BUFFER) //buffer is allocated in user space, physical Non-Contiguous
           if(prepare_dma_buffer(xfer,req,up_down,&usr_req))
                return -ENOMEM;

        switch (up_down) {
        case BIFROST_DMA_DIRECTION_DOWN: /* system memory -> FPGA memory */
                req->src = xfer->system;
                req->dst = xfer->device;
                req->len = xfer->size;
                req->dir = VALHALLA_ADDR_DMA_DIR_UP_STRM_DOWN;
                break;
        case BIFROST_DMA_DIRECTION_UP: /* FPGA memory -> system memory */
                req->src = xfer->device;
                req->dst = xfer->system;
                req->len = xfer->size;
                req->dir = VALHALLA_ADDR_DMA_DIR_UP_STRM_UP;
                break;
        default:
                free_dma_req(req);
                return -EINVAL;
        }
        start_dma_xfer(ctl, req);

        if(xfer->flags & BIFROST_DMA_USER_BUFFER)
            wait_for_done(&usr_req);

        return (int)ticket;
}

#define NO_ACCESS 0
#define RD_ACCESS 0x1 /* Read */
#define WR_ACCESS 0x2 /* Write */
#define EV_ACCESS 0x4 /* Events */

static int check_bar_access(struct bifrost_device *dev, int bar, int access,
			    unsigned int offset)
{
	    /* In normal PCI mode, user-space doesn't have write access to BAR0 */
        const static int user_space_bar_access[6] = {
                RD_ACCESS | EV_ACCESS,             /* BAR0 */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR1 */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR2 */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR3 */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR4 */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR5 */
        };

        const static int user_space_bar_access_fvd[6] = {  /*used for */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR0  fvd registers       */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR1  ddr memory window   */
                0, /* BAR2 */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR3  dma engine control*/
                0, /* BAR4 */
                0, /* BAR5 */
        };

        /* In normal Membus mode, user-space doesn't have access to BAR2-5 */
        const static int user_space_membus_access[6] = {
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR0 */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR1 */
                0, /* BAR2 */
                0, /* BAR3 */
                0, /* BAR4 */
                0, /* BAR5 */
        };
        /* In simulator mode, user-space has full access to the BARs */
        const static int sim_user_space_bar_access[6] = {
		RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR0 */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR1 */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR2 */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR3 */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR4 */
                RD_ACCESS | WR_ACCESS | EV_ACCESS, /* BAR5 */
        };
        int v, mask;

        if (bar >= ARRAY_SIZE(user_space_bar_access)) {
                v = -EINVAL;
                goto e_exit;
        }
        if (!dev->regb[bar].enabled) {
                v = -EFAULT; /* BAR is not mapped into memory */
                goto e_exit;
        }
        if (dev->info.simulator) {
                mask = (access & sim_user_space_bar_access[bar]);
        } else if (platform_rocky()) {
            mask = (access & user_space_bar_access_fvd[bar]);
        } else if (dev->membus) {
           mask = (access & user_space_membus_access[bar]);
        } else {
                mask = (access & user_space_bar_access[bar]);
        }
        if (access != mask) {
                v = -EACCES; /* User hasn't sufficient access rights */
                goto e_exit;
        }
	if (offset >= dev->regb[bar].size) {
		v = -EFAULT; /* Offset is out-of-range */
		goto e_exit;
	}
        return 0;

  e_exit:
        ALERT("BAR%d not accessible from user-space (errno=%d)\n", bar, v);
        return v;
}

static int do_modify_regb(struct bifrost_device *dev, int bar, u32 offset,
                          u32 clear, u32 set, u32 *value)
{
        struct device_memory *mem;
        int rc;
        u32 v;

        rc = check_bar_access(dev, bar, (RD_ACCESS | WR_ACCESS), offset);
        if (rc < 0)
                return rc;

        mem = &dev->regb[bar];

        /*
         * Note: these registers are only accessed from user-space and
         *       never from ISRs etc. so a spin_lock/unlock should be
         *       enough...
         */
        spin_lock(&mem->lock);
        rc = mem->rd(mem->handle, offset, &v);
        if (rc < 0)
                goto e_exit;
        v = (v & ~clear) | set;
        rc = mem->wr(mem->handle, offset, v);
        if (rc < 0)
                goto e_exit;
        spin_unlock(&mem->lock);
        *value = v;

        INFO("BIFROST_IOCTL_MODIFY_REGB%u %#08x=%#08x\n", bar, offset, *value);

        return 0;

  e_exit:
        spin_unlock(&mem->lock);
        return rc;
}

static int do_read_regb(struct bifrost_device *dev, int bar,
                        unsigned int offset, unsigned int *value)
{
        int v;

        v = check_bar_access(dev, bar, RD_ACCESS, offset);
        if (v < 0)
                return v;

        v = dev->regb[bar].rd(dev->regb[bar].handle, offset, value);
        if (v < 0)
                return v;

        INFO("BIFROST_IOCTL_READ_REGB%u %#08x=%#08x\n", bar, offset, *value);

        return 0;
}

static int do_write_regb(struct bifrost_device *dev, int bar,
                         unsigned int offset, unsigned int value)
{
        int v;

        v = check_bar_access(dev, bar, WR_ACCESS, offset);
        if (v < 0)
                return v;

        v = dev->regb[bar].wr(dev->regb[bar].handle, offset, value);
        if (v < 0)
                return v;

        INFO("BIFROST_IOCTL_WRITE_REGB%u %#08x=%#08x\n", bar, offset, value);

        return 0;
}

/**
 * Handler for file operation ioctl().
 *
 * @param inode
 * @param cmd The IOCTL command provided by user space.
 * @param arg The IOCTL argument provided by user space.
 * @return 0 on success.
 */
static long bifrost_unlocked_ioctl(struct file *file, unsigned int cmd,
                                   unsigned long arg)
{
        struct bifrost_user_handle *hnd = file->private_data;
        struct bifrost_device *dev = hnd->dev;
        void __user *uarg = (void __user *)arg;
        int rc = 0;

        dev->stats.ioctls++;

        switch (cmd) {
        case BIFROST_IOCTL_INFO:
        {
                INFO("BIFROST_IOCTL_INFO\n");
                if (copy_to_user(uarg, &dev->info, sizeof(dev->info)))
                        return -EFAULT;
                break;
        }

        case BIFROST_IOCTL_READ_REGB:
        {
                struct bifrost_access a;

                if (copy_from_user(&a, uarg, sizeof(a)))
                        return -ENOMEM;
                rc = do_read_regb(dev, a.bar, a.offset, &a.value);
                if (rc < 0)
                        return rc;
                if (copy_to_user(uarg, &a, sizeof(a)))
                        return -EFAULT;
                break;
        }

        case BIFROST_IOCTL_READ_RANGE_REGB:
        {
                u32 i, *buf;
                void __user *user_ptr;
                struct bifrost_access_range a;

                if (copy_from_user(&a, uarg, sizeof(a)))
                        return -ENOMEM;

                user_ptr = (void __user *)a.values;
                buf = kmalloc(sizeof(u32) * a.count, GFP_KERNEL);
                if (NULL == buf)
                        return -ENOMEM;

                for (i = 0; i < a.count; ++i) {
                        rc = do_read_regb(dev, a.bar, a.offset + i * 4, &buf[i]);
                        if (rc < 0) {
                                kfree(buf);
                                return rc;
                        }
                }

                if (copy_to_user(user_ptr, (void *)buf, sizeof(u32) * a.count))
                        rc = -EFAULT;

                kfree(buf);
                break;
        }

        case BIFROST_IOCTL_WRITE_REGB:
        {
                struct bifrost_access a;

                if (copy_from_user(&a, uarg, sizeof(a)))
                        return -ENOMEM;

                rc = do_write_regb(dev, a.bar, a.offset, a.value);
                if (rc < 0)
                        return rc;
                break;
        }

        case BIFROST_IOCTL_MODIFY_REGB:
        {
                struct bifrost_modify m;

                if (copy_from_user(&m, uarg, sizeof(m)))
                        return -EFAULT;
                rc = do_modify_regb(dev, m.bar, m.offset, m.clear, m.set,
                                    &m.value);
                if (rc < 0)
                        return rc;
                if (copy_to_user(uarg, &m, sizeof(m)))
                        return -EFAULT;
                break;
        }

        case BIFROST_IOCTL_START_DMA_UP:
        case BIFROST_IOCTL_START_DMA_DOWN:
        {
                struct bifrost_dma_transfer xfer;
                int dir;

                if (cmd == BIFROST_IOCTL_START_DMA_DOWN)
                        dir = BIFROST_DMA_DIRECTION_DOWN;
                else
                        dir = BIFROST_DMA_DIRECTION_UP;

                if (copy_from_user(&xfer, uarg, sizeof(xfer)))
                        return -EFAULT;

                /*
                 * Note: the user handle is used as cookie for DMA done event
                 * matching.
                 */
                if (dev->membus)
                        rc = do_membus_xfer(dev, &xfer, dir);
                else
                        rc = do_dma_start_xfer(dev->dma_ctl, &xfer, dir, hnd);
                if (rc >= 0) {
                        INFO("BIFROST_DMA_TRANSFER_%s: sys=%08lx, dev=%08x, "
                             "len=%d\n",
                             dir == BIFROST_DMA_DIRECTION_DOWN ? "DOWN" : "UP",
                             xfer.system, xfer.device, xfer.size);
                }
                break;
        }

        case BIFROST_IOCTL_ENABLE_EVENT:
        {
                u32 mask = (u32)arg;

                hnd->event_enable_mask = mask;
                INFO("BIFROST_IOCTL_ENABLE_EVENT mask %#08x\n", mask);
                break;
        }

        case BIFROST_IOCTL_IRQ_FORWARDING:
        {
                u32 mask = (u32)arg;

                hnd->irq_forwarding_mask = mask;
                INFO("BIFROST_IOCTL_IRQ_FORWARDING mask %#08x\n", mask);
                break;
        }

        case BIFROST_IOCTL_DEQUEUE_EVENT:
        {
                struct bifrost_event_cont *p;

                INFO("BIFROST_IOCTL_DEQUEUE_EVENT\n");
                p = dequeue_event(hnd);
                if (p == NULL)
                        return -EINVAL;
                do_gettimeofday(&p->event.timestamp.forwarded);
                if (copy_to_user(uarg, &p->event, sizeof(p->event)) != 0) {
                        kfree(p);
                        return -EFAULT;
                }
                kfree(p);
                break;
        }

        case BIFROST_IOCTL_SIMULATE_IRQ:
        {
                u32 source;
                int msi;

                /* Sending fake IRQs are not allowed if not in simulator mode */
                if (dev->info.simulator == 0) {
                        INFO("not in simulator mode");
                        return -EINVAL;
                }
                if (get_user(source, (u32 __user *)arg))
                        return -EFAULT;
                msi = map_event_to_msi(source);
                if (msi < 0)
                        return -EINVAL;
                if (bifrost_simulate_msi(msi) < 0) {
                        INFO("failed to simulate MSI%d\n", msi);
                        return -EINVAL;
                }
                INFO("BIFROST_IOCTL_SIMULATE_IRQ 0x%08x\n", source);
                break;
        }

        case BIFROST_IOCTL_SETUP_SIMULATOR_MEMORY:
        {
                struct bifrost_simulator_memory memory;

                if (dev->info.simulator == 0) {
                        INFO("not in simulator mode");
                        return -EINVAL;
                }
                if (copy_from_user(&memory, uarg, sizeof(memory)))
                        return -ENOMEM;
                INFO("BIFROST_IOCTL_SETUP_SIMULATOR_MEMORY addr=%08lX, size=%d\n",
                     memory.address, memory.size);
                dev->simulator.ram.hnd = hnd;
                dev->simulator.ram.size = memory.size;
                dev->simulator.ram.address = memory.address;
                break;
        }

        case BIFROST_IOCTL_SIMULATE_DMA_TRANSFER:
        {
                struct bifrost_simulator_memory_transfer transfer;

                if (copy_from_user(&transfer, uarg, sizeof(transfer)))
                        return -ENOMEM;

                INFO("BIFROST_IOCTL_SIMULATE_DMA_TRANSFER_%s sys=0x%08lX, "
                     "sim=0x%08lX, len=%d\n", transfer.to ? "DOWN" : "UP",
                     transfer.system, transfer.simulator, transfer.size);

                if (dev->simulator.ram.hnd != hnd) {
                        INFO("this handle is not responsible for "
                             "the simulator memory\n");
                        return -EINVAL;
                }

                if (transfer.simulator < dev->simulator.ram.address ||
                    (transfer.simulator + transfer.size) >
                    (dev->simulator.ram.address + dev->simulator.ram.size)) {
                        INFO("risk for out of bounds memory copy\n");
                        return -EFAULT;
                }

                if (transfer.to) {
                        /* To FPGA-simulator from system memory: to user */
                        if (copy_to_user((void __user *)transfer.simulator,
                                        (void *)transfer.system,
                                         transfer.size) != 0)
                                return -EFAULT;
                } else {
                        /* To system memory from FPGA-simulator: from user */
                        if (copy_from_user((void *)transfer.system,
                                           (void __user *)transfer.simulator,
                                           transfer.size) != 0)
                                return -EFAULT;
                }
                break;
        }

        case BIFROST_IOCTL_SET_REGB_MODE:
        {
                struct bifrost_access a;
                void *h;

                if (copy_from_user(&a, uarg, sizeof(a)))
                        return -ENOMEM;
                rc = check_bar_access(dev, a.bar, EV_ACCESS, a.offset);
                if (rc < 0)
                        return rc;
                h = dev->regb[a.bar].handle;
                rc = dev->regb[a.bar].mset(h, a.offset, a.value);
                if (rc < 0)
                        return rc;
                INFO("BIFROST_IOCTL_SET_MODE_REGB%u %#08x=%#08x\n",
                     a.bar, a.offset, a.value);
                break;
        }

        case BIFROST_IOCTL_RESET_DMA:
        {
                /* This is a NOP when running on target! */
                if (dev->info.simulator) {
                        extern void reset_dma_sim(struct dma_ctl *ctl);
                        INFO("BIFROST_IOCTL_RESET_DMA\n");
                        reset_dma_sim(dev->dma_ctl);
                }
                break;
        }

        default:
                return -ENOTTY;
        }

        return rc;
}

#ifndef HAVE_UNLOCKED_IOCTL
static int bifrost_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
                         unsigned long arg)
{
        (void)inode;
        return (int)bifrost_unlocked_ioctl(file, cmd, arg);
}
#endif

/**
 * Defines the file operation supported by this driver.
 */
static struct file_operations bifrost_fops = {
        .owner = THIS_MODULE,
        .open = bifrost_open,
        .release = bifrost_release,
        .mmap = bifrost_mmap,
        .poll = bifrost_poll,
#ifdef HAVE_UNLOCKED_IOCTL
        .unlocked_ioctl = bifrost_unlocked_ioctl,
#else
        .ioctl = bifrost_ioctl,
#endif
};
