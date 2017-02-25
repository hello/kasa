/*
 *
 * mw_config.c
 *
 * History:
 *	2010/02/28 - [Jian Tang] Created this file
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <unistd.h>

#include <basetypes.h>

#include "mw_defines.h"
#include "mw_struct.h"
#include "mw_api.h"
#include "mw_aaa_params.h"
#include "mw_pri_struct.h"

#define	MAX_ITEMS_TO_PARSE		(3*1000)
#define	FILE_CONTENT_SIZE		(128 * 1024)

extern _mw_global_config G_mw_config;

static Mapping AAA_Map[] = {
	{"saturation",			&G_mw_config.image_params.saturation,			MAP_TO_S32,	0.0,		MIN_MAX_LIMIT,		0.0,		255.0,	},
	{"brightness",			&G_mw_config.image_params.brightness,			MAP_TO_S32,	0.0,		MIN_MAX_LIMIT,		-255.0,		255.0,	},
	{"hue",				&G_mw_config.image_params.hue,				MAP_TO_S32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"contrast",			&G_mw_config.image_params.contrast,				MAP_TO_S32,	0.0,		MIN_MAX_LIMIT,		0.0,		128.0,	},
	{"sharpness",			&G_mw_config.image_params.sharpness,			MAP_TO_S32,	0.0,		MIN_MAX_LIMIT,		0.0,		11.0,	},

	{"exposure_mode",	&G_mw_config.ae_params.anti_flicker_mode,		MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"shutter_min",		&G_mw_config.ae_params.shutter_time_min,		MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"shutter_max",		&G_mw_config.ae_params.shutter_time_max,		MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"max_gain",			&G_mw_config.ae_params.sensor_gain_max,		MAP_TO_U16,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"slow_shutter",		&G_mw_config.ae_params.slow_shutter_enable,	MAP_TO_U32,	1.0,		NO_LIMIT,		0.0,		0.0,	},
	{"vin_fps",			&G_mw_config.ae_params.current_vin_fps,		MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"metering_mode",		&G_mw_config.ae_params.ae_metering_mode,			MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"ae_target_ratio",		&G_mw_config.ae_params.ae_level[0],			MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},

	{"sharpen_strength",		&G_mw_config.enh_params.sharpen_strength,			MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"mctf_strength",		&G_mw_config.enh_params.mctf_strength,			MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"dn_mode",			&G_mw_config.enh_params.dn_mode,			MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"chroma_noise_str",		&G_mw_config.enh_params.chroma_noise_str,			MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"backlight_comp",		&G_mw_config.enh_params.bl_compensation,	MAP_TO_U32,	0.0,		MIN_MAX_LIMIT,	0.0,		2.0,	},
	{"local_exposure",		&G_mw_config.enh_params.local_expo_mode,		MAP_TO_U32,	0.0,		MIN_MAX_LIMIT,	0.0,		5.0,	},
	{"auto_contrast",		&G_mw_config.enh_params.auto_contrast,			MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},
	{"auto_wdr_str",		&G_mw_config.enh_params.auto_wdr_str,			MAP_TO_U32,	0.0,		NO_LIMIT,		0.0,		0.0,	},

	{NULL,			NULL,						-1,	0.0,					0,	0.0,	0.0,		},
};


static int parse_items(char **items, char *buf, int bufsize)
{
	int item = 0;
	int InString = 0, InItem = 0;
	char *p = buf;
	char *bufend = &buf[bufsize];

	while (p < bufend) {
		switch (*p) {
		case 13:
			++p;
			break;

		case '#':					// Found comment
			*p = '\0';
			while ((*p != '\n') && (p < bufend))
				++p;
			InString = 0;
			InItem = 0;
			break;

		case '\n':
			InItem = 0;
			InString = 0;
			*p++ = '\0';
			break;

		case ' ':
		case '\t':					// Skip whitespace, leave state unchanged
			if (InString) {
				++p;
			} else {
				*p++ = '\0';
				InItem = 0;
			}
			break;

		case '"':					// Begin/End of String
			*p++ = '\0';
			if (!InString) {
				items[item++] = p;
				InItem = ~InItem;
			} else {
				InItem = 0;
			}
			InString = ~InString;	// Toggle
			break;

		default:
			if (!InItem) {
				items[item++] = p;
				InItem = ~InItem;
			}
			++p;
			break;
		}
	}

	return (item - 1);
}

static int parse_name_to_map_index(Mapping * Map, char * s)
{
	int i = 0;

	while (NULL != Map[i].TokenName) {
		if (0 == strcmp(Map[i].TokenName, s))
			return i;
		else
			++i;
	}

	return -1;
}

static void parse_content(Mapping * Map, char * buf, int bufsize)
{
	char		* items[MAX_ITEMS_TO_PARSE] = {NULL};
	int		MapIndex;
	int		item = 0;
	int		IntContent;
	u32		U32Content;
	double	DoubleContent;
	char		msg[256];
	int i;

	// Stage one: Generate an argc/argv-type list in items[], without comments and whitespace.
	// This is context insensitive and could be done most easily with lex(1).
	item = parse_items(items, buf, bufsize);

	memset(msg, 0, sizeof(msg));
	// Stage two: interpret the Value, context sensitive.
	for (i = 0; i < item; i += 3) {
		if (0 > (MapIndex = parse_name_to_map_index(Map, items[i]))) {
			continue;
		}
		if (strcmp("=", items[i+1])) {
			MW_ERROR(" Parsing error in config file: '=' expected as the second token in each line.\n");
			exit(0);
		}

		// Now interpret the Value, context sensitive...
		switch (Map[MapIndex].Type) {
		case MAP_TO_U32:						// Numerical, unsigned integer (u32)
			if (1 != sscanf(items[i+2], "%u", &U32Content)) {
				MW_ERROR("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (u32 *) (Map[MapIndex].Address) = U32Content;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_U16:						// Numerical, unsigned integer (u16)
			if (1 != sscanf(items[i+2], "%u", &U32Content)) {
				MW_ERROR("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (u16 *) (Map[MapIndex].Address) = U32Content;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_U8:							// Numerical, unsigned integer (u8)
			if (1 != sscanf(items[i+2], "%u", &U32Content)) {
				MW_ERROR("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (u8 *) (Map[MapIndex].Address) = U32Content;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_S32:						// Numerical, signed integer
			if (1 != sscanf(items[i+2], "%d", &IntContent)) {
				MW_ERROR("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (int *) (Map[MapIndex].Address) = IntContent;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_S16:						// Numerical, signed short
			if (1 != sscanf(items[i+2], "%d", &IntContent)) {
				MW_ERROR("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (s16 *) (Map[MapIndex].Address) = IntContent;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_S8:						// Numerical, signed char
			if (1 != sscanf(items[i+2], "%d", &IntContent)) {
				MW_ERROR("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (s8 *) (Map[MapIndex].Address) = IntContent;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_DOUBLE:					// Numerical, double
			if (1 != sscanf(items[i+2], "%lf", &DoubleContent)) {
				MW_ERROR("Parsing error: Expected numerical value for Parameter [%s], found [%s].\n",
					items[i], items[i+2]);
				exit(0);
			}
			* (double *) (Map[MapIndex].Address) = DoubleContent;
			sprintf(msg, "%s.", msg);
			break;

		case MAP_TO_STRING:						// String
			memset((char *) Map[MapIndex].Address, 0, Map[MapIndex].StringLengthLimit);
			if (NULL != items[i+2]) {
				strncpy((char *) Map[MapIndex].Address, items[i+2], Map[MapIndex].StringLengthLimit - 1);
			} else {
				memset((char *) Map[MapIndex].Address, 0, Map[MapIndex].StringLengthLimit);
			}
			sprintf(msg, "%s.", msg);
			break;

		default:
			MW_ERROR("== parse_content == Unknown value type in the map definition!\n");
			exit(0);
		}
	}
	MW_INFO("Parse parameters : %s\n", msg);
}

static int load_default_params(Mapping * Map)
{
	int i = 0;

	while (NULL != Map[i].TokenName) {
		switch (Map[i].Type) {
		case MAP_TO_U32:
			* (u32 *) (Map[i].Address) = (u32) ((int) Map[i].Default);
			break;
		case MAP_TO_U16:
			* (u16 *) (Map[i].Address) = (u16) ((int) Map[i].Default);
			break;
		case MAP_TO_U8:
			* (u8 *) (Map[i].Address) = (u8) ((int) Map[i].Default);
			break;
		case MAP_TO_S32:
			* (int *) (Map[i].Address) = (int) Map[i].Default;
			break;
		case MAP_TO_S16:
			* (s16 *) (Map[i].Address) = (s16) ((int) Map[i].Default);
			break;
		case MAP_TO_S8:
			* (s8 *) (Map[i].Address) = (s8) ((int) Map[i].Default);
			break;
		case MAP_TO_DOUBLE:
			* (double *) (Map[i].Address) = Map[i].Default;
			break;
		case MAP_TO_STRING:
			* (char *) (Map[i].Address) = '\0';
			break;
		default:
			MW_ERROR("== load_default_params == Unknown value type in the map definition!\n");
			exit(0);
		}
		++i;
	}

	return 0;
}

static int check_params(Mapping * Map)
{
	int i = 0;

	while (Map[i].TokenName != NULL) {
		switch (Map[i].Type) {
		case MAP_TO_U32:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(u32 *)(Map[i].Address) < (u32)(int)(Map[i].MinLimit)) ||
					(*(u32 *)(Map[i].Address) > (u32)(int)(Map[i].MaxLimit))) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%d, %d] range.\n"
						"Use default value [%d]\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].MaxLimit, (int)Map[i].Default);
					*(u32 *)(Map[i].Address) = (u32)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(u32 *)(Map[i].Address) < (u32)(int)(Map[i].MinLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].Default);
					*(u32 *)(Map[i].Address) = (u32)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(u32 *)(Map[i].Address) > (u32)(int)(Map[i].MaxLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MaxLimit,
						(int)Map[i].Default);
					*(u32 *)(Map[i].Address) = (u32)(int)(Map[i].Default);
				}
			}
			break;

		case MAP_TO_U16:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(u16 *)(Map[i].Address) < (u16)(int)(Map[i].MinLimit)) ||
					(*(u16 *)(Map[i].Address) > (u16)(int)(Map[i].MaxLimit))) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%d, %d] range.\n"
						"Use default value [%d]\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].MaxLimit, (int)Map[i].Default);
					*(u16 *)(Map[i].Address) = (u16)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(u16 *)(Map[i].Address) < (u16)(int)(Map[i].MinLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].Default);
					*(u16 *)(Map[i].Address) = (u16)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(u16 *)(Map[i].Address) > (u16)(int)(Map[i].MaxLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MaxLimit,
						(int)Map[i].Default);
					*(u16 *)(Map[i].Address) = (u16)(int)(Map[i].Default);
				}
			}
			break;

		case MAP_TO_U8:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(u8 *)(Map[i].Address) < (u8)(int)(Map[i].MinLimit)) ||
					(*(u8 *)(Map[i].Address) > (u8)(int)(Map[i].MaxLimit))) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%d, %d] range.\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].MaxLimit, (int)Map[i].Default);
					*(u8 *)(Map[i].Address) = (u8)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(u8 *)(Map[i].Address) < (u8)(int)(Map[i].MinLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].Default);
					*(u8 *)(Map[i].Address) = (u8)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(u8 *)(Map[i].Address) > (u8)(int)(Map[i].MaxLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MaxLimit,
						(int)Map[i].Default);
					*(u8 *)(Map[i].Address) = (u8)(int)(Map[i].Default);
				}
			}
			break;

		case MAP_TO_S32:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(int *)(Map[i].Address) < (int)(Map[i].MinLimit)) ||
					(*(int *)(Map[i].Address) > (int)(Map[i].MaxLimit))) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%d, %d] range.\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].MaxLimit, (int)Map[i].Default);
					*(int *)(Map[i].Address) = (int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(int *)(Map[i].Address) < (int)(Map[i].MinLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].Default);
					*(int *)(Map[i].Address) = (int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(int *)(Map[i].Address) > (int)(Map[i].MaxLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MaxLimit,
						(int)Map[i].Default);
					*(int *)(Map[i].Address) = (int)(Map[i].Default);
				}
			}
			break;

		case MAP_TO_S16:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(s16 *)(Map[i].Address) < (s16)(int)(Map[i].MinLimit)) ||
					(*(s16 *)(Map[i].Address) > (s16)(int)(Map[i].MaxLimit))) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%d, %d] range.\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].MaxLimit, (int)Map[i].Default);
					*(s16 *)(Map[i].Address) = (s16)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(s16 *)(Map[i].Address) < (s16)(int)(Map[i].MinLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].Default);
					*(s16 *)(Map[i].Address) = (s16)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(s16 *)(Map[i].Address) > (s16)(int)(Map[i].MaxLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MaxLimit,
						(int)Map[i].Default);
					*(s16 *)(Map[i].Address) = (s16)(int)(Map[i].Default);
				}
			}
			break;

		case MAP_TO_S8:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(s8 *)(Map[i].Address) < (s8)(int)(Map[i].MinLimit)) ||
					(*(s8 *)(Map[i].Address) > (s8)(int)(Map[i].MaxLimit))) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%d, %d] range.\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].MaxLimit, (int)Map[i].Default);
					*(s8 *)(Map[i].Address) = (s8)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(s8 *)(Map[i].Address) < (s8)(int)(Map[i].MinLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MinLimit,
						(int)Map[i].Default);
					*(s8 *)(Map[i].Address) = (s8)(int)(Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(s8 *)(Map[i].Address) > (s8)(int)(Map[i].MaxLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%d].\n"
						"Use default value [%d].\n",
						Map[i].TokenName, (int)Map[i].MaxLimit,
						(int)Map[i].Default);
					*(s8 *)(Map[i].Address) = (s8)(int)(Map[i].Default);
				}
			}
			break;

		case MAP_TO_DOUBLE:
			if (Map[i].ParamLimits == MIN_MAX_LIMIT) {
				if ((*(double *)(Map[i].Address) < (Map[i].MinLimit)) ||
					(*(double *)(Map[i].Address) > (Map[i].MaxLimit))) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be in [%.2lf, %.2lf] range.\n"
						"Use default value [%.2lf].\n",
						Map[i].TokenName, Map[i].MinLimit,
						Map[i].MaxLimit, Map[i].Default);
					*(double *)(Map[i].Address) = (Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MIN_LIMIT) {
				if (*(double *)(Map[i].Address) < (Map[i].MinLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be larger than [%.2lf].\n"
						"Use default value [%.2lf].\n",
						Map[i].TokenName, Map[i].MinLimit,
						Map[i].Default);
					*(double *)(Map[i].Address) = (Map[i].Default);
				}
			} else if (Map[i].ParamLimits == MAX_LIMIT) {
				if (*(double *)(Map[i].Address) > (Map[i].MaxLimit)) {
					MW_ERROR("Error in input parameter %s. Please "
						"check configuration file.\n"
						"Value should be smaller than [%.2lf].\n"
						"Use default value [%.2lf].\n",
						Map[i].TokenName, Map[i].MaxLimit,
						Map[i].Default);
					*(double *)(Map[i].Address) = (Map[i].Default);
				}
			}
			break;

		default:			// string
			break;
		}
		++i;
	}

	return 0;
}

static int update_params(Mapping * Map)
{
	check_params(Map);
//	memcpy(pInp, &cfg, sizeof(InputParameters));
	return 0;
}

static int configure(Mapping *Map, const char * content)
{
	if (NULL == content) {
		load_default_params(Map);
	} else {
		char *pContent = NULL;
		u32 contentSize = 0;
		contentSize = strlen(content);
		pContent = (char *) malloc(contentSize + 1);
		strncpy(pContent, content, contentSize);
		pContent[contentSize] = 0;
		parse_content(Map, pContent, contentSize);
		update_params(Map);
		free(pContent);
	}
	return 0;
}

/*************************************************
 *
 *		Public Functions, for external used
 *
 *************************************************/

