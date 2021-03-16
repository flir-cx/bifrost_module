/*
 * Copyright (c) FLIR Systems AB.
 */
#include <linux/version.h>
#include "bifrost_platform.h"

/* This version 5.4 is not exact - using dts information like this would
 * work also for earlier kernels */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
#include <linux/of.h>
bool cpu_is_imx6q(void)
{
	return of_machine_is_compatible("fsl,imx6q");
}
bool cpu_is_imx6dl(void)
{
	return of_machine_is_compatible("fsl,imx6dl");
}

#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
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
