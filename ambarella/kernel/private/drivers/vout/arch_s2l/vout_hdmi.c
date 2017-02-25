/*
 * kernel/private/drivers/ambarella/vout/arch_s2l/vout_arch.c
 *
 * History:
 *    2009/07/23 - [Zhenwu Xue] Create
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

#include <linux/i2c.h>
#include <linux/clk.h>
#include <plat/clk.h>
#include <iav_ioctl.h>
#include "../vout_pri.h"
#include "vout_arch.h"

typedef struct {
        enum amba_video_mode    vmode;
        u8                      system;
        u32                     fps;
        enum PLL_CLK_HZ         clock;
        u8                      vid_format;
        u16                     h_active;
        u16                     v_active;
        u8                      interlace;
} vout_hdmi_mode_list;

const static vout_hdmi_mode_list Vout_Hdmi_mode[] = {
        {AMBA_VIDEO_MODE_VGA,       	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_60,    PLL_CLK_27MHZ,        	VD_NON_FIXED,   640,    480,    0},
        {AMBA_VIDEO_MODE_480I,      	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_59_94, PLL_CLK_27MHZ,        	VD_480I60,      1440,   240,    1},
        {AMBA_VIDEO_MODE_576I,      	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_50,    PLL_CLK_27MHZ,        	VD_576I50,      1440,   288,    1},
        {AMBA_VIDEO_MODE_D1_NTSC,   	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_59_94, PLL_CLK_27MHZ,        	VD_480P60,      720,    480,    0},
        {AMBA_VIDEO_MODE_D1_PAL,    	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_50,    PLL_CLK_27MHZ,        	VD_576P50,      720,    576,    0},
        {AMBA_VIDEO_MODE_720P,      	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_59_94, PLL_CLK_74_25D1001MHZ,	VD_720P60,      1280,   720,    0},
        {AMBA_VIDEO_MODE_720P50,  	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_50,    PLL_CLK_74_25MHZ,     	VD_720P50,      1280,   720,    0},
        {AMBA_VIDEO_MODE_1080I,     	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_59_94, PLL_CLK_74_25D1001MHZ,	VD_1080I60,     1920,   540,    1},
        {AMBA_VIDEO_MODE_1080I50, 	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_50,    PLL_CLK_74_25MHZ,     	VD_1080I50,     1920,   540,    1},
        {AMBA_VIDEO_MODE_1080P24,   	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_24,    PLL_CLK_74_25MHZ,     	VD_1080P24,     1920,   1080,   0},
        {AMBA_VIDEO_MODE_1080P25,   	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_25,    PLL_CLK_74_25MHZ,     	VD_1080P25,     1920,   1080,   0},
        {AMBA_VIDEO_MODE_1080P30,   	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_30,    PLL_CLK_74_25D1001MHZ,	VD_1080P30,     1920,   1080,   0},
        {AMBA_VIDEO_MODE_1080P,     	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_59_94, PLL_CLK_148_5D1001MHZ,	VD_1080P60,     1920,   1080,   0},
        {AMBA_VIDEO_MODE_1080P50, 	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_50,    PLL_CLK_148_5MHZ,     	VD_1080P50,     1920,   1080,   0},
        {AMBA_VIDEO_MODE_HDMI_NATIVE,	AMBA_VIDEO_SYSTEM_AUTO, AMBA_VIDEO_FPS_AUTO,  PLL_CLK_27MHZ,        	VD_NON_FIXED,   0,      0,      0},
        {AMBA_VIDEO_MODE_720P24,      	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_24,    PLL_CLK_59_4MHZ,          VD_NON_FIXED,   1280,   720,    0},
        {AMBA_VIDEO_MODE_720P25,      	AMBA_VIDEO_SYSTEM_NTSC, AMBA_VIDEO_FPS_25,    PLL_CLK_74_25MHZ,     	VD_NON_FIXED,   1280,   720,    0},
        {AMBA_VIDEO_MODE_720P30,      	AMBA_VIDEO_SYSTEM_PAL,  AMBA_VIDEO_FPS_30,    PLL_CLK_74_25D1001MHZ,	VD_NON_FIXED,   1280,   720,    0},
};

#define	VHMODE_NUM	ARRAY_SIZE(Vout_Hdmi_mode)

/* ========================================================================== */
void amba_s2l_vout_hdmi_get_native_mode(amba_hdmi_video_timing_t *pvtiming)
{
        u32     i, fps, tmp[4];

        tmp[0]  =  1000 * pvtiming->pixel_clock;
        tmp[1]  =  (pvtiming->h_active + pvtiming->h_blanking);
        tmp[1]  *= (pvtiming->v_active + pvtiming->v_blanking);
        fps     =  tmp[0] / tmp[1];

        for (i = 0; i < ARRAY_SIZE(Vout_Hdmi_mode); i++) {
                if (pvtiming->h_active != Vout_Hdmi_mode[i].h_active)
                        continue;

                if (pvtiming->v_active != Vout_Hdmi_mode[i].v_active)
                        continue;

                if (fps != Vout_Hdmi_mode[i].fps)
                        continue;

                if (pvtiming->interlace != Vout_Hdmi_mode[i].interlace)
                        continue;

                break;
        }

        if (i < ARRAY_SIZE(Vout_Hdmi_mode)) {
                pvtiming->vmode = Vout_Hdmi_mode[i].vmode;
        } else {
                pvtiming->vmode = AMBA_VIDEO_MODE_MAX;
        }
}

