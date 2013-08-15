/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_tpg.h
 *
 *  Created on: Dec 07, 2012
 *      Author: Jonas Romfelt
 *
 * Register map FPGA Valhalla test pattern generator block.
 *
 */

#ifndef VALHALLA_TPG_H_
#define VALHALLA_TPG_H_

#define VALHALLA_ADDR_TPG_BASE                            0x00000900

#define VALHALLA_ADDR_TPG_SIZE                            0x00000010 // 26:16 rows, 10:0 cols
#define VALHALLA_ADDR_TPG_CONTROL1                        0x00000014
#define VALHALLA_ADDR_TPG_CONTROL2                        0x00000018

#define VALHALLA_TPG_CONTROL1_PATTERN_MASK                (0x1f << 0)
#define VALHALLA_TPG_CONTROL1_PATTERN_OFF                 (0x00 << 0)
#define VALHALLA_TPG_CONTROL1_PATTERN_CROSS               (0x01 << 0)
#define VALHALLA_TPG_CONTROL1_PATTERN_RAMP                (0x07 << 0)
#define VALHALLA_TPG_CONTROL1_PATTERN_HIRAMP              (0x0f << 0) // high resolution ramp
#define VALHALLA_TPG_CONTROL1_PATTERN_10COL               (0x02 << 0)
#define VALHALLA_TPG_CONTROL1_PATTERN_ASCENDING           (0x03 << 0)
#define VALHALLA_TPG_CONTROL1_PATTERN_VERTICAL            (0x04 << 0)
#define VALHALLA_TPG_CONTROL1_PATTERN_HORIZONTAL          (0x05 << 0)
#define VALHALLA_TPG_CONTROL1_PATTERN_HORIZONTAL_VERTICAL (0x06 << 0)
#define VALHALLA_TPG_CONTROL1_PATTERN_CROSS_FLASH         (0x09 << 0)

#define VALHALLA_TPG_CONTROL1_PATTERN_OFFSET_SHIFT        16
#define VALHALLA_TPG_CONTROL1_PATTERN_OFFSET_MASK         (0xffff << (VALHALLA_TPG_CONTROL1_PATTERN_OFFSET_SHIFT))

#define VALHALLA_TPG_CONTROL2_PATTERN_DIFF_SHIFT          0
#define VALHALLA_TPG_CONTROL2_PATTERN_DIFF_MASK           (0xffff << (VALHALLA_TPG_CONTROL2_PATTERN_DIFF_SHIFT)) // pattern diff offset

#endif
