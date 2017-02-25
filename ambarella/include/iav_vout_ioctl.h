
/*
 * iav_vout_ioctl.h
 *
 * History:
 *	2008/04/02 - [Oliver Li] created file
 *	2011/06/10 - [Jian Tang] modified file
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

#ifndef __IAV_VOUT_IOCTL_H__
#define __IAV_VOUT_IOCTL_H__

#include <linux/ioctl.h>

#define AMBA_VOUT_NAME_LENGTH		(32)

#define AMBA_VOUT_SOURCE_TYPE_DVE	(1 << 0)
#define AMBA_VOUT_SOURCE_TYPE_DIGITAL	(1 << 1)
#define AMBA_VOUT_SOURCE_TYPE_HDMI	(1 << 2)
#define AMBA_VOUT_SOURCE_TYPE_MIPI	(1 << 3)

#define AMBA_VOUT_SOURCE_STARTING_ID	(0)
#define AMBA_VOUT_SINK_STARTING_ID	(0)

enum amba_vout_display_device_type {
	AMBA_VOUT_DISPLAY_DEVICE_LCD		= 0,
	AMBA_VOUT_DISPLAY_DEVICE_HDMI,
};

enum amba_vout_output_port_id {
        AMBA_VOUT_OUTPUT_DVE = AMBA_VOUT_SOURCE_TYPE_DVE,
        AMBA_VOUT_OUTPUT_DIGITAL = AMBA_VOUT_SOURCE_TYPE_DIGITAL,
        AMBA_VOUT_OUTPUT_HDMI = AMBA_VOUT_SOURCE_TYPE_HDMI,
        AMBA_VOUT_OUTPUT_MIPI = AMBA_VOUT_SOURCE_TYPE_MIPI,
};

enum amba_vout_sink_type {
	AMBA_VOUT_SINK_TYPE_AUTO = 0,
	AMBA_VOUT_SINK_TYPE_CVBS = ((0 << 16) | AMBA_VOUT_SOURCE_TYPE_DVE),
	AMBA_VOUT_SINK_TYPE_SVIDEO = ((1 << 16) | AMBA_VOUT_SOURCE_TYPE_DVE),
	AMBA_VOUT_SINK_TYPE_YPBPR = ((2 << 16) | AMBA_VOUT_SOURCE_TYPE_DVE),
	AMBA_VOUT_SINK_TYPE_HDMI = ((0 << 16) | AMBA_VOUT_SOURCE_TYPE_HDMI),
	AMBA_VOUT_SINK_TYPE_DIGITAL = ((0 << 16) | AMBA_VOUT_SOURCE_TYPE_DIGITAL),
	AMBA_VOUT_SINK_TYPE_MIPI = ((0 << 16) | AMBA_VOUT_SOURCE_TYPE_MIPI),
};

enum amba_vout_display_input {
	AMBA_VOUT_INPUT_FROM_MIXER = 0,
	AMBA_VOUT_INPUT_FROM_SMEM,
};

enum amba_vout_flip_info {
	AMBA_VOUT_FLIP_NORMAL = 0,
	AMBA_VOUT_FLIP_HV,
	AMBA_VOUT_FLIP_HORIZONTAL,
	AMBA_VOUT_FLIP_VERTICAL,
};

enum amba_vout_rotate_info {
	AMBA_VOUT_ROTATE_NORMAL,
	AMBA_VOUT_ROTATE_90,
};

enum amba_vout_tailored_info {
	AMBA_VOUT_OSD_NO_CSC	= 0x01,			//No Software CSC
	AMBA_VOUT_OSD_AUTO_COPY	= 0x02,			//Auto copy to other fb
};

enum amba_vout_mixer_csc {
       AMBA_VOUT_MIXER_DISABLE            = 0x00, //No Mixer CSC
       AMBA_VOUT_MIXER_ENABLE_FOR_VIDEO	= 0x01, //Mixer CSC for video
       AMBA_VOUT_MIXER_ENABLE_FOR_OSD     = 0x02, //Mixer CSC for OSD
};

enum amba_vout_hdmi_color_space {
	AMBA_VOUT_HDMI_CS_AUTO = 0,
	AMBA_VOUT_HDMI_CS_RGB,
	AMBA_VOUT_HDMI_CS_YCBCR_444,
	AMBA_VOUT_HDMI_CS_YCBCR_422,
};

enum amba_vout_hdmi_overscan {
	AMBA_VOUT_HDMI_OVERSCAN_AUTO = 0,
	AMBA_VOUT_HDMI_NON_FORCE_OVERSCAN,
	AMBA_VOUT_HDMI_FORCE_OVERSCAN,
};

enum amba_vout_sink_state {
	AMBA_VOUT_SINK_STATE_IDLE = 0,
	AMBA_VOUT_SINK_STATE_RUNNING,
	AMBA_VOUT_SINK_STATE_SUSPENDED,
};

enum amba_vout_lcd_mode_info {
	AMBA_VOUT_LCD_MODE_DISABLE = 0,
	AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT,
	AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT,
	AMBA_VOUT_LCD_MODE_RGB565,
	AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT,
	AMBA_VOUT_LCD_MODE_RGB888,
};

enum amba_vout_lcd_seq_info {
	AMBA_VOUT_LCD_SEQ_R0_G1_B2 = 0,
	AMBA_VOUT_LCD_SEQ_R0_B1_G2,
	AMBA_VOUT_LCD_SEQ_G0_R1_B2,
	AMBA_VOUT_LCD_SEQ_G0_B1_R2,
	AMBA_VOUT_LCD_SEQ_B0_R1_G2,
	AMBA_VOUT_LCD_SEQ_B0_G1_R2,
};

enum amba_vout_lcd_clk_edge_info {
	AMBA_VOUT_LCD_CLK_RISING_EDGE	= 0,
	AMBA_VOUT_LCD_CLK_FALLING_EDGE,
};

enum amba_vout_lcd_hvld_pol_info {
	AMBA_VOUT_LCD_HVLD_POL_LOW	= 0,
	AMBA_VOUT_LCD_HVLD_POL_HIGH,
};

enum amba_vout_lcd_model {
	AMBA_VOUT_LCD_MODEL_DIGITAL	= 0,
	AMBA_VOUT_LCD_MODEL_AUO27,
	AMBA_VOUT_LCD_MODEL_P28K,
	AMBA_VOUT_LCD_MODEL_TPO489,
	AMBA_VOUT_LCD_MODEL_TPO648,
	AMBA_VOUT_LCD_MODEL_TD043,
	AMBA_VOUT_LCD_MODEL_WDF2440,
	AMBA_VOUT_LCD_MODEL_1P3831,
	AMBA_VOUT_LCD_MODEL_1P3828,
	AMBA_VOUT_LCD_MODEL_EJ080NA,
	AMBA_VOUT_LCD_MODEL_AT070TNA2,
	AMBA_VOUT_LCD_MODEL_E330QHD,
	AMBA_VOUT_LCD_MODEL_PPGA3,
};

enum amba_vout_sink_plug {
	AMBA_VOUT_SINK_PLUGGED = 0,
	AMBA_VOUT_SINK_REMOVED,
};

enum ddd_structure {
	DDD_FRAME_PACKING		= 0,
	DDD_FIELD_ALTERNATIVE		= 1,
	DDD_LINE_ALTERNATIVE		= 2,
	DDD_SIDE_BY_SIDE_FULL		= 3,
	DDD_L_DEPTH			= 4,
	DDD_L_DEPTH_GRAPHICS_DEPTH	= 5,
	DDD_TOP_AND_BOTTOM		= 6,
	DDD_RESERVED			= 7,

	DDD_SIDE_BY_SIDE_HALF		= 8,

	DDD_UNSUPPORTED			= 16,
};

struct amba_vout_bg_color_info {
	u8 y;
	u8 cb;
	u8 cr;
};

struct amba_vout_video_size {
	u32 specified;
	u16 vout_width;		//Vout width
	u16 vout_height;	//Vout height
	u16 video_width;	//Video width
	u16 video_height;	//Video height
};

struct amba_vout_video_offset {
	u32 specified;
	s16 offset_x;
	s16 offset_y;
};

struct amba_vout_osd_size {
	u16 width;
	u16 height;
};

struct amba_vout_osd_rescale {
	u32 enable;
	u16 width;
	u16 height;
};

struct amba_vout_osd_offset {
	u32 specified;
	s16 offset_x;
	s16 offset_y;
};

struct amba_vout_lcd_info {
	enum amba_vout_lcd_mode_info		mode;
	enum amba_vout_lcd_seq_info		seqt;
	enum amba_vout_lcd_seq_info		seqb;
	enum amba_vout_lcd_clk_edge_info	dclk_edge;
	enum amba_vout_lcd_hvld_pol_info	hvld_pol;
	u32					dclk_freq_hz;	/* PLL_CLK_XXX */
	enum amba_vout_lcd_model		model;
};

