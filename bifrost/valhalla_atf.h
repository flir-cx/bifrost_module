/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_atf.h
 *
 *  Created on: Oct 3, 2012
 *      Author: jromfelt
 *
 * Register map FPGA Valhalla ATF block.
 *
 */

#ifndef VALHALLA_ATF_H_
#define VALHALLA_ATF_H_

#define VALHALLA_ADDR_MATLAB_REG_BASE               0x00008000

#define VALHALLA_ADDR_ATF_REG_BASE                  (VALHALLA_ADDR_MATLAB_REG_BASE + 0x00000300)

#define VALHALLA_ATF_COMPATIBLE_API_VER             3


#define VALHALLA_ADDR_ATF_CONTROL                   0x0040
#define VALHALLA_ADDR_ATF_OUT_MUX_SELECT            0x0044
#define VALHALLA_ADDR_ATF_NF                        0x0048

#define VALHALLA_ATF_CONTROL_BYPASS_ENABLE          (1 << 0)
#define VALHALLA_ATF_NF_MAX                         65535

#define VALHALLA_ATF_OUT_MUX_SELECT_OUTPUT          0x0
#define VALHALLA_ATF_OUT_MUX_SELECT_BYPASS          0x1
//#define VALHALLA_ATF_OUT_MUX_SELECT_WEIGHTS         0x2
//#define VALHALLA_ATF_OUT_MUX_SELECT_CURRENT_FRAME   0x3
//#define VALHALLA_ATF_OUT_MUX_SELECT_PREV_FRAME      0x4
//#define VALHALLA_ATF_OUT_MUX_SELECT_DIFF_FRAME      0x5

#define VALHALLA_ADDR_ATF_PAN_CTRL                  0x005C
#define VALHALLA_ATF_MOVE_READ_PANNING_MASK         (1 << 0)

#define VALHALLA_ADDR_ATF_NF_GLOB                   0x0060
#define VALHALLA_ATF_NF_GLOB_MASK                   0xFFFF

#define VALHALLA_ADDR_ATF_PAN_QUAL                  0x0064
#define VALHALLA_ATF_PAN_QUAL_MASK                  0x0FFF

#endif /* VALHALLA_ATF_H_ */
