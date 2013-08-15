/*
 * Copyright (c) FLIR Systems AB.
 *
 * bifrost_cdev.c
 *
 *  Created on: Feb 1, 2012
 *      Author: Jonas Romfelt <jonas.romfelt@flir.se>, Tommy Karlsson
 *
 * Bifrost's simulator part...
 *
 */

#include <linux/errno.h>
#include <linux/vmalloc.h>
#include "bifrost.h"

static int bifrost_sim_alloc_dma_buffer(struct dma_buffer *buf, size_t size,
                                        struct pci_dev *pdev)
{
        unsigned long addr;

        size = PAGE_ALIGN(size);
        buf->addr = vmalloc(size);
        if (buf->addr == NULL)
                return -ENOMEM;

        buf->addr_phys = 0;
        buf->pdev = NULL;
        buf->size = size;
        buf->length = 0;

        /*
         * Clear memory to prevent kernel info leakage into user-space
         * */
        memset(buf->addr, 0, buf->size);

        addr = (unsigned long)buf->addr;
        while (size > 0) {
                SetPageReserved(vmalloc_to_page((void *)addr));
                addr += PAGE_SIZE;
                size -= PAGE_SIZE;
        }

        INFO("Simulator DMA buffer allocated size=%dKiB\n",
             buf->size / 1024);
        return 0;        
}

static void bifrost_sim_free_dma_buffer(struct dma_buffer *buf)
{
        size_t size;
        unsigned long addr;

        if (buf && buf->addr) {
                size = buf->size;
                addr = (unsigned long)buf->addr;
                while (size > 0) {
                        ClearPageReserved(vmalloc_to_page((void *)addr));
                        addr += PAGE_SIZE;
                        size -= PAGE_SIZE;
                }
                vfree(buf->addr);
        }
}

static int bifrost_sim_remap_pfn_range(struct bifrost_device *dev,
                                       struct vm_area_struct *vma)
{
        unsigned long start = vma->vm_start;
        unsigned long size = vma->vm_end - vma->vm_start;
        unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
        unsigned long pfn, ptr;
        
        INFO("mmap() start=%08lX offset=%08lX size=%08lX\n", start,
             offset, size);

        if (offset + size > dev->scratch.size) {
                INFO("Request to mmap() %ld bytes starting at %ld, but only "
                     "%d total is available\n", size, offset,
                     dev->scratch.size);
                return -EINVAL;
        }

        ptr = (unsigned long)bdev->scratch.addr + offset;
        
        while (size > 0) {
                pfn = vmalloc_to_pfn((void *)ptr);
                if (remap_pfn_range(vma, start, pfn, PAGE_SIZE, PAGE_SHARED))
                        return -EAGAIN;
                        
                start += PAGE_SIZE;
                ptr += PAGE_SIZE;
                if (size > PAGE_SIZE)
                        size -= PAGE_SIZE;
                else
                        size = 0;
        }

        vma->vm_flags |= VM_RESERVED; /* Avoid to swap out */
        return 0;
}

struct bifrost_operations bifrost_sim_ops = {
        .alloc_dma_buffer = bifrost_sim_alloc_dma_buffer,
        .free_dma_buffer = bifrost_sim_free_dma_buffer,
        .remap_pfn_range = bifrost_sim_remap_pfn_range,        
};

/* These are the simulated PCIe BARs */
struct pcie_sim_bar {
        char *name;
        unsigned long num_32bit_regs;
        unsigned long flags;
        struct bifrost_regb *regb;
};

static struct pcie_sim_bar bar[6] = {
        {"DMA", 512, 0, NULL},
        {"CTL", 1024, 0, NULL},
        {"", 0, 0, NULL},
        {"", 0, 0, NULL},
        {"", 0, 0, NULL},
        {"", 0, 0, NULL},
};

static void remove_pcie_sim_bars(void)
{
        int n;

        for (n = 0; n < ARRAY_SIZE(bar); n++)
                kfree(bar[n].regb);
}

static int setup_pcie_sim_bars(void)
{
        int k, n;
        size_t size;

        for (n = 0; n < ARRAY_SIZE(bar); n++) {
                size = bar[n].num_32bit_regs * sizeof(*(bar[0].regb));
                bar[n].regb = kzalloc(size, GFP_KERNEL);
                if (bar[n].regb == NULL) {
                        remove_pcie_sim_bars();
                        return -ENOMEM;
                }

                /* Set default mode */
                for (k = 0; k < bar[n].num_32bit_regs; k++)
                        bar[n].regb[k].mode = (BIFROST_REGB_MODE_READABLE |
                                               BIFROST_REGB_MODE_WRITABLE);                
        }
        return 0;
}

/* Returns a pointer to the simulated device register */
static inline struct bifrost_regb *regb(const struct device_memory *mem,
                                        unsigned int offset)
{
        return ((struct bifrost_regb *)mem->addr + (offset / 4));
}

