/*
 * vout_init.c
 *
 * History:
 *	2009/7/15 - [Zhenwu Xue] created file
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


/*==============================================================================
  Utilities to initialize external LCD modules
==============================================================================*/
#include "lcd/lcd_api.h"
#include "lcd/lcd_digital.c"
#include "lcd/lcd_digital601.c"
#include "lcd/lcd_td043.c"
#include "lcd/lcd_st7789v.c"
#include "lcd/lcd_ili8961.c"

typedef struct lcd_model {
	const char			*model;
	const LCD_SETMODE_FUNC		lcd_setmode;
	const LCD_POST_SETMODE_FUNC	lcd_post_setmode;
} lcd_model_t;

lcd_model_t lcd_model_list[] = {
	{"digital",	lcd_digital_setmode,	NULL				},
	{"digital601",	lcd_digital601_setmode,	NULL				},
	{"td043",	lcd_td043_setmode,	NULL				},
	{"td043_16",	lcd_td043_16_setmode,	NULL				},
	{"st7789v",	lcd_st7789v_16_setmode,	NULL				},
	{"ili8961",	lcd_ili8961_setmode,	NULL				},
};

int get_lcd_model_index(const char *lcd_model)
{
	int i;

	for (i = 0; i < sizeof(lcd_model_list) / sizeof(lcd_model_list[0]); i++)
		if (strcmp(lcd_model_list[i].model, lcd_model) == 0)
			return i;

	printf("LCD Model %s is unavailable, use \"digital\" instead!\n", lcd_model);
	return 0;
}

int vout_get_sink_id(int chan, int sink_type);


/*==============================================================================
  Vout options which will affect the whole vout(both Video and OSD)
==============================================================================*/
#define	NUM_VOUT	2
typedef enum {
	VOUT_0 = 0,
	VOUT_1 = 1,
	VOUT_TBD = 2,
} vout_id_t;
int current_vout = VOUT_TBD;		//For parsing command-line only
int vout_flag[NUM_VOUT + 1] = {0};	//whether vout specified
int vout_mode[NUM_VOUT + 1] = {0};	//480i, 576i, ...
int vout_lcd_model_index[NUM_VOUT + 1] = {0}; //0: digital; 1: auo27, ...
int vout_fps[NUM_VOUT + 1] = {AMBA_VIDEO_FPS_AUTO, AMBA_VIDEO_FPS_AUTO}; //vout framerate, typically used for LCD
int vout_csc_en[NUM_VOUT + 1] = {1, 1};	//enable/disable csc(color space conversion: yuv -> rgb)
int vout_device_selected_flag[NUM_VOUT + 1] = {0}; //if external device type(ypbpr, lcd, hdmi) specified
int vout_device_type[NUM_VOUT + 1] = {AMBA_VOUT_SINK_TYPE_AUTO, AMBA_VOUT_SINK_TYPE_AUTO}; //External device type
enum amba_vout_hdmi_color_space vout_hdmi_cs[NUM_VOUT + 1] = {AMBA_VOUT_HDMI_CS_AUTO, AMBA_VOUT_HDMI_CS_AUTO}; //HDMI Color Space
enum ddd_structure vout_hdmi_3d_structure[NUM_VOUT + 1] = {DDD_RESERVED, DDD_RESERVED, DDD_RESERVED}; //HDMI 3D Structure
enum amba_vout_hdmi_overscan vout_hdmi_overscan[NUM_VOUT + 1] = {AMBA_VOUT_HDMI_OVERSCAN_AUTO, AMBA_VOUT_HDMI_OVERSCAN_AUTO}; //HDMI Overscan
enum amba_vout_display_input vout_display_input[NUM_VOUT + 1] =  {AMBA_VOUT_INPUT_FROM_MIXER, AMBA_VOUT_INPUT_FROM_MIXER};
enum amba_vout_mixer_csc vout_mixer_csc[NUM_VOUT +1] = {AMBA_VOUT_MIXER_ENABLE_FOR_OSD, AMBA_VOUT_MIXER_ENABLE_FOR_OSD};
int vout_qt_osd[NUM_VOUT + 1] = {0, 0};
int restart_vout_flag = 0;//Dynamically restart vout
int vout_halt_flag = 0;	//Dynamically halt vout
int enable_csc_flag = 0; //Dynamically enable/disable csc
int configure_mixer_flag[NUM_VOUT + 1] = {0, 0}; //Dynamically configure mixer


typedef struct {
	const char 	*name;
	int		mode;
	int		width;
	int		height;
} vout_res_t;

vout_res_t vout_res[] = {
	//Typically for Analog and HDMI
	{"480i",	AMBA_VIDEO_MODE_480I,			720, 480},
	{"576i",	AMBA_VIDEO_MODE_576I,			720, 576},
	{"480p",	AMBA_VIDEO_MODE_D1_NTSC,		720, 480},
	{"576p",	AMBA_VIDEO_MODE_D1_PAL,			720, 576},
	{"720p",	AMBA_VIDEO_MODE_720P,			1280, 720},
	{"720p50",	AMBA_VIDEO_MODE_720P50,		1280, 720},
	{"720p30",	AMBA_VIDEO_MODE_720P30,			1280, 720},
	{"720p25",	AMBA_VIDEO_MODE_720P25,			1280, 720},
	{"720p24",	AMBA_VIDEO_MODE_720P24,			1280, 720},
	{"1080i",	AMBA_VIDEO_MODE_1080I,			1920, 1080},
	{"1080i50",	AMBA_VIDEO_MODE_1080I50,		1920, 1080},
	{"1080p24",	AMBA_VIDEO_MODE_1080P24,		1920, 1080},
	{"1080p25",	AMBA_VIDEO_MODE_1080P25,		1920, 1080},
	{"1080p30",	AMBA_VIDEO_MODE_1080P30,		1920, 1080},
	{"1080p",	AMBA_VIDEO_MODE_1080P,			1920, 1080},
	{"1080p50",	AMBA_VIDEO_MODE_1080P50,		1920, 1080},
	{"4mp30",	AMBA_VIDEO_MODE_2560X1440P30,		2560, 1440},
	{"4mp25",	AMBA_VIDEO_MODE_2560X1440P25,		2560, 1440},
	{"native",	AMBA_VIDEO_MODE_HDMI_NATIVE,		0,    0},
	{"2160p30",	AMBA_VIDEO_MODE_2160P30,		3840, 2160},
	{"2160p25",	AMBA_VIDEO_MODE_2160P25,		3840, 2160},
	{"2160p24",	AMBA_VIDEO_MODE_2160P24,		3840, 2160},
	{"2160p24se",	AMBA_VIDEO_MODE_2160P24_SE,	4096, 2160},

	// Typically for LCD
	{"D480I",	AMBA_VIDEO_MODE_480I,			720, 480},
	{"D576I",	AMBA_VIDEO_MODE_576I,			720, 576},
	{"D480P",	AMBA_VIDEO_MODE_D1_NTSC,		720, 480},
	{"D576P",	AMBA_VIDEO_MODE_D1_PAL,			720, 576},
	{"D720P",	AMBA_VIDEO_MODE_720P,			1280, 720},
	{"D720P50",	AMBA_VIDEO_MODE_720P50,		1280, 720},
	{"D1080I",	AMBA_VIDEO_MODE_1080I,			1920, 1080},
	{"D1080I50",	AMBA_VIDEO_MODE_1080I50,		1920, 1080},
	{"D1080P24",	AMBA_VIDEO_MODE_1080P24,		1920, 1080},
	{"D1080P25",	AMBA_VIDEO_MODE_1080P25,		1920, 1080},
	{"D1080P30",	AMBA_VIDEO_MODE_1080P30,		1920, 1080},
	{"D1080P",	AMBA_VIDEO_MODE_1080P,			1920, 1080},
	{"D1080P50",	AMBA_VIDEO_MODE_1080P50,		1920, 1080},
	{"D960x240",	AMBA_VIDEO_MODE_960_240,		960, 240},	//AUO27
	{"D320x240",	AMBA_VIDEO_MODE_320_240,		320, 240},	//AUO27
	{"D320x288",	AMBA_VIDEO_MODE_320_288,		320, 288},	//AUO27
	{"D360x240",	AMBA_VIDEO_MODE_360_240,		360, 240},	//AUO27
	{"D360x288",	AMBA_VIDEO_MODE_360_288,		360, 288},	//AUO27
	{"D480x640",	AMBA_VIDEO_MODE_480_640,		480, 640},	//P28K
	{"D480x800",	AMBA_VIDEO_MODE_480_800,		480, 800},	//TPO648
	{"hvga",	AMBA_VIDEO_MODE_HVGA,			320,  480},	//TPO489
	{"vga",		AMBA_VIDEO_MODE_VGA,			640,  480},
	{"wvga",	AMBA_VIDEO_MODE_WVGA,			800,  480},	//TD043
	{"D240x320",	AMBA_VIDEO_MODE_240_320,                240,  320},	//ST7789V
	{"D240x400",	AMBA_VIDEO_MODE_240_400,		240,  400},	//WDF2440
	{"xga",		AMBA_VIDEO_MODE_XGA,		       1024,  768},	//EJ080NA
	{"wsvga",	AMBA_VIDEO_MODE_WSVGA,	       1024,  600},	//AT070TNA2
	{"D960x540",	AMBA_VIDEO_MODE_960_540,		960,  540},	//E330QHD
	//{"D960x576",	AMBA_VIDEO_MODE_960_576,		960,    576},
	//{"D960x480",	AMBA_VIDEO_MODE_960_480,		960,    480},
};

