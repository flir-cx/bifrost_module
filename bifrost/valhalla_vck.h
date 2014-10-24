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
    /* I2C_DEVICEADDR */
    const uint32_t      REG_VCK_I2C_DEV_ADDR    = 0x0010;       ///< offset relative to unit base address
    const uint32_t      I2C_DEV_ADDR_MASK       = (0xFF << 0);  ///< bit[7:0]= I2C Device address
    const uint_fast8_t  I2C_DEV_ADDR_SHIFT      = 0;

    /* I2C_REGISTERADDR */
    const uint32_t      REG_VCK_I2C_ADDRESS     = 0x0014;       ///< offset relative to unit base address
    const uint32_t      I2C_REG_ADDR_MASK       = (0xFF << 0);  ///< bit[7:0]= I2C Register address (i.e command)
    const uint_fast8_t  I2C_REG_ADDR_SHIFT      = 0;

    /* I2C_WRITE_REG */
    const uint32_t      REG_VCK_I2C_WR_DATA     = 0x0018;       ///< offset relative to unit base address
    // bit[31:0] = I2C write data

    /* I2C_RST */
    const uint32_t      REG_VCK_I2C_RST         = 0x0024;       ///< offset relative to unit base address
    const uint32_t      I2C_RESET_MASK          = (1 << 0);     ///< bit[0]= I2C Reset, active low

    /* I2C_RUN_SEQ */
    const uint32_t      REG_VCK_I2C_RUN_SEQ     = 0x0028;       ///< run i2c command sequence
    const uint32_t      REG_VCK_SYNC_SRC_SEL    = 0x0034;

    /* STA_VCK_I2C_RD_DATA */
    const uint32_t      STA_VCK_I2C_RD_DATA     = 0x0080;       ///< offset relative to unit base address
    // bit[31:0] = I2C read data

    /* STA_VCK_I2C_DONE */
    const uint32_t      STA_VCK_I2C_DONE        = 0x0088;

}

#endif