static inline void set_regb_value(struct device_memory *mem,
                                  unsigned int offset,
                                  unsigned int value)
{
        regb(mem, offset)->value = value;
}

static inline unsigned int get_regb_value(const struct device_memory *mem,
                                          unsigned int offset)
{
        return regb(mem, offset)->value;
}

static inline void set_regb_mode(struct device_memory *mem,
                                 unsigned int offset,
                                 unsigned int mode)
{
        regb(mem, offset)->mode = mode;
}

static inline unsigned int get_regb_mode(const struct device_memory *mem,
                                         unsigned int offset)
{
        return regb(mem, offset)->mode;
}

static inline int check_regb_mode(const struct device_memory *mem,
                                  unsigned int offset,
                                  unsigned int mode)
{
        return (get_regb_mode(mem, offset) & mode) ? 0 : -EINVAL;
}

static int write_device_memory(void *handle, u32 offset, u32 value)
{
        struct device_memory *mem = handle;
        struct bifrost_device *dev;
        struct bifrost_event event;

        dev = (struct bifrost_device *)mem->pdev;
        
        if (check_regb_mode(mem, offset, BIFROST_REGB_MODE_WRITABLE) < 0) {
                INFO("register is not writable");
                return -EACCES;
        }
        set_regb_value(mem, offset, value);

        if (check_regb_mode(mem, offset, BIFROST_REGB_MODE_WRITE_EVENT) == 0) {
                event.type = BIFROST_EVENT_TYPE_WRITE_REGB;
                event.data.regb_access.bar = mem->bar;
                event.data.regb_access.offset = offset;
                event.data.regb_access.value = value;                                
                bifrost_create_event(dev, &event);
        }
        return 0;
}

static int read_device_memory(void *handle, u32 offset, u32 *value)
{
        struct device_memory *mem = handle;
        struct bifrost_device *dev;
        struct bifrost_event event;
        
        dev = (struct bifrost_device *)mem->pdev;

        if (check_regb_mode(mem, offset, BIFROST_REGB_MODE_READABLE) < 0) {
                INFO("register is not readable");
                return -EACCES;
        }
        *value = get_regb_value(mem, offset);
                
        if (check_regb_mode(mem, offset, BIFROST_REGB_MODE_READ_EVENT) == 0) {
                event.type = BIFROST_EVENT_TYPE_READ_REGB;
                event.data.regb_access.bar = mem->bar;
                event.data.regb_access.offset = offset;
                event.data.regb_access.value = *value;
                bifrost_create_event(dev, &event);
        }
        return 0;
}

static int set_mode_device_memory(void *handle, u32 offset, u32 mode)
{
        struct device_memory *mem = handle;
        
        set_regb_mode(mem, offset, mode);
        return 0;
}

static void init_device_memory(struct bifrost_device *dev, int n,
                               struct device_memory *mem)
{
        memset(mem, 0, sizeof(*mem));
        
        mem->bar = n;
        mem->pdev = (struct pci_dev *)dev; /* We need the bifrost handle in wr/rd */
        mem->addr_bus = (unsigned long)bar[n].regb;
        mem->flags = bar[n].flags;
        mem->size = bar[n].num_32bit_regs * 4;
        mem->handle = mem;
        mem->rd = read_device_memory;
        mem->wr = write_device_memory;
        mem->mset = set_mode_device_memory;
        spin_lock_init(&mem->lock);
}

static int map_device_memory(struct device_memory *mem)
{
        unsigned long size;

        if (mem->size == 0)
                return -EINVAL; /* Nothing to I/O map */

        mem->addr = (void *)mem->addr_bus;
        mem->enabled = 1;

        size = mem->size * sizeof(u32);
        INFO("BAR%d %08lx - %08lx => %08lx - %08lx (%lu KByte)\n",
             mem->bar, mem->addr_bus, mem->addr_bus + size - 1,
             (unsigned long)mem->addr, (unsigned long)mem->addr + size - 1,
             size / 1024);

        return 0;
}

static void unmap_device_memory(struct device_memory *mem)
{
        mem->enabled = 0;
        mem->addr = NULL;
}

static int setup_io_regions(struct bifrost_device *bifrost)
{
        int n, nbars;

        for (nbars = 0, n = 0; n < ARRAY_SIZE(bifrost->regb); n++) {
                init_device_memory(bifrost, n, &bifrost->regb[n]);
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

int bifrost_sim_pci_init(struct bifrost_device *dev)
{
        if (setup_pcie_sim_bars() < 0) {
                ALERT("no memory to simulate PCIe BARs\n");
                return -ENOMEM;
        }
        if (setup_io_regions(dev) == 0) {
                ALERT("failed to simulate PCIe BARs\n");
                return -EINVAL;
        }
        return 0;
}

void bifrost_sim_pci_exit(struct bifrost_device *dev)
{
        remove_io_regions(dev);
        remove_pcie_sim_bars();
}

