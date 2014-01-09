/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_nuc.h
 *
 *  Created on: Mar 06, 2013
 *      Author: Jonas Romfelt
 *
 * Register map FPGA Valhalla NUC block.
 *
 */

#ifndef VALHALLA_NUC_H_
#define VALHALLA_NUC_H_

#define VALHALLA_ADDR_NUC_BASE                      0x00001b00 // default base address for Valhalla

#define VALHALLA_ADDR_NUC_CONTROL                   0x00000014 // bit[0] = NUC_ENBLE, set one to start HW-NUC. bit[6:4]= NUMBER_FRAMES, defines number consecutive frames used for the HW-NUC. Range 1 to 128 frame: 2^NUMBER_FRAMES
#define VALHALLA_ADDR_NUC_FRAME_SUM                 0x00000018 // Pixel average value
#define VALHALLA_ADDR_NUC_ADDRESS                   0x0000001c // Start address of NUC calculation memory map
#define VALHALLA_ADDR_NUC_OFFSET_ADDRESS            0x00000020 // Start address of Offset memory map
#define VALHALLA_ADDR_NUC_STATUS                    0x00000080 // bit[0] = NUC_COMPLETED, indicates that the HW_NUC is completed

#define VALHALLA_NUC_CONTROL_START_NUC              (1 << 0)
#define VALHALLA_NUC_CONTROL_MEDIAN                 (1 << 1) // median filtering (used in optic NUC)
#define VALHALLA_NUC_CONTROL_LOWPASS                (1 << 2) // low pass filtering
#define VALHALLA_NUC_CONTROL_AVERAGE                (1 << 3) // average frame calculation only
#define VALHALLA_NUC_CONTROL_NUMBER_FRAMES_SHIFT    4
#define VALHALLA_NUC_CONTROL_NUMBER_FRAMES_MASK     (0x7 << (VALHALLA_NUC_CONTROL_NUMBER_FRAMES_SHIFT))

#define VALHALLA_NUC_STATUS_NUC_COMPLETED           (1 << 0)

#endif
