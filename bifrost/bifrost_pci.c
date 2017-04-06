/*
 * Copyright (c) FLIR Systems AB.
 *
 * bifrost_pci.c
 *
 *  Created on: Mar 1, 2010
 *      Author: Jonas Romfelt <jonas.romfelt@flir.se>, Tommy Karlsson
 *
 * PCI parts of driver.
 *
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/stat.h>
#include <linux/pci.h>

#include <asm/byteorder.h>
#include <asm/atomic.h>

#include "bifrost.h"
#include "bifrost_dma.h"
#include "valhalla_msi.h"
#include "valhalla_dma.h"
#include "bifrost_platform.h"


extern void bifrost_dma_chan_start(void *data, u32 ch, u32 src, u32 dst,
                                   u32 len, u32 dir);

/* x86 doesn't define this */
#ifndef NO_IRQ
#define NO_IRQ  ((unsigned int)(-1))
#endif


#ifndef __devinitdata
#define __devinitdata
#endif

#ifndef __devinit
#define __devinit
#endif

/*
 * Using IDs from Xilinx devboard, OK since it is only for internal
 * on-board use.
 */
#define XILINX_VENDOR_ID 0x10EE
#define ALTERA_VENDOR_ID 0x1172

#define FLIR_DEVICE_ID 0x7022
#define EVANDER_DEVICE_ID 0x1002
#define ROCKY_DEVICE_ID 0x1001


static struct pci_driver bifrost_pci_driver;

struct msi_action {
        unsigned int irq;
        unsigned int msi_vec;
        unsigned long flags;
        const char *name;
        irq_handler_t handler;
        void *data;
};

static inline void *get_msi_data(void *p)
{
        return ((struct msi_action *)p)->data;
}

static inline unsigned int get_msi_vector(void *p)
{
        return ((struct msi_action *)p)->msi_vec;
}

static irqreturn_t dma_msi_handler(int irq, void *dev_id);
static irqreturn_t default_msi_handler(int irq, void *dev_id);
static irqreturn_t fvd_msi_handler(int irq, void *dev_id);

#define MSI_ENABLE(name, handler, flags) {NO_IRQ, 0, flags, name, handler, NULL}
#define MSI_DISABLED {NO_IRQ, 0, 0, "", NULL, NULL}

static int msi_interrupts = 32;

static struct msi_action msi[32] = {
        MSI_ENABLE("dma0", dma_msi_handler, 0), /* MSI vector 0 */
        MSI_ENABLE("dma1", dma_msi_handler, 0), /* MSI vector 1 */
        MSI_ENABLE("dma2", dma_msi_handler, 0), /* MSI vector 2 */
        MSI_ENABLE("dma3", dma_msi_handler, 0), /* MSI vector 3 */
        MSI_ENABLE("dma4", dma_msi_handler, 0), /* MSI vector 4 */
        MSI_ENABLE("dma5", dma_msi_handler, 0), /* MSI vector 5 */
        MSI_ENABLE("dma6", dma_msi_handler, 0), /* MSI vector 6 */
        MSI_ENABLE("dma7", dma_msi_handler, 0), /* MSI vector 7 */
        MSI_ENABLE("err", default_msi_handler, 0), /* MSI vector 8 */
        MSI_DISABLED, /* MSI vector 9 */
        MSI_ENABLE("syssync", default_msi_handler, 0), /* MSI vector 10 */
        MSI_ENABLE("clrx_sync", default_msi_handler, 0), /* MSI vector 11 */
        MSI_ENABLE("vin0_sync", default_msi_handler, 0), /* MSI vector 12 */
        MSI_ENABLE("vin1_sync", default_msi_handler, 0), /* MSI vector 13 */
        MSI_ENABLE("irig_pps", default_msi_handler, 0), /* MSI vector 14 */
        MSI_ENABLE("mdst_done", default_msi_handler, 0), /* MSI vector 15 */
        MSI_ENABLE("mdsrx6_done", default_msi_handler, 0), /* MSI vector 16 */
        MSI_ENABLE("manual_IRQ", default_msi_handler,0), /* MSI vector 17 */
        MSI_DISABLED, /* MSI vector 18 */
        MSI_DISABLED, /* MSI vector 19 */
        MSI_DISABLED, /* MSI vector 20 */
        MSI_DISABLED, /* MSI vector 21 */
        MSI_DISABLED, /* MSI vector 22 */
        MSI_DISABLED, /* MSI vector 23 */
        MSI_DISABLED, /* MSI vector 24 */
        MSI_DISABLED, /* MSI vector 25 */
        MSI_DISABLED, /* MSI vector 26 */
        MSI_DISABLED, /* MSI vector 27 */
        MSI_DISABLED, /* MSI vector 28 */
        MSI_DISABLED, /* MSI vector 29 */
        MSI_DISABLED, /* MSI vector 30 */
        MSI_DISABLED, /* MSI vector 31 */
};

