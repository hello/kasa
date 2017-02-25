/*
 * kernel/private/drivers/ambarella/vout/osd/amb_osd/arch_a5s/ambosd_arch.c
 *
 * History:
 *    2009/11/02 - [Zhenwu Xue] Initial revision
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

#include <mach/init.h>
#include "dsp_cmd_msg.h"

#define        MIN(a, b) (((a) < (b)) ? (a) : (b))

extern int dma_memcpy(u8 *dest_addr, u8 *src_addr, u32 size);

static int fb_memcpy(u8 *dest_addr, u8 *src_addr, u32 size)
{
#if 0
	return dma_memcpy((u8 *)ambarella_virt_to_phys((u32)dest_addr),
		(u8 *)ambarella_virt_to_phys((u32)src_addr),
		size);
#else
	memcpy(dest_addr, src_addr, size);
	return 0;
#endif
}

static int amba_direct_pan_display_arch(struct fb_info *info,
	ambosd_info_t *posd_info, u8 *pDstOSDBuffer)
{
	int			i, vout_id;
	vout_osd_buf_setup_t	osd_buf_setup;

       if(posd_info->conversion_buf.available){
              osd_buf_setup.osd_buf_dram_addr =
                     posd_info->conversion_buf.base_buf_phy;
       }else{
              osd_buf_setup.osd_buf_dram_addr =
                     ambarella_virt_to_phys((u32)pDstOSDBuffer);
       }

	osd_buf_setup.osd_buf_pitch = info->fix.line_length;

	for (i = 0; i < 2; i++) {
		vout_id = posd_info->vout_id[i];
		if (vout_id == 0 || vout_id == 1) {
			amba_vout_video_source_cmd(vout_id,
				AMBA_VIDEO_SOURCE_SET_OSD_BUFFER,
				&osd_buf_setup);
		}
	}

	return 0;
}

static int amba_rgb2clut_pan_display_arch(struct fb_info *info,
	ambosd_info_t *posd_info, u8 *pDstOSDBuffer)
{
	int			i, vout_id;
	vout_osd_buf_setup_t	osd_buf_setup;

       if(posd_info->conversion_buf.available){
              if(pDstOSDBuffer == (u8 *)posd_info->conversion_buf.ping_buf)
                     osd_buf_setup.osd_buf_dram_addr =
                            posd_info->conversion_buf.ping_buf_phy;
              if(pDstOSDBuffer == (u8 *)posd_info->conversion_buf.pong_buf)
                     osd_buf_setup.osd_buf_dram_addr =
                            posd_info->conversion_buf.pong_buf_phy;
       }else{
              osd_buf_setup.osd_buf_dram_addr =
                     ambarella_virt_to_phys((u32)pDstOSDBuffer);
       }

	osd_buf_setup.osd_buf_pitch = info->fix.line_length >> 1;

	for (i = 0; i < 2; i++) {
		vout_id = posd_info->vout_id[i];
		if (vout_id == 0 || vout_id == 1) {
			amba_vout_video_source_cmd(vout_id,
				AMBA_VIDEO_SOURCE_SET_OSD_BUFFER,
				&osd_buf_setup);
		}
	}

	return 0;
}

static int amba_rgb2yuv_pan_display_arch(struct fb_info *info,
	ambosd_info_t *posd_info, u16 *pDstOSDBuffer)
{
	int			i, vout_id;
	vout_osd_buf_setup_t	osd_buf_setup;

       if(posd_info->conversion_buf.available){
              if(pDstOSDBuffer == (u16 *)posd_info->conversion_buf.ping_buf)
                     osd_buf_setup.osd_buf_dram_addr =
                            posd_info->conversion_buf.ping_buf_phy;
              if(pDstOSDBuffer == (u16 *)posd_info->conversion_buf.pong_buf)
                     osd_buf_setup.osd_buf_dram_addr =
                            posd_info->conversion_buf.pong_buf_phy;
       }else{
              osd_buf_setup.osd_buf_dram_addr =
                     ambarella_virt_to_phys((u32)pDstOSDBuffer);
       }

	osd_buf_setup.osd_buf_pitch = info->fix.line_length;
	if (posd_info->color_format == AMBFB_COLOR_RGBA8888 ||
		posd_info->color_format == AMBFB_COLOR_ABGR8888) {
		osd_buf_setup.osd_buf_pitch >>= 1;
	}

	for (i = 0; i < 2; i++) {
		vout_id = posd_info->vout_id[i];
		if (vout_id == 0 || vout_id == 1) {
			amba_vout_video_source_cmd(vout_id,
				AMBA_VIDEO_SOURCE_SET_OSD_BUFFER,
				&osd_buf_setup);
		}
	}

	return 0;
}

static int amba_osd_on_vout_change_pre_arch(ambosd_info_t *posd_info)
{
	if (posd_info) {
		posd_info->support_direct_mode	= 1;
		posd_info->support_mixer_csc	= 1;
	}

	return 0;
}

static int amba_osd_on_vout_change_arch(int vout_id,
	struct amba_video_sink_mode *sink_mode,
	ambosd_info_t *posd_info,
	struct ambarella_platform_fb *pplatform_info)
{
	int 					ret = 0;
	struct amba_vout_window_info		active_window;
	int					width, height;
	vout_osd_setup_t			def_osd_setup;
        struct amba_video_source_osd_clut_info  osd_clut_info;
	int					blank_x, blank_y;

	/* Disable OSD */
	if (!posd_info) {
		memset(&def_osd_setup, 0, sizeof(vout_osd_setup_t));
		amba_vout_video_source_cmd(vout_id,
			AMBA_VIDEO_SOURCE_SET_OSD, &def_osd_setup);
		goto amba_osd_on_vout_change_arch_exit;
	}

        ret = amba_vout_video_source_cmd(vout_id,
		AMBA_VIDEO_SOURCE_GET_ACTIVE_WIN, &active_window);
        if (ret) {
                vout_err("%s: AMBA_VIDEO_SOURCE_GET_ACTIVE_WIN %d\n",
                        __func__, ret);
                goto amba_osd_on_vout_change_arch_exit;
        } else {
		width	= active_window.width;
		height	= active_window.end_y - active_window.start_y + 1;
		if (posd_info->interlace[vout_id]) {
			height <<= 1;
		}
        }

	def_osd_setup.en		= 1;
	def_osd_setup.flip		= sink_mode->osd_flip;
	def_osd_setup.rescaler_en	= sink_mode->osd_rescale.enable;
	def_osd_setup.premultiplied	= 0;
	def_osd_setup.global_blend	= 0xFF;
	if (!def_osd_setup.rescaler_en) {
		def_osd_setup.win_width		= MIN(width, pplatform_info->screen_var.xres);
		def_osd_setup.win_height	= MIN(height, pplatform_info->screen_var.yres);
	} else {
		def_osd_setup.win_width		= sink_mode->osd_rescale.width;
		def_osd_setup.win_height	= sink_mode->osd_rescale.height;
	}
	blank_x	= width  - def_osd_setup.win_width;
	blank_y	= height - def_osd_setup.win_height;
	if (sink_mode->osd_offset.specified) {
		def_osd_setup.win_offset_x = MIN(sink_mode->osd_offset.offset_x, blank_x);
		def_osd_setup.win_offset_y = MIN(sink_mode->osd_offset.offset_y, blank_y);
	} else {
		def_osd_setup.win_offset_x = blank_x >> 1;
		def_osd_setup.win_offset_y = blank_y >> 1;
	}
	if (!def_osd_setup.rescaler_en) {
		def_osd_setup.rescaler_input_width  = 0;
		def_osd_setup.rescaler_input_height = 0;
	} else {
		def_osd_setup.rescaler_input_width  = MIN(width, pplatform_info->screen_var.xres);
		def_osd_setup.rescaler_input_height = MIN(height, pplatform_info->screen_var.yres);
	}
	def_osd_setup.osd_buf_repeat_field	= 0;
	def_osd_setup.osd_buf_dram_addr		= pplatform_info->screen_fix.smem_start
		+ pplatform_info->screen_fix.line_length * pplatform_info->screen_var.yoffset;
	def_osd_setup.osd_buf_pitch		=
		pplatform_info->screen_fix.line_length;

	if (posd_info->interlace[vout_id]) {
		def_osd_setup.win_height		>>= 1;
		def_osd_setup.win_offset_y		>>= 1;
		def_osd_setup.rescaler_input_height	>>= 1;
	}

	switch (pplatform_info->color_format) {
	case AMBFB_COLOR_CLUT_8BPP:
		def_osd_setup.src			= OSD_SRC_MAPPED_IN;
		def_osd_setup.osd_direct_mode		= 0;
		def_osd_setup.osd_transparent_color_en	= 0;
		amba_vout_video_source_cmd(vout_id, AMBA_VIDEO_SOURCE_SET_OSD, &def_osd_setup);

		osd_clut_info.pclut_table		= pplatform_info->clut_table;
		osd_clut_info.pblend_table		= pplatform_info->blend_table;
		amba_vout_video_source_cmd(vout_id, AMBA_VIDEO_SOURCE_SET_OSD_CLUT, &osd_clut_info);

		break;

	case AMBFB_COLOR_AUTO:
	case AMBFB_COLOR_RGB565:
		def_osd_setup.src			= OSD_SRC_DIRECT_IN_16;
		def_osd_setup.osd_direct_mode		= 0;
		def_osd_setup.osd_transparent_color	= 0x0000;
		def_osd_setup.osd_transparent_color_en	= 1;
		amba_vout_video_source_cmd(vout_id, AMBA_VIDEO_SOURCE_SET_OSD, &def_osd_setup);

		break;

	case AMBFB_COLOR_VYU565:
		def_osd_setup.src			= OSD_SRC_DIRECT_IN_16;
		def_osd_setup.osd_direct_mode		= 0;
		def_osd_setup.osd_transparent_color	= 0x0000;
		def_osd_setup.osd_transparent_color_en	= 1;
		amba_vout_video_source_cmd(vout_id, AMBA_VIDEO_SOURCE_SET_OSD, &def_osd_setup);

		break;

	default:
		break;
	}

