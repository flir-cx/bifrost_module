/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_sdi0.h
 *
 *  Created on: Nov 15, 2013
 *      Author: Jonas Lundberg <jonas.lundberg@flir.se>
 *
 * Register map for FPGA Valhalla SDI block.
 *
 */

#ifndef VALHALLA_SDI0_H_
#define VALHALLA_SDI0_H_

namespace VALHALLA_SDI0
{
    const uint32_t  BASE_ADDRESS              =  0x00007000;

    /* REG_POWER_DOWN_CH0 */
    const uint32_t  REG_POWER_DOWN_CH0_OFFSET =  0x0144;    ///< offset relative to unit base address
    const uint32_t  GTX_TX_POWER_DOWN_MASK    = (1 << 0);   ///< bit[0] = GTX TX Power Down
    const uint32_t  TX_USER_RESET_MASK        = (1 << 1);   ///< bit[1] = TX User Reset
    const uint32_t  GTX_RX_POWER_DOWN_MASK    = (1 << 2);   ///< bit[2] = GTX RX Power Down
    const uint32_t  RX_USER_RESET_MASK        = (1 << 3);   ///< bit[3] = RX User Reset
}

#endif