int amba_s2l_vout_hdmi_init(struct __amba_vout_video_source *psrc,
                            struct amba_video_sink_mode *sink_mode)
{
        int                                     rval, mode_index = 0;
        struct amba_video_source_clock_setup	clk_setup;
        const amba_hdmi_video_timing_t		*hvt = NULL;
        struct amba_vout_hv_size_info		sink_hv;
        struct amba_vout_window_info		sink_window;
        vd_config_t                             sink_cfg;
        struct amba_vout_hv_sync_info		sink_sync;
        struct amba_video_info			sink_video_info;
        struct amba_video_source_csc_info	sink_csc;
        vout_video_setup_t		        video_setup;

        psrc->active_sink_type = AMBA_VOUT_SINK_TYPE_HDMI;
        //check mode
        for(mode_index = 0; mode_index < VHMODE_NUM; mode_index++)
                if(Vout_Hdmi_mode[mode_index].vmode == sink_mode->mode)
                        break;
        if(mode_index >= VHMODE_NUM) {
                vout_err("vout_hdmi don't support mode %d \n", sink_mode->mode);
                return -EINVAL;
        }

        //set vout clock
        hvt = sink_mode->hdmi_displayer_timing;
        if(sink_mode->mode == AMBA_VIDEO_MODE_HDMI_NATIVE) {
                if(sink_mode->hdmi_3d_structure == DDD_FRAME_PACKING ||
                    sink_mode->hdmi_3d_structure == DDD_SIDE_BY_SIDE_FULL)
                        clk_setup.freq_hz =
                                hvt->pixel_clock * 1000 * 2;
                else {
                        clk_setup.freq_hz =
                                hvt->pixel_clock * 1000;
                }
        } else {
                if (sink_mode->hdmi_3d_structure == DDD_FRAME_PACKING ||
                    sink_mode->hdmi_3d_structure == DDD_SIDE_BY_SIDE_FULL) {
                        clk_setup.freq_hz = Vout_Hdmi_mode[mode_index].clock * 2;
                } else {
                        clk_setup.freq_hz = Vout_Hdmi_mode[mode_index].clock;
                }

        }
        clk_setup.src = VO_CLK_ONCHIP_PLL_27MHZ;
        psrc->freq_hz = clk_setup.freq_hz;
        amba_s2l_vout_set_clock_setup(psrc, &clk_setup);

        //set hv
        sink_hv.hsize = hvt->h_blanking + hvt->h_active;
        sink_hv.vtsize = hvt->v_blanking + hvt->v_active;
        if(hvt->interlace)
                sink_hv.vbsize = sink_hv.vtsize + 1;
        else
                sink_hv.vbsize = sink_hv.vtsize;
        rval = amba_s2l_vout_set_hv(psrc, &sink_hv);
        if(rval)
                return rval;

