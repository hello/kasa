/*
 * test_vin.c
 *
 * History:
 *	2009/11/17 - [Qiao Wang] create
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )

extern int fd_iav;

int vin_flag		= 0;
int vin_mode		= 0;

int vin_framerate_flag	= 0;
int vin_framerate		= 0;

int mirror_pattern	= VINDEV_MIRROR_AUTO;
int bayer_pattern	= VINDEV_BAYER_PATTERN_AUTO;

int channel = -1;
int source = -1; //not specify source

int check_input = -1;

int hdr_mode	= AMBA_VIDEO_LINEAR_MODE;

int bits = AMBA_VIDEO_BITS_AUTO;

struct vindev_mirror mirror_mode;

struct vin_mode_s {
	const char *name;
	enum amba_video_mode mode;
} ;

struct vin_mode_s __vin_modes[] = {
	{"0",		AMBA_VIDEO_MODE_AUTO	},
	{"auto",	AMBA_VIDEO_MODE_AUTO	},

	{"480i",	AMBA_VIDEO_MODE_480I	},
	{"576i",	AMBA_VIDEO_MODE_576I	},
	{"480p",	AMBA_VIDEO_MODE_D1_NTSC	},
	{"576p",	AMBA_VIDEO_MODE_D1_PAL	},
	{"720p",	AMBA_VIDEO_MODE_720P	},
	{"1080i",	AMBA_VIDEO_MODE_1080I	},
	{"1080p",	AMBA_VIDEO_MODE_1080P	},	//1080p, max 60fps mode

	{"d1",		AMBA_VIDEO_MODE_D1_NTSC	},
	{"d1_pal",	AMBA_VIDEO_MODE_D1_PAL	},

	{"qvga",	AMBA_VIDEO_MODE_320_240	},
	{"vga",		AMBA_VIDEO_MODE_VGA	},
	{"wvga",	AMBA_VIDEO_MODE_WVGA	},
	{"wsvga",	AMBA_VIDEO_MODE_WSVGA	},

	{"qxga",	AMBA_VIDEO_MODE_QXGA	},
	{"xga",		AMBA_VIDEO_MODE_XGA	},
	{"wxga",	AMBA_VIDEO_MODE_WXGA	},
	{"uxga",	AMBA_VIDEO_MODE_UXGA	},
	{"qsxga",	AMBA_VIDEO_MODE_QSXGA	},
	{"qhd",   AMBA_VIDEO_MODE_2560_1440 },

	{"3m",		AMBA_VIDEO_MODE_QXGA	},
	{"5m",		AMBA_VIDEO_MODE_QSXGA	},

	{"1024x768",	AMBA_VIDEO_MODE_XGA	},
	{"1152x648",	AMBA_VIDEO_MODE_1152_648	},
	{"1280x720",	AMBA_VIDEO_MODE_720P	},
	{"1280x960",	AMBA_VIDEO_MODE_1280_960},
	{"1280x1024",	AMBA_VIDEO_MODE_SXGA	},
	{"1600x1200",	AMBA_VIDEO_MODE_UXGA	},
	{"1920x1200",	AMBA_VIDEO_MODE_WUXGA	},
	{"1920x1440",	AMBA_VIDEO_MODE_1920_1440},
	{"1920x1080",	AMBA_VIDEO_MODE_1080P	},
	{"2048x1080",	AMBA_VIDEO_MODE_2048_1080},
	{"2048x1152",	AMBA_VIDEO_MODE_2048_1152},
	{"2048x1536",	AMBA_VIDEO_MODE_QXGA	},
	{"2560x1440",	AMBA_VIDEO_MODE_2560_1440	},
	{"2592x1944",	AMBA_VIDEO_MODE_QSXGA	},
	{"2208x1242",	AMBA_VIDEO_MODE_2208_1242	},
	{"2304x1296",	AMBA_VIDEO_MODE_2304_1296	},
	{"2304x1536",	AMBA_VIDEO_MODE_2304_1536	},
	{"2304x1728",	AMBA_VIDEO_MODE_2304_1728	},
	{"3280x2464",	AMBA_VIDEO_MODE_3280_2464	},
	{"3280x1852",	AMBA_VIDEO_MODE_3280_1852	},
	{"2520x1424",	AMBA_VIDEO_MODE_2520_1424	},
	{"1640x1232",	AMBA_VIDEO_MODE_1640_1232	},
	{"4096x2160",	AMBA_VIDEO_MODE_4096_2160	},
	{"4016x3016",	AMBA_VIDEO_MODE_4016_3016	},
	{"4000x3000",	AMBA_VIDEO_MODE_4000_3000	},
	{"3840x2160",	AMBA_VIDEO_MODE_3840_2160	},
	{"3072x2048",	AMBA_VIDEO_MODE_3072_2048	},
	{"2688x1512",	AMBA_VIDEO_MODE_2688_1512	},
	{"2688x1520",	AMBA_VIDEO_MODE_2688_1520	},
	{"1296x1032",	AMBA_VIDEO_MODE_1296_1032	},
	{"2560x2048",	AMBA_VIDEO_MODE_2560_2048	},
	{"848x480",	AMBA_VIDEO_MODE_848_480	},
	{"3072x1728",	AMBA_VIDEO_MODE_3072_1728	},
	{"poweroff",	AMBA_VIDEO_MODE_OFF	},


	{"",		AMBA_VIDEO_MODE_AUTO	},
};


/***************************************************************
	VIN command line options
****************************************************************/
#define	VIN_NUMVERIC_SHORT_OPTIONS				\
	SPECIFY_NO_VIN_CHECK	=	VIN_OPTIONS_BASE,	\
	SPECIFY_MIRROR_PATTERN,							\
	SPECIFY_BAYER_PATTERN,							\
	SPECIFY_HDR_PATTERN,							\
	SPECIFY_BITS