static struct msi_action msi_fvd[32] = {
        MSI_ENABLE("dma0", dma_msi_handler, 0), /* MSI vector 0 */
        MSI_DISABLED, /* MSI vector 1 */
        MSI_DISABLED, /* MSI vector 2 */
        MSI_DISABLED, /* MSI vector 3 */
        MSI_DISABLED, /* MSI vector 4 */
        MSI_DISABLED, /* MSI vector 5 */
        MSI_DISABLED, /* MSI vector 6 */
        MSI_DISABLED, /* MSI vector 7 */
        MSI_ENABLE("fvd_interrupt", fvd_msi_handler, 0), /* MSI vector 8 */
        MSI_DISABLED, /* MSI vector 9 */
        MSI_DISABLED, /* MSI vector 10 */
        MSI_DISABLED, /* MSI vector 11 */
        MSI_DISABLED, /* MSI vector 12 */
        MSI_DISABLED, /* MSI vector 13 */
        MSI_DISABLED, /* MSI vector 14 */
        MSI_DISABLED, /* MSI vector 15 */
        MSI_DISABLED, /* MSI vector 16 */
        MSI_DISABLED, /* MSI vector 17 */
        MSI_DISABLED, /* MSI vector 18 */
        MSI_DISABLED, /* MSI vector 19 */
        MSI_DISABLED, /* MSI vector 20 */
        MSI_DISABLED, /* MSI vector 21 */
        MSI_DISABLED, /* MSI vector 22 */
        MSI_DISABLED, /* MSI vector 23 */
        MSI_DISABLED, /* MSI vector 24 */
        MSI_DISABLED, /* MSI vector 25 */
        MSI_DISABLED, /* MSI vector 26 */
        MSI_DISABLED, /* MSI vector 27 */
        MSI_DISABLED, /* MSI vector 28 */
        MSI_DISABLED, /* MSI vector 29 */
        MSI_DISABLED, /* MSI vector 30 */
        MSI_DISABLED, /* MSI vector 31 */
};
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)

static ssize_t show_read_speed_avg(struct device *dev, struct device_attribute *attr,
                         char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%lu kB/s \n", bdev->stats.read_speed_avg/(1024));
}
static ssize_t show_write_speed_avg(struct device *dev, struct device_attribute *attr,
                         char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%lu kB/s \n", bdev->stats.write_speed_avg/(1024));
}
static ssize_t show_read_speed_last(struct device *dev, struct device_attribute *attr,
                         char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%lu kB/s \n", bdev->stats.read_speed_last/(1024));
}
static ssize_t show_write_speed_last(struct device *dev, struct device_attribute *attr,
                         char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%lu kB/s \n", bdev->stats.write_speed_last/(1024));
}

static ssize_t show_write_b(struct device *dev, struct device_attribute *attr,
                         char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%lu kB\n", bdev->stats.write_b/1024);
}

static ssize_t show_read_b(struct device *dev, struct device_attribute *attr,
                         char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%lu kB\n", bdev->stats.read_b/1024);
}

static ssize_t show_enable(struct device *dev, struct device_attribute *attr,
                         char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", bdev->stats.enabled);
}
static ssize_t store_enable(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t count)
{
	unsigned long val;
	if (kstrtoul(buf, 0, &val) < 0)
		return -EINVAL;

	bdev->stats.enabled = !!val;
	return count;
}

