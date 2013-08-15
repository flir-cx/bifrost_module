/* Copyright (c) 2013 FLIR Systems AB. All rights reserved. */
#ifndef __LINUX_I2C_USIM__H
#define __LINUX_I2C_USIM__H

#include <linux/ioctl.h>
#include <linux/types.h>

#define I2C_USIM_IOC_MAGIC 'U'

#define I2C_USIM_IOC_GET_EVENT \
        _IOWR(I2C_USIM_IOC_MAGIC, 1, struct i2c_usim_ioc_event)

#define I2C_USIM_IOC_PUT_EVENT \
        _IOWR(I2C_USIM_IOC_MAGIC, 2, struct i2c_usim_ioc_event)

#define READ_ACCESS 0x1
#define WRITE_ACCESS 0x2

#define I2C 1
#define SMB 2

#define EVENT_TYPE(protocol, access) (((protocol)<<16) | ((access)&0xffff))
#define ACCESS_TYPE(event) ((event) & 0xffff)
#define BUS_TYPE(event) (((event) >> 16) & 0xffff)

#define I2C_USIM_I2C_READ EVENT_TYPE(I2C, READ_ACCESS)
#define I2C_USIM_I2C_WRITE EVENT_TYPE(I2C, WRITE_ACCESS)
#define I2C_USIM_SMB_READ EVENT_TYPE(SMB, READ_ACCESS)
#define I2C_USIM_SMB_WRITE EVENT_TYPE(SMB, WRITE_ACCESS)

struct i2c_usim_ioc_event {
        /* Don't touch! */
        __u32 sender;
        __u32 ticket;

        /* i2c data */
        __u32 type;
        __u32 bus;
        __u32 addr;
        __u32 len;
        __u8 data[512];
};

#ifdef __KERNEL__

#endif

#endif
