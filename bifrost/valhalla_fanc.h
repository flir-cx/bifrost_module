/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_fanc.h
 *
 *  Created on: Nov 8, 2012
 *      Author: Jonas Lundberg <jonas.lundberg@flir.se>
 *
 * Register map FPGA Valhalla Fan Control block.
 *
 */

#ifndef VALHALLA_FANC_H_
#define VALHALLA_FANC_H_

#define VALHALLA_ADDR_FANC_REG_BASE           0x00001400

/* relative unit base offset to FAN_ON control */
#define VALHALLA_ADDR_FANC_FAN_ON_BASE        0x0010    // bit[7:4] = FAN_ON[3:0]

/* relative unit base offset that holds speed values */
#define VALHALLA_ADDR_FANC_SPEED_BASE         0x0080    // bit[31:0] = Speed fan x

/* number of fans */
#define VALHALLA_FANC_NO_OF_FANS              4

/* masks for fan on/off control */
#define VALHALLA_FANC_FAN0_MASK               0x10      // bit[4] = FAN_ON[0]
#define VALHALLA_FANC_FAN1_MASK               0x20      // bit[5] = FAN_ON[1]
#define VALHALLA_FANC_FAN2_MASK               0x40      // bit[6] = FAN_ON[2]
#define VALHALLA_FANC_FAN3_MASK               0x80      // bit[7] = FAN_ON[3]

#endif
