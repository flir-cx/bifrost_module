/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_frmi.h
 *
 *  Created on: Nov 15, 2013
 *      Author: Jonas Lundberg <jonas.lundberg@flir.se>
 *
 * Register map for FPGA Valhalla Framing block.
 *
 */

#ifndef VALHALLA_FRMI_H_
#define VALHALLA_FRMI_H_

namespace VALHALLA_FRMI
{
    const uint32_t      BASE_ADDRESS              =  0x0000E00;

    /* Masks used with register CONTROL1, CONTROL2, CONTROL3 */
    const uint32_t      BIT_MASK_10_0             = (0x7FF << 0);   ///< bit[10:0]
    const uint_fast8_t  BIT_SHIFT_0               =  0;
    const uint32_t      BIT_MASK_26_16            = (0x7FF << 16);  ///< bit[26:16]
    const uint_fast8_t  BIT_SHIFT_16              =  16;

    /* CONTROL1 register */
    const uint32_t      CONTROL1_OFFSET           =  0x0010;        ///< offset relative to unit base address
    // bit[10:0] = Invideo column size
    // bit[26:16] = Invideo row size

    /* CONTROL2 register */
    const uint32_t      CONTROL2_OFFSET           =  0x0014;        ///< offset relative to unit base address
    // bit[10:0] = Outvideo column size
    // bit[26:16] = Outvideo row size

    /* CONTROL3 register */
    const uint32_t      CONTROL3_OFFSET           =  0x0018;        ///< offset relative to unit base address
    // bit[10:0] = Column offset
    // bit[26:16] = Row offset

    /* CONTROL4 register */
    const uint32_t      CONTROL4_OFFSET           =  0x001C;        ///< offset relative to unit base address
    const uint32_t      BORDER_COLOR_B_MASK       = (0x3FF << 0);   ///< bit[9:0] = Border color B
    const uint_fast8_t  BORDER_COLOR_B_SHIFT      =  0;
    const uint32_t      BORDER_COLOR_G_MASK       = (0x3FF << 10);  ///< bit[19:10] = Border color G
    const uint_fast8_t  BORDER_COLOR_G_SHIFT      =  10;
    const uint32_t      BORDER_COLOR_R_MASK       = (0x3FF << 20);  ///< bit[29:20] = Border color R
    const uint_fast8_t  BORDER_COLOR_R_SHIFT      =  20;

    /* CONTROL5 register */
    const uint32_t      CONTROL5_OFFSET           =  0x0020;        ///< offset relative to unit base address
    const uint32_t      DEBUG_CLEAR_MASK          =  (1 << 0);      ///< bit[0] = Debug clear

    /* STATUS1 register */
    const uint32_t      STATUS1_OFFSET            =  0x0080;        ///< offset relative to unit base address
    const uint32_t      WRITE_WHEN_FIFO_FULL_MASK = (1 << 0);       ///< bit[0] = Write when FIFO full
    const uint32_t      READ_WHEN_FIFO_EMPTY_MASK = (1 << 1);       ///< bit[1] = Read when FIFO empty
}

#endif
