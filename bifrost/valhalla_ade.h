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

#define VALHALLA_ADDR_ADE_REG_BASE              (VALHALLA_ADDR_MATLAB_REG_BASE + 0x00000500)

/*
 * ROI
 *
 * NOTE relative unit offsets!
 */

#define VALHALLA_ADDR_ADE_ROI_UL                0x0128 // Upper Left Corner [31:16]=x, [15:0]=y
#define VALHALLA_ADDR_ADE_ROI_DR                0x012c // Down Right Corner [31:16]=x, [15:0]=y

#define VALHALLA_ADDR_ADE_ROI_X_COORD_MASK      0xffff
#define VALHALLA_ADDR_ADE_ROI_X_COORD_SHIFT     16
#define VALHALLA_ADDR_ADE_ROI_Y_COORD_MASK      0xffff
#define VALHALLA_ADDR_ADE_ROI_Y_COORD_SHIFT     0

/*
 * Module ADE
 */

#define VALHALLA_ADDR_ADE_OUT_MUX_SELECT        (VALHALLA_ADDR_ADE_REG_BASE + 0x0010)
#define VALHALLA_ADDR_ADE_LP_GAIN               (VALHALLA_ADDR_ADE_REG_BASE + 0x0030)
#define VALHALLA_ADDR_ADE_HP_GAIN               (VALHALLA_ADDR_ADE_REG_BASE + 0x0054)
#define VALHALLA_ADDR_ADE_STREAM_SELECT         (VALHALLA_ADDR_ADE_REG_BASE + 0x0048)
#define VALHALLA_ADDR_ADE_THRESHOLD             (VALHALLA_ADDR_ADE_REG_BASE + 0x0018)

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

#define VALHALLA_ADDR_ADE_ADJ_MODE              (VALHALLA_ADDR_ADE_REG_BASE + 0x007c)
#define VALHALLA_ADDR_ADE_ADJ_MAN_GAIN          (VALHALLA_ADDR_ADE_REG_BASE + 0x0080)
#define VALHALLA_ADDR_ADE_ADJ_MAN_OFFSET        (VALHALLA_ADDR_ADE_REG_BASE + 0x0084)

#define VALHALLA_ADE_ADJ_MODE_NORMAL            0x0
#define VALHALLA_ADE_ADJ_MODE_CONTINUOUS        0x1
#define VALHALLA_ADE_ADJ_MODE_MANUAL            0x2
#define VALHALLA_ADE_ADJ_MODE_ONESHOT           0x3

/* FIXME: Old values, Stefan is unsure which one should be correct. For now the 
 * defined values correspond to the FPGA implementation but have lower granultity
 * then the previously defined.
 */
//#define VALHALLA_ADE_ADJ_MAN_GAIN_MAX           (65535 / 2)
//#define VALHALLA_ADE_ADJ_MAN_OFFSET_MAX         (65535 / 2)
#define VALHALLA_ADE_ADJ_MAN_GAIN_MAX           (0x1FFF >> 1)
#define VALHALLA_ADE_ADJ_MAN_OFFSET_MAX         (0x1FFF >> 1)

/*
 * Histogram
 */

#define VALHALLA_ADDR_ADE_LOW_BIN_LIMIT         (VALHALLA_ADDR_ADE_REG_BASE + 0x0024) /* head percentage */
#define VALHALLA_ADDR_ADE_HIGH_BIN_LIMIT        (VALHALLA_ADDR_ADE_REG_BASE + 0x0028) /* tail percentage */
#define VALHALLA_ADDR_ADE_PREV_ALPHA_VALUE      (VALHALLA_ADDR_ADE_REG_BASE + 0x002C)
#define VALHALLA_ADDR_ADE_MAX_BIN_VAL           (VALHALLA_ADDR_ADE_REG_BASE + 0x0074) /* saturate bin values at this limit */

#define VALHALLA_ADDR_ADE_LOW_BIN               (VALHALLA_ADDR_ADE_REG_BASE + 0x0040) /* head bin */
#define VALHALLA_ADDR_ADE_HIGH_BIN              (VALHALLA_ADDR_ADE_REG_BASE + 0x0044) /* tail bin */

#define VALHALLA_ADDR_ADE_HIST_ADD_LUT          (VALHALLA_ADDR_ADE_REG_BASE + 0x2300) /* start of 8-bit LUT */
#define VALHALLA_ADDR_ADE_HISTOGRAM             (VALHALLA_ADDR_ADE_REG_BASE + 0x3b00) /* start of HIST */

#define VALHALLA_ADE_HIST_ADD_LUT_DEPTH         256
#define VALHALLA_ADE_HISTOGRAM_BINS             4096
#define VALHALLA_ADE_BIN_LIMIT_MAX              65535
#define VALHALLA_ADE_PREV_ALPHA_VALUE_MAX       4096
#define VALHALLA_ADE_MAX_BIN_VAL_MAX            0xffffffff

/*
 * HEQ
 */

#define VALHALLA_ADDR_ADE_EDGE_GAIN_HP          (VALHALLA_ADDR_ADE_REG_BASE + 0x004c) /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_HE_FRAC_GAIN          (VALHALLA_ADDR_ADE_REG_BASE + 0x005c) /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_HE_FRAC_OFFSET        (VALHALLA_ADDR_ADE_REG_BASE + 0x0058) /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_FOOT_OFFSET           (VALHALLA_ADDR_ADE_REG_BASE + 0x0060) /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_FOOT_GAIN             (VALHALLA_ADDR_ADE_REG_BASE + 0x0064) /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_HEAD_OFFSET           (VALHALLA_ADDR_ADE_REG_BASE + 0x0068) /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_HEAD_GAIN             (VALHALLA_ADDR_ADE_REG_BASE + 0x006c) /* 0 - 65535 */

#define VALHALLA_ADE_HE_FRAC_GAIN_MAX           65535
#define VALHALLA_ADE_HE_FRAC_OFFSET_MAX         65535
#define VALHALLA_ADE_FOOT_GAIN_MAX              65535
#define VALHALLA_ADE_HEAD_GAIN_MAX              65535
#define VALHALLA_ADE_FOOT_OFFSET_MAX            65535
#define VALHALLA_ADE_HEAD_OFFSET_MAX            65535

/*
 * ADD
 */

#define VALHALLA_ADDR_ADE_DET_GAIN              (VALHALLA_ADDR_ADE_REG_BASE + 0x004C) /* amount of detail gain */
#define VALHALLA_ADDR_ADE_DET_GAIN_MAX          (VALHALLA_ADDR_ADE_REG_BASE + 0x0014) /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_DET_GAIN_MIN          (VALHALLA_ADDR_ADE_REG_BASE + 0x0078) /* 0 - 65535 */
#define VALHALLA_ADDR_ADE_ADD_LUT               (VALHALLA_ADDR_ADE_REG_BASE + 0x2700) /* start of 16-bit LUT */

#define VALHALLA_ADE_ADD_LUT_DEPTH              1024
#define VALHALLA_ADE_DET_GAIN_MAX               65535

#endif
