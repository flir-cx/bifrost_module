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
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/version.h>

#include <linux/uaccess.h>
#include <linux/atomic.h>

#include <asm/byteorder.h>

#include "bifrost.h"
#include "valhalla_dma.h"
#include "bifrost_platform.h"

struct bifrost_device *bdev;

/*
 * Module parameters
 */

static u32 membus;
module_param(membus, uint, 0400);
MODULE_PARM_DESC(membus, "Enable memory bus interface to FPGA (instead of PCI)");

#include <linux/mempool.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

struct bifrost_work {
	struct work_struct work;
	struct bifrost_device *bifrost;
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
	bifrost_create_event(w->bifrost, &w->event);
	mempool_free(w, work_pool);
}

void bifrost_create_event_in_atomic(struct bifrost_device *bifrost,
				    struct bifrost_event *event)
{
	struct bifrost_work *w;

	w = mempool_alloc(work_pool, GFP_ATOMIC);
	if (w == NULL) {
		ALERT("dropped event type=%d", event->type);
		return;
	}

	INIT_WORK(&w->work, work_create_event);
	w->bifrost = bifrost;
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

	if (bdev->membus) {
		/* register as real Membus driver */
		if (bifrost_membus_init(bdev) != 0) {
			goto err_pci;
		}
	} else {
		/* register as real PCI driver */
		if (bifrost_pci_init(bdev) != 0)
			goto err_pci;
	}

	if (bifrost_cdev_init(bdev))
		goto err_pci;
	INFO("init done\n");
	return 0;

err_pci:
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
	bifrost_cdev_exit(bdev);

	if (bdev->membus)
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
