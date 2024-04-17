// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) FLIR Systems AB.
 *
 *  Created on: Mar 1, 2010
 *	Author: Jonas Romfelt <jonas.romfelt@flir.se>
 *
 * Main driver file that implements entry and exit points.
 *
 */

#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
#include <linux/uaccess.h>
#include <linux/atomic.h>
#else
#include <asm/uaccess.h>
#include <asm/atomic.h>
#endif

#include <asm/byteorder.h>

#include "bifrost.h"
#include "valhalla_dma.h"
#include "bifrost_platform.h"

struct bifrost_device *bdev;

/*
 * Module parameters
 */

static u32 simulator;
module_param(simulator, uint, 0400);
MODULE_PARM_DESC(simulator, "Enable simulator interface");

static u32 membus;
module_param(membus, uint, 0400);
MODULE_PARM_DESC(membus, "Enable memory bus interface to FPGA (instead of PCI)");
#if KERNEL_VERSION(3, 10, 0) > LINUX_VERSION_CODE
static int device_mem_info(struct bifrost_device *dev, char *buf, size_t bufsz)
{
	int bar, len, n;
	struct device_memory *mem;

	for (len = 0, bar = 0; bar < ARRAY_SIZE(dev->regb); bar++) {
		mem = &dev->regb[bar];
		if (mem->enabled)
			n = snprintf(buf + len,
				     bufsz - len,
				     "BAR%d: bus-addr=%08lx cpu-addr=%p "
				     "size=%lu KByte\n",
				     bar,
				     mem->addr_bus,
				     mem->addr,
				     mem->size / 1024);
		else
			n = snprintf(buf + len,
				     bufsz - len,
				     "BAR%d: disabled\n",
				     bar);
		if (n < 0)
			return len;
		len += n;
	}
	return len;
}

/**
 * Put data into the proc fs file.
 *
 * @param page Where to start write data
 * @param start Where in page
 * @param offset Offset in page that read is done
 * @param page_size Size of page (bytes)
 * @param eof Set to 1 to signal EOF
 * @return 0 to indicate that there is no more data to read.
 */
static int bifrost_procfs_read(char *page, char **start, off_t offset,
			       int page_size, int *eof, void *data)
{
	struct bifrost_device *dev = (struct bifrost_device *)data;
	int len;

	INFO("read /proc/%s\n", BIFROST_DEVICE_NAME);
	dev->stats.procfsreads++;

	/*
	 * Driver gives all information in one go, hence if user requests
	 * more information return EOF.
	 */
	if (offset > 0) {
		*eof = 1;
		return 0;
	}

	len = snprintf(page, page_size,
		       "Device name		 : %s\n"
		       "Driver version		 : %d.%d.%d%s\n"
		       "Build timestamp		 : "__DATE__" "__TIME__"\n"
		       "Mode			 : %s\n"
		       "Interface		 : %s\n",
		       BIFROST_DEVICE_NAME,
		       BIFROST_VERSION_MAJOR,
		       BIFROST_VERSION_MINOR,
		       BIFROST_VERSION_MICRO,
		       BIFROST_VERSION_APPEND_STR,
		       dev->info.simulator == 1 ? "simulator" : "normal",
		       dev->membus == 1 ? "memory bus" : "PCIe");

	len += device_mem_info(dev, &page[len], page_size - len);

	len += snprintf(&page[len], page_size - len,
			"Open ops.		  : %ld\n"
			"Release ops.		  : %ld\n"
			"Poll ops.		  : %ld\n"
			"Read buffer size	  : %d\n"
			"Read buffer head	  : %d\n"
			"Read buffer tail	  : %d\n"
			"Read buffer overruns	  : %d\n"
			"Read ops.		  : %ld\n"
			"Read bytes		  : %ld\n"
			"Write ops.		  : %ld\n"
			"Written bytes		  : %ld\n"
			"Ioctl ops.		  : %ld\n"
			"Lseek ops.		  : %ld\n"
			"Proc fs reads		  : %ld\n",
			dev->stats.opens,
			dev->stats.releases,
			dev->stats.polls,
			CIRCULAR_BUFFER_SIZE,
			0, 0, 0,
			dev->stats.reads,
			dev->stats.read_b,
			dev->stats.writes,
			dev->stats.write_b,
			dev->stats.ioctls,
			dev->stats.llseeks,
			dev->stats.procfsreads);
	*eof = 1;
	return len;
}
#endif

