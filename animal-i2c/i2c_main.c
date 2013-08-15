/* Copyright (c) 2013 FLIR Systems AB. All rights reserved. */
/*
 * animal-i2c - FLIR Heimdall Driver, for GNU/Linux kernel 2.6.x
 *
 *  Created on: Apr 20, 2010
 *      Author: Jonas Romfelt <jonas.romfelt@flir.se>
 *
 * A generic driver that allows user space written drivers for i2c and SMBus
 * devices, which also supports custom (exotic?) GPIO device chip select.
 *
 * The i2c_core implements per adapter locking to avoid race conditions. This
 * driver adds an extra layer of locking in order to avoid possible out of
 * sync issues with GPIO chip select. Each i2c transfer that this driver does
 * begins with setting a per adapter lock and conditionally also selecting
 * the device with GPIO. After transfer is completed, GPIO chip select is
 * released and lock is released.
 *
 */

#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "animal-i2c_api.h"
#include "i2c_usim.h"

#define ANIMAL_DEVICE_NAME "animal-i2c"

#define ANIMAL_VERSION_MAJOR 0
#define ANIMAL_VERSION_MINOR 1
#define ANIMAL_VERSION_MICRO 0
#define ANIMAL_VERSION_APPEND_STR ""
#define ANIMAL_VERSION_NUMBER ((ANIMAL_VERSION_MAJOR*10000)+(ANIMAL_VERSION_MINOR*100)+(ANIMAL_VERSION_MICRO))

/*
 * Module parameters
 */

static unsigned int adapters = 2;
module_param(adapters, uint, S_IRUSR);
MODULE_PARM_DESC(adapters, "Number of i2c adapters to export");

static unsigned int simulator = 0;
module_param(simulator, uint, S_IRUSR);
MODULE_PARM_DESC(simulator, "Enable simulator interface");

static int sim_adap_nr = -1;
module_param(sim_adap_nr, int, S_IRUSR);
MODULE_PARM_DESC(sim_adap_nr, "Adapter number for simulated i2c bus. "
                 "Get it from /proc");

/*
 * Debug macros
 */

#define DEBUG_ON

#ifndef DEBUG_ON
  #define INFO(fmt, args...)
  #define NOTICE(fmt, args...)
#else
  #define INFO(fmt, args...) printk(KERN_INFO "%s %s:%d> " fmt, ANIMAL_DEVICE_NAME, __FUNCTION__, __LINE__, ##args)
  #define NOTICE(fmt, args...) printk(KERN_NOTICE "%s %s:%d> " fmt, ANIMAL_DEVICE_NAME, __FUNCTION__, __LINE__, ##args)
#endif
#define ALERT(fmt, args...) printk(KERN_ALERT "%s %s:%d> " fmt, ANIMAL_DEVICE_NAME, __FUNCTION__, __LINE__, ##args)

static struct file_operations animal_fops;
static struct animal_gpio_operations animal_gpio_ops;
static struct animal_gpio_operations simulator_gpio_ops;

struct animal_gpio_operations {
        int (*is_valid)(int);
        int (*request)(unsigned gpio, const char *label);
        void (*free)(unsigned gpio);
        int (*direction_input)(unsigned gpio);
        int (*direction_output)(unsigned gpio, int value);
        void (*set)(unsigned gpio, int value);
};

struct animal_adapter {
        struct i2c_adapter *adapter;
        struct mutex gpio_lock; /* lock adapter when using GPIO chip-select */
};

struct animal_device {
        struct animal_i2c_info info;
        struct cdev cdev; /* char device structure */
        struct animal_adapter *animal_adapter;
        struct animal_gpio_operations *gpio_ops;
        int users;
};
static struct animal_device *animal_dev;

struct animal_user_handle {
        struct animal_device *dev;
        int gpio; /* gpio pin number to select i2c device, default is -1 i.e. don't use gpio */
        struct animal_adapter *animal_adapter; /* pointer to animal_adapter that user has set */
        struct i2c_client client; /* client struct used for SMBus calls */
        /* address is stored in client.addr */
        /* adapter is also stored in client.adapter so that the i2c_client can be used directly */
};

static struct i2c_driver animal_driver = {
        .driver = {
                .name   = "animal-i2c",
        },
};

