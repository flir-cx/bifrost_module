/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_fanm.h
 *
 *  Created on: Nov 11, 2013
 *      Author: Jonas Romfelt <jonas.romfelt@flir.se>
 *
 * Register map FPGA Valhalla SHTx block.
 *
 */

#ifndef VALHALLA_SHTX_H_
#define VALHALLA_SHTX_H_

#define VALHALLA_ADDR_SHTX_REG_BASE                     0x00000700

#define VALHALLA_ADDR_SHTX_CONTROL                      0x00000010
#define VALHALLA_SHTX_CONTROL_SAMPLE_ALL_MASK           0x1 /* toggle from 0 to 1 starts new sampling of temperature and humidity */

#define VALHALLA_ADDR_SHTX_STATUS1                      0x00000080
#define VALHALLA_SHTX_STATUS1_SAMPLE_COMPLETED_MASK     0x2 /* when set, readings are available */

#define VALHALLA_ADDR_SHTX_STATUS3                      0x00000088
#define VALHALLA_SHTX_STATUS3_TEMPERATURE_SHIFT         0
#define VALHALLA_SHTX_STATUS3_TEMPERATURE_MASK          0x0000ffff
#define VALHALLA_SHTX_STATUS3_HUMIDITY_SHIFT            16
#define VALHALLA_SHTX_STATUS3_HUMIDITY_MASK             0xffff0000

#endif
