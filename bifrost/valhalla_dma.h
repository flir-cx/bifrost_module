/*
 * Copyright (c) FLIR Systems AB. All rights reserved.
 *
 * valhalla_dma.h
 *
 *  Created on: Mar 7, 2012
 *      Author: Jonas Romfelt <jonas.romfelt@flir.se>
 *
 * Register map FPGA Valhalla DMA controller block.
 *
 */
#ifndef VALHALLA_DMA_H_
#define VALHALLA_DMA_H_

#define VALHALLA_ADDR_DMA_REG_BASE 0x00000100

#define VALHALLA_ADDR_DMA_UNIT_ID     (VALHALLA_ADDR_DMA_REG_BASE + 0x00)
#define VALHALLA_ADDR_DMA_FW_VER_NUM  (VALHALLA_ADDR_DMA_REG_BASE + 0x04)
#define VALHALLA_ADDR_DMA_HEADER_CONF (VALHALLA_ADDR_DMA_REG_BASE + 0x08)
#define VALHALLA_ADDR_DMA_CAPABILITY  (VALHALLA_ADDR_DMA_REG_BASE + 0x0C)

#define VALHALLA_ADDR_DMA_STATUS (VALHALLA_ADDR_DMA_REG_BASE + 0x10)
#define VALHALLA_ADDR_DMA_ABORT  (VALHALLA_ADDR_DMA_REG_BASE + 0x14)
#define VALHALLA_ADDR_DMA_CHAN   (VALHALLA_ADDR_DMA_REG_BASE + 0x2C)

/*
 * Each DMA channel has its own set of the following registers. The
 * VALHALLA_ADDR_DMA_CHAN register selects which register set that
 * is active (i.e. index into an array) and must be set prior to
 * accessing these registers.
 *
 * Note that the DMA length must be a multiple of 32 bytes.
 */
#define VALHALLA_ADDR_DMA_SRC_ADDR    (VALHALLA_ADDR_DMA_REG_BASE + 0x18)
#define VALHALLA_ADDR_DMA_DEST_ADDR   (VALHALLA_ADDR_DMA_REG_BASE + 0x1C)
#define VALHALLA_ADDR_DMA_LEN_BYTES   (VALHALLA_ADDR_DMA_REG_BASE + 0x20)
#define VALHALLA_ADDR_DMA_DIR_UP_STRM (VALHALLA_ADDR_DMA_REG_BASE + 0x24)
#define VALHALLA_ADDR_DMA_START       (VALHALLA_ADDR_DMA_REG_BASE + 0x28)

/*
 * Possible VALHALLA_ADDR_DMA_DIR_UP_STRM register values
 *
 * Down-stream PCIe, i.e. system RAM to FPGA RAM
 * Up-stream PCIe, i.e. FPGA RAM to system RAM
 */
#define VALHALLA_ADDR_DMA_DIR_UP_STRM_DOWN 0x0
#define VALHALLA_ADDR_DMA_DIR_UP_STRM_UP   0x1

#endif