/**
 * Helper function to initialize a gpio pin
 * @dev: animal device
 * @gpio: pin number
 * @label: set a text label associated with this pin
 * @direction: specifies direction of gpio, direction == 0 means output and direction != 0 input
 * @value: if gpio is configured as output, value is used to set current gpio value
 *
 * Returns 0 on success
 */
static int animal_gpio_init(const struct animal_device *dev, unsigned gpio,
                            const char *label, int direction, int value)
{
        int rc;

        if ((rc = dev->gpio_ops->is_valid(gpio)) == 0) {
                ALERT("Invalid gpio %d\n", gpio);
                return rc;
        }

        if ((rc = dev->gpio_ops->request(gpio, label)) < 0) {
                ALERT("Failed to request gpio %d (%s), err %d\n", gpio, label, rc);
                return rc;
        }

        if (direction == 0)
                rc = dev->gpio_ops->direction_output(gpio, value);
        else
                rc = dev->gpio_ops->direction_input(gpio);

        if (rc != 0) {
                ALERT("Failed to set gpio %d direction\n", gpio);
                return rc;
        }

        return 0;
}


/**
 * Helper functions that should be called prior and after an access
 * to i2c-core functions. If GPIO is used for this particular device
 * the functions will set the GPIO accordingly and ensure that there
 * is no addressing collision by using per bus locks.
 */
static inline void animal_i2c_xfer_begin(struct animal_user_handle *hnd)
{
        mutex_lock(&hnd->animal_adapter->gpio_lock);

        if (hnd->gpio >= 0)
                hnd->dev->gpio_ops->set(hnd->gpio, 1);
}
static inline void animal_i2c_xfer_done(struct animal_user_handle *hnd)
{
        if (hnd->gpio >= 0)
                hnd->dev->gpio_ops->set(hnd->gpio, 0);

        mutex_unlock(&hnd->animal_adapter->gpio_lock);
}


/**
 * Handler for file operation ioctl()
 */
