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

#ifndef VALHALLA_EZOOM_H_
#define VALHALLA_EZOOM_H_

#define VALHALLA_ADDR_EZOOM_REG_BASE             0x00001d00

#define VALHALLA_ADDR_EZOOM_MDS0_CONTROL1        0x00000010
#define VALHALLA_ADDR_EZOOM_CAMVIDEO_SIZE        0x00000030
#define VALHALLA_ADDR_EZOOM_OUTVIDEO_SIZE        0x00000034
#define VALHALLA_ADDR_EZOOM_CONTROL              0x00000038
#define VALHALLA_ADDR_EZOOM_X_FACTOR             0x0000003c // X scale factor (ZLFACT)
#define VALHALLA_ADDR_EZOOM_Y_FACTOR             0x00000040 // Y scale factor (ZPFACT)

#define VALHALLA_EZOOM_CONTROL_ENABLE            (1 << 0) // enable ezoom
#define VALHALLA_EZOOM_CONTROL_INTERPOLATE       (1 << 1) // enable interpolation
#define VALHALLA_EZOOM_CONTROL_EXTCOLROW         (1 << 2)

/* FIXME: 1000 is better to use then 3ff */
#define VALHALLA_EZOOM_SCALE_PMAX                 0x000003ff /**< Pre-max, last highest value before wrap to 0 */

#define VALHALLA_EZOOM_SCALE_FACT_BITS           14
#define VALHALLA_EZOOM_SCALE_FACT_MASK           ((1 << VALHALLA_EZOOM_SCALE_FACT_BITS) -1)
#define VALHALLA_EZOOM_SCALE_FACT_SHIFT          0
#define VALHALLA_EZOOM_SCALE_FRAC_BITS           10
#define VALHALLA_EZOOM_SCALE_FRAC_MASK           ((1 << VALHALLA_EZOOM_SCALE_FRAC_BITS) -1)
#define VALHALLA_EZOOM_SCALE_FRAC_SHIFT          16

#define VALHALLA_ADDR_EZOOM_MDS0_CONTROL1_FREEZE 0xFFFFFFFF /**< write this to MSD0_CONTROL1 to freeze image */

#endif
