/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_ycon.h
 *
 *  Created on: Mar 13, 2013
 *      Author: Jonas Romfelt
 *
 * Register map FPGA Valhalla YCON block.
 *
 */

#ifndef VALHALLA_YCON_H_
#define VALHALLA_YCON_H_

#define VALHALLA_ADDR_YCON_BASE                      0x00000e00 // default base address for Valhalla

#define VALHALLA_ADDR_YCON_CONTROL                   0x00000020 // bit[0] = polarity

#define VALHALLA_YCON_CONTROL_POLARITY               (1 << 0)

#endif
