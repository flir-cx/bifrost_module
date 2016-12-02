/* Copyright (c) 2013 FLIR Systems AB. All rights reserved. */
#ifndef __BIFROST_DMA_H
#define __BIFROST_DMA_H

#include <linux/list.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/completion.h>

typedef void (*dma_xfer_t)(void *, u32, u32, u32, u32, u32);

struct dma_req {
        u32 src;
        u32 dst;
        u32 len;
        u32 dir;

        /* Don't touch */
        struct list_head node;
        int ticket;
        void *cookie;
        struct timespec ts;
        struct completion *pwork;
};

struct dma_usr_req {
    void * usr_buff;
    void * dev_buff;
    u32 size;
    dma_addr_t bus_addr;
    dma_addr_t phy_addr;
    int up_down;
    struct completion work;
    void *cookie;
};

struct dma_ch;
struct dma_ctl;
struct bifrost_device;

extern struct dma_ctl *alloc_dma_ctl(int num_ch, int idle_map, dma_xfer_t xfer,
				void *data);
extern void free_dma_ctl(struct dma_ctl *ctl);

extern void enable_dma_ch(struct dma_ctl *ctl, int ch, int irq);
extern void disable_dma_ch(struct dma_ctl *ctl, int ch);

extern struct dma_req *alloc_dma_req(unsigned int *ticket, void *cookie,
                                     gfp_t flags);
extern void free_dma_req(struct dma_req *req);
extern int start_dma_xfer(struct dma_ctl *ctl, struct dma_req *req);
extern void *dma_done(struct dma_ctl *ctl, int irq, unsigned int *ticket,
                      s64 *time, struct bifrost_device *dev);

#endif