typedef enum {
	COLORIMETRY_NO_DATA	= 0,
	COLORIMETRY_ITU601	= 1,
	COLORIMETRY_ITU709	= 2,
	COLORIMETRY_EXTENDED	= 3,
} colorimetry_t;

typedef struct {
	enum amba_video_mode			vmode;
	char					name[32];

	u32					pixel_clock;		/* kHz */

	u16					hsync_offset;		/* pixels */
	u16					hsync_width;		/* pixels */
	u16					h_blanking;		/* pixels */
	u16					h_active;		/* pixels */

	u16					vsync_offset;		/* lines */
	u16					vsync_width;		/* lines */
	u16					v_blanking;		/* lines */
	u16					v_active;		/* lines */

	u8					interlace;		/* 1: Interlace; 0: Progressive */
	u8					hsync_polarity;		/* 1: Positive; 0: Negative */
	u8					vsync_polarity;		/* 1: Positive; 0: Negative */

	u8					pixel_repetition;	/* repetition times */
	u32					aspect_ratio;		/* 16:9, 4:3, ... */
	colorimetry_t				colorimetry;		/* ITU601, ITU709 */
} amba_hdmi_video_timing_t;


struct amba_video_sink_mode {
	/* Sink */
	int				id;		//Sink ID
	u32				mode;		//enum amba_video_mode
	u32				ratio;		//AMBA_VIDEO_RATIO
	u32				bits;		//AMBA_VIDEO_BITS
	u32				type;		//AMBA_VIDEO_TYPE
	u32				format;		//AMBA_VIDEO_FORMAT
	u32				frame_rate;	//AMBA_VIDEO_FPS
	int				csc_en;		//enable csc or not
	struct amba_vout_bg_color_info	bg_color;
	enum amba_vout_display_input	display_input;	// input from SMEM or Mixer
	enum amba_vout_sink_type	sink_type;
       enum amba_vout_mixer_csc	mixer_csc;

