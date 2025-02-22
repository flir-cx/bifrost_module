// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) FLIR Systems AB.
 *
 *  Created on: April 16, 2014
 *	Author: Peter Fitger <peter.fitger@flir.se>
 *
 * Memory bus parts of driver.
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
#include <linux/platform_device.h>

#include <asm/byteorder.h>
#include <asm/atomic.h>
#include <asm/gpio.h>
#include <linux/gpio.h>

#include "bifrost.h"
#include "bifrost_dma.h"
#include "valhalla_msi.h"
#include "valhalla_dma.h"

/* FIXME: This should have a ifdef for kernel version to get sizes. I don't know when this changed. */
#ifndef SZ_4K
#include <asm-generic/sizes.h>
#endif

#define FPGA_MEM_TIME	30    // Time in us from address write to memory access
#define FPGA_WR_BIT         (0x8000)  // Write-enable bit for address-hi
#define FPGA_RD_BIT         (0x0000)
#define FPGA_ADDR_HI(a) ((a >> 16) & (~FPGA_WR_BIT))
#define FPGA_ADDR_LO(a) (a & 0xFFFE)  // Do not allow odd addresses

static irqreturn_t FVDIRQ1Service(int irq, void *dev_id);
static irqreturn_t FVDIRQ2Service(int irq, void *dev_id);

#define FPGA_IRQ_0	((3-1)*32 + 16)
#define FPGA_IRQ_1	((3-1)*32 + 17) // GPIO3.17
#define FPGA_IRQ_2	((3-1)*32 + 18) // GPIO3.18


// NOTE! - This function must perform 8-bytes (4 16 bit words) bursts to FPGA (bug in iMX6)
static void fpgaread(u32 dst_addr, void __iomem *src, u32 len)
{
	u16 *dst = (u16 *)dst_addr;

	len >>= 1;
	ioread16_rep(src, dst, len);
}

// NOTE! - This function must perform 8-bytes (4 16 bit words) bursts to FPGA (bug in iMX6)
static void fpgawrite(void  __iomem *dst, u32 src_addr, u32 len)
{
	u16 *src = (u16 *)src_addr;

	len >>= 1;
	iowrite16_rep(dst, src, len);
}

int membus_write_device_memory(void *handle, u32 offset, u32 value)
{
	struct device_memory *mem = handle;

	iowrite16(cpu_to_le16(value), mem->addr + (offset << 1));
	return 0;
}

int membus_read_device_memory(void *handle, u32 offset, u32 *value)
{
	struct device_memory *mem = handle;
	u32 v;

	v = ioread16(mem->addr + (offset << 1));
	*value = le16_to_cpu(v);
	return 0;
}

static int set_mode_device_memory(void *handle, u32 offset, u32 mode)
{
	return 0; /* A no-op on HW */
}

static void init_device_memory(int n, struct device_memory *mem)
{
	memset(mem, 0, sizeof(*mem));

	mem->bar = n;
	mem->addr_bus = 0;
	mem->flags = 0;
	mem->size = SZ_4K;
	mem->handle = mem;
	mem->rd = membus_read_device_memory;
	mem->wr = membus_write_device_memory;
	mem->mset = set_mode_device_memory;
	spin_lock_init(&mem->lock);
	mutex_init(&mem->iolock);
}