static DEVICE_ATTR(read_b, S_IRUGO, show_read_b, NULL);
static DEVICE_ATTR(write_b, S_IRUGO, show_write_b, NULL);
static DEVICE_ATTR(read_speed_last, S_IRUGO, show_read_speed_last, NULL);
static DEVICE_ATTR(write_speed_last, S_IRUGO, show_write_speed_last, NULL);
static DEVICE_ATTR(read_speed_avg, S_IRUGO, show_read_speed_avg, NULL);
static DEVICE_ATTR(write_speed_avg, S_IRUGO, show_write_speed_avg, NULL);
static DEVICE_ATTR(stats_enable, S_IWUSR | S_IRUGO , show_enable, store_enable);

static struct attribute *bifrost_attrs[] = {
	&dev_attr_read_speed_last.attr,
	&dev_attr_write_speed_last.attr,
    &dev_attr_read_speed_avg.attr,
	&dev_attr_write_speed_avg.attr,
 	&dev_attr_read_b.attr,
	&dev_attr_write_b.attr,
	&dev_attr_stats_enable.attr,
	NULL
};

static const struct attribute_group bifrost_groups = {
	.attrs	= bifrost_attrs,
};
#endif

int bifrost_simulate_msi(unsigned int msi_vec)
{
        struct msi_action *m;

        if (msi_vec >= msi_interrupts)
                return -EINVAL; /* Too big MSI vector */

        m = &msi[msi_vec];
        if (m->handler == NULL)
                return -EINVAL; /* MSI vector not setup */

        return m->handler(m->irq, m);
}

static int request_msi(struct msi_action *m, int hw_irq, int vec, void *data)
{
        /*
         * MSI IRQ numbers, see arch/arm/plat-omap/include/plat/irqs.h
         */
        m->msi_vec = vec;
        m->irq = hw_irq + vec;
        m->data = data;

        if (bdev->info.simulator)
                return 0;

        return request_irq(m->irq, m->handler, m->flags, m->name, m);
}

int bifrost_attach_msis_to_irq(int hw_irq, struct bifrost_device *dev)
{
        int n, v;
        if(platform_fvd())
            memcpy(msi,msi_fvd,msi_interrupts * sizeof(struct msi_action));

        for (n = 0; n < msi_interrupts; n++) {
                if (msi[n].handler == NULL)
                        continue; /* No handler defined for this MSI */
                if (msi[n].irq != NO_IRQ)
                        continue; /* MSI already setup else where */

                /* Note: All MSI interrupts are OR:ed into hw_irq! */
                v = request_msi(&msi[n], hw_irq, n, dev);
                if (v != 0) {
                        ALERT("failed to attach MSI%u to IRQ%u, errno=%d\n",
                              msi[n].msi_vec, msi[n].irq, v);
                        msi[n].irq = NO_IRQ;
                }
        }

        return 0;
}

void bifrost_detach_msis(void)
{
        int n;

        for (n = 0; n < ARRAY_SIZE(msi); n++) {
                if (msi[n].irq != NO_IRQ) {
                        if (!bdev->info.simulator)
                                free_irq(msi[n].irq, &msi[n]);
                        msi[n].irq = NO_IRQ;
                }
        }
}

int bifrost_dma_init(int irq, struct bifrost_device *dev)
{
        int n, num_ch, idle_map;

	if (dev->info.simulator) {
		num_ch = 1;
		idle_map = 1;
	} else {
        struct device_memory *mem;
        u32 val;

        if(platform_rocky())
            dev->regb_dma = &dev->regb[3];
        else if(platform_evander())
            dev->regb_dma = &dev->regb[2];
        else
            dev->regb_dma = &dev->regb[0];
        mem = dev->regb_dma;

		mem->rd(mem->handle, VALHALLA_ADDR_DMA_CAPABILITY, &val);
		num_ch = val & 0xf;

		mem->rd(mem->handle, VALHALLA_ADDR_DMA_STATUS, &val);
		idle_map = ~val & ((1 << num_ch) - 1);
	}
        dev->dma_ctl = alloc_dma_ctl(num_ch, idle_map, bifrost_dma_chan_start,
				dev);
        if (dev->dma_ctl == NULL)
                return -ENOMEM;

        /*
         * Note: this requires that all DMA interrupts are consecutive and
         *       starting at irq
         */
        for (n = 0; n < num_ch; n++) {
                enable_dma_ch(dev->dma_ctl, n, irq + n);
        }
        return 0;
}

