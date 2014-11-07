
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
#include "../arch/arm/mach-imx/hardware.h"
#else
bool cpu_is_imx6q(){return false;}
#endif

bool platform_rocky(void)
{

    return cpu_is_imx6q();

}