static int map_device_memory(struct device_memory *mem)
{
	if (mem->size == 0)
		return -EINVAL; /* Nothing to I/O map */
	if (mem->bar >= 2)
		return -EINVAL; /* Only two chip selects active */

#ifndef CS0_BASE_ADDR
#if KERNEL_VERSION(3, 10, 0) <= LINUX_VERSION_CODE
#define CS0_BASE_ADDR 0x08000000
#else
#define CS0_BASE_ADDR 0
	ALERT("CS0_BASE_ADDR undefined\n");
#endif
#endif

	mem->addr_bus = CS0_BASE_ADDR + (mem->bar * 0x02000000);
#if KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE
	mem->addr = ioremap(mem->addr_bus, SZ_4K);
#else
	mem->addr = ioremap_nocache(mem->addr_bus, SZ_4K);
#endif
	if (mem->addr == NULL) {
		ALERT("Map BAR%d region failed (%08lx - %08lx)\n",
		      mem->bar, mem->addr_bus, mem->addr_bus + mem->size - 1);
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
		iounmap(mem->addr);

	mutex_destroy(&mem->iolock);
	mem->enabled = 0;
}

static int setup_io_regions(struct bifrost_device *bifrost)
{
	int n, nbars;

	for (nbars = 0, n = 0; n < ARRAY_SIZE(bifrost->regb); n++) {
		init_device_memory(n, &bifrost->regb[n]);
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

int __init bifrost_membus_init(struct bifrost_device *bifrost)
{
	int rc;

	INFO("\n");

	/* Gain CPU access to FPGA register map */
	rc = setup_io_regions(bdev);
	if (rc < 2)
		goto err_ioregions;

	if (bifrost_fvd_init(bifrost) != 0)
		goto err_ioregions;

	return 0;

	/* stack-like cleanup on error */
err_ioregions:
	remove_io_regions(bdev);
	return -ENODEV;
}

int  bifrost_fvd_init(struct bifrost_device *bifrost)
{
	int rc;
	struct device *dev;

	// Register platform driver
	bdev->pMemDev = platform_device_alloc(BIFROST_DEVICE_NAME, 1);
	if (bdev->pMemDev == NULL) {
		ALERT("Registering MEMBUS driver failed\n");
		goto err_platform_alloc;
	}
	platform_device_add(bdev->pMemDev);

	dev = &bdev->pMemDev->dev;
	bdev->dev = dev;
	dev_set_drvdata(dev, bifrost);
	platform_set_drvdata(bdev->pMemDev, bifrost);

	if (gpio_is_valid(FPGA_IRQ_0) == 0) {
		ALERT("FPGA_IRQ_0 can not be used\n");
		goto err_gpio;
	}

	rc = devm_gpio_request(dev, FPGA_IRQ_0, "FpgaIrq0");
	rc = gpio_direction_input(FPGA_IRQ_0);

	rc = devm_request_irq(dev, gpio_to_irq(FPGA_IRQ_0), FVDInterruptService,
			 IRQF_TRIGGER_FALLING, "FpgaIrq0", bdev);

	if (rc) {
		ALERT("Failed to request FPGA IRQ (%d)\n", rc);
		goto err_gpio;
	} else {
		INFO("Successfully requested FPGA IRQ\n");
	}

	if (gpio_is_valid(FPGA_IRQ_1) == 0) {
		ALERT("FPGA_IRQ_1 can not be used\n");
		goto err_gpio;
	}

	rc = devm_gpio_request(dev, FPGA_IRQ_1, "FpgaIrq1");
	rc = gpio_direction_input(FPGA_IRQ_1);

	rc = devm_request_irq(dev, gpio_to_irq(FPGA_IRQ_1), FVDIRQ1Service,
			 IRQF_TRIGGER_FALLING, "FpgaIrq1", bdev);

	if (rc) {
		ALERT("Failed to request FPGA IRQ1 (%d)\n", rc);
		goto err_gpio;
	} else {
		INFO("Successfully requested FPGA IRQ1\n");
	}

	if (gpio_is_valid(FPGA_IRQ_2) == 0) {
		ALERT("FPGA_IRQ_2 can not be used\n");
		goto err_gpio;
	}

	devm_gpio_request(dev, FPGA_IRQ_2, "FpgaIrq2");
	gpio_direction_input(FPGA_IRQ_2);

	rc = devm_request_irq(dev, gpio_to_irq(FPGA_IRQ_2), FVDIRQ2Service,
			 IRQF_TRIGGER_FALLING, "FpgaIrq2", bdev);

	if (rc) {
		ALERT("Failed to request FPGA IRQ2 (%d)\n", rc);
		goto err_gpio;
	} else {
		INFO("Successfully requested FPGA IRQ2\n");
	}

	return 0;

err_gpio:
	platform_device_unregister(bifrost->pMemDev);
err_platform_alloc:
	return rc;
}

void bifrost_fvd_exit(struct bifrost_device *bifrost)
{
	INFO("\n");
	platform_device_unregister(bifrost->pMemDev);
}

void bifrost_membus_exit(struct bifrost_device *bifrost)
{
	INFO("\n");

	remove_io_regions(bifrost);
	bifrost_fvd_exit(bifrost);
}


/**
 * WriteSDRAM
 *
 * @param dev
 * @param pDst
 * @param addr SDRAM byte offset
 * @param sz Number of bytes to write
 */
static void WriteSDRAM(struct bifrost_device *bifrost, u32 pSrc, u32 addr, u32 sz)
{
	INFO("addr: hi %04x lo %04x len: 0x%x (%u)\n",
	     FPGA_ADDR_HI(addr), FPGA_ADDR_LO(addr), sz, sz);

	// SDRAM WR/RD-bit must be toggled to trig a new write
	membus_write_device_memory(bifrost->regb[0].handle, 1, FPGA_RD_BIT);

	// Set up SDRAM address register for write

	membus_write_device_memory(bifrost->regb[0].handle, 0, FPGA_ADDR_LO(addr));
	membus_write_device_memory(bifrost->regb[0].handle, 1, FPGA_ADDR_HI(addr) | FPGA_WR_BIT);

	// Let FPGA prepare to receive data
	udelay(FPGA_MEM_TIME);

	// Process main chunk data
	fpgawrite(((struct device_memory *)bifrost->regb[1].handle)->addr, pSrc, sz);
}


/**
 * ReadSDRAM
 *
 * @param dev
 * @param pDst
 * @param addr SDRAM byte offset
 * @param sz Number of bytes to read
 */
static void ReadSDRAM(struct bifrost_device *bifrost, u32 pDst, u32 addr, u32 sz)
{
	INFO("addr: hi %04x lo %04x len: 0x%x (%u)\n",
	     FPGA_ADDR_HI(addr), FPGA_ADDR_LO(addr), sz, sz);

	// SDRAM WR/RD-bit must be toggled to trig a new read
	membus_write_device_memory(bifrost->regb[0].handle, 1, FPGA_WR_BIT);

	// Set SDRAM address register
	membus_write_device_memory(bifrost->regb[0].handle, 0, FPGA_ADDR_LO(addr));
	membus_write_device_memory(bifrost->regb[0].handle, 1, FPGA_ADDR_HI(addr));

	// Let FPGA prepare data
	udelay(FPGA_MEM_TIME);

	fpgaread(pDst, ((struct device_memory *)bifrost->regb[1].handle)->addr, sz);
}

int do_membus_xfer(struct bifrost_device *bifrost,
		   struct bifrost_dma_transfer *xfer,
		   int up_down)
{
	switch (up_down) {
	case BIFROST_DMA_DIRECTION_DOWN: /* system memory -> FPGA memory */
		WriteSDRAM(bifrost, xfer->system, xfer->device, xfer->size);
		break;
	case BIFROST_DMA_DIRECTION_UP: /* FPGA memory -> system memory */
		ReadSDRAM(bifrost, xfer->system, xfer->device, xfer->size);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

/*
 * This is the interrupt service thread
 */
irqreturn_t FVDInterruptService(int irq, void *dev_id)
{
	struct bifrost_device *bifrost = (struct bifrost_device *)dev_id;
	struct bifrost_event event;
	u32 exec_status;

	INFO("Irq %d\n", irq);
	memset(&event, 0, sizeof(event));

	// Clear interrupt by reading Execute Status Register
	membus_read_device_memory(bifrost->regb[0].handle, 0x1C, &exec_status);

	// Indicate completion
	event.type = BIFROST_EVENT_TYPE_IRQ;
	event.data.irq_source = 1;
	event.data.irqstatus.value = exec_status;
	bifrost_create_event_in_atomic(bifrost, &event);

	return IRQ_HANDLED;
}

/*
 * This is the interrupt service thread for HostIntr_n(1)
 */
static irqreturn_t FVDIRQ1Service(int irq, void *dev_id)
{
	struct bifrost_device *bifrost = (struct bifrost_device *)dev_id;
	struct bifrost_event event;
	u32 vector, mask;

	memset(&event, 0, sizeof(event));

	INFO("Irq1 %d\n", irq);

	// Read interrupt mask
	membus_read_device_memory(bifrost->regb[0].handle, 0x38, &mask);

	// Read interrupt vector
	membus_read_device_memory(bifrost->regb[0].handle, 0x37, &vector);

	// printk("irg1:0x%04x, vec:0x%04x mask:0x%04x\n", irq, vector, mask);

	if (mask & vector & 0x10) {	// Board execute irq
		// Indicate completion
		event.type = BIFROST_EVENT_TYPE_IRQ;
		event.data.irq_source = 0x40;
		bifrost_create_event_in_atomic(bifrost, &event);
	}
	if (mask & vector & 0x20) {	  // HSI (BOB) irq
		// Indicate completion
		event.type = BIFROST_EVENT_TYPE_IRQ;
		event.data.irq_source = 0x02;
		bifrost_create_event_in_atomic(bifrost, &event);
	}
	if (mask & vector & 0x100) {  // JPEGLS irq
		u32 frameNo, frameSize;
		u32 frameSizeReg = 0x171;

		// Indicate completion
		event.type = BIFROST_EVENT_TYPE_IRQ;
		event.data.irq_source = 0x04;

		// Read interrupt mask
		membus_read_device_memory(bifrost->regb[0].handle, 0x170, &frameNo);      // JLSLastBuffer

		// Read interrupt vector
		frameSizeReg += frameNo;

		membus_read_device_memory(bifrost->regb[0].handle, frameSizeReg, &frameSize);    // JLSBufferSize
		event.data.frame.frameNo = frameNo;
		event.data.frame.frameSize = frameSize;
#if KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE
		{
			struct timespec64 ts64;

			ktime_get_real_ts64(&ts64);
			event.data.frame.time.tv_sec = (__kernel_old_time_t) ts64.tv_sec;
			event.data.frame.time.tv_nsec = (long) ts64.tv_nsec;
		}
#else
		getnstimeofday(&event.data.frame.time);
#endif

		// printk("lastbuf:%d, size:%d\n", frameNo, frameSize);

		bifrost_create_event_in_atomic(bifrost, &event);
	}
	if (mask & vector & 0x200) {  // DIO irq
		u32 status = 0;
		u32 camtype;

		// Read camera type
		membus_read_device_memory(bifrost->regb[0].handle, 0x23, &camtype);	 // Cam type

		// Indicate completion
		event.type = BIFROST_EVENT_TYPE_IRQ;
		event.data.irq_source = 0x08;

		// Read digital input status
		if ((camtype & 0xFFFF) == 0x8) // T1K
			membus_read_device_memory(bifrost->regb[0].handle, 0x150, &status);  // Digital IN
		else if ((camtype & 0xFFFF) == 0x19) // EC501
			membus_read_device_memory(bifrost->regb[0].handle, 0x152, &status);  // Digital IN
		else if ((camtype & 0xFFFF) == 0x1F) // EC501 i3
			membus_read_device_memory(bifrost->regb[0].handle, 0x152, &status);  // Digital IN

		// printk("DIO irq status:%d\n", status);

		event.data.irqstatus.value = status;

		bifrost_create_event_in_atomic(bifrost, &event);
	}
	if (mask & vector & 0x400) {  // HSI cable irq
		u32 hsi_state;
		u32 cable_state;

		// Indicate completion
		event.type = BIFROST_EVENT_TYPE_IRQ;
		event.data.irq_source = 0x10;

		// Read interrupt mask
		membus_read_device_memory(bifrost->regb[0].handle, 0x141, &hsi_state);  // HSI cable state
		cable_state = !!!(hsi_state & 0x10);  // bit4 indicates cable link up or down.
						      // '0' == link is up, '1' == link is down.

		// printk("HSI state: 0x%04x  => cablestate:%d\n", hsi_state, cable_state);

		event.data.irqstatus.value = cable_state;

		bifrost_create_event_in_atomic(bifrost, &event);
	}

	return IRQ_HANDLED;
}

/*
 * This is the interrupt service thread for HostIntr_n(2)
 */
static irqreturn_t FVDIRQ2Service(int irq, void *dev_id)
{
	struct bifrost_device *bifrost = (struct bifrost_device *)dev_id;
	struct bifrost_event event;
	u32 camtype;
	u32 bufNo, frameCnt, hd1, hd2, hd3, hd4, hd5;

	memset(&event, 0, sizeof(event));

	// Read camera type
	membus_read_device_memory(bifrost->regb[0].handle, 0x23, &camtype);	 // Cam type

	if (((camtype & 0xFFFF) == 0x19) ||  // EC501
	    ((camtype & 0xFFFF) == 0x1F)) {  // EC501 i3
		// Read frame trig data and line stamp data
		membus_read_device_memory(bifrost->regb[0].handle, 0xb9, &bufNo); // last live IR buffer
		membus_read_device_memory(bifrost->regb[0].handle, 0x14E, &hd1);
		membus_read_device_memory(bifrost->regb[0].handle, 0x14F, &hd2);
		membus_read_device_memory(bifrost->regb[0].handle, 0x150, &hd3);
		membus_read_device_memory(bifrost->regb[0].handle, 0x151, &hd4);
		membus_read_device_memory(bifrost->regb[0].handle, 0x152, &hd5);
		membus_read_device_memory(bifrost->regb[0].handle, 0x153, &frameCnt);

		// Fill in frame data
		event.data.frame.frameNo = bufNo & 0xF;
		// Line state
		if (hd5 & 0x1)
			event.data.frame.frameNo |= 0x10;
		if (hd5 & 0x100)
			event.data.frame.frameNo |= 0x20;
		// Trig state
		if (hd1 & 0x1)
			event.data.frame.frameNo |= 0x40;
		if (hd1 & 0x100)
			event.data.frame.frameNo |= 0x80;

		event.data.frame.frameNo |= (hd2 & 0xF << 8); // Frame time stamp channel 0
		event.data.frame.frameNo |= (((hd3>>8) & 0xF) << 12); // Frame time stamp channel 1
		event.data.frame.frameNo |= (frameCnt << 16);

		// Line time stamps
		event.data.frame.frameSize = ((hd2>>8)&0xFF) | ((hd3&0xFF) << 8) | ((hd4&0xFFFF) << 16);
	}
	else if (((camtype & 0xFFFF) == 0x2A) ||  // EOCO
	         ((camtype & 0xFFFF) == 0x26)) {  
		membus_read_device_memory(bifrost->regb[0].handle, 0xb9, &bufNo); // last live IR buffer
		// Fill in frame data
		event.data.frame.frameNo = bufNo & 0xF;
    }

	// Indicate completion
	event.type = BIFROST_EVENT_TYPE_IRQ;
	event.data.irq_source = 0x20;
#if  KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE
	{
		struct timespec64 ts64;

		ktime_get_real_ts64(&ts64);
		event.data.frame.time.tv_sec = (__kernel_old_time_t) ts64.tv_sec;
		event.data.frame.time.tv_nsec = (long) ts64.tv_nsec;
	}
#else
	getnstimeofday(&event.data.frame.time);
#endif
	bifrost_create_event_in_atomic(bifrost, &event);

	return IRQ_HANDLED;
}