void bifrost_dma_cleanup(struct bifrost_device *dev)
{
        int n, num_ch;

        if (dev->dma_ctl == NULL)
                return;

        num_ch = (dev->info.simulator) ? 1 : 8;

        for (n = 0; n < num_ch; n++)
                disable_dma_ch(dev->dma_ctl, n);
        free_dma_ctl(dev->dma_ctl);
}

/**
 * Initialize PCI parts of driver. The probe function will be called by the
 * Linux PCI framework and takes care of most device initializations.
 */
int __init bifrost_pci_init(struct bifrost_device *dev)
{
        int rc;

        INFO("\n");

        rc = pci_register_driver(&bifrost_pci_driver);
        if (rc) {
                ALERT("Registering PCI driver failed: %d\n", rc);
                return rc;
        }

        return 0;
}

/**
 * Clean up PCI parts of driver
 */
void bifrost_pci_exit(struct bifrost_device *dev)
{
        INFO("\n");
        pci_unregister_driver(&bifrost_pci_driver);
}

static irqreturn_t fvd_msi_handler(int irq, void *dev_id)
{
     struct bifrost_device *dev = get_msi_data(dev_id);
     return FVDInterruptService(irq,dev);

}

static irqreturn_t dma_msi_handler(int irq, void *dev_id)
{
        struct bifrost_device *dev;
        struct bifrost_event event;
        unsigned int ticket;
        void *cookie;
        s64 xfer_time;

        dev = get_msi_data(dev_id);

        cookie = dma_done(dev->dma_ctl, irq, &ticket, &xfer_time,dev);
        if (!IS_ERR(cookie)) {
                event.type = BIFROST_EVENT_TYPE_DMA_DONE;
                event.data.dma.id = ticket;
                event.data.dma.time = xfer_time;
                event.data.dma.cookie = (u64)(unsigned long)cookie;
                bifrost_create_event_in_atomic(dev, &event);
        }

        return IRQ_HANDLED;
}

static irqreturn_t default_msi_handler(int irq, void *dev_id)
{
        struct bifrost_device *dev = get_msi_data(dev_id);
        unsigned int vec = get_msi_vector(dev_id);
        struct bifrost_event event;

        if (!dev)
                return IRQ_NONE;

        event.type = BIFROST_EVENT_TYPE_IRQ;
        event.data.irq_source = map_msi_to_event(vec);
        bifrost_create_event_in_atomic(dev, &event);

        return IRQ_HANDLED;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,33))
static inline int pci_pcie_cap(struct pci_dev *dev)
{
	int pos;

	/* PCI express device capabilities */
	pos = pci_find_capability(dev, PCI_CAP_ID_EXP);
	return (pos != 0) ? pos : -EINVAL;
}
#endif