static long do_animal_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        struct animal_user_handle *hnd = (struct animal_user_handle *)file->private_data;
        struct animal_device *dev = hnd->dev;
        struct animal_i2c_message __user *user_arg = (struct animal_i2c_message __user *)arg;
        struct i2c_msg msg[2];
        u8 command;
        u32 size;
        u8 byte;
        u16 word;
        u8 block[ANIMAL_SMB_BUFFER_MAX_SIZE];
        u8 *pblock;
        u16 tmp_us;
        int tmp_i;
        int rc = 0;

        switch (cmd) {
        case ANIMAL_IOCTL_INFO :
                /* INFO("ANIMAL_IOCTL_INFO\n"); */
                if (copy_to_user((void __user *)arg, &dev->info, sizeof(struct animal_i2c_info)))
                        return -EFAULT;
                break;

        case ANIMAL_IOCTL_SET_ADDRESS :
                if (get_user(tmp_us, (unsigned short __user *)arg))
                        return -EFAULT;

                /* INFO("ANIMAL_IOCTL_SET_ADDRESS address=0x%02X\n", tmp_us); */

                hnd->client.addr = tmp_us;
                break;

        case ANIMAL_IOCTL_SET_GPIO :
                if (get_user(tmp_i, (int __user *)arg))
                        return -EFAULT;

                /* INFO("ANIMAL_IOCTL_SET_GPIO gpio=%d\n", tmp_i); */

                if (tmp_i != hnd->gpio) {
                        if (hnd->gpio >= 0)
                                dev->gpio_ops->free(hnd->gpio);
                        if (tmp_i >= 0) {
                                if (animal_gpio_init(dev, tmp_i, "animal-i2c gpio", 0, 0) == 0) {
                                        hnd->gpio = tmp_i;
                                } else {
                                        hnd->gpio = -1;
                                        return -EINVAL;
                                }
                        }
                }
                break;

        case ANIMAL_IOCTL_SET_ADAPTER :
                if (get_user(tmp_i, (int __user *)arg))
                        return -EFAULT;

                if (tmp_i < 0 || tmp_i > (adapters - 1))
                        return -EINVAL;

                /* return error if trying to set an adapter that was not initialized properly */
                if (dev->animal_adapter[tmp_i].adapter == NULL)
                        return -EINVAL;
		  
                /* INFO("ANIMAL_IOCTL_SET_ADAPTER adapter=%d\n", tmp_i); */

                hnd->animal_adapter = &dev->animal_adapter[tmp_i];
                hnd->client.adapter = hnd->animal_adapter->adapter;
                break;

        case ANIMAL_IOCTL_SMB_READ8 :
                if (get_user(command, &user_arg->command))
                    return -EFAULT;

                /* INFO("ANIMAL_IOCTL_SMB_READ8 command=0x%02X", command); */

                animal_i2c_xfer_begin(hnd);
                rc = i2c_smbus_read_byte_data(&hnd->client, command);
                animal_i2c_xfer_done(hnd);

                if (rc >= 0) {
                        byte = rc & 0xff;
                        if (put_user(byte, &user_arg->data.byte))
                                return -EFAULT;
                        rc = 0; /* OK */
                }
                break;

        case ANIMAL_IOCTL_SMB_WRITE8 :
                if (get_user(command, &user_arg->command))
                        return -EFAULT;
                if (get_user(byte, &user_arg->data.byte))
                          return -EFAULT;

                /* INFO("ANIMAL_IOCTL_SMB_WRITE8 command=0x%02X, byte=0x%02X", command, byte); */

                animal_i2c_xfer_begin(hnd);
                rc = i2c_smbus_write_byte_data(&hnd->client, command, byte);
                animal_i2c_xfer_done(hnd);
                break;

        case ANIMAL_IOCTL_SMB_READ16 :
                if (get_user(command, &user_arg->command))
                    return -EFAULT;

                /* INFO("ANIMAL_IOCTL_SMB_READ16 command=0x%02X", command); */

                animal_i2c_xfer_begin(hnd);
                rc = i2c_smbus_read_word_data(&hnd->client, command);
                animal_i2c_xfer_done(hnd);

                if (rc >= 0) {
                        word = rc & 0xffff;
                        if (put_user(word, &user_arg->data.word))
                                return -EFAULT;
                        rc = 0; /* OK */
                }
                break;

        case ANIMAL_IOCTL_SMB_WRITE16 :
                if (get_user(command, &user_arg->command))
                        return -EFAULT;
                if (get_user(word, &user_arg->data.word))
                        return -EFAULT;

                /* INFO("ANIMAL_IOCTL_SMB_WRITE16 command=0x%02X, word=0x%04X", command, word); */

                animal_i2c_xfer_begin(hnd);
                rc = i2c_smbus_write_word_data(&hnd->client, command, word);
                animal_i2c_xfer_done(hnd);
                break;

        case ANIMAL_IOCTL_SMB_READ_BLOCK :
                if (get_user(command, &user_arg->command))
                        return -EFAULT;
                if (get_user(size, &user_arg->size))
                        return -EFAULT;

                /* INFO("ANIMAL_IOCTL_SMB_READ_BLOCK command=0x%02X, size=%d\n", command, size); */

                if (size > ANIMAL_SMB_BUFFER_MAX_SIZE)
                        size = ANIMAL_SMB_BUFFER_MAX_SIZE;

                animal_i2c_xfer_begin(hnd);
                rc = i2c_smbus_read_block_data(&hnd->client, command, block);
                animal_i2c_xfer_done(hnd);

                if (rc >= 0) {
                        if (copy_to_user(user_arg->data.block, block, size))
                                return -EFAULT;
                        rc = 0;
                }
                break;

        case ANIMAL_IOCTL_SMB_WRITE_BLOCK :
                if (get_user(command, &user_arg->command))
                        return -EFAULT;
                if (get_user(size, &user_arg->size))
                        return -EFAULT;

                /* INFO("ANIMAL_IOCTL_SMB_WRITE_BLOCK command=0x%02X, size=%d\n", command, size); */

                if (size > ANIMAL_SMB_BUFFER_MAX_SIZE)
                        size = ANIMAL_SMB_BUFFER_MAX_SIZE;

                if (copy_from_user(block, user_arg->data.block, size))
                        return -EFAULT;

                animal_i2c_xfer_begin(hnd);
                rc = i2c_smbus_write_block_data(&hnd->client, command, size, block);
                animal_i2c_xfer_done(hnd);
                break;

        /*
        * Corresponds to a "write sequence" as specified in ADV7180 data sheet, i.e.
        * a i2c write message.
        *
        * Prepend the message data with the command (offset) byte.
        */
        case ANIMAL_IOCTL_I2C_WRITE :
                if (get_user(command, &user_arg->command))
                        return -EFAULT;
                if (get_user(size, &user_arg->size))
                        return -EFAULT;

                /* INFO("ANIMAL_IOCTL_I2C_WRITE command=0x%02X, size=%d\n", command, size); */

                if (size > ANIMAL_I2C_BUFFER_MAX_SIZE)
                        return -EFAULT;

                if ((size + 1) > ANIMAL_SMB_BUFFER_MAX_SIZE) {
                        pblock = kmalloc(size + 1, GFP_KERNEL);
                        if (pblock == NULL)
                                return -ENOMEM;
                } else {
                        pblock = block; /* re-use block buffer */
                }

                if (copy_from_user(pblock + 1, user_arg->data.block, size)) {
                        rc = -EFAULT;
                } else {
                        pblock[0] = command;
                        msg[0].addr = hnd->client.addr;
                        msg[0].flags = 0; /* I2C_M_WR */
                        msg[0].buf = pblock;
                        msg[0].len = size + 1;

                        animal_i2c_xfer_begin(hnd);
                        rc = i2c_transfer(hnd->client.adapter, msg, 1);
                        animal_i2c_xfer_done(hnd);
                }

                if ((size + 1) > ANIMAL_SMB_BUFFER_MAX_SIZE)
                        kfree(pblock);
                break;

        /*
         * Corresponds to a "read sequence" as specified in ADV7180 data sheet, i.e.
         * a i2c transfer comprising a write message containing the command (offset)
         * byte followed by a read message.
         */
        case ANIMAL_IOCTL_I2C_WRITEREAD :
                if (get_user(command, &user_arg->command))
                        return -EFAULT;
                if (get_user(size, &user_arg->size))
                        return -EFAULT;

                /* INFO("ANIMAL_IOCTL_I2C_WRITEREAD command=0x%02X, size=%d\n", command, size); */

                if (size > ANIMAL_I2C_BUFFER_MAX_SIZE)
                        return -EFAULT;

                if (size > ANIMAL_SMB_BUFFER_MAX_SIZE) {
                        pblock = kmalloc(size, GFP_KERNEL);
                        if (pblock == NULL)
                                return -ENOMEM;
                } else {
                        pblock = block; /* re-use block buffer */
                }

                /* write command information 1 byte */
                msg[0].addr = hnd->client.addr;
                msg[0].flags = 0; /* I2C_M_WR */
                msg[0].buf = &command;
                msg[0].len = 1;

                /* read registers */
                msg[1].addr = hnd->client.addr;
                msg[1].flags = I2C_M_RD;
                msg[1].buf = pblock;
                msg[1].len = size;

                animal_i2c_xfer_begin(hnd);
                rc = i2c_transfer(hnd->client.adapter, msg, 2);
                animal_i2c_xfer_done(hnd);

                if (rc >= 0) {
                        if (copy_to_user(user_arg->data.block, block, size))
                                rc = -EFAULT;
                        else
                                rc = 0;
                }

                if (size > ANIMAL_SMB_BUFFER_MAX_SIZE)
                        kfree(pblock);
                break;

        default :
                /* INFO("unknown ioctl-command\n"); */
                return -ENOTTY;
        }

        return rc;
}