amba_osd_on_vout_change_arch_exit:
	return ret;
}

int amba_osd_on_csc_change_arch(int vout_id, int csc_en)
{
	int					errorCode = 0;
	struct amba_video_sink_mode		sink_mode;
	ambosd_sink_cfg_t			*psink_cfg;
	struct amba_video_source_csc_info	csc_info;

	psink_cfg = &ambosd_sink_cfg[vout_id];
	if (!psink_cfg->mtx_init) {
		mutex_init(&psink_cfg->mtx);
		psink_cfg->mtx_init = 1;
	}

	mutex_lock(&psink_cfg->mtx);
	if (!psink_cfg->data_valid) {
		vout_warn("%s: Please init vout first!\n", __func__);
		errorCode = -EACCES;
		mutex_unlock(&psink_cfg->mtx);
		goto amba_osd_on_csc_change_arch_exit;
	} else {
		memcpy(&sink_mode, &psink_cfg->sink_mode,
			sizeof(struct amba_video_sink_mode));

		csc_info.path = sink_mode.sink_type;
		switch (sink_mode.sink_type) {
		case AMBA_VOUT_SINK_TYPE_DIGITAL:
			csc_info.path = AMBA_VIDEO_SOURCE_CSC_DIGITAL;
			if (csc_en)
				csc_info.mode = AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB;
			else
				csc_info.mode = AMBA_VIDEO_SOURCE_CSC_RGB2RGB;
			csc_info.clamp =
				AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL;
			break;

		case AMBA_VOUT_SINK_TYPE_CVBS:
			csc_info.path = AMBA_VIDEO_SOURCE_CSC_ANALOG;
			if (csc_en)
				csc_info.mode = AMBA_VIDEO_SOURCE_CSC_ANALOG_SD;
			else
				csc_info.mode = AMBA_VIDEO_SOURCE_CSC_RGB2RGB;
			csc_info.clamp = AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_SD;
			break;

		case AMBA_VOUT_SINK_TYPE_HDMI:
			csc_info.path = AMBA_VIDEO_SOURCE_CSC_HDMI;
			if (csc_en)
				csc_info.mode = AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB;
			else
				csc_info.mode = AMBA_VIDEO_SOURCE_CSC_RGB2RGB;
			csc_info.clamp =
				AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL;
			break;

		default:
			break;
		}
		errorCode = amba_vout_video_source_cmd(vout_id,
			AMBA_VIDEO_SOURCE_SET_CSC_DYNAMICALLY, &csc_info);
		if (errorCode)
			goto amba_osd_on_csc_change_arch_exit;

		sink_mode.csc_en = csc_en;

	}
	mutex_unlock(&psink_cfg->mtx);

	errorCode = amba_osd_on_vout_change(vout_id, &sink_mode);

amba_osd_on_csc_change_arch_exit:
	return errorCode;
}