	/* Video */
	int				video_en;	//enable video or not
	struct amba_vout_video_size	video_size;	//video size
	struct amba_vout_video_offset	video_offset;	//video offset
	enum amba_vout_flip_info	video_flip;	//flip
	enum amba_vout_rotate_info	video_rotate;	//rotate

	/* OSD */
	int				fb_id;		//frame buffer id
	struct amba_vout_osd_size	osd_size;	//OSD size
	struct amba_vout_osd_rescale	osd_rescale;	//OSD rescale
	struct amba_vout_osd_offset	osd_offset;	//OSD offset
	enum amba_vout_flip_info	osd_flip;	//flip
	enum amba_vout_rotate_info	osd_rotate;	//rotate
	enum amba_vout_tailored_info	osd_tailor;	//no csc, auto copy

	/* Misc */
	u32				direct_to_dsp;	//bypass iav
	struct amba_vout_lcd_info	lcd_cfg;	//LCD only
	enum amba_vout_hdmi_color_space hdmi_color_space;//HDMI only
	enum ddd_structure		hdmi_3d_structure;//HDMI only
	enum amba_vout_hdmi_overscan	hdmi_overscan;	//HDMI only
	const amba_hdmi_video_timing_t  *hdmi_displayer_timing; //HDMI only
};

struct amba_vout_sink_info {
	int					id;		//Sink ID
	int					source_id;
	char					name[AMBA_VOUT_NAME_LENGTH];
	enum amba_vout_sink_type		sink_type;
	struct amba_video_sink_mode		sink_mode;
	enum amba_vout_sink_state		state;
	enum amba_vout_sink_plug		hdmi_plug;
	enum amba_video_mode			hdmi_modes[32];
	enum amba_video_mode			hdmi_native_mode;
	u16					hdmi_native_width;
	u16					hdmi_native_height;
	enum amba_vout_hdmi_overscan		hdmi_overscan;

};

typedef struct iav_vout_fb_sel_s {
	int		vout_id;
	int		fb_id;
} iav_vout_fb_sel_t;

typedef struct iav_vout_enable_video_s {
	int		vout_id;
	int		video_en;
} iav_vout_enable_video_t;

typedef struct iav_vout_flip_video_s {
	int		vout_id;
	int		flip;
} iav_vout_flip_video_t;

typedef struct iav_vout_rotate_video_s {
	int		vout_id;
	int		rotate;
} iav_vout_rotate_video_t;

typedef struct iav_vout_enable_csc_s {
	int		vout_id;
	int		csc_en;
} iav_vout_enable_csc_t;

typedef struct iav_vout_change_video_size_s {
	int		vout_id;
	int		width;
	int		height;
} iav_vout_change_video_size_t;

typedef struct iav_vout_change_video_offset_s {
	int		vout_id;
	int		specified;
	int		offset_x;
	int		offset_y;
} iav_vout_change_video_offset_t;

typedef struct iav_vout_flip_osd_s {
	int		vout_id;
	int		flip;
} iav_vout_flip_osd_t;

typedef struct iav_vout_enable_osd_rescaler_s {
	int		vout_id;
	int		enable;
	int		width;
	int		height;
} iav_vout_enable_osd_rescaler_t;

