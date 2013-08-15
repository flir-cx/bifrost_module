/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_stat_tgrd.h
 *
 *  Created on: Dec 07, 2012
 *      Author: Jonas Romfelt
 *
 * Register map FPGA Valhalla  tenengrad statistics block.
 *
 */

#ifndef VALHALLA_STAT_TGRD_H_
#define VALHALLA_STAT_TGRD_H_

#define VALHALLA_ADDR_STAT_TGRD_BASE            0x00001200

#define VALHALLA_ADDR_STAT_TGRD_SIZE            0x00000010 // 26:16 rows, 10:0 cols
#define VALHALLA_ADDR_STAT_TGRD_CONTROL         0x00000014
#define VALHALLA_ADDR_STAT_TGRD_ROIH            0x00000018 // region of interest: 26:16 right, 10:0 left
#define VALHALLA_ADDR_STAT_TGRD_ROIV            0x0000001c // region of interest: 26:16 bottom, 10:0 top
#define VALHALLA_ADDR_STAT_TGRD_MAGNITUDE_SUM   0x00000080 // focus value: 31 = 0, 30:25 leading zeroes, 24:16 MS-word, 15 = 1, 14:0 LS-word

#define VALHALLA_STAT_TGRD_CONTROL_ENABLE       (1 << 0)
#define VALHALLA_STAT_TGRD_CONTROL_DISPLAY      (1 << 1) // show Tenengrad output for debug

#define VALHALLA_STAT_TGRD_ROI_COORD1_MASK      0x000007ff
#define VALHALLA_STAT_TGRD_ROI_COORD1_SHIFT     0
#define VALHALLA_STAT_TGRD_ROI_COORD2_MASK      0x000007ff
#define VALHALLA_STAT_TGRD_ROI_COORD2_SHIFT     16

#endif
