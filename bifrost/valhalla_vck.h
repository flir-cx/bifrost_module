/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_vck.h
 *
 *  Created on: Nov 18, 2013
 *      Author: Jonas Lundberg <jonas.lundberg@flir.se>
 *
 * Register map for FPGA Valhalla Video Clocks block.
 *
 */

#ifndef VALHALLA_VCK_H_
#define VALHALLA_VCK_H_

namespace VALHALLA_VCK
{
    const uint32_t      BASE_ADDRESS          =  0x00000B00;

    /* I2C_RST */
    const uint32_t      I2C_RST_OFFSET        =  0x0010;      ///< offset relative to unit base address
    const uint32_t      I2C_RESET_MASK        = (1 << 0);     ///< bit[0]= I2C Reset, active low

    /* I2C_DEVICEADDR */
    const uint32_t      I2C_DEVICEADDR_OFFSET =  0x0018;      ///< offset relative to unit base address
    const uint32_t      I2C_DEVICEADDR_MASK   = (0xFF << 0);  ///< bit[7:0]= I2C Device address
    const uint_fast8_t  I2C_DEVICEADDR_SHIFT  =  0;

    /* I2C_WRITE_REG */
    const uint32_t      I2C_WRITE_REG_OFFSET  =  0x0020;      ///< offset relative to unit base address
    // bit[31:0]= I2C write data
}

#endif