#ifdef HAVE_UNLOCKED_IOCTL
static long animal_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
        return do_animal_ioctl(file, cmd, arg);
}
#else
static int animal_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
                        unsigned long arg)
{
        (void)inode;
        return (int)do_animal_ioctl(file, cmd, arg);
}
#endif


/**
 * Handler for file operation open()
 */
static int animal_open(struct inode *inode, struct file *file)
{
        struct animal_device *dev;
        struct animal_user_handle *hnd;
        int i;

        /* INFO("open()\n"); */

        /* get struct that contains this cdev */
        dev = container_of(inode->i_cdev, struct animal_device, cdev);

        if ((hnd = kzalloc(sizeof(struct animal_user_handle), GFP_KERNEL)) == NULL) {
                ALERT("Unable to allocate user handle struct\n");
                return -ENOMEM;
        }

        /* use first available adapter as default */
        for (i = 0; i < adapters; i++)
                if (dev->animal_adapter[i].adapter != NULL)
                        break;

        if (i == adapters) {
                ALERT("No i2c adapter initilized successfully\n");
                return -EFAULT;
        }

        hnd->dev = dev;
        hnd->gpio = -1; /* default is not to use any gpio */
        hnd->animal_adapter = &dev->animal_adapter[i];
        hnd->client.addr = 0;
        hnd->client.adapter = hnd->animal_adapter->adapter;
        hnd->client.driver = &animal_driver;
        snprintf(hnd->client.name, I2C_NAME_SIZE, "animal-i2c %d", dev->users++);

        /* allow access to user handle via file struct data pointer */
        file->private_data = hnd;

        return 0;
}


