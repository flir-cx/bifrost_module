/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_rtc.h
 *
 *  Created on: Mar 18, 2014
 *      Author: Ari Mannila <ari.mannila@flir.se>
 *
 * Register map for FPGA Valhalla RTC block.
 *
 */

#ifndef VALHALLA_RTC_H_
#define VALHALLA_RTC_H_

namespace VALHALLA_RTC
{
    /* CONTROL0 register */
    const uint32_t      RTC_CONTROL0_OFFSET         = 0x0010;
    const uint32_t      RTC_SET_TIME_MASK           = (1 << 2); // bit[2] = set RTC time
    const uint32_t      RTC_MILLISEC_COUNT_MASK     = (1 << 4); // bit[4] = 0:Frame counter 1:Millisec counter

    /* CONTROL1 register */
    const uint32_t      RTC_CONTROL1_OFFSET         = 0x0014;   // set years since 2000
    /* CONTROL2 register */
    const uint32_t      RTC_CONTROL2_OFFSET         = 0x0018;   // set days since Jan 1st
    /* CONTROL3 register */
    const uint32_t      RTC_CONTROL3_OFFSET         = 0x001c;   // set hours since midnight
    /* CONTROL4 register */
    const uint32_t      RTC_CONTROL4_OFFSET         = 0x0020;   // set minutes since last hour
    /* CONTROL5 register */
    const uint32_t      RTC_CONTROL5_OFFSET         = 0x0024;   // set seconds since last minute
    /* CONTROL6 register */
    const uint32_t      RTC_CONTROL6_OFFSET         = 0x0028;   // set frames since last second
    /* CONTROL10 register */
    const uint32_t      RTC_CONTROL10_OFFSET        = 0x0034;   // set millisecs since last second
}

#endif