static int load_params(Mapping *pMap, char * pContent)
{
	int retv = 0;
	if (NULL == pMap)
		return -1;
	retv = configure(pMap, pContent);
	return retv;
}

static int save_params(Mapping * Map, char ** pContentOut)
{
	char * content = *pContentOut;
	int	i = 0;
	u32	unsigned_value;
	int	signed_value;

	content[0] = '\0';
	while (NULL != Map[i].TokenName) {
		sprintf(content, "%s%s", content, Map[i].TokenName);
		switch (Map[i].Type) {
		case MAP_TO_U32:
			unsigned_value = * (u32 *) (Map[i].Address);
			sprintf(content, "%s = %u\n", content, unsigned_value);
			break;
		case MAP_TO_U16:
			unsigned_value = * (u16 *) (Map[i].Address);
			sprintf(content, "%s = %u\n", content, unsigned_value);
			break;
		case MAP_TO_U8:
			unsigned_value = * (u8 *) (Map[i].Address);
			sprintf(content, "%s = %u\n", content, unsigned_value);
			break;
		case MAP_TO_S32:
			signed_value = * (int *) (Map[i].Address);
			sprintf(content, "%s = %d\n", content, signed_value);
			break;
		case MAP_TO_S16:
			signed_value = * (s16 *) (Map[i].Address);
			sprintf(content, "%s = %d\n", content, signed_value);
			break;
		case MAP_TO_S8:
			signed_value = * (s8 *) (Map[i].Address);
			sprintf(content, "%s = %d\n", content, signed_value);
			break;
		case MAP_TO_DOUBLE:
			sprintf(content, "%s = %.2lf\n", content, * (double *) (Map[i].Address));
			break;
		case MAP_TO_STRING:
			sprintf(content, "%s = \"%s\"\n", content, (char *) (Map[i].Address));
			break;
		default:
			sprintf(content, "%s : [%d] unknown value type.\n", content, Map[i].Type);
			MW_ERROR("== Output Parameters == Unknown value type in the map definition !!!\n");
			break;
		}
		++i;
	}
	return 0;
}

