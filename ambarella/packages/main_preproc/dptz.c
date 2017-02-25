/*
 * dptz.c
 *
 * History:
 *  2015/04/02 - [Zhaoyang Chen] created file
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

#include <basetypes.h>
#include "iav_ioctl.h"

#include "lib_vproc.h"
#include "priv.h"

static int G_dptz_inited;

static int check_dptz(struct iav_digital_zoom* dptz)
{
	int src_width, src_height;

	if (!dptz) {
		ERROR("DPTZ is NULL.\n");
		return -1;
	}
	if (dptz->buf_id < IAV_SRCBUF_MN || dptz->buf_id > IAV_SRCBUF_LAST) {
		ERROR("Invalid DPTZ source buffer id [%d].\n", dptz->buf_id);
		return -1;
	}
	if (dptz->buf_id == IAV_SRCBUF_MN) {
		src_width = vproc.vin.info.width;
		src_height = vproc.vin.info.height;
	} else {
		src_width = vproc.srcbuf[0].size.width;
		src_height = vproc.srcbuf[0].size.height;
	}
	if (!dptz->input.width) {
		dptz->input.width = src_width;
	}
	if (!dptz->input.height) {
		dptz->input.height = src_height;
	}
	if ((dptz->input.x + dptz->input.width > src_width)
	    || (dptz->input.y + dptz->input.height > src_height)) {
		ERROR("DPTZ input window (x %d, y %d, w %d, h %d) is "
			"out of %s %dx%d.\n", dptz->input.x, dptz->input.y,
		    dptz->input.width, dptz->input.height,
		    dptz->buf_id == IAV_SRCBUF_MN ? "vin" :
		        "main buffer", src_width, src_height);
		return -1;
	}
	return 0;
}

int init_dptz(void)
{
	int i;

	//always reflesh DPTZ parameter
	for (i = 0; i < IAV_SRCBUF_LAST; i++) {
		if (ioctl_get_dptz(i) < 0) {
			return -1;
		}
	}

	if (unlikely(!G_dptz_inited)) {
		do {
			if (ioctl_get_vin_info() < 0) {
				break;
			}
			DEBUG("vin %dx%d\n", vproc.vin.info.width, vproc.vin.info.height);
			for (i = 0; i < IAV_SRCBUF_LAST; i++) {
				if (ioctl_get_srcbuf_format(i) < 0) {
					break;
				}
				DEBUG("buffer %d %dx%d, input %dx%d+%dx%d\n",
				    vproc.dptz[i]->buf_id, vproc.srcbuf[i].size.width,
				    vproc.srcbuf[i].size.height, vproc.dptz[i]->input.width,
				    vproc.dptz[i]->input.height, vproc.dptz[i]->input.x,
				    vproc.dptz[i]->input.y);
			}
			G_dptz_inited = 1;
		} while (0);
	}
	return G_dptz_inited ? 0 : -1;
}

int deinit_dptz(void)
{
	if (G_dptz_inited) {
		G_dptz_inited = 0;
		DEBUG("done\n");
	}
	return G_dptz_inited ? -1 : 0;
}

int operate_dptz(struct iav_digital_zoom* dptz)
{
	if (init_dptz() < 0)
		return -1;

	if (check_dptz(dptz) < 0)
		return -1;

	*vproc.dptz[dptz->buf_id] = *dptz;

	return ioctl_cfg_dptz(dptz->buf_id);
}