int get_vout_mode(const char *name)
{
	int i;

	for (i = 0; i < sizeof(vout_res) / sizeof(vout_res[0]); i++)
		if (strcmp(vout_res[i].name, name) == 0)
			return vout_res[i].mode;

	printf("vout resolution '%s' not found\n", name);
	return -1;
}

typedef struct {
	int		width;
	int		height;
} vout_size_t;

int get_hdmi_native_size(vout_size_t *psize)
{
	int					sink_id;
	struct amba_vout_sink_info		sink_info;

	sink_id = vout_get_sink_id(VOUT_1, vout_device_type[VOUT_1]);
	if (sink_id < 0) {
		return -1;
	}

	sink_info.id = sink_id;
	if (ioctl(fd_iav, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info)) {
		return -1;
	}

	if (sink_info.hdmi_native_mode != AMBA_VIDEO_MODE_HDMI_NATIVE) {
		return -1;
	}

	psize->width	= sink_info.hdmi_native_width;
	psize->height	= sink_info.hdmi_native_height;

	return 0;
}

int get_vout_size(int mode, vout_size_t *psize)
{
	int i, got = 0;

	if (mode == AMBA_VIDEO_MODE_HDMI_NATIVE)
		return get_hdmi_native_size(psize);

	for (i = 0; i < sizeof(vout_res) / sizeof(vout_res[0]); i++) {
		if (vout_res[i].mode == mode) {
			psize->width = vout_res[i].width;
			psize->height = vout_res[i].height;
			got = 1;
			break;
		}
	}

	if (got)
		return 0;
	else
		return -1;
}

int get_hdmi_color_space(enum amba_vout_hdmi_color_space *hdmi_cs,
	const char *_cs)
{
	if (!strcmp(_cs, "rgb")) {
		*hdmi_cs = AMBA_VOUT_HDMI_CS_RGB;
		return 0;
	}

	if (!strcmp(_cs, "ycbcr444")) {
		*hdmi_cs = AMBA_VOUT_HDMI_CS_YCBCR_444;
		return 0;
	}

	if (!strcmp(_cs, "ycbcr422")) {
		*hdmi_cs = AMBA_VOUT_HDMI_CS_YCBCR_422;
		return 0;
	}

	return -1;
}

int get_mixer_csc(enum amba_vout_mixer_csc *mixer_csc,
	const char *_mixer_csc)
{
	if (!strcmp(_mixer_csc, "disable")) {
		*mixer_csc = AMBA_VOUT_MIXER_DISABLE;
		return 0;
	}

	if (!strcmp(_mixer_csc, "video")) {
		*mixer_csc = AMBA_VOUT_MIXER_ENABLE_FOR_VIDEO;
		return 0;
	}

	if (!strcmp(_mixer_csc, "osd")) {
		*mixer_csc = AMBA_VOUT_MIXER_ENABLE_FOR_OSD;
		return 0;
	}

	return -1;
}

int get_hdmi_3d_structure(enum ddd_structure *hdmi_3d_structure,
	const char *_3d_struct)
{
	int		ddd_struct;

	ddd_struct = atoi(_3d_struct);
	if (ddd_struct >= 0 && ddd_struct <= 15) {
		*hdmi_3d_structure = ddd_struct;
		return 0;
	} else {
		return -1;
	}
}

int get_hdmi_overscan(enum amba_vout_hdmi_overscan *hdmi_overscan,
	const char *_hdmi_overscan)
{
	if (!strcmp(_hdmi_overscan, "force")) {
		*hdmi_overscan = AMBA_VOUT_HDMI_FORCE_OVERSCAN;
		return 0;
	}

	if (!strcmp(_hdmi_overscan, "free")) {
		*hdmi_overscan = AMBA_VOUT_HDMI_NON_FORCE_OVERSCAN;
		return 0;
	}

	if (!strcmp(_hdmi_overscan, "auto")) {
		*hdmi_overscan = AMBA_VOUT_HDMI_OVERSCAN_AUTO;
		return 0;
	}

	return -1;
}


/*==============================================================================
  Vout options which will affect Video only
==============================================================================*/
int vout_video_en[NUM_VOUT + 1] = {1, 1};	//enable/disable video layer
struct amba_vout_video_size vout_video_size[NUM_VOUT + 1] = {
	{
		.specified = 0,
	},
	{
		.specified = 0,
	},
}; //Size of video window
struct amba_vout_video_offset vout_video_offset[NUM_VOUT + 1] = {
	{
		.specified = 0,
	},
	{
		.specified = 0,
	},
}; //Offset of video window
enum {
	FLIP_NORMAL = 0,
	FLIP_HV = 1,
	FLIP_H = 2,
	FLIP_V = 3,
};
int vout_video_flip[NUM_VOUT + 1] = {0}; //Flip video layer horizontally, vertically or not
int vout_video_rotate[NUM_VOUT + 1] = {0}; //Rotate video layer or not, valid only for LCD
int enable_video_flag = 0;	//Dynamically enable/disable video layer
int flip_video_flag = 0;	//Dynamically flip video
int rotate_video_flag = 0; //Dynamically rotate video
int change_video_size_flag = 0; //Dynamically change video window size
int change_video_offset_flag = 0; //Dynamically change video window offset

int get_video_flip(int *pflip, const char *parg)
{
	if (!strcmp(parg, "hv")) {
		*pflip = FLIP_HV;
		return 0;
	}

	if (!strcmp(parg, "h")) {
		*pflip = FLIP_H;
		return 0;
	}

	if (!strcmp(parg, "v")) {
		*pflip = FLIP_V;
		return 0;
	}

	return -1;
}

int get_video_size(struct amba_vout_video_size *pvsize,
	const char *psize)
{
	int w = -1, h = -1;

	sscanf(psize, "%dx%d", &w, &h);
	if (w != -1 && h != -1) {
		pvsize->specified = 1;
		pvsize->video_width = w;
		pvsize->video_height = h;
		return 0;
	} else {
		pvsize->specified = 0;
		return -1;
	}
}

int get_video_offset(struct amba_vout_video_offset *pvoffset,
	const char *poffset)
{
	int x = -1, y = -1;

	sscanf(poffset, "%dx%d", &x, &y);
	if (x != -1 && y != -1) {
		pvoffset->specified = 1;
		pvoffset->offset_x = x;
		pvoffset->offset_y = y;
		return 0;
	} else {
		pvoffset->specified = 0;
		return -1;
	}
}


/*==============================================================================
  Vout options which will affect OSD only
==============================================================================*/
int vout_osd_flip[NUM_VOUT + 1] = {0}; //Flip osd layer horizontally, vertically or not
int vout_fb_id[NUM_VOUT + 1] = {-2, -2}; //associated fb id, no fb by default
struct amba_vout_osd_rescale vout_osd_rescale[NUM_VOUT + 1] = {
	{
		.enable = 0,
	},
	{
		.enable = 0,
	},
};	//OSD size after rescaling, no rescaling by default
struct amba_vout_osd_offset vout_osd_offset[NUM_VOUT + 1] = {
	{
		.specified = 0,
	},
	{
		.specified = 0,
	},
}; //Offset of osd window
int select_fb_flag = 0;	//Dynamically specify fb id
int flip_osd_flag = 0;	//Dynamically flip osd
int rescale_osd_flag = 0; //Dynamically rescale OSD
int change_osd_offset_flag = 0; //Dynamically change OSD offset

