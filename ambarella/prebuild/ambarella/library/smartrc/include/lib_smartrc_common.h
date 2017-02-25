/*******************************************************************************
 * lib_smartrc_common.h
 *
 * History:
 *   2015/11/23 - [ypxu] created file
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
 ******************************************************************************/

#ifndef _LIB_SMARTRC_COMMON_H_
#define _LIB_SMARTRC_COMMON_H_

#include <basetypes.h>
#include <config.h>
#include <iav_ioctl.h>

#ifdef CONFIG_ARCH_S2L
#define QP_MATRIX_SINGLE_SIZE	(IAV_MEM_QPM_SIZE)
#elif defined CONFIG_ARCH_S3L
#define QP_MATRIX_SINGLE_SIZE	(IAV_MEM_ROI_MATRIX_SIZE)
#else
//reserved
#endif
#define SMARTRC_MAX_STREAM_NUM		(IAV_STREAM_MAX_NUM_ALL)
#define SMARTRC_ENCODE_WIDTH_MIN	(160)
#define SMARTRC_ENCODE_WIDTH_MAX	(4096)
#define SMARTRC_ENCODE_HEIGHT_MIN	(90)
#define SMARTRC_ENCODE_HEIGHT_MAX	(3008)

#ifndef AM_IOCTL
#define AM_IOCTL(_flip, _cmd, _arg)	\
	do {	\
		if (ioctl(_flip, _cmd, _arg) < 0) {	\
			perror(#_cmd);	\
			return -1;	\
		}	\
	} while(0);
#endif

#ifndef VERIFY_STREAM_ID
#define VERIFY_STREAM_ID(x) \
	do {	\
		if ((x) < 0 || (x) >= SMARTRC_MAX_STREAM_NUM) {	\
			printf("stream id out of range\n");	\
			return -1;	\
		}	\
	} while(0);
#endif

#ifndef VERIFY_ENCODE_RESOLUTION
#define VERIFY_ENCODE_RESOLUTION(w, h) \
	do {	\
		if ((w) < SMARTRC_ENCODE_WIDTH_MIN ||	\
			(w) > SMARTRC_ENCODE_WIDTH_MAX ||	\
			(h) < SMARTRC_ENCODE_HEIGHT_MIN ||	\
			(h) > SMARTRC_ENCODE_HEIGHT_MAX) {	\
			printf("encode resolution invalid\n");	\
			return -1;	\
		}	\
	} while (0);
#endif

#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif
#ifndef ROUND_DOWN
#define ROUND_DOWN(size, align) ((size) & ~((align) - 1))
#endif

extern int g_smartrc_log_level;

typedef enum motion_level_e
{
	MOTION_NONE	= 0,
	MOTION_LOW 	= 1,
	MOTION_MID	= 2,
	MOTION_HIGH = 3, //worst case is default
	MOTION_NUM,
	MOTION_FIRST = MOTION_NONE,
	MOTION_LAST = MOTION_NUM,
} motion_level_t;

typedef enum noise_level_e
{
	NOISE_NONE	= 0,
	NOISE_LOW 	= 1,
	NOISE_HIGH	= 2,//worst case is default
	NOISE_NUM,
	NOISE_FIRST = NOISE_NONE,
	NOISE_LAST = NOISE_NUM,
} noise_level_t;

typedef enum quality_level_e
{
	QUALITY_LOW = 0,	//default setting, bitrate ceiling 1Mbps for 1080p
	QUALITY_MEDIUM = 1,	//bitrate ceiling 2Mbps for 1080p
	QUALITY_HIGH = 2,	//bitrate ceiling 4Mbps for 1080p
	QUALITY_ULTIMATE = 3,	//bitrate ceiling 6Mbps for 1080p
	QUALITY_RESERVED_1 = 4,	//leaved for customers to design
	QUALITY_RESERVED_2 = 5,
	QUALITY_RESERVED_3 = 6,
	QUALITY_RESERVED_4 = 7,
	QUALITY_RESERVED_5 = 8,
	QUALITY_RESERVED_6 = 9,
	QUALITY_NUM,
	QUALITY_FIRST = QUALITY_LOW,
	QUALITY_LAST = QUALITY_NUM,
} quality_level_t;

typedef enum style_e
{
	STYLE_FPS_KEEP_BITRATE_AUTO = 0, //try to keep max fps, bitrate auto drop when possible (default, consumer style)
	STYLE_QUALITY_KEEP_FPS_AUTO_DROP = 1, //similar to default, but fps can auto drop when no budget to keep quality(Quality keeping)
	STYLE_FPS_KEEP_CBR_ALIKE = 2, //FPS keep, CBR alike. (traditional professional IPCAM style)
	STYLE_NUM,
	STYLE_FIRST = STYLE_FPS_KEEP_BITRATE_AUTO,
	STYLE_LAST = STYLE_NUM,
} style_t;

typedef enum profile_e
{
	PROFILE_STATIC 						= 0,
	PROFILE_SMALL_MOTION 				= 1,
	PROFILE_MID_MOTION					= 2,
	PROFILE_BIG_MOTION					= 3,
	PROFILE_LOW_LIGHT					= 4,
	PROFILE_BIG_MOTION_WITH_FRAME_DROP	= 5,
	PROFILE_SECURITY_IPCAM_CBR 			= 6,
	PROFILE_NUM,
	PROFILE_FIRST = PROFILE_STATIC,
	PROFILE_LAST = PROFILE_NUM,
} profile_t;

typedef struct version_s
{
	int major;
	int minor;
	int patch;
	unsigned int mod_time;
	char description[64];
} version_t;

typedef struct threshold_s
{
	u32 motion_low;
	u32 motion_mid;
	u32 motion_high;
	u16 noise_low;
	u16 noise_high;
} threshold_t;

typedef struct qp_limit_s
{
	u32 I_qp_limit_min;
	u32 I_qp_limit_max;
	u32 P_qp_limit_min;
	u32 P_qp_limit_max;

	u32 B_qp_limit_min;
	u32 B_qp_limit_max;
	u32 Q_qp_limit_min;
	u32 Q_qp_limit_max;

	u32 I_qp_reduce;
	u32 P_qp_reduce;
	u32 Q_qp_reduce;

	u32 adapt_qp;

}qp_param_t;

typedef enum qp_param_num_s
{
	I_QP_LIMIT_MIN = 0,
	I_QP_LIMIT_MAX = 1,
	P_QP_LIMIT_MIN = 2,
	P_QP_LIMIT_MAX = 3,

	B_QP_LIMIT_MIN = 4,
	B_QP_LIMIT_MAX = 5,
	Q_QP_LIMIT_MIN = 6,
	Q_QP_LIMIT_MAX = 7,

	I_QP_REDUCE = 8,
	P_QP_REDUCE = 9,
	Q_QP_REDUCE = 10,

	ADAPT_QP = 11,

	QP_PARAM_NUM,
	QP_FIRST = I_QP_LIMIT_MIN,
	QP_LAST = Q_QP_REDUCE,

}qp_param_num_t;

typedef struct bitrate_param_case_target_s
{
	u32 bitrate_per_MB[SMARTRC_MAX_STREAM_NUM];
	u32 bitrate_per_MB_ref;	//bitrate for every Macroblock
} bitrate_param_case_target_t;

typedef struct roi_s
{
	s8 static_qp_adjust;
	s8 dynamic_qp_adjust;
} roi_t;

typedef struct base_param_s
{
	bitrate_param_case_target_t bitrate_t[QUALITY_NUM][PROFILE_NUM];
	u32 force_pskip[PROFILE_NUM];
	qp_param_t qp_param[PROFILE_NUM];
	u32 gop_N[PROFILE_NUM];
	roi_t roi_params[PROFILE_NUM];
}base_param_t;

typedef enum param_flag_s
{
	BITRATE_FLAG = 0,
	GOP_FLAG = 1,
	QP_FLAG =2,
	PSKIP_FLAG = 3,
	ROI_PARAM = 4,
	FLAG_NUM,
	PARAM_FLAG_FIRST = BITRATE_FLAG,
	PARAM_FLAG_NUM = FLAG_NUM,
}param_flag_t;

typedef enum roi_flag_s
{
	STATIC_ROI_PARAM = 0,
	DYNAMIC_ROI_PARAM = 1,
	ROI_NUM,
	ROI_FLAG_NUM = ROI_NUM,
}roi_flag_t;

typedef enum gop_type_s
{
	IPPP = 0,
	IBBP = 1,
	NUM,
	GOP_TYPE_NUM = NUM,
}gop_type_t;

typedef struct delay_s
{
	u32 motion_indicator;
	u16 motion_none;
	u16 motion_low;
	u16 motion_mid;
	u16 motion_high;
	u16 noise_none;
	u16 noise_low;
	u16 noise_high;
	u16 reserved;
} delay_t;

typedef struct encode_config_s
{
	u32 param_inited;
	u32 src_buf_id;
	u32 codec_type;
	u32 fps;
	u32 pitch;
	u32 width;
	u32 height;
	u32 x;
	u32 y;
	u32 M;
	u32 N;
	u32 is_2_ref;
	u32 roi_width;
	u32 roi_height;
} encode_config_t;

typedef struct init_s
{
	int fd_iav;
	int fd_vin;
} init_t;

typedef struct param_config_s
{
	encode_config_t enc_cfg;
	u32 stream_id;
	u32 bitrate_gap_adj;
} param_config_t;

typedef struct bitrate_target_s
{
	u32 auto_target;		//if auto is 1, ignore other fields
	u32 bitrate_ceiling;	//useful when auto is 0
} bitrate_target_t;

typedef struct roi_session_s
{
	u32 roi_started;
	u32 dsp_pts;
	u32 count;
	u8 *tmpdata;
	u8 *motion_matrix;
	u8 *prev1;
	u8 *prev2;
	u8 *daddr[QP_FRAME_TYPE_NUM]; //qp roi param daddr, used by s2/s2e
} roi_session_t;

#ifndef VERIFY_MOTION_LEVEL
#define VERIFY_MOTION_LEVEL(x)	\
	do {	\
		if ((x) < MOTION_FIRST || (x) >= MOTION_LAST) {	\
			printf("invalid motion level\n");	\
			return -1;	\
		}	\
	} while (0);
#endif

#ifndef VERIFY_NOISE_LEVEL
#define VERIFY_NOISE_LEVEL(x) \
	do {	\
		if ((x) < NOISE_FIRST || (x) >= NOISE_LAST) {	\
			printf("invalid noise level\n");	\
			return -1;	\
		}	\
	} while (0);
#endif

#ifndef VERIFY_QUALITY_LEVEL
#define VERIFY_QUALITY_LEVEL(x) \
	do {	\
		if ((x) < QUALITY_FIRST || (x) >= QUALITY_LAST) {	\
			printf("invalid quality level\n");	\
			return -1;	\
		}	\
	} while (0);
#endif

#ifndef VERIFY_STYLE
#define VERIFY_STYLE(x)	\
	do {	\
		if ((x) < STYLE_FIRST || (x) >= STYLE_LAST) {	\
			printf("invalid style\n");	\
			return -1;	\
		}	\
	} while (0);
#endif

#ifndef VERIFY_PROFILE
#define VERIFY_PROFILE(x)	\
	do {	\
		if ((x) < PROFILE_FIRST || (x) >= PROFILE_LAST) {	\
			printf("invalid profile\n");	\
			return -1;	\
		}	\
	} while (0);
#endif

#endif /* _LIB_SMARTRC_COMMON_H_ */
