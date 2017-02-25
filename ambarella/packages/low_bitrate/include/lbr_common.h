/*******************************************************************************
 * lbr_common.h
 *
 * History:
 *   2014-02-17 - [Louis Sun] created file
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
 ******************************************************************************/

#ifndef __LBR_COMMON_H__
#define __LBR_COMMON_H__
//LBR library version
#include <basetypes.h>
typedef struct lbr_version_s
{
	int major;
	int minor;
	int patch;
	unsigned int mod_time;
	char description[64];
} lbr_version_t;

typedef enum
{
	LBR_MOTION_NONE = 0,
	LBR_MOTION_LOW = 1,
	LBR_MOTION_HIGH = 2, //worst case is default
} LBR_MOTION_LEVEL;

typedef enum
{
	LBR_NOISE_NONE = 0,
	LBR_NOISE_LOW = 1,
	LBR_NOISE_HIGH = 2, //worst case is default
} LBR_NOISE_LEVEL;

typedef enum
{
	LBR_STYLE_FPS_KEEP_BITRATE_AUTO = 0, //try to keep max fps, bitrate auto drop when possible (default, consumer style)
	LBR_STYLE_QUALITY_KEEP_FPS_AUTO_DROP = 1, //similar to default, but fps can auto drop when no budget to keep quality(Quality keeping)
	LBR_STYLE_FPS_KEEP_CBR_ALIKE = 2, //FPS keep, CBR alike. (traditional professional IPCAM style)
} LBR_STYLE;

typedef struct lbr_encode_config_s
{
	u16 width;
	u16 height;
	u16 M;
	u16 N;
} lbr_encode_config_t;

typedef enum
{
	LBR_PROFILE_STATIC = 0,
	LBR_PROFILE_SMALL_MOTION = 1,
	LBR_PROFILE_BIG_MOTION = 2,
	LBR_PROFILE_LOW_LIGHT = 3,
	LBR_PROFILE_BIG_MOTION_WITH_FRAME_DROP = 4,
	LBR_PROFILE_SECURIY_IPCAM_CBR = 5,
	LBR_PROFILE_RESERVE2 = 6,
	LBR_PROFILE_RESERVE3 = 7,
	LBR_PROFILE_NUM
} LBR_PROFILE_TYPE;

typedef struct lbr_init_s
{
	int fd_iav;
	int fd_vin;
} lbr_init_t;

typedef struct lbr_bitrate_case_target_s
{
	u32 bitrate_1080p; //if resolution is 1080p, choose this bitrate as lbr target
	u32 bitrate_720p; // if resolution is 720p, choose this bitrate as lbr target
	u32 bitrate_per_MB; // bitrate for every Macroblock, for example if 480p bitrate is 120Kbps, then this value is  91 bits per second
} lbr_bitrate_case_target_t;

typedef struct lbr_control_s
{
	lbr_bitrate_case_target_t bitrate_target;
	u8 zmv_threshold;
	u8 I_qp_reduce;
	u8 I_qp_limit_min;
	u8 I_qp_limit_max;
	u8 P_qp_limit_min;
	u8 P_qp_limit_max;
	u8 B_qp_limit_min;
	u8 B_qp_limit_max;
	u8 skip_frame_mode;
	u8 adapt_qp;
} lbr_control_t;

typedef struct lbr_bitrate_target_s
{
	u32 auto_target;      //if auto is 1, ignore other fields
	u32 bitrate_ceiling;  //useful when auto is 0
} lbr_bitrate_target_t;

typedef struct lbr_control_context_s
{
	//input, initial config
	lbr_encode_config_t encode_config;
	//input, on the fly update
	LBR_MOTION_LEVEL motion_level;
	LBR_NOISE_LEVEL noise_level;
	LBR_STYLE lbr_style;
	lbr_bitrate_target_t bitrate_target; //ceiling is a user experienced bitrate max. not equivalent to what DSP is set to
	//internal
	LBR_PROFILE_TYPE lbr_profile_type;
} lbr_control_context_t;

typedef struct lbr_profile_bitrate_gap_s
{
	//bitrate gap between big motin profile and small motion profile
	lbr_bitrate_case_target_t bm_sm;
	//bitrate gap between small motin profile and no motion(static) profile
	lbr_bitrate_case_target_t sm_nm;
} lbr_profile_bitrate_gap_t;

typedef struct lbr_profile_bitrate_gap_gop_s
{
	lbr_profile_bitrate_gap_t bg_IPPP;
	lbr_profile_bitrate_gap_t bg_IBBP;
} lbr_profile_bitrate_gap_gop_t;

#define LBR_MAX_STREAM_NUM    4
#define LBR_ENCODE_WIDTH_MIN  160
#define LBR_ENCODE_WIDTH_MAX  4096
#define LBR_ENCODE_HEIGHT_MIN 90
#define LBR_ENCODE_HEIGHT_MAX 2160
#define LBR_BITRATE_MIN       (32*1024)
#define LBR_BITRATE_MAX       (20*1024*1024)
#define LBR_GOP_LENGTH_MIN    10
#define LBR_GOP_LENGTH_MAX    240
#define LBR_GOP_LENGTH_PREFER 120
#define LBR_BITRATE_GAP_ADJ   1
//use 1080p30 1Mbps as the threshold to decide whether it's a high bitrate mode
#define LOW_BITRATE_CEILING_DEFAULT  (1024*1024)
#define HIGH_BITRATE_PER_MB_TRESHOLD (LOW_BITRATE_CEILING_DEFAULT/((1920*1088)/256))
#define LBR_VERSION_MAJOR 0
#define LBR_VERSION_MINOR 1

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg) \
		do {	\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);	\
				return -1;	\
			}	\
		} while (0);
#endif

#ifndef VERIFY_STREAM_ID
#define VERIFY_STREAM_ID(x) \
		do {	\
			if (((x) < 0) || ( (x) >= LBR_MAX_STREAM_NUM)) {	\
				printf("stream id out of range \n");	\
				return -1;	\
			}	\
		} while(0);
#endif

//for debug
enum lbr_log_level
{
	LBR_ERROR_LEVEL = 0,
	LBR_MSG_LEVEL = 1,
	LBR_INFO_LEVEL = 2,
	LBR_DEBUG_LEVEL = 3,
	LBR_LOG_LEVEL_NUM = 4
};

extern int G_lbr_log_level;
#define LBR_PRINT(mylog, LOG_LEVEL, format, args...) \
		do {	\
			if (mylog >= LOG_LEVEL) {	\
				printf(format, ##args);	\
			}	\
		} while (0)

#define LBR_ERROR(format, args...) LBR_PRINT(G_lbr_log_level, \
LBR_ERROR_LEVEL, "!!! [%s: %d]" format "\n", __FILE__, __LINE__, ##args)
#define LBR_MSG(format, args...) LBR_PRINT(G_lbr_log_level, \
LBR_MSG_LEVEL, ">>> " format, ##args)
#define LBR_INFO(format, args...) LBR_PRINT(G_lbr_log_level, \
LBR_INFO_LEVEL, "::: " format, ##args)
#define LBR_DEBUG(format, args...) LBR_PRINT(G_lbr_log_level, \
LBR_DEBUG_LEVEL, "=== " format, ##args)

#endif /* __LBR_COMMON_H__ */
