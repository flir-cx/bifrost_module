/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_stat.h
 *
 *  Created on: Feb 28, 2013
 *      Author: Jonas Romfelt
 *
 * Register map FPGA Valhalla statistics block.
 *
 */

#ifndef VALHALLA_STAT_H_
#define VALHALLA_STAT_H_

#define VALHALLA_ADDR_STAT_BASE                 0x00001c00 // default unit offset

#define VALHALLA_ADDR_STAT_SIZE                 0x00000010 // 26:16 rows, 10:0 cols

#define VALHALLA_ADDR_STAT_CONTROL              0x00000014 // bit[0] = ROI_SUM_EN, set to one to run a ROI_PIXEL_SUM calculation.
                                                            // bit[1] = ROI_SQSUM_EN, set to one to run a ROI_SQPIXEL_SUM calculation.
                                                            // bit[2] = FRAME_SUM_EN, set to one to run a IMAGE_PIXEL_SUM calculation.

#define VALHALLA_ADDR_STAT_ROIH                 0x00000018 // region of interest: 26:16 right, 10:0 left
#define VALHALLA_ADDR_STAT_ROIV                 0x0000001c // region of interest: 26:16 bottom, 10:0 top

#define VALHALLA_ADDR_STAT_PIXEL_SUM0           0x00000080 // bit[31:0] = ROI_PIXEL_SUM[31:0], pixel sum of ROI
#define VALHALLA_ADDR_STAT_PIXEL_SUM1           0x00000084 // bit[4:0] = ROI_PIXEL_SUM[36:32], pixel sum of ROI
#define VALHALLA_ADDR_STAT_SQPIXEL_SUM0         0x00000088 // bit[31:0] = ROI_SQPIXEL_SUM[31:0], square pixel sum of ROI
#define VALHALLA_ADDR_STAT_SQPIXEL_SUM1         0x0000008c // bit[20:0] = ROI_SQPIXEL_SUM[52:32], square pixel sum of ROI
#define VALHALLA_ADDR_STAT_FRAME_PIXEL_SUM0     0x00000090 // bit[31:0] = IMAGE_PIXEL_SUM[31:0], pixel sum of complete image
#define VALHALLA_ADDR_STAT_FRAME_PIXEL_SUM1     0x00000094 // bit[4:0] = IMAGE_PIXEL_SUM[36:32], pixel sum of complete image

#define VALHALLA_ADDR_STAT_STATUS               0x00000098 // bit[0] = ROI_SUM_RDY, set to one after the ROI_PIXEL_SUM calculation has been completed.
                                                            // bit[1] = ROI_SQSUM_RDY, set to one after the ROI_SQPIXEL_SUM calculation has been completed.
                                                            // bit[2] = FRAME_SUM_RDY, set to one after the IMAGE_PIXEL_SUM calculation has been completed.
#define VALHALLA_STAT_CONTROL_START_PIXEL_SUM   (1 << 0)
#define VALHALLA_STAT_CONTROL_START_SQPIXEL_SUM (1 << 1)
#define VALHALLA_STAT_CONTROL_START_FRAME_SUM   (1 << 2)

#define VALHALLA_STAT_STATUS_DONE_PIXEL_SUM     (1 << 0)
#define VALHALLA_STAT_STATUS_DONE_SQPIXEL_SUM   (1 << 1)
#define VALHALLA_STAT_STATUS_DONE_FRAME_SUM     (1 << 2)

#define VALHALLA_STAT_PIXEL_SUM1_MASK           0x0000001f
#define VALHALLA_STAT_SQPIXEL_SUM1_MASK         0x001fffff
#define VALHALLA_STAT_FRAME_PIXEL_SUM1_MASK     0x0000001f

#define VALHALLA_STAT_PIXEL_SUM_MAX             ((((uint64_t)(VALHALLA_STAT_PIXEL_SUM1_MASK)) << 32) | 0xffffffffLL)
#define VALHALLA_STAT_SQPIXEL_SUM_MAX           ((((uint64_t)(VALHALLA_STAT_SQPIXEL_SUM1_MASK)) << 32) | 0xffffffffLL)
#define VALHALLA_STAT_FRAME_PIXEL_SUM_MAX       ((((uint64_t)(VALHALLA_STAT_FRAME_PIXEL_SUM1_MASK)) << 32) | 0xffffffffLL)

#define VALHALLA_STAT_ROI_COORD1_MASK           0x000007ff
#define VALHALLA_STAT_ROI_COORD1_SHIFT          0
#define VALHALLA_STAT_ROI_COORD2_MASK           0x000007ff
#define VALHALLA_STAT_ROI_COORD2_SHIFT          16

#endif
