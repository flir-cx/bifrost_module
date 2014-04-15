/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_ovla.h
 *
 *  Created on: Mar 22, 2012
 *      Author: Jonas Zetterberg <jonas.zetterberg@flir.se>
 *
 * Register map FPGA Valhalla Overlay controller block.
 *
 */

#ifndef VALHALLA_OVERLAY_H_
#define VALHALLA_OVERLAY_H_

#define VALHALLA_ADDR_OVERLAY_REG_BASE             0x00001a00

#define VALHALLA_ADDR_OVERLAY_CONTROL              0x00000014
#define VALHALLA_ADDR_OVERLAY_COLOR                0x00000018 /**< Control register 2, color as #AARRGGBB */
#define VALHALLA_ADDR_OVERLAY_OFFSET               0x0000001c
#define VALHALLA_OVERLAY_CONTROL_ENABLE_BIT        0
#define VALHALLA_OVERLAY_CONTROL_ALPHA_SHIFT       16
#define VALHALLA_OVERLAY_CONTROL_ALPHA_BITS        8
#define VALHALLA_OVERLAY_CONTROL_ALPHA_MASK        0xff
#define VALHALLA_OVERLAY_COLOR_ALPHA_SHIFT         24

#endif
