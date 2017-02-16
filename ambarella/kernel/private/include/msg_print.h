#ifndef __PRINT_PRIVATE_DRV__
#define __PRINT_PRIVATE_DRV__
#include <linux/compiler.h>
#include <linux/types.h>
extern int print_drv(const char *fmt, ...);
extern int do_drv_syslog(int type, char __user *buf, int len, bool from_file);
#endif
