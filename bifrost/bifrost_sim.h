/* Copyright (c) 2013 FLIR Systems AB. All rights reserved. */
#ifndef __LINUX_BIFROST_SIM__H
#define __LINUX_BIFROST_SIM__H

#define SIM_MSI_IRQ 1 /* Must not be NO_IRQ! */

struct bifrost_regb {
        u32 value;
        u32 mode;
};

struct bifrost_simulator {
        struct {
                /* Handle that holds memory pointer to by address */
                struct bifrost_user_handle *hnd;

                /* Size of simulator memory */
                u32 size;

                /* User space address to simulator memory */
                unsigned long address;
        } ram;
};

struct bifrost_device;

extern int bifrost_sim_pci_init(struct bifrost_device *dev);
extern void bifrost_sim_pci_exit(struct bifrost_device *dev);
#endif