#define	NO_VIN_CHECK_OPTIONS	\
	{"no-vin-check",		NO_ARG,		0,	SPECIFY_NO_VIN_CHECK	},
#define	NO_VIN_CHECK_HINTS	\
	{"",	"\tdo not check all vin format" },

#define	MIRROR_PATTERN_OPTIONS	\
	{"mirror-pattern",		HAS_ARG,	0,	SPECIFY_MIRROR_PATTERN,	},
#define	MIRROR_PATTERN_HINTS	\
	{"0~3",	"set vin mirror pattern" },

#define	BAYER_PATTERN_OPTIONS	\
	{"bayer-pattern",		HAS_ARG,	0,	SPECIFY_BAYER_PATTERN,	},
#define	BAYER_PATTERN_HINTS		\
	{"0~3",	"set vin bayer pattern" },

#define	HDR_MODE_OPTIONS		\
	{"hdr-mode",		HAS_ARG,	0,	SPECIFY_HDR_PATTERN,	},
#define	HDR_MODE_HINTS		\
	{"0~15",	"\tSet hdr mode. 0 for linear, 1~3 for 2X / 3X / 4X HDR, 15 for sensor built-in WDR." },

#define	BITS_OPTIONS	\
	{"bits",		HAS_ARG,	0,	SPECIFY_BITS,	},
#define	BITS_HINTS		\
	{"0~16",	"\tset bits. 0 for AUTO, 8~16 for special bits." },

#define	VIN_LONG_OPTIONS()		\
	{"vin",			HAS_ARG,	0,	'i' },		\
	{"src",			HAS_ARG,	0,	'S' },		\
	{"ch",			HAS_ARG,	0,	'c' },		\
	{"frame-rate",	HAS_ARG,	0,	'f' },		\
	NO_VIN_CHECK_OPTIONS		\
	MIRROR_PATTERN_OPTIONS		\
	BAYER_PATTERN_OPTIONS		\
	HDR_MODE_OPTIONS			\
	BITS_OPTIONS

#define	VIN_PARAMETER_HINTS()		\
	{"vin mode",		"\tchange vin mode" },		\
	{"source",		"\tselect source" },		\
	{"channel",		"\tselect channel" },		\
	{"frate",			"\tset VIN frame rate" },		\
	NO_VIN_CHECK_HINTS			\
	MIRROR_PATTERN_HINTS		\
	BAYER_PATTERN_HINTS			\
	HDR_MODE_HINTS				\
	BITS_HINTS

