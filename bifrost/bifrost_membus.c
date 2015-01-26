/*
 * Copyright (c) FLIR Systems AB.
 *
 * bifrost_membus.c
 *
 *  Created on: April 16, 2014
 *      Author: Peter Fitger <peter.fitger@flir.se>
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

#include "bifrost.h"
#include "bifrost_dma.h"
#include "valhalla_msi.h"
#include "valhalla_dma.h"

/* FIXME: This should have a ifdef for kernel version to get sizes. I don't know when this changed. */
#ifndef SZ_4K
#include <asm-generic/sizes.h>
#endif

#define FPGA_MEM_TIME   30    // Time in us from address write to memory access

#define FPGA_IRQ_0      ((3-1)*32 + 16)


// NOTE! - This function must perform 8-bytes (4 16 bit words) bursts to FPGA (bug in iMX6)
static void fpgaread (u32 dst, void *src, u32 len)
{
        u64 * pDst = (u64 *)dst;
        u64 * pSrc = (u64 *)src;
        len >>= 3;

        while (len--)
                *pDst++=*pSrc;
}

// NOTE! - This function must perform 8-bytes (4 16 bit words) bursts to FPGA (bug in iMX6)
static void fpgawrite (void *dst, u32 src, u32 len)
{
        u64 * pDst = (u64 *)dst;
        u64 * pSrc = (u64 *)src;
        len >>= 3;

        while (len--)
                *pDst=*pSrc++;
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
}

static int map_device_memory(struct device_memory *mem)
{
        if (mem->size == 0)
                return -EINVAL; /* Nothing to I/O map */
        if (mem->bar >= 2)
                return -EINVAL; /* Only two chip selects active */

#ifndef CS0_BASE_ADDR
#define CS0_BASE_ADDR 0
        ALERT("CS0_BASE_ADDR undefined\n");
#endif

        mem->addr_bus = CS0_BASE_ADDR + (mem->bar * 0x02000000);
        mem->addr = ioremap_nocache(mem->addr_bus, SZ_4K);
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

int __init bifrost_membus_init(struct bifrost_device *dev)
{
        int rc;

        INFO("\n");

        /* Gain CPU access to FPGA register map */
        rc = setup_io_regions(bdev);
        if (rc < 2)
                goto err_pci_iomap_regb;

        /* Run post init and make driver accessible */
        if (bifrost_cdev_init(bdev) != 0)
                goto err_pci_iomap_regb;

        if (bifrost_fvd_init(dev) != 0)
                goto err_pci_iomap_regb;

        return 0;

        /* stack-like cleanup on error */
  err_pci_iomap_regb:
        remove_io_regions(bdev);
        return -ENODEV;
}

int  bifrost_fvd_init(struct bifrost_device *dev)
{
        int rc;
        // Register platform driver
        bdev->pMemDev = platform_device_alloc(BIFROST_DEVICE_NAME, 1);
        if (bdev->pMemDev == NULL) {
                ALERT("Registering MEMBUS driver failed\n");
                goto err_platform_alloc;
        }
        platform_device_add(bdev->pMemDev);

        bdev->pClass = class_create(THIS_MODULE, BIFROST_DEVICE_NAME);
        device_create(bdev->pClass, NULL, bdev->cdev.dev, NULL, "bif0");

        if (gpio_is_valid(FPGA_IRQ_0) == 0)
                ALERT("FPGA_IRQ_0 can not be used\n");

        gpio_request(FPGA_IRQ_0, "FpgaIrq0");
        gpio_direction_input(FPGA_IRQ_0);

        rc = request_irq(gpio_to_irq(FPGA_IRQ_0), FVDInterruptService,
                IRQF_TRIGGER_FALLING, "FpgaIrq0", bdev);

        if (rc != 0)
                ALERT("Failed to request FPGA IRQ (%d)\n", rc);
        else
                INFO("Successfully requested FPGA IRQ\n");

        return 0;

        err_platform_alloc:
        return -ENODEV;

}

void bifrost_fvd_exit(struct bifrost_device *dev)
{
        INFO("\n");

        free_irq(gpio_to_irq(FPGA_IRQ_0), bdev);
        gpio_free(FPGA_IRQ_0);
        device_destroy(dev->pClass, dev->cdev.dev);
        class_destroy(dev->pClass);
        platform_device_unregister(dev->pMemDev);
}

void bifrost_membus_exit(struct bifrost_device *dev)
{
        INFO("\n");

        remove_io_regions(dev);
        bifrost_fvd_exit(dev);
}


////////////////////////////////////////////////////////
//
// WriteSDRAM
//
////////////////////////////////////////////////////////
static void WriteSDRAM(struct bifrost_device *dev,
                 u32 pSrc,
                 u32 addr,    // SDRAM byte offset
                 u32 sz)  // Number of bytes to write
{
        // Prepare input data
        addr >>= 2; // Adjust for 32 bit address

        // Set up SDRAM address register for write
        membus_write_device_memory(dev->regb[0].handle, 0, addr & 0xFFF8);
        membus_write_device_memory(dev->regb[0].handle, 1, (addr >> 16) | 0x8000);

        // Let FPGA prepare to receive data
        udelay(FPGA_MEM_TIME);

        // Process main chunk data
        fpgawrite(((struct device_memory*)dev->regb[1].handle)->addr, pSrc, sz);
}

////////////////////////////////////////////////////////
//
// ReadSDRAM
//
////////////////////////////////////////////////////////
static void ReadSDRAM(struct bifrost_device *dev,
                       u32 pDst,
                       u32 addr,      // SDRAM byte offset
                       u32 sz)    // Number of bytes to read
{
        // Prepare input data
        addr >>= 2; // Adjust for 32 bit address

        // Set SDRAM address register
        membus_write_device_memory(dev->regb[0].handle, 0, addr & 0xFFF8);
        membus_write_device_memory(dev->regb[0].handle, 1, (addr >> 16) & 0x7FFF);

        // Let FPGA prepare data
        udelay(FPGA_MEM_TIME);

        fpgaread(pDst, ((struct device_memory*)dev->regb[1].handle)->addr, sz);
}

int do_membus_xfer(struct bifrost_device *dev,
                     struct bifrost_dma_transfer *xfer,
                     int up_down)
{
        switch (up_down) {
        case BIFROST_DMA_DIRECTION_DOWN: /* system memory -> FPGA memory */
                WriteSDRAM(dev, xfer->system, xfer->device, xfer->size);
                break;
        case BIFROST_DMA_DIRECTION_UP: /* FPGA memory -> system memory */
                ReadSDRAM(dev, xfer->system, xfer->device, xfer->size);
                break;
        default:
                return -EINVAL;
        }
        return 0;
}

/////////////////////////////////////////////////
// This is the interrupt service thread
/////////////////////////////////////////////////
irqreturn_t FVDInterruptService(int irq, void *dev_id)
{
        struct bifrost_device *dev = (struct bifrost_device *)dev_id;
        struct bifrost_event event;
        u32 dummy;

        INFO("Irq %d\n", irq);

        // Clear interrupt by reading Execute Status Register
        membus_read_device_memory(dev->regb[0].handle, 0x1C, &dummy);

        // Indicate completion
        event.type = BIFROST_EVENT_TYPE_IRQ;
        event.data.irq_source = 1;
        bifrost_create_event_in_atomic(dev, &event);

        return IRQ_HANDLED;
}