        //set hvsync
        sink_sync.hsync_start = 0;
        sink_sync.hsync_end = hvt->hsync_width;
        sink_sync.vtsync_start = 0;
        sink_sync.vtsync_end = hvt->vsync_width * sink_hv.hsize;
        sink_sync.vbsync_start = sink_sync.vtsync_start + (sink_hv.hsize >> 1);
        sink_sync.vbsync_end = sink_sync.vtsync_end + (sink_hv.hsize >> 1);
        sink_sync.vtsync_start_row = 0;
        sink_sync.vtsync_start_col = 0;
        sink_sync.vtsync_end_row = hvt->vsync_width;
        sink_sync.vtsync_end_col = 0;
        sink_sync.vbsync_start_row = 0;
        sink_sync.vbsync_start_col = sink_hv.hsize >> 1;
        sink_sync.vbsync_end_row = hvt->vsync_width;
        sink_sync.vbsync_end_col = sink_hv.hsize >> 1;
        sink_sync.sink_type = sink_mode->sink_type;
        rval = amba_s2l_vout_set_hvsync(psrc, &sink_sync);
        if(rval)
                return rval;

        //set active window
        sink_window.start_x = hvt->h_blanking - hvt->hsync_offset;
        sink_window.end_x = sink_window.start_x + hvt->h_active - 1;
        sink_window.start_y = hvt->v_blanking - hvt->vsync_offset;
        sink_window.end_y = sink_window.start_y + hvt->v_active - 1;
	if (sink_mode->mode == AMBA_VIDEO_MODE_480I ||
		sink_mode->mode == AMBA_VIDEO_MODE_576I)
                sink_window.width =
                        (sink_window.end_x - sink_window.start_x + 1) >> 1;
        else
                sink_window.width =
                        sink_window.end_x - sink_window.start_x + 1;
        sink_window.field_reverse = 0;
        rval = amba_s2l_vout_set_active_win(psrc, &sink_window);
        if(rval)
                return rval;

        //set video size
        sink_window.start_x = sink_mode->video_offset.offset_x;
        sink_window.end_x =
                sink_window.start_x + sink_mode->video_size.video_width - 1;
        sink_window.start_y = sink_mode->video_offset.offset_y;
        sink_window.end_y =
                sink_window.start_y + sink_mode->video_size.video_height - 1;
	if (sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE) {
		sink_window.start_y >>= 1;
		sink_window.end_y >>= 1;
	}
        rval = amba_s2l_vout_set_video_size(psrc, &sink_window);
        if(rval)
                return rval;

        //set video info
        sink_video_info.width = sink_mode->video_size.video_width;
        sink_video_info.height = sink_mode->video_size.video_height;
        sink_video_info.system = Vout_Hdmi_mode[mode_index].system;
        sink_video_info.fps = Vout_Hdmi_mode[mode_index].fps;
        sink_video_info.format = sink_mode->format;
        sink_video_info.type = sink_mode->type;
        sink_video_info.bits = sink_mode->bits;
        sink_video_info.ratio = sink_mode->ratio;
        sink_video_info.flip = sink_mode->video_flip;
        sink_video_info.rotate = sink_mode->video_rotate;
        rval = amba_s2l_vout_set_video_info(psrc, &sink_video_info);
        if(rval)
                return rval;

