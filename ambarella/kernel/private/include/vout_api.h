/*
 * kernel/private/include/vout_api.h
 *
 * History:
 *    2009/05/13 - [Anthony Ginger] Create
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


#ifndef __VOUT_API_H__
#define __VOUT_API_H__

#define AMBA_VOUT_CLUT_SIZE		(256)

enum amba_video_source_status {
	AMBA_VIDEO_SOURCE_STATUS_IDLE = 0,
	AMBA_VIDEO_SOURCE_STATUS_RUNNING = 1,
	AMBA_VIDEO_SOURCE_STATUS_SUSPENDED = 2,
};

struct amba_video_source_scale_analog_info {
	u16 y_coeff;
	u16 pb_coeff;
	u16 pr_coeff;
	u16 y_cost;
	u16 pb_cost;
	u16 pr_cost;
};

struct amba_video_source_display_info {
	u8 enable_video;
	u8 enable_osd0;
	u8 enable_osd1;
	u8 enable_cursor;
};

struct amba_video_source_info {
	u32 enabled;
	struct amba_video_info video_info;
};

struct amba_video_source_osd_info {
	u32 osd_id;
	u32 gblend;
	u16 width;
	u16 height;
	u16 offset_x;
	u16 offset_y;
	u16 zoom_x;
	u16 zoom_y;
};

struct amba_video_source_osd_clut_info {
	u8 *pclut_table;
	u8 *pblend_table;
};

struct amba_video_source_clock_setup {
	u32				src;
	u32				freq_hz;
};

struct amba_vout_window_info {
	u16 start_x;
	u16 start_y;
	u16 end_x;
	u16 end_y;
	u16 width;
	u16 field_reverse;
};

struct amba_vout_hv_sync_info {
	u16 hsync_start;
	u16 hsync_end;
	u16 vtsync_start;
	u16 vtsync_end;
	u16 vbsync_start;
	u16 vbsync_end;

	u16 vtsync_start_row;
	u16 vtsync_start_col;
	u16 vtsync_end_row;
	u16 vtsync_end_col;
	u16 vbsync_start_row;
	u16 vbsync_start_col;
	u16 vbsync_end_row;
	u16 vbsync_end_col;

	enum amba_vout_sink_type sink_type;
};

enum amba_vout_hvld_type{
	AMBA_VOUT_HVLD_POL_LOW	= 0,
	AMBA_VOUT_HVLD_POL_HIGH,
};

struct amba_vout_hvld_sync_info {
	enum amba_vout_hvld_type hvld_type;
};

struct amba_vout_hv_size_info {
	u16 hsize;
	u16 vtsize;	//vsize for progressive
	u16 vbsize;
};


enum amba_video_source_cmd {
	AMBA_VIDEO_SOURCE_IDLE = 30000,
	AMBA_VIDEO_SOURCE_UPDATE_IAV_INFO,

	AMBA_VIDEO_SOURCE_RESET = 30100,
	AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP,
	AMBA_VIDEO_SOURCE_RUN,
	AMBA_VIDEO_SOURCE_SUSPEND,
	AMBA_VIDEO_SOURCE_RESUME,
	AMBA_VIDEO_SOURCE_SUSPEND_TOSS,
	AMBA_VIDEO_SOURCE_RESUME_TOSS,
	AMBA_VIDEO_SOURCE_HALT,

	AMBA_VIDEO_SOURCE_REGISTER_SINK = 30200,
	AMBA_VIDEO_SOURCE_UNREGISTER_SINK,
	AMBA_VIDEO_SOURCE_REGISTER_IRQ_CALLBACK,
	AMBA_VIDEO_SOURCE_GET_SINK_NUM,
	AMBA_VIDEO_SOURCE_REGISTER_AR_NOTIFIER,
	AMBA_VIDEO_SOURCE_REPORT_SINK_EVENT,

	AMBA_VIDEO_SOURCE_GET_CONFIG = 31000,
	AMBA_VIDEO_SOURCE_GET_OSD,
	AMBA_VIDEO_SOURCE_GET_ACTIVE_WIN,
	AMBA_VIDEO_SOURCE_GET_VOUT_SETUP,
	AMBA_VIDEO_SOURCE_GET_DVE,
	AMBA_VIDEO_SOURCE_GET_VIDEO_AR,

	AMBA_VIDEO_SOURCE_SET_CONFIG = 32000,
	AMBA_VIDEO_SOURCE_SET_VIDEO_SIZE,
	AMBA_VIDEO_SOURCE_SET_OSD,
	AMBA_VIDEO_SOURCE_SET_OSD_CLUT,
	AMBA_VIDEO_SOURCE_SET_BG_COLOR,
	AMBA_VIDEO_SOURCE_SET_LCD,
	AMBA_VIDEO_SOURCE_SET_ACTIVE_WIN,
	AMBA_VIDEO_SOURCE_SET_HV,
	AMBA_VIDEO_SOURCE_SET_HVSYNC,
	AMBA_VIDEO_SOURCE_SET_HVLD,
	AMBA_VIDEO_SOURCE_SET_SCALE_SD_ANALOG_OUT,
	AMBA_VIDEO_SOURCE_SET_VBI,
	AMBA_VIDEO_SOURCE_SET_VIDEO_INFO,
	AMBA_VIDEO_SOURCE_SET_VOUT_SETUP,
	AMBA_VIDEO_SOURCE_SET_CSC,
	AMBA_VIDEO_SOURCE_SET_CSC_DYNAMICALLY,
	AMBA_VIDEO_SOURCE_SET_DVE,
	AMBA_VIDEO_SOURCE_SET_CLOCK_SETUP,
	AMBA_VIDEO_SOURCE_SET_OSD_BUFFER,
	AMBA_VIDEO_SOURCE_SET_DISPLAY_INPUT,
	AMBA_VIDEO_SOURCE_SET_VIDEO_AR,
	AMBA_VIDEO_SOURCE_SET_MIXER_CSC,

       AMBA_VIDEO_SOURCE_INIT_HDMI = 3300,
       AMBA_VIDEO_SOURCE_INIT_ANALOG,
       AMBA_VIDEO_SOURCE_INIT_DIGITAL,
       AMBA_VIDEO_SOURCE_HDMI_GET_NATIVE_MODE,
};
#define AMBA_VIDEO_SOURCE_FORCE_RESET		(1 << 0)
#define AMBA_VIDEO_SOURCE_UPDATE_MIXER_SETUP	(1 << 1)
#define AMBA_VIDEO_SOURCE_UPDATE_VIDEO_SETUP	(1 << 2)
#define AMBA_VIDEO_SOURCE_UPDATE_DISPLAY_SETUP	(1 << 3)
#define AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP	(1 << 4)
#define AMBA_VIDEO_SOURCE_UPDATE_CSC_SETUP	(1 << 5)

enum amba_video_sink_cmd {
	AMBA_VIDEO_SINK_IDLE = 40000,

	AMBA_VIDEO_SINK_RESET = 40100,
	AMBA_VIDEO_SINK_SUSPEND,
	AMBA_VIDEO_SINK_RESUME,
	AMBA_VIDEO_SINK_GET_SOURCE_ID,
	AMBA_VIDEO_SINK_GET_INFO,

	AMBA_VIDEO_SINK_GET_MODE = 41000,

	AMBA_VIDEO_SINK_SET_MODE = 42000,
};

enum amba_video_source_csc_path_info {
	AMBA_VIDEO_SOURCE_CSC_DIGITAL = 0,
	AMBA_VIDEO_SOURCE_CSC_ANALOG = 1,
	AMBA_VIDEO_SOURCE_CSC_HDMI = 2,
};

enum amba_video_source_csc_mode_info {
	AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVHD	= 0,	/* YUV601 -> YUV709 */
	AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD	= 1,	/* YUV601 -> YUV601 */
	AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB		= 2,	/* YUV601 -> RGB    */
	AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVSD	= 3,	/* YUV709 -> YUV601 */
	AMBA_VIDEO_SOURCE_CSC_YUVHD2YUVHD	= 4,	/* YUV709 -> YUV709 */
	AMBA_VIDEO_SOURCE_CSC_YUVHD2RGB		= 5,	/* YUV709 -> RGB    */
	AMBA_VIDEO_SOURCE_CSC_RGB2RGB		= 6,	/* RGB    -> RGB */
	AMBA_VIDEO_SOURCE_CSC_RGB2YUV		= 7,	/* RGB    -> YUV */
	AMBA_VIDEO_SOURCE_CSC_RGB2YUV_12BITS	= 8,	/* RGB    -> YUV_12bits */

	AMBA_VIDEO_SOURCE_CSC_ANALOG_SD		= 0,
	AMBA_VIDEO_SOURCE_CSC_ANALOG_HD		= 1,
};

