/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_msi.h
 *
 *  Created on: Mar 16, 2012
 *      Author: Tommy Karlsson, Jonas Romfelt <jonas.romfelt@flir.se>
 *
 * Valhalla MSI definitions (max 32):
 *
 * MSI vector   Interrupt      Description
 *
 *   0          0x00000001     DMA done on channel 0
 *   1          0x00000002     DMA done on channel 1
 *   2          0x00000004     DMA done on channel 2
 *   3          0x00000008     DMA done on channel 3
 *   4          0x00000010     DMA done on channel 4
 *   5          0x00000020     DMA done on channel 5
 *   6          0x00000040     DMA done on channel 6
 *   7          0x00000080     DMA done on channel 7
 *   8          0x00000100     FPGA error
 *   ...
 *  10          0x00000400     System sync
 *  11          0x00000800     clrx_sync
 *  12          0x00001000     Video frame sync: VIN0
 *  13          0x00002000     Video frame sync: VIN1
 *  14          0x00004000     IRIG pps
 *  14          0x00008000     MDST done
 *  15          0x00010000     MDSRX6 done
 *  16          0x00020000     Manual interrupt
 *   ...
 */

#ifndef VALHALLA_MSI_H_
#define VALHALLA_MSI_H_

#ifdef __KERNEL__
#include <linux/bitops.h>
#include <linux/errno.h>

/*
 * Map an MSI vector to an interrupt event and vs.
 */
static inline unsigned int map_msi_to_event(unsigned int vec)
{
        return (1 << vec);
}

static inline int map_event_to_msi(unsigned int event)
{
        int vec;

        vec = ffs(event) - 1;
        if (vec < 0)
                return -EINVAL;

        return vec;
}
#endif

#endif
