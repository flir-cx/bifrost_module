/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_ocg.h
 *
 *  Created on: Jan 21, 2014
 *      Author: Ari Mannila <ari.mannila@flir.se>
 *
 * Register map for FPGA Valhalla OCG block.
 *
 */

#ifndef VALHALLA_OCG_H_
#define VALHALLA_OCG_H_

namespace VALHALLA_OCG
{
    const uint32_t      BASE_ADDRESS              =  0x00003000;

    /* CONTROL1 register */
    const uint32_t      CONTROL1_OFFSET             = 0x0020;      ///< offset relative to unit base address
    const uint32_t      VIDEO_ROWS_MASK             = (0xFFFF << 16);
    const uint_fast8_t  VIDEO_ROWS_SHIFT            = 16;
    const uint32_t      VIDEO_COLUMNS_MASK          = (0xFFFF << 0);
    const uint_fast8_t  VIDEO_COLUMNS_SHIFT         = 0;

    /* CONTROL2 register */
    const uint32_t      CONTROL2_OFFSET             = 0x0024;      ///< offset relative to unit base address
    const uint32_t      NUM_DISPLAYED_CHARS_MASK    = (0xFF << 16);
    const uint_fast8_t  NUM_DISPLAYED_CHARS_SHIFT   = 16;
    const uint32_t      CHAR_HEIGHT_MASK            = (0xFF << 8);
    const uint_fast8_t  CHAR_HEIGHT_SHIFT           = 8;
    const uint32_t      CHAR_WIDTH_MASK             = (0xFF << 0);
    const uint_fast8_t  CHAR_WIDTH_SHIFT            = 0;

    /* CONTROL3 register */
    const uint32_t      CONTROL3_OFFSET             = 0x0028;      ///< offset relative to unit base address
    const uint32_t      CHAR_ROW_POSITION_MASK      = (0xFFFF << 16);
    const uint_fast8_t  CHAR_ROW_POSITION_SHIFT     = 16;
    const uint32_t      CHAR_COLUMN_POSITION_MASK   = (0xFFFF << 0);
    const uint_fast8_t  CHAR_COLUMN_POSITION_SHIFT  = 0;

    /* CONTROL4 register */
    const uint32_t      CONTROL4_OFFSET             = 0x002C;      ///< offset relative to unit base address
    const uint32_t      FG_COLOR_A_MASK             = (0xFF << 24);
    const uint_fast8_t  FG_COLOR_A_SHIFT            = 24;
    const uint32_t      FG_COLOR_R_MASK             = (0xFF << 16);
    const uint_fast8_t  FG_COLOR_R_SHIFT            = 16;
    const uint32_t      FG_COLOR_G_MASK             = (0xFF << 8);
    const uint_fast8_t  FG_COLOR_G_SHIFT            = 8;
    const uint32_t      FG_COLOR_B_MASK             = (0xFF << 0);
    const uint_fast8_t  FG_COLOR_B_SHIFT            = 0;

    /* CONTROL5 register */
    const uint32_t      CONTROL5_OFFSET             = 0x0030;      ///< offset relative to unit base address
    const uint32_t      BG_COLOR_A_MASK             = (0xFF << 24);
    const uint_fast8_t  BG_COLOR_A_SHIFT            = 24;
    const uint32_t      BG_COLOR_R_MASK             = (0xFF << 16);
    const uint_fast8_t  BG_COLOR_R_SHIFT            = 16;
    const uint32_t      BG_COLOR_G_MASK             = (0xFF << 8);
    const uint_fast8_t  BG_COLOR_G_SHIFT            = 8;
    const uint32_t      BG_COLOR_B_MASK             = (0xFF << 0);
    const uint_fast8_t  BG_COLOR_B_SHIFT            = 0;

    /* Character map, defines the character to display */
    const uint32_t      CHAR_MAP_BASE               = 0x0100;
    const uint32_t      EXTERNAL_TIME_REF_MASK      = (0xF << 12);
    const uint_fast8_t  EXTERNAL_TIME_REF_SHIFT     = 12;
    const uint32_t      CHAR_ADDRESS_MASK           = (0xFFF << 0);
    const uint_fast8_t  CHAR_ADDRESS_SHIFT          = 0;

    /* Pixel map, defines the character appearance */
    const uint32_t      PIXEL_MAP_BASE              = 0x0500;
}

#endif