int get_osd_flip(int *pflip, const char *parg)
{
	if (!strcmp(parg, "hv")) {
		*pflip = FLIP_HV;
		return 0;
	}

	if (!strcmp(parg, "h")) {
		*pflip = FLIP_H;
		return 0;
	}

	if (!strcmp(parg, "v")) {
		*pflip = FLIP_V;
		return 0;
	}

	return -1;
}

int get_rescaled_osd_size(struct amba_vout_osd_rescale *prescale,
	const char *psize)
{
	int w = 0, h = 0;

	sscanf(psize, "%dx%d", &w, &h);
	if (w && h) {
		prescale->enable = 1;
		prescale->width = w;
		prescale->height = h;
		return 0;
	} else {
		prescale->enable = 0;
		return -1;
	}
}

int get_osd_offset(struct amba_vout_osd_offset *pooffset,
	const char *poffset)
{
	int x = -1, y = -1;

	sscanf(poffset, "%dx%d", &x, &y);
	if (x != -1 && y != -1) {
		pooffset->specified = 1;
		pooffset->offset_x = x;
		pooffset->offset_y = y;
		return 0;
	} else {
		pooffset->specified = 0;
		return -1;
	}
}


/*==============================================================================
  Get Sink ID from sink type
==============================================================================*/
int vout_get_sink_id(int chan, int sink_type)
{
	int					num;
	int					i;
	struct amba_vout_sink_info		sink_info;
	u32					sink_id = -1;

	num = 0;
	if (ioctl(fd_iav, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
		perror("IAV_IOC_VOUT_GET_SINK_NUM");
		return -1;
	}
	if (num < 1) {
		printf("Please load vout driver!\n");
		return -1;
	}

	for (i = num - 1; i >= 0; i--) {
		sink_info.id = i;
		if (ioctl(fd_iav, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
			perror("IAV_IOC_VOUT_GET_SINK_INFO");
			return -1;
		}

		printf("sink %d is %s\n", sink_info.id, sink_info.name);

		if ((sink_info.sink_type == sink_type) &&
			(sink_info.source_id == chan))
			sink_id = sink_info.id;
	}

	printf("%s: %d %d, return %d\n", __func__, chan, sink_type, sink_id);

	return sink_id;
}


/*==============================================================================
  Vout command line options
==============================================================================*/
#define VOUT_NUMERIC_SHORT_OPTIONS	\
	RESTART_VOUT = VOUT_OPTIONS_BASE,	\
	HALT_VOUT,	\
	OUTPUT_CVBS,	\
	OUTPUT_YPBPR,	\
	OUTPUT_LCD,	\
	OUTPUT_HDMI,	\
	SPECIFY_VOUT_FRAMERATE,	\
	SPECIFY_HDMI_COLOR_SPACE,	\
	SPECIFY_HDMI_3D_STRUCTURE,	\
	SPECIFY_HDMI_OVERSCAN,	\
	DISABLE_VOUT_CSC,	\
	DYNAMICALLY_ED_VOUT_CSC,	\
	DISABLE_VIDEO,	\
	DYNAMICALLY_ED_VIDEO,	\
	FLIP_VIDEO,	\
	DYNAMICALLY_ED_VIDEO_FLIP,	\
	ROTATE_VIDEO,	\
	DYNAMICALLY_ED_VIDEO_ROTATION,	\
	SPECIFY_VIDEO_SIZE,	\
	DYNAMICALLY_CHANGE_VIDEO_SIZE,	\
	SPECIFY_VIDEO_OFFSET,	\
	DYNAMICALLY_CHANGE_VIDEO_OFFSET,	\
	SPECIFY_FB_ID,	\
	DYNAMICALLY_SPECIFY_FB_ID,	\
	FLIP_OSD,	\
	DYNAMICALLY_ED_OSD_FLIP,	\
	SPECIFY_RESCALED_OSD_SIZE,	\
	DYNAMICALLY_ED_OSD_RESCALING,	\
	SPECIFY_OSD_OFFSET,	\
	SPECIFY_QT_OSD,	\
	DYNAMICALLY_CHANGE_OSD_OFFSET,	\
	MIXER_ENABLE,   \
	MIXER_CSC

#define CVBS_OPTIONS	\
	{"cvbs",                  NO_ARG,   0,   OUTPUT_CVBS,                            },
#define CVBS_HINTS	\
	{"", "\t\tSelect cvbs output"},

#define YPBPR_OPTIONS	\
	{"ypbpr",                 NO_ARG,   0,   OUTPUT_YPBPR,                           },
#define YPBPR_HINTS	\
	{"", "\t\tSelect ypbpr output"},

#define LCD_OPTIONS	\
	{"lcd",                   HAS_ARG,  0,   OUTPUT_LCD,                             },
#define LCD_HINTS	\
	{"", "\t\tSelect lcd output"},

#define HDMI_OPTIONS	\
	{"hdmi",                  NO_ARG,   0,   OUTPUT_HDMI,                            },
#define HDMI_HINTS	\
	{"", "\t\tSelect hdmi output"},

#define FRAMERATE_OPTIONS	\
	{"fps",                   HAS_ARG,  0,   SPECIFY_VOUT_FRAMERATE,                 },
#define FRAMERATE_HINTS	\
	{"50|60", "\tSpecify vout frame rate"},

#define HDMI_COLOR_SPACE_OPTIONS	\
	{"cs",                    HAS_ARG,  0,   SPECIFY_HDMI_COLOR_SPACE,               },
#define HDMI_COLOR_SPACE_HINTS	\
	{"rgb|ycbcr444", "\tSpecify hdmi output color space"},

#define CSC_OPTIONS	\
	{"disable-csc",           NO_ARG,   0,   DISABLE_VOUT_CSC,                       },	\
	{"edc",                   NO_ARG,   0,   DYNAMICALLY_ED_VOUT_CSC,                },     \
	{"mixer-csc",             HAS_ARG,  0,   MIXER_CSC,                      },
#define CSC_HINTS	\
	{"", "\tDisable vout csc"},	\
	{"", "\t\tEnable or Disable color space conversion of the corresponding vout"}, \
	{"disable|video|osd", "Select mixer csc"},

#define HDMI_3D_STRUCTURE_OPTIONS	\
	{"3d",                    HAS_ARG,  0,   SPECIFY_HDMI_3D_STRUCTURE,               },
#define HDMI_3D_STRUCTURE_HINTS	\
	{"0-15", "\t\tSpecify hdmi output 3d structure"},

#define HDMI_OVERSCAN_OPTIONS	\
	{"overscan",              HAS_ARG,  0,   SPECIFY_HDMI_OVERSCAN,                   },
#define HDMI_OVERSCAN_HINTS	\
	{"force|free|auto", "Specify hdmi overscan information"},

#define VIDEO_OPTIONS	\
	{"disable-video",         NO_ARG,   0,   DISABLE_VIDEO,                          },	\
	{"edv",                   NO_ARG,   0,   DYNAMICALLY_ED_VIDEO,                   },
#define VIDEO_HINTS	\
	{"", "\tDisable vout video layer"},	\
	{"", "\t\tEnable or Disable video of the corresponding vout"},

#define ROTATE_VIDEO_OPTIONS	\
	{"rotatev",                NO_ARG,   0,   ROTATE_VIDEO,                           },	\
	{"rv",                    NO_ARG,   0,   DYNAMICALLY_ED_VIDEO_ROTATION,          },
#define ROTATE_VIDEO_HINTS	\
	{"", "\t\tSelect vout rotation"},	\
	{"", "\t\t\tRotate video of the corresponding vout"},

#define FLIP_VIDEO_OPTIONS	\
	{"flipv",               HAS_ARG,   0,   FLIP_VIDEO,                              },	\
	{"fv",                  NO_ARG,    0,   DYNAMICALLY_ED_VIDEO_FLIP,               },
#define FLIP_VIDEO_HINTS	\
	{"", "\t\tSelect vout video flip"},	\
	{"", "\t\t\tFlip video of the corresponding vout"},

#define VIDEO_SIZE_OPTIONS	\
	{"vs",                    HAS_ARG,  0,   SPECIFY_VIDEO_SIZE,                     },	\
	{"cvs",                   NO_ARG,   0,   DYNAMICALLY_CHANGE_VIDEO_SIZE,          },
#define VIDEO_SIZE_HINTS	\
	{"<w>x<h>", "\tSpecify size of the video window"},	\
	{"", "\t\tDynamically change the size of the video window"},

#define VIDEO_OFFSET_OPTIONS	\
	{"vo",                    HAS_ARG,  0,   SPECIFY_VIDEO_OFFSET,                     },	\
	{"cvo",                   NO_ARG,   0,   DYNAMICALLY_CHANGE_VIDEO_OFFSET,          },
#define VIDEO_OFFSET_HINTS	\
	{"<xoff>x<yoff>", "\tSpecify offset of the video window"},	\
	{"", "\t\tDynamically change the offset of the video window"},

#define FB_ID_OPTIONS	\
	{"fb",                    HAS_ARG,  0,   SPECIFY_FB_ID,                          },	\
	{"sfb",                   NO_ARG,   0,   DYNAMICALLY_SPECIFY_FB_ID,              },
#define FB_ID_HINTS	\
	{"-1|0|1", "\tSelect frame buffer id"},	\
	{"", "\t\tSelect frame buffer of the corresponding vout"},

#define FLIP_OSD_OPTIONS	\
	{"flipo",               HAS_ARG,   0,   FLIP_OSD,                              },	\
	{"fo",                  NO_ARG,    0,   DYNAMICALLY_ED_OSD_FLIP,               },
#define FLIP_OSD_HINTS	\
	{"", "\t\tSelect vout osd flip"},	\
	{"", "\t\t\tFlip osd of the corresponding vout"},

#define OSD_RESCALE_OPTIONS	\
	{"ors",                   HAS_ARG,  0,   SPECIFY_RESCALED_OSD_SIZE,              },	\
	{"edr",                   NO_ARG,   0,   DYNAMICALLY_ED_OSD_RESCALING,           },
#define OSD_RESCALE_HINTS	\
	{"<w>x<h>", "\tSpecify OSD rescale size"},	\
	{"", "\t\tEnable or Disable OSD rescaler of the corresponding vout"},

#define OSD_OFFSET_OPTIONS	\
	{"oo",                    HAS_ARG,  0,   SPECIFY_OSD_OFFSET,                     },	\
	{"coo",                   NO_ARG,   0,   DYNAMICALLY_CHANGE_OSD_OFFSET,          },
#define OSD_OFFSET_HINTS	\
	{"<xoff>x<yoff>", "\tSpecify offset of the osd window"},	\
	{"", "\t\tDynamically change the offset of the osd window"},

#define OSD_QT_OPTIONS	\
	{"qt",                    NO_ARG,  0,   SPECIFY_QT_OSD,                     },
#define OSD_QT_HINTS	\
	{"", "\t\t\tQT tailored OSD"},

#define MIXER_ENABLE_OPTIONS	\
	{"mixer",                    HAS_ARG,  0,   MIXER_ENABLE,                     },
#define MIXER_ENABLE_HINTS	\
	{"0|1", "\tEnable mixer for the corresponding vout"},


#define VOUT_LONG_OPTIONS()	\
	{"vout",                  HAS_ARG,  0,   'v',                                    },	\
	{"vout2",                 HAS_ARG,  0,   'V',                                    },	\
	{"rsv",		  	  NO_ARG,   0,    RESTART_VOUT,                          },	\
	{"htv",		  	  NO_ARG,   0,    HALT_VOUT,                             },	\
	CVBS_OPTIONS	\
	YPBPR_OPTIONS	\
	LCD_OPTIONS	\
	HDMI_OPTIONS	\
	FRAMERATE_OPTIONS	\
	HDMI_COLOR_SPACE_OPTIONS	\
	CSC_OPTIONS	\
	HDMI_3D_STRUCTURE_OPTIONS	\
	HDMI_OVERSCAN_OPTIONS	\
	VIDEO_OPTIONS	\
	FLIP_VIDEO_OPTIONS	\
	ROTATE_VIDEO_OPTIONS	\
	VIDEO_SIZE_OPTIONS	\
	VIDEO_OFFSET_OPTIONS	\
	FB_ID_OPTIONS	\
	FLIP_OSD_OPTIONS	\
	OSD_RESCALE_OPTIONS	\
	OSD_OFFSET_OPTIONS \
	OSD_QT_OPTIONS	\
	MIXER_ENABLE_OPTIONS

#define VOUT_PARAMETER_HINTS()	\
	{"vout mode", "\tChange vout mode"},	\
	{"vout2 mode", "\tChange vout2 mode"},	\
	{"", "\t\tRestart the corresponding vout"},	\
	{"", "\t\tHalt the corresponding vout"},	\
	CVBS_HINTS	\
	YPBPR_HINTS	\
	LCD_HINTS	\
	HDMI_HINTS	\
	FRAMERATE_HINTS	\
	HDMI_COLOR_SPACE_HINTS	\
	CSC_HINTS	\
	HDMI_3D_STRUCTURE_HINTS	\
	HDMI_OVERSCAN_HINTS	\
	VIDEO_HINTS	\
	FLIP_VIDEO_HINTS	\
	ROTATE_VIDEO_HINTS	\
	VIDEO_SIZE_HINTS	\
	VIDEO_OFFSET_HINTS	\
	FB_ID_HINTS	\
	FLIP_OSD_HINTS	\
	OSD_RESCALE_HINTS	\
	OSD_OFFSET_HINTS	\
	OSD_QT_HINTS	\
	MIXER_ENABLE_HINTS

#define VOUT_INIT_PARAMETERS()	\
		case 'v':	\
			if (current_vout == VOUT_TBD && vout_device_type[VOUT_TBD] != AMBA_VOUT_SINK_TYPE_AUTO) {	\
				current_vout = VOUT_0;	\
				vout_device_selected_flag[current_vout] = vout_device_selected_flag[VOUT_TBD];	\
				vout_device_type[current_vout] = vout_device_type[VOUT_TBD];	\
				vout_lcd_model_index[current_vout] = vout_lcd_model_index[VOUT_TBD];	\
				vout_fps[current_vout] = vout_fps[VOUT_TBD];	\
				vout_video_rotate[current_vout] = vout_video_rotate[VOUT_TBD];	\
				vout_csc_en[current_vout] = vout_csc_en[VOUT_TBD];	\
				vout_fb_id[current_vout] = vout_fb_id[VOUT_TBD];	\
				vout_video_en[current_vout] = vout_video_en[VOUT_TBD];	\
				vout_video_size[current_vout] = vout_video_size[VOUT_TBD];	\
				vout_qt_osd[current_vout] = vout_qt_osd[VOUT_TBD];	\
			}	\
			current_vout = VOUT_0;	\
			vout_flag[current_vout] = 1;	\
			vout_mode[current_vout] = get_vout_mode(optarg);	\
			if (vout_mode[current_vout] < 0)	\
				return -1;	\
			break;	\
		case 'V':	\
			if (current_vout == VOUT_TBD && vout_device_type[VOUT_TBD] != AMBA_VOUT_SINK_TYPE_AUTO) {	\
				current_vout = VOUT_1;	\
				vout_device_selected_flag[current_vout] = vout_device_selected_flag[VOUT_TBD];	\
				vout_device_type[current_vout] = vout_device_type[VOUT_TBD];	\
				vout_lcd_model_index[current_vout] = vout_lcd_model_index[VOUT_TBD];	\
				vout_fps[current_vout] = vout_fps[VOUT_TBD];	\
				vout_hdmi_cs[current_vout] = vout_hdmi_cs[VOUT_TBD];	\
				vout_hdmi_3d_structure[current_vout] = vout_hdmi_3d_structure[VOUT_TBD];	\
				vout_hdmi_overscan[current_vout] = vout_hdmi_overscan[VOUT_TBD];	\
				vout_video_rotate[current_vout] = vout_video_rotate[VOUT_TBD];	\
				vout_csc_en[current_vout] = vout_csc_en[VOUT_TBD];	\
				vout_fb_id[current_vout] = vout_fb_id[VOUT_TBD];	\
				vout_video_en[current_vout] = vout_video_en[VOUT_TBD];	\
				vout_video_size[current_vout] = vout_video_size[VOUT_TBD];	\
				vout_qt_osd[current_vout] = vout_qt_osd[VOUT_TBD];	\
			}	\
			current_vout = VOUT_1;	\
			vout_flag[current_vout] = 1;	\
			vout_mode[current_vout] = get_vout_mode(optarg);	\
			if (vout_mode[current_vout] < 0)	\
				return -1;	\
			break;	\
		case RESTART_VOUT:	\
			restart_vout_flag = 1;	\
			break;	\
		case HALT_VOUT:	\
			vout_halt_flag = 1;	\
			break;	\
		case OUTPUT_CVBS:	\
			vout_device_selected_flag[current_vout] = 1;	\
			vout_device_type[current_vout] = AMBA_VOUT_SINK_TYPE_CVBS;	\
			break;	\
		case OUTPUT_YPBPR:	\
			vout_device_selected_flag[current_vout] = 1;	\
			vout_device_type[current_vout] = AMBA_VOUT_SINK_TYPE_YPBPR;	\
			break;	\
		case OUTPUT_LCD:	\
			vout_device_selected_flag[current_vout] = 1;	\
			vout_device_type[current_vout] = AMBA_VOUT_SINK_TYPE_DIGITAL;	\
			vout_lcd_model_index[current_vout] = get_lcd_model_index(optarg);	\
			break;	\
		case OUTPUT_HDMI:	\
			vout_device_selected_flag[current_vout] = 1;	\
			vout_device_type[current_vout] = AMBA_VOUT_SINK_TYPE_HDMI;	\
			break;	\
		case SPECIFY_VOUT_FRAMERATE:	\
			vout_fps[current_vout] = AMBA_VIDEO_FPS(atoi(optarg));	\
			break;	\
		case SPECIFY_HDMI_COLOR_SPACE:	\
			if (get_hdmi_color_space(&vout_hdmi_cs[current_vout], optarg))	\
				printf("Warning: Wrong hdmi color space, hdmi color space of vout%d keeps unchanged!\n", current_vout);	\
			break;	\
		case SPECIFY_HDMI_3D_STRUCTURE:	\
			if (get_hdmi_3d_structure(&vout_hdmi_3d_structure[current_vout], optarg))	\
				printf("Warning: Wrong hdmi 3d structure, hdmi 3d structure of vout%d keeps unchanged!\n", current_vout);	\
			break;	\
		case SPECIFY_HDMI_OVERSCAN:	\
			if (get_hdmi_overscan(&vout_hdmi_overscan[current_vout], optarg))	\
				printf("Warning: Wrong hdmi overscan information, hdmi overscan information of vout%d keeps unchanged!\n", current_vout);	\
			break;	\
		case DISABLE_VOUT_CSC:	\
			vout_csc_en[current_vout] = 0;	\
			break;	\
		case MIXER_CSC:	\
			if (get_mixer_csc(&vout_mixer_csc[current_vout], optarg))	\
				printf("Warning: Wrong mixer csc!\n");	\
			break;	\
		case DYNAMICALLY_ED_VOUT_CSC:	\
			enable_csc_flag = 1;	\
			break;	\
	\
		case DISABLE_VIDEO:	\
			vout_video_en[current_vout] = 0;	\
			break;	\
		case DYNAMICALLY_ED_VIDEO:	\
			enable_video_flag = 1;	\
			break;	\
		case FLIP_VIDEO:	\
			if (get_video_flip(&vout_video_flip[current_vout], optarg))	\
				printf("Warning: Wrong video flip, video flip of vout%d keeps unchanged!\n", current_vout);	\
			break;	\
		case DYNAMICALLY_ED_VIDEO_FLIP:	\
			flip_video_flag = 1;	\
			break;	\
		case ROTATE_VIDEO:	\
			vout_video_rotate[current_vout] = 1;	\
			break;	\
		case DYNAMICALLY_ED_VIDEO_ROTATION:	\
			rotate_video_flag = 1;	\
			break;	\
		case SPECIFY_VIDEO_SIZE:	\
			if (get_video_size(&vout_video_size[current_vout], optarg))	\
				printf("Warning: Wrong video size, video size of vout%d keeps unchanged!\n", current_vout);	\
			break;	\
		case DYNAMICALLY_CHANGE_VIDEO_SIZE:	\
			change_video_size_flag = 1;	\
			break;	\
		case SPECIFY_VIDEO_OFFSET:	\
			if (get_video_offset(&vout_video_offset[current_vout], optarg))	\
				printf("Warning: Wrong video offset, video offset of vout%d keeps unchanged!\n", current_vout);	\
			break;	\
		case DYNAMICALLY_CHANGE_VIDEO_OFFSET:	\
			change_video_offset_flag = 1;	\
			break;	\
	\
		case SPECIFY_FB_ID:	\
			vout_fb_id[current_vout] = atoi(optarg);	\
			break;	\
		case DYNAMICALLY_SPECIFY_FB_ID:	\
			select_fb_flag = 1;	\
			break;	\
		case FLIP_OSD:	\
			if (get_osd_flip(&vout_osd_flip[current_vout], optarg))	\
				printf("Warning: Wrong osd flip, osd flip of vout%d keeps unchanged!\n", current_vout);	\
			break;	\
		case DYNAMICALLY_ED_OSD_FLIP:	\
			flip_osd_flag = 1;	\
			break;	\
		case SPECIFY_RESCALED_OSD_SIZE:	\
			if (get_rescaled_osd_size(&vout_osd_rescale[current_vout], optarg))	\
				printf("Warning: Wrong OSD rescale parameter, OSD rescale ignored for vout%d!\n", current_vout);	\
			break;	\
		case DYNAMICALLY_ED_OSD_RESCALING:	\
			rescale_osd_flag = 1;	\
			break;	\
	\
		case SPECIFY_OSD_OFFSET:	\
			if (get_osd_offset(&vout_osd_offset[current_vout], optarg))	\
				printf("Warning: Wrong osd offset, osd offset of vout%d keeps unchanged!\n", current_vout);	\
			break;	\
		case SPECIFY_QT_OSD:	\
			vout_qt_osd[current_vout] = 1;	\
			break;	\
		case DYNAMICALLY_CHANGE_OSD_OFFSET:	\
			change_osd_offset_flag = 1;		\
			break;	\
	\
		case MIXER_ENABLE:	\
			configure_mixer_flag[current_vout] = 1;	\
			if (atoi(optarg) == 0)	\
				vout_display_input[current_vout] = AMBA_VOUT_INPUT_FROM_SMEM;		\
			else		\
				vout_display_input[current_vout] = AMBA_VOUT_INPUT_FROM_MIXER;		\
			break;


/*==============================================================================
  Check vout arguments
==============================================================================*/
int check_vout(void)
{
	int i;

	if (vout_flag[VOUT_0] && vout_flag[VOUT_1]) {
		//Two vouts display the same fb but use different csc
		if (vout_fb_id[VOUT_0] == vout_fb_id[VOUT_1]
			&& vout_fb_id[VOUT_0] >= 0) {
			if (vout_csc_en[VOUT_0] != vout_csc_en[VOUT_1]) {
				printf("Error: %s, two vouts should ", __func__);
				printf("both enable or disable csc when ");
				printf("displaying the same frame buffer!\n");
				return -1;
			}
		}

		//Rescale OSD of both vouts
		if (vout_osd_rescale[VOUT_0].enable && vout_osd_rescale[VOUT_1].enable) {
			printf("Error: Can not enable OSD rescaler for both vouts at the same time!\n");
			return -1;
		}
	}

	//Video size
	for (i = 0; i < NUM_VOUT; i++) {
		vout_size_t vout_size;

		if (!vout_flag[i])
			continue;

		memset(&vout_size, 0, sizeof(vout_size_t));
		if (get_vout_size(vout_mode[i], &vout_size)) {
			printf("%s: Incorrect vout%d mode!\n", __func__, i);
			return -1;
		}

		vout_video_size[i].vout_width	= vout_size.width;
		vout_video_size[i].vout_height	= vout_size.height;
		if (vout_video_size[i].specified) {
			if (vout_video_size[i].video_width > vout_size.width) {
				vout_video_size[i].video_width = vout_size.width;
				printf("%s: Vout%d video width changed to %d\n", __func__, i, vout_size.width);
			}
			if (vout_video_size[i].video_height > vout_size.height) {
				vout_video_size[i].video_height = vout_size.height;
				printf("%s: Vout%d video height changed to %d\n", __func__, i, vout_size.height);
			}
		} else {
			vout_video_size[i].video_width = vout_size.width;
			vout_video_size[i].video_height = vout_size.height;
			vout_video_size[i].specified = 1;
		}
	}

	//Video offset
	for (i = 0; i < NUM_VOUT; i++) {
		vout_size_t vout_size;
		int blank_x, blank_y;

		if (!vout_flag[i])
			continue;

		memset(&vout_size, 0, sizeof(vout_size_t));
		if (get_vout_size(vout_mode[i], &vout_size)) {
			printf("%s: Incorrect vout%d mode!\n", __func__, i);
			return -1;
		}

		blank_x = vout_size.width - vout_video_size[i].video_width;
		blank_y = vout_size.height- vout_video_size[i].video_height;

		if (vout_video_offset[i].specified) {
			if (vout_video_offset[i].offset_x < 0)
				vout_video_offset[i].offset_x = 0;
			if (vout_video_offset[i].offset_x > blank_x)
				vout_video_offset[i].offset_x = blank_x;
			if (vout_video_offset[i].offset_y < 0)
				vout_video_offset[i].offset_y = 0;
			if (vout_video_offset[i].offset_y > blank_y)
				vout_video_offset[i].offset_y = blank_y;
		} else {
			vout_video_offset[i].offset_x = blank_x >> 1;
			vout_video_offset[i].offset_y = blank_y >> 1;
		}

		/*if (vout_mode[i] == AMBA_VIDEO_MODE_480I || vout_mode[i] == AMBA_VIDEO_MODE_576I)
				vout_video_offset[i].offset_x <<= 1;
		if (vout_mode[i] == AMBA_VIDEO_MODE_480I || vout_mode[i] == AMBA_VIDEO_MODE_576I ||
			vout_mode[i] == AMBA_VIDEO_MODE_1080I || vout_mode[i] == AMBA_VIDEO_MODE_1080I50)
			vout_video_offset[i].offset_y >>= 1;*/

		if (vout_video_offset[i].offset_x > 0)
			vout_video_offset[i].offset_x--;
		if (vout_video_offset[i].offset_y > 0)
			vout_video_offset[i].offset_y--;
	}

	//OSD size
	for (i = 0; i < NUM_VOUT; i++) {
		if (!vout_flag[i])
			continue;

		if (vout_osd_rescale[i].enable) {
			vout_size_t vout_size;

			memset(&vout_size, 0, sizeof(vout_size_t));
			if (get_vout_size(vout_mode[i], &vout_size)) {
				printf("%s: Incorrect vout%d mode!\n", __func__, i);
				return -1;
			}

			if (vout_osd_rescale[i].width > vout_size.width) {
				vout_osd_rescale[i].width = vout_size.width;
				printf("%s: Vout%d OSD rescale width changed to %d\n", __func__, i, vout_size.width);
			}
			if (vout_osd_rescale[i].height > vout_size.height) {
				vout_osd_rescale[i].height = vout_size.height;
				printf("%s: Vout%d OSD rescale height changed to %d\n", __func__, i, vout_size.height);
			}
		}
	}

	//OSD offset
	for (i = 0; i < NUM_VOUT; i++) {
		if (!vout_flag[i])
			continue;

		if (vout_osd_offset[i].specified) {
			if (vout_osd_offset[i].offset_x < 0)
				vout_osd_offset[i].offset_x = 0;
			if (vout_osd_offset[i].offset_y < 0)
				vout_osd_offset[i].offset_y = 0;
		}
	}

	return 0;
}


/*==============================================================================
  Initialize vout:
  	vout_id: 0-LCD 1-HDMI
  	direct_to_dsp: 0-dsp cmds will be cached in vout and sent to dsp by iav
  		       1-dsp cmds will be sent to dsp directly,
  			usually used to restart vout
==============================================================================*/
int init_vout(int vout_id, u32 direct_to_dsp)
{
	//external devices
	int device_selected, sink_type, lcd_model;

	//Options related to the whole vout
	int mode, csc_en, fps, qt;
	enum amba_vout_hdmi_color_space hdmi_cs;
	enum ddd_structure hdmi_3d_structure;
	enum amba_vout_hdmi_overscan hdmi_overscan;
	enum amba_vout_display_input display_input;
	enum amba_vout_mixer_csc mixer_csc;

	//Options related to Video only
	int video_en, video_flip, video_rotate;
	struct amba_vout_video_size video_size;
	struct amba_vout_video_offset video_offset;

	//Options related to OSD only
	int fb_id, osd_flip;
	struct amba_vout_osd_rescale osd_rescale;
	struct amba_vout_osd_offset osd_offset;

	//For sink config
	int sink_id = -1;
	struct amba_video_sink_mode sink_cfg;

	struct iav_system_resource resource;

	device_selected = vout_device_selected_flag[vout_id];
	sink_type = vout_device_type[vout_id];
	lcd_model = vout_lcd_model_index[vout_id];
	mode = vout_mode[vout_id];
	csc_en = vout_csc_en[vout_id];
	fps = vout_fps[vout_id];
	qt = vout_qt_osd[vout_id];
	hdmi_cs = vout_hdmi_cs[vout_id];
	hdmi_3d_structure = vout_hdmi_3d_structure[vout_id];
	hdmi_overscan = vout_hdmi_overscan[vout_id];
	video_en = vout_video_en[vout_id];
	video_flip = vout_video_flip[vout_id];
	video_rotate = vout_video_rotate[vout_id];
	video_size = vout_video_size[vout_id];
	video_offset = vout_video_offset[vout_id];
	fb_id = vout_fb_id[vout_id];
	osd_flip = vout_osd_flip[vout_id];
	osd_rescale = vout_osd_rescale[vout_id];
	osd_offset = vout_osd_offset[vout_id];
	mixer_csc = vout_mixer_csc[vout_id];

	//Specify external device type for vout
	if (device_selected) {
		sink_id = vout_get_sink_id(vout_id, sink_type);
		if (ioctl(fd_iav, IAV_IOC_VOUT_SELECT_DEV, sink_id) < 0) {
			perror("IAV_IOC_VOUT_SELECT_DEV");
			switch (sink_type) {
			case AMBA_VOUT_SINK_TYPE_CVBS:
				printf("No CVBS sink: ");
				printf("Driver not loaded!\n");
				break;

			case AMBA_VOUT_SINK_TYPE_YPBPR:
				printf("No YPBPR sink: ");
				printf("Driver not loaded ");
				printf("or ypbpr output not supported!\n");
				break;

			case AMBA_VOUT_SINK_TYPE_DIGITAL:
				printf("No DIGITAL sink: ");
				printf("Driver not loaded ");
				printf("or digital output not supported!\n");
				break;

			case AMBA_VOUT_SINK_TYPE_HDMI:
				printf("No HDMI sink: ");
				printf("Hdmi cable not plugged, ");
				printf("driver not loaded, ");
				printf("or hdmi output not supported!\n");
				break;

			default:
				break;
			}
			return -1;
		}
	}

	// update display input
	if (!configure_mixer_flag[vout_id]) {
		memset(&resource, 0, sizeof(resource));
		resource.encode_mode = DSP_CURRENT_MODE;
		if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource) < 0) {
			perror("IAV_IOC_GET_SYSTEM_RESOURCE");
			return -1;
		}
		if (vout_id == VOUT_0) {
			vout_display_input[vout_id] = resource.mixer_a_enable ?
				AMBA_VOUT_INPUT_FROM_MIXER : AMBA_VOUT_INPUT_FROM_SMEM;
		} else {
			vout_display_input[vout_id] = resource.mixer_b_enable ?
				AMBA_VOUT_INPUT_FROM_MIXER : AMBA_VOUT_INPUT_FROM_SMEM;
		}
	}
	display_input = vout_display_input[vout_id];

	//Configure vout
	memset(&sink_cfg, 0, sizeof(sink_cfg));
	if (sink_type == AMBA_VOUT_SINK_TYPE_DIGITAL) {
		if (lcd_model_list[lcd_model].lcd_setmode(mode, &sink_cfg) < 0) {
			perror("lcd_setmode fail.");
			return -1;
		}
	} else {
		sink_cfg.mode = mode;
		sink_cfg.ratio = AMBA_VIDEO_RATIO_AUTO;
		sink_cfg.bits = AMBA_VIDEO_BITS_AUTO;
		sink_cfg.type = sink_type;
		if (mode == AMBA_VIDEO_MODE_480I || mode == AMBA_VIDEO_MODE_576I
			|| mode == AMBA_VIDEO_MODE_1080I
			|| mode == AMBA_VIDEO_MODE_1080I50)
			sink_cfg.format = AMBA_VIDEO_FORMAT_INTERLACE;
		else
			sink_cfg.format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		sink_cfg.sink_type = AMBA_VOUT_SINK_TYPE_AUTO;
		sink_cfg.bg_color.y = 0x10;
		sink_cfg.bg_color.cb = 0x80;
		sink_cfg.bg_color.cr = 0x80;
		sink_cfg.lcd_cfg.mode = AMBA_VOUT_LCD_MODE_DISABLE;
	}
	if (!csc_en) {
		sink_cfg.bg_color.y	= 0;
		sink_cfg.bg_color.cb	= 0;
		sink_cfg.bg_color.cr	= 0;
	}
	if (qt) {
		sink_cfg.osd_tailor = AMBA_VOUT_OSD_NO_CSC | AMBA_VOUT_OSD_AUTO_COPY;
	}
	sink_cfg.id = sink_id;
	sink_cfg.frame_rate = fps;
	sink_cfg.csc_en = csc_en;
	sink_cfg.hdmi_color_space = hdmi_cs;
	sink_cfg.hdmi_3d_structure = hdmi_3d_structure;
	sink_cfg.hdmi_overscan = hdmi_overscan;
	sink_cfg.video_en = video_en;
	sink_cfg.mixer_csc = mixer_csc;

	switch (video_flip) {
	case FLIP_NORMAL:
		sink_cfg.video_flip = AMBA_VOUT_FLIP_NORMAL;
		break;
	case FLIP_HV:
		sink_cfg.video_flip = AMBA_VOUT_FLIP_HV;
		break;
	case FLIP_H:
		sink_cfg.video_flip = AMBA_VOUT_FLIP_HORIZONTAL;
		break;
	case FLIP_V:
		sink_cfg.video_flip = AMBA_VOUT_FLIP_VERTICAL;
		break;
	default:
		sink_cfg.video_flip = AMBA_VOUT_FLIP_NORMAL;
		break;
	}

	if (video_rotate) {
		sink_cfg.video_rotate = AMBA_VOUT_ROTATE_90;
	} else {
		sink_cfg.video_rotate = AMBA_VOUT_ROTATE_NORMAL;
	}

	sink_cfg.video_size = video_size;
	sink_cfg.video_offset = video_offset;
	sink_cfg.fb_id = fb_id;
	switch (osd_flip) {
	case FLIP_NORMAL:
		sink_cfg.osd_flip = AMBA_VOUT_FLIP_NORMAL;
		break;
	case FLIP_HV:
		sink_cfg.osd_flip = AMBA_VOUT_FLIP_HV;
		break;
	case FLIP_H:
		sink_cfg.osd_flip = AMBA_VOUT_FLIP_HORIZONTAL;
		break;
	case FLIP_V:
		sink_cfg.osd_flip = AMBA_VOUT_FLIP_VERTICAL;
		break;
	default:
		sink_cfg.osd_flip = AMBA_VOUT_FLIP_NORMAL;
		break;
	}
	sink_cfg.osd_rotate = AMBA_VOUT_ROTATE_NORMAL;
	sink_cfg.osd_rescale = osd_rescale;
	sink_cfg.osd_offset = osd_offset;
	sink_cfg.display_input = display_input;
	sink_cfg.direct_to_dsp = direct_to_dsp;

	if (ioctl(fd_iav, IAV_IOC_VOUT_CONFIGURE_SINK, &sink_cfg) < 0) {
		perror("IAV_IOC_VOUT_CONFIGURE_SINK");
		return -1;
	}

	printf("init_vout%d done\n", vout_id);
	return 0;
}