/**
 * Handler for file operation close(), should clean up everything that the user has been
 * messing with.
 */
static int animal_release(struct inode *inode, struct file *file)
{
        struct animal_user_handle *hnd = (struct animal_user_handle *)file->private_data;

        /* INFO("close()\n"); */

        if (hnd->gpio >= 0)
                hnd->dev->gpio_ops->free(hnd->gpio);

        kfree(hnd);

        return 0;
}

static int install_adapters(struct animal_device *dev, unsigned int n_adap,
                            int adap_nr)
{
        int i, *nr;

        /* A non-negative adapter number indicates simulator mode */
        nr = (adap_nr < 0) ? &i : &adap_nr;
        for (i = 0; i < n_adap; i++) {
                dev->animal_adapter[i].adapter = i2c_get_adapter(*nr);
                if (dev->animal_adapter[i].adapter == NULL) {
                        ALERT("failed to get i2c adapter %d\n", *nr);
                }
                mutex_init(&dev->animal_adapter[i].gpio_lock);
        }
        return 0;
}

/**
 * Entry point to driver.
 */
static int __init animal_init(void)
{
        int rc, i;
        dev_t dev_no = MKDEV(ANIMAL_DEVICE_MAJOR, ANIMAL_DEVICE_MINOR);

        if (simulator)
                INFO("driver started in simulator mode\n");

        if ((animal_dev = kzalloc(sizeof(struct animal_device), GFP_KERNEL)) == NULL) {
                ALERT("failed to allocate animal_device\n");
                return -ENOMEM;
        }

        animal_dev->info.version.major = ANIMAL_VERSION_MAJOR;
        animal_dev->info.version.minor = ANIMAL_VERSION_MINOR;
        animal_dev->info.version.revision = ANIMAL_VERSION_MICRO;
        if (simulator)
            animal_dev->info.simulator = 1;

        if ((animal_dev->animal_adapter = kzalloc(adapters * sizeof(struct animal_adapter), GFP_KERNEL)) == NULL) {
                ALERT("failed to allocate %d x animal_adapter\n", adapters);
                goto err_alloc_adapter;
        }
        animal_dev->gpio_ops = simulator ? &simulator_gpio_ops : &animal_gpio_ops;

        /* initialize adapter handles and GPIO mutexes */
        if (simulator) {
                /* In simulator mode, install same adapter for all i2c buses */
                if (sim_adap_nr < 0) {
                        ALERT("illegal/missing adapter number for simulated "
                              "i2c bus\n");
                        goto err_get_adapter;
                }
                if (install_adapters(animal_dev, adapters, sim_adap_nr) < 0)
                        goto err_get_adapter;
        } else {
                if (install_adapters(animal_dev, adapters, -1) < 0)
                        goto err_get_adapter;                
        }

        if ((rc = i2c_add_driver(&animal_driver)) != 0)
                goto err_i2c;

        if ((rc = register_chrdev_region(dev_no, 1, ANIMAL_DEVICE_NAME)) != 0) {
                ALERT("register_chrdev_region() failed: %d\n", rc);
                goto err_chrdev;
        }

        /* init char device structure and register char device to kernel -- go "live" */
        cdev_init(&(animal_dev->cdev), &animal_fops);
        animal_dev->cdev.owner = THIS_MODULE;
        if ((rc = cdev_add(&(animal_dev->cdev), dev_no, 1)) != 0) {
                ALERT("cdev_add() failed: %d\n", rc);
                goto err_cdev_add;
        }

        INFO("init done, version %d.%d.%d%s, exporting %d i2c adapter(s)\n",
                ANIMAL_VERSION_MAJOR, ANIMAL_VERSION_MINOR, ANIMAL_VERSION_MICRO, ANIMAL_VERSION_APPEND_STR, adapters);

        return 0;

        /* stack-like clean up on error */

err_cdev_add:
        unregister_chrdev_region(dev_no, 1);
err_chrdev:
        i2c_del_driver(&animal_driver);
err_i2c:
err_get_adapter:
        for (i = 0; i < adapters; i++)
                if (animal_dev->animal_adapter[i].adapter != NULL)
                        i2c_put_adapter(animal_dev->animal_adapter[i].adapter);
        kfree(animal_dev->animal_adapter);
err_alloc_adapter:
        kfree(animal_dev);
        ALERT("init failed\n");
        return -1;
}


