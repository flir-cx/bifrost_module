/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_dpr.h
 *
 *  Created on: Oct 09, 2012
 *      Author: Jonas Romfelt
 *
 * Register map FPGA Valhalla Dead Pixel Replacement controller block.
 *
 */

#ifndef VALHALLA_DPR_H_
#define VALHALLA_DPR_H_

#define VALHALLA_ADDR_DPR_BASE                      0x00001600

#define VALHALLA_ADDR_DPR_MAP_OFFSET                0x00000014
#define VALHALLA_ADDR_DPR_MAP_CONTROL               0x00000018

#define VALHALLA_DPR_MAP_OFFSET_VALUE               0x01000000

#define VALHALLA_DPR_MAP_CONTROL_ENABLE             (1 << 0)

#define VALHALLA_DPR_MAP_CONTROL_DEAD_DEBUG_MASK    (3 << 1)
#define VALHALLA_DPR_MAP_CONTROL_DEAD_DEBUG_OFF     (0 << 1) // debug off, replace
#define VALHALLA_DPR_MAP_CONTROL_DEAD_DEBUG_DEAD    (1 << 1) // mark with 0xdead
#define VALHALLA_DPR_MAP_CONTROL_DEAD_DEBUG_ZEROES  (2 << 1) // mark with 0x0
#define VALHALLA_DPR_MAP_CONTROL_DEAD_DEBUG_ONES    (3 << 1) // mark with 0xffff

#endif