int post_init_vout()
{
	int sink_type, lcd_model, mode;

	sink_type	= vout_device_type[0];
	lcd_model	= vout_lcd_model_index[0];
	mode		= vout_mode[0];

	if (sink_type == AMBA_VOUT_SINK_TYPE_DIGITAL &&
		lcd_model_list[lcd_model].lcd_post_setmode != NULL) {
		if (lcd_model_list[lcd_model].lcd_post_setmode(mode) < 0) {
			perror("lcd_setmode fail.");
			return -1;
		}
	}

	return 0;
}


/*==============================================================================
  Dynamically change vout
==============================================================================*/
int dynamically_change_vout(void)
{
	//restart vout
	if (restart_vout_flag) {

		if (check_vout() < 0) {
			return -1;
		}

		if (vout_flag[VOUT_0]) {
			init_vout(VOUT_0, 0);
		}

		if (vout_flag[VOUT_1]) {
			init_vout(VOUT_1, 0);
		}

		return 1;
	}

	//halt vout if instructed
	if (vout_halt_flag) {
		if (vout_flag[VOUT_0]) {
			if (ioctl(fd_iav, IAV_IOC_VOUT_HALT, VOUT_0) < 0) {
				perror("IAV_IOC_VOUT_HALT");
				return -1;
			}
		}

		if (vout_flag[VOUT_1]) {
			if (ioctl(fd_iav, IAV_IOC_VOUT_HALT, VOUT_1) < 0) {
				perror("IAV_IOC_VOUT_HALT");
				return -1;
			}
		}

		return 1;
	}

	//enable or disable csc only
	if (enable_csc_flag) {
		iav_vout_enable_csc_t enable_csc;

		if (vout_flag[VOUT_0]) {
			enable_csc.vout_id = 0;
			enable_csc.csc_en = vout_csc_en[VOUT_0];
			if (ioctl(fd_iav, IAV_IOC_VOUT_ENABLE_CSC, &enable_csc)) {
				perror("IAV_IOC_VOUT_ENABLE_CSC of vout0");
				return -1;
			}
		}

		if (vout_flag[VOUT_1]) {
			enable_csc.vout_id = 1;
			enable_csc.csc_en = vout_csc_en[VOUT_1];
			if (ioctl(fd_iav, IAV_IOC_VOUT_ENABLE_CSC, &enable_csc)) {
				perror("IAV_IOC_VOUT_ENABLE_CSC of vout1");
				return -1;
			}
		}

		return 1;
	}

	//enable or disable video only
	if (enable_video_flag) {
		iav_vout_enable_video_t enable_video;

		if (vout_flag[VOUT_0]) {
			enable_video.vout_id = 0;
			enable_video.video_en = vout_video_en[VOUT_0];
			if (ioctl(fd_iav, IAV_IOC_VOUT_ENABLE_VIDEO, &enable_video)) {
				perror("IAV_IOC_VOUT_ENABLE_VIDEO of vout0");
				return -1;
			}
		}

		if (vout_flag[VOUT_1]) {
			enable_video.vout_id = 1;
			enable_video.video_en = vout_video_en[VOUT_1];
			if (ioctl(fd_iav, IAV_IOC_VOUT_ENABLE_VIDEO, &enable_video)) {
				perror("IAV_IOC_VOUT_ENABLE_VIDEO of vout1");
				return -1;
			}
		}

		return 1;
	}

	//rotate video only
	if (rotate_video_flag) {
		iav_vout_rotate_video_t rotate_video;

		if (vout_flag[VOUT_0]) {
			rotate_video.vout_id = 0;
			rotate_video.rotate = vout_video_rotate[VOUT_0];
			if (ioctl(fd_iav, IAV_IOC_VOUT_ROTATE_VIDEO, &rotate_video)) {
				perror("IAV_IOC_VOUT_ROTATE_VIDEO of vout0");
				return -1;
			}
		}

		if (vout_flag[VOUT_1]) {
			rotate_video.vout_id = 1;
			rotate_video.rotate = vout_video_rotate[VOUT_1];
			if (ioctl(fd_iav, IAV_IOC_VOUT_ROTATE_VIDEO, &rotate_video)) {
				perror("IAV_IOC_VOUT_ROTATE_VIDEO of vout1");
				return -1;
			}
		}

		return 1;
	}

	//flip video only
	if (flip_video_flag) {
		iav_vout_flip_video_t flip_video;

		if (vout_flag[VOUT_0]) {
			flip_video.vout_id = 0;
			flip_video.flip = vout_video_flip[VOUT_0];
			if (ioctl(fd_iav, IAV_IOC_VOUT_FLIP_VIDEO, &flip_video)) {
				perror("IAV_IOC_VOUT_FLIP_VIDEO of vout0");
				return -1;
			}
		}

		if (vout_flag[VOUT_1]) {
			flip_video.vout_id = 1;
			flip_video.flip = vout_video_flip[VOUT_1];
			if (ioctl(fd_iav, IAV_IOC_VOUT_FLIP_VIDEO, &flip_video)) {
				perror("IAV_IOC_VOUT_FLIP_VIDEO of vout1");
				return -1;
			}
		}

		return 1;
	}

	//Change video size only
	if (change_video_size_flag) {
		iav_vout_change_video_size_t cvs;

		if (vout_flag[VOUT_0]) {
			if (vout_video_size[VOUT_0].specified) {
				cvs.vout_id = 0;
				cvs.width = vout_video_size[VOUT_0].video_width;
				cvs.height = vout_video_size[VOUT_0].video_height;
				if (ioctl(fd_iav, IAV_IOC_VOUT_CHANGE_VIDEO_SIZE, &cvs)) {
					perror("IAV_IOC_VOUT_CHANGE_VIDEO_SIZE of vout0");
					return -1;
				}
			} else {
				printf("Please specify right video size for vout0.\n");
			}
		}

		if (vout_flag[VOUT_1]) {
			if (vout_video_size[VOUT_1].specified) {
				cvs.vout_id = 1;
				cvs.width = vout_video_size[VOUT_1].video_width;
				cvs.height = vout_video_size[VOUT_1].video_height;
				if (ioctl(fd_iav, IAV_IOC_VOUT_CHANGE_VIDEO_SIZE, &cvs)) {
					perror("IAV_IOC_VOUT_CHANGE_VIDEO_SIZE of vout1");
					return -1;
				}
			} else {
				printf("Please specify right video size for vout1.\n");
			}
		}

		return 1;
	}

	//Change video offset only
	if (change_video_offset_flag) {
		iav_vout_change_video_offset_t cvo;

		if (vout_flag[VOUT_0]) {
			cvo.vout_id = 0;
			cvo.specified = vout_video_offset[VOUT_0].specified;
			cvo.offset_x = vout_video_offset[VOUT_0].offset_x;
			cvo.offset_y = vout_video_offset[VOUT_0].offset_y;
			if (!cvo.specified)
				printf("No video offset specified for vout0, place it in the middle of the window!\n");

			if (ioctl(fd_iav, IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET, &cvo)) {
				perror("IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET of vout0");
				return -1;
			}
		}

		if (vout_flag[VOUT_1]) {
			cvo.vout_id = 1;
			cvo.specified = vout_video_offset[VOUT_1].specified;
			cvo.offset_x = vout_video_offset[VOUT_1].offset_x;
			cvo.offset_y = vout_video_offset[VOUT_1].offset_y;
			if (!cvo.specified)
				printf("No video offset specified for vout1, place it in the middle of the window!\n");

			if (ioctl(fd_iav, IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET, &cvo)) {
				perror("IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET of vout1");
				return -1;
			}
		}

		return 1;
	}

	//select frame buffer only
	if (select_fb_flag) {
		iav_vout_fb_sel_t fb_sel;

		if (vout_flag[VOUT_0]) {
			fb_sel.vout_id = 0;
			fb_sel.fb_id = vout_fb_id[VOUT_0];
			if (ioctl(fd_iav, IAV_IOC_VOUT_SELECT_FB, &fb_sel)) {
				perror("Change osd of vout0");
				return -1;
			}
		}

		if (vout_flag[VOUT_1]) {
			fb_sel.vout_id = 1;
			fb_sel.fb_id = vout_fb_id[VOUT_1];
			if (ioctl(fd_iav, IAV_IOC_VOUT_SELECT_FB, &fb_sel)) {
				perror("Change osd of vout1");
				return -1;
			}
		}

		return 1;
	}

	//flip video only
	if (flip_osd_flag) {
		iav_vout_flip_osd_t flip_osd;

		if (vout_flag[VOUT_0]) {
			flip_osd.vout_id = 0;
			flip_osd.flip = vout_osd_flip[VOUT_0];
			if (ioctl(fd_iav, IAV_IOC_VOUT_FLIP_OSD, &flip_osd)) {
				perror("IAV_IOC_VOUT_FLIP_OSD of vout0");
				return -1;
			}
		}

		if (vout_flag[VOUT_1]) {
			flip_osd.vout_id = 1;
			flip_osd.flip = vout_osd_flip[VOUT_1];
			if (ioctl(fd_iav, IAV_IOC_VOUT_FLIP_OSD, &flip_osd)) {
				perror("IAV_IOC_VOUT_FLIP_OSD of vout1");
				return -1;
			}
		}

		return 1;
	}

	//enable or disable OSD rescaler only
	if (rescale_osd_flag) {
		iav_vout_enable_osd_rescaler_t enable_rescaler;

		if (vout_flag[VOUT_0] && vout_flag[VOUT_1]) {
			if (vout_osd_rescale[VOUT_1].enable && vout_osd_rescale[VOUT_0].enable) {
				printf("Error: Can not enable OSD rescaler for both vouts at the same time!\n");
				return -1;
			}
		}

		if (vout_flag[VOUT_0]) {
			enable_rescaler.vout_id = 0;
			enable_rescaler.enable = vout_osd_rescale[VOUT_0].enable;
			enable_rescaler.width = vout_osd_rescale[VOUT_0].width;
			enable_rescaler.height = vout_osd_rescale[VOUT_0].height;
			if (ioctl(fd_iav, IAV_IOC_VOUT_ENABLE_OSD_RESCALER, &enable_rescaler)) {
				perror("IAV_IOC_VOUT_ENABLE_RESCALER of vout0");
				return -1;
			}
		}

		if (vout_flag[VOUT_1]) {
			enable_rescaler.vout_id = 1;
			enable_rescaler.enable = vout_osd_rescale[VOUT_1].enable;
			enable_rescaler.width = vout_osd_rescale[VOUT_1].width;
			enable_rescaler.height = vout_osd_rescale[VOUT_1].height;
			if (ioctl(fd_iav, IAV_IOC_VOUT_ENABLE_OSD_RESCALER, &enable_rescaler)) {
				perror("IAV_IOC_VOUT_ENABLE_RESCALER of vout1");
				return -1;
			}
		}

		return 1;
	}

	//Change osd offset only
	if (change_osd_offset_flag) {
		iav_vout_change_osd_offset_t coo;

		if (vout_flag[VOUT_0]) {
			coo.vout_id = 0;
			coo.specified = vout_osd_offset[VOUT_0].specified;
			coo.offset_x = vout_osd_offset[VOUT_0].offset_x;
			coo.offset_y = vout_osd_offset[VOUT_0].offset_y;
			if (!coo.specified)
				printf("No osd offset specified for vout0, place it in the middle of the window\n");

			if (ioctl(fd_iav, IAV_IOC_VOUT_CHANGE_OSD_OFFSET, &coo)) {
				perror("IAV_IOC_VOUT_CHANGE_OSD_OFFSET of vout0");
				return -1;
			}
		}

		if (vout_flag[VOUT_1]) {
			coo.vout_id = 1;
			coo.specified = vout_osd_offset[VOUT_1].specified;
			coo.offset_x = vout_osd_offset[VOUT_1].offset_x;
			coo.offset_y = vout_osd_offset[VOUT_1].offset_y;
			if (!coo.specified)
				printf("No osd offset specified for vout1, place it in the middle of the window\n");

			if (ioctl(fd_iav, IAV_IOC_VOUT_CHANGE_OSD_OFFSET, &coo)) {
				perror("IAV_IOC_VOUT_CHANGE_OSD_OFFSET of vout1");
				return -1;
			}
		}

		return 1;
	}

	return 0;
}
