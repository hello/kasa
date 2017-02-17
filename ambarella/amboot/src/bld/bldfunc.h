/**
 * bld/bldfunc.h
 *
 * Boot loader function prototypes.
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
 *    2008/02/19 - [Allen Wang] changed to use capabilities and chip ID
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


#ifndef __BLDFUNC_H__
#define __BLDFUNC_H__

#include <amboot.h>
#include <fio/firmfl.h>

/*===========================================================================*/
#if defined(AMBOOT_DEFAULT_LINUX_MACHINE_ID)
#define AMBARELLA_LINUX_MACHINE_ID		(AMBOOT_DEFAULT_LINUX_MACHINE_ID)
#else
#define AMBARELLA_LINUX_MACHINE_ID		(1223)
#endif

/*===========================================================================*/
#ifndef __ASM__
/*===========================================================================*/
int thaw_hibernation(void);
int thaw_info(void);

/*===========================================================================*/
extern int secure_boot_main(int ret_val, flpart_table_t *pptb);

/*===========================================================================*/
int malloc_init(void);
void *malloc(int size);
void free(void *ptr);

/*===========================================================================*/
extern int rand(void);

/*===========================================================================*/
extern int bld_loader_load_partition(int part_id,
	const flpart_table_t *pptb, int verbose);
extern void bld_loader_display_part(const char *s, const flpart_t *part);
extern void bld_loader_display_ptb_content(const flpart_table_t *ptb);

/*===========================================================================*/
extern void *memcpy(void *dst, const void *src, unsigned int n);
extern void *memmove(void *dst,const void *src, unsigned int n);
extern void memzero(void *dst, unsigned int n);
extern int  memcmp(const void *dst, const void *src, unsigned int n);
extern void *memset(void *s, int c, unsigned int n);
extern unsigned int crc32(const void *buf, unsigned int size);

/*===========================================================================*/
extern void fio_exit_random_mode(void);
extern void enable_fio_dma(void);

/*===========================================================================*/
extern u32 decompress(const char *name, u32 input_start, u32 input_end,
	u32 output_start, u32 free_mem_ptr_p, u32 free_mem_ptr_end_p);

/*===========================================================================*/
extern int strlen(const char *s);
extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, unsigned int maxlen);
extern char *strncpy(char *dest, const char *src, unsigned int n);
extern char *strlcpy(char *dest, const char *src, unsigned int n);
extern char *strchr(const char * s, int c);
extern void *memchr(const void *s, int c, unsigned int n);
extern int strtou32(const char *str, u32 *value);
extern char *strfromargv(char *s, unsigned int slen, int argc, char **argv);
extern int str_to_ipaddr(const char *, u32 *);
extern int str_to_hwaddr(const char *, u8 *);
extern char *strcat(register char *src, register const char *append);
extern char *strcpy(char *dst, const char *src);
extern char *strstr(const char *s1, const char *s2, int n);

/*===========================================================================*/
#define COMMAND_MAGIC	0x436d6420

typedef int(*cmdfunc_t)(int, char **);

typedef struct cmdlist_s
{
	u32	magic;
	char	*name;
	char	*help;
	cmdfunc_t	fn;
	struct cmdlist_s	*next;
} cmdlist_t;

#define __COMMAND __attribute__((unused, __section__(".cmdlist")))

#define __CMDLIST(f, nm, hlp)				\
	cmdlist_t __COMMAND##f __COMMAND = {		\
		magic:	COMMAND_MAGIC,			\
		name:	nm,				\
		help:	hlp,				\
		fn:	f				\
	}

extern void commands_init(void);
extern int  parse_command(char *cmdline);

#define MAX_CMDLINE_LEN	1024	/* atag cmdling */
#define MAX_CMD_ARGS	32
#define MAX_ARG_LEN	64

/*===========================================================================*/
/* Booting and loading functions                                             */
/*===========================================================================*/
typedef int (*boot_fn_t)(const char *, int);
extern int bios(const char *, int);
extern int boot(const char *, int);
extern int netprogram(const char *, int);
extern void jump_to_kernel(void *, int);

/*===========================================================================*/
void fdt_print_error(const char *str, int err);
extern int fdt_update_cmdline(void *fdt, const char *cmdline);

extern u32 fdt_update_tags(void *jump_addr, const char *cmdline, u32 cpux_jump,
	u32 initrd2_start, u32 initrd2_size, int verbose);
extern u32 setup_tags(void *jump_addr, const char *cmdline, u32 cpux_jump,
	u32 initrd2_start, u32 initrd2_size, int verbose);

/*===========================================================================*/
extern int amboot_bsp_early_init(void)
	__attribute__ ((weak));
extern int amboot_bsp_hw_init(void)
	__attribute__ ((weak));
extern int amboot_bsp_check_usbmode(void)
	__attribute__ ((weak));
extern int amboot_bsp_entry(flpart_table_t *pptb)
	__attribute__ ((weak));

extern int amboot_bsp_self_refresh_enter(void)
	__attribute__ ((weak, noinline, aligned(32)));
extern int amboot_bsp_self_refresh_exit(void)
	__attribute__ ((weak));
extern int amboot_bsp_self_refresh_check_valid(void)
	__attribute__ ((weak));

/*===========================================================================*/
#endif
/*===========================================================================*/

#endif