#define	VIN_INIT_PARAMETERS()		\
	case 'i':		\
		vin_flag = 1;				\
		vin_mode = get_vin_mode(optarg);	\
		if (vin_mode < 0)			\
			return -1;			\
		break;					\
	case 'S':		\
		source = atoi(optarg);		\
		break;					\
	case 'c':		\
		channel = atoi(optarg);	\
		break;					\
	case 'f':		\
		vin_framerate_flag = 1;		\
		vin_framerate = atoi(optarg);	\
		switch (vin_framerate) {		\
		case 0:					\
			vin_framerate = AMBA_VIDEO_FPS_AUTO;	\
			break;				\
		case 0x10000:			\
			vin_framerate = AMBA_VIDEO_FPS_29_97;	\
			break;				\
		case 0x10001:			\
			vin_framerate = AMBA_VIDEO_FPS_59_94;	\
			break;				\
		case 0x10003:			\
			vin_framerate = AMBA_VIDEO_FPS_12_5;	\
			break;				\
		case 0x10006:			\
			vin_framerate = AMBA_VIDEO_FPS_7_5;		\
			break;				\
		default:					\
			vin_framerate = DIV_ROUND(512000000, vin_framerate);	\
			break;				\
		}						\
		break;					\
	case SPECIFY_NO_VIN_CHECK:	\
		check_input = 0;			\
		break;					\
	case SPECIFY_MIRROR_PATTERN:	\
		mirror_pattern = atoi(optarg);	\
		if (mirror_pattern < 0)	\
			return -1;			\
		mirror_mode.pattern = mirror_pattern;		\
		break;					\
	case SPECIFY_BAYER_PATTERN:		\
		bayer_pattern = atoi(optarg);	\
		if (bayer_pattern < 0)	\
			return -1;			\
		mirror_mode.bayer_pattern = bayer_pattern;	\
		break;					\
	case SPECIFY_HDR_PATTERN:	\
		hdr_mode = atoi(optarg);	\
		if ((hdr_mode < AMBA_VIDEO_MODE_FIRST) || (hdr_mode >= AMBA_VIDEO_MODE_LAST))	\
			return -1;			\
		break;					\
	case SPECIFY_BITS:	\
		bits = atoi(optarg);	\
		if ((bits < AMBA_VIDEO_BITS_AUTO) || (bits > AMBA_VIDEO_BITS_16))	\
			return -1;			\
		break;


/*************************************************************************
	Initialize VIN
*************************************************************************/
int select_channel(void)
{
	return -1;
}

enum amba_video_mode get_vin_mode(const char *mode)
{
	int i;

	for (i = 0; i < sizeof (__vin_modes) / sizeof (__vin_modes[0]); i++)
		if (strcmp(__vin_modes[i].name, mode) == 0)
			return __vin_modes[i].mode;

	printf("vin mode '%s' not found\n", mode);
	return -1;
}

/*
 * The function is used to set 3A data, only for sensor that output RGB raw data
 */
static int set_vin_param(void)
{
	struct vindev_shutter vsrc_shutter;
	struct vindev_agc vsrc_agc;
	int vin_eshutter_time = 60;	// 1/60 sec
	int vin_agc_db = 6;	// 6dB

	u32 shutter_time_q9;
	s32 agc_db;

	shutter_time_q9 = 512000000/vin_eshutter_time;
	agc_db = vin_agc_db<<24;

	vsrc_shutter.vsrc_id = 0;
	vsrc_shutter.shutter = shutter_time_q9;
	if (ioctl(fd_iav, IAV_IOC_VIN_SET_SHUTTER, &vsrc_shutter) < 0) {
		perror("IAV_IOC_VIN_SET_SHUTTER");
		return -1;
	}

	vsrc_agc.vsrc_id = 0;
	vsrc_agc.agc = agc_db;
	if (ioctl(fd_iav, IAV_IOC_VIN_SET_AGC, &vsrc_agc) < 0) {
		perror("IAV_IOC_VIN_SET_AGC");
		return -1;
	}

	return 0;
}

