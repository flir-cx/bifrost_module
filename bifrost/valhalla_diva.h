/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_diva.h
 *
 *  Created on: Nov 14, 2013
 *      Author: Jonas Lundberg <jonas.lundberg@flir.se>
 *
 * Register map for FPGA Valhalla DIVA block.
 *
 */

#ifndef VALHALLA_DIVA_H_
#define VALHALLA_DIVA_H_

namespace VALHALLA_DIVA
{
    uint32_t      BASE_ADDRESS              =  0x00001E00;

    /* CONTROL1 register */
    uint32_t      CONTROL1_OFFSET           =  0x0010;      ///< offset relative to unit base address
    uint32_t      DIVA_ENABLE_MASK          = (1 << 0);     ///< bit[0] = DIVA enable
    uint32_t      CLEAR_DEBUG_STATUS_MASK   = (1 << 31);    ///< bit[31] = Clear debug status

    /* CONTROL2 register */
    uint32_t      CONTROL2_OFFSET           =  0x0014;      ///< offset relative to unit base address
    // bit[15:0]  = Sample no start for DIVA_VS
    // bit[31:16] = Line no start for DIVA_VS

    /* CONTROL3 register */
    uint32_t      CONTROL3_OFFSET           =  0x0018;      ///< offset relative to unit base address
    // bit[15:0]  = Sample no stop for DIVA_VS
    // bit[31:16] = Line no stop for DIVA_VS

    /* CONTROL4 register */
    uint32_t      CONTROL4_OFFSET           =  0x001C;      ///< offset relative to unit base address
    // bit[15:0]  = Sample no start for DIVA_HS
    // bit[31:16] = Line no start for DIVA_HS

    /* CONTROL5 register */
    uint32_t      CONTROL5_OFFSET           =  0x0020;      ///< offset relative to unit base address
    // bit[15:0]  = Sample no stop for DIVA_HS
    // bit[31:16] = Line no stop for DIVA_HS

    /* CONTROL6 register */
    uint32_t      CONTROL6_OFFSET           =  0x0024;      ///< offset relative to unit base address
    // bit[15:0]  = Sample no start for DIVA_DE
    // bit[31:16] = Line no start for DIVA_DE

    /* CONTROL7 register */
    uint32_t      CONTROL7_OFFSET           =  0x0028;      ///< offset relative to unit base address
    // bit[15:0]  = Sample no stop for DIVA_DE
    // bit[31:16] = Line no stop for DIVA_DE


    uint32_t      HIGH_2_BYTE_MASK          = (0xFF << 16); ///< bit[31:16]
    uint_fast8_t  HIGH_2_BYTE_SHIFT         =  16;
    uint32_t      LOW_2_BYTE_MASK           = (0xFF << 0);  ///< bit[15:0]
    uint_fast8_t  LOW_2_BYTE_SHIFT          =  0;


    /* STATUS1 register */
    uint32_t      STATUS1_OFFSET            =  0x0080;      ///< offset relative to unit base address
    uint32_t      DIVA_ID_MASK              = (0xF << 0);   ///< bit[3:0] = DIVA id
    uint_fast8_t  DIVA_ID_SHIFT             =  0;
    uint32_t      READ_WHEN_FIFO_EMPTY_MASK = (1 << 30);    ///< bit[30] = Read when FIFO empty
    uint32_t      WRITE_WHEN_FIFO_FULL_MASK = (1 << 31);    ///< bit[31] = Write when FIFO full

}

#endif