static int printf_parse_msg(Mapping *map)
{
	if (map == NULL) return -1;

	MW_DEBUG("\n%s\t",map->TokenName);
	switch (map->Type) {
		case MAP_TO_U32:
			MW_DEBUG("\t%u\t", (u32)*(u32 **)map->Address);
			break;
		case MAP_TO_U16:
			MW_DEBUG("\t%u\t", (u32)*(u16 **)map->Address);
			break;
		case MAP_TO_U8:
			MW_DEBUG("\t%u\t", (u32)*(u8 **)map->Address);
			break;
		case MAP_TO_S32:
			MW_DEBUG("\t%d\t", (int)*(s32 **)map->Address);
			break;
		case MAP_TO_S16:
			MW_DEBUG("\t%d\t", (int)*(s16 **)map->Address);
			break;
		case MAP_TO_S8:
			MW_DEBUG("\t%d\t", (int)*(s8 **)map->Address);
			break;
		case MAP_TO_STRING:
			MW_DEBUG("\t%s\t", *(char **)map->Address);
			break;
		default:
			MW_DEBUG("No the type\n");
			return -1;
	}

	return 0;
}

int mw_save_config_file(char *filename)
{
	int file;
	char *buffer = NULL;


	buffer = malloc(FILE_CONTENT_SIZE);
	if (buffer == NULL) {
		MW_ERROR("Malloc buffer error\n");
		return -1;
	}
	memset(buffer, 0, FILE_CONTENT_SIZE);

	save_params(AAA_Map, &buffer);

	if ((file = open(filename, O_RDWR | O_CREAT, 0)) < 0) {
		MW_ERROR("Open file: %s error\n", filename);
		return -1;
	}

	if (write(file, buffer, strlen(buffer)) < 0) {
		printf("write error.\n");
		return -1;
	}
	if (buffer) {
		free(buffer);
		buffer = NULL;
	}
	if (file > 0) {
		close(file);
		file = -1;
	}
	return 0;
}

int mw_load_config_file(char *filename)
{
	int file, i;
	char *buffer = NULL;

	buffer = malloc(FILE_CONTENT_SIZE);
	if (buffer == NULL) {
		MW_ERROR("Malloc buffer error\n");
		return -1;
	}
	memset(buffer, 0, FILE_CONTENT_SIZE);

	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		MW_ERROR("Open file: %s error\n", filename);
		return -1;
	}
	if (read(file, buffer, FILE_CONTENT_SIZE) < 0) {
		printf("read error.\n");
		return -1;
	}
	load_params(AAA_Map, buffer);

	for(i = 0; i < sizeof(AAA_Map) / sizeof(Mapping); i++) {
		printf_parse_msg(&AAA_Map[i]);
	}

	G_mw_config.init_params.load_cfg_file = 1;

	if (buffer) {
		free(buffer);
		buffer = NULL;
	}

	if (file > 0) {
		close(file);
		file = -1;
	}

	return 0;
}

#define __END_OF_FILE__

