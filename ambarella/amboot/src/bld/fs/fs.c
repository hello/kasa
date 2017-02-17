/**
 * fs/fs.c
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
#include "fs.h"
#include "fat.h"


struct fstype_info {
	int fstype;
	char *name;
	int (*ls)(const char *dirname);
	int (*cd)(const char *dirname);
	int (*read)(const char *filename, u32 addr, int exec);
	int (*write)(const char *filename, u32 addr, u32 size);
	int (*info)(void);
};

static struct fstype_info fstypes[] = {
	{
		.fstype = FS_TYPE_FAT,
		.name = "fat",
		.ls = file_fat_ls,
		.cd = file_fat_chdir,
		.info = file_fat_info,
		.read = file_fat_read,
		.write = file_fat_write,
	}
};

static struct fstype_info *fs_get_info(int fstype)
{
	struct fstype_info *info = NULL;
	int i;

	for (i = 0, info = fstypes; i < ARRAY_SIZE(fstypes) - 1; i++, info++) {
		if (fstype == info->fstype)
			return info;
	}

	/* Return the 'unsupported' sentinel */
	return info;
}
int do_ls(int argc, char *argv[], int fstype)
{
	struct fstype_info *fsinfo;

	fsinfo = fs_get_info(fstype);
	if(fsinfo == NULL){
		putstr("fstype is not supported\r\n");
		return -1;
	}

	fsinfo->ls(argv[1]);

	return 0;
}
int do_info(int fstype)
{
	struct fstype_info *fsinfo;

	fsinfo = fs_get_info(fstype);
	if(fsinfo == NULL){
		putstr("fstype is not supported\r\n");
		return -1;
	}

	fsinfo->info();

	return 0;

}

int do_chdir(int argc, char *argv[], int fstype)
{
	struct fstype_info *fsinfo;

	fsinfo = fs_get_info(fstype);
	if(fsinfo == NULL){
		putstr("fstype is not supported\r\n");
		return -1;
	}

	fsinfo->cd(argv[1]);

	return 0;

}
int do_fsread(const char *filename, u32 addr, int fstype, int exec)
{
	struct fstype_info *fsinfo;

	fsinfo = fs_get_info(fstype);
	if(fsinfo == NULL){
		putstr("fstype is not supported\r\n");
		return -1;
	}

	fsinfo->read(filename, addr, exec);

	return 0;
}

int do_fswrite(const char *filename, u32 addr, int fstype, u32 size)
{
	struct fstype_info *fsinfo;

	fsinfo = fs_get_info(fstype);
	if(fsinfo == NULL){
		putstr("fstype is not supported\r\n");
		return -1;
	}

	fsinfo->write(filename, addr, size);

	return 0;
}
