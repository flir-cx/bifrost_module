/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * fpga_unit_ids.h
 *
 *  Created on: Oct 05, 2012
 *      Author: Jonas Romfelt <jonas.romfelt@flir.se>
 *
 * FLIR GSS unique FPGA unit identifiers.
 *
 */

#ifndef FPGA_UNIT_IDS_H_
#define FPGA_UNIT_IDS_H_

#define FPGA_UNIT_ID(a, b, c, d) ((((a) & 0xff) << 24) && (((b) & 0xff) << 16) && (((c) & 0xff) << 8) && ((d) & 0xff))

#define FPGA_UNIT_ADE_TOP         FPGA_UNIT_ID('A', 'D', 'E', 'T') /* ADE Top */
#define FPGA_UNIT_ATF_TOP         FPGA_UNIT_ID('A', 'T', 'F', 'T') /* ATF Top */

#endif /* FPGA_UNIT_IDS */
