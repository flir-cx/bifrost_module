/* Copyright (c) 2013 FLIR Systems AB. All rights reserved. */
/*
 * animal-i2c - FLIR Heimdall Driver, for GNU/Linux.
 *
 *  Created on: Apr 20, 2010
 *      Author: Jonas Romfelt <jonas.romfelt@flir.se>
 *
 * Public driver interface for animal-i2c adapter driver.
 *
 */

#ifndef ANIMAL_I2C_API_H_
#define ANIMAL_I2C_API_H_

#include <linux/ioctl.h>
#include <linux/types.h>

/*
 * According to "linux/Documentation/devices.txt", the major number range 240-254
 * is allocated for local/experimental use
 */

#define ANIMAL_DEVICE_MAJOR 241
#define ANIMAL_DEVICE_MINOR 0

struct animal_i2c_info {
        struct {
                __u8 major;
                __u8 minor;
                __u8 revision;
        } version;
        __u8 simulator; /* set to 0 if simulator interface disabled */
};

#define ANIMAL_I2C_BUFFER_MAX_SIZE 2048
#define ANIMAL_SMB_BUFFER_MAX_SIZE 32

struct animal_i2c_message {
        __u8 command; /* command, a.k.a. offset */
        __u32 size; /* size of data, ignored for READ{8,16} and WRITE{8,16} */
        union {
                __u8 byte;
                __u16 word;
                __u8 *block; /* block data, must hold at least `size' bytes */
        } data;
};

/*
 * Before using SMB/I2C read/write commands, the address (and if required
 * GPIO chip select) must have been set!
 */

#define ANIMAL_IOCTL_INFO             _IOR(ANIMAL_DEVICE_MAJOR,   0,  struct animal_i2c_info *)

#define ANIMAL_IOCTL_SET_ADDRESS      _IOW(ANIMAL_DEVICE_MAJOR,   1,  __u16 *) /* Addresses are written as 7 bit without the R/W bit */
#define ANIMAL_IOCTL_SET_GPIO         _IOW(ANIMAL_DEVICE_MAJOR,   2,  int *)
#define ANIMAL_IOCTL_SET_ADAPTER      _IOW(ANIMAL_DEVICE_MAJOR,   3,  int *)

#define ANIMAL_IOCTL_SMB_READ8        _IOWR(ANIMAL_DEVICE_MAJOR, 10,  struct animal_i2c_message *)
#define ANIMAL_IOCTL_SMB_WRITE8       _IOW(ANIMAL_DEVICE_MAJOR,  11,  struct animal_i2c_message *)

#define ANIMAL_IOCTL_SMB_READ16       _IOWR(ANIMAL_DEVICE_MAJOR, 12,  struct animal_i2c_message *)
#define ANIMAL_IOCTL_SMB_WRITE16      _IOW(ANIMAL_DEVICE_MAJOR,  13,  struct animal_i2c_message *)

#define ANIMAL_IOCTL_SMB_WRITE_BLOCK  _IOW(ANIMAL_DEVICE_MAJOR,  14,  struct animal_i2c_message *)
#define ANIMAL_IOCTL_SMB_READ_BLOCK   _IOWR(ANIMAL_DEVICE_MAJOR, 15,  struct animal_i2c_message *)

#define ANIMAL_IOCTL_I2C_WRITE        _IOW(ANIMAL_DEVICE_MAJOR,  16,  struct animal_i2c_message *)
#define ANIMAL_IOCTL_I2C_WRITEREAD    _IOWR(ANIMAL_DEVICE_MAJOR, 17,  struct animal_i2c_message *)

#endif /* ANIMAL_I2C_API_H_ */
