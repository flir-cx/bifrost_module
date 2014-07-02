/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_goc.h
 *
 *  Created on: Mar 22, 2012
 *      Author: Jonas Romfelt, Emanuel Johansson
 *
 * Register map FPGA Valhalla Gain and Offset Correction controller block.
 *
 */

#ifndef VALHALLA_GOC_H_
#define VALHALLA_GOC_H_

#define VALHALLA_ADDR_GOC_REG_BASE            0x00000F00

#define VALHALLA_ADDR_GOC_CONTROL1            0x00000014
#define VALHALLA_ADDR_GAIN_MAP_OFFSET         0x0000001c
#define VALHALLA_ADDR_OFFSET_MAP_OFFSET       0x00000020
#define VALHALLA_ADDR_COARSE_MAP_OFFSET       0x00000024
#define VALHALLA_ADDR_GOC_CONTROL2            0x0000002c

#define VALHALLA_GAIN_MAP_OFFSET_VALUE        0x00800000
#define VALHALLA_OFFSET_MAP_OFFSET_VALUE      0x00c00000
#define VALHALLA_COARSE_MAP_OFFSET_VALUE      0x00a00000

#define VALHALLA_GOC_CONTROL1_GAIN_ENABLE     (1 << 0) // enable gain map
#define VALHALLA_GOC_CONTROL1_DEAD_SELECT     (1 << 1) // if set, set value of pixel if gain data is 0
#define VALHALLA_GOC_CONTROL1_CLIP_ENABLE     (1 << 3) // enable clip to prevent pixels to wrap-around

#define VALHALLA_GOC_CONTROL2_OFFSET_ENABLE   (1 << 1) // enable offset map
#define VALHALLA_GOC_CONTROL2_COARSE_ENABLE   (1 << 2) // enable coarse map

#endif