        //set csc
        if (sink_mode->csc_en) {
                switch (sink_mode->hdmi_color_space) {
                case AMBA_VOUT_HDMI_CS_RGB:
                        sink_csc.path = AMBA_VIDEO_SOURCE_CSC_HDMI;
                        sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB;
                        sink_csc.clamp =
                                AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL;
                        break;

                case AMBA_VOUT_HDMI_CS_YCBCR_444:
                        sink_csc.path = AMBA_VIDEO_SOURCE_CSC_HDMI;
                        sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVHD;
                        sink_csc.clamp =
                                AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_CLAMP;
                        break;

                case AMBA_VOUT_HDMI_CS_YCBCR_422:
                        sink_csc.path = AMBA_VIDEO_SOURCE_CSC_HDMI;
                        sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVHD;
                        sink_csc.clamp =
                                AMBA_VIDEO_SOURCE_CSC_DATARANGE_HDMI_YCBCR422_CLAMP;
                        break;

                default:
                        sink_csc.path = AMBA_VIDEO_SOURCE_CSC_HDMI;
                        sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB;
                        sink_csc.clamp =
                                AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL;
                        break;
                }
        } else {
                switch (sink_mode->hdmi_color_space) {
                case AMBA_VOUT_HDMI_CS_RGB:
                        sink_csc.path = AMBA_VIDEO_SOURCE_CSC_HDMI;
                        sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_RGB2RGB;
                        sink_csc.clamp =
                                AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL;
                        break;
                case AMBA_VOUT_HDMI_CS_YCBCR_444:
                        sink_csc.path = AMBA_VIDEO_SOURCE_CSC_HDMI;
                        sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_RGB2YUV;
                        sink_csc.clamp =
                                AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_CLAMP;
                        break;
                case AMBA_VOUT_HDMI_CS_YCBCR_422:
                        sink_csc.path = AMBA_VIDEO_SOURCE_CSC_HDMI;
                        sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_RGB2YUV_12BITS;
                        sink_csc.clamp =
                                AMBA_VIDEO_SOURCE_CSC_DATARANGE_HDMI_YCBCR422_CLAMP;
                        break;
                default:
                        sink_csc.path = AMBA_VIDEO_SOURCE_CSC_HDMI;
                        sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_RGB2RGB;
                        sink_csc.clamp =
                                AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL;
                        break;
                }
        }
        rval = amba_s2l_vout_set_csc(psrc, &sink_csc);
        if(rval)
                return rval;

        //set vout hdmi config
        rval = amba_s2l_vout_get_config(psrc, &sink_cfg);
        if(rval)
                return rval;
        sink_cfg.d_control.s.analog_out = 0;
        sink_cfg.d_control.s.digital_out = 0;
        sink_cfg.d_control.s.hdmi_out = 1;
        sink_cfg.d_control.s.interlace = hvt->interlace;
        sink_cfg.d_control.s.vid_format = Vout_Hdmi_mode[mode_index].vid_format;
        sink_cfg.d_hdmi_output_mode.s.hspol = hvt->hsync_polarity;
        sink_cfg.d_hdmi_output_mode.s.vspol = hvt->vsync_polarity;
        switch (sink_mode->hdmi_color_space) {
        case AMBA_VOUT_HDMI_CS_RGB:
                sink_cfg.d_hdmi_output_mode.s.mode = HDMI_MODE_RGB_444;
                break;
        case AMBA_VOUT_HDMI_CS_YCBCR_444:
                sink_cfg.d_hdmi_output_mode.s.mode = HDMI_MODE_YCBCR_444;
                break;
        case AMBA_VOUT_HDMI_CS_YCBCR_422:
                sink_cfg.d_hdmi_output_mode.s.mode = HDMI_MODE_YC_422;
                break;
        default:
                break;
        }
        rval = amba_s2l_vout_set_config(psrc, &sink_cfg);
        if(rval)
                return rval;

        //set vout video setup
        rval = amba_s2l_vout_get_setup(psrc, &video_setup);
        if(rval)
                return rval;
        video_setup.en = sink_mode->video_en;
        switch (sink_mode->video_flip) {
        case AMBA_VOUT_FLIP_NORMAL:
                video_setup.flip = 0;
                break;
        case AMBA_VOUT_FLIP_HV:
                video_setup.flip = 1;
                break;
        case AMBA_VOUT_FLIP_HORIZONTAL:
                video_setup.flip = 2;
                break;
        case AMBA_VOUT_FLIP_VERTICAL:
                video_setup.flip = 3;
                break;
        default:
                vout_err("%s can't support flip[%d]!\n",
                         __func__, sink_mode->video_flip);
                return -EINVAL;
        }
        switch (sink_mode->video_rotate) {
        case AMBA_VOUT_ROTATE_NORMAL:
                video_setup.rotate = 0;
                break;

        case AMBA_VOUT_ROTATE_90:
                video_setup.rotate = 1;
                break;

        default:
                vout_info("%s can't support rotate[%d]!\n",
                          __func__, sink_mode->video_rotate);
                return -EINVAL;
        }

        rval = amba_s2l_vout_set_setup(psrc, &video_setup);
        if(rval)
                return rval;

        return 0;
}

