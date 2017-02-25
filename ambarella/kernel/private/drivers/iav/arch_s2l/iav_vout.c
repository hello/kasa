
/*
 * iav_vout.c
 *
 * History:
 *	2009/9/9 - [Oliver Li] created file
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


#include <linux/delay.h>
#include <iav_utils.h>
#include <iav_ioctl.h>
#include <dsp_api.h>
#include <dsp_format.h>
#include <vout_api.h>
#include "iav_vout.h"

#define VOUT_FLAGS_ALL		0xFFFFFFFF
#define VOUT_UPDATE_VIDEO	AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP
#define VOUT_UPDATE_OSD		AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP

DEFINE_MUTEX(vout_mutex);

static int update_vout_info(int chan)
{
	int				errorCode = 0;
	struct amba_video_source_info	vout;

	errorCode = amba_vout_video_source_cmd(chan,
		AMBA_VIDEO_SOURCE_UPDATE_IAV_INFO,
		&vout);
	if (errorCode) {
		iav_error("%s: AMBA_VIDEO_SOURCE_UPDATE_IAV_INFO %d\n",
			__func__, errorCode);
		G_voutinfo[chan].enabled = 0;
		goto iav_update_vout_exit;
	}

	G_voutinfo[chan].enabled = vout.enabled;
	G_voutinfo[chan].video_info = vout.video_info;

iav_update_vout_exit:
	return errorCode;
}

static int stop_vout(int chan)
{
	int rval;

	rval = amba_vout_video_source_cmd(chan,
		AMBA_VIDEO_SOURCE_RESET, NULL);
	if (rval)
		goto iav_stop_vout_exit;

	if (G_voutinfo[chan].active_sink_id > 0) {
		rval = amba_vout_video_sink_cmd(G_voutinfo[chan].active_sink_id,
			AMBA_VIDEO_SINK_RESET, NULL);
		if (rval)
			goto iav_stop_vout_exit;
	}

	rval = update_vout_info(chan);
	if (rval)
		goto iav_stop_vout_exit;

iav_stop_vout_exit:
	return rval;
}

static int start_vout(int chan)
{
	int	rval;
	int	vout_enabled = 0;

	rval = update_vout_info(chan);
	if (rval)
		goto iav_start_vout_exit;

	vout_enabled = G_voutinfo[chan].enabled;

	if (!vout_enabled) {
		rval = amba_vout_video_source_cmd(chan,
			AMBA_VIDEO_SOURCE_RUN, NULL);
		if (rval)
			goto iav_start_vout_exit;

		rval = update_vout_info(chan);
		if (rval)
			goto iav_start_vout_exit;
	} else {
		iav_error("%s: bad vout state %d:%d\n", __func__,
			chan, vout_enabled);
		rval = -EAGAIN;
	}

iav_start_vout_exit:
	return rval;
}

static int iav_halt_vout(int vout_id)
{
	int	rval;

	rval = amba_vout_video_source_cmd(vout_id,
		AMBA_VIDEO_SOURCE_HALT, NULL);
	if (rval)
		goto iav_halt_vout_exit;

	rval = update_vout_info(vout_id);
	if (rval)
		goto iav_halt_vout_exit;

iav_halt_vout_exit:
	return rval;
}

static int iav_select_output_dev(int dev)
{
	int	rval;
	int	source_id;

	rval = amba_vout_video_sink_cmd(dev,
		AMBA_VIDEO_SINK_GET_SOURCE_ID, &source_id);
	if (rval)
		goto iav_select_output_dev_exit;

	if (source_id) {
		G_voutinfo[1].active_sink_id = dev;
	} else {
		G_voutinfo[0].active_sink_id = dev;
	}

iav_select_output_dev_exit:
	return rval;
}

static int iav_configure_sink(struct amba_video_sink_mode __user *pcfg)
{
	struct amba_video_sink_mode		sink_cfg, vout_mode;
	int					rval;
	int					source_id;

	if (copy_from_user(&sink_cfg, pcfg, sizeof(sink_cfg))) {
		rval = -EFAULT;
		goto iav_configure_sink_exit;
	}

	if (sink_cfg.id == -1)
		sink_cfg.id = G_voutinfo[1].active_sink_id;
	rval = amba_vout_video_sink_cmd(sink_cfg.id,
		AMBA_VIDEO_SINK_GET_SOURCE_ID, &source_id);
	if (rval)
		goto iav_configure_sink_exit;

	rval = stop_vout(source_id);
	if (rval)
		goto iav_configure_sink_exit;

	memcpy(&vout_mode, &sink_cfg, sizeof(vout_mode));
	rval = amba_vout_video_sink_cmd(sink_cfg.id,
		AMBA_VIDEO_SINK_SET_MODE, &vout_mode);
	if (rval)
		goto iav_configure_sink_exit;
	if (source_id) {
		G_voutinfo[1].active_mode = sink_cfg;
		G_voutinfo[1].active_sink_id = sink_cfg.id;
	} else {
		G_voutinfo[0].active_mode = sink_cfg;
		G_voutinfo[0].active_sink_id = sink_cfg.id;
	}

	rval = amba_vout_video_source_cmd(source_id,
		AMBA_VIDEO_SOURCE_SET_DISPLAY_INPUT, &sink_cfg.display_input);
	if (rval)
		goto iav_configure_sink_exit;

	rval = amba_vout_video_source_cmd(source_id,
		AMBA_VIDEO_SOURCE_SET_MIXER_CSC, &sink_cfg.mixer_csc);
	if (rval)
		goto iav_configure_sink_exit;

	rval = amba_vout_video_source_cmd(source_id,
		AMBA_VIDEO_SOURCE_SET_BG_COLOR, &sink_cfg.bg_color);
	if (rval)
		goto iav_configure_sink_exit;

	rval = start_vout(source_id);

	//Send cmds to dsp directly if needed
	if (sink_cfg.direct_to_dsp) {
		u32				flag = VOUT_FLAGS_ALL;

		amba_vout_video_source_cmd(source_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);

	}

iav_configure_sink_exit:
	return rval;
}

static int iav_enable_vout_csc(struct iav_vout_enable_csc_s * arg)
{
	int				rval;
	struct iav_vout_enable_csc_s	vout_enable_csc;
	u32				flag;

	if (copy_from_user(&vout_enable_csc, arg,
		sizeof(struct iav_vout_enable_csc_s))) {
		rval = -EFAULT;
		goto iav_enable_vout_csc_exit;
	}

	if (vout_enable_csc.vout_id < 0 || vout_enable_csc.vout_id > 1) {
		rval = -EPERM;
		goto iav_enable_vout_csc_exit;
	}

	rval = amba_osd_on_csc_change(vout_enable_csc.vout_id,
		vout_enable_csc.csc_en);
	if (rval)
		goto iav_enable_vout_csc_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_CSC_SETUP |
			AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP;
	rval = amba_vout_video_source_cmd(vout_enable_csc.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	if (rval)
		goto iav_enable_vout_csc_exit;

	G_voutinfo[vout_enable_csc.vout_id].active_mode.csc_en
		= vout_enable_csc.csc_en;

iav_enable_vout_csc_exit:
	return rval;
}

static int iav_enable_vout_video(struct iav_vout_enable_video_s * arg)
{
	int				rval;
	vout_video_setup_t		video_setup;
	struct iav_vout_enable_video_s	vout_enable_video;
	u32				flag;

	if (copy_from_user(&vout_enable_video, arg,
		sizeof(struct iav_vout_enable_video_s))) {
		rval = -EFAULT;
		goto iav_enable_vout_video_exit;
	}

	if (vout_enable_video.vout_id < 0 || vout_enable_video.vout_id > 1) {
		rval = -EPERM;
		goto iav_enable_vout_video_exit;
	}

	rval = amba_vout_video_source_cmd(vout_enable_video.vout_id,
		AMBA_VIDEO_SOURCE_GET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_enable_vout_video_exit;

	video_setup.en = vout_enable_video.video_en;
	rval = amba_vout_video_source_cmd(vout_enable_video.vout_id,
		AMBA_VIDEO_SOURCE_SET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_enable_vout_video_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP;
	rval = amba_vout_video_source_cmd(vout_enable_video.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	if (rval)
		goto iav_enable_vout_video_exit;

	G_voutinfo[vout_enable_video.vout_id].active_mode.video_en
		= vout_enable_video.video_en;

	msleep(100);

iav_enable_vout_video_exit:
	return rval;
}

static int iav_flip_vout_video(struct iav_vout_flip_video_s * arg)
{
	int				rval;
	vout_video_setup_t		video_setup;
	struct iav_vout_flip_video_s	vout_flip_video;
	u32				flag;

	if (copy_from_user(&vout_flip_video, arg,
		sizeof(struct iav_vout_flip_video_s))) {
		rval = -EFAULT;
		goto iav_flip_vout_video_exit;
	}

	if (vout_flip_video.vout_id < 0 || vout_flip_video.vout_id > 1) {
		rval = -EPERM;
		goto iav_flip_vout_video_exit;
	}

	rval = amba_vout_video_source_cmd(vout_flip_video.vout_id,
		AMBA_VIDEO_SOURCE_GET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_flip_vout_video_exit;

	video_setup.flip = vout_flip_video.flip;
	rval = amba_vout_video_source_cmd(vout_flip_video.vout_id,
		AMBA_VIDEO_SOURCE_SET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_flip_vout_video_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP;
	rval = amba_vout_video_source_cmd(vout_flip_video.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	if (rval)
		goto iav_flip_vout_video_exit;

	G_voutinfo[vout_flip_video.vout_id].active_mode.video_flip = vout_flip_video.flip;

iav_flip_vout_video_exit:
	return rval;
}

static int iav_rotate_vout_video(struct iav_vout_rotate_video_s * arg)
{
	int				rval;
	vout_video_setup_t		video_setup;
	struct iav_vout_rotate_video_s	vout_rotate_video;
	u32				flag;

	if (copy_from_user(&vout_rotate_video, arg,
		sizeof(struct iav_vout_rotate_video_s))) {
		rval = -EFAULT;
		goto iav_rotate_vout_video_exit;
	}

	if (vout_rotate_video.vout_id < 0 || vout_rotate_video.vout_id > 1) {
		rval = -EPERM;
		goto iav_rotate_vout_video_exit;
	}

	rval = amba_vout_video_source_cmd(vout_rotate_video.vout_id,
		AMBA_VIDEO_SOURCE_GET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_rotate_vout_video_exit;

	video_setup.rotate = vout_rotate_video.rotate;
	rval = amba_vout_video_source_cmd(vout_rotate_video.vout_id,
		AMBA_VIDEO_SOURCE_SET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_rotate_vout_video_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP;
	rval = amba_vout_video_source_cmd(vout_rotate_video.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	if (rval)
		goto iav_rotate_vout_video_exit;

	G_voutinfo[vout_rotate_video.vout_id].active_mode.video_rotate
		= vout_rotate_video.rotate;

iav_rotate_vout_video_exit:
	return rval;
}

static int iav_change_vout_video_size(struct iav_vout_change_video_size_s *arg)
{
	int					rval;
	vout_video_setup_t			video_setup;
	struct iav_vout_change_video_size_s	cvs;
	video_preview_t				video_preview;
	u32					flag;
	int					video_en;

	if (copy_from_user(&cvs, arg,
		sizeof(struct iav_vout_change_video_size_s))) {
		rval = -EFAULT;
		goto iav_change_vout_video_size_exit;
	}

	if (cvs.vout_id < 0 || cvs.vout_id > 1) {
		rval = -EPERM;
		goto iav_change_vout_video_size_exit;
	}

	/* Change video size and disable video temporarily */
	rval = amba_vout_video_source_cmd(cvs.vout_id,
		AMBA_VIDEO_SOURCE_GET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_change_vout_video_size_exit;

	video_en		= video_setup.en;
	video_setup.en		= 0;
	video_setup.win_width	= cvs.width;
	video_setup.win_height	= cvs.height;
	if (G_voutinfo[cvs.vout_id].video_info.format
		== AMBA_VIDEO_FORMAT_INTERLACE)
		video_setup.win_height >>= 1;
	rval = amba_vout_video_source_cmd(cvs.vout_id,
		AMBA_VIDEO_SOURCE_SET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_change_vout_video_size_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP;
	rval = amba_vout_video_source_cmd(cvs.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	msleep(50);

	/* Chage preview video size */
	memset(&video_preview, 0, sizeof(video_preview_t));
	video_preview.cmd_code		= VIDEO_PREVIEW_SETUP;
	video_preview.preview_id	= cvs.vout_id;
	video_preview.preview_en	= 0;
	video_preview.preview_w		= cvs.width;
	video_preview.preview_h		= cvs.height;
	video_preview.preview_format	=
		amba_iav_format_to_format(G_voutinfo[cvs.vout_id].video_info.format);
	video_preview.preview_frame_rate=
		amba_iav_fps_to_fps(G_voutinfo[cvs.vout_id].video_info.fps);
	dsp_issue_cmd(&video_preview, sizeof(video_preview_t));
	msleep(100);

	memset(&video_preview, 0, sizeof(video_preview_t));
	video_preview.cmd_code		= VIDEO_PREVIEW_SETUP;
	video_preview.preview_id	= cvs.vout_id;
	video_preview.preview_en	= 1;
	video_preview.preview_w		= cvs.width;
	video_preview.preview_h		= cvs.height;
	video_preview.preview_format	=
		amba_iav_format_to_format(G_voutinfo[cvs.vout_id].video_info.format);
	video_preview.preview_frame_rate=
		amba_iav_fps_to_fps(G_voutinfo[cvs.vout_id].video_info.fps);
	dsp_issue_cmd(&video_preview, sizeof(video_preview_t));
	msleep(100);

	/* Enable video again if enabled before */
	if (video_en) {
		rval = amba_vout_video_source_cmd(cvs.vout_id,
			AMBA_VIDEO_SOURCE_GET_VOUT_SETUP, &video_setup);
		if (rval)
			goto iav_change_vout_video_size_exit;

		video_setup.en = video_en;
		rval = amba_vout_video_source_cmd(cvs.vout_id,
			AMBA_VIDEO_SOURCE_SET_VOUT_SETUP, &video_setup);
		if (rval)
			goto iav_change_vout_video_size_exit;

		flag = AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP;
		rval = amba_vout_video_source_cmd(cvs.vout_id,
				AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
		if (rval)
			goto iav_change_vout_video_size_exit;
		else
			msleep(50);
	}

	G_voutinfo[cvs.vout_id].active_mode.video_size.specified
		= 1;
	G_voutinfo[cvs.vout_id].active_mode.video_size.video_width
		= cvs.width;
	G_voutinfo[cvs.vout_id].active_mode.video_size.video_height
		= cvs.height;

iav_change_vout_video_size_exit:
	return rval;
}

static int iav_change_vout_video_offset(struct iav_vout_change_video_offset_s *arg)
{
	int					rval;
	vout_video_setup_t			video_setup;
	struct iav_vout_change_video_offset_s	cvo;
	struct amba_vout_window_info		active_window;
	int					height;
	u32					flag;

	if (copy_from_user(&cvo, arg,
		sizeof(struct iav_vout_change_video_offset_s))) {
		rval = -EFAULT;
		goto iav_change_vout_video_offset_exit;
	}

	if (cvo.vout_id < 0 || cvo.vout_id > 1) {
		rval = -EPERM;
		goto iav_change_vout_video_offset_exit;
	}

	/* Change video size and disable video temporarily */
	rval = amba_vout_video_source_cmd(cvo.vout_id,
		AMBA_VIDEO_SOURCE_GET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_change_vout_video_offset_exit;

	rval = amba_vout_video_source_cmd(cvo.vout_id,
		AMBA_VIDEO_SOURCE_GET_ACTIVE_WIN, &active_window);
	if (rval)
		goto iav_change_vout_video_offset_exit;

	height = active_window.end_y - active_window.start_y + 1;
	if (cvo.specified) {
		video_setup.win_offset_x = cvo.offset_x;
		video_setup.win_offset_y = cvo.offset_y;
		if (G_voutinfo[cvo.vout_id].video_info.format
			== AMBA_VIDEO_FORMAT_INTERLACE)
			video_setup.win_offset_y >>= 1;

		if (video_setup.win_offset_x + video_setup.win_width > active_window.width)
			video_setup.win_offset_x = active_window.width - video_setup.win_width;
		if (video_setup.win_offset_y + video_setup.win_height > height)
			video_setup.win_offset_y = height - video_setup.win_height;
	} else {
		video_setup.win_offset_x = (active_window.width - video_setup.win_width) >> 1;
		video_setup.win_offset_y = (height - video_setup.win_height) >> 1;
	}

	rval = amba_vout_video_source_cmd(cvo.vout_id,
		AMBA_VIDEO_SOURCE_SET_VOUT_SETUP, &video_setup);
	if (rval)
		goto iav_change_vout_video_offset_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP;
	rval = amba_vout_video_source_cmd(cvo.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	if (rval)
		goto iav_change_vout_video_offset_exit;

	G_voutinfo[cvo.vout_id].active_mode.video_offset.specified
		= 1;
	G_voutinfo[cvo.vout_id].active_mode.video_offset.offset_x
		= cvo.offset_x;
	G_voutinfo[cvo.vout_id].active_mode.video_offset.offset_y
		= cvo.offset_y;

iav_change_vout_video_offset_exit:
	return rval;
}

static int iav_select_vout_fb(struct iav_vout_fb_sel_s * arg)
{
	int				rval;
	struct iav_vout_fb_sel_s	vout_fb_sel;
	u32				flag;

	if (copy_from_user(&vout_fb_sel, arg, sizeof(struct iav_vout_fb_sel_s))) {
		rval = -EFAULT;
		goto iav_select_vout_fb_exit;
	}

	if (vout_fb_sel.vout_id < 0 || vout_fb_sel.vout_id > 1) {
		rval = -EPERM;
		goto iav_select_vout_fb_exit;
	}

	rval = amba_osd_on_fb_switch(vout_fb_sel.vout_id, vout_fb_sel.fb_id);
	if (rval)
		goto iav_select_vout_fb_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP;
	rval = amba_vout_video_source_cmd(vout_fb_sel.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	if (rval)
		goto iav_select_vout_fb_exit;

	G_voutinfo[vout_fb_sel.vout_id].active_mode.fb_id = vout_fb_sel.fb_id;

iav_select_vout_fb_exit:
	return rval;
}

static int iav_flip_vout_osd(struct iav_vout_flip_osd_s * arg)
{
	int				rval;
	vout_osd_setup_t		osd_setup;
	struct iav_vout_flip_osd_s	vout_flip_osd;
	u32				flag;

	if (copy_from_user(&vout_flip_osd, arg,
		sizeof(struct iav_vout_flip_osd_s))) {
		rval = -EFAULT;
		goto iav_flip_vout_osd_exit;
	}

	if (vout_flip_osd.vout_id < 0 || vout_flip_osd.vout_id > 1) {
		rval = -EPERM;
		goto iav_flip_vout_osd_exit;
	}

	rval = amba_vout_video_source_cmd(vout_flip_osd.vout_id,
		AMBA_VIDEO_SOURCE_GET_OSD, &osd_setup);
	if (rval)
		goto iav_flip_vout_osd_exit;

	osd_setup.flip = vout_flip_osd.flip;
	rval = amba_vout_video_source_cmd(vout_flip_osd.vout_id,
		AMBA_VIDEO_SOURCE_SET_OSD, &osd_setup);
	if (rval)
		goto iav_flip_vout_osd_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP;
	rval = amba_vout_video_source_cmd(vout_flip_osd.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	if (rval)
		goto iav_flip_vout_osd_exit;

	G_voutinfo[vout_flip_osd.vout_id].active_mode.osd_flip = vout_flip_osd.flip;

iav_flip_vout_osd_exit:
	return rval;
}

static int iav_enable_vout_osd_rescaler(struct iav_vout_enable_osd_rescaler_s * arg)
{
	int					rval;
	struct iav_vout_enable_osd_rescaler_s	vout_enable_rescaler;
	u32					flag;

	if (copy_from_user(&vout_enable_rescaler, arg,
		sizeof(struct iav_vout_enable_osd_rescaler_s))) {
		rval = -EFAULT;
		goto iav_enable_vout_osd_rescaler_exit;
	}

	if (vout_enable_rescaler.vout_id < 0
		|| vout_enable_rescaler.vout_id > 1) {
		rval = -EPERM;
		goto iav_enable_vout_osd_rescaler_exit;
	}

	rval = amba_osd_on_rescaler_change(vout_enable_rescaler.vout_id,
			vout_enable_rescaler.enable, vout_enable_rescaler.width,
			vout_enable_rescaler.height);
	if (rval)
		goto iav_enable_vout_osd_rescaler_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP;
	rval = amba_vout_video_source_cmd(vout_enable_rescaler.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	if (rval)
		goto iav_enable_vout_osd_rescaler_exit;

	G_voutinfo[vout_enable_rescaler.vout_id].active_mode.osd_rescale.enable
		= vout_enable_rescaler.enable;
	G_voutinfo[vout_enable_rescaler.vout_id].active_mode.osd_rescale.width
		= vout_enable_rescaler.width;
	G_voutinfo[vout_enable_rescaler.vout_id].active_mode.osd_rescale.height
		= vout_enable_rescaler.height;

iav_enable_vout_osd_rescaler_exit:
	return rval;
}

int iav_change_vout_osd_offset(struct iav_vout_change_osd_offset_s *arg)
{
	int					rval;
	struct iav_vout_change_osd_offset_s	coo;
	u32					flag;

	if (copy_from_user(&coo, arg,
		sizeof(struct iav_vout_change_osd_offset_s))) {
		rval = -EFAULT;
		goto iav_change_vout_osd_offset_exit;
	}

	if (coo.vout_id < 0 || coo.vout_id > 1) {
		rval = -EPERM;
		goto iav_change_vout_osd_offset_exit;
	}

	rval = amba_osd_on_offset_change(coo.vout_id, coo.specified,
		coo.offset_x, coo.offset_y);
	if (rval)
		goto iav_change_vout_osd_offset_exit;

	flag = AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP;
	rval = amba_vout_video_source_cmd(coo.vout_id,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	if (rval)
		goto iav_change_vout_osd_offset_exit;

	G_voutinfo[coo.vout_id].active_mode.osd_offset.specified
		= coo.specified;
	G_voutinfo[coo.vout_id].active_mode.osd_offset.offset_x
		= coo.offset_x;
	G_voutinfo[coo.vout_id].active_mode.osd_offset.offset_y
		= coo.offset_y;

iav_change_vout_osd_offset_exit:
	return rval;
}

static int iav_get_vout_sink_info(struct amba_vout_sink_info *args)
{
	int				errorCode = 0;
	struct amba_vout_sink_info	sink_info;

	errorCode = copy_from_user(&sink_info,
		(struct amba_vout_sink_info __user *)args,
		sizeof(struct amba_vout_sink_info)) ? -EFAULT : 0;
	if (errorCode)
		return errorCode;

	errorCode = amba_vout_video_sink_cmd(sink_info.id,
		AMBA_VIDEO_SINK_GET_INFO, &sink_info);
	if (errorCode)
		return errorCode;

	if (sink_info.source_id) {
		sink_info.state = G_voutinfo[1].enabled;
		sink_info.sink_mode = G_voutinfo[1].active_mode;

	} else {
		sink_info.state = G_voutinfo[0].enabled;
		sink_info.sink_mode = G_voutinfo[0].active_mode;
	}

	errorCode = copy_to_user(
		(struct amba_vout_sink_info __user *)args,
		&sink_info,
		sizeof(struct amba_vout_sink_info)) ? -EFAULT : 0;

	return errorCode;
}
static int iav_get_vout_sink_num(int __user *arg)
{
	int			counter = 0;
	int			tmp;
	int			errorCode = 0;

	errorCode = amba_vout_video_source_cmd(0,
		AMBA_VIDEO_SOURCE_GET_SINK_NUM, &tmp);
	if (errorCode)
		return errorCode;

	counter += tmp;
	errorCode = amba_vout_video_source_cmd(1,
		AMBA_VIDEO_SOURCE_GET_SINK_NUM, &tmp);
	if (errorCode)
		return errorCode;

	counter += tmp;
	if (!errorCode) {
		errorCode = put_user(counter, (int __user *)arg);
	}

	return errorCode;
}

void iav_config_vout(vout_src_t vout_src)
{
	u32 flag = VOUT_FLAGS_ALL, i;
	vout_video_setup_t def_vout_data;

	for (i = 0; i < 2; i++) {
		amba_vout_video_source_cmd(i, AMBA_VIDEO_SOURCE_GET_VOUT_SETUP,
			&def_vout_data);

		def_vout_data.src = vout_src;

		amba_vout_video_source_cmd(i, AMBA_VIDEO_SOURCE_SET_VOUT_SETUP,
			&def_vout_data);
		amba_vout_video_source_cmd(i,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	}
}

void iav_change_vout_src(vout_src_t vout_src)
{
	u32 flag = VOUT_UPDATE_VIDEO, i;
	vout_video_setup_t def_vout_data;

	for (i = 0; i < 2; i++) {
		amba_vout_video_source_cmd(i, AMBA_VIDEO_SOURCE_GET_VOUT_SETUP,
			&def_vout_data);

		def_vout_data.src = vout_src;

		amba_vout_video_source_cmd(i, AMBA_VIDEO_SOURCE_SET_VOUT_SETUP,
			&def_vout_data);
		amba_vout_video_source_cmd(i,
			AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	}
}

int iav_vout_ioctl(struct ambarella_iav *iav, unsigned int cmd, unsigned long arg)
{
	int rval = -ENOIOCTLCMD;

	mutex_lock(&vout_mutex);

	switch (cmd) {
	case IAV_IOC_VOUT_HALT:
		rval = iav_halt_vout((int)arg);
		break;

	case IAV_IOC_VOUT_SELECT_DEV:
		rval = iav_select_output_dev((int)arg);
		break;

	case IAV_IOC_VOUT_CONFIGURE_SINK:
		rval = iav_configure_sink((struct amba_video_sink_mode __user *)arg);
		break;

	case IAV_IOC_VOUT_ENABLE_CSC:
		rval = iav_enable_vout_csc((struct iav_vout_enable_csc_s *)arg);
		break;

	case IAV_IOC_VOUT_ENABLE_VIDEO:
		rval = iav_enable_vout_video((struct iav_vout_enable_video_s *)arg);
		break;

	case IAV_IOC_VOUT_FLIP_VIDEO:
		rval = iav_flip_vout_video((struct iav_vout_flip_video_s *)arg);
		break;

	case IAV_IOC_VOUT_ROTATE_VIDEO:
		rval = iav_rotate_vout_video((struct iav_vout_rotate_video_s *)arg);
		break;

	case IAV_IOC_VOUT_CHANGE_VIDEO_SIZE:
		rval = iav_change_vout_video_size((struct iav_vout_change_video_size_s *)arg);
		break;

	case IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET:
		rval = iav_change_vout_video_offset((struct iav_vout_change_video_offset_s *)arg);
		break;

	case IAV_IOC_VOUT_SELECT_FB:
		rval = iav_select_vout_fb((struct iav_vout_fb_sel_s *)arg);
		break;

	case IAV_IOC_VOUT_FLIP_OSD:
		rval = iav_flip_vout_osd((struct iav_vout_flip_osd_s *)arg);
		break;

	case IAV_IOC_VOUT_ENABLE_OSD_RESCALER:
		rval = iav_enable_vout_osd_rescaler((struct iav_vout_enable_osd_rescaler_s *)arg);
		break;

	case IAV_IOC_VOUT_CHANGE_OSD_OFFSET:
		rval = iav_change_vout_osd_offset((struct iav_vout_change_osd_offset_s *)arg);
		break;

	case IAV_IOC_VOUT_GET_SINK_NUM:
		rval = iav_get_vout_sink_num((int __user *)arg);
		break;

	case IAV_IOC_VOUT_GET_SINK_INFO:
		rval = iav_get_vout_sink_info((struct amba_vout_sink_info *)arg);
		break;


	default:
		iav_error("Unknown IOCTL: (0x%x) %d:%c:%d:%d\n", cmd,
			_IOC_DIR(cmd), _IOC_TYPE(cmd), _IOC_NR(cmd), _IOC_SIZE(cmd));
		rval = -ENOIOCTLCMD;
		break;
	}

	mutex_unlock(&vout_mutex);

	return rval;
}

