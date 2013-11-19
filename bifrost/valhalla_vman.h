/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_vman.h
 *
 *  Created on: Nov 11, 2013
 *      Author: Jonas Lundberg <jonas.lundberg@flir.se>
 *
 * Register map for FPGA Valhalla Video Management block.
 *
 */

#ifndef VALHALLA_VMAN_H_
#define VALHALLA_VMAN_H_

namespace VALHALLA_VMAN
{
    const uint32_t      BASE_ADDRESS                    =  0x00001100;

    /** relative unit base offset to SYSTEM SYNC control register */
    const uint32_t      SYSTEM_SYNC_CONTROL_OFFSET      =  0x0010;

    /* constants for SYSTEM_SYNC_CONTROL register */
    const uint32_t      SYSTEM_SYNC_ENABLE_MASK         = (1 << 0);       ///< bit[0] = System Sync Enable
    const uint32_t      SYSTEM_SYNC_MODE_MASK           = (0x1F << 1);    ///< bit[5:1] = System Sync Mode
    const uint_fast8_t  SYSTEM_SYNC_MODE_SHIFT          =  1;
    const uint32_t      EXTERN_SYNC_SELECT_MASK         = (1 << 6);       ///< bit[6] = Extern sync select
    const uint32_t      FRAME_SELECT_ENABLE_MASK        = (1 << 7);       ///< bit[7] = Frame select enable

    /* relative unit base offset to SYNC1 control registers */
    const uint32_t      CONTROL_SYNC1_1_OFFSET          =  0x0020;        ///< relative unit base offset to SYNC1_1 control register
    const uint32_t      CONTROL_SYNC1_2_OFFSET          =  0x0024;        ///< relative unit base offset to SYNC1_2 control register
    const uint32_t      CONTROL_SYNC1_3_OFFSET          =  0x0028;        ///< relative unit base offset to SYNC1_3 control register

    /* relative unit base offset to SYNC2 control registers */
    const uint32_t      CONTROL_SYNC2_1_OFFSET          =  0x0030;        ///< relative unit base offset to SYNC2_1 control register
    const uint32_t      CONTROL_SYNC2_2_OFFSET          =  0x0034;        ///< relative unit base offset to SYNC2_2 control register
    const uint32_t      CONTROL_SYNC2_3_OFFSET          =  0x0038;        ///< relative unit base offset to SYNC2_3 control register

    /* relative unit base offset to SYNC3 control registers */
    const uint32_t      CONTROL_SYNC3_1_OFFSET          =  0x0040;        ///< relative unit base offset to SYNC3_1 control register
    const uint32_t      CONTROL_SYNC3_2_OFFSET          =  0x0044;        ///< relative unit base offset to SYNC3_2 control register
    const uint32_t      CONTROL_SYNC3_3_OFFSET          =  0x0048;        ///< relative unit base offset to SYNC3_3 control register

    /* relative unit base offset to SYNC4 control registers */
    const uint32_t      CONTROL_SYNC4_1_OFFSET          =  0x0050;        ///< relative unit base offset to SYNC4_1 control register
    const uint32_t      CONTROL_SYNC4_2_OFFSET          =  0x0054;        ///< relative unit base offset to SYNC4_2 control register
    const uint32_t      CONTROL_SYNC4_3_OFFSET          =  0x0058;        ///< relative unit base offset to SYNC4_3 control register

    /* constants for CONTROL_SYNC1-4_1 register */
    const uint32_t      SYNC_ENABLE_MASK                = (1 << 0);       ///< bit[0] = Sync1-4 enable
    const uint32_t      SYNC_ACTIVE_SAMPLES_MASK        = (0xFFF << 4);   ///< bit[15:4] = Sync1-4 active samples
    const uint_fast8_t  SYNC_ACTIVE_SAMPLES_SHIFT       =  4;
    const uint32_t      SYNC_ACTIVE_SAMPLE_OFFSET_MASK  = (0xFFFF << 16); ///< bit[31:16] = Sync1-4 active sample offset
    const uint_fast8_t  SYNC_ACTIVE_SAMPLE_OFFSET_SHIFT =  16;

    /* constants for CONTROL_SYNC1-4_2 register */
    const uint32_t      EXTERN_SYNC_OFFSET_MASK         = (0xFFFF << 16); ///< bit[31:16] = Sync1-4 extern sync offset
    const uint_fast8_t  EXTERN_SYNC_OFFSET_SHIFT        =  16;

    /* constants for CONTROL_SYNC1-4_3 register */
    const uint32_t      SYNC_NUMBER_SAMPLES_MASK        = (0xFFFF << 0);  ///< bit[15:0] = Sync1-4 number samples
    const uint_fast8_t  SYNC_NUMBER_SAMPLES_SHIFT       =  0;


    /* relative unit base offset to SYNC5 control registers */
    const uint32_t      CONTROL_SYNC5_1_OFFSET          =  0x0060;        ///< relative unit base offset to SYNC5_1 control register
    const uint32_t      CONTROL_SYNC5_2_OFFSET          =  0x0064;        ///< relative unit base offset to SYNC5_2 control register

    /* relative unit base offset to SYNC6 control registers */
    const uint32_t      CONTROL_SYNC6_1_OFFSET          =  0x0070;        ///< relative unit base offset to SYNC6_1 control register
    const uint32_t      CONTROL_SYNC6_2_OFFSET          =  0x0074;        ///< relative unit base offset to SYNC6_2 control register


    /* constants for CONTROL_SYNC5-6_1 register */
    const uint32_t      HORISONTAL_OFFSET_MASK          = (0xFFFF << 0);  ///< bit[15:0] = Horisontal Offset
    const uint_fast8_t  HORISONTAL_OFFSET_SHIFT         =  0;
    const uint32_t      VERTICAL_OFFSET_MASK            = (0xFFFF << 16); ///< bit[31:16] = Vertical Offset
    const uint_fast8_t  VERTICAL_OFFSET_SHIFT           =  16;

    /* constants for CONTROL_SYNC5-6_2 register */
    const uint32_t      NO_SAMPLES_MASK                 = (0xFFFF << 0);  ///< bit[15:0] = No samples
    const uint_fast8_t  NO_SAMPLES_SHIFT                =  0;
}

#endif
