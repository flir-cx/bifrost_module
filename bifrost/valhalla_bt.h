/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_stat_tgrd.h
 *
 *  Created on: Dec 07, 2012
 *      Author: Jonas Romfelt
 *
 * Register map FPGA Valhalla BT controller block.
 *
 */

#ifndef VALHALLA_BT_H_
#define VALHALLA_BT_H_

#define VALHALLA_ADDR_BT_BASE                            0x00000700

#define VALHALLA_ADDR_BT_SIZE                            (VALHALLA_ADDR_BT_BASE + 0x0010) // 26:16 rows, 10:0 cols
#define VALHALLA_ADDR_BT_CONTROL                         (VALHALLA_ADDR_BT_BASE + 0x0014)
#define VALHALLA_ADDR_BT_BORDER                          (VALHALLA_ADDR_BT_BASE + 0x0018) // color: YCrCb
#define VALHALLA_ADDR_BT_BLANK                           (VALHALLA_ADDR_BT_BASE + 0x001c) // color: YCrCb

#define VALHALLA_BT_CONTROL_ENABLE                       (1 << 0)
#define VALHALLA_BT_CONTROL_BLANK                        (1 << 1) // blank video
#define VALHALLA_BT_CONTROL_STRETCH                      (1 << 2)
#define VALHALLA_BT_CONTROL_MISB0403                     (1 << 3) // by-pass YCrCb and 0403 encoded
#define VALHALLA_BT_CONTROL_ERROR_CLEAR                  (1 << 7)

#define VALHALLA_BT_CONTROL_VIDE0_MODE_MASK              (0xf << 8)
#define VALHALLA_BT_CONTROL_VIDE0_MODE_1080p60           (0x0 << 8)
#define VALHALLA_BT_CONTROL_VIDE0_MODE_1080p30           (0x1 << 8)
#define VALHALLA_BT_CONTROL_VIDE0_MODE_1080i60           (0x2 << 8)
#define VALHALLA_BT_CONTROL_VIDE0_MODE_1080p50           (0x3 << 8)
#define VALHALLA_BT_CONTROL_VIDE0_MODE_1080p25           (0x4 << 8)
#define VALHALLA_BT_CONTROL_VIDE0_MODE_1080i50           (0x5 << 8)
#define VALHALLA_BT_CONTROL_VIDE0_MODE_1080i24           (0x6 << 8)
#define VALHALLA_BT_CONTROL_VIDE0_MODE_720p60            (0x7 << 8)

#define VALHALLA_BT_CONTROL_LINE_REQUEST_MASK            (0xff << 16) // 0 .. 255

#endif
