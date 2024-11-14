/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) FLIR Systems AB.
 */
#ifndef BIFROST_PLATFORM_H_
#define BIFROST_PLATFORM_H_

#include <linux/types.h>

extern bool platform_fvd(void);
extern bool platform_evander(void);
extern bool platform_rocky(void);
extern bool platform_eoco(void);
extern bool platform_ec702(void);

#endif
