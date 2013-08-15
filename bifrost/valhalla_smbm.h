/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_smbm.h
 *
 *  Created on: Jan 25, 2013
 *      Author: Jonas Romfelt
 *
 * Register map FPGA Valhalla SMBus Master controller block. Sending one
 * byte at a time.
 *
 */

#ifndef VALHALLA_SMBM_H_
#define VALHALLA_SMBM_H_

#define VALHALLA_ADDR_SMBM_BASE                  0x00007600

#define VALHALLA_ADDR_SMBM_ADDRESS               0x00000010 // register address to access
#define VALHALLA_ADDR_SMBM_WR_DATA               0x00000014 // byte to write to device, started when written
#define VALHALLA_ADDR_SMBM_RD_ACC                0x0000001c // start read access
#define VALHALLA_ADDR_SMBM_DEV_ADDR              0x00000020 // device address
#define VALHALLA_ADDR_SMBM_RST                   0x00000024
#define VALHALLA_ADDR_SMBM_STATUS                0x00000084

#define VALHALLA_SMBM_STATUS_BUSY                0x1

#endif
