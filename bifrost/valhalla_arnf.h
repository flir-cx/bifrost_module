/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_arnf.h
 *
 *  Created on: Jan 21, 2014
 *      Author: Ari Mannila <ari.mannila@flir.se>
 *
 * Register map for FPGA Valhalla ARNF (Adaptive Row Noise Filter) block.
 *
 */

#ifndef VALHALLA_ARNF_H_
#define VALHALLA_ARNF_H_

namespace VALHALLA_ARNF
{
    const uint32_t      BASE_ADDRESS              = 0x00008600;

    const uint32_t      CONTROL1_OFFSET           = 0x0010;
    const uint32_t      OUT_MUX_MASK              = 0x000F;
    const uint_fast8_t  OUT_MUX_SHIFT             = 0;

    const uint32_t      NFTEMP_OFFSET             = 0x0014;
    const uint32_t      NFTEMP_MASK               = 0xFFFF;
    const uint_fast8_t  NFTEMP_SHIFT              = 0;

    const uint32_t      NFSPAT_OFFSET             = 0x0018;
    const uint32_t      NFSPAT_MASK               = 0xFFFF;
    const uint_fast8_t  NFSPAT_SHIFT              = 0;
}

#endif
