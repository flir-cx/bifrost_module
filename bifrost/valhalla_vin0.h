/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_vin.h
 *
 *  Created on: Oct 23, 2014
 *      Author: Ari Mannila <ari.mannila@flir.se>
 *
 * Register map for FPGA Valhalla Video In0 block.
 *
 */

#ifndef VALHALLA_VIN0_H_
#define VALHALLA_VIN0_H_

namespace VALHALLA_VIN0
{
    const uint32_t      REG_VIN0_1                    = 0x0010;       ///< offset relative to unit base address

    const uint32_t      REG_VIN0_2                    = 0x0014;       ///< offset relative to unit base address
    const uint32_t      REG_VIN0_2_ENABLE_MASK        = (1 << 0);
    const uint32_t      REG_VIN0_2_BLANK_MASK         = (1 << 1);
    const uint32_t      REG_VIN0_2_STRETCH_MASK       = (1 << 2);
    const uint32_t      REG_VIN0_2_MISB0403_MASK      = (1 << 3);
    const uint32_t      REG_VIN0_2_ERROR_CLR_MASK     = (1 << 7);

    const uint32_t      REG_VIN0_2_VIDEOMODE_MASK     = (0x1F << 8);
    const uint_fast8_t  REG_VIN0_2_VIDEOMODE_SHIFT    = 8;

    const uint32_t      REG_VIN0_2_LINEREQ_TRIG_MASK  = (0xFF << 8);
    const uint_fast8_t  REG_VIN0_2_LINEREQ_TRIG_SHIFT = 16;
}

#endif