void bifrost_dma_chan_start(void *data, u32 ch, u32 src, u32 dst, u32 len,
			    u32 dir)
{
	struct bifrost_device *dev = data;
	struct device_memory *mem = dev->regb_dma;
	unsigned long flags;

	spin_lock_irqsave(&mem->lock, flags);

	mem->wr(mem->handle, VALHALLA_ADDR_DMA_CHAN, ch);
	smp_wmb();
	mem->wr(mem->handle, VALHALLA_ADDR_DMA_SRC_ADDR, src);
	mem->wr(mem->handle, VALHALLA_ADDR_DMA_DEST_ADDR, dst);
	mem->wr(mem->handle, VALHALLA_ADDR_DMA_DIR_UP_STRM, dir);
	mem->wr(mem->handle, VALHALLA_ADDR_DMA_LEN_BYTES, len);
	smp_wmb();
	mem->wr(mem->handle, VALHALLA_ADDR_DMA_START, 1);

	spin_unlock_irqrestore(&mem->lock, flags);
}

/**
 * Post init handles initialization of driver interfaces (frame buffer and
 * character device) and the call to this function depends on whether driver
 * is loaded with or without PCI support.
 *
 * Call this function last in during initialization because the driver will
 * go "live" and allow access as soon as this function returns.
 */
int bifrost_pci_probe_post_init(struct pci_dev *pdev)
{
	int rc = 0, msi_irq;

	INFO("\n");

	msi_irq = pdev ? pdev->irq : SIM_MSI_IRQ;
	rc = bifrost_attach_msis_to_irq(msi_irq, bdev);
	if (rc < 0) {
		ALERT("Failed to attch MSI vectors\n");
		return rc;
	}

	rc = bifrost_dma_init(msi_irq, bdev);
	if (rc < 0) {
		ALERT("Failed to setup DMA\n");
		goto err_dma;
	}

	if (platform_fvd() && bifrost_fvd_init(bdev) != 0)
		goto err_alloc;


	return 0;

err_alloc:
	bifrost_dma_cleanup(bdev);
err_dma:
	bifrost_detach_msis();
	return -1;
}

#include <linux/mempool.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

struct bifrost_work {
	struct work_struct work;
	struct bifrost_device *dev;
	struct bifrost_event event;
};

static mempool_t *work_pool;
static struct workqueue_struct *work_queue;

static void *mempool_alloc_work(gfp_t flags, void *pool_data)
{
	(void)pool_data;
	return kmalloc(sizeof(struct bifrost_work), flags);
}

static void mempool_free_work(void *work, void *pool_data)
{
	(void)pool_data;
	kfree(work);
}

static void work_create_event(struct work_struct *work)
{
	struct bifrost_work *w;

	w = container_of(work, struct bifrost_work, work);
	bifrost_create_event(w->dev, &w->event);
	mempool_free(w, work_pool);
}

void bifrost_create_event_in_atomic(struct bifrost_device *dev,
				    struct bifrost_event *event)
{
	struct bifrost_work *w;

	w = mempool_alloc(work_pool, GFP_ATOMIC);
	if (w == NULL) {
		ALERT("dropped event type=%d", event->type);
		return;
	}

	INIT_WORK(&w->work, work_create_event);
	w->dev = dev;
	memcpy(&w->event, event, sizeof(*event));
	queue_work(work_queue, &w->work);
}

/*
 * Entry point to driver.
 */