/**
 * Exit point from driver.
 */
static void __exit animal_exit(void)
{
        int i;
        dev_t dev_no = MKDEV(ANIMAL_DEVICE_MAJOR, ANIMAL_DEVICE_MINOR);

        INFO("driver exit\n");

        cdev_del(&(animal_dev->cdev));
        unregister_chrdev_region(dev_no, 1);

        for (i = 0; i < adapters; i++) {
                if (animal_dev->animal_adapter[i].adapter != NULL)
                        i2c_put_adapter(animal_dev->animal_adapter[i].adapter);
        }

        i2c_del_driver(&animal_driver);
        kfree(animal_dev->animal_adapter);
        kfree(animal_dev);
}


/*
 * Specify kernel module enter and exit points
 */

module_init(animal_init);
module_exit(animal_exit);

/*
 * Define the file operation supported by this device driver
 */

static struct file_operations animal_fops = {
        .owner = THIS_MODULE,
#ifdef HAVE_UNLOCKED_IOCTL
        .unlocked_ioctl = animal_ioctl,
#else
        .ioctl = animal_ioctl,
#endif
        .open = animal_open,
        .release = animal_release,
};

/*
 * Define gpio support
 */

static int animal_gpio_is_valid(int gpio)
{
        return gpio_is_valid(gpio);
}

static int animal_gpio_request(unsigned gpio, const char *label)
{
        return gpio_request(gpio, label);
}

static void animal_gpio_free(unsigned gpio)
{
        gpio_free(gpio);
}

static int animal_gpio_direction_input(unsigned gpio)
{
        return gpio_direction_input(gpio);
}

static int animal_gpio_direction_output(unsigned gpio, int value)
{
        return gpio_direction_output(gpio, value);
}

static void animal_gpio_set_value(unsigned gpio, int value)
{
        gpio_set_value(gpio, value);
}

static struct animal_gpio_operations animal_gpio_ops = {
        .is_valid           = animal_gpio_is_valid,
        .request            = animal_gpio_request,
        .free               = animal_gpio_free,
        .direction_input    = animal_gpio_direction_input,
        .direction_output   = animal_gpio_direction_output,
        .set                = animal_gpio_set_value,
};

static int simulator_gpio_is_valid(int gpio)
{
        return 1;
}

static int simulator_gpio_request(unsigned gpio, const char *label)
{
        return 0;
}

static void simulator_gpio_free(unsigned gpio)
{
}

static int simulator_gpio_direction_input(unsigned gpio)
{
        return 0;
}

static int simulator_gpio_direction_output(unsigned gpio, int value)
{
        return 0;
}

static void simulator_gpio_set_value(unsigned gpio, int value)
{
}

static struct animal_gpio_operations simulator_gpio_ops = {
        .is_valid           = simulator_gpio_is_valid,
        .request            = simulator_gpio_request,
        .free               = simulator_gpio_free,
        .direction_input    = simulator_gpio_direction_input,
        .direction_output   = simulator_gpio_direction_output,
        .set                = simulator_gpio_set_value,
};

/*
 * Specify kernel module info
 */

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Jonas Romfelt <jonas.romfelt@flir.se>");
MODULE_AUTHOR("Tommy Karlsson <tommy.karlsson@flir.se>");
MODULE_DESCRIPTION("Generic i2c/SMBus driver with gpio chip select addressing support");