static int set_vin_frame_rate(void)
{
	struct vindev_fps vsrc_fps;

	vsrc_fps.vsrc_id = 0;
	vsrc_fps.fps = vin_framerate;
	//printf("set framerate %d\n", framerate);
	if (ioctl(fd_iav, IAV_IOC_VIN_SET_FPS, &vsrc_fps) < 0) {
		perror("IAV_IOC_VIN_SET_FPS");
		return -1;
	}
	return 0;
}

/* (0 < index < AMBA_VIDEO_MODE_NUM) will change, use AMBA_VIDEO_MODE_XXX when
   you know the format, index2mode is mainly for the app to scan all
   possiable mode in a simple for/while loop.*/
static inline enum amba_video_mode amba_video_mode_index2mode(int index)
{
	enum amba_video_mode mode = AMBA_VIDEO_MODE_MAX;

	if (index < 0)
		goto amba_video_mode_index2mode_exit;


	if ((index >= 0) && (index < AMBA_VIDEO_MODE_MISC_NUM)) {
		mode = (enum amba_video_mode)index;
		goto amba_video_mode_index2mode_exit;
	}

       if ((index >= AMBA_VIDEO_MODE_MISC_NUM) && (index < AMBA_VIDEO_MODE_NUM)) {
               mode = (enum amba_video_mode)(AMBA_VIDEO_MODE_480I + (index -
                       AMBA_VIDEO_MODE_MISC_NUM));
               goto amba_video_mode_index2mode_exit;
       }

amba_video_mode_index2mode_exit:
	return mode;
}

static int change_fps_to_hz(u32 fps_q9, u32 *fps_hz, char *fps)
{
	u32				hz = 0;

	switch(fps_q9) {
	case AMBA_VIDEO_FPS_AUTO:
		snprintf(fps, 32, "%s", "AUTO");
		break;
	case AMBA_VIDEO_FPS_29_97:
		snprintf(fps, 32, "%s", "29.97");
		break;
	case AMBA_VIDEO_FPS_59_94:
		snprintf(fps, 32, "%s", "59.94");
		break;
	default:
		hz = DIV_ROUND(512000000, fps_q9);
		snprintf(fps, 32, "%d", hz);
		break;
	}

	if (fps_hz) {
		*fps_hz = hz;
	}

	return 0;
}

