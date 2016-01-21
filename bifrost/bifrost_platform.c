/*
 * Copyright (c) FLIR Systems AB.
 */
#include <linux/version.h>
#include "bifrost_platform.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
#include "../arch/arm/mach-imx/hardware.h"
#else
bool cpu_is_imx6q(void)
{
	return false;
}
bool cpu_is_imx6dl(void)
{
	return false;
}
#endif

bool platform_fvd(void)
{
	return platform_rocky() || platform_evander() ;
}

bool platform_rocky(void)
{
	return cpu_is_imx6q();
}

bool platform_evander(void)
{
	return cpu_is_imx6dl() ;
}
