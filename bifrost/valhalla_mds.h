/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_mds.h
 *
 *  Created on: Dec 11, 2012
 *      Author: Jonas Romfelt
 *
 * Register map FPGA Valhalla memory download stream block.
 *
 */

#ifndef VALHALLA_MDS_H_
#define VALHALLA_MDS_H_

#define VALHALLA_ADDR_MDS_BASE                          0x00001000

#define VALHALLA_ADDR_MDS_0FFSET                        (VALHALLA_ADDR_MDS_BASE + 0x0010) // memory offset where to store data
#define VALHALLA_ADDR_MDS_CONTROL                       (VALHALLA_ADDR_MDS_BASE + 0x0014)
#define VALHALLA_ADDR_MDS_ADDRESS                       (VALHALLA_ADDR_MDS_BASE + 0x0018) // start address used for multi-frame recording
#define VALHALLA_ADDR_MDS_FRAMES                        (VALHALLA_ADDR_MDS_BASE + 0x001c) // number of frames to record (must first be set to 0, started when set to > 0)
#define VALHALLA_ADDR_MDS_STATUS                        (VALHALLA_ADDR_MDS_BASE + 0x0080) // status register, number of frames left record, when == 0 recording is done

#define VALHALLA_MDS_CONTROL_RECORD_SRC_MASK            0x0000000f // tap ID that specifies video that is recorded
#define VALHALLA_MDS_CONTROL_BT_SRC_MASK                0x000000f0 // tap ID that specifies video that is sent to CPU

// Source tap IDs in the video chain
#define VALHALLA_MDS_SRC_RAW                            0x0 // Buffered raw video chain 1
#define VALHALLA_MDS_SRC_TESTPATTERN                    0x1 // Testpattern video chain 1
#define VALHALLA_MDS_SRC_GOC                            0x2 // Gain & Offset compensated video chain 1
#define VALHALLA_MDS_SRC_DPR                            0x3 // Dead pixel replaced video chain 1
#define VALHALLA_MDS_SRC_TENENGRAD                      0x5 // Tenengrad video chain 1
#define VALHALLA_MDS_SRC_ADE                            0x6 // ADE filtered video chain 1
#define VALHALLA_MDS_SRC_ATF                            0x7 // ATF filtered video chain 1
#define VALHALLA_MDS_SRC_TESTPATTERN2                   0x9 // Testpattern video chain 2

#endif