typedef struct iav_vout_change_osd_offset_s {
	int		vout_id;
	int		specified;
	int		offset_x;
	int		offset_y;
} iav_vout_change_osd_offset_t;


/*
 * general APIs	- VOUTIOC_MAGIC
 */
enum {
	// For VOUT, from 0x20 to 0x3F
	IOC_VOUT_HALT = 0x20,
	IOC_VOUT_SELECT_DEV = 0x21,
	IOC_VOUT_CONFIGURE_SINK = 0x22,
	IOC_VOUT_SELECT_FB = 0x23,
	IOC_VOUT_ENABLE_VIDEO = 0x24,
	IOC_VOUT_FLIP_VIDEO = 0x25,
	IOC_VOUT_ROTATE_VIDEO = 0x26,
	IOC_VOUT_ENABLE_CSC = 0x27,
	IOC_VOUT_CHANGE_VIDEO_SIZE = 0x28,
	IOC_VOUT_CHANGE_VIDEO_OFFSET = 0x29,
	IOC_VOUT_FLIP_OSD = 0x2A,
	IOC_VOUT_ENABLE_OSD_RESCALER = 0x2B,
	IOC_VOUT_CHANGE_OSD_OFFSET = 0x2C,
	IOC_VOUT_GET_SINK_NUM = 0x2D,
	IOC_VOUT_GET_SINK_INFO = 0x2E,
};


#define VOUTIOC_MAGIC			'v'

#define _IAV_IO(IOCTL)			_IO(VOUTIOC_MAGIC, IOCTL)
#define _IAV_IOR(IOCTL, param)		_IOR(VOUTIOC_MAGIC, IOCTL, param)
#define _IAV_IOW(IOCTL, param)		_IOW(VOUTIOC_MAGIC, IOCTL, param)
#define _IAV_IOWR(IOCTL, param)		_IOWR(VOUTIOC_MAGIC, IOCTL, param)

#define IAV_IOC_VOUT_HALT			_IAV_IOW(IOC_VOUT_HALT, int)
#define IAV_IOC_VOUT_SELECT_DEV		_IAV_IOW(IOC_VOUT_SELECT_DEV, int)
#define IAV_IOC_VOUT_CONFIGURE_SINK	_IAV_IOW(IOC_VOUT_CONFIGURE_SINK, struct amba_video_sink_mode *)
#define IAV_IOC_VOUT_SELECT_FB		_IAV_IOW(IOC_VOUT_SELECT_FB, struct iav_vout_fb_sel_s *)
#define IAV_IOC_VOUT_ENABLE_VIDEO	_IAV_IOW(IOC_VOUT_ENABLE_VIDEO, struct iav_vout_enable_video_s *)
#define IAV_IOC_VOUT_FLIP_VIDEO		_IAV_IOW(IOC_VOUT_FLIP_VIDEO, struct iav_vout_flip_video_s *)
#define IAV_IOC_VOUT_ROTATE_VIDEO	_IAV_IOW(IOC_VOUT_ROTATE_VIDEO, struct iav_vout_rotate_video_s *)
#define IAV_IOC_VOUT_ENABLE_CSC		_IAV_IOW(IOC_VOUT_ENABLE_CSC, struct iav_vout_enable_csc_s *)
#define IAV_IOC_VOUT_CHANGE_VIDEO_SIZE		_IAV_IOW(IOC_VOUT_CHANGE_VIDEO_SIZE, struct iav_vout_change_video_size_s *)
#define IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET	_IAV_IOW(IOC_VOUT_CHANGE_VIDEO_OFFSET, struct iav_vout_change_video_offset_s *)
#define IAV_IOC_VOUT_FLIP_OSD		_IAV_IOW(IOC_VOUT_FLIP_OSD, struct iav_vout_flip_osd_s *)
#define IAV_IOC_VOUT_ENABLE_OSD_RESCALER	_IAV_IOW(IOC_VOUT_ENABLE_OSD_RESCALER, struct iav_vout_enable_osd_rescaler_s *)
#define IAV_IOC_VOUT_CHANGE_OSD_OFFSET		_IAV_IOW(IOC_VOUT_CHANGE_OSD_OFFSET, struct iav_vout_change_osd_offset_s *)
#define IAV_IOC_VOUT_GET_SINK_NUM	_IAV_IOR(IOC_VOUT_GET_SINK_NUM, int *)
#define IAV_IOC_VOUT_GET_SINK_INFO	_IAV_IOR(IOC_VOUT_GET_SINK_INFO, struct amba_vout_sink_info *)

#endif // __IAV_IOCTL_ARCH_H__


