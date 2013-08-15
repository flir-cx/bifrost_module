/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_detectorSCD_Hercules.h
 *
 *  Created on: Mar 7, 2012
 *      Author: Jonas Romfelt <jonas.romfelt@flir.se>
 *
 * Register map FPGA Valhalla DMA controller block.
 *
 */
#ifndef VALHALLA_DMA_H_
#define VALHALLA_DMA_H_

#define VALHALLA_ADDR_DETECTOR_SCD_WRITE_FIFO   0x00003000 // start of write FIFO
#define VALHALLA_ADDR_DETECTOR_SCD_READ_FIFO    0x00004000 // start of read FIFO

#define VALHALLA_ADDR_DETECTOR_SCD_CONTROL      0x00000d10
#define VALHALLA_ADDR_DETECTOR_SCD_STATUS       0x00000d80

#define VALHALLA_DETECTOR_SCD_CONTROL_WRITE     (1 << 0) // toggle to start sending FIFO
#define VALHALLA_DETECTOR_SCD_CONTROL_READ_ACK  (1 << 1) // toggle when FIFO has been read
#define VALHALLA_DETECTOR_SCD_CONTROL_CLEAR     (1 << 2) // reset state-machine(s)

#define VALHALLA_DETECTOR_SCD_STATUS_DONE       (1 << 0) // received reply
#define VALHALLA_DETECTOR_SCD_STATUS_TIMEOUT    (1 << 1)
#define VALHALLA_DETECTOR_SCD_STATUS_BUSY       (1 << 2) // transmitting command

#endif