void test_print_video_info( const struct amba_video_info *pvinfo, int active_flag)
{
	char				format[32];
	char				hdrmode[32];
	char				fps[32];
	char				type[32];
	char				bits[32];
	char				ratio[32];
	char				system[32];
	u32				fps_q9 = 0;
	struct vindev_fps active_fps;

	switch(pvinfo->format) {

	case AMBA_VIDEO_FORMAT_PROGRESSIVE:
		snprintf(format, 32, "%s", "P");
		break;
	case AMBA_VIDEO_FORMAT_INTERLACE:
		snprintf(format, 32, "%s", "I");
		break;
	case AMBA_VIDEO_FORMAT_AUTO:
		snprintf(format, 32, "%s", "Auto");
		break;
	default:
		snprintf(format, 32, "format?%d", pvinfo->format);
		break;
	}

	switch(pvinfo->hdr_mode) {

	case AMBA_VIDEO_2X_HDR_MODE:
		snprintf(hdrmode, 32, "%s", "(HDR 2x)");
		break;
	case AMBA_VIDEO_3X_HDR_MODE:
		snprintf(hdrmode, 32, "%s", "(HDR 3x)");
		break;
	case AMBA_VIDEO_4X_HDR_MODE:
		snprintf(hdrmode, 32, "%s", "(HDR 4x)");
		break;
	case AMBA_VIDEO_INT_HDR_MODE:
		snprintf(hdrmode, 32, "%s", "(WDR In)");
		break;
	case AMBA_VIDEO_LINEAR_MODE:
	default:
		snprintf(hdrmode, 32, "%s", "");
		break;
	}

	if (active_flag) {
		memset(&active_fps, 0, sizeof(active_fps));
		active_fps.vsrc_id = 0;
		if (ioctl(fd_iav, IAV_IOC_VIN_GET_FPS, &active_fps) < 0) {
			perror("IAV_IOC_VIN_GET_FPS");
		}
		fps_q9 = active_fps.fps;
	} else {
		fps_q9 = pvinfo->max_fps;
	}
	change_fps_to_hz(fps_q9, NULL, fps);

	switch(pvinfo->type) {
	case AMBA_VIDEO_TYPE_RGB_RAW:
		snprintf(type, 32, "%s", "RGB");
		break;
	case AMBA_VIDEO_TYPE_YUV_601:
		snprintf(type, 32, "%s", "YUV BT601");
		break;
	case AMBA_VIDEO_TYPE_YUV_656:
		snprintf(type, 32, "%s", "YUV BT656");
		break;
	case AMBA_VIDEO_TYPE_YUV_BT1120:
		snprintf(type, 32, "%s", "YUV BT1120");
		break;
	case AMBA_VIDEO_TYPE_RGB_601:
		snprintf(type, 32, "%s", "RGB BT601");
		break;
	case AMBA_VIDEO_TYPE_RGB_656:
		snprintf(type, 32, "%s", "RGB BT656");
		break;
	case AMBA_VIDEO_TYPE_RGB_BT1120:
		snprintf(type, 32, "%s", "RGB BT1120");
		break;
	default:
		snprintf(type, 32, "type?%d", pvinfo->type);
		break;
	}

	switch(pvinfo->bits) {
	case AMBA_VIDEO_BITS_AUTO:
		snprintf(bits, 32, "%s", "Bits Not Availiable");
		break;
	default:
		snprintf(bits, 32, "%dbits", pvinfo->bits);
		break;
	}

	switch(pvinfo->ratio) {
	case AMBA_VIDEO_RATIO_AUTO:
		snprintf(ratio, 32, "%s", "AUTO");
		break;
	case AMBA_VIDEO_RATIO_4_3:
		snprintf(ratio, 32, "%s", "4:3");
		break;
	case AMBA_VIDEO_RATIO_16_9:
		snprintf(ratio, 32, "%s", "16:9");
		break;
	default:
		snprintf(ratio, 32, "ratio?%d", pvinfo->ratio);
		break;
	}

	switch(pvinfo->system) {
	case AMBA_VIDEO_SYSTEM_AUTO:
		snprintf(system, 32, "%s", "AUTO");
		break;
	case AMBA_VIDEO_SYSTEM_NTSC:
		snprintf(system, 32, "%s", "NTSC");
		break;
	case AMBA_VIDEO_SYSTEM_PAL:
		snprintf(system, 32, "%s", "PAL");
		break;
	case AMBA_VIDEO_SYSTEM_SECAM:
		snprintf(system, 32, "%s", "SECAM");
		break;
	case AMBA_VIDEO_SYSTEM_ALL:
		snprintf(system, 32, "%s", "ALL");
		break;
	default:
		snprintf(system, 32, "system?%d", pvinfo->system);
		break;
	}

	printf("\t%dx%d%s%s\t%s\t%s\t%s\t%s\t%s\trev[%d]\n",
		pvinfo->width, pvinfo->height,
		format, hdrmode, fps, type, bits, ratio, system, pvinfo->rev);
}