static int __init bifrost_init(void)
{
	int ret;

	bdev = kzalloc(sizeof(struct bifrost_device), GFP_KERNEL);
	if (bdev == NULL) {
		ALERT("failed to allocate BIFROST_DEVICE\n");
		return -ENOMEM;
	}

	bdev->info.version.major = BIFROST_VERSION_MAJOR;
	bdev->info.version.minor = BIFROST_VERSION_MINOR;
	bdev->info.version.revision = BIFROST_VERSION_MICRO;

	if (simulator > 0)
		bdev->info.simulator = 1;

	INFO("simulator interface %s\n",
	     bdev->info.simulator == 0 ? "disabled" : "enabled");

	if (membus > 0)
		bdev->membus = 1;

	INFO("FPGA interface %s\n",
	     bdev->membus == 0 ? "PCIe" : "memory bus");

	INIT_LIST_HEAD(&bdev->list);
	spin_lock_init(&bdev->lock_list);

	work_pool = mempool_create(20, mempool_alloc_work, mempool_free_work, NULL);
	if (work_pool == NULL) {
		ret = -ENOMEM;
		goto err_alloc_pool;
	}

	work_queue = create_singlethread_workqueue("bifrost");
	if (work_queue == NULL) {
		ret = -ENOMEM;
		goto err_alloc_queue;
	}

#if KERNEL_VERSION(3, 10, 0) > LINUX_VERSION_CODE
	/*
	 * Setup /proc file system entry. Create a read-only entry in
	 * proc root with a file name same as the device (use NULL as
	 * parent dir)
	 */
	bdev->proc = create_proc_read_entry(BIFROST_DEVICE_NAME, 0, NULL,
					    bifrost_procfs_read, bdev);
	if (bdev->proc == NULL) {
		ALERT("failed to add proc fs entry\n");
		goto err_alloc_queue;
	}
#endif

	if (bdev->info.simulator) {
		if (bifrost_sim_pci_init(bdev) < 0)
			goto err_pci;

		/* Continue to initialize driver without real PCI support */
		if (bifrost_pci_probe_post_init(NULL) != 0) {
			bifrost_sim_pci_exit(bdev);
			goto err_pci;
		}

		if (bifrost_cdev_init(bdev))
			goto err_pci;

	} else if (bdev->membus) {
		/* Simulator interface disabled, register as real Membus driver */
		if (bifrost_membus_init(bdev) != 0) {
			bifrost_membus_exit(bdev);
			goto err_pci;
		}
	} else {

		/* Simulator interface disabled, register as real PCI driver */
		if (bifrost_pci_init(bdev) != 0)
			goto err_pci;

		if (bifrost_cdev_init(bdev))
			goto err_pci;
	}

	INFO("init done\n");
	return 0;

err_pci:
#if KERNEL_VERSION(3, 10, 0) > LINUX_VERSION_CODE
	remove_proc_entry(BIFROST_DEVICE_NAME, NULL);
#endif
	flush_workqueue(work_queue);
	destroy_workqueue(work_queue);
err_alloc_queue:
	mempool_destroy(work_pool);
err_alloc_pool:
	kfree(bdev);

	ALERT("init failed\n");
	return ret;
}

/**
 * Exit point from driver.
 */
static void __exit bifrost_exit(void)
{
	INFO("exit\n");
#if KERNEL_VERSION(3, 10, 0) > LINUX_VERSION_CODE
	remove_proc_entry(BIFROST_DEVICE_NAME, NULL);
#endif
	bifrost_cdev_exit(bdev);
	bifrost_dma_cleanup(bdev);

	if (bdev->info.simulator)
		bifrost_sim_pci_exit(bdev);
	else if (bdev->membus)
		bifrost_membus_exit(bdev);
	else
		bifrost_pci_exit(bdev);

	flush_workqueue(work_queue);
	destroy_workqueue(work_queue);
	mempool_destroy(work_pool);
	kfree(bdev);
}

/*
 * Specify kernel module enter and exit points
 */

module_init(bifrost_init);
module_exit(bifrost_exit);

/*
 * Specify kernel module info
 */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jonas Romfelt <jonas.romfelt@flir.se>");
MODULE_AUTHOR("Tommy Karlsson <tommy.karlsson@flir.se>");
MODULE_DESCRIPTION("FLIR FPGA driver");
