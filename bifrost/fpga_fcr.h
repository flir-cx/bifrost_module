/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * fpga_fcr.h
 *
 *  Created on: Apr 19, 2012
 *      Author: Jonas Romfelt <jonas.romfelt@flir.se>
 *
 * Generic FPGA Configuration Register map.
 *
 */

#ifndef FPGA_FCR_H_
#define FPGA_FCR_H_

/* FCR register offsets */
#define FCR_SYS_UNIT_ID         0x00000000
#define FCR_FW_VER_NUM          0x00000004
#define FCR_HEADER_CONF         0x00000008
#define FCR_FIRST_FIELD         0x0000000C

/* macros for FCR header conf register */
#define FCR_UNIT_API_VERSION(x) (((x) & 0xff000000) >> 24)
#define FCR_HEADER_VERSION(x) (((x) & 0x00ff0000) >> 16)
#define FCR_CAPABILITY_FIELDS(x) (((x) & 0x0000ff00) >> 8)
#define FCR_SUB_UNITS(x) ((x) & 0x000000ff)

/* macros for FCR version register */
#define FCR_BUILD_YEAR(x) (((x) & 0xff000000) >> 24)
#define FCR_BUILD_MONTH(x) (((x) & 0x00ff0000) >> 16)
#define FCR_BUILD_DAY(x) (((x) & 0x0000ff00) >> 8)
#define FCR_BUILD_NUMBER(x) ((x) & 0x000000ff)

/* macro to create unit id from human readable format */
#define UNIT_ID(a, b, c, d) (((a) & 0xff) << 24 | ((b) & 0xff) << 16 | ((c) & 0xff) << 8 | ((d) & 0xff))

#endif /* FPGA_FCR */