void check_source_info(void)
{
	char					format[32];
	u32					i, j, k;
	struct vindev_devinfo			vsrc_info;
	struct vindev_video_info		video_info;

	vsrc_info.vsrc_id = 0;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_DEVINFO, &vsrc_info) < 0) {
		perror("IAV_IOC_VIN_GET_DEVINFO");
		return;
	}

	if (source < 0)
		source = vsrc_info.vsrc_id;

	printf("\nFind Vin Source %s ", vsrc_info.name);

	if (vsrc_info.dev_type == VINDEV_TYPE_SENSOR) {
		printf("it supports:\n");

		for (i = 0; i < AMBA_VIDEO_MODE_NUM; i++) {
			for (j = AMBA_VIDEO_MODE_FIRST; j < AMBA_VIDEO_MODE_LAST; j++) {
				video_info.vsrc_id = 0;
				video_info.info.hdr_mode = j;
				video_info.info.mode = amba_video_mode_index2mode(i);
				for (k = AMBA_VIDEO_BITS_8; k <= AMBA_VIDEO_BITS_16; k += 2) {
					if (i == AMBA_VIDEO_MODE_AUTO)
						video_info.info.bits = AMBA_VIDEO_BITS_AUTO;
					else
						video_info.info.bits = k;

					if (ioctl(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
						if (errno == EINVAL)
							continue;

						perror("IAV_IOC_VIN_GET_VIDEOINFO");
						break;
					}

					test_print_video_info(&video_info.info, 0);

					if (i == AMBA_VIDEO_MODE_AUTO)
						break;
				}
				if (i == AMBA_VIDEO_MODE_AUTO)
					break;
			}
		}
	} else {
		switch(vsrc_info.sub_type) {
		case VINDEV_SUBTYPE_CVBS:
			snprintf(format, 32, "%s", "CVBS");
			break;
		case VINDEV_SUBTYPE_SVIDEO:
			snprintf(format, 32, "%s", "S-Video");
			break;
		case VINDEV_SUBTYPE_YPBPR:
			snprintf(format, 32, "%s", "YPbPr");
			break;
		case VINDEV_SUBTYPE_HDMI:
			snprintf(format, 32, "%s", "HDMI");
			break;
		case VINDEV_SUBTYPE_VGA:
			snprintf(format, 32, "%s", "VGA");
			break;
		default:
			snprintf(format, 32, "format?%d", vsrc_info.sub_type);
			break;
		}
		printf("Channel[%s]'s type is %s\n", vsrc_info.name, format);

		video_info.info.mode = AMBA_VIDEO_MODE_AUTO;
		video_info.vsrc_id = 0;
		if (ioctl(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
			perror("IAV_IOC_VIN_GET_VIDEOINFO");
			return;
		}

		if (video_info.info.height == 0) {
			printf("No signal yet.\n");
		} else {
			printf("The signal is:\n");
			test_print_video_info(&video_info.info, 0);
		}
	}
}

int init_vin(enum amba_video_mode video_mode, int hdr_mode)
{
	struct vindev_mode vsrc_mode;
	struct vindev_devinfo vsrc_info;
	struct vindev_video_info video_info;

	//IAV_IOC_VIN_GET_SOURCE_NUM

	check_source_info();

	vsrc_mode.vsrc_id = 0;
	vsrc_mode.video_mode = (unsigned int)video_mode;
	vsrc_mode.hdr_mode = hdr_mode;
	vsrc_mode.bits = bits;
	if (ioctl(fd_iav, IAV_IOC_VIN_SET_MODE, &vsrc_mode) < 0) {
		perror("IAV_IOC_VIN_SET_MODE 1111");
		return -1;
	}

	if (video_mode == AMBA_VIDEO_MODE_OFF) {
		printf("VIN is power off.\n");
	} else {
		if (mirror_pattern != VINDEV_MIRROR_AUTO || bayer_pattern != VINDEV_BAYER_PATTERN_AUTO) {
			mirror_mode.vsrc_id = 0;
			if (mirror_pattern == VINDEV_MIRROR_AUTO)
				mirror_mode.pattern = mirror_pattern;
			else if (bayer_pattern == VINDEV_BAYER_PATTERN_AUTO)
				mirror_mode.bayer_pattern = bayer_pattern;
			if (ioctl(fd_iav, IAV_IOC_VIN_SET_MIRROR, &mirror_mode) < 0) {
				perror("IAV_IOC_VIN_SET_MIRROR");
				return -1;
			}
		}

		vsrc_info.vsrc_id = 0;
		if (ioctl(fd_iav, IAV_IOC_VIN_GET_DEVINFO, &vsrc_info) < 0) {
			perror("IAV_IOC_VIN_GET_DEVINFO");
			return -1;
		}

		if (vsrc_info.dev_type == VINDEV_TYPE_SENSOR) {
			if (set_vin_param() < 0)	//set aaa here
				return -1;
			if (vin_framerate_flag) {
				if (set_vin_frame_rate()< 0)
					return -1;
			}
		}

		/* get current video info */
		video_info.vsrc_id = 0;
		video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
		if (ioctl(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
			perror("IAV_IOC_VIN_GET_VIDEOINFO");
			return -1;
		}

		printf("Active src %d's mode is:\n", source);
		test_print_video_info(&video_info.info, 1);
		printf("init_vin done\n");
	}

	return 0;
}