/*
 * Note: this function is implemented in the 3.1 version of the kernel,
 *       so remove this one when we are using kernel version 3.1 and
 *       later... but it is not exported at this moment.
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0))
static
#endif
int pcie_set_mps(struct pci_dev *dev, int pl)
{
        int cap, err = -EINVAL;
        u16 ctl, v;

        if (pl < 128 || pl > 4096 || !is_power_of_2(pl))
                goto out;

        v = (ffs(pl) - 8) << 5;

        cap = pci_pcie_cap(dev);
        if (!cap)
                goto out;

        err = pci_read_config_word(dev, cap + PCI_EXP_DEVCTL, &ctl);
        if (err)
                goto out;

        if ((ctl & PCI_EXP_DEVCTL_PAYLOAD) != v) {
                ctl &= ~PCI_EXP_DEVCTL_PAYLOAD;
                ctl |= v;
                err = pci_write_config_dword(dev, cap + PCI_EXP_DEVCTL, ctl);
        }

  out:
        return err;
}

static int pcie_set_ext_tag(struct pci_dev *dev)
{
        int cap, err = -EINVAL;
        u16 ctl;

        cap = pci_pcie_cap(dev);
        if (!cap)
                goto out;

        err = pci_read_config_word(dev, cap + PCI_EXP_DEVCTL, &ctl);
        if (err)
                goto out;

        if (!(ctl & PCI_EXP_DEVCTL_EXT_TAG)) {
                ctl |= PCI_EXP_DEVCTL_EXT_TAG;
                err = pci_write_config_dword(dev, cap + PCI_EXP_DEVCTL, ctl);
        }

  out:
        return err;
}

static int write_device_memory(void *handle, u32 offset, u32 value)
{
        struct device_memory *mem = handle;
#if 1
        iowrite32(cpu_to_le32(value), mem->addr + offset);
#else
        iowrite32(cpu_to_be32(value), mem->addr + offset);
#endif
        return 0;
}

static int read_device_memory(void *handle, u32 offset, u32 *value)
{
        struct device_memory *mem = handle;
        u32 v;

        v = ioread32(mem->addr + offset);
#if 1
        *value = le32_to_cpu(v);
#else
        *value be32_to_cpu(v);
#endif
        return 0;
}

static int set_mode_device_memory(void *handle, u32 offset, u32 mode)
{
        return 0; /* A no-op on HW */
}

static void init_device_memory(struct pci_dev *pci, int n,
                               struct device_memory *mem)
{
        memset(mem, 0, sizeof(*mem));

        mem->bar = n;
        mem->pdev = pci;
        mem->addr_bus = pci_resource_start(pci, n);
        mem->flags = pci_resource_flags(pci, n);
        mem->size = pci_resource_len(pci, n);
        mem->handle = mem;
        mem->rd = read_device_memory;
        mem->wr = write_device_memory;
        mem->mset = set_mode_device_memory;
        spin_lock_init(&mem->lock);
}

static int map_device_memory(struct device_memory *mem)
{
        if (mem->size == 0)
                return -EINVAL; /* Nothing to I/O map */

        mem->addr = pci_iomap(mem->pdev, mem->bar, mem->size);
        if (mem->addr == NULL) {
                ALERT("Map BAR%d region failed (%08lx - %08lx\n",
                      mem->bar, mem->addr_bus, mem->addr_bus + mem-> size - 1);
                return -EFAULT;
        }

        mem->enabled = 1;
        INFO("BAR%d %08lx - %08lx => %08lx - %08lx (%lu KByte)\n",
             mem->bar, mem->addr_bus, mem->addr_bus + mem->size - 1,
             (unsigned long)mem->addr, (unsigned long)mem->addr + mem->size - 1,
             mem->size / 1024);

        return 0;
}

static void unmap_device_memory(struct device_memory *mem)
{
        if (mem->enabled)
                pci_iounmap(mem->pdev, mem->addr);

        mem->enabled = 0;
}

static int setup_io_regions(struct bifrost_device *bifrost,
                            struct pci_dev *pci)
{
        int n, nbars;

        for (nbars = 0, n = 0; n < ARRAY_SIZE(bifrost->regb); n++) {
                init_device_memory(pci, n, &bifrost->regb[n]);
                if (map_device_memory(&bifrost->regb[n]) == 0)
                        nbars++;
        }

        return nbars;
}

static void remove_io_regions(struct bifrost_device *bifrost)
{
        int n;

        for (n = 0; n < ARRAY_SIZE(bifrost->regb); n++)
                unmap_device_memory(&bifrost->regb[n]);
}

