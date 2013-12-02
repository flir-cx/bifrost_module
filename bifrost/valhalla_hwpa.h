/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_dma.h
 *
 *  Created on: Apr 23, 2013
 *      Author: Ari Mannila <ari.mannila@flir.se>
 *
 * Register map FPGA Valhalla HW-palette controller block.
 *
 */

#ifndef VALHALLA_HWPA_H_
#define VALHALLA_HWPA_H_

#define VALHALLA_ADDR_HWPA_BASE                     0x00001300u
#define VALHALLA_ADDR_HWPA_PALETTE_INVERT           0x00000020u

#define VALHALLA_ADDR_HWPA_SEGMENT_BASE             0x00005000u
#define VALHALLA_ADDR_HWPA_SEGMENT_TABLE_BASE       0x00000400
#define VALHALLA_ADDR_HWPA_DELTA_OFFSET_TABLE_BASE  0x00000000

#define VALHALLA_ADDR_HWPA_CB_DELTA                 0x00000001
#define VALHALLA_ADDR_HWPA_CR_DELTA                 0x00000002
#define VALHALLA_ADDR_HWPA_Y_DELTA                  0x00000003
#define VALHALLA_ADDR_HWPA_YCRCB_OFFSET             0x00000004

#define VALHALLA_HWPA_NUMBER_OF_SEGMENTS            41

#endif
