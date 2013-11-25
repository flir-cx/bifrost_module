/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_r2yc.h
 *
 *  Created on: Nov 20, 2013
 *      Author: Jonas Lundberg <jonas.lundberg@flir.se>
 *
 * Register map for FPGA Valhalla RGB to YCbCr block.
 *
 */

#ifndef VALHALLA_R2YC_H_
#define VALHALLA_R2YC_H_

namespace VALHALLA_R2YC
{
    const uint32_t  BASE_ADDRESS            =  0x00000D00;

    const uint32_t  CONTROL1_OFFSET         =  0x0010;      ///< offset relative to unit base address
    const uint32_t  ENABLE_CONVERSION_MASK  = (1 << 0);     ///< bit[0] = Enable RGB to YCbCr conversion

}

#endif