int __devinit bifrost_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
        int rc;

        INFO("probing PCI\n");

        /*
         * Ask low-level PCI layers to initialize device, enable I/O and
         * memory regions
         */
        rc = pci_enable_device(pdev);
        if (rc) {
                ALERT("Enable device failed: %d\n", rc);
                return -ENODEV;
        }
        /*
         * Enable bus master capability on device to allow device DMA to/from
         * system memory
         */
        pci_set_master(pdev);
        rc = pcie_set_readrq(pdev, 256);
        if (rc) {
                ALERT("Setting max read request failed: %d\n", rc);
                return -ENODEV;
        }
        rc = pcie_set_mps(pdev, 128);
        if (rc) {
                ALERT("Setting max payload failed: %d\n", rc);
                return -ENODEV;
        }
        rc = pcie_set_ext_tag(pdev);
        if (rc) {
                ALERT("Setting max payload failed: %d\n", rc);
                return -ENODEV;
        }

        /* Enable message signaled interrupts (MSI) */
        if(platform_fvd())
            msi_interrupts = 1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
        rc = pci_enable_msi_block(pdev,msi_interrupts);
#else
        rc = pci_enable_msi(pdev);
#endif
        if (rc<0) {
                ALERT("Enable MSI failed: %d\n", rc);
                return -ENODEV;
        }

        /* DMA mask should be 32-bit by default, prior 2.6.24
         * use DMA_32BIT_MASK */
        if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(32)) &&
            !pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32)))
                INFO("Using 32-bit DMA mask\n");
        else
                ALERT("No suitable DMA mask available\n");

        /* Claim device and allocate I/O memory region */
        rc = pci_request_regions(pdev, BIFROST_DEVICE_NAME);
        if (rc) {
                ALERT("Request regions failed: %d\n", rc);
                goto err_pci_request;
        }
        /* Gain CPU access to FPGA register map */
        rc = setup_io_regions(bdev, pdev);
        if (rc < 2)
                goto err_pci_iomap_regb;

        /*Patch BAR0 memory read/write functions to use 16 bit accesses*/
        if(platform_fvd())
        {
            bdev->regb[0].wr = membus_write_device_memory;
            bdev->regb[0].rd = membus_read_device_memory;
        }

        /* Run post init and make driver accessible */
        if (bifrost_pci_probe_post_init(pdev) != 0)
                goto err_pci_post_init; 
        /*
         * Add sysfs group
        */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
        rc = sysfs_create_group(&pdev->dev.kobj, &bifrost_groups);
        if (rc) {
                ALERT("failed to add sys fs entry\n");
                goto err_pci_sysfs;
        }
#endif

        /* store private data pointer */
        pci_set_drvdata(pdev, NULL);

        bdev->pdev = pdev;

        return 0;

        /* stack-like cleanup on error */
 err_pci_sysfs:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
        sysfs_remove_group(&pdev->dev.kobj, &bifrost_groups);
#endif
  err_pci_post_init:
        remove_io_regions(bdev);
  err_pci_iomap_regb:
        pci_release_regions(pdev);
  err_pci_request:
        pci_disable_device(pdev);
        bdev->pdev = NULL;
        return -ENODEV;
}

void bifrost_pci_remove(struct pci_dev *pdev)
{
        INFO("removing PCI\n");
        if(platform_fvd())
            bifrost_fvd_exit(bdev);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
        sysfs_remove_group(&pdev->dev.kobj, &bifrost_groups);
#endif
        bifrost_detach_msis();
        pci_disable_msi(pdev);
        remove_io_regions(bdev);
        pci_iounmap(pdev, bdev->ddr.addr);
        pci_release_regions(pdev);
        pci_disable_device(pdev);
        pci_set_drvdata(pdev, NULL);
}

struct pci_device_id bifrost_pci_device_table[] __devinitdata = {

        { PCI_DEVICE(XILINX_VENDOR_ID, FLIR_DEVICE_ID) },
        { PCI_DEVICE(ALTERA_VENDOR_ID, ROCKY_DEVICE_ID) },
        { PCI_DEVICE(XILINX_VENDOR_ID, EVANDER_DEVICE_ID) },
        { 0, },
    };


static struct pci_driver bifrost_pci_driver = {
        .name = BIFROST_DEVICE_NAME,
        .probe = bifrost_pci_probe,
        .remove = bifrost_pci_remove,
        .id_table = bifrost_pci_device_table,
};

MODULE_DEVICE_TABLE(pci, bifrost_pci_device_table);
