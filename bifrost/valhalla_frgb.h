/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_frgb.h
 *
 *  Created on: Nov 18, 2013
 *      Author: Jonas Romfelt
 *
 * Register map FPGA Valhalla frame grab block.
 *
 */

#ifndef VALHALLA_FRGB_H_
#define VALHALLA_FRGB_H_

#define VALHALLA_ADDR_FRGB_SOURCE                        0x00000010 // Video source to record
#define VALHALLA_ADDR_FRGB_OFFSET                        0x00000014 // Start address used for multi-frame recording
#define VALHALLA_ADDR_FRGB_FRAMES                        0x00000018 // Number of frames to record (must first be set to 0, started when set to > 0), it may
                                                                                             // take up to one frame period before the status register reflects ongoing recording
#define VALHALLA_ADDR_FRGB_STATUS                        0x00000080 // Status register, number of frames left record, when == 0 recording is done

#define VALHALLA_FRGB_SOURCE_MASK                        0x0000000f
#define VALHALLA_FRGB_SOURCE_SHIFT                       0

// Source IDs in the video chain
#define VALHALLA_FRGB_SRC_CVI                            0x0 // raw video
#define VALHALLA_FRGB_SRC_TPG                            0x1 // testpattern
#define VALHALLA_FRGB_SRC_GOC                            0x2 // Gain & Offset compensated video
#define VALHALLA_FRGB_SRC_DPR                            0x3 // Dead pixel replaced video
#define VALHALLA_FRGB_SRC_MLU                            0x4 // Matlab upload video
#define VALHALLA_FRGB_SRC_MATLAB                         0x5 // Matlab video
#define VALHALLA_FRGB_SRC_OVU                            0x7 // Output video upload
#define VALHALLA_FRGB_SRC_EZOOM                          0x8 // Ezoom

#endif