enum amba_video_source_csc_clamp_info {
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_HD_FULL		= 0,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_SD_FULL		= 1,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_HD_FULL		= 2,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL		= 3,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_HD_CLAMP		= 4,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_ANALOG_SD_CLAMP		= 5,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_HD_CLAMP	= 6,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_CLAMP	= 7,
	AMBA_VIDEO_SOURCE_CSC_DATARANGE_HDMI_YCBCR422_CLAMP	= 8,

	AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_SD			= 0,
	AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_HD			= 1,
	AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_SD_NTSC		= 2,
	AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_SD_PAL		= 3,
};

struct amba_video_source_csc_info {
	enum amba_video_source_csc_path_info path;
	enum amba_video_source_csc_mode_info mode;
	enum amba_video_source_csc_clamp_info clamp;
};

#include "amba_arch_vout.h"

typedef void irq_callback_t(void);
typedef void (*vout_ar_notifier_t)(u8 video_src, u8 ar);

/* ========================================================================== */
int amba_vout_pm(u32 pmval);

int amba_vout_video_source_cmd(int id,
	enum amba_video_source_cmd cmd, void *args);
int amba_vout_video_sink_cmd(int id,
	enum amba_video_sink_cmd cmd, void *args);
extern int amba_osd_on_vout_change(int vout_id,	\
	struct amba_video_sink_mode *sink_mode);
extern int amba_osd_on_fb_switch(int vout_id, int fb_id);
extern int amba_osd_on_csc_change(int vout_id, int csc_en);
extern int amba_osd_on_rescaler_change(int vout_id, int enable,
	int width, int height);
extern int amba_osd_on_offset_change(int vout_id, int change,
	int x, int y);

#endif

