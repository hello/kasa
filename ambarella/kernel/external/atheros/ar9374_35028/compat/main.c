#include <linux/module.h>

MODULE_AUTHOR("Luis R. Rodriguez");
MODULE_DESCRIPTION("Kernel compatibility module");
MODULE_LICENSE("GPL");

static int __init compat_init(void)
{
	/* pm-qos for kernels <= 2.6.24, this is a no-op on newer kernels */
	compat_pm_qos_power_init();

        return 0;
}
module_init(compat_init);

static void __exit compat_exit(void)
{
	compat_pm_qos_power_deinit();

        return;
}
module_exit(compat_exit);

