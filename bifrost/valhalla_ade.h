/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_ade.h
 *
 *  Created on: Sept 26, 2012
 *      Author: jromfelt
 *
 * Register map FPGA Valhalla ADE block.
 *
 */

#ifndef VALHALLA_ADE_H_
#define VALHALLA_ADE_H_

#define VALHALLA_ADDR_MATLAB_REG_BASE           0x00008000

#define VALHALLA_ADDR_ADE_REG_BASE              (VALHALLA_ADDR_MATLAB_REG_BASE + 0x00000400)

/*
 * ROI
 *
 * NOTE relative unit offsets!
 */

#define VALHALLA_ADDR_ADE_ROI_UL                0x0228 // Upper Left Corner [31:16]=x, [15:0]=y
#define VALHALLA_ADDR_ADE_ROI_DR                0x022c // Down Right Corner [31:16]=x, [15:0]=y

#define VALHALLA_ADDR_ADE_ROI_X_COORD_MASK      0xffff
#define VALHALLA_ADDR_ADE_ROI_X_COORD_SHIFT     16
#define VALHALLA_ADDR_ADE_ROI_Y_COORD_MASK      0xffff
#define VALHALLA_ADDR_ADE_ROI_Y_COORD_SHIFT     0

/*
 * Module ADE
 */

#define VALHALLA_ADDR_ADE_OUT_MUX_SELECT        0x0110
#define VALHALLA_ADDR_ADE_LP_GAIN               0x0130
#define VALHALLA_ADDR_ADE_HP_GAIN               0x0154
#define VALHALLA_ADDR_ADE_STREAM_SELECT         0x0148
#define VALHALLA_ADDR_ADE_THRESHOLD             0x0118

#define VALHALLA_ADE_OUT_MUX_SELECT_OUTPUT      0x00000000
#define VALHALLA_ADE_OUT_MUX_SELECT_ALP_H       0x00000003
#define VALHALLA_ADE_OUT_MUX_SELECT_ALP_V_LP    0x00000005
#define VALHALLA_ADE_OUT_MUX_SELECT_ALP_V_HP    0x00000006
#define VALHALLA_ADE_OUT_MUX_SELECT_ADD_DETAIL  0x0000000b
#define VALHALLA_ADE_OUT_MUX_SELECT_ADJ_BYPASS  0x00000011

#define VALHALLA_ADE_STREAM_SELECT_BYPASS       0x00000000
#define VALHALLA_ADE_STREAM_SELECT_LOWPASS      0x00000001

#define VALHALLA_ADE_GAIN_MAX                   0x0000ffff
#define VALHALLA_ADE_THRESHOLD_MAX              0x0000ffff

/*
 * ADJ
 */

#define VALHALLA_ADDR_ADE_ADJ_MODE              0x017c
#define VALHALLA_ADDR_ADE_ADJ_MAN_GAIN          0x0180
#define VALHALLA_ADDR_ADE_ADJ_MAN_OFFSET        0x0184

#define VALHALLA_ADE_ADJ_MODE_NORMAL            0x0
#define VALHALLA_ADE_ADJ_MODE_CONTINUOUS        0x1
#define VALHALLA_ADE_ADJ_MODE_MANUAL            0x2
#define VALHALLA_ADE_ADJ_MODE_ONESHOT           0x3

/* 13-bits signed */
#define VALHALLA_ADE_ADJ_MAN_GAIN_MAX           (0x1FFF >> 1)
#define VALHALLA_ADE_ADJ_MAN_OFFSET_MAX         (0x1FFF >> 1)

/*
 * Histogram
 */

#define VALHALLA_ADDR_ADE_LOW_BIN_LIMIT         0x0124 /* head percentage */
#define VALHALLA_ADDR_ADE_HIGH_BIN_LIMIT        0x0128 /* tail percentage */
#define VALHALLA_ADDR_ADE_PREV_ALPHA_VALUE      0x012C
#define VALHALLA_ADDR_ADE_MAX_BIN_VAL           0x0174 /* saturate bin values at this limit */

#define VALHALLA_ADDR_ADE_LOW_BIN               0x0140 /* head bin */
#define VALHALLA_ADDR_ADE_HIGH_BIN              0x0144 /* tail bin */

#define VALHALLA_ADDR_ADE_HIST_ADD_LUT          0x2400 /* start of 8-bit LUT */
#define VALHALLA_ADDR_ADE_HISTOGRAM             0x3c00 /* start of HIST */

#define VALHALLA_ADE_HIST_ADD_LUT_DEPTH         256
#define VALHALLA_ADE_HISTOGRAM_BINS             4096
#define VALHALLA_ADE_BIN_LIMIT_MAX              65535
#define VALHALLA_ADE_PREV_ALPHA_VALUE_MAX       4096
#define VALHALLA_ADE_MAX_BIN_VAL_MAX            0xffffffff

/*
 * HEQ
 */

#define VALHALLA_ADDR_ADE_EDGE_GAIN_HP          0x014c /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_HE_FRAC_GAIN          0x015c /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_HE_FRAC_OFFSET        0x0158 /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_FOOT_OFFSET           0x0160 /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_FOOT_GAIN             0x0164 /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_HEAD_OFFSET           0x0168 /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_HEAD_GAIN             0x016c /* 0 - 65535 */

#define VALHALLA_ADE_HE_FRAC_GAIN_MAX           65535
#define VALHALLA_ADE_HE_FRAC_OFFSET_MAX         65535
#define VALHALLA_ADE_FOOT_GAIN_MAX              65535
#define VALHALLA_ADE_HEAD_GAIN_MAX              65535
#define VALHALLA_ADE_FOOT_OFFSET_MAX            65535
#define VALHALLA_ADE_HEAD_OFFSET_MAX            65535

/*
 * ADD
 */

#define VALHALLA_ADDR_ADE_DET_GAIN              0x014C /* amount of detail gain */
#define VALHALLA_ADDR_ADE_DET_GAIN_MAX          0x0114 /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_DET_GAIN_MIN          0x0178 /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_ADD_LUT               0x2800 /* start of 16-bit LUT */

#define VALHALLA_ADE_ADD_LUT_DEPTH              1024
#define VALHALLA_ADE_DET_GAIN_MAX               65535

#endif
