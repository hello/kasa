/*
 *
 * mw_api.c
 *
 * History:
 *	2010/02/28 - [Jian Tang] Created this file
 *	2011/08/05 - [Jian Tang] Modified this file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <basetypes.h>

#include "iav_common.h"
#include "iav_ioctl.h"
#include "iav_vin_ioctl.h"

#include "mw_struct.h"
#include "mw_defines.h"
#include "mw_api.h"
#include "mw_aaa_params.h"

int fd_iav;
int G_mw_init_already = 0;
int G_mw_log_level = 1;
static struct iav_driver_version mw_drv_info;
extern mw_version_info mw_version;

/*************************************************
 *
 *		Static Functions, for file internal used
 *
 *************************************************/

static int get_driver_info(struct iav_driver_version *mw_drv_info)
{
	if (ioctl(fd_iav, IAV_IOC_GET_DRIVER_INFO, mw_drv_info) < 0) {
		perror("IAV_IOC_GET_DRIVER_INFO");
		return -1;
	}

	return 0;
}

/*************************************************
 *
 *		Public Functions, for external used
 *
 *************************************************/

int mw_init(void)
{
	int state = 0;

	if (G_mw_init_already) {
		return 0;
	}

	// open the iav device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	get_driver_info(&mw_drv_info);

	if (ioctl(fd_iav, IAV_IOC_GET_IAV_STATE, &state) < 0) {
		MW_ERROR("Cannot get IAV state.\n");
		return -1;
	}
	G_mw_init_already = 1;

	return 0;
}

int mw_uninit(void)
{
	close(fd_iav);
	G_mw_init_already = 0;

	return 0;
}

int mw_get_driver_info(mw_driver_info *pDriverInfo)
{
	if (pDriverInfo == NULL) {
		MW_MSG("[mw_get_driver_info] : Pointer is NULL");
		return -1;
	}

	pDriverInfo->arch = mw_drv_info.arch;
	pDriverInfo->model = mw_drv_info.model;
	pDriverInfo->major = mw_drv_info.major;
	pDriverInfo->minor = mw_drv_info.minor;
	strncpy(pDriverInfo->description, mw_drv_info.description, 63);

	MW_DEBUG("= [G] ====== Driver Info ========\n");
	MW_DEBUG("		 arch = %d\n", pDriverInfo->arch);
	MW_DEBUG("		model = %d\n", pDriverInfo->model);
	MW_DEBUG("		major = %d\n", pDriverInfo->major);
	MW_DEBUG("		minor = %d\n", pDriverInfo->minor);
	MW_DEBUG("  description = %s\n", pDriverInfo->description);
	MW_DEBUG("= [G] ====== Driver Info ========\n");

	return 0;
}

int mw_get_version_info(mw_version_info *pVersion)
{
	if (pVersion == NULL) {
		MW_MSG("[mw_get_version_info] : Pointer is NULL!\n");
		return -1;
	}

	pVersion->major = mw_version.major;
	pVersion->minor = mw_version.minor;
	pVersion->patch = mw_version.patch;
	pVersion->update_time = mw_version.update_time;

	MW_DEBUG("= [G] ====== MW Version Info ========\n");
	MW_DEBUG("         major = %x\n", pVersion->major);
	MW_DEBUG("         minor = %x\n", pVersion->minor);
	MW_DEBUG("         patch = %x\n", pVersion->patch);
	MW_DEBUG("     update time = %x\n", pVersion->update_time);
	MW_DEBUG("= [G] ====== MW Version Info ========\n");

	return 0;
}

int mw_display_adj_bin_version(void)
{
	MW_MSG("Adj/Aeb magic number:\t 0x%x\n", G_adj_bin_header.magic_number);
	MW_MSG("Adj/Aeb version:\t %x.%x\n", G_adj_bin_header.header_ver_major,
		G_adj_bin_header.header_ver_minor);

	return 0;
}

int mw_get_log_level(int *pLog)
{
	if (pLog == NULL) {
		MW_MSG("[mw_get_log_level] : Pointer is NULL");
		return -1;
	}
	*pLog = G_mw_log_level;

	return 0;
}

int mw_set_log_level(mw_log_level log)
{
	if (log < MW_ERROR_LEVEL || log >= (MW_LOG_LEVEL_NUM - 1)) {
		MW_MSG("Invalid log level, please set it in the range of (0~%d).\n",
			(MW_LOG_LEVEL_NUM - 1));
		return -1;
	}
	G_mw_log_level = log + 1;

	return 0;
}


