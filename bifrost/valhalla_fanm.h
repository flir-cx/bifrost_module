/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_fanm.h
 *
 *  Created on: Oct 23, 2012
 *      Author: Jonas Romfelt <jonas.romfelt@flir.se>
 *
 * Register map FPGA Valhalla Fan Monitor block.
 *
 */

#ifndef VALHALLA_FAN_MONITOR_H_
#define VALHALLA_FAN_MONITOR_H_

#define VALHALLA_ADDR_FAN_MONITOR_REG_BASE           0x00001400

/* relative unit base offset that holds speed values */
#define VALHALLA_ADDR_FAN_MONITOR_SPEED_BASE         0x0080

/* number of fans */
#define VALHALLA_FAN_MONITOR_FANS                    5

#endif
