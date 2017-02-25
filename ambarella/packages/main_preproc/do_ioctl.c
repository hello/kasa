/*
 * do_ioctl.c
 *
 * History:
 *  2015/04/01 - [Zhaoyang Chen] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
#include <ctype.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <basetypes.h>
#include "iav_ioctl.h"

#include "lib_vproc.h"
#include "priv.h"

static const char* VIDEO_PROC_STR[IAV_VIDEO_PROC_NUM] = {
	"DPTZ", "PM", "CAWARP"
};

static int G_iav_fd = -1;

static void open_iav(void)
{
	if (unlikely(G_iav_fd < 0)) {
		if ((G_iav_fd = open("/dev/iav", O_RDWR, 0)) < 0) {
			perror("/dev/iav");
			exit(-1);
		}
		TRACE("Open /dev/iav\n");
	}
}

int ioctl_map_pm(u32 buf_id)
{
	static int mapped = 0;
	struct iav_querybuf querybuf;
	open_iav();
	if (unlikely(!mapped)) {
		querybuf.buf = buf_id;
		if (ioctl(G_iav_fd, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
			perror("IAV_IOC_QUERY_BUF");
			return -1;
		}
		vproc.pm_mem.length = querybuf.length;
		vproc.pm_mem.addr = mmap(NULL, querybuf.length, PROT_WRITE, MAP_SHARED,
			G_iav_fd, querybuf.offset);
		mapped = 1;
		TRACE("IAV_IOC_QUERY_BUF for PM\n");
	}

	return 0;
}

int ioctl_get_pm_info(void)
{
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_GET_PRIVACY_MASK_INFO, &vproc.pm_info) < 0) {
		perror("IAV_IOC_GET_PRIVACY_MASK_INFO");
		return -1;
	}
	TRACE("IAV_IOC_GET_PRIVACY_MASK_INFO\n");
	return 0;
}

int ioctl_get_srcbuf_format(int source_id)
{
	open_iav();
	vproc.srcbuf[source_id].buf_id = source_id;
	if (ioctl(G_iav_fd, IAV_IOC_GET_SOURCE_BUFFER_FORMAT,
		&vproc.srcbuf[source_id]) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
		return -1;
	}
	TRACE("IAV_IOC_GET_SOURCE_BUFFER_FORMAT\n");
	return 0;
}

int ioctl_get_srcbuf_setup(void)
{
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &vproc.srcbuf_setup) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP");
		return -1;
	}
	TRACE("IAV_IOC_GET_SOURCE_BUFFER_SETUP\n");
	return 0;
}

int ioctl_get_vin_info(void)
{
	vproc.vin.vsrc_id = 0;
	vproc.vin.info.mode = AMBA_VIDEO_MODE_CURRENT;
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_VIN_GET_VIDEOINFO, &vproc.vin) < 0) {
		perror("IAV_IOC_VIN_GET_VIDEOINFO");
		return -1;
	}
	TRACE("IAV_IOC_VIN_GET_VIDEOINFO\n");
	return 0;
}

int ioctl_gdma_copy(struct iav_gdma_copy* gdma)
{
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_GDMA_COPY, gdma) < 0) {
		perror("IAV_IOC_GDMA_COPY");
		return -1;
	}
	TRACE("IAV_IOC_GDMA_COPY\n");
	return 0;
}

int ioctl_get_system_resource(void)
{
	open_iav();
	vproc.resource.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(G_iav_fd, IAV_IOC_GET_SYSTEM_RESOURCE, &vproc.resource) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE");
		return -1;
	}
	TRACE("IAV_IOC_GET_SYSTEM_RESOURCE\n");
	return 0;
}

int ioctl_get_pm(void)
{
	struct iav_video_proc* vp = &vproc.ioctl_pm;
	vp->cid = IAV_VIDEO_PROC_PM;
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_GET_VIDEO_PROC, vp) < 0) {
		perror("IAV_IOC_GET_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_GET_VIDEO_PROC: %s\n", VIDEO_PROC_STR[vp->cid]);
	return 0;
}

int ioctl_get_dptz(int source_id)
{
	struct iav_video_proc* vp = &vproc.ioctl_dptz[source_id];
	vp->cid = IAV_VIDEO_PROC_DPTZ;
	vp->arg.dptz.buf_id = source_id;
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_GET_VIDEO_PROC, vp) < 0) {
		perror("IAV_IOC_GET_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_GET_VIDEO_PROC: %s %d\n", VIDEO_PROC_STR[vp->cid], source_id);
	return 0;
}

int ioctl_cfg_pm(void)
{
	struct iav_video_proc* vp = &vproc.ioctl_pm;
	vp->cid = IAV_VIDEO_PROC_PM;
	vproc.apply_flag[IAV_VIDEO_PROC_PM].apply = 1;
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_CFG_VIDEO_PROC, vp) < 0) {
		vproc.apply_flag[IAV_VIDEO_PROC_PM].apply = 0;
		perror("IAV_IOC_CFG_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_CFG_VIDEO_PROC: %s\n",VIDEO_PROC_STR[vp->cid]);
	return 0;
}

int ioctl_cfg_cawarp(void)
{
	struct iav_video_proc* vp = &vproc.ioctl_cawarp;
	vp->cid = IAV_VIDEO_PROC_CAWARP;
	vproc.apply_flag[IAV_VIDEO_PROC_CAWARP].apply = 1;
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_CFG_VIDEO_PROC, vp) < 0) {
		vproc.apply_flag[IAV_VIDEO_PROC_CAWARP].apply = 0;
		perror("IAV_IOC_CFG_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_CFG_VIDEO_PROC: %s\n", VIDEO_PROC_STR[vp->cid]);
	return 0;
}

int ioctl_cfg_dptz(int source_id)
{
	struct iav_video_proc* vp = &vproc.ioctl_dptz[source_id];
	vp->cid = IAV_VIDEO_PROC_DPTZ;
	vp->arg.dptz.buf_id = source_id;
	vproc.apply_flag[IAV_VIDEO_PROC_DPTZ].apply = 1;
	vproc.apply_flag[IAV_VIDEO_PROC_DPTZ].param |= (1 << source_id);
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_CFG_VIDEO_PROC, vp) < 0) {
		vproc.apply_flag[IAV_VIDEO_PROC_DPTZ].apply = 0;
		perror("IAV_IOC_CFG_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_CFG_VIDEO_PROC: %s %d\n", VIDEO_PROC_STR[vp->cid], source_id);
	return 0;
}

int ioctl_apply(void)
{
	int i;
	int do_apply = 0;
	for (i = 0; i < IAV_VIDEO_PROC_NUM; i++) {
		if (vproc.apply_flag[i].apply) {
			do_apply = 1;
		}
		DEBUG("%s:\tapply %d, params 0x%x\n", VIDEO_PROC_STR[i],
		    vproc.apply_flag[i].apply, vproc.apply_flag[i].param);
	}
	if (do_apply) {
		open_iav();
		if (ioctl(G_iav_fd, IAV_IOC_APPLY_VIDEO_PROC, vproc.apply_flag) < 0) {
				perror("IAV_IOC_APPLY_VIDEO_PROC");
				return -1;
			}
			TRACE("IAV_IOC_APPLY_VIDEO_PROC\n");
	}
	// Clear all apply flags
	memset(vproc.apply_flag, 0, sizeof(vproc.apply_flag));
	return 0;
}

