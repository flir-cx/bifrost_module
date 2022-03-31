/*
 * Copyright (c) FLIR Systems AB.
 */
#include <linux/version.h>
#include "bifrost_platform.h"

/* This version 5.4 is not exact - using dts information like this would
 * work also for earlier kernels */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0)
#include <linux/of.h>
bool platform_rocky(void)
{
	return of_machine_is_compatible("fsl,imx6q");
}

bool platform_evander(void)
{
	return of_machine_is_compatible("fsl,imx6dl");
}

bool platform_eoco(void)
{
	return of_machine_is_compatible("fsl,imx6qp-eoco");
}

bool platform_fvd(void)
{
	return platform_rocky() || platform_evander() || platform_eoco();
}

#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
#include "../arch/arm/mach-imx/hardware.h"

bool platform_rocky(void)
{
	return cpu_is_imx6q();
}

bool platform_evander(void)
{
	return cpu_is_imx6dl() ;
}

bool platform_fvd(void)
{
	return platform_rocky() || platform_evander();
}

#else
bool platform_rocky(void)
{
	return false;
}

bool platform_evander(void)
{
	return false;
}

bool platform_fvd(void)
{
	return platform_rocky() || platform_evander();
}


#endif

