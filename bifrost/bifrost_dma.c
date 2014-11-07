/* Copyright (c) 2013 FLIR Systems AB. All rights reserved. */
#include <asm/atomic.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/time.h>
#include <linux/types.h>

#include "bifrost_dma.h"

#define MAX_DMA_CHANNELS 32

struct dma_ch {
        int irq;
        struct dma_req *in_progress;
};

struct dma_ctl {
        struct list_head list;
        spinlock_t lock;
        unsigned int num_ch;
        unsigned int idle_bitmap;
        struct dma_ch ch[MAX_DMA_CHANNELS];

        void *data;
        dma_xfer_t start_xfer;
};

static s64 get_xfer_time_ns(struct timespec *start)
{
        struct timespec stop, ts;

        getnstimeofday(&stop);
        ts = timespec_sub(stop, *start);

        return timespec_to_ns(&ts);
}

static void kick_off_xfer(struct dma_ctl *ctl, int ch, struct dma_req *req)
{
        getnstimeofday(&req->ts);
        ctl->start_xfer(ctl->data, ch, req->src, req->dst, req->len, req->dir);
}

static int get_ticket(void)
{
        static atomic_t ticket = ATOMIC_INIT(0);

        /*
         * Sometimes ticket is returned from functions, which need to indicate
         * an error with a negative return value.
         */
        return atomic_add_return(1, &ticket) & 0x7fffffff;
}

/* Requires that the DMA controller's spinlock is held */
static int __find_1st_idle_chan(struct dma_ctl *ctl)
{
        int ch;

        ch = ffs(ctl->idle_bitmap) - 1;
        if (ch < 0)
                return -EBUSY;

        ctl->idle_bitmap &= ~(1 << ch);
        return ch;
}

/* Requires that the DMA controller's spinlock is held */
static void __free_chan(struct dma_ctl *ctl, int ch)
{
        ctl->idle_bitmap |= (1 << ch);
}

static int lookup_chan(struct dma_ctl *ctl, int irq)
{
        unsigned int n;

        for (n = 0; n < ctl->num_ch; n++) {
                if (ctl->ch[n].irq == irq)
                        return n;
        }
        return -EINVAL;
}

struct dma_ctl *alloc_dma_ctl(int num_ch, int idle_map, dma_xfer_t xfer,
			void *data)
{
        struct dma_ctl *ctl;

        ctl = kzalloc(sizeof(*ctl), GFP_KERNEL);
        if (ctl == NULL)
                return NULL;

        num_ch = min(num_ch, MAX_DMA_CHANNELS);
        ctl->num_ch = num_ch;
        ctl->idle_bitmap = idle_map;
        ctl->start_xfer = xfer;
        ctl->data = data;
        INIT_LIST_HEAD(&ctl->list);
	spin_lock_init(&ctl->lock);

	printk("bifrost: DMA channels = %d, DMA idle map = %x\n",
		num_ch, idle_map);

        return ctl;
}

void enable_dma_ch(struct dma_ctl *ctl, int ch, int irq)
{
        ctl->ch[ch].irq = irq;
}

void disable_dma_ch(struct dma_ctl *ctl, int ch)
{
        /* FIXME: block until all transfers are done */
}

void free_dma_ctl(struct dma_ctl *ctl)
{
        kfree(ctl);
}

struct dma_req *alloc_dma_req(unsigned int *ticket, void *cookie, gfp_t flags)
{
        struct dma_req *req;

        req = kzalloc(sizeof(*req), flags);
        if (req == NULL)
                return NULL;

        req->cookie = cookie;
        req->ticket = get_ticket();
        *ticket = req->ticket;

        return req;
}

void free_dma_req(struct dma_req *req)
{
        kfree(req);
}

int start_dma_xfer(struct dma_ctl *ctl, struct dma_req *req)
{
        unsigned long flags;
        int ch, start_xfer;

        spin_lock_irqsave(&ctl->lock, flags);
        ch = __find_1st_idle_chan(ctl);
        if (ch >= 0) {
                ctl->ch[ch].in_progress = req;
                start_xfer = 1;
        } else {
                list_add_tail(&req->node, &ctl->list);
                start_xfer = 0;
        }
        spin_unlock_irqrestore(&ctl->lock, flags);

        if (start_xfer)
                kick_off_xfer(ctl, ch, req);

        return 0;
}



void *dma_done(struct dma_ctl *ctl, int irq, unsigned int *ticket, s64 *time)
{
        unsigned long flags;
        int ch, start_xfer;
        void *cookie;
        struct dma_req *req;

        ch = lookup_chan(ctl, irq);
        if ((ch < 0) || (ctl == NULL)) {
                printk("Spurious DMA interrupt %d\n", irq);
                return ERR_PTR(-EINVAL);
        }
        if (ctl->ch[ch].in_progress == NULL) {
                printk("Spurious DMA interrupt %d on channel %d\n", irq, ch);
                return ERR_PTR(-EINVAL);
        }

        req = ctl->ch[ch].in_progress;
        ctl->ch[ch].in_progress = NULL;
        cookie = req->cookie;
        *ticket = req->ticket;
        *time = get_xfer_time_ns(&req->ts);

        if(req->pwork)
            complete(req->pwork);


        kfree(req);

        spin_lock_irqsave(&ctl->lock, flags);
        if (!list_empty(&ctl->list)) {
                req = list_first_entry(&ctl->list, struct dma_req, node);
                list_del(&req->node);
                ctl->ch[ch].in_progress = req;
                start_xfer = 1;
        } else {
                __free_chan(ctl, ch);
                start_xfer = 0;
        }
        spin_unlock_irqrestore(&ctl->lock, flags);

        if (start_xfer)
                kick_off_xfer(ctl, ch, req);

        return cookie;
}

int get_dma_info(struct dma_ctl *ctl, char *buf, size_t bufsz)
{
        return snprintf(buf, bufsz,
                        "Number of channels: %u\n"
                        "Idle bitmap:        %x\n",
                        ctl->num_ch, ctl->idle_bitmap);
}

/*
 * Special function to "reset" the DMA controller when we are
 * running in simulator mode.
 */
void reset_dma_sim(struct dma_ctl *ctl)
{
        unsigned long flags;
        struct list_head *pos, *tmp;
        struct dma_req *req;
        int n;

        if (ctl == NULL)
                return;

        spin_lock_irqsave(&ctl->lock, flags);
        for (n = 0; n < ARRAY_SIZE(ctl->ch); n++) {
                kfree(ctl->ch[n].in_progress);
                ctl->ch[n].in_progress = NULL;
        }
        ctl->idle_bitmap = (1 << ctl->num_ch) - 1;

        list_for_each_safe(pos, tmp, &ctl->list) {
                req = list_entry(pos, struct dma_req, node);
                list_del(&req->node);
                kfree(req);
        }
        spin_unlock_irqrestore(&ctl->lock, flags);
}
