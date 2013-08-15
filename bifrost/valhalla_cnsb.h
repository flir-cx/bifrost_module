/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_dma.h
 *
 *  Created on: Mar 22, 2012
 *      Author: Jonas Romfelt <jonas.romfelt@flir.se>
 *
 * Register map FPGA Valhalla EZOOM controller block.
 *
 */

#ifndef VALHALLA_CNSB_H_
#define VALHALLA_CNSB_H_

#include <bifrost/valhalla_ezoom.h>

#define VALHALLA_ADDR_CNSB_REG_BASE            0x00001500u

#define VALHALLA_ADDR_CNSB_EZOOM_STEP          0x00000014u
#define VALHALLA_ADDR_CNSB_EZOOM_START         0x00000018u
#define VALHALLA_ADDR_CNSB_EZOOM_STOP          0x0000001Cu
#define VALHALLA_ADDR_CNSB_EZOOM_CONTROL       0x00000020u
#define VALHALLA_ADDR_CNSB_EZOOM_STATUS        0x00000080u

#define VALHALLA_ADDR_CNSB_EZOOM_SCALE_PMAX                 0x00004000 /**< Pre-max, last highest value before wrap to 0 */
#define VALHALLA_ADDR_CNSB_EZOOM_SCALE_MASK                 0x00003fff /**< Pre-max, last highest value before wrap to 0 */

#endif
