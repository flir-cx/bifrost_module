/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_valh.h
 *
 *  Created on: Mar 14, 2013
 *      Author: Jonas Romfelt
 *
 * Register map FPGA Valhalla top.
 *
 */

#ifndef VALHALLA_VALH_H_
#define VALHALLA_VALH_H_

#define VALHALLA_ADDR_VALH_BASE                 0x00000000 // default unit offset

// cap[3] holds build timestamp
#define VALHALLA_VALH_TIMESTAMP_d_MASK          0x0000001f // days
#define VALHALLA_VALH_TIMESTAMP_d_SHIFT         27

#define VALHALLA_VALH_TIMESTAMP_M_MASK          0x0000000f // month
#define VALHALLA_VALH_TIMESTAMP_M_SHIFT         23

#define VALHALLA_VALH_TIMESTAMP_y_MASK          0x0000002f // year
#define VALHALLA_VALH_TIMESTAMP_y_SHIFT         17

#define VALHALLA_VALH_TIMESTAMP_h_MASK          0x0000001f // hour
#define VALHALLA_VALH_TIMESTAMP_h_SHIFT         12

#define VALHALLA_VALH_TIMESTAMP_m_MASK          0x0000002f // minute
#define VALHALLA_VALH_TIMESTAMP_m_SHIFT         6

#define VALHALLA_VALH_TIMESTAMP_s_MASK          0x0000002f // second
#define VALHALLA_VALH_TIMESTAMP_s_SHIFT         0

#define BITCUT(value, mask, shift) (((value) >> shift) & mask)

#endif
