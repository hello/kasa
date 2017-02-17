/**
 * bld/cmd_fs.c
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

#include <bldfunc.h>
#include "fs/fs.h"

#define AMBARELLA_INTERNAL_CMD 0

static int cmd_ls(int argc , char *argv[])
{
	do_ls(argc, argv, FS_TYPE_FAT);

	return 0;
}

static int cmd_cd(int argc , char *argv[])
{

	if(argc < 2){
		uart_putstr("Type 'help cd' for help\r\n");
		return -3;
	}

	do_chdir(argc, argv, FS_TYPE_FAT);

	return 0;
}

static int cmd_fsread(int argc , char *argv[])
{

	u32 addr;
	u32 exec = 0;

	if(argc < 3){
		uart_putstr("Type 'help fsread' for help\r\n");
		return -3;
	}


	if (strtou32(argv[2], &addr) == -1) {
		uart_putstr("invalid address!\r\n");
		return -1;
	}


	if(argc == 4){
		if (strtou32(argv[3], &exec) == -1) {
			uart_putstr("invalid exec!\r\n");
			return -1;
		}
	}

	if(exec != 1)
		exec = 0;

	do_fsread(argv[1], addr, FS_TYPE_FAT, (int)exec);

	return 0;
}

static int cmd_fswrite(int argc , char *argv[])
{

	u32 addr;
	u32 size;

	if(argc < 4){
		uart_putstr("Type 'help fsread' for help\r\n");
		return -3;
	}


	if (strtou32(argv[2], &addr) == -1) {
		uart_putstr("invalid address!\r\n");
		return -1;
	}


	if (strtou32(argv[3], &size) == -1) {
		uart_putstr("invalid size!\r\n");
		return -1;
	}


	do_fswrite(argv[1], addr, FS_TYPE_FAT, size);

	return 0;
}
static int cmd_fsinfo(int argc , char *argv[])
{
	do_info(FS_TYPE_FAT);
	return 0;
}

#if AMBARELLA_INTERNAL_CMD

static char help_ls[] =
" \t ls\r\n";
__CMDLIST(cmd_ls, "ls", help_ls);

static char help_cd[] =
" \t cd [dir]\r\n";
__CMDLIST(cmd_cd, "cd", help_cd);

static char help_fsinfo[] =
" \t fsinfo \r\n";
__CMDLIST(cmd_fsinfo, "fsinfo", help_fsinfo);

static char help_read[] =
" \t fsread [file] [addr] [exec]\r\n";
__CMDLIST(cmd_fsread, "fsread", help_read);

static char help_write[] =
" \t fswrite [file] [addr] [size]\r\n";
__CMDLIST(cmd_fswrite, "fswrite", help_write);

#else

static int cmd_fs(int argc, char *argv[])
{
	int ret_val = -1;

	if(argc < 2)
		return ret_val;

	if (strcmp(argv[1], "ls") == 0){
		ret_val = cmd_ls(argc - 1, &argv[1]);
	}else if(strcmp(argv[1], "info") == 0){
		ret_val = cmd_fsinfo(argc - 1, &argv[1]);
	}else if(strcmp(argv[1], "cd") == 0){
		ret_val = cmd_cd(argc - 1, &argv[1]);
	}else if(strcmp(argv[1], "read") == 0){
		ret_val = cmd_fsread(argc - 1, &argv[1]);
	}else if(strcmp(argv[1], "write") == 0){
		ret_val = cmd_fswrite(argc - 1, &argv[1]);
	}

	return ret_val;
}

static char help_fs[] =
"\t fs ls\r\n"
"\t fs info\r\n"
"\t fs cd [dir]\r\n"
"\t fs read [file] [addr] [exec]\r\n"
"\t fs write [file] [addr] [size]\r\n";
__CMDLIST(cmd_fs, "fs", help_fs);

#endif


