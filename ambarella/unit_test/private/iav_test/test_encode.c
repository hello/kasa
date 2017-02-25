/*
 * test_encode.c
 * the program can setup VIN , preview and start encoding/stop
 * encoding for flexible multi streaming encoding.
 * after setup ready or start encoding/stop encoding, this program
 * will exit
 *
 * History:
 *	2010/12/31 - [Louis Sun] create this file base on test2.c
 *	2011/10/31 - [Jian Tang] modified this file.
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
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>
#include <iav_ioctl.h>
#include <iav_ucode_ioctl.h>

#include <signal.h>


#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif

#ifndef DIV_ROUND
#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#endif

#define MAX_ENCODE_STREAM_NUM		4
#define MAX_SOURCE_BUFFER_NUM		(4 + 1)
#define MAX_PREVIEW_BUFFER_NUM		2

#define MIN_ABS_FPS		1
#define MAX_ABS_FPS		60

//support up to 4 streams by this design
#define ALL_ENCODE_STREAMS	((1 << MAX_ENCODE_STREAM_NUM) - 1)

#define COLOR_PRINT0(msg)		printf("\033[34m"msg"\033[39m")
#define COLOR_PRINT(msg, arg...)		printf("\033[34m"msg"\033[39m", arg)
#define BOLD_PRINT0(msg, arg...)		printf("\033[1m"msg"\033[22m")
#define BOLD_PRINT(msg, arg...)		printf("\033[1m"msg"\033[22m", arg)


/* osd blending mixer selection */
#define OSD_BLENDING_OFF				0
#define OSD_BLENDING_FROM_MIXER_A		1
#define OSD_BLENDING_FROM_MIXER_B		2

typedef enum
{
	DSP_PARTITION_OFF = 0,
	DSP_PARTITION_RAW = 1,
	DSP_PARTITION_MAIN_CAPTURE = 2,
	DSP_PARTITION_PREVIEW_A = 3,
	DSP_PARTITION_PREVIEW_B = 4,
	DSP_PARTITION_PREVIEW_C = 5,
	DSP_PARTITION_WARPED_MAIN_CAPTURE = 6, /* post-main, for dewarp pipeline only */
	DSP_PARTITION_WARPED_INT_MAIN_CAPTURE = 7, /* intermediate-main, for dewarp pipeline only */
	DSP_PARTITION_EFM = 8,
	FRM_BUF_POOL_TYPE_NUM = 9,
} DSP_PARTITION_TYPE ;


//static struct timeval start_enc_time;

/*
 * use following commands to enable timer1
 *

amba_debug -w 0x7000b014 -d 0xffffffff
amba_debug -w 0x7000b030 -d 0x15
amba_debug -r 0x7000b000 -s 0x40

*/

typedef enum {
	IAV_CBR = 0,
	IAV_VBR,
	IAV_CBR_QUALITY_KEEPING,
	IAV_VBR_QUALITY_KEEPING,
	IAV_CBR2,
	IAV_VBR2,
} iav_rate_control_mode;

typedef enum {
	IAV_SHOW_SYSTEM_STATE = 0x00,
	IAV_SHOW_ENCODE_CFG = 0x01,
	IAV_SHOW_STREAM_INFO = 0x02,
	IAV_SHOW_BUFFER_INFO = 0x03,
	IAV_SHOW_RESOURCE_INFO = 0x04,
	IAV_SHOW_ENCMODE_CAP = 0x05,
	IAV_SHOW_DRIVER_INFO = 0x06,
	IAV_SHOW_CHIP_INFO = 0x07,
	IAV_SHOW_DRAM_LAYOUT = 0x08,
	IAV_SHOW_FEATURE_SET = 0x09,
	IAV_SHOW_CMD_EXAMPPLES = 0x0A,
	IAV_SHOW_QP_HIST = 0x0B,
	IAV_SHOW_INFO_TOTAL_NUM,
	IAV_SHOW_INFO_FIRST = 0,
	IAV_SHOW_INFO_LAST = IAV_SHOW_INFO_TOTAL_NUM,
} iav_show_info;

// the device file handle
static int fd_iav;

// QP Matrix buffer
static u8 * G_qp_matrix_addr = NULL;
static int G_qp_matrix_size = 0;

// vin
#include "../vin_test/vin_init.c"
#include "../vout_test/vout_init.c"

#define VERIFY_STREAMID(x)   do {		\
			if (((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) {	\
				printf ("stream id wrong %d \n", (x));			\
				return -1; 	\
			}	\
		} while (0)

#define VERIFY_BUFFERID(x)	do {		\
			if ((x) < 0 || ((x) >= MAX_SOURCE_BUFFER_NUM)) {	\
				printf ("buffer id wrong %d\n", (x));			\
				return -1;	\
			}	\
		} while (0)

#define VERIFY_TYPE_CHANGEABLE_BUFFERID(x)	do {		\
			if ((x) < IAV_SUB_SRCBUF_FIRST|| ((x) >= IAV_SUB_SRCBUF_LAST)) {	\
				printf ("buffer id wrong %d, cannot change type\n", (x));			\
				return -1;	\
			}	\
		} while (0)

//encode format
typedef struct encode_format_s {
	int type;
	int type_changed_flag;

	int width;
	int height;
	int resolution_changed_flag;

	int offset_x;
	int offset_y;
	int offset_changed_flag;

	int source;
	int source_changed_flag;

	int hflip;
	int hflip_flag;

	int vflip;
	int vflip_flag;

	int rotate;
	int rotate_flag;

	u16 duration;
	u16 duration_flag;
} encode_format_t;

//source buffer type
typedef struct source_buffer_type_s {
	enum iav_srcbuf_type buffer_type;
	int buffer_type_changed_flag;
}  source_buffer_type_t;

//source buffer format
typedef struct source_buffer_format_s {
	int width;
	int height;
	int resolution_changed_flag;

	int input_width;
	int input_height;
	int input_size_changed_flag;

	int input_x;
	int input_y;
	int input_offset_changed_flag;

	int unwarp;
	int unwarp_flag;

	int dump_interval;
	int dump_interval_flag;

	int dump_duration;
	int dump_duration_flag;
}  source_buffer_format_t;

//resolution
typedef struct resolution_s {
	int width;
	int height;
	int resolution_changed_flag;
} resolution_t;

//qp limit
typedef struct qp_limit_params_s {
	u8	qp_min_i;
	u8	qp_max_i;
	u8	qp_min_p;
	u8	qp_max_p;
	u8	qp_min_b;
	u8	qp_max_b;
	u8	qp_min_q;
	u8	qp_max_q;
	u8	adapt_qp;
	u8	i_qp_reduce;
	u8	p_qp_reduce;
	u8	q_qp_reduce;
	u8	log_q_num_plus_1;
	u8	skip_frame;
	u16	max_i_size_KB;

	u8	qp_i_flag;
	u8	qp_p_flag;
	u8	qp_b_flag;
	u8	qp_q_flag;
	u8	adapt_qp_flag;
	u8	i_qp_reduce_flag;
	u8	p_qp_reduce_flag;
	u8	q_qp_reduce_flag;
	u8	log_q_num_plus_1_flag;
	u8	skip_frame_flag;
	u8	max_i_size_KB_flag;
	u8	reserved1;
} qp_limit_params_t;

// qp matrix delta value
typedef struct qp_matrix_params_s {
	s8	delta[4];
	int	delta_flag;

	int	matrix_mode;
	int	matrix_mode_flag;
} qp_matrix_params_t;

// h.264 config
typedef struct h264_param_s {
	int h264_M;
	int h264_N;
	int h264_idr_interval;
	int h264_gop_model;
	int h264_bitrate_control;
	int h264_cbr_avg_bitrate;
	int h264_vbr_min_bitrate;
	int h264_vbr_max_bitrate;

	int h264_deblocking_filter_alpha;
	int h264_deblocking_filter_beta;
	int h264_deblocking_filter_enable;

	int h264_chrome_format;			// 0: YUV420; 1: Mono
	int h264_chrome_format_flag;

	int h264_M_flag;
	int h264_N_flag;
	int h264_idr_interval_flag;
	int h264_gop_model_flag;
	int h264_bitrate_control_flag;
	int h264_cbr_bitrate_flag;
	int h264_vbr_bitrate_flag;

	int h264_deblocking_filter_alpha_flag;
	int h264_deblocking_filter_beta_flag;
	int h264_deblocking_filter_enable_flag;

	int h264_profile_level;
	int h264_profile_level_flag;

	int h264_mv_threshold;
	int h264_mv_threshold_flag;

	int h264_flat_area_improve;
	int h264_flat_area_improve_flag;

	int h264_fast_seek_intvl;
	int h264_fast_seek_intvl_flag;

	int h264_multi_ref_p;
	int h264_multi_ref_p_flag;

	int h264_drop_frames;
	int h264_drop_frames_flag;

	int h264_intrabias_p;
	int h264_intrabias_p_flag;

	int h264_intrabias_b;
	int h264_intrabias_b_flag;

	int	h264_user1_intra_bias;
	int	h264_user1_intra_bias_flag;
	int	h264_user1_direct_bias;
	int	h264_user1_direct_bias_flag;
	int	h264_user2_intra_bias;
	int	h264_user2_intra_bias_flag;
	int	h264_user2_direct_bias;
	int	h264_user2_direct_bias_flag;
	int	h264_user3_intra_bias;
	int	h264_user3_intra_bias_flag;
	int	h264_user3_direct_bias;
	int	h264_user3_direct_bias_flag;

	int force_intlc_tb_iframe;
	int force_intlc_tb_iframe_flag;

	int au_type;
	int au_type_flag;

	u8 cpb_buf_idc;
	u8 cpb_cmp_idc;
	u8 en_panic_rc;
	u8 fast_rc_idc;
	u32 cpb_user_size;
	int panic_mode_flag;

	u16 h264_frame_crop_left_offset;
	u16 h264_frame_crop_right_offset;
	u16 h264_frame_crop_top_offset;
	u16 h264_frame_crop_bottom_offset;

	u16 h264_frame_crop_left_offset_flag;
	u16 h264_frame_crop_right_offset_flag;
	u16 h264_frame_crop_top_offset_flag;
	u16 h264_frame_crop_bottom_offset_flag;

	int h264_abs_br;
	int h264_abs_br_flag;

	int force_pskip_repeat_enable;
	int force_pskip_repeat_num;
	int force_pskip_flag;
} h264_param_t;

typedef struct jpeg_param_s{
	int quality;
	int quality_changed_flag;

	int jpeg_chrome_format;			// 1: YUV420; 2: Mono
	int jpeg_chrome_format_flag;
} jpeg_param_t;

//use struct instead of union here, so that mis config of jpeg param does not break h264 param
typedef struct encode_param_s{
	h264_param_t h264_param;
	jpeg_param_t jpeg_param;
} encode_param_t;

typedef struct system_resource_setup_s {
	resolution_t source_buffer_max_size[MAX_SOURCE_BUFFER_NUM];
	resolution_t stream_max_size[MAX_ENCODE_STREAM_NUM];
	int source_buffer_max_size_changed_id;
	int stream_max_size_changed_id;
	int encode_mode;
	int encode_mode_flag;
	int rotate_possible;
	int rotate_possible_flag;
	int raw_capture_possible;
	int raw_capture_possible_flag;
	int osd_mixer;
	int osd_mixer_changed_flag;
	int hdr_exposure_num;
	int hdr_exposure_num_flag;
	int extra_dram_buf[MAX_SOURCE_BUFFER_NUM];
	int extra_dram_buf_changed_id;
	int lens_warp;
	int lens_warp_flag;
	int max_enc_num;
	int max_enc_num_flag;
	int max_warp_input_width;
	int max_warp_input_width_flag;
	int max_warp_input_height;
	int max_warp_input_height_flag;
	int max_warp_output_width;
	int max_warp_output_width_flag;
	int enc_raw_rgb;
	int enc_raw_rgb_flag;
	int max_padding_width;
	int max_padding_width_flag;
	int idsp_upsample_type;
	int idsp_upsample_type_flag;
	int mctf_pm;
	int mctf_pm_flag;
	int vout_swap;
	int vout_swap_flag;
	int eis_delay_count;
	int eis_delay_count_flag;
	int me0_scale;
	int me0_scale_flag;
	int enc_from_mem;
	int enc_from_mem_flag;
	int efm_buf_num;
	int efm_buf_num_flag;
	int enc_raw_yuv;
	int enc_raw_yuv_flag;
	int vin_overflow_protection;
	int vin_overflow_protection_flag;

	/* Intermediate buffer*/
	int v_warped_main_max_width;
	int v_warped_main_max_height;
	int intermediate_buf_size_flag;

	resolution_t raw_size;
	resolution_t efm_size;

	int enc_dummy_latency;
	int enc_dummy_latency_flag;

	int dsp_partition_map;
	int dsp_partition_map_flag;

	int long_ref_b_frame;
	int long_ref_b_frame_flag;

	int stream_long_ref_enable[MAX_ENCODE_STREAM_NUM];
	int stream_long_ref_enable_flag[MAX_ENCODE_STREAM_NUM];
	int max_gop_M[MAX_ENCODE_STREAM_NUM];
	int max_gop_M_flag[MAX_ENCODE_STREAM_NUM];

	int debug_stitched;
	int debug_stitched_flag;
	int debug_iso_type;
	int debug_iso_type_flag;
	int debug_chip_id;
	int debug_chip_id_flag;

	int extra_top_row_buf;
	int extra_top_row_buf_flag;
}system_resource_setup_t;

typedef struct debug_setup_s {
	u8 check_disable;
	u8 check_disable_flag;

	u8 wait_frame_flag;

	u8 audit_int_enable;
	u8 audit_int_enable_flag;
	u8 audit_id;
	u8 audit_id_flag;
} debug_setup_t;

// state control
static int idle_cmd = 0;
static int nopreview_flag = 0;		// do not enter preview automatically

//stream and source buffer identifier
static int current_stream = -1; 	// -1 is a invalid stream, for initialize data only
static int current_buffer = -1;	// -1 is a invalid buffer, for initialize data only

//encode start/stop/format control variables
static u32 start_stream_id = 0;
static u32 stop_stream_id = 0;
static u32 abort_stream_id = 0;

//encoding settings
static encode_format_t encode_format[MAX_ENCODE_STREAM_NUM];
static encode_param_t encode_param[MAX_ENCODE_STREAM_NUM];
static u32  encode_format_changed_id = 0;
static u32 encode_param_changed_id  = 0;
static u32 GOP_changed_id  = 0;

//system information display
static u32 show_info_flag = 0;

//source buffer settings
static source_buffer_type_t source_buffer_type[MAX_SOURCE_BUFFER_NUM];
static source_buffer_format_t  source_buffer_format[MAX_SOURCE_BUFFER_NUM];
static int source_buffer_type_changed_flag = 0;
static u32 source_buffer_format_changed_id = 0;

//preview buffer settings
static resolution_t  preview_buffer_format[MAX_PREVIEW_BUFFER_NUM];

// system resource settings
static system_resource_setup_t system_resource_setup;
static int system_resource_limit_changed_flag = 0;

//force idr generation
static u32 force_idr_id = 0;

//long ref p generation
static u32 long_ref_p_id = 0;

//force fast seek frame
static u32 force_fast_seek_id = 0;

//encoding frame rate settings
static u32 framerate_factor_changed_id = 0;
static u32 framerate_factor_sync_id = 0;
static int framerate_factor[MAX_ENCODE_STREAM_NUM][2];

static int stream_abs_fps_enable[MAX_ENCODE_STREAM_NUM];
static int stream_abs_fps_enabled_id;
static int stream_abs_fps[MAX_ENCODE_STREAM_NUM];
static int stream_abs_fps_changed_id;

//rate control settings
static qp_limit_params_t qp_limit_param[MAX_ENCODE_STREAM_NUM];
static u32 qp_limit_changed_id = 0;

//intra refresh MB rows
static int intra_mb_rows[MAX_ENCODE_STREAM_NUM];
static u32 intra_mb_rows_changed_id = 0;

//qp matrix settings
static qp_matrix_params_t qp_matrix_param[MAX_ENCODE_STREAM_NUM];
static u32 qp_matrix_changed_id = 0;

//misc settings
static u32 restart_stream_id = 0;
static int dump_idsp_bin_flag = 0;

//debug settings
static debug_setup_t debug_setup;
static int debug_setup_flag = 0;

static int dsp_clock_state_disable = 0;
static int dsp_clock_state_disable_flag = 0;

struct encode_resolution_s {
	const char 	*name;
	int		width;
	int		height;
}
__encode_res[] =
{
	{"4kx3k",4016,3016},
	{"4kx2k",4096,2160},
	{"4k",4096,2160},
	{"qfhd",3840,2160},
	{"5m", 2592, 1944},
	{"5m_169", 2976, 1674},
	{"3m", 2048, 1536},
	{"3m_169", 2304, 1296},
	{"qhd", 2560, 1440},
	{"1080p", 1920, 1080},
	{"720p", 1280, 720},
	{"480p", 720, 480},
	{"576p", 720, 576},
	{"4sif", 704, 480},
	{"4cif", 704, 576},
	{"xga", 1024, 768},
	{"vga", 640, 480},
	{"wvga", 800, 480},
	{"fwvga", 854, 480},	//a 16:9 style
	{"360p", 640, 360},
	{"cif", 352, 288},
	{"sif", 352, 240},
	{"qvga", 320, 240},
	{"qwvga", 400, 240},
	{"qcif", 176, 144},
	{"qsif", 176, 120},
	{"qqvga", 160, 120},
	{"svga", 800, 600},
	{"sxga", 1280, 1024},
	{"480i", 720, 480},
	{"576i", 720, 576},
	{"1080i", 1920, 1080},

	{"", 0, 0},

	{"1920x1080", 1920, 1080},
	{"1600x1200", 1600, 1200},
	{"1440x1080", 1440, 1080},
	{"1366x768", 1366, 768},
	{"1280x1024", 1280, 1024},
	{"1280x960", 1280, 960},
	{"1280x720", 1280, 720},
	{"1024x768", 1024, 768},
	{"720x480", 720, 480},
	{"720x576", 720, 576},

	{"", 0, 0},

	{"704x480", 704, 480},
	{"704x576", 704, 576},
	{"640x480", 640, 480},
	{"640x360", 640, 360},
	{"352x288", 352, 288},
	{"352x256", 352, 256},	//used for interlaced MJPEG 352x256 encoding ( crop to 352x240 by app)
	{"352x240", 352, 240},
	{"320x240", 320, 240},
	{"176x144", 176, 144},
	{"176x120", 176, 120},
	{"160x120", 160, 120},

	//vertical video resolution
	{"480x640", 480, 640},
	{"480x854", 480, 854},

	//for preview size only to keep aspect ratio in preview image for different VIN aspect ratio
	{"16_9_vin_ntsc_preview", 720, 360},
	{"16_9_vin_pal_preview", 720, 432},
	{"4_3_vin_ntsc_preview", 720, 480},
	{"4_3_vin_pal_preview", 720, 576},
	{"5_4_vin_ntsc_preview", 672, 480},
	{"5_4_vin_pal_preview", 672, 576 },
	{"ntsc_vin_ntsc_preview", 720, 480},
	{"pal_vin_pal_preview", 720, 576},
};

#define	NO_ARG		0
#define	HAS_ARG		1

static const char *short_options = "ABb:Cc:Dd:ef:h:JKm:nsM:N:q:i:S:Pv:V:w:W:XY";

struct hint_s {
	const char *arg;
	const char *str;
};

#define	STREAM_NUMVERIC_SHORT_OPTIONS				\
	ENCODING_OFFSET_X_Y	=	STREAM_OPTIONS_BASE,	\
	ENCODING_MAX_SIZE

#define	ENCODE_CONTROL_NUMVERIC_SHORT_OPTIONS				\
	SPECIFY_MULTISTREAMS_START	=	ENCODE_CONTROL_OPTIONS_BASE,	\
	SPECIFY_MULTISTREAMS_STOP,	\
	SPECIFY_FORCE_IDR,			\
	ENCODING_RESTART,			\
	FRAME_FACTOR,				\
	FRAME_FACTOR_SYNC,			\
	STREAM_ABS_FPS,			\
	SPECIFY_STREAM_ABS_FPS,	\
	SPECIFY_LONG_REF_P,		\
	SPECIFY_STREAM_ABORT

#define	H264ENC_NUMVERIC_SHORT_OPTIONS					\
	SPECIFY_GOP_IDR				=	H264ENC_OPTIONS_BASE,	\
	SPECIFY_GOP_MODEL,			\
	BITRATE_CONTROL,			\
	SPECIFY_CBR_BITRATE,			\
	SPECIFY_VBR_BITRATE,			\
	CHANGE_QP_LIMIT_I,			\
	CHANGE_QP_LIMIT_P,			\
	CHANGE_QP_LIMIT_B,			\
	CHANGE_QP_LIMIT_Q,			\
	CHANGE_ADAPT_QP,			\
	CHANGE_I_QP_REDUCE,		\
	CHANGE_P_QP_REDUCE,		\
	CHANGE_Q_QP_REDUCE,		\
	CHANGE_LOG_Q_NUM_PLUS_1,	\
	CHANGE_SKIP_FRAME_MODE,	\
	CHANGE_MAX_I_SIZE_KB,	\
	CHANGE_INTRA_MB_ROWS,		\
	CHANGE_QP_MATRIX_DELTA,	\
	CHANGE_QP_MATRIX_MODE,		\
	DEBLOCKING_ALPHA,			\
	DEBLOCKING_BETA,			\
	DEBLOCKING_ENABLE,			\
	LEFT_FRAME_CROP,			\
	RIGHT_FRAME_CROP,			\
	TOP_FRAME_CROP,				\
	BOTTOM_FRAME_CROP,			\
	SPECIFY_PROFILE_LEVEL,		\
	INTLC_IFRAME,				\
	SPECIFY_MV_THRESHOLD,		\
	SPECIFY_FLAT_AREA_IMPROVE,		\
	SPECIFY_FAST_SEEK_INTVL,	\
	SPECIFY_MULTI_REF_P,			\
	SPECIFY_MAX_GOP_M,			\
	SPECIFY_FORCE_FAST_SEEK,		\
	SPECIFY_FRAME_DROP,			\
	SPECIFY_INTRABIAS_P,			\
	SPECIFY_INTRABIAS_B,			\
	SPECIFY_USER1_INTRABIAS,		\
	SPECIFY_USER1_DIRECTBIAS,	\
	SPECIFY_USER2_INTRABIAS,		\
	SPECIFY_USER2_DIRECTBIAS,	\
	SPECIFY_USER3_INTRABIAS,		\
	SPECIFY_USER3_DIRECTBIAS,	\
	SPECIFY_AU_TYPE,			\
	SPECIFY_ABS_BR_FLAG,		\
	SPECIFY_FORCE_PSKIP_REPEAT,	\
	SPECIFY_REPEAT_PSKIP_NUM


#define	PANIC_NUMVERIC_SHORT_OPTIONS		\
	CPB_BUF_IDC			=	PANIC_OPTIONS_BASE,	\
	CPB_CMP_IDC,		\
	ENABLE_PANIC_RC,	\
	FAST_RC_IDC,			\
	CPB_USER_SIZE

#define	COMMON_ENCODE_NUMVERIC_SHORT_OPTIONS		\
	SPECIFY_HFLIP		=	COMMON_ENCODE_OPTIONS_BASE,	\
	SPECIFY_VFLIP,		\
	SPECIFY_ROTATE,		\
	SPECIFY_CHROME_FORMAT

#define	SYSTEM_NUMVERIC_SHORT_OPTIONS		\
	NO_PREVIEW = SYSTEM_OPTIONS_BASE,			\
	SYSTEM_IDLE,					\
	SPECIFY_ENCODE_MODE,		\
	SPECIFY_ROTATE_POSSIBLE,		\
	SPECIFY_RAW_CAPTURE,		\
	SPECIFY_HDR_EXPOSURE_NUM,	\
	SPECIFY_LENS_WARP,			\
	SPECIFY_MAX_ENC_NUM,		\
	SPECIFY_MAX_WARP_INPUT_HEIGHT,	\
	SPECIFY_MAX_PADDING_WIDTH,		\
	SPECIFY_INTERMEDIATE_BUF_SIZE,	\
	SPECIFY_ENC_DUMMY_LATENCY,		\
	SPECIFY_LONG_REF_ENABLE,		\
	SPECIFY_LONG_REF_B_FRAME,		\
	SPECIFY_DSP_PARTITION_MAP,		\
	SPECIFY_IDSP_UPSAMPLE_TYPE,		\
	SPECIFY_MCTF_PM,				\
	SPECIFY_VOUT_SWAP,			\
	SPECIFY_EIS_DELAY_COUNT,	\
	SPECIFY_ME0_SCALE,			\
	SPECIFY_ENC_RAW_RGB,		\
	SPECIFY_ENC_RAW_YUV,		\
	SPECIFY_RAW_SIZE,			\
	SPECIFY_ENC_FROM_MEM,		\
	SPECIFY_EFM_BUF_NUM,		\
	SPECIFY_EFM_SIZE,			\
	DUMP_IDSP_CONFIG,			\
	SPECIFY_OSD_MIXER,			\
	SPECIFY_OVERFLOW_PROTECTION,\
	SPECIFY_DSP_CLCOK_STATE,	\
	SPECIFY_EXTRA_TOP_RAW_BUF

#define	SOURCE_BUFFER_NUMVERIC_SHORT_OPTIONS		\
	SPECIFY_BUFFER_TYPE = SOURCE_BUFFER_OPTIONS_BASE,	\
	SPECIFY_BUFFER_SIZE,			\
	SPECIFY_BUFFER_MAX_SIZE,		\
	SPECIFY_BUFFER_INPUT_SIZE,	\
	SPECIFY_BUFFER_INPUT_OFFSET,	\
	SPECIFY_BUFFER_PREWARP,		\
	SPECIFY_EXTRA_DRAM_BUF,		\
	SPECIFY_VCA_DUMP_INTERVAL,	\
	SPECIFY_VCA_DUMP_DURATION

#define	SHOW_NUMVERIC_SHORT_OPTIONS		\
	SHOW_SYSTEM_STATE = MISC_OPTIONS_BASE,		\
	SHOW_ENCODE_CONFIG,	\
	SHOW_STREAM_INFO,		\
	SHOW_BUFFER_INFO,		\
	SHOW_RESOURCE_INFO,	\
	SHOW_ENCODE_MODE_CAP,	\
	SHOW_DRIVER_INFO,		\
	SHOW_CHIP_INFO,			\
	SHOW_DRAM_LAYOUT,		\
	SHOW_FEATURE_SET,		\
	SHOW_CMD_EXAMPLES,		\
	SHOW_QP_HIST,		\
	SHOW_ALL_INFO

#define	DEBUG_NUMVERIC_SHORT_OPTIONS		\
	SPECIFY_CHECK_DISABLE = DEBUG_OPTIONS_BASE,	\
	SPECIFY_DEBUG_STITCHED,		\
	SPECIFY_DEBUG_ISO_TYPE,		\
	SPECIFY_DEBUG_CHIP_ID,		\
	SPECIFY_DEBUG_WAIT_FRAME,	\
	SPECIFY_DEBUG_AUDIT_ENABLE,	\
	SPECIFY_DEBUG_AUDIT_READ

enum numeric_short_options {
	STREAM_OPTIONS_BASE = 1000,
	ENCODE_CONTROL_OPTIONS_BASE = 1100,
	H264ENC_OPTIONS_BASE = 1200,
	PANIC_OPTIONS_BASE = 1300,
	COMMON_ENCODE_OPTIONS_BASE = 1400,
	VIN_OPTIONS_BASE = 1500,
	SYSTEM_OPTIONS_BASE = 1600,
	VOUT_OPTIONS_BASE = 1700,
	SOURCE_BUFFER_OPTIONS_BASE = 1800,
	MISC_OPTIONS_BASE = 1900,
	DEBUG_OPTIONS_BASE = 2000,

	STREAM_NUMVERIC_SHORT_OPTIONS,

	ENCODE_CONTROL_NUMVERIC_SHORT_OPTIONS,

	H264ENC_NUMVERIC_SHORT_OPTIONS,

	PANIC_NUMVERIC_SHORT_OPTIONS,

	COMMON_ENCODE_NUMVERIC_SHORT_OPTIONS,

	VIN_NUMVERIC_SHORT_OPTIONS,

	SYSTEM_NUMVERIC_SHORT_OPTIONS,

	VOUT_NUMERIC_SHORT_OPTIONS,

	SOURCE_BUFFER_NUMVERIC_SHORT_OPTIONS,

	SHOW_NUMVERIC_SHORT_OPTIONS,

	DEBUG_NUMVERIC_SHORT_OPTIONS,
};

/***************************************************************
	STREAM command line options
****************************************************************/
// -A xxxxx    means all following configs will be applied to stream A
#define	STREAM_A_OPTIONS			{"stream_A",	NO_ARG,		0,	'A'},
#define	STREAM_A_HINTS			{"",			"\t\tconfig for stream A"},

#define	STREAM_B_OPTIONS			{"stream_B",	NO_ARG,		0,	'B'},
#define	STREAM_B_HINTS			{"",			"\t\tconfig for stream B"},

#define	STREAM_C_OPTIONS			{"stream_C",	NO_ARG,		0,	'C'},
#define	STREAM_C_HINTS			{"",			"\t\tconfig for stream C"},

#define	STREAM_D_OPTIONS			{"stream_D",	NO_ARG,		0,	'D'},
#define	STREAM_D_HINTS			{"",			"\t\tconfig for stream D\n"},

#define	STREAM_H264_OPTIONS		{"h264", 		HAS_ARG,	0,	'h'},
#define	STREAM_H264_HINTS			{"resolution",	"\tenter H.264 encoding resolution"},

#define	STREAM_MJPEG_OPTIONS	{"mjpeg",	HAS_ARG,	0,	'm'},
#define	STREAM_MJPEG_HINTS		{"resolution",	"\tenter MJPEG encoding resolution"},

#define	STREAM_NONE_OPTIONS		{"none",		NO_ARG,		0,	'n'},
#define	STREAM_NONE_HINTS		{"",			"\t\tset stream encode type to NONE"},

// encode source buffer
#define	STREAM_SRCBUF_OPTIONS	{"src-buf",	HAS_ARG,	0,	'b'},
#define	STREAM_SRCBUF_HINTS		{"0|1|2|3|5","source buffers 0~3, and efm buffer 5" },

#define	STREAM_DURATION_OPTIONS		\
	{"duration",	HAS_ARG,	0,	'd'},
#define	STREAM_DURATION_HINTS		\
	{"0~65535",	"\tencode duration. The number of frames will be encoded."},

//encoding offset
#define	STREAM_OFFSET_OPTIONS		\
	{"offset",		HAS_ARG,	0,	ENCODING_OFFSET_X_Y},
#define	STREAM_OFFSET_HINTS			\
	{"resolution",	"cut out encoding offset X x Y from source buffer"},

//encoding max size
#define	STREAM_SMAXSIZE_OPTIONS		\
	{"smaxsize",	HAS_ARG,	0,	ENCODING_MAX_SIZE},
#define	STREAM_SMAXSIZE_HINTS		\
	{"resolution",	"specify stream max size for system resouce limit"},



#define	STREAM_LONG_OPTIONS()	\
	STREAM_A_OPTIONS			\
	STREAM_B_OPTIONS			\
	STREAM_C_OPTIONS			\
	STREAM_D_OPTIONS			\
	STREAM_H264_OPTIONS		\
	STREAM_MJPEG_OPTIONS		\
	STREAM_NONE_OPTIONS		\
	STREAM_SRCBUF_OPTIONS		\
	STREAM_DURATION_OPTIONS	\
	STREAM_OFFSET_OPTIONS		\
	STREAM_SMAXSIZE_OPTIONS	\

#define STREAM_PARAMETER_HINTS()	\
	STREAM_A_HINTS	\
	STREAM_B_HINTS	\
	STREAM_C_HINTS	\
	STREAM_D_HINTS	\
	STREAM_H264_HINTS		\
	STREAM_MJPEG_HINTS		\
	STREAM_NONE_HINTS		\
	STREAM_SRCBUF_HINTS	\
	STREAM_DURATION_HINTS	\
	STREAM_OFFSET_HINTS		\
	STREAM_SMAXSIZE_HINTS	\

#define	STREAM_INIT_PARAMETERS()		\
	case 'A':\
		current_stream = 0;\
		break;\
	case 'B':\
		current_stream = 1;\
		break;\
	case 'C':\
		current_stream = 2;\
		break;\
	case 'D':\
		current_stream = 3;\
		break;\
	case 'h':\
			VERIFY_STREAMID(current_stream);\
			if (get_encode_resolution(optarg, &width, &height) < 0)\
				return -1;	\
			encode_format[current_stream].type = IAV_STREAM_TYPE_H264;\
			encode_format[current_stream].type_changed_flag = 1;\
			encode_format[current_stream].width = width;\
			encode_format[current_stream].height = height;\
			encode_format[current_stream].resolution_changed_flag = 1;\
			encode_format_changed_id |= (1 << current_stream);\
			break;\
	case 'm':\
			VERIFY_STREAMID(current_stream);\
			if (get_encode_resolution(optarg, &width, &height) < 0)\
				return -1;\
			encode_format[current_stream].type = IAV_STREAM_TYPE_MJPEG;\
			encode_format[current_stream].type_changed_flag = 1;\
			encode_format[current_stream].width = width;\
			encode_format[current_stream].height = height;\
			encode_format[current_stream].resolution_changed_flag = 1;\
			encode_format_changed_id |= (1 << current_stream);\
			break;\
	case 'n':\
			printf("do not supported.\n");\
			break;\
	case 'b':\
			VERIFY_STREAMID(current_stream);\
			encode_format[current_stream].source = atoi(optarg);\
			encode_format[current_stream].source_changed_flag = 1;\
			encode_format_changed_id |= (1 << current_stream);\
			break;\
	case 'd':\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			if (min_value < 0 || min_value > 65535) {\
				printf("Invalid encode duration %d, must be in the "\
					"range of [0~65535].\n", min_value);\
				return -1;\
			}\
			encode_format[current_stream].duration = min_value;\
			encode_format[current_stream].duration_flag = 1;\
			encode_format_changed_id |= (1 << current_stream);\
			break;\
	case ENCODING_OFFSET_X_Y:\
			VERIFY_STREAMID(current_stream);\
			if (get_encode_resolution(optarg, &width, &height) < 0)\
				return -1;	\
			encode_format[current_stream].offset_x = width;	\
	                            encode_format[current_stream].offset_y = height;\
			encode_format[current_stream].offset_changed_flag = 1;\
			encode_format_changed_id |= (1 << current_stream);\
			break;\
	case ENCODING_MAX_SIZE:	\
			VERIFY_STREAMID(current_stream);\
			if (get_encode_resolution(optarg, &width, &height) < 0)\
				return -1;\
			system_resource_setup.stream_max_size[current_stream].width = width;\
			system_resource_setup.stream_max_size[current_stream].height = height;\
			system_resource_setup.stream_max_size_changed_id |= (1 << current_stream);\
			system_resource_limit_changed_flag = 1;\
			break;\

/***************************************************************
	ENCODE CONTROL command line options
****************************************************************/
//start encoding
#define	ENCODE_START_OPTIONS	{"encode",	NO_ARG,		0,	'e'},
#define	ENCODE_START_HINTS		{"", 			"\t\tstart encoding for current stream"},
//stop encoding
#define	ENCODE_STOP_OPTIONS		{"stop",		NO_ARG,		0,	's'},
#define	ENCODE_STOP_HINTS		{"", 			"\t\tstop encoding for current stream"},
//clear stream state
#define	ENCODE_ABORT_OPTIONS			\
	{"abort-encode",NO_ARG,	0,	SPECIFY_STREAM_ABORT},
#define	ENCODE_ABORT_HINTS				\
	{"",			"\tabort encoding for current stream."},

#define	ENCODE_START_MULTI_OPTIONS			\
	{"start-multi",	HAS_ARG,	0,	SPECIFY_MULTISTREAMS_START},
#define	ENCODE_START_MULTI_HINTS			\
	{"m~n",			"\tstart encoding for multiple streams. Maximum range is 0~3."},

#define	ENCODE_STOP_MULTI_OPTIONS			\
	{"stop-multi",		HAS_ARG,	0,	SPECIFY_MULTISTREAMS_STOP},
#define	ENCODE_STOP_MULTI_HINTS				\
	{"m~n",			"\tstop encoding for multiple streams. Maximum range is 0~3."},

#define	ENCODE_FORCE_IDR_OPTIONS			\
	{"force-idr",		NO_ARG,		0,	SPECIFY_FORCE_IDR},
#define	ENCODE_FORCE_IDR_HINTS				\
	{"",				"\t\tforce IDR at once for current stream"},

//immediate stop and start encoding
#define	ENCODE_RESTART_OPTIONS				\
	{"restart",		NO_ARG,		0,	ENCODING_RESTART},
#define	ENCODE_RESTART_HINTS					\
	{"",				"\t\trestart encoding for current stream"},

#define	ENCODE_FRAME_FACTOR_OPTIONS		\
	{"frame-factor",	HAS_ARG,	0,	FRAME_FACTOR},
#define	ENCODE_FRAME_FACTOR_HINTS				\
	{"1~255/1~255",	"change frame rate interval for current stream, numerator/denominator"},

#define	ENCODE_FRAME_FACTOR_SYNC_OPTIONS	\
	{"frame-factor-sync",	HAS_ARG,	0,	FRAME_FACTOR_SYNC},
#define	ENCODE_FRAME_FACTOR_SYNC_HINTS		\
	{"1~255/1~255",		"Simutaneously change frame rate interval for current streams "\
	"during encoding, numerator/denominator"},

#define	STREAM_ABS_FPS_OPTIONS	\
	{"stream-abs-fps",	HAS_ARG,	0,	STREAM_ABS_FPS},
#define	STREAM_ABS_FPS_HINTS	\
	{"1~60",	"set abs frame rate for streams"},

#define	SPECIFY_STREAM_ABS_FPS_OPTIONS	\
	{"stream-abs-fps-enable",	HAS_ARG,	0,	SPECIFY_STREAM_ABS_FPS},
#define	SPECIFY_STREAM_ABS_FPS_HINTS	\
	{"0|1",	"Enable setting abs frame rate. Default is 0\n"},

#define	ENCODE_LONG_REF_P_OPTIONS			\
	{"long-ref-p",		NO_ARG,		0,	SPECIFY_LONG_REF_P},
#define	ENCODE_LONG_REF_P_HINTS				\
	{"",				"\t\tInserted long term P frame will replace IDR and be referenced by later frames."},

#define	ENCODE_CONTROL_LONG_OPTIONS()		\
	ENCODE_START_OPTIONS			\
	ENCODE_STOP_OPTIONS			\
	ENCODE_ABORT_OPTIONS		\
	ENCODE_START_MULTI_OPTIONS		\
	ENCODE_STOP_MULTI_OPTIONS		\
	ENCODE_FORCE_IDR_OPTIONS		\
	ENCODE_RESTART_OPTIONS			\
	ENCODE_FRAME_FACTOR_OPTIONS	\
	ENCODE_FRAME_FACTOR_SYNC_OPTIONS	\
	STREAM_ABS_FPS_OPTIONS	\
	SPECIFY_STREAM_ABS_FPS_OPTIONS	\
	ENCODE_LONG_REF_P_OPTIONS

#define	ENCODE_CONTROL_PARAMETER_HINTS()		\
	ENCODE_START_HINTS		\
	ENCODE_STOP_HINTS			\
	ENCODE_ABORT_HINTS			\
	ENCODE_START_MULTI_HINTS	\
	ENCODE_STOP_MULTI_HINTS	\
	ENCODE_FORCE_IDR_HINTS		\
	ENCODE_RESTART_HINTS		\
	ENCODE_FRAME_FACTOR_HINTS	\
	ENCODE_FRAME_FACTOR_SYNC_HINTS	\
	STREAM_ABS_FPS_HINTS	\
	SPECIFY_STREAM_ABS_FPS_HINTS	\
	ENCODE_LONG_REF_P_HINTS

#define	ENCODE_CONTROL_INIT_PARAMETERS()		\
	case 'e':\
			VERIFY_STREAMID(current_stream);\
			start_stream_id |= (1 << current_stream);\
			break;\
	case 's':\
			VERIFY_STREAMID(current_stream);\
			stop_stream_id |= (1 << current_stream);\
			break;\
	case SPECIFY_STREAM_ABORT:\
			VERIFY_STREAMID(current_stream);\
			abort_stream_id |= (1 << current_stream);\
			break;\
	case SPECIFY_MULTISTREAMS_START:\
			if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {\
				return -1;\
			}\
			if (min_value < 0 || max_value >= MAX_ENCODE_STREAM_NUM || min_value > max_value) {\
				printf("Invalid stream ID range [%d~%d], it must be in the range of [0~%d].\n",\
					min_value, max_value, MAX_ENCODE_STREAM_NUM - 1);\
				return -1;\
			}\
			start_stream_id |= ((1 << (max_value + 1)) - 1U);\
			start_stream_id &= ~((1 << min_value) - 1U);\
			break;\
	case SPECIFY_MULTISTREAMS_STOP:\
			if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {\
				return -1;\
			}\
			if (min_value < 0 || max_value >= MAX_ENCODE_STREAM_NUM || min_value > max_value) {\
				printf("Invalid stream ID range [%d~%d], it must be in the range of [0~%d].\n",\
					min_value, max_value, MAX_ENCODE_STREAM_NUM - 1);\
				return -1;\
			}\
			stop_stream_id |= ((1 << (max_value + 1)) - 1U);\
			stop_stream_id &= ~((1 << min_value) - 1U);\
			break;\
	case SPECIFY_FORCE_IDR:\
			VERIFY_STREAMID(current_stream);\
			force_idr_id |= (1 << current_stream);\
			break;\
	case ENCODING_RESTART:\
			VERIFY_STREAMID(current_stream);\
			restart_stream_id |= (1 << current_stream);\
			break;\
	case FRAME_FACTOR:\
			VERIFY_STREAMID(current_stream);\
			if (get_two_unsigned_int(optarg, &numerator, &denominator, '/') < 0) {\
				return -1;\
			}\
			framerate_factor_changed_id |= (1 << current_stream);\
			framerate_factor[current_stream][0] = numerator;\
			framerate_factor[current_stream][1] = denominator;\
			break;\
	case FRAME_FACTOR_SYNC:\
			VERIFY_STREAMID(current_stream);\
			if (get_two_unsigned_int(optarg, &numerator, &denominator, '/') < 0) {\
				return -1;\
			}\
			framerate_factor_sync_id |= (1 << current_stream);\
			framerate_factor[current_stream][0] = numerator;\
			framerate_factor[current_stream][1] = denominator;\
			break;\
	case STREAM_ABS_FPS:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			if (min_value < MIN_ABS_FPS || min_value > MAX_ABS_FPS) {\
				printf("Invalid value [%d], must be in [1~60].\n", min_value);\
				return -1;\
			}\
			stream_abs_fps[current_stream] = min_value;\
			stream_abs_fps_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_STREAM_ABS_FPS:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			if (min_value != 0 && min_value != 1) {\
				printf("Invalid value [%d], must be in [0|1].\n", min_value);\
				return -1;\
			}\
			stream_abs_fps_enable[current_stream] = min_value;\
			stream_abs_fps_enabled_id |= (1 << current_stream);\
			break;\
	case SPECIFY_LONG_REF_P:\
			VERIFY_STREAMID(current_stream);\
			long_ref_p_id |= (1 << current_stream);\
			break;

/***************************************************************
	H264 ENCODE command line options
****************************************************************/
#define	H264ENC_M_OPTIONS	{"M",	HAS_ARG,	0,	'M'},
#define	H264ENC_M_HINTS		{"1~3",	"\t\tH.264 GOP parameter M"},

#define	H264ENC_N_OPTIONS	{"N",	HAS_ARG,	0,	'N'},
#define	H264ENC_N_HINTS		\
	{"1~4095",	"\t\tH.264 GOP parameter N, must be multiple of M, can be changed during encoding"},

#define	H264ENC_IDR_OPTIONS	{"idr",	HAS_ARG,	0,	SPECIFY_GOP_IDR},
#define	H264ENC_IDR_HINTS		\
	{"1~128",	"\thow many GOP's, an IDR picture should happen, can be changed during encoding"},

#define	H264ENC_GOP_OPTIONS	{"gop",	HAS_ARG,	0,	SPECIFY_GOP_MODEL},
#define	H264ENC_GOP_HINTS	{"0|1|2|3|6|8", \
	"\tGOP model, 0: simple GOP, 1: advanced GOP, 2: 2 level SVCT, 3: 3 level SVCT,"	\
				" 6: unchained GOP model, 8: long reference P GOP model"},

#define	H264ENC_BC_OPTIONS	{"bc",	HAS_ARG,	0,	BITRATE_CONTROL},
#define	H264ENC_BC_HINTS		\
	{"cbr|vbr|cbr-quality|vbr-quality|cbr2", "\tbitrate control method"},

#define	H264ENC_CBR_BITRATE_OPTIONS		\
	{"bitrate",	HAS_ARG,	0,	SPECIFY_CBR_BITRATE},
#define	H264ENC_CBR_BITRATE_HINTS		\
	{"value",		"\tset cbr average bitrate, can be changed during encoding"},

#define	H264ENC_VBR_BITRATE_OPTIONS		\
	{"vbr-bitrate",HAS_ARG,	0,	SPECIFY_VBR_BITRATE},
#define	H264ENC_VBR_BITRATE_HINTS		\
	{"min~max",	"set vbr bitrate range, can be changed during encoding"},

#define	H264ENC_QP_LIMIT_I_OPTIONS		\
	{"qp-limit-i", 	HAS_ARG, 	0, 	CHANGE_QP_LIMIT_I},
#define	H264ENC_QP_LIMIT_I_HINTS			\
	{"0~51",		"\tset I-frame qp limit range, 0:auto 1~51:qp limit range"},

#define	H264ENC_QP_LIMIT_P_OPTIONS		\
	{"qp-limit-p", HAS_ARG, 	0, 	CHANGE_QP_LIMIT_P},
#define	H264ENC_QP_LIMIT_P_HINTS			\
	{"0~51",		"\tset P-frame qp limit range, 0:auto 1~51:qp limit range"},

#define	H264ENC_QP_LIMIT_B_OPTIONS		\
	{"qp-limit-b", HAS_ARG, 	0, 	CHANGE_QP_LIMIT_B},
#define	H264ENC_QP_LIMIT_B_HINTS			\
	{"0~51",		"\tset B-frame qp limit range, 0:auto 1~51:qp limit range"},

#define	H264ENC_QP_LIMIT_Q_OPTIONS		\
	{"qp-limit-q", HAS_ARG, 	0, 	CHANGE_QP_LIMIT_Q},
#define	H264ENC_QP_LIMIT_Q_HINTS			\
	{"0~51",		"\tset Q-frame qp limit range, 0:auto 1~51:qp limit range"},

#define	H264ENC_ADAPT_QP_OPTIONS		\
	{"adapt-qp",	HAS_ARG, 	0, 	CHANGE_ADAPT_QP},
#define	H264ENC_ADAPT_QP_HINTS			\
	{"0~4",		"\tset strength of adaptive qp"},

#define	H264ENC_I_QP_REDUCE_OPTIONS		\
	{"i-qp-reduce",	HAS_ARG, 	0, 	CHANGE_I_QP_REDUCE},
#define	H264ENC_I_QP_REDUCE_HINTS		\
	{"1~10",			"\tset diff of I QP less than P QP"},

#define	H264ENC_P_QP_REDUCE_OPTIONS	\
	{"p-qp-reduce",		HAS_ARG, 	0, 	CHANGE_P_QP_REDUCE},
#define	H264ENC_P_QP_REDUCE_HINTS		\
	{"1~5", "\tset diff of P QP less than B QP"},

#define	H264ENC_Q_QP_REDUCE_OPTIONS		\
	{"q-qp-reduce",	HAS_ARG, 	0, 	CHANGE_Q_QP_REDUCE},
#define	H264ENC_Q_QP_REDUCE_HINTS		\
	{"1~10",			"\tset diff of Q QP less than P QP"},

#define	H264ENC_LOG_Q_NUM_PLUS_1_OPTIONS		\
	{"log-q-num-plus-1",	HAS_ARG, 	0, 	CHANGE_LOG_Q_NUM_PLUS_1},
#define	H264ENC_LOG_Q_NUM_PLUS_1_HINTS		\
		{"0~4", 		"set Q frame number for one GOP, 0: no Q frame, 1: 1 Q frame, 2: 3 Q frames,"\
		"\n\t\t\t\t3: 7 Q frames, 4: 15 Q frames"},

#define	H264ENC_SKIP_FRAME_OPTIONS		\
	{"skip-frame-mode",	HAS_ARG, 	0, 	CHANGE_SKIP_FRAME_MODE},
#define	H264ENC_SKIP_FRAME_HINTS			\
	{"0|1|2",			"0: disable, 1: skip based on CPB size, 2: skip based on target bitrate and max QP"},

#define	H264ENC_MAX_I_SIZE_KB_OPTIONS		\
	{"max-i-size-KB",	HAS_ARG, 	0, 	CHANGE_MAX_I_SIZE_KB},
#define	H264ENC_MAX_I_SIZE_KB_HINTS		\
	{"0~8192",			"set max size for I frame in KB, 0: disable max I size, > 0: max size for I frame in KB"},

#define	H264ENC_MB_ROWS_OPTIONS		\
	{"intra-mb-rows",	HAS_ARG, 	0, 	CHANGE_INTRA_MB_ROWS},
#define	H264ENC_MB_ROWS_HINTS			\
	{"0~n",			"set intra refresh MB rows number, default value is 0, which means disable"},

#define	H264ENC_QM_DELTA_OPTIONS 		\
	{"qm-delta",	HAS_ARG,	0,	CHANGE_QP_MATRIX_DELTA},
#define	H264ENC_QM_DELTA_HINTS 			\
	{"-50~50",		"\tset QP Matrix delta value in format of 'd0,d1,d2,d3'"},

#define	H264ENC_QM_MODE_OPTIONS		\
	{"qm-mode",	HAS_ARG,	0,	CHANGE_QP_MATRIX_MODE},
#define	H264ENC_QM_MODE_HINTS			\
	{"0~4",		"\tset QP Matrix mode, 0: default; 1: skip left region; 2: skip right; 3: skip top; 4: skip bottom"},

#define	H264ENC_DEBLOCKING_ALPHA_OPTIONS		\
	{"deblocking-alpha",	HAS_ARG,	0,	DEBLOCKING_ALPHA},
#define	H264ENC_DEBLOCKING_ALPHA_HINTS			\
	{"-6~6",				"deblocking-alpha"},

#define	H264ENC_DEBLOCKING_BETA_OPTIONS		\
	{"deblocking-beta",	HAS_ARG,	0,	DEBLOCKING_BETA},
#define	H264ENC_DEBLOCKING_BETA_HINTS			\
	{"-6~6",				"deblocking-beta"},

#define	H264ENC_DEBLOCKING_ENABLE_OPTIONS		\
	{"deblocking-enable",	HAS_ARG,	0,	DEBLOCKING_ENABLE},
#define	H264ENC_DEBLOCKING_ENABLE_HINTS			\
	{"0|1|2", 			"deblocking-enable, 2 is auto and default value"},

#define	H264ENC_FRAME_CORP_LEFT_OPTIONS		\
	{"left-frame-crop",	HAS_ARG,	0,	LEFT_FRAME_CROP},
#define	H264ENC_FRAME_CORP_LEFT_HINTS			\
	{"offset", 			"left offset of frame crop"},

#define	H264ENC_FRAME_CORP_RIGHT_OPTIONS		\
	{"right-frame-crop",	HAS_ARG,	0,	RIGHT_FRAME_CROP},
#define	H264ENC_FRAME_CORP_RIGHT_HINTS			\
	{"offset", 			"right offset of frame crop"},

#define	H264ENC_FRAME_CORP_TOP_OPTIONS		\
	{"top-frame-crop",	HAS_ARG,	0,	TOP_FRAME_CROP},
#define	H264ENC_FRAME_CORP_TOP_HINTS			\
	{"offset", 			"top offset of frame crop"},

#define	H264ENC_FRAME_CORP_BOTTOM_OPTIONS		\
	{"bottom-frame-crop",	HAS_ARG,	0,	BOTTOM_FRAME_CROP},
#define	H264ENC_FRAME_CORP_BOTTOM_HINTS			\
	{"offset", 			"bottom offset of frame crop"},

#define	H264ENC_PROFILE_OPTIONS					\
	{"profile",	HAS_ARG,	0,	SPECIFY_PROFILE_LEVEL},
#define	H264ENC_PROFILE_HINTS						\
	{"0|1|2",	"\tH264 profile level, 0 is baseline, 1 is main, 2 is high, default is 1"},

#define	H264ENC_INTLC_IFRAME_OPTIONS			\
	{"intlc_iframe",	HAS_ARG,	0,	INTLC_IFRAME},
#define	H264ENC_INTLC_IFRAME_HINTS				\
	{"0|1",			"\tInterlaced Encoding 0:default, 1: force two fields be I-picture"},

#define	H264ENC_MV_IMPROVE_OPTIONS				\
	{"mv-threshold",	HAS_ARG, 	0, 	SPECIFY_MV_THRESHOLD},
#define	H264ENC_MV_IMPROVE_HINTS				\
	{"0|1",		"\tdisable/enable zmv threshold roi for current stream, 0: disable, 1: enable, default 0"},

#define	H264ENC_FLAT_AREA_IMPROVE_OPTIONS					\
	{"flat-area-improve",	HAS_ARG, 	0, 	SPECIFY_FLAT_AREA_IMPROVE},
#define	H264ENC_FLAT_AREA_IMPROVE_HINTS					\
	{"0|1",			"disable/enable h264 encode improvement for flat area, default is 0, which means disable"},

#define	H264ENC_FAST_SEEK_INTVL_OPTIONS				\
	{"fast-seek-intvl",	HAS_ARG, 	0, 	SPECIFY_FAST_SEEK_INTVL},
#define	H264ENC_FAST_SEEK_INTVL_HINTS				\
	{"0~63",			"Specify fast seek P frame interval"},

#define	H264ENC_MULTI_REFP_OPTIONS				\
	{"multi-ref-p",	HAS_ARG, 	0, 	SPECIFY_MULTI_REF_P},
#define	H264ENC_MULTI_REFP_HINTS				\
	{"0|1",			"\tdisable/enable multiple references for P frame, 0:disable, 1:enable"},

#define	H264ENC_MAX_GOPM_OPTIONS				\
	{"max-gop-M",	HAS_ARG, 	0, 	SPECIFY_MAX_GOP_M},
#define	H264ENC_MAX_GOPM_HINTS					\
	{"0~3",			"\tmax GOP M for stream, support B frame when value > 1, 0 for mjpeg encoding"},

#define	H264ENC_FORCE_SEEK_OPTIONS				\
	{"force-fast-seek",	NO_ARG,		0,	SPECIFY_FORCE_FAST_SEEK},
#define	H264ENC_FORCE_SEEK_HINTS					\
	{"",					"\tforce fast seek frame at once for current stream, used in gop 8"},

#define	H264ENC_FRAME_DROP_OPTIONS				\
	{"frame-drop",		HAS_ARG,	0,	SPECIFY_FRAME_DROP},
#define	H264ENC_FRAME_DROP_HINTS				\
	{"0~255",			"\tSpecify how many frames encoder will drop, can update on the fly"},

#define	H264ENC_INTRABIAS_P_OPTIONS				\
	{"intrabias-p",		HAS_ARG,	0,	SPECIFY_INTRABIAS_P},
#define	H264ENC_INTRABIAS_P_HINTS				\
	{"1~4000",			"Specify intrabias for P frames of current stream"},

#define	H264ENC_INTRABIAS_B_OPTIONS				\
	{"intrabias-b",		HAS_ARG,	0,	SPECIFY_INTRABIAS_B},
#define	H264ENC_INTRABIAS_B_HINTS				\
	{"1~4000",			"Specify intrabias for B frames of current stream"},

#define	H264ENC_USER1_INTRABIAS_OPTIONS			\
	{"user1-intrabias",		HAS_ARG,	0,	SPECIFY_USER1_INTRABIAS},
#define	H264ENC_USER1_INTRABIAS_HINTS			\
	{"0~9",				"Specify user1 intra bias strength, 0: no bias, 9: the strongest."},

#define	H264ENC_USER1_DIRECTBIAS_OPTIONS		\
	{"user1-directbias",	HAS_ARG,	0,	SPECIFY_USER1_DIRECTBIAS},
#define	H264ENC_USER1_DIRECTBIAS_HINTS			\
	{"0~9",				"Specify user1 direct bias strength, 0: no bias, 9: the strongest."},

#define	H264ENC_USER2_INTRABIAS_OPTIONS			\
	{"user2-intrabias",		HAS_ARG,	0,	SPECIFY_USER2_INTRABIAS},
#define	H264ENC_USER2_INTRABIAS_HINTS			\
	{"0~9",				"Specify user2 intra bias strength, 0: no bias, 9: the strongest."},

#define	H264ENC_USER2_DIRECTBIAS_OPTIONS		\
	{"user2-directbias",	HAS_ARG,	0,	SPECIFY_USER2_DIRECTBIAS},
#define	H264ENC_USER2_DIRECTBIAS_HINTS			\
	{"0~9",				"Specify user2 direct bias strength, 0: no bias, 9: the strongest."},

#define	H264ENC_USER3_INTRABIAS_OPTIONS			\
	{"user3-intrabias",		HAS_ARG,	0,	SPECIFY_USER3_INTRABIAS},
#define	H264ENC_USER3_INTRABIAS_HINTS			\
	{"0~9",				"Specify user3 intra bias strength, 0: no bias, 9: the strongest."},

#define	H264ENC_USER3_DIRECTBIAS_OPTIONS		\
	{"user3-directbias",	HAS_ARG,	0,	SPECIFY_USER3_DIRECTBIAS},
#define	H264ENC_USER3_DIRECTBIAS_HINTS			\
	{"0~9",				"Specify user3 direct bias strength, 0: no bias, 9: the strongest."},

//H.264 syntax options
#define	H264ENC_AU_TYPE_OPTIONS				\
	{"au-type",			HAS_ARG,	0,	SPECIFY_AU_TYPE},
#define	H264ENC_AU_TYPE_HINTS				\
	{"0~3",		"\t0: No AUD, No SEI; 1: AUD before SPS, PPS, with SEI; 2: AUD after SPS, "\
	"PPS, with SEI; 3: No AUD, with SEI.\n"},

#define	H264ENC_ABS_BR_OPTIONS				\
	{"abs-br",			HAS_ARG,	0,	SPECIFY_ABS_BR_FLAG},
#define	H264ENC_ABS_BR_HINTS				\
	{"0|1",		"Enable absolute bitrate, 0: disable, 1: enable\n"},

#define	H264ENC_REPEAT_PSKIP_OPTIONS			\
	{"force-pskip-repeat",		HAS_ARG,	0,	SPECIFY_FORCE_PSKIP_REPEAT},
#define	H264ENC_REPEAT_PSKIP_HINTS				\
	{"0|1", 	"Repeatly generate P-skip or force pskip at once for current stream. 0: no-repeat, 1: repeat."},

#define	H264ENC_REPEAT_PSKIP_NUM_OPTIONS			\
	{"repeat-pskip-num",		HAS_ARG,	0,	SPECIFY_REPEAT_PSKIP_NUM},
#define	H264ENC_REPEAT_PSKIP_NUM_HINTS				\
	{"0~254",	"P-skip number when repeat pattern is ON."},

#define	H264_ENCODE_LONG_OPTIONS()		\
	H264ENC_M_OPTIONS				\
	H264ENC_N_OPTIONS				\
	H264ENC_IDR_OPTIONS			\
	H264ENC_GOP_OPTIONS			\
	H264ENC_BC_OPTIONS				\
	H264ENC_CBR_BITRATE_OPTIONS	\
	H264ENC_VBR_BITRATE_OPTIONS	\
	H264ENC_QP_LIMIT_I_OPTIONS		\
	H264ENC_QP_LIMIT_P_OPTIONS		\
	H264ENC_QP_LIMIT_B_OPTIONS		\
	H264ENC_QP_LIMIT_Q_OPTIONS		\
	H264ENC_ADAPT_QP_OPTIONS		\
	H264ENC_I_QP_REDUCE_OPTIONS	\
	H264ENC_P_QP_REDUCE_OPTIONS	\
	H264ENC_Q_QP_REDUCE_OPTIONS	\
	H264ENC_LOG_Q_NUM_PLUS_1_OPTIONS	\
	H264ENC_SKIP_FRAME_OPTIONS		\
	H264ENC_MAX_I_SIZE_KB_OPTIONS	\
	H264ENC_MB_ROWS_OPTIONS		\
	H264ENC_QM_DELTA_OPTIONS		\
	H264ENC_QM_MODE_OPTIONS		\
	H264ENC_DEBLOCKING_ALPHA_OPTIONS		\
	H264ENC_DEBLOCKING_BETA_OPTIONS		\
	H264ENC_DEBLOCKING_ENABLE_OPTIONS		\
	H264ENC_FRAME_CORP_LEFT_OPTIONS		\
	H264ENC_FRAME_CORP_RIGHT_OPTIONS		\
	H264ENC_FRAME_CORP_TOP_OPTIONS		\
	H264ENC_FRAME_CORP_BOTTOM_OPTIONS		\
	H264ENC_PROFILE_OPTIONS					\
	H264ENC_INTLC_IFRAME_OPTIONS			\
	H264ENC_MV_IMPROVE_OPTIONS				\
	H264ENC_FLAT_AREA_IMPROVE_OPTIONS				\
	H264ENC_FAST_SEEK_INTVL_OPTIONS			\
	H264ENC_MULTI_REFP_OPTIONS				\
	H264ENC_MAX_GOPM_OPTIONS				\
	H264ENC_FORCE_SEEK_OPTIONS				\
	H264ENC_FRAME_DROP_OPTIONS			\
	H264ENC_INTRABIAS_P_OPTIONS			\
	H264ENC_INTRABIAS_B_OPTIONS			\
	H264ENC_USER1_INTRABIAS_OPTIONS		\
	H264ENC_USER1_DIRECTBIAS_OPTIONS		\
	H264ENC_USER2_INTRABIAS_OPTIONS		\
	H264ENC_USER2_DIRECTBIAS_OPTIONS		\
	H264ENC_USER3_INTRABIAS_OPTIONS		\
	H264ENC_USER3_DIRECTBIAS_OPTIONS		\
	H264ENC_AU_TYPE_OPTIONS				\
	H264ENC_ABS_BR_OPTIONS				\
	H264ENC_REPEAT_PSKIP_OPTIONS		\
	H264ENC_REPEAT_PSKIP_NUM_OPTIONS

#define	H264_ENCODE_PARAMETER_HINTS()		\
	H264ENC_M_HINTS		\
	H264ENC_N_HINTS		\
	H264ENC_IDR_HINTS	\
	H264ENC_GOP_HINTS	\
	H264ENC_BC_HINTS	\
	H264ENC_CBR_BITRATE_HINTS	\
	H264ENC_VBR_BITRATE_HINTS	\
	H264ENC_QP_LIMIT_I_HINTS	\
	H264ENC_QP_LIMIT_P_HINTS	\
	H264ENC_QP_LIMIT_B_HINTS	\
	H264ENC_QP_LIMIT_Q_HINTS	\
	H264ENC_ADAPT_QP_HINTS		\
	H264ENC_I_QP_REDUCE_HINTS	\
	H264ENC_P_QP_REDUCE_HINTS	\
	H264ENC_Q_QP_REDUCE_HINTS	\
	H264ENC_LOG_Q_NUM_PLUS_1_HINTS \
	H264ENC_SKIP_FRAME_HINTS	\
	H264ENC_MAX_I_SIZE_KB_HINTS	\
	H264ENC_MB_ROWS_HINTS		\
	H264ENC_QM_DELTA_HINTS		\
	H264ENC_QM_MODE_HINTS		\
	H264ENC_DEBLOCKING_ALPHA_HINTS		\
	H264ENC_DEBLOCKING_BETA_HINTS		\
	H264ENC_DEBLOCKING_ENABLE_HINTS	\
	H264ENC_FRAME_CORP_LEFT_HINTS		\
	H264ENC_FRAME_CORP_RIGHT_HINTS		\
	H264ENC_FRAME_CORP_TOP_HINTS		\
	H264ENC_FRAME_CORP_BOTTOM_HINTS		\
	H264ENC_PROFILE_HINTS		\
	H264ENC_INTLC_IFRAME_HINTS	\
	H264ENC_MV_IMPROVE_HINTS	\
	H264ENC_FLAT_AREA_IMPROVE_HINTS		\
	H264ENC_FAST_SEEK_INTVL_HINTS	\
	H264ENC_MULTI_REFP_HINTS	\
	H264ENC_MAX_GOPM_HINTS		\
	H264ENC_FORCE_SEEK_HINTS	\
	H264ENC_FRAME_DROP_HINTS	\
	H264ENC_INTRABIAS_P_HINTS	\
	H264ENC_INTRABIAS_B_HINTS	\
	H264ENC_USER1_INTRABIAS_HINTS	\
	H264ENC_USER1_DIRECTBIAS_HINTS	\
	H264ENC_USER2_INTRABIAS_HINTS	\
	H264ENC_USER2_DIRECTBIAS_HINTS	\
	H264ENC_USER3_INTRABIAS_HINTS	\
	H264ENC_USER3_DIRECTBIAS_HINTS	\
	H264ENC_AU_TYPE_HINTS			\
	H264ENC_ABS_BR_HINTS			\
	H264ENC_REPEAT_PSKIP_HINTS		\
	H264ENC_REPEAT_PSKIP_NUM_HINTS

#define	H264_ENCODE_INIT_PARAMETERS()		\
	case 'M':\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_M = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_M_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			GOP_changed_id |= (1 << current_stream);\
			break;\
	case 'N':\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_N = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_N_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			GOP_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_GOP_IDR:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_idr_interval = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_idr_interval_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			GOP_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_GOP_MODEL:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_gop_model = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_gop_model_flag  = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case BITRATE_CONTROL:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_bitrate_control = get_bitrate_control(optarg);\
			encode_param[current_stream].h264_param.h264_bitrate_control_flag  = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_CBR_BITRATE:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_cbr_avg_bitrate = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_cbr_bitrate_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_VBR_BITRATE:\
			VERIFY_STREAMID(current_stream);\
			if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {\
				return -1;\
			}\
			encode_param[current_stream].h264_param.h264_vbr_min_bitrate = min_value;\
			encode_param[current_stream].h264_param.h264_vbr_max_bitrate = max_value;\
			encode_param[current_stream].h264_param.h264_vbr_bitrate_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case CHANGE_QP_LIMIT_I:\
			VERIFY_STREAMID(current_stream);\
			if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {\
				return -1;\
			}\
			qp_limit_changed_id |= (1 << current_stream);\
			qp_limit_param[current_stream].qp_min_i = min_value;\
			qp_limit_param[current_stream].qp_max_i = max_value;\
			qp_limit_param[current_stream].qp_i_flag = 1;\
			break;\
	case CHANGE_QP_LIMIT_P:\
			VERIFY_STREAMID(current_stream);\
			if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {\
				return -1;\
			}\
			qp_limit_changed_id |= (1 << current_stream);\
			qp_limit_param[current_stream].qp_min_p = min_value;\
			qp_limit_param[current_stream].qp_max_p = max_value;\
			qp_limit_param[current_stream].qp_p_flag = 1;\
			break;\
	case CHANGE_QP_LIMIT_B:\
			VERIFY_STREAMID(current_stream);\
			if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {\
				return -1;\
			}\
			qp_limit_changed_id |= (1 << current_stream);\
			qp_limit_param[current_stream].qp_min_b = min_value;\
			qp_limit_param[current_stream].qp_max_b = max_value;\
			qp_limit_param[current_stream].qp_b_flag = 1;\
			break;\
	case CHANGE_QP_LIMIT_Q:\
			VERIFY_STREAMID(current_stream);\
			if (get_two_unsigned_int(optarg, &min_value, &max_value, '~') < 0) {\
				return -1;\
			}\
			qp_limit_changed_id |= (1 << current_stream);\
			qp_limit_param[current_stream].qp_min_q = min_value;\
			qp_limit_param[current_stream].qp_max_q = max_value;\
			qp_limit_param[current_stream].qp_q_flag = 1;\
			break;\
	case CHANGE_ADAPT_QP:\
			VERIFY_STREAMID(current_stream);\
			qp_limit_changed_id |= (1 << current_stream);\
			qp_limit_param[current_stream].adapt_qp = atoi(optarg);\
			qp_limit_param[current_stream].adapt_qp_flag = 1;\
			break;\
	case CHANGE_I_QP_REDUCE:\
			VERIFY_STREAMID(current_stream);\
			qp_limit_changed_id |= (1 << current_stream);\
			qp_limit_param[current_stream].i_qp_reduce = atoi(optarg);\
			qp_limit_param[current_stream].i_qp_reduce_flag = 1;\
			break;\
	case CHANGE_P_QP_REDUCE:\
			VERIFY_STREAMID(current_stream);\
			qp_limit_changed_id |= (1 << current_stream);\
			qp_limit_param[current_stream].p_qp_reduce = atoi(optarg);\
			qp_limit_param[current_stream].p_qp_reduce_flag = 1;\
			break;\
	case CHANGE_Q_QP_REDUCE:\
			VERIFY_STREAMID(current_stream);\
			qp_limit_changed_id |= (1 << current_stream);\
			qp_limit_param[current_stream].q_qp_reduce = atoi(optarg);\
			qp_limit_param[current_stream].q_qp_reduce_flag = 1;\
			break;\
	case CHANGE_LOG_Q_NUM_PLUS_1:\
			VERIFY_STREAMID(current_stream);\
			qp_limit_changed_id |= (1 << current_stream);\
			qp_limit_param[current_stream].log_q_num_plus_1 = atoi(optarg);\
			qp_limit_param[current_stream].log_q_num_plus_1_flag = 1;\
			break;\
	case CHANGE_SKIP_FRAME_MODE:\
			VERIFY_STREAMID(current_stream);\
			qp_limit_changed_id |= (1 << current_stream);\
			qp_limit_param[current_stream].skip_frame = atoi(optarg);\
			qp_limit_param[current_stream].skip_frame_flag = 1;\
			break;\
	case CHANGE_MAX_I_SIZE_KB:\
			VERIFY_STREAMID(current_stream);\
			qp_limit_changed_id |= (1 << current_stream);\
			qp_limit_param[current_stream].max_i_size_KB = atoi(optarg);\
			qp_limit_param[current_stream].max_i_size_KB_flag = 1;\
			break;\
	case CHANGE_INTRA_MB_ROWS:\
			VERIFY_STREAMID(current_stream);\
			intra_mb_rows[current_stream] = atoi(optarg);\
			intra_mb_rows_changed_id |= (1 << current_stream);\
			break;\
	case CHANGE_QP_MATRIX_DELTA:\
			VERIFY_STREAMID(current_stream);\
			if (get_multi_s8_arg(optarg, qp_matrix_param[current_stream].delta, 4) < 0) {\
				printf("need %d args for qp matrix delta array.\n", 4);\
				return -1;\
			}\
			qp_matrix_param[current_stream].delta_flag = 1;\
			qp_matrix_changed_id |= (1 << current_stream);\
			break;\
	case CHANGE_QP_MATRIX_MODE:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			if (min_value > 4) {\
				printf("Invalid QP Matrix mode [%d], please choose from 0~4.\n", min_value);\
				return -1;\
			}\
			qp_matrix_param[current_stream].matrix_mode = min_value;\
			qp_matrix_param[current_stream].matrix_mode_flag = 1;\
			qp_matrix_changed_id |= (1 << current_stream);\
			printf("set qp matrix mode : %d.\n", min_value);\
			break;\
	case DEBLOCKING_ALPHA:\
			VERIFY_STREAMID(current_stream);\
			if (atoi(optarg) > 6 || atoi(optarg) < -6) {\
				printf("Invalid param for alpha, range [-6~6].\n");\
				return -1;\
			}\
			encode_param[current_stream].h264_param.h264_deblocking_filter_alpha = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_deblocking_filter_alpha_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case DEBLOCKING_BETA:\
			VERIFY_STREAMID(current_stream);\
			if (atoi(optarg) > 6 || atoi(optarg) < -6) {\
				printf("Invalid param for beta, range [-6~6].\n");\
				return -1;\
			}\
			encode_param[current_stream].h264_param.h264_deblocking_filter_beta = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_deblocking_filter_beta_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case DEBLOCKING_ENABLE:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			if (min_value > 2) {\
				printf("Invalid param for loop filter, range [0|1|2].\n");\
				return -1;\
			}\
			encode_param[current_stream].h264_param.h264_deblocking_filter_enable = min_value;\
			encode_param[current_stream].h264_param.h264_deblocking_filter_enable_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case LEFT_FRAME_CROP:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_frame_crop_left_offset = min_value;\
			encode_param[current_stream].h264_param.h264_frame_crop_left_offset_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case RIGHT_FRAME_CROP:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_frame_crop_right_offset = min_value;\
			encode_param[current_stream].h264_param.h264_frame_crop_right_offset_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case TOP_FRAME_CROP:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_frame_crop_top_offset = min_value;\
			encode_param[current_stream].h264_param.h264_frame_crop_top_offset_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case BOTTOM_FRAME_CROP:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_frame_crop_bottom_offset = min_value;\
			encode_param[current_stream].h264_param.h264_frame_crop_bottom_offset_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_PROFILE_LEVEL:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_profile_level = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_profile_level_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			if (check_encode_profile(current_stream) < 0)\
				return -1;\
			break;\
	case INTLC_IFRAME:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.force_intlc_tb_iframe = atoi(optarg);\
			encode_param[current_stream].h264_param.force_intlc_tb_iframe_flag  = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_MV_THRESHOLD:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_mv_threshold = !!(atoi(optarg));\
			encode_param[current_stream].h264_param.h264_mv_threshold_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_FLAT_AREA_IMPROVE:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for encode improve option [0|1].\n");\
				return -1;\
			}\
			encode_param[current_stream].h264_param.h264_flat_area_improve = min_value;\
			encode_param[current_stream].h264_param.h264_flat_area_improve_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_FAST_SEEK_INTVL:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_fast_seek_intvl = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_fast_seek_intvl_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_MULTI_REF_P:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for multi_ref_p option [0|1].\n");\
				return -1;\
			}\
			encode_param[current_stream].h264_param.h264_multi_ref_p = min_value;\
			encode_param[current_stream].h264_param.h264_multi_ref_p_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_MAX_GOP_M:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			if (min_value > 3) {\
				printf("Invalid param for max gop M [0~3].\n");\
				return -1;\
			}\
			system_resource_setup.max_gop_M[current_stream] = min_value;\
			system_resource_setup.max_gop_M_flag[current_stream] = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_FORCE_FAST_SEEK:\
			VERIFY_STREAMID(current_stream);\
			force_fast_seek_id |= (1 << current_stream);\
			break;\
	case SPECIFY_FRAME_DROP:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_drop_frames = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_drop_frames_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_INTRABIAS_P:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_intrabias_p = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_intrabias_p_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_INTRABIAS_B:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_intrabias_b = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_intrabias_b_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_USER1_INTRABIAS:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_user1_intra_bias = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_user1_intra_bias_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_USER1_DIRECTBIAS:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_user1_direct_bias = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_user1_direct_bias_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_USER2_INTRABIAS:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_user2_intra_bias = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_user2_intra_bias_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_USER2_DIRECTBIAS:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_user2_direct_bias = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_user2_direct_bias_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_USER3_INTRABIAS:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_user3_intra_bias = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_user3_intra_bias_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_USER3_DIRECTBIAS:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.h264_user3_direct_bias = atoi(optarg);\
			encode_param[current_stream].h264_param.h264_user3_direct_bias_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_AU_TYPE:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.au_type = atoi(optarg);\
			encode_param[current_stream].h264_param.au_type_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_ABS_BR_FLAG:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			if (min_value != 0 && min_value != 1) {\
				printf("Invalid value [%d], must be in [0|1].\n", min_value);\
				return -1;\
			}\
			encode_param[current_stream].h264_param.h264_abs_br = min_value;\
			encode_param[current_stream].h264_param.h264_abs_br_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_FORCE_PSKIP_REPEAT:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.force_pskip_repeat_enable = !!(atoi(optarg));\
			encode_param[current_stream].h264_param.force_pskip_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_REPEAT_PSKIP_NUM:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			if (min_value < 0 || min_value > 254) {\
				printf("Invalid value [%d], must be [0~254].\n", min_value);\
				return -1;\
			}\
			encode_param[current_stream].h264_param.force_pskip_repeat_num = min_value;\
			break;

/***************************************************************
	PANIC command line options
****************************************************************/
#define	PANIC_CPB_BUF_IDC_OPTIONS	{"cpb-buf-idc", HAS_ARG, 0, CPB_BUF_IDC},
#define	PANIC_CPB_BUF_IDC_HINTS		\
	{"0~31",	"\tspecify cpb buffer size. 0: using buffer size based on the encoding size," \
	"\n\t\t\t\t31: using user-defined cbp size set in cpb_user_size field" \
	"\n\t\t\t\tothers: cpb size is the in seconds multipled by the bitrate"},

#define	PANIC_CPB_CMP_IDC_OPTIONS	{"cpb-cmp-idc", 	HAS_ARG, 0, CPB_CMP_IDC},
#define	PANIC_CPB_CMP_IDC_HINTS		{"0|1",			"\tenable the rate control"},

#define	PANIC_EN_RC_OPTIONS	{"en-panic-rc", HAS_ARG, 0, ENABLE_PANIC_RC},
#define	PANIC_EN_RC_HINTS		{"0|1",	"\t0: disable panic mode;\t1: enable panic mode"},

#define	PANIC_FAST_RC_IDC_OPTIONS	{"fast-rc-idc",		HAS_ARG, 0, FAST_RC_IDC},
#define	PANIC_FAST_RC_IDC_HINTS		\
	{"0|2",	"\tfast rate control reaction time. \t0: no fast RC; 2: use fast RC"},

#define	PANIC_CPB_USER_SIZE_OPTIONS	{"cpb-user-size",	HAS_ARG, 0, CPB_USER_SIZE},
#define	PANIC_CPB_USER_SIZE_HINTS	\
	{"value",	"user defined cpb buffer size. It will be used only when cpb_buf_idc is 0x1F\n"},

#define	PANIC_LONG_OPTIONS()		\
	PANIC_CPB_BUF_IDC_OPTIONS	\
	PANIC_CPB_CMP_IDC_OPTIONS	\
	PANIC_EN_RC_OPTIONS		\
	PANIC_FAST_RC_IDC_OPTIONS	\
	PANIC_CPB_USER_SIZE_OPTIONS

#define	PANIC_PARAMETER_HINTS()		\
	PANIC_CPB_BUF_IDC_HINTS		\
	PANIC_CPB_CMP_IDC_HINTS		\
	PANIC_EN_RC_HINTS			\
	PANIC_FAST_RC_IDC_HINTS		\
	PANIC_CPB_USER_SIZE_HINTS

#define	PANIC_INIT_PARAMETERS()		\
	case CPB_BUF_IDC:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.cpb_buf_idc = atoi(optarg);\
			encode_param[current_stream].h264_param.panic_mode_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case CPB_CMP_IDC:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.cpb_cmp_idc = atoi(optarg);\
			encode_param[current_stream].h264_param.panic_mode_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case ENABLE_PANIC_RC:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.en_panic_rc = atoi(optarg);\
			encode_param[current_stream].h264_param.panic_mode_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case FAST_RC_IDC:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.fast_rc_idc = atoi(optarg);\
			encode_param[current_stream].h264_param.panic_mode_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;\
	case CPB_USER_SIZE:\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].h264_param.cpb_user_size = atoi(optarg);\
			encode_param[current_stream].h264_param.panic_mode_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;

/***************************************************************
	MPEG ENCODE command line options
****************************************************************/
//quality factor
#define	MPEG_ENCODE_QUALITY_OPTIONS	{"quality",	HAS_ARG,	0,	'q'},
#define	MPEG_ENCODE_QUALITY_HINTS		\
	{"quality",	"\tset jpeg/mjpeg encode quality"},

#define	MPEG_ENCODE_LONG_OPTIONS()	MPEG_ENCODE_QUALITY_OPTIONS
#define	MPEG_ENCODE_PARAMETER_HINTS()		MPEG_ENCODE_QUALITY_HINTS

#define	MPEG_ENCODE_INIT_PARAMETERS()		\
	case 'q':\
			VERIFY_STREAMID(current_stream);\
			encode_param[current_stream].jpeg_param.quality = atoi(optarg);\
			encode_param[current_stream].jpeg_param.quality_changed_flag  = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;

/***************************************************************
	COMMON ENCODE command line options
****************************************************************/
//horizontal flip
#define	COMMON_ENCODE_HFLIP_OPTIONS	\
	{"hflip",		HAS_ARG,	0,	SPECIFY_HFLIP},
#define	COMMON_ENCODE_HFLIP_HINTS		\
	{"0|1",		"\tdsp horizontal flip for current stream"},

//vertical flip
#define	COMMON_ENCODE_VFLIP_OPTIONS	\
	{"vflip",		HAS_ARG,	0,	SPECIFY_VFLIP},
#define	COMMON_ENCODE_VFLIP_HINTS		\
	{"0|1",		"\tdsp vertical flip for current stream"},


//clockwise rotate
#define	COMMON_ENCODE_ROTATE_OPTIONS	\
	{"rotate",	HAS_ARG,	0,	SPECIFY_ROTATE},
#define	COMMON_ENCODE_ROTATE_HINTS		\
	{"0|1",	"\tdsp clockwise rotate for current stream"},

// chrome format
#define	COMMON_ENCODE_CHROME_OPTIONS	\
	{"chrome",	HAS_ARG,	0,	SPECIFY_CHROME_FORMAT},
#define	COMMON_ENCODE_CHROME_HINTS		\
	{"0|1",		"\tset chrome format, 0 for YUV420 format, 1 for monochrome\n"},

#define	COMMON_ENCODE_LONG_OPTIONS()		\
	COMMON_ENCODE_HFLIP_OPTIONS	\
	COMMON_ENCODE_VFLIP_OPTIONS	\
	COMMON_ENCODE_ROTATE_OPTIONS	\
	COMMON_ENCODE_CHROME_OPTIONS

#define	COMMON_ENCODE_PARAMETER_HINTS()		\
	COMMON_ENCODE_HFLIP_HINTS		\
	COMMON_ENCODE_VFLIP_HINTS		\
	COMMON_ENCODE_ROTATE_HINTS	\
	COMMON_ENCODE_CHROME_HINTS

#define	COMMON_ENCODE_INIT_PARAMETERS()		\
	case SPECIFY_HFLIP:\
			VERIFY_STREAMID(current_stream);\
			encode_format[current_stream].hflip = atoi(optarg);\
			encode_format[current_stream].hflip_flag = 1;\
			encode_format_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_VFLIP:\
			VERIFY_STREAMID(current_stream);\
			encode_format[current_stream].vflip = atoi(optarg);\
			encode_format[current_stream].vflip_flag = 1;\
			encode_format_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_ROTATE:\
			VERIFY_STREAMID(current_stream);\
			encode_format[current_stream].rotate = atoi(optarg);\
			encode_format[current_stream].rotate_flag = 1;\
			encode_format_changed_id |= (1 << current_stream);\
			break;\
	case SPECIFY_CHROME_FORMAT:\
			VERIFY_STREAMID(current_stream);\
			if ((min_value = get_chrome_format(optarg, IAV_STREAM_TYPE_H264)) < 0)\
				return -1;\
			if ((max_value = get_chrome_format(optarg, IAV_STREAM_TYPE_MJPEG)) < 0)\
				return -1;\
			encode_param[current_stream].h264_param.h264_chrome_format = min_value;\
			encode_param[current_stream].h264_param.h264_chrome_format_flag = 1;\
			encode_param[current_stream].jpeg_param.jpeg_chrome_format = max_value;\
			encode_param[current_stream].jpeg_param.jpeg_chrome_format_flag = 1;\
			encode_param_changed_id |= (1 << current_stream);\
			break;

/***************************************************************
	SYSTEM command line options
****************************************************************/
#define	SYSTEM_NOPREVIEW_OPTIONS	\
	{"nopreview",	NO_ARG,		0,	NO_PREVIEW},
#define	SYSTEM_NOPREVIEW_HINTS		\
	{"",			"\t\tdo not enter preview"},

//put system to IDLE  (turn off all encoding )
#define	SYSTEM_IDLE_OPTIONS			\
	{"idle",		NO_ARG,		0,	SYSTEM_IDLE},
#define	SYSTEM_IDLE_HINTS				\
	{"",			"\t\tput system to IDLE  (turn off all encoding)"},

#define	SYSTEM_ENC_MODE_OPTIONS		\
	{"enc-mode",	HAS_ARG,	0,	SPECIFY_ENCODE_MODE},
#define	SYSTEM_ENC_MODE_HINTS		\
	{"0~9",		"\tChoose encode mode, default is 0. 0: Normal ISO mode,"	\
		"\n\t\t\t\t1: Multi region warp mode, 2: Blend ISO mode,"	\
			"\n\t\t\t\t3: Single region warp mode, 4: Advanced ISO mode,"	\
		"\n\t\t\t\t5: HDR line interleaved mode, 6: High MP (low delay) mode,"	\
		"\n\t\t\t\t7: Full FPS (low delay) mode, 8: Multiple VIN mode,"			\
			"\n\t\t\t\t9: HISO video mode"},

#define	SYSTEM_ENC_ROTATE_OPTIONS	\
	{"enc-rotate-possible",		HAS_ARG,	0,	SPECIFY_ROTATE_POSSIBLE},
#define	SYSTEM_ENC_ROTATE_HINTS		\
	{"0|1",	"Disable/Enable rotate possibility on Non-run time. Default is 0. 0: disable, 1: enable"},

#define	SYSTEM_RAW_CAPTURE_OPTIONS	\
	{"raw-capture",			HAS_ARG,	0,	SPECIFY_RAW_CAPTURE},
#define	SYSTEM_RAW_CAPTURE_HINTS	\
	{"0|1",	"\tDisable/Enable raw capture on non-run time. Default is 0. 0: disable, 1: enable."},

#define	SYSTEM_HDR_EXPO_OPTIONS		\
	{"hdr-expo",	HAS_ARG,	0,	SPECIFY_HDR_EXPOSURE_NUM},
#define	SYSTEM_HDR_EXPO_HINTS		\
	{"2~4",		"\tChoose exposure number in HDR line interleaved mode"},

#define	SYSTEM_LENS_WARP_OPTIONS	\
	{"lens-warp",	HAS_ARG,	0,	SPECIFY_LENS_WARP},
#define	SYSTEM_LENS_WARP_HINTS		\
	{"0|1",		"\tEnable Lens warp"},

#define	SYSTEM_MAX_ENC_NUM_OPTIONS		\
	{"max-enc-num",		HAS_ARG,	0,	SPECIFY_MAX_ENC_NUM},
#define	SYSTEM_MAX_ENC_NUM_HINTS		\
	{"2~4",		"\tSpecify max encode stream num for current mode"},

#define	SYSTEM_WARP_IN_MAXWIDTH_OPTIONS		\
	{"max-warp-in-width",		HAS_ARG,	0,	'w'},
#define	SYSTEM_WARP_IN_MAXWIDTH_HINTS			\
	{">0",					"Max warp input width"},

#define	SYSTEM_WARP_IN_MAXHEIGHT_OPTIONS		\
	{"max-warp-in-height",	HAS_ARG,	0,	SPECIFY_MAX_WARP_INPUT_HEIGHT},
#define	SYSTEM_WARP_IN_MAXHEIGHT_HINTS			\
	{">0",					"Max warp input height"},

#define	SYSTEM_WARP_OUT_MAXWIDTH_OPTIONS		\
	{"max-warp-out-width",	HAS_ARG,	0,	'W'},
#define	SYSTEM_WARP_OUT_MAXWIDTH_HINTS		\
	{">0",					"Max warp output width"},

#define	SYSTEM_PADDING_MAXWIDTH_OPTIONS		\
	{"max-padding-width",		HAS_ARG,	0,	SPECIFY_MAX_PADDING_WIDTH},
#define	SYSTEM_PADDING_MAXWIDTH_HINTS			\
	{">0",					"Max padding width"},

#define	SYSTEM_INTERMEDIATE_OPTIONS				\
	{"intermediate",	HAS_ARG,	0,	SPECIFY_INTERMEDIATE_BUF_SIZE},
#define	SYSTEM_INTERMEDIATE_HINTS				\
	{"resolution",		"Specify Intermediate buffer resolution for Fisheye mode"},

#define	SYSTEM_ENC_DUMMY_LATENCY_OPTIONS		\
	{"enc-dummy-latency", 	HAS_ARG,	0,	SPECIFY_ENC_DUMMY_LATENCY},
#define	SYSTEM_ENC_DUMMY_LATENCY_HINTS			\
	{"0~5",					"Specify the latancy for encode dummy"},

#define	SYSTEM_LONG_REF_ENABLE_OPTIONS			\
	{"long-ref-enable", 		HAS_ARG,	0,	SPECIFY_LONG_REF_ENABLE},
#define	SYSTEM_LONG_REF_ENABLE_HINTS			\
	{"0|1",					"Disable/Enable stream long term reference frame"},

#define	SYSTEM_LONG_REF_B_FRAME_OPTIONS			\
	{"long-ref-B-frame", 		HAS_ARG,	0,	SPECIFY_LONG_REF_B_FRAME},
#define	SYSTEM_LONG_REF_B_FRAME_HINTS			\
	{"0|1", 				"Disable/Enable B frame in long term reference case for all streams"},

#define	SYSTEM_DSP_PARTITION_MAP_OPTIONS		\
	{"dsp-partition-map", 		HAS_ARG,	0,	SPECIFY_DSP_PARTITION_MAP},
#define	SYSTEM_DSP_PARTITION_MAP_HINTS			\
	{"0~8",	"Specify DSP Partitions to be mapped in format of d0,d1,d2,d3,"		\
	"\n\t\t\t\t0: Map the whole DSP size, 1: Map Raw buffer, 2: Map Main buffer,"	\
	"\n\t\t\t\t3: Map Prev A buffer, 4: Map Prev B buffer, 5: Map Prev C buffer,"		\
	"\n\t\t\t\t6: Map Warpped Main buffer, 7: Map INT Main buffer, 8: Map EFM buffer"},

#define	SYSTEM_IDSP_UPSAMPLE_TYPE_OPTIONS		\
	{"idsp-upsample-type", 	HAS_ARG,	0,	SPECIFY_IDSP_UPSAMPLE_TYPE},
#define	SYSTEM_IDSP_UPSAMPLE_TYPE_HINTS		\
	{"0~2",	"Specify the idsp upsample type. 0: Off, 1: idsp frame rate upsample"	\
	" to 25fps, 2: idsp frame rate upsample to 30fps"},

#define	SYSTEM_MCTF_PM_OPTIONS		\
	{"mctf-pm", 	HAS_ARG,	0,	SPECIFY_MCTF_PM},
#define	SYSTEM_MCTF_PM_HINTS			\
	{"0|1",		"\tDisable/Enable mctf based privacy mask"},

#define	SYSTEM_VOUT_SWAP_OPTIONS	\
	{"vout-swap", HAS_ARG,	0,	SPECIFY_VOUT_SWAP},
#define	SYSTEM_VOUT_SWAP_HINTS		\
	{"0|1",		"\tDisable/Enable Vout Swap"},

#define	SYSTEM_EIS_DELAY_COUNT_OPTIONS		\
	{"eis-delay-count",HAS_ARG,	0,	SPECIFY_EIS_DELAY_COUNT},
#define	SYSTEM_EIS_DELAY_COUNT_HINTS		\
	{"0~2", 			"EIS delay count"},

#define	SYSTEM_ME0_SCALE_OPTIONS	\
	{"me0-scale", HAS_ARG,	0,	SPECIFY_ME0_SCALE},
#define	SYSTEM_ME0_SCALE_HINTS		\
	{"0|1|2",	"\tSpecify ME0 scale factor. 0: Off, 1: 8X downscale, 2: 16X downscale"},

#define	SYSTEM_ENC_RAW_RGB_OPTIONS	\
	{"enc-raw-rgb", 	HAS_ARG,	0,	SPECIFY_ENC_RAW_RGB},
#define	SYSTEM_ENC_RAW_RGB_HINTS	\
	{"0|1",			"\tDisable/Enable encode from bayer raw"},

#define	SYSTEM_ENC_RAW_YUV_OPTIONS	\
	{"enc-raw-yuv", 	HAS_ARG,	0,	SPECIFY_ENC_RAW_YUV},
#define	SYSTEM_ENC_RAW_YUV_HINTS	\
	{"0|1",			"\tDisable/Enable encode from yuv raw"},

#define	SYSTEM_RAW_SIZE_OPTIONS		\
	{"raw-size",		HAS_ARG,	0,	SPECIFY_RAW_SIZE},
#define	SYSTEM_RAW_SIZE_HINTS		\
	{"resolution",		"Specify raw resolution for encode from raw (RGB/YUV)"},

#define	SYSTEM_ENC_FROM_MEM_OPTIONS	\
	{"enc-from-mem", HAS_ARG,	0,	SPECIFY_ENC_FROM_MEM},
#define	SYSTEM_ENC_FROM_MEM_HINTS		\
	{"0|1",			"\tDisable/Enable encode from memory"},

#define	SYSTEM_EFM_BUG_NUM_OPTIONS		\
	{"efm-buf-num",	HAS_ARG, 	0, 	SPECIFY_EFM_BUF_NUM},
#define	SYSTEM_EFM_BUG_NUM_HINTS		\
	{"5~10",			"\tSpecify the buffer num for encode from memory case"},

#define	SYSTEM_EFM_SIZE_OPTIONS			\
	{"efm-size",		HAS_ARG,	0,	SPECIFY_EFM_SIZE},
#define	SYSTEM_EFM_SIZE_HINTS				\
	{"resolution",		"Specify encode from memory resolution"},

#define	SYSTEM_IDSP_DUMP_CFG_OPTIONS	\
	{"dump-idsp-cfg",	NO_ARG,		0,	DUMP_IDSP_CONFIG},
#define	SYSTEM_IDSP_DUMP_CFG_HINTS		\
	{"",				"\tdump iDSP config for debug purpose"},

#define	SYSTEM_OSD_MIXER_OPTIONS		\
	{"osd-mixer",		HAS_ARG,	0,	SPECIFY_OSD_MIXER},
#define	SYSTEM_OSD_MIXER_HINTS				\
	{"off|a|b",		"OSD blending mixer, off: disable, a: select from VOUTA, b: select from VOUTB"},

#define	SYSTEM_OVERFLOW_PROTECTION_OPTIONS	\
	{"overflow-protection",		HAS_ARG,	0,	SPECIFY_OVERFLOW_PROTECTION},
#define	SYSTEM_OVERFLOW_PROTECTION_HINTS		\
	{"0|1",			"Disable/Enable vin overflow protection\n"},

#define	SYSTEM_DSP_CLOCK_STATE_OPTIONS	\
	{"disable-dsp-clock",		HAS_ARG,	0,	SPECIFY_DSP_CLCOK_STATE},
#define	SYSTEM_DSP_CLOCK_STATE_HINTS		\
	{"0|1",			"set DSP clock state. 0: Normal, 1: Off, Default is 0."},

#define	SYSTEM_EXTRA_TOP_ROW_BUF_OPTIONS	\
	{"top-row-buf",			HAS_ARG,	0,	SPECIFY_EXTRA_TOP_RAW_BUF},
#define	SYSTEM_EXTRA_TOP_ROW_BUF_HINTS	\
	{"0|1",	"\tAdd extra 16 lines buf in source buffer to improve encode efficiency for rotate case."	\
	"\n\t\t\t\tDefault is 0. 0: disable, 1: enable."},

#define	SYSTEM_LONG_OPTIONS()		\
	SYSTEM_NOPREVIEW_OPTIONS			\
	SYSTEM_IDLE_OPTIONS				\
	SYSTEM_ENC_MODE_OPTIONS				\
	SYSTEM_ENC_ROTATE_OPTIONS				\
	SYSTEM_RAW_CAPTURE_OPTIONS			\
	SYSTEM_HDR_EXPO_OPTIONS				\
	SYSTEM_LENS_WARP_OPTIONS				\
	SYSTEM_MAX_ENC_NUM_OPTIONS			\
	SYSTEM_WARP_IN_MAXWIDTH_OPTIONS		\
	SYSTEM_WARP_IN_MAXHEIGHT_OPTIONS		\
	SYSTEM_WARP_OUT_MAXWIDTH_OPTIONS		\
	SYSTEM_PADDING_MAXWIDTH_OPTIONS		\
	SYSTEM_INTERMEDIATE_OPTIONS			\
	SYSTEM_ENC_DUMMY_LATENCY_OPTIONS		\
	SYSTEM_LONG_REF_ENABLE_OPTIONS			\
	SYSTEM_LONG_REF_B_FRAME_OPTIONS			\
	SYSTEM_DSP_PARTITION_MAP_OPTIONS		\
	SYSTEM_IDSP_UPSAMPLE_TYPE_OPTIONS		\
	SYSTEM_MCTF_PM_OPTIONS					\
	SYSTEM_VOUT_SWAP_OPTIONS				\
	SYSTEM_EIS_DELAY_COUNT_OPTIONS			\
	SYSTEM_ME0_SCALE_OPTIONS				\
	SYSTEM_ENC_RAW_RGB_OPTIONS			\
	SYSTEM_ENC_RAW_YUV_OPTIONS			\
	SYSTEM_RAW_SIZE_OPTIONS				\
	SYSTEM_ENC_FROM_MEM_OPTIONS			\
	SYSTEM_EFM_BUG_NUM_OPTIONS			\
	SYSTEM_EFM_SIZE_OPTIONS					\
	SYSTEM_IDSP_DUMP_CFG_OPTIONS			\
	SYSTEM_OSD_MIXER_OPTIONS				\
	SYSTEM_OVERFLOW_PROTECTION_OPTIONS		\
	SYSTEM_DSP_CLOCK_STATE_OPTIONS			\
	SYSTEM_EXTRA_TOP_ROW_BUF_OPTIONS

#define	SYSTEM_PARAMETER_HINTS()		\
	SYSTEM_NOPREVIEW_HINTS		\
	SYSTEM_IDLE_HINTS			\
	SYSTEM_ENC_MODE_HINTS		\
	SYSTEM_ENC_ROTATE_HINTS	\
	SYSTEM_RAW_CAPTURE_HINTS	\
	SYSTEM_HDR_EXPO_HINTS		\
	SYSTEM_LENS_WARP_HINTS		\
	SYSTEM_MAX_ENC_NUM_HINTS	\
	SYSTEM_WARP_IN_MAXWIDTH_HINTS		\
	SYSTEM_WARP_IN_MAXHEIGHT_HINTS	\
	SYSTEM_WARP_OUT_MAXWIDTH_HINTS	\
	SYSTEM_PADDING_MAXWIDTH_HINTS		\
	SYSTEM_INTERMEDIATE_HINTS			\
	SYSTEM_ENC_DUMMY_LATENCY_HINTS	\
	SYSTEM_LONG_REF_ENABLE_HINTS		\
	SYSTEM_LONG_REF_B_FRAME_HINTS		\
	SYSTEM_DSP_PARTITION_MAP_HINTS		\
	SYSTEM_IDSP_UPSAMPLE_TYPE_HINTS	\
	SYSTEM_MCTF_PM_HINTS		\
	SYSTEM_VOUT_SWAP_HINTS		\
	SYSTEM_EIS_DELAY_COUNT_HINTS	\
	SYSTEM_ME0_SCALE_HINTS		\
	SYSTEM_ENC_RAW_RGB_HINTS	\
	SYSTEM_ENC_RAW_YUV_HINTS	\
	SYSTEM_RAW_SIZE_HINTS		\
	SYSTEM_ENC_FROM_MEM_HINTS	\
	SYSTEM_EFM_BUG_NUM_HINTS	\
	SYSTEM_EFM_SIZE_HINTS		\
	SYSTEM_IDSP_DUMP_CFG_HINTS	\
	SYSTEM_OSD_MIXER_HINTS		\
	SYSTEM_OVERFLOW_PROTECTION_HINTS	\
	SYSTEM_DSP_CLOCK_STATE_HINTS		\
	SYSTEM_EXTRA_TOP_ROW_BUF_HINTS

#define	SYSTEM_INIT_PARAMETERS()		\
	case NO_PREVIEW:\
			nopreview_flag = 1;\
			break;\
	case SYSTEM_IDLE:\
			idle_cmd = 1;\
			break;\
	case SPECIFY_ENCODE_MODE:\
			min_value = atoi(optarg);\
			if (min_value >= DSP_ENCODE_MODE_TOTAL_NUM) {\
				printf("Invalid param for encode mode option [0~%d].\n",\
					DSP_ENCODE_MODE_TOTAL_NUM - 1);\
				return -1;\
			}\
			system_resource_setup.encode_mode = min_value;\
			system_resource_setup.encode_mode_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_ROTATE_POSSIBLE:\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for rotate possible option [0|1].\n");\
				return -1;\
			}\
			system_resource_setup.rotate_possible = min_value;\
			system_resource_setup.rotate_possible_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_RAW_CAPTURE:\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for raw capture option [0|1].\n");\
				return -1;\
			}\
			system_resource_setup.raw_capture_possible = min_value;\
			system_resource_setup.raw_capture_possible_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_HDR_EXPOSURE_NUM:\
			min_value = atoi(optarg);\
			if (min_value < 1 || min_value > 4) {\
				printf("Invalid exposure number [%d], should be in [1~4].\n",\
					min_value);\
				return -1;\
			}\
			system_resource_setup.hdr_exposure_num = min_value;\
			system_resource_setup.hdr_exposure_num_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_LENS_WARP:\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for lens warp option [0|1].\n");\
				return -1;\
			}\
			system_resource_setup.lens_warp = min_value;\
			system_resource_setup.lens_warp_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_MAX_ENC_NUM:\
			min_value = atoi(optarg);\
			if (min_value > MAX_ENCODE_STREAM_NUM) {\
				printf("Invalid param for max-enc-num option [2~4].\n");\
				return -1;\
			}\
			system_resource_setup.max_enc_num = min_value;\
			system_resource_setup.max_enc_num_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case 'w':\
			min_value = atoi(optarg);\
			system_resource_setup.max_warp_input_width = min_value;\
			system_resource_setup.max_warp_input_width_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_MAX_WARP_INPUT_HEIGHT:\
			min_value = atoi(optarg);\
			system_resource_setup.max_warp_input_height = min_value;\
			system_resource_setup.max_warp_input_height_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case 'W':\
			min_value = atoi(optarg);\
			system_resource_setup.max_warp_output_width = min_value;\
			system_resource_setup.max_warp_output_width_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_MAX_PADDING_WIDTH:\
			min_value = atoi(optarg);\
			system_resource_setup.max_padding_width = min_value;\
			system_resource_setup.max_padding_width_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_INTERMEDIATE_BUF_SIZE:\
			if (get_encode_resolution(optarg, &width, &height) < 0)\
				return -1;\
			system_resource_setup.v_warped_main_max_width = width;\
			system_resource_setup.v_warped_main_max_height = height;\
			system_resource_setup.intermediate_buf_size_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_ENC_DUMMY_LATENCY:\
			min_value = atoi(optarg);\
			system_resource_setup.enc_dummy_latency = min_value;\
			system_resource_setup.enc_dummy_latency_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_LONG_REF_ENABLE:\
			VERIFY_STREAMID(current_stream);\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for long ref enable option [0|1].\n");\
				return -1;\
			}\
			system_resource_setup.stream_long_ref_enable[current_stream]\
				= min_value;\
			system_resource_setup.stream_long_ref_enable_flag[current_stream] = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_LONG_REF_B_FRAME:\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for long ref B frame option [0|1].\n");\
				return -1;\
			}\
			system_resource_setup.long_ref_b_frame\
				= min_value;\
			system_resource_setup.long_ref_b_frame_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_DSP_PARTITION_MAP:\
			min_value = get_u32_map_arg(optarg);\
			if (min_value < 0 || min_value >= (1 << IAV_DSP_SUB_BUF_NUM)) {\
				printf("Invalid param for dsp partition map.\n");\
				return -1;\
			}\
			system_resource_setup.dsp_partition_map =\
				get_dsp_partition_map(min_value);\
			system_resource_setup.dsp_partition_map_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_IDSP_UPSAMPLE_TYPE:\
			min_value = atoi(optarg);\
			if (min_value < 0 || min_value >= IDSP_UPSAMPLE_TYPE_TOTAL_NUM) {\
				printf("Invalid param for idsp upsample type [0|1|2].\n");\
				return -1;\
			}\
			system_resource_setup.idsp_upsample_type = min_value;\
			system_resource_setup.idsp_upsample_type_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_MCTF_PM:\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for mctf pm [0|1].\n");\
				return -1;\
			}\
			system_resource_setup.mctf_pm = min_value;\
			system_resource_setup.mctf_pm_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_VOUT_SWAP:\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for vout swap [0|1].\n");\
				return -1;\
			}\
			system_resource_setup.vout_swap = min_value;\
			system_resource_setup.vout_swap_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_EIS_DELAY_COUNT:\
			min_value = atoi(optarg); \
			if (min_value < EIS_DELAY_COUNT_OFF || min_value > EIS_DELAY_COUNT_MAX) { \
				printf("Invalid param for EIS delay count [0~%d].\n"\
					, EIS_DELAY_COUNT_MAX);\
				return -1; \
			}\
			system_resource_setup.eis_delay_count = min_value; \
			system_resource_setup.eis_delay_count_flag = 1; \
			system_resource_limit_changed_flag = 1; \
			break;\
	case SPECIFY_ME0_SCALE:\
			min_value = atoi(optarg);\
			if (min_value > 2) {\
				printf("Invalid param for me0 scale [0|1|2].\n");\
				return -1;\
			}\
			system_resource_setup.me0_scale = min_value;\
			system_resource_setup.me0_scale_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_ENC_RAW_RGB:\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for encode raw rgb option [0|1].\n");\
				return -1;\
			}\
			system_resource_setup.enc_raw_rgb = min_value;\
			system_resource_setup.enc_raw_rgb_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_ENC_RAW_YUV:\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for enc raw yuv [0|1].\n");\
				return -1;\
			}\
			system_resource_setup.enc_raw_yuv = min_value;\
			system_resource_setup.enc_raw_yuv_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_RAW_SIZE:\
			if (get_encode_resolution(optarg, &width, &height) < 0)\
				return -1;\
			system_resource_setup.raw_size.width = width;\
			system_resource_setup.raw_size.height = height;\
			system_resource_setup.raw_size.resolution_changed_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_ENC_FROM_MEM:\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for enc from mem [0|1].\n");\
				return -1;\
			}\
			system_resource_setup.enc_from_mem = min_value;\
			system_resource_setup.enc_from_mem_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_EFM_BUF_NUM:\
			min_value = atoi(optarg);\
			system_resource_setup.efm_buf_num = min_value;\
			system_resource_setup.efm_buf_num_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_EFM_SIZE:\
			if (get_encode_resolution(optarg, &width, &height) < 0)\
				return -1;\
			system_resource_setup.efm_size.width = width;\
			system_resource_setup.efm_size.height = height;\
			system_resource_setup.efm_size.resolution_changed_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case DUMP_IDSP_CONFIG:\
			dump_idsp_bin_flag = 1;\
			break;\
	case SPECIFY_OSD_MIXER:\
			system_resource_setup.osd_mixer = get_osd_mixer_selection(optarg);\
			if (system_resource_setup.osd_mixer < 0)\
				return -1;\
			system_resource_setup.osd_mixer_changed_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_OVERFLOW_PROTECTION:\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for vin overflow protection [0|1].\n");\
				return -1;\
			}\
			system_resource_setup.vin_overflow_protection= min_value;\
			system_resource_setup.vin_overflow_protection_flag= 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_DSP_CLCOK_STATE:\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid value [%d] for 'disable-dsp-clock' option.\n", min_value);\
				return -1;\
			}\
			dsp_clock_state_disable = min_value;\
			dsp_clock_state_disable_flag = 1;\
			break;\
	case SPECIFY_EXTRA_TOP_RAW_BUF:\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid param for extra top row buf option [0|1].\n");\
				return -1;\
			}\
			system_resource_setup.extra_top_row_buf = min_value;\
			system_resource_setup.extra_top_row_buf_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;

/***************************************************************
	SOURCE BUFFER command line options
****************************************************************/
//main source buffer
#define	SOURCE_PREMAIN_OPTIONS		\
	{"premain",		NO_ARG, 	0,	'P'},
#define	SOURCE_PREMAIN_HINTS			\
	{"",				"\t\tconfig for Pre-Main buffer"},

#define	SOURCE_MAINBUF_OPTIONS		\
	{"main-buffer",	NO_ARG,		0,	'X'},
#define	SOURCE_MAINBUF_HINTS			\
	{"",				"\tconfig for Main source buffer"},

#define	SOURCE_SECONDBUF_OPTIONS	\
	{"second-buffer",	NO_ARG,		0,	'Y'},
#define	SOURCE_SECONDBUF_HINTS		\
	{"",				"\tconfig for Second source buffer"},

#define	SOURCE_THIRDBUF_OPTIONS		\
	{"third-buffer",	NO_ARG,		0,	'J'},
#define	SOURCE_THIRDBUF_HINTS		\
	{"",				"\tconfig for Third source buffer"},

#define	SOURCE_FOURTHBUF_OPTIONS	\
	{"fourth-buffer",	NO_ARG,		0,	'K'},
#define	SOURCE_FOURTHBUF_HINTS		\
	{"",				"\tconfig for Fourth source buffer"},

#define	SOURCE_BTYPE_OPTIONS			\
	{"btype",			HAS_ARG,	0,	SPECIFY_BUFFER_TYPE},
#define	SOURCE_BTYPE_HINTS			\
	{"enc|prev",		"\tspecify source buffer type"},

#define	SOURCE_BSIZE_OPTIONS			\
	{"bsize",			HAS_ARG,	0,	SPECIFY_BUFFER_SIZE},
#define	SOURCE_BSIZE_HINTS			\
	{"resolution",		"\tspecify source buffer resolution, set 0x0 to disable it"},

#define	SOURCE_BMAXSIZE_OPTIONS		\
	{"bmaxsize",		HAS_ARG,	0,	SPECIFY_BUFFER_MAX_SIZE},
#define	SOURCE_BMAXSIZE_HINTS		\
	{"resolution",		"specify source buffer max resolution, set 0x0 to cleanly disable it"},

#define	SOURCE_BINSIZE_OPTIONS		\
	{"binsize",		HAS_ARG,	0,	SPECIFY_BUFFER_INPUT_SIZE},
#define	SOURCE_BINSIZE_HINTS			\
	{"resolution",		"specify source buffer input window size, so as to crop before downscale"},

#define	SOURCE_BINOFFSET_OPTIONS	\
	{"binoffset",		HAS_ARG,	0,	SPECIFY_BUFFER_INPUT_OFFSET},
#define	SOURCE_BINOFFSET_HINTS		\
	{"resolution",		"specify source buffer input window offset, so as to crop before downscale"},

#define	SOURCE_PREWARP_OPTIONS		\
	{"prewarp",	HAS_ARG,	0,	SPECIFY_BUFFER_PREWARP},
#define	SOURCE_PREWARP_HINTS			\
	{"0|1",		"\tPre-warp option for source buffer in warp encoding mode, "	\
	"0: after warping; 1: before warping. Default is 0"},

#define	SOURCE_EXTRABUF_OPTIONS		\
	{"extra-buf",		HAS_ARG,	0,	SPECIFY_EXTRA_DRAM_BUF},
#define	SOURCE_EXTRABUF_HINTS	{"0~3", "\tSet extra DRAM buffer number for extremely heavy load cases."},

#define	SOURCE_DUMP_INTERVAL_OPTIONS		\
	{"vca-dump-interval",		HAS_ARG,	0,	SPECIFY_VCA_DUMP_INTERVAL},
#define	SOURCE_DUMP_INTERVAL_HINTS	{"0~N", "Capture one frame every N frame in VCA buffer. "	\
	"0: off, 1: every frame, 2: half frame, N < VIN_FPS."},

#define	SOURCE_DUMP_DURATION_OPTIONS		\
	{"vca-dump-duration",		HAS_ARG,	0,	SPECIFY_VCA_DUMP_DURATION},
#define	SOURCE_DUMP_DURATION_HINTS	{"0~32", "Set the duration of vca dump frames, "	\
	"DSP will stop dumping after 'duration' frames. "},

#define	SOURCE_BUFFER_LONG_OPTIONS()		\
	SOURCE_PREMAIN_OPTIONS		\
	SOURCE_MAINBUF_OPTIONS		\
	SOURCE_SECONDBUF_OPTIONS	\
	SOURCE_THIRDBUF_OPTIONS	\
	SOURCE_FOURTHBUF_OPTIONS	\
	SOURCE_BTYPE_OPTIONS		\
	SOURCE_BSIZE_OPTIONS		\
	SOURCE_BMAXSIZE_OPTIONS	\
	SOURCE_BINSIZE_OPTIONS		\
	SOURCE_BINOFFSET_OPTIONS	\
	SOURCE_PREWARP_OPTIONS	\
	SOURCE_EXTRABUF_OPTIONS	\
	SOURCE_DUMP_INTERVAL_OPTIONS	\
	SOURCE_DUMP_DURATION_OPTIONS

#define	SOURCE_BUFFER_PARAMETER_HINTS()		\
	SOURCE_PREMAIN_HINTS		\
	SOURCE_MAINBUF_HINTS		\
	SOURCE_SECONDBUF_HINTS		\
	SOURCE_THIRDBUF_HINTS		\
	SOURCE_FOURTHBUF_HINTS		\
	SOURCE_BTYPE_HINTS			\
	SOURCE_BSIZE_HINTS			\
	SOURCE_BMAXSIZE_HINTS		\
	SOURCE_BINSIZE_HINTS		\
	SOURCE_BINOFFSET_HINTS		\
	SOURCE_PREWARP_HINTS		\
	SOURCE_EXTRABUF_HINTS		\
	SOURCE_DUMP_INTERVAL_HINTS	\
	SOURCE_DUMP_DURATION_HINTS

#define	SOURCE_BUFFER_INIT_PARAMETERS()		\
	case 'P':\
			current_buffer = IAV_SRCBUF_PMN;\
			break;\
	case 'X':\
			current_buffer = IAV_SRCBUF_MN;\
			break;\
	case 'Y':\
			current_buffer = IAV_SRCBUF_PC;\
			break;\
	case 'J':\
			current_buffer = IAV_SRCBUF_PB;\
			break;\
	case 'K':\
			current_buffer = IAV_SRCBUF_PA;\
			break;\
	case SPECIFY_BUFFER_TYPE:\
			VERIFY_TYPE_CHANGEABLE_BUFFERID(current_buffer);\
			int buffer_type = get_buffer_type(optarg);\
			if (buffer_type < 0)\
				return -1;\
			source_buffer_type[current_buffer].buffer_type = buffer_type;\
			source_buffer_type[current_buffer].buffer_type_changed_flag = 1;\
			source_buffer_type_changed_flag = 1;\
			break;\
	case SPECIFY_BUFFER_SIZE:\
			VERIFY_BUFFERID(current_buffer);\
			if (get_encode_resolution(optarg, &width, &height) < 0)\
				return -1;\
			source_buffer_format[current_buffer].width = width;\
			source_buffer_format[current_buffer].height = height;\
			source_buffer_format[current_buffer].resolution_changed_flag = 1;\
			source_buffer_format_changed_id |= (1 << current_buffer);\
			break;\
	case SPECIFY_BUFFER_MAX_SIZE:\
			VERIFY_BUFFERID(current_buffer);\
			if (get_encode_resolution(optarg, &width, &height) < 0)\
				return -1;\
			system_resource_setup.source_buffer_max_size[current_buffer].width = width;\
			system_resource_setup.source_buffer_max_size[current_buffer].height = height;\
			system_resource_setup.source_buffer_max_size_changed_id |= (1 << current_buffer);\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_BUFFER_INPUT_SIZE:\
			VERIFY_BUFFERID(current_buffer);\
			if (get_encode_resolution(optarg, &width, &height) < 0)\
				return -1;\
			source_buffer_format[current_buffer].input_width = width;\
			source_buffer_format[current_buffer].input_height = height;\
			source_buffer_format[current_buffer].input_size_changed_flag = 1;\
			source_buffer_format_changed_id |= (1 << current_buffer);\
			break;\
	case SPECIFY_BUFFER_INPUT_OFFSET:\
			VERIFY_BUFFERID(current_buffer);\
			if (get_encode_resolution(optarg, &width, &height) < 0)\
				return -1;\
			source_buffer_format[current_buffer].input_x = width;\
			source_buffer_format[current_buffer].input_y = height;\
			source_buffer_format[current_buffer].input_offset_changed_flag = 1;\
			source_buffer_format_changed_id |= (1 << current_buffer);\
			break;\
	case SPECIFY_BUFFER_PREWARP:\
			VERIFY_BUFFERID(current_buffer);\
			min_value = atoi(optarg);\
			source_buffer_format[current_buffer].unwarp = min_value;\
			source_buffer_format[current_buffer].unwarp_flag = 1;\
			source_buffer_format_changed_id |= (1 << current_buffer);\
			break;\
	case SPECIFY_EXTRA_DRAM_BUF:\
			VERIFY_BUFFERID(current_buffer);\
			min_value = atoi(optarg);\
			if (min_value < 0 || min_value > 3) {\
				printf("Invalid value [%d], valid range [0~3].\n",\
					min_value);\
				return -1;\
			}\
			system_resource_setup.extra_dram_buf[current_buffer] = min_value;\
			system_resource_setup.extra_dram_buf_changed_id |= (1 << current_buffer);\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_VCA_DUMP_INTERVAL:\
			VERIFY_BUFFERID(current_buffer);\
			min_value = atoi(optarg);\
			source_buffer_format[current_buffer].dump_interval = min_value;\
			source_buffer_format[current_buffer].dump_interval_flag = 1;\
			source_buffer_format_changed_id |= (1 << current_buffer);\
			break;\
	case SPECIFY_VCA_DUMP_DURATION:\
			VERIFY_BUFFERID(current_buffer);\
			min_value = atoi(optarg);\
			if (min_value < 0 || min_value > MAX_NUM_VCA_DUMP_DURATION) {\
				printf("Invalid value [%d], valid range [0~32].\n",\
					min_value);\
				return -1;\
			}\
			source_buffer_format[current_buffer].dump_duration = min_value;\
			source_buffer_format[current_buffer].dump_duration_flag = 1;\
			source_buffer_format_changed_id |= (1 << current_buffer);\
			break;

/***************************************************************
	SHOW INFO command line options
****************************************************************/
//show system state
#define	SHOW_SYSTEM_STATE_OPTIONS		\
	{"show-system-state",		NO_ARG,		0,	SHOW_SYSTEM_STATE},
#define	SHOW_SYSTEM_STATE_HINTS			\
	{"",						"\tShow system state"},

#define	SHOW_ENCODE_CONFIG_OPTIONS		\
	{"show-encode-config",		NO_ARG,		0,	SHOW_ENCODE_CONFIG},
#define	SHOW_ENCODE_CONFIG_HINTS		\
	{"",						"\tshow stream(H.264/MJPEG) encode config"},

#define	SHOW_STREAM_INFO_OPTIONS		\
	{"show-stream-info",		NO_ARG,		0,	SHOW_STREAM_INFO},
#define	SHOW_STREAM_INFO_HINTS			\
	{"",						"\tShow stream format , size, info & state"},

#define	SHOW_BUFFER_INFO_OPTIONS		\
	{"show-buffer-info",		NO_ARG,		0,	SHOW_BUFFER_INFO},
#define	SHOW_BUFFER_INFO_HINTS			\
	{"",						"\tShow source buffer info & state"},

#define	SHOW_RESOURCE_INFO_OPTIONS		\
	{"show-resource-info",		NO_ARG,		0,	SHOW_RESOURCE_INFO},
#define	SHOW_RESOURCE_INFO_HINTS		\
	{"",						"\tShow codec resource limit info"},

#define	SHOW_ENC_MODE_OPTIONS			\
	{"show-enc-mode-cap",	NO_ARG,		0,	SHOW_ENCODE_MODE_CAP},
#define	SHOW_ENC_MODE_HINTS				\
	{"",						"\tShow encode mode capability"},

#define	SHOW_DRIVER_INFO_OPTIONS		\
	{"show-driver-info",		NO_ARG,		0,	SHOW_DRIVER_INFO},
#define	SHOW_DRIVER_INFO_HINTS			\
	{"",						"\tShow IAV driver info"},

#define	SHOW_CHIP_INFO_OPTIONS			\
	{"show-chip-info",			NO_ARG,		0,	SHOW_CHIP_INFO},
#define	SHOW_CHIP_INFO_HINTS				\
	{"",						"\tShow chip info"},

#define	SHOW_DRAM_LAYOUT_OPTIONS		\
	{"show-dram-layout",		NO_ARG,		0,	SHOW_DRAM_LAYOUT},
#define	SHOW_DRAM_LAYOUT_HINTS			\
	{"",						"\tShow DRAM layout info"},

#define	SHOW_FEATURE_SET_OPTIONS		\
	{"show-feature-set",		NO_ARG,		0,	SHOW_FEATURE_SET},
#define	SHOW_FEATURE_SET_HINTS			\
	{"",						"\tShow feature set info"},

#define	SHOW_CMD_EXAM_OPTIONS			\
	{"show-cmd-exam",		NO_ARG,		0,	SHOW_CMD_EXAMPLES},
#define	SHOW_CMD_EXAM_HINTS				\
	{"",						"\tShow example command info"},

#define	SHOW_QP_HIST_OPTIONS			\
	{"show-qp-hist",		NO_ARG,		0,	SHOW_QP_HIST},
#define	SHOW_QP_HIST_HINTS				\
	{"",						"\tShow QP histogram\n"},

#define	SHOW_ALL_INFO_OPTIONS			\
	{"show-all-info",			NO_ARG,		0,	SHOW_ALL_INFO},
#define	SHOW_ALL_INFO_HINTS				\
	{"",						"\tShow all info \n"},

#define	SHOW_INFO_LONG_OPTIONS()		\
	SHOW_SYSTEM_STATE_OPTIONS		\
	SHOW_ENCODE_CONFIG_OPTIONS	\
	SHOW_STREAM_INFO_OPTIONS		\
	SHOW_BUFFER_INFO_OPTIONS		\
	SHOW_RESOURCE_INFO_OPTIONS	\
	SHOW_ENC_MODE_OPTIONS			\
	SHOW_DRIVER_INFO_OPTIONS		\
	SHOW_CHIP_INFO_OPTIONS			\
	SHOW_DRAM_LAYOUT_OPTIONS		\
	SHOW_FEATURE_SET_OPTIONS		\
	SHOW_CMD_EXAM_OPTIONS			\
	SHOW_QP_HIST_OPTIONS			\
	SHOW_ALL_INFO_OPTIONS

#define	SHOW_PARAMETER_HINTS()		\
	SHOW_SYSTEM_STATE_HINTS		\
	SHOW_ENCODE_CONFIG_HINTS		\
	SHOW_STREAM_INFO_HINTS			\
	SHOW_BUFFER_INFO_HINTS			\
	SHOW_RESOURCE_INFO_HINTS		\
	SHOW_ENC_MODE_HINTS			\
	SHOW_DRIVER_INFO_HINTS			\
	SHOW_CHIP_INFO_HINTS			\
	SHOW_DRAM_LAYOUT_HINTS		\
	SHOW_FEATURE_SET_HINTS			\
	SHOW_CMD_EXAM_HINTS			\
	SHOW_QP_HIST_HINTS			\
	SHOW_ALL_INFO_HINTS

#define	SHOW_INIT_PARAMETERS()		\
	case SHOW_SYSTEM_STATE:\
	case SHOW_ENCODE_CONFIG:\
	case SHOW_STREAM_INFO:\
	case SHOW_BUFFER_INFO:\
	case SHOW_RESOURCE_INFO:\
	case SHOW_ENCODE_MODE_CAP:\
	case SHOW_DRIVER_INFO:\
	case SHOW_CHIP_INFO:\
	case SHOW_DRAM_LAYOUT:\
	case SHOW_FEATURE_SET:\
	case SHOW_CMD_EXAMPLES:\
	case SHOW_QP_HIST:\
			show_info_flag |= (1 << (ch - SHOW_SYSTEM_STATE));\
			break;\
	case SHOW_ALL_INFO:\
			show_info_flag = 0xFFFFFFFF;\
			break;

/***************************************************************
	DEBUG command line options
****************************************************************/
#define	DEBUG_CHECK_DISABLE_OPTIONS	\
	{"check-disable",	HAS_ARG, 0,	SPECIFY_CHECK_DISABLE},
#define	DEBUG_CHECK_DISABLE_HINTS		\
	{"0|1",			"Disable IAV kernel protection. Default is 0."},

#define	DEBUG_STITCHED_OPTIONS		\
	{"debug-stitched",	HAS_ARG, 0,	SPECIFY_DEBUG_STITCHED},
#define	DEBUG_STITCHED_HINTS			\
	{"-1~1",			"Enable / Disable force stitching in debug mode. Use -1 to disable this debug option"},

#define	DEBUG_ISO_TYPE_OPTIONS		\
	{"debug-iso-type",HAS_ARG, 0,	SPECIFY_DEBUG_ISO_TYPE},
#define	DEBUG_ISO_TYPE_HINTS			\
	{"-1~3",			"Specify debug ISO type. 0: low ISO, 1: middle ISO, 2: "	\
	"Advanced ISO, 3: Blend ISO, -1: disable this debug option."},

#define	DEBUG_CHIP_ID_OPTIONS		\
	{"debug-chip-id",	HAS_ARG, 0,	SPECIFY_DEBUG_CHIP_ID},
#define	DEBUG_CHIP_ID_HINTS			\
	{"-1~11",			"Specify Chip ID in debug mode. 0: S2L22M, 1: S2L33M, 2: S2L55M, 3: S2L99M,"\
	"\n\t\t\t\t4: S2L63, 5: S2L66, 6: S2L88, 7: S2L99, 8: Test, 9: S2L22, 10: S2L33MEX, 11: S2L33EX, -1: disable this debug option"},

#define	DEBUG_WAIT_OPTIONS			\
	{"wait",	NO_ARG, 0,	SPECIFY_DEBUG_WAIT_FRAME},
#define	DEBUG_WAIT_HINTS				\
	{"",		"\t\tDisplay the time diff for \"wait next frame\" IOCTL.\n"},

#define	DEBUG_AUDIT_EN_OPTIONS	\
	{"debug-audit-en",	HAS_ARG, 0,	SPECIFY_DEBUG_AUDIT_ENABLE},
#define	DEBUG_AUDIT_EN_HINTS	\
	{"0|1",	"Enable DSP audit function, default is disabled. 0: disable, 1: enable"},

#define	DEBUG_AUDIT_RD_OPTIONS			\
	{"debug-audit-read",	HAS_ARG, 0,	SPECIFY_DEBUG_AUDIT_READ},
#define	DEBUG_AUDIT_RD_HINTS			\
	{"0|1",	"Specify audit interrupt No. to read. 0: VDSP, 1: VENC"},

#define	DEBUG_LONG_OPTIONS()		\
	DEBUG_CHECK_DISABLE_OPTIONS	\
	DEBUG_STITCHED_OPTIONS		\
	DEBUG_ISO_TYPE_OPTIONS		\
	DEBUG_CHIP_ID_OPTIONS		\
	DEBUG_WAIT_OPTIONS			\
	DEBUG_AUDIT_EN_OPTIONS	\
	DEBUG_AUDIT_RD_OPTIONS

#define	DEBUG_PARAMETER_HINTS()		\
	DEBUG_CHECK_DISABLE_HINTS	\
	DEBUG_STITCHED_HINTS		\
	DEBUG_ISO_TYPE_HINTS		\
	DEBUG_CHIP_ID_HINTS		\
	DEBUG_WAIT_HINTS			\
	DEBUG_AUDIT_EN_HINTS	\
	DEBUG_AUDIT_RD_HINTS

#define	DEBUG_INIT_PARAMETERS()		\
	case SPECIFY_CHECK_DISABLE:\
			min_value = atoi(optarg);\
			if (min_value > 1) {\
				printf("Invalid value [%d] for 'check-disable' option.\n", min_value);\
				return -1;\
			}\
			debug_setup.check_disable = min_value;\
			debug_setup.check_disable_flag = 1;\
			debug_setup_flag = 1;\
			break;\
	case SPECIFY_DEBUG_STITCHED:\
			min_value = atoi(optarg);\
			if (min_value > 1 && min_value != -1) {\
				printf("Invalid value [%d] for 'debug-stitched' option.\n", min_value);\
				return -1;\
			}\
			system_resource_setup.debug_stitched = min_value;\
			system_resource_setup.debug_stitched_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_DEBUG_ISO_TYPE:\
			min_value = atoi(optarg);\
			if (min_value > ISO_TYPE_BLEND && min_value != -1) {\
				printf("Invalid value [%d] for 'debug-iso-type' option.\n", min_value);\
				return -1;\
			}\
			system_resource_setup.debug_iso_type = min_value;\
			system_resource_setup.debug_iso_type_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_DEBUG_CHIP_ID:\
			min_value = atoi(optarg);\
			if (min_value >= IAV_CHIP_ID_S2L_LAST && min_value != -1) {\
				printf("Invalid value [%d] for 'debug-chip-id' option.\n", min_value);\
				return -1;\
			}\
			system_resource_setup.debug_chip_id = min_value;\
			system_resource_setup.debug_chip_id_flag = 1;\
			system_resource_limit_changed_flag = 1;\
			break;\
	case SPECIFY_DEBUG_WAIT_FRAME:\
			debug_setup.wait_frame_flag = 1;\
			debug_setup_flag = 1;\
			break;\
	case SPECIFY_DEBUG_AUDIT_ENABLE:\
			min_value = atoi(optarg);\
			if (min_value > 1 || min_value < 0) {\
				printf("Invalid value [%d] for 'debug-audit-en' [0|1].\n",\
					min_value);\
				return -1;\
			}\
			debug_setup.audit_int_enable = min_value;\
			debug_setup.audit_int_enable_flag = 1;\
			debug_setup_flag = 1;\
			break;\
	case SPECIFY_DEBUG_AUDIT_READ:\
			min_value = atoi(optarg);\
			if (min_value > 1 || min_value < 0) {\
				printf("Invalid value [%d] for 'debug-audit-read' option [0|1].\n",\
					min_value);\
				return -1;\
			}\
			debug_setup.audit_id = min_value;\
			debug_setup.audit_id_flag = 1;\
			debug_setup_flag = 1;\
			break;

static struct option long_options[] = {
	//Stream configurations
	STREAM_LONG_OPTIONS()

	//immediate action, configure encode stream on the fly
	ENCODE_CONTROL_LONG_OPTIONS()

	//H.264 encode configurations
	H264_ENCODE_LONG_OPTIONS()

	//panic mode
	PANIC_LONG_OPTIONS()

	//MJPEG encode configurations
	MPEG_ENCODE_LONG_OPTIONS()

	//common encode configurations
	COMMON_ENCODE_LONG_OPTIONS()

	//vin configurations
	VIN_LONG_OPTIONS()

	//system state
	SYSTEM_LONG_OPTIONS()

	//Vout
	VOUT_LONG_OPTIONS()

	//source buffer param
	SOURCE_BUFFER_LONG_OPTIONS()

	//show info options
	SHOW_INFO_LONG_OPTIONS()

	// debug
	DEBUG_LONG_OPTIONS()
	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	//Stream configurations
	STREAM_PARAMETER_HINTS()

	//immediate action, configure encode stream on the fly
	ENCODE_CONTROL_PARAMETER_HINTS()

	//H.264 encode configurations
	H264_ENCODE_PARAMETER_HINTS()

	//panic mode
	PANIC_PARAMETER_HINTS()

	//MJPEG encode configurations
	MPEG_ENCODE_PARAMETER_HINTS()

	//common encode configurations
	COMMON_ENCODE_PARAMETER_HINTS()

	//vin configurations
	VIN_PARAMETER_HINTS()

	//system state
	SYSTEM_PARAMETER_HINTS()

	//VOUT
	VOUT_PARAMETER_HINTS()

	//source buffer param
	SOURCE_BUFFER_PARAMETER_HINTS()

	//show info options
	SHOW_PARAMETER_HINTS()

	//debug
	DEBUG_PARAMETER_HINTS()
};

static int get_arbitrary_resolution(const char *name, int *width, int *height)
{
	sscanf(name, "%dx%d", width, height);
	return 0;
}

static int get_encode_resolution(const char *name, int *width, int *height)
{
	int i;

	for (i = 0; i < sizeof(__encode_res) / sizeof(__encode_res[0]); i++)
		if (strcmp(__encode_res[i].name, name) == 0) {
			*width = __encode_res[i].width;
			*height = __encode_res[i].height;
			printf("%s resolution is %dx%d\n", name, *width, *height);
			return 0;
		}
	get_arbitrary_resolution(name, width, height);
	printf("resolution %dx%d\n", *width, *height);
	return 0;
}

//first second value must in format "x~y" if delimiter is '~'
static int get_two_unsigned_int(const char *name, u32 *first, u32 *second, char delimiter)
{
	char tmp_string[16];
	char * separator;

	separator = strchr(name, delimiter);
	if (!separator) {
		printf("range should be like a%cb \n", delimiter);
		return -1;
	}

	strncpy(tmp_string, name, separator - name);
	tmp_string[separator - name] = '\0';
	*first = atoi(tmp_string);
	strncpy(tmp_string, separator + 1,  name + strlen(name) -separator);
	*second = atoi(tmp_string);

//	printf("input string %s,  first value %d, second value %d \n",name, *first, *second);
	return 0;
}

static int get_multi_s8_arg(char * optarg, s8 *argarr, int argcnt)
{
	int i;
	char *delim = ",:";
	char *ptr;

	ptr = strtok(optarg, delim);
	argarr[0] = atoi(ptr);
	printf("[0], : %d]\n", argarr[0]);

	for (i = 1; i < argcnt; i++) {
		ptr = strtok(NULL, delim);
		if (ptr == NULL)
			break;
		argarr[i] = atoi(ptr);
	}
	if (i < argcnt)
		return -1;

	return 0;
}

static u32 get_u32_map_arg(char * optarg)
{
	char *delim = ",:";
	char *ptr;
	u32 result = 0;
	int value = 0;

	ptr = strtok(optarg, delim);

	while (ptr != NULL) {
		value = strtoul(ptr, 0, 10);
		if (value > 31 || value < 0) {
			return -1;
		}
		result |= 1 << value;
		ptr = strtok(NULL, delim);
	}

	return result;
}

int get_gop_model(const char *name)
{
	if (strcmp(name, "simple") == 0)
		return IAV_GOP_SIMPLE;

	if (strcmp(name, "advanced") == 0)
		return IAV_GOP_ADVANCED;

	printf("invalid gop model: %s\n", name);
	return -1;
}

static int get_bitrate_control(const char *name)
{
	if (strcmp(name, "cbr") == 0)
		return IAV_CBR;
	else if (strcmp(name, "vbr") == 0)
		return IAV_VBR;
	else if (strcmp(name, "cbr-quality") == 0)
		return IAV_CBR_QUALITY_KEEPING;
	else if (strcmp(name, "vbr-quality") == 0)
		return IAV_VBR_QUALITY_KEEPING;
	else if (strcmp(name, "cbr2") == 0)
		return IAV_CBR2;
	else
		return -1;
}

static int get_chrome_format(const char *format, int encode_type)
{
	int chrome = atoi(format);
	if (chrome == 0) {
		return (encode_type == IAV_STREAM_TYPE_H264) ?
			H264_CHROMA_YUV420 :
			JPEG_CHROMA_YUV420;
	} else if (chrome == 1) {
		return (encode_type == IAV_STREAM_TYPE_H264) ?
			H264_CHROMA_MONO :
			JPEG_CHROMA_MONO;
	} else {
		printf("invalid chrome format : %d.\n", chrome);
		return -1;
	}
}

static int get_buffer_type(const char *name)
{
	if (strcmp(name, "enc") == 0)
		return IAV_SRCBUF_TYPE_ENCODE;
	if (strcmp(name, "vca") == 0)
		return IAV_SRCBUF_TYPE_VCA;
	if (strcmp(name, "prev") == 0)
		return IAV_SRCBUF_TYPE_PREVIEW;
	if (strcmp(name, "off") == 0)
		return IAV_SRCBUF_TYPE_OFF;

	printf("invalid buffer type: %s\n", name);
	return -1;
}

static int get_osd_mixer_selection(const char *name)
{
	if (strcmp(name, "off") == 0)
		return OSD_BLENDING_OFF;
	if (strcmp(name, "a") == 0)
		return OSD_BLENDING_FROM_MIXER_A;
	if (strcmp(name, "b") == 0)
		return OSD_BLENDING_FROM_MIXER_B;

	printf("invalid osd mixer selection: %s\n", name);
	return -1;
}

static u32 get_dsp_partition_map(u32 in_map)
{
	u32 out_map = 0;

	if (in_map & (1 << DSP_PARTITION_RAW)) {
		out_map |= 1 << IAV_DSP_SUB_BUF_RAW;
	}
	if (in_map & (1 << DSP_PARTITION_MAIN_CAPTURE)) {
		out_map |= (1 << IAV_DSP_SUB_BUF_MAIN_YUV |
			1 << IAV_DSP_SUB_BUF_MAIN_ME1);
	}
	if (in_map & (1 << DSP_PARTITION_PREVIEW_A)) {
		out_map |= (1 << IAV_DSP_SUB_BUF_PREV_A_YUV |
			1 << IAV_DSP_SUB_BUF_PREV_A_ME1);
	}
	if (in_map & (1 << DSP_PARTITION_PREVIEW_B)) {
		out_map |= (1 << IAV_DSP_SUB_BUF_PREV_B_YUV |
			1 << IAV_DSP_SUB_BUF_PREV_B_ME1);
	}
	if (in_map & (1 << DSP_PARTITION_PREVIEW_C)) {
		out_map |= (1 << IAV_DSP_SUB_BUF_PREV_C_YUV |
			1 << IAV_DSP_SUB_BUF_PREV_C_ME1);
	}
	if (in_map & (1 << DSP_PARTITION_WARPED_MAIN_CAPTURE)) {
		out_map |= (1 << IAV_DSP_SUB_BUF_POST_MAIN_YUV |
			1 << IAV_DSP_SUB_BUF_POST_MAIN_ME1);
	}
	if (in_map & (1 << DSP_PARTITION_WARPED_INT_MAIN_CAPTURE)) {
		out_map |= 1 << IAV_DSP_SUB_BUF_INT_MAIN_YUV;
	}
	if (in_map & (1 << DSP_PARTITION_EFM)) {
		out_map |= (1 << IAV_DSP_SUB_BUF_EFM_YUV |
			1 << IAV_DSP_SUB_BUF_EFM_ME1);
	}
	// output map 0x0 indicates dsp map off
	if (in_map & (1 << DSP_PARTITION_OFF)) {
		out_map = 0x0;
	}

	return out_map;
}

static int check_encode_profile(int stream)
{
	int profile = encode_param[stream].h264_param.h264_profile_level;
	if (profile < 0 || profile > H264_PROFILE_HIGH) {
		printf("Invalid profile [%d] of stream [%d], not in the range of [%d~%d].\n",
			profile, stream, H264_PROFILE_BASELINE, H264_PROFILE_HIGH);
		return -1;
	}
	return 0;
}

static void usage(void)
{
	int i;

	printf("test_encode usage:\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");

	printf("vin mode:  ");
	for (i = 0; i < sizeof(__vin_modes)/sizeof(__vin_modes[0]); i++) {
		if (__vin_modes[i].name[0] == '\0') {
			printf("\n");
			printf("           ");
		} else
			printf("%s  ", __vin_modes[i].name);
	}
	printf("\n");

	printf("vout mode:  ");
	for (i = 0; i < sizeof(vout_res) / sizeof(vout_res[0]); i++)
		printf("%s  ", vout_res[i].name);
	printf("\n");

	printf("resolution:  ");
	for (i = 0; i < sizeof(__encode_res)/sizeof(__encode_res[0]); i++) {
		if (__encode_res[i].name[0] == '\0') {
			printf("\n");
			printf("             ");
		} else
			printf("%s  ", __encode_res[i].name);
	}
	printf("\n");
}

static int check_init_param(void)
{
	if (system_resource_setup.raw_size.resolution_changed_flag &&
		!system_resource_setup.enc_raw_rgb_flag &&
		!system_resource_setup.enc_raw_yuv_flag) {
		printf("RAW size is available when 'enc-raw-rgb' or 'enc-raw-yuv' is enabled!\n");
		return -1;
	}
	if (system_resource_setup.efm_size.resolution_changed_flag &&
		!system_resource_setup.enc_from_mem_flag) {
		printf("EFM size is ONLY available when 'enc-from-mem' is enabled!\n");
		return -1;
	}

	return 0;
}

static int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;
	int width, height;
	u32 min_value, max_value;
	u32 numerator, denominator;

	opterr = 0;

	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {

		// handle all other options
		switch (ch) {
			STREAM_INIT_PARAMETERS()

			ENCODE_CONTROL_INIT_PARAMETERS()

			H264_ENCODE_INIT_PARAMETERS()

			PANIC_INIT_PARAMETERS()

			MPEG_ENCODE_INIT_PARAMETERS()

			COMMON_ENCODE_INIT_PARAMETERS()

			VIN_INIT_PARAMETERS()

			SYSTEM_INIT_PARAMETERS()

			VOUT_INIT_PARAMETERS()

			SOURCE_BUFFER_INIT_PARAMETERS()

			SHOW_INIT_PARAMETERS()

			DEBUG_INIT_PARAMETERS()

			default:
				printf("unknown option found: [%c]\n", ch);
				return -1;
			}
	}

	if (check_init_param() < 0) {
		printf("Check init param error.\n");
		return -1;
	}

	return 0;
}

static void get_chip_id_str(u32 chip_id, char *chip_str)
{
	switch (chip_id) {
	case IAV_CHIP_ID_S2L_22M:
		strcpy(chip_str, "S2L22M");
		break;
	case IAV_CHIP_ID_S2L_33M:
		strcpy(chip_str, "S2L33M");
		break;
	case IAV_CHIP_ID_S2L_55M:
		strcpy(chip_str, "S2L55M");
		break;
	case IAV_CHIP_ID_S2L_99M:
		strcpy(chip_str, "S2L99M");
		break;
	case IAV_CHIP_ID_S2L_63:
		strcpy(chip_str, "S2L63");
		break;
	case IAV_CHIP_ID_S2L_66:
		strcpy(chip_str, "S2L66");
		break;
	case IAV_CHIP_ID_S2L_88:
		strcpy(chip_str, "S2L88");
		break;
	case IAV_CHIP_ID_S2L_99:
		strcpy(chip_str, "S2L99");
		break;
	case IAV_CHIP_ID_S2L_TEST:
		strcpy(chip_str, "S2LM_test");
		break;
	case IAV_CHIP_ID_S2L_22:
		strcpy(chip_str, "S2L22");
		break;
	case IAV_CHIP_ID_S2L_33MEX:
		strcpy(chip_str, "S2L33MEX");
		break;
	case IAV_CHIP_ID_S2L_33EX:
		strcpy(chip_str, "S2L33EX");
		break;
	default:
		strcpy(chip_str, "Unknown");
		break;
	}

	return ;
}

static void get_chip_arch_str(u32 chip_arch, char *chip_str)
{
	switch (chip_arch) {
	case UCODE_ARCH_S2L:
		strcpy(chip_str, "S2L");
		break;
	case UCODE_ARCH_S3L:
		strcpy(chip_str, "S3L");
		break;
	default:
		sprintf(chip_str, "Unknown(%d)", chip_arch);
		break;
	}
}

static int show_encode_stream_info(void)
{
	struct iav_stream_info info;
	struct iav_stream_format format;
	struct iav_stream_cfg cfg;
	int format_configured;
	char state_str[16];
	char type_str[16];
	char source_str[16];
	int i;

	printf("\n[Encode stream info]:\n");
	for (i = 0; i < MAX_ENCODE_STREAM_NUM ; i++) {
		memset(&info, 0, sizeof(info));
		info.id = i;

		AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_INFO, &info);
		switch (info.state) {
		case IAV_STREAM_STATE_IDLE:
			strcpy(state_str, "idle");
			break;
		case IAV_STREAM_STATE_STARTING:
			strcpy(state_str, "encode starting");
			break;
		case IAV_STREAM_STATE_ENCODING:
			strcpy(state_str, "encoding");
			break;
		case IAV_STREAM_STATE_STOPPING:
			strcpy(state_str, "encode stopping");
			break;
		case IAV_STREAM_STATE_UNKNOWN:
			strcpy(state_str, "unknown");
			break;
		default:
			return -1;
		}

		memset(&format, 0, sizeof(format));
		format.id = i;
		AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_FORMAT, &format);

		switch (format.type) {
		case IAV_STREAM_TYPE_H264:
			strcpy(type_str, "H.264");
			format_configured = 1;
			break;
		case IAV_STREAM_TYPE_MJPEG:
			strcpy(type_str, "MJPEG");
			format_configured = 1;
			break;
		default:
			strcpy(type_str, "NULL");
			format_configured = 0;
			break;
		}

		switch (format.buf_id) {
		case IAV_SRCBUF_MN:
			strcpy(source_str, "MAIN");
			break;
		case IAV_SRCBUF_PC:
			strcpy(source_str, "SECOND");
			break;
		case IAV_SRCBUF_PB:
			strcpy(source_str, "THIRD");
			break;
		case IAV_SRCBUF_PA:
			strcpy(source_str, "FOURTH");
			break;
		default:
			strcpy(source_str, "Invalid");
			break;
		}

		memset(&cfg, 0, sizeof(cfg));
		cfg.id = i;
		cfg.cid = IAV_STMCFG_FPS;
		AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_CONFIG, &cfg);

		COLOR_PRINT("Stream %c \n", i + 'A');
		BOLD_PRINT("\t        Format : %s\n", type_str);
		if (format_configured) {
			printf("\t         State : %s\n", state_str);
			printf("\t        Source : %s source bufffer\n", source_str);
			printf("\t      Duration : %d\n", format.duration);
			printf("\t    Resolution : (%dx%d) \n",
				format.enc_win.width,
				format.enc_win.height);
			printf("\t Encode offset : (%d,%d)\n",
				format.enc_win.x, format.enc_win.y);
			printf("\t        H-Flip : %d\n", format.hflip);
			printf("\t        V-Flip : %d\n", format.vflip);
			printf("\t        Rotate : %d\n", format.rotate_cw);
			printf("\t     FPS ratio : %d/%d\n",
				cfg.arg.fps.fps_multi, cfg.arg.fps.fps_div);
			printf("\tAbs FPS enable : %d\n", cfg.arg.fps.abs_fps_enable);
			printf("\t       Abs FPS : %d\n", cfg.arg.fps.abs_fps);
		}
		printf("\n");
	}

	return 0;
}

static int show_source_buffer_info(void)
{
	struct iav_srcbuf_info srcbuf_info;
	struct iav_srcbuf_setup srcbuf_setup;
	struct iav_system_resource resource;
	struct vindev_video_info video_info;
	struct vindev_fps vsrc_fps;
	struct vindev_mode vsrc_mode;
	char fps[32];
	u32 fps_hz;
	u32 input_width, input_height, input_offset_x, input_offset_y;
	u32 width, height, buffer_type, i, dump_duration, dump_interval;
	char state_str[16];
	char *buffer_type_str[3] = {"off", "encode", "preview"};
	char *buffer_name[] = {"Main", "Second", "Third", "Fourth", "Pre-main"};
	u32 state;

	memset(&resource, 0, sizeof(resource));
	resource.encode_mode = DSP_CURRENT_MODE;
	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource);
	memset(&srcbuf_setup, 0, sizeof(srcbuf_setup));
	AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &srcbuf_setup);

	AM_IOCTL(fd_iav, IAV_IOC_GET_IAV_STATE, &state);
	if (state != IAV_STATE_INIT) {
		memset(&video_info, 0, sizeof(video_info));
		video_info.vsrc_id = 0;
		video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
		AM_IOCTL(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info);

		printf("\n[VIN Capture info]:\n");
		printf("  VIN Capture window : %dx%d\n", video_info.info.width, video_info.info.height);

		vsrc_fps.vsrc_id = 0;
		AM_IOCTL(fd_iav, IAV_IOC_VIN_GET_FPS, &vsrc_fps);
		change_fps_to_hz(vsrc_fps.fps, &fps_hz, fps);
		printf("  VIN Frame Rate : %s\n", fps);

		vsrc_mode.vsrc_id = 0;
		AM_IOCTL(fd_iav, IAV_IOC_VIN_GET_MODE, &vsrc_mode);
		printf("  VIN Mode : %s\n", (vsrc_mode.hdr_mode == 0 ? "Linear mode" : "HDR Mode"));
		if (vsrc_mode.hdr_mode > 0) {
			printf("  VIN Max_act window : %dx%d\n",
				video_info.info.max_act_width, video_info.info.max_act_height);
		}
	}

	printf("\n[Source buffer info]:\n");
	if (resource.encode_mode == DSP_MULTI_REGION_WARP_MODE ||
		resource.encode_mode == DSP_SINGLE_REGION_WARP_MODE) {
		COLOR_PRINT0("Pre Main buffer: \n");
		printf("\tformat : %dx%d\n", srcbuf_setup.size[IAV_SRCBUF_PMN].width,
		       srcbuf_setup.size[IAV_SRCBUF_PMN].height);
		printf("\tinput_format : %dx%d\n",
		       srcbuf_setup.input[IAV_SRCBUF_PMN].width,
		       srcbuf_setup.input[IAV_SRCBUF_PMN].height);
		printf("\tinput_offset : %dx%d\n\n",
		       srcbuf_setup.input[IAV_SRCBUF_PMN].x,
		       srcbuf_setup.input[IAV_SRCBUF_PMN].y);
	}

	for (i = 0; i < MAX_SOURCE_BUFFER_NUM; i++) {
		if (i != IAV_SRCBUF_PMN) {
			memset(&srcbuf_info, 0, sizeof(srcbuf_info));
			srcbuf_info.buf_id = i;
			AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_INFO, &srcbuf_info);
			switch (srcbuf_info.state) {
			case IAV_SRCBUF_STATE_ERROR:
				strcpy(state_str, "error");
				break;
			case IAV_SRCBUF_STATE_IDLE:
				strcpy(state_str, "idle");
				break;
			case IAV_SRCBUF_STATE_BUSY:
				strcpy(state_str, "busy");
				break;
			case IAV_SRCBUF_STATE_UNKNOWN:
			default:
				strcpy(state_str, "unknown");
				break;
			}
		} else {
			if (resource.encode_mode != DSP_MULTI_REGION_WARP_MODE) {
				continue;
			}
			strcpy(state_str, "idle");
		}
		width = srcbuf_setup.size[i].width;
		height = srcbuf_setup.size[i].height;

		input_width = srcbuf_setup.input[i].width;
		input_height = srcbuf_setup.input[i].height;
		input_offset_x = srcbuf_setup.input[i].x;
		input_offset_y = srcbuf_setup.input[i].y;

		buffer_type = srcbuf_setup.type[i];
		dump_duration = srcbuf_setup.dump_duration[i];
		dump_interval = srcbuf_setup.dump_interval[i];

		COLOR_PRINT("%s source buffer: \n", buffer_name[i]);
		printf("\ttype : %s \n", buffer_type_str[buffer_type]);
		printf("\tformat : %dx%d\n", width, height);
		if (buffer_type == IAV_SRCBUF_TYPE_ENCODE) {
			printf("\tinput_format : %dx%d\n", input_width, input_height);
			printf("\tinput_offset : %dx%d\n", input_offset_x, input_offset_y);
			printf("\tstate        : %s\n", state_str);
		} else if (buffer_type == IAV_SRCBUF_TYPE_VCA) {
			printf("\tinput_format : %dx%d\n", input_width, input_height);
			printf("\tinput_offset : %dx%d\n", input_offset_x, input_offset_y);
			printf("\tvca_dump_duration : %d\n", dump_duration);
			printf("\tvca_dump_interval : %d\n", dump_interval);
		}
		printf("\n");
	}

	return 0;
}

static int show_resource_limit_info(void)
{
	struct iav_system_resource resource;
	char temp[32];
	char chip_str[128];
	char flag[2][16] = { "Disabled", "Enabled" };
	char hdr[3][20] = {"OFF", "Basic (Preblend)", "Adv (With CE)"};
	char iso[5][20] = {"LOW", "Middle", "Advanced", "Blend", "High"};
	char *bufstr[] = {"main", "second", "third", "fourth", "pre-main"};
	char idsp_upsample[3][16] = {"OFF", "25fps", "30fps"};
	char me0[3][16] = {"Disabled", "1/8 X", "1/16 X"};
	int i, warp_mode = 0;
	u8 max_stream_num, hdr_expo_num, sharpen;
	u8 rotatable, lens_warp, idsp_upsample_type;
	u8 enc_from_mem, enc_raw_yuv, enc_raw_rgb;

	memset(&resource, 0, sizeof(resource));
	resource.encode_mode = DSP_CURRENT_MODE;
	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource);

	system_resource_setup.encode_mode = resource.encode_mode;
	sharpen = 0;
	max_stream_num = resource.max_num_encode;
	rotatable = resource.rotate_enable;
	lens_warp = resource.lens_warp_enable;
	idsp_upsample_type = resource.idsp_upsample_type;
	enc_raw_rgb = resource.enc_raw_rgb;
	hdr_expo_num = resource.exposure_num;
	enc_from_mem = resource.enc_from_mem;
	enc_raw_yuv = resource.enc_raw_yuv;

	switch (system_resource_setup.encode_mode) {
	case DSP_NORMAL_ISO_MODE:
		strcpy(temp, "Normal ISO");
		break;
	case DSP_MULTI_REGION_WARP_MODE:
		strcpy(temp, "Multi region warping");
		warp_mode = 1;
		break;
	case DSP_BLEND_ISO_MODE:
		strcpy(temp, "Blend ISO");
		break;
	case DSP_SINGLE_REGION_WARP_MODE:
		strcpy(temp, "Single region warping");
		break;
	case DSP_ADVANCED_ISO_MODE:
		if (resource.iso_type == ISO_TYPE_MIDDLE)
			strcpy(temp, "ISO Plus");
		else if (resource.iso_type == ISO_TYPE_ADVANCED)
			strcpy(temp, "Advanced ISO");
		else
			strcpy(temp, "Unknown");
		break;
		break;
	case DSP_HDR_LINE_INTERLEAVED_MODE:
		strcpy(temp, "HDR line");
		break;
	case DSP_HIGH_MP_LOW_DELAY_MODE:
		strcpy(temp, "High MP (Low Delay)");
		break;
	case DSP_FULL_FPS_LOW_DELAY_MODE:
		strcpy(temp, "Full FPS (Low Delay)");
		break;
	case DSP_MULTI_VIN_MODE:
		strcpy(temp, "Multiple VIN");
		break;
	case DSP_HISO_VIDEO_MODE:
		strcpy(temp, "HISO video");
		break;
	default :
		sprintf(temp, "Unknown mode [%d]", system_resource_setup.encode_mode);
		break;
	}

	printf("\n[System information]:\n");
	printf("  Encode mode : [%s]\n", temp);
	printf("\n[Codec resource limit info]:\n");
	for (i = IAV_SRCBUF_FIRST; i < IAV_SRCBUF_LAST_PMN; i++) {
		if (resource.buf_max_size[i].width) {
			printf("  max size for %s source buffer : %dx%d\n", bufstr[i],
				resource.buf_max_size[i].width,
				resource.buf_max_size[i].height);
		}
	}
	printf("  Extra DRAM buffer (0~3) : %d, %d, %d, %d\n\n",
		resource.extra_dram_buf[0], resource.extra_dram_buf[1],
		resource.extra_dram_buf[2], resource.extra_dram_buf[3]);

	printf("  max number of encode streams : %d\n", max_stream_num);
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		printf("  Stream %c Max : [%4dx%4d], M [%d], N [%3d].\n", 'A' + i,
			resource.stream_max_size[i].width,
			resource.stream_max_size[i].height,
			resource.stream_max_M[i], resource.stream_max_N[i]);
		printf("  Stream %c long term reference enable : %s.\n", 'A' + i,
			flag[resource.stream_long_ref_enable[i]]);
	}

	printf("      Long Ref B frame : %s\n", flag[resource.long_ref_b_frame]);
	printf("             Sharpen-B : %s\n", flag[sharpen]);
	printf("               Mixer-A : %s\n", flag[resource.mixer_a_enable]);
	printf("               Mixer-B : %s\n", flag[resource.mixer_b_enable]);
	printf("           RAW capture : %s\n", flag[resource.raw_capture_enable]);
	printf("     Rotation possible : %s\n", flag[rotatable]);
	printf("    Lens warp possible : %s\n", flag[lens_warp]);
	printf("    IDSP Upsample Type : %s\n", idsp_upsample[idsp_upsample_type]);
	printf("       EIS delay count : %d\n", resource.eis_delay_count);
	printf("  Encode dummy latency : %d\n", resource.enc_dummy_latency);
	printf("     DSP partition map : 0x%x\n", resource.dsp_partition_map);
	printf("      MCTF privacymask : %s\n", flag[resource.mctf_pm_enable]);
	printf("        Encode RAW RGB : %s\n", flag[enc_raw_rgb]);
	printf("       Encode from MEM : %s\n", flag[enc_from_mem]);
	printf("        Encode RAW YUV : %s\n", flag[enc_raw_yuv]);
	printf("             ME0 scale : %s\n", me0[resource.me0_scale]);
	printf("     High MP stitching : %s\n", flag[resource.is_stitched]);
	printf("   Overflow Protection : %s\n", flag[resource.vin_overflow_protection]);
	if (enc_raw_rgb) {
		printf("              RAW size : %dx%d\n",
			resource.raw_size.width, resource.raw_size.height);
	}
	printf("              ISO Type : %s\n", iso[resource.iso_type]);
	if (hdr_expo_num > 1) {
		printf("              HDR type : %s\n", hdr[resource.hdr_type]);
		printf("          HDR expo num : %d\n", hdr_expo_num);
	}

	if (warp_mode) {
		printf("      Max warp input W : %d\n", resource.max_warp_input_width);
		printf("      Max warp input H : %d\n", resource.max_warp_input_height);
		printf("    Max Vwarp output W : %d\n", resource.v_warped_main_max_width);
		printf("    Max Vwarp output H : %d\n", resource.v_warped_main_max_height);
		printf("     Max warp output W : %d\n", resource.max_warp_output_width);
	}

	printf("  Total DRAM size (MB) : %d\n", ((resource.total_memory_size << 10) / 8));

	/* Debug info */
	if (resource.debug_enable_map & DEBUG_TYPE_STITCH) {
		printf("       Debug Stitching : %s\n", flag[resource.debug_stitched]);
	}
	if (resource.debug_enable_map & DEBUG_TYPE_ISO_TYPE) {
		printf("        Debug ISO type : %s\n", iso[resource.debug_iso_type]);
	}
	if (resource.debug_enable_map & DEBUG_TYPE_CHIP_ID) {
		memset(chip_str, 0, sizeof(chip_str));
		get_chip_id_str(resource.debug_chip_id, chip_str);
		printf("         Debug Chip ID : %s\n", chip_str);
	}

	printf("     extra top row buf : %s\n", flag[resource.extra_top_row_buf_enable]);
	return 0;
}

static int show_h264_encode_config(int stream_id)
{
	struct iav_h264_cfg h264cfg;
	struct iav_stream_cfg streamcfg;
	struct iav_bitrate h264_rc;
	struct iav_h264_pskip h264_pskip;
	char tmp[32];
	int tmp_abs_br_flag;

	memset(tmp, 0, sizeof(tmp));

	memset(&h264cfg, 0, sizeof(h264cfg));
	h264cfg.id = stream_id;
	memset(&streamcfg, 0, sizeof(streamcfg));
	streamcfg.id = stream_id;

	strcpy(tmp, "H.264");
	AM_IOCTL(fd_iav, IAV_IOC_GET_H264_CONFIG, &h264cfg);

	streamcfg.cid = IAV_H264_CFG_RC_STRATEGY;
	AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_CONFIG, &streamcfg);
	tmp_abs_br_flag = streamcfg.arg.h264_rc_strategy.abs_br_flag;

	streamcfg.cid = IAV_H264_CFG_BITRATE;
	AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_CONFIG, &streamcfg);
	h264_rc = streamcfg.arg.h264_rc;

	streamcfg.cid = IAV_H264_CFG_FORCE_PSKIP;
	AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_CONFIG, &streamcfg);
	h264_pskip = streamcfg.arg.h264_pskip;

	BOLD_PRINT("\t%s\n", tmp);
	memset(tmp, 0, sizeof(tmp));

	switch (h264cfg.profile) {
	case H264_PROFILE_BASELINE:
		strcpy(tmp, "Baseline");
		break;
	case H264_PROFILE_MAIN:
		strcpy(tmp, "Main");
		break;
	case H264_PROFILE_HIGH:
		strcpy(tmp, "High");
		break;
	default:
		strcpy(tmp, "Unknown");
		break;
	}
	printf("\t             profile = %s\n", tmp);
	printf("\t                   M = %d\n", h264cfg.M);
	printf("\t                   N = %d\n", h264cfg.N);
	printf("\t        idr interval = %d\n", h264cfg.idr_interval);
	printf("\t           gop model = %s\n",
		(h264cfg.gop_structure == 0) ? "simple":"advanced");
	printf("\t       chrome format = %s\n",
		(h264cfg.chroma_format == H264_CHROMA_YUV420) ? "YUV420" : "MONO");
	printf("\t        mv threshold = %s\n", h264cfg.mv_threshold ? "Enable" : "Disable");
	printf("\t   flat_area_improve = %d\n", h264cfg.flat_area_improve);
	printf("\t     fast_seek_intvl = %d\n", h264cfg.fast_seek_intvl);
	printf("\t         multi_ref_p = %d\n", h264cfg.multi_ref_p);
	printf("\t         intrabias_p = %d\n", h264cfg.intrabias_p);
	printf("\t         intrabias_b = %d\n", h264cfg.intrabias_b);
	printf("\t               alpha = %d\n", h264cfg.deblocking_filter_alpha);
	printf("\t                beta = %d\n", h264cfg.deblocking_filter_beta);
	printf("\t         loop_filter = %d\n", h264cfg.deblocking_filter_enable);
	printf("\t     left frame crop = %d\n", h264cfg.frame_crop_left_offset);
	printf("\t    right frame crop = %d\n", h264cfg.frame_crop_right_offset);
	printf("\t      top frame crop = %d\n", h264cfg.frame_crop_top_offset);
	printf("\t   bottom frame crop = %d\n", h264cfg.frame_crop_bottom_offset);
	printf("\t         vbr_setting = %d\n", h264_rc.vbr_setting);
	printf("\t     average_bitrate = %d\n", h264_rc.average_bitrate);
	printf("\t            adapt_qp = %d\n", h264_rc.adapt_qp);
	printf("\t         i_qp_reduce = %d\n", h264_rc.i_qp_reduce);
	printf("\t         p_qp_reduce = %d\n", h264_rc.p_qp_reduce);
	printf("\t         q_qp_reduce = %d\n", h264_rc.q_qp_reduce);
	printf("\t    log_q_num_plus_1 = %d\n", h264_rc.log_q_num_plus_1);
	printf("\t         qp_max_on_I = %d\n", h264_rc.qp_max_on_I);
	printf("\t         qp_min_on_I = %d\n", h264_rc.qp_min_on_I);
	printf("\t         qp_max_on_B = %d\n", h264_rc.qp_max_on_B);
	printf("\t         qp_min_on_B = %d\n", h264_rc.qp_min_on_B);
	printf("\t         qp_max_on_P = %d\n", h264_rc.qp_max_on_P);
	printf("\t         qp_min_on_P = %d\n", h264_rc.qp_min_on_P);
	printf("\t         qp_max_on_Q = %d\n", h264_rc.qp_max_on_Q);
	printf("\t         qp_min_on_Q = %d\n", h264_rc.qp_min_on_Q);
	printf("\t           skip_flag = %d\n", h264_rc.skip_flag);
	if (h264_rc.max_i_size_KB) {
		printf("\t       max_i_size_KB = %d\n", h264_rc.max_i_size_KB);
	} else {
		printf("\t       max_i_size_KB = Disable\n");
	}
	printf("\t         abs_br_flag = %s\n", tmp_abs_br_flag ? "Enable" : "Disable");
	printf("\t          pskip type = %s\n", h264_pskip.repeat_enable ? "Repeat Pskip" : "Force Pskip");
	printf("\t    repeat pskip num = %d\n", h264_pskip.repeat_num);

	return 0;
}

static int show_mjpeg_encode_config(int stream_id )
{
	struct iav_mjpeg_cfg mjpegcfg;

	mjpegcfg.id = stream_id;
	AM_IOCTL(fd_iav, IAV_IOC_GET_MJPEG_CONFIG, &mjpegcfg);
	BOLD_PRINT0("\tMJPEG\n");
	printf("\tQuality factor = %d\n", mjpegcfg.quality);
	printf("\t Chroma format = %s\n",
		(mjpegcfg.chroma_format == JPEG_CHROMA_YUV420) ? "YUV420" : "MONO");

	return 0;
}

static int show_encode_config(void)
{
	struct iav_stream_format format;
	int i;

	printf("\n[Encode stream config]:\n");
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {

		format.id = i;
		AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_FORMAT, &format);

		COLOR_PRINT(" Stream %c\n ", i + 'A');
		if (format.type == IAV_STREAM_TYPE_H264) {
			show_h264_encode_config(i);
		} else if (format.type == IAV_STREAM_TYPE_MJPEG) {
			show_mjpeg_encode_config(i);
		} else {
			printf("\tencoding not configured\n");
		}
	}
	return 0;
}

static int show_encode_mode_capability(void)
{
	struct iav_enc_mode_cap cap;
	char temp[32];
	char flag[2][16] = {"Disabled", "Enabled"};
	char iso[5][20] = {"LOW", "Middle", "Advanced", "Blend", "High"};
	char hdr[3][20] = {"OFF", "Basic (Preblend)", "Adv (With CE)"};
	char me0[3][16] = {"Disabled", "1/8 X", "1/16 X"};

	memset(&cap, 0, sizeof(cap));
	cap.encode_mode = DSP_CURRENT_MODE;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_ENC_MODE_CAP, &cap);

	switch (cap.encode_mode) {
	case DSP_NORMAL_ISO_MODE:
		strcpy(temp, "Normal ISO");
		break;
	case DSP_MULTI_REGION_WARP_MODE:
		strcpy(temp, "Multi region warping");
		break;
	case DSP_SINGLE_REGION_WARP_MODE:
		strcpy(temp, "Single region warping");
		break;
	case DSP_BLEND_ISO_MODE:
		strcpy(temp, "Blend ISO");
		break;
	case DSP_ADVANCED_ISO_MODE:
		if (cap.iso_type_possible == ISO_TYPE_MIDDLE)
			strcpy(temp, "ISO Plus");
		else if (cap.iso_type_possible == ISO_TYPE_ADVANCED)
			strcpy(temp, "Advanced ISO");
		else
			strcpy(temp, "Unknown");
		break;
	case DSP_HDR_LINE_INTERLEAVED_MODE:
		strcpy(temp, "HDR line");
		break;
	case DSP_HIGH_MP_LOW_DELAY_MODE:
		strcpy(temp, "High MP (Full performance)");
		break;
	case DSP_FULL_FPS_LOW_DELAY_MODE:
		strcpy(temp, "Full FPS (Full performance)");
		break;
	case DSP_MULTI_VIN_MODE:
		strcpy(temp, "Multiple VIN");
		break;
	case DSP_HISO_VIDEO_MODE:
		strcpy(temp, "HISO video");
		break;
	default :
		sprintf(temp, "Unknown mode [%d]", cap.encode_mode);
		break;
	}

	printf("\n[Encode mode capability]:\n");
	printf("  Encode mode : [%s]\n", temp);
	printf("  Supported main buffer size : Min [%dx%d], Max [%dx%d]\n",
		cap.min_main.width, cap.min_main.height,
		cap.max_main.width, cap.max_main.height);
	printf("  Supported max encode streams : %d\n", cap.max_streams_num);
	printf("  Supported min encode stream size : %dx%d\n",
		cap.min_enc.width, cap.min_enc.height);
	printf("  Supported max encode macro blocks : [%d]\n", cap.max_encode_MB);
	printf("        Supported max chroma radius : %d\n",
		(1 << (5 + cap.max_chroma_radius)));
	printf("   Supported max wide chroma radius : %d\n",
		cap.wcr_possible ? (1 << (5 + cap.max_wide_chroma_radius)) : 0);
	printf("\n  Supported possible functions :\n");
	printf("     Stream Rotation : %s\n", flag[cap.rotate_possible]);
	printf("         Raw Capture : %s\n", flag[cap.raw_cap_possible]);
	printf("           VOUT swap : %s\n", flag[cap.vout_swap_possible]);
	printf("           Lens warp : %s\n", flag[cap.lens_warp_possible]);
	printf("    High mega sensor : %s\n", flag[cap.high_mp_possible]);
	printf("  Sensor linear mode : %s\n", flag[cap.linear_possible]);
	printf("            ISO type : %s\n", iso[cap.iso_type_possible]);
	printf("     HDR 2 exposures : %s\n", hdr[cap.hdr_2x_possible]);
	printf("     HDR 3 exposures : %s\n", hdr[cap.hdr_3x_possible]);
	printf("             Mixer B : %s\n", flag[cap.mixer_b_possible]);
	printf("  Wide chroma radius : %s\n", flag[cap.wcr_possible]);
	printf("     BPC PrivacyMask : %s\n", flag[cap.pm_bpc_possible]);
	printf("    MCTF PrivacyMask : %s\n", flag[cap.pm_mctf_possible]);
	printf("           ME0 Scale : %s\n", me0[cap.me0_possible]);
	printf("      Encode RAW RGB : %s\n", flag[cap.enc_raw_rgb_possible]);
	printf("     Encode RAW YUV : %s\n", flag[cap.enc_raw_yuv_possible]);
	printf("     Encode from MEM : %s\n", flag[cap.enc_from_mem_possible]);

	printf("\n");

	return 0;
}

static int show_iav_state(void)
{
	u32 state;
	char state_str[64];

	AM_IOCTL(fd_iav, IAV_IOC_GET_IAV_STATE, &state);

	switch(state) {
	case IAV_STATE_IDLE:
		strcpy(state_str, "IDLE");
		break;
	case IAV_STATE_PREVIEW:
		strcpy(state_str, "Preview");
		break;
	case IAV_STATE_ENCODING:
		strcpy(state_str, "Encoding");
		break;
	case IAV_STATE_STILL_CAPTURE:
		strcpy(state_str, "Still Capture");
		break;
	case IAV_STATE_DECODING:
		strcpy(state_str, "Decoding");
		break;
	case IAV_STATE_TRANSCODING:
		strcpy(state_str, "Transcoding");
		break;
	case IAV_STATE_DUPLEX:
		strcpy(state_str, "Duplex");
		break;
	case IAV_STATE_EXITING_PREVIEW:
		strcpy(state_str, "Exiting Preview");
		break;
	case IAV_STATE_INIT:
		strcpy(state_str, "Init");
		break;
	default:
		strcpy(state_str, "Unknown");
		break;
	}

	printf("\n[IAV state info]:\n");
	printf("   IAV State : %s\n", state_str);

	return 0;
}

static int show_driver_info(void)
{
	int fd_ucode = -1;
	ucode_version_t ucode_ver;
	struct iav_driver_version iav_driver_info;
	char chip_arch[16];

	if ((fd_ucode = open("/dev/ucode", O_RDWR, 0)) < 0) {
		perror("/dev/ucode");
		return -1;
	}

	memset(&ucode_ver, 0, sizeof(ucode_ver));
	AM_IOCTL(fd_ucode, IAV_IOC_GET_UCODE_VERSION, &ucode_ver);
	AM_IOCTL(fd_iav, IAV_IOC_GET_DRIVER_INFO, &iav_driver_info);

	get_chip_arch_str(ucode_ver.chip_arch, chip_arch);

	printf("\n[IAV driver info]:\n");
	printf("   IAV Driver Version : %s-%d.%d.%d (Last updated: %x)\n"
		"   DSP Driver version : %d-%d\n"
		"        Ucode version : %s %d-%d (Last updated: %d%02d%02d)\n",
		iav_driver_info.description, iav_driver_info.major,
		iav_driver_info.minor, iav_driver_info.patch,
		iav_driver_info.mod_time,
		iav_driver_info.api_version, iav_driver_info.idsp_version,
		chip_arch, ucode_ver.edition_num, ucode_ver.edition_ver,
		ucode_ver.year, ucode_ver.month, ucode_ver.day);

	if (fd_ucode >= 0) {
		close(fd_ucode);
		fd_ucode = -1;
	}

	return 0;
}

static int show_chip_info(void)
{
	u32 chip_id;
	char chip_str[128];

	chip_id = IAV_CHIP_ID_UNKNOWN;
	AM_IOCTL(fd_iav, IAV_IOC_GET_CHIP_ID, &chip_id);

	memset(chip_str, 0, sizeof(chip_str));
	get_chip_id_str(chip_id, chip_str);

	COLOR_PRINT0("\n[Chip version]:\n");
	printf("   CHIP Version : %s\n", chip_str);

	return 0;
}

static int show_dram_layout(void)
{
	struct iav_querybuf buf;

	printf("\n[DRAM Layout info]:\n");

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_BSB;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("        [BSB] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_USR;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("        [USR] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_MV;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("         [MV] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_OVERLAY;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("    [OVERLAY] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_QPMATRIX;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("   [QPMATRIX] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_WARP;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("       [WARP] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_QUANT;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("      [QUANT] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_IMG;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("    [IMGPROC] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_PM_MCTF;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("    [PM_MCTF] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_PM_BPC;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("     [PM_BPC] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_BPC;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("        [BPC] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_CMD_SYNC;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("   [CMD_SYNC] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_VCA;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("        [VCA] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_FB_DATA;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("    [FB_DATA] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_FB_AUDIO;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("   [FB_AUDIO] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	memset(&buf, 0, sizeof(buf));
	buf.buf = IAV_BUFFER_DSP;
	AM_IOCTL(fd_iav, IAV_IOC_QUERY_BUF, &buf);
	printf("        [DSP] Base Address: [0x%08X], Size [%8d KB].\n",
		buf.offset, (buf.length >> 10));

	return 0;
}

static int show_feature_set(void)
{
	show_chip_info();

	COLOR_PRINT0("\n[Feature set info]:");

	COLOR_PRINT0("\n[Clock for chips]\n");
	printf(" Clock (MHz) |  S2L22M    S2L33M    S2L55M    S2L63     S2L66    S2L99\n");
	printf("   Cortex    |   600       600       600       816      1008     1008\n");
	printf("    IDSP     |   144       216       216       336       504      504\n");
	printf("    Core     |   144       216       216       312       432      432\n");
	printf("    DRAM     | 456(16b)  528(16b)  528(16b)  600(32b)  660(32b) 660(32b)\n");
	printf("\nNotes:\n");
	printf("  1. Use 'cat /proc/ambarella/clock' to check the clocks.\n");

	COLOR_PRINT0("\n[Mode for chips]\n");
	printf("   Mode          | S2L22M  |  S2L33M  |  S2L55M  |  S2L63  |  S2L66  |  S2L99\n");
	printf("0 (Normal ISO)   |   O     |    O     |    O     |    O    |    O    |    O\n");
	printf("1 (Multi dewarp) |   -     |    -     |    -     |    -    |    -    |    O\n");
	printf("4 (Advanced ISO) |   O     |    O     |    O     |    O    |    O    |    O\n");
	printf("5 (Advanced HDR) |   -     |    -     |    -     |    O    |    O    |    O\n");
	printf("\nNotes:\n");
	printf("  1. 'Multi dewarp' is used to do fisheye dewarp. It's not supported yet.\n");
	printf("  2. 'Advanced ISO' already support to do lens disctortion correction (LDC).\n");
	printf("  3. 'Advanced HDR' is used to do 2X / 3X HDR with contrast enhancement. It's not supported yet.\n");

	COLOR_PRINT0("\n[Features for chips]\n");
	printf(" Mode | Basic HDR | Advanced HDR | Normal ISO | Advanced ISO | Single dewarp | Multi dewarp | High MP encode\n");
	printf("  0   |     O     |      -       |     O      |      -       |       -       |      -       |        O\n");
	printf("  1   |     O*    |      -       |     O      |      -       |       -       |      O       |        O*\n");
	printf("  4   |     O     |      -       |     -      |      O       |       O       |      -       |        O\n");
	printf("  5   |     -     |      O       |     O      |      -       |       -       |      -       |        O\n");
	printf("\nNotes:\n");
	printf("  1. '*' means the feature is not ready yet, but will be implemented later.\n");

	printf("\n");

	return 0;
}

static int show_cmd_examples(void)
{
	COLOR_PRINT0("\n[Example Commands]:\n");
	printf("  Run 3A process first before entering preview:\n");
	printf("    # test_tuning -a ;\n");

	COLOR_PRINT0("\nMode 0:\n");
	printf("  (1) Basic 1080p30:\n");
	printf("    # test_encode -i 1080p --hdr-mode 0 -V 480p --hdmi --enc-mode 0 "
		"--hdr-expo 1 -X --bsize 1080p --bmax 1080p ;\n\n");
	printf("  (2) Basic high mega 5MP30:\n");
	printf("    # test_encode -i 2592x1944 --hdr-mode 0 -V 480p --hdmi "
		"--enc-mode 0 --hdr-expo 1 -X --bsize 2592x1944 --bmax 2592x1944 "
		"-A --smax 2592x1944 -h 2592x1944 ;\n\n");
	printf("  (3) Basic 1080p30 + basic 2X HDR (Sensor needs to support 2X 1080p30 HDR):\n");
	printf("    # test_encode -i 1080p --hdr-mode 1 -V 480p --hdmi --enc-mode 0 "
		"--hdr-expo 2 -X --bsize 1080p --bmax 1080p ;\n\n");
	printf("  (4) Basic high mega 4MP30 + basic 2X HDR (Sensor needs to support 2X 4MP30 HDR):\n");
	printf("    # test_encode -i 2560x1440 --hdr-mode 1 -V 480p --hdmi "
		"--enc-mode 0 --hdr-expo 2 -X --bsize 2560x1440 --bmax 2560x1440 "
		"-A -h 2560x1440 --smax 2560x1440 ;\n\n");

	COLOR_PRINT0("\nMode 1:\n");
	printf("  (1) Prepare command for showing intermediate buffer size needed\n");
	printf("    # test_encode --idle --nopreview ;\n\n");
	printf("  (2) Basic 1080p30 fisheye dewarp\n");
	printf("    # test_encode -i1080p -f 30 -V480p --hdmi -w 1920 -W 1920 "
		"--max-warp-in-height 1088 -X --bsize 1080p --bmaxsize 1080p -P "
		"--binsize 1080p --bsize 1080p --bmaxsize 1080p --enc-mode 1 -J "
		"--btype off -Y --btype enc --bsize 480p -K --btype enc --bsize 480p "
		"-D --smaxsize 1080p --intermediate 2176x1088 ;\n\n");

	COLOR_PRINT0("\nMode 2:\n");
	printf("  (1) Basic 1080p30:\n");
	printf("    # test_encode -i 1080p --hdr-mode 0 -V 480p --hdmi "
		"--enc-mode 2 --hdr-expo 1 --mixer 0 -X --bsize 1080p --bmax 1080p ;\n\n");
	printf("  (2) Advanced 1080p30 2X HDR\n");
	printf("    # test_encode -i 1080p --hdr-mode 1 -V 480p --hdmi "
		"--enc-mode 2 --hdr-expo 2 --mixer 0 -X --bsize 1080p --bmax 1080p ;\n\n");

	COLOR_PRINT0("\nMode 4:\n");
	printf("  (1) Advanced ISO 1080p30\n");
	printf("    # test_encode -i 1080p --hdr-mode 0 -V 1080p --hdmi "
		"--enc-mode 4 --hdr-expo 1 ;\n\n");
	printf("  (2) Advanced ISO 4MP30\n");
	printf("    # test_encode -i 2560x1440 --hdr-mode 0 -V 480p --hdmi "
		"--enc-mode 4 --hdr-expo 1 -X --bsize 2560x1440 --bmax 2560x1440 "
		" -A -h 2560x1440 --smax 2560x1440 ;\n\n");
	printf("  (3) Advanced ISO 1080p30 + basic 2X HDR\n");
	printf("    # test_encode -i 1080p --hdr-mode 1 -V 480p --hdmi "
		"--enc-mode 4 --hdr-expo 2 ;\n\n");
	printf("  (4) Advanced ISO QHD@30 + basic 2X HDR\n");
	printf("    # test_encode -i 2560x1440 --hdr-mode 1 -V 480p --hdmi "
		"--enc-mode 4 --hdr-expo 2 -X --bsize 2560x1440 --bmax 2560x1440 "
		"-A -h 2560x1440 --smax 2560x1440 ;\n\n");

	COLOR_PRINT0("\nMode 5:\n");
	printf("  (1) Advanced 1080p30 2X HDR\n");
	printf("    # test_encode -i 1080p --hdr-mode 1 -V 480p --hdmi --enc-mode 5 "
		"--hdr-expo 2 --mixer 0 -X --bsize 1080p --bmax 1080p ;\n\n");
	printf("  (2) Advanced 1080p30 3X HDR\n");
	printf("    # test_encode -i 1080p --hdr-mode 2 -V 480p --hdmi --enc-mode 5 "
		"--hdr-expo 3 --mixer 0 -X --bsize 1080p --bmax 1080p ;\n\n");

	COLOR_PRINT0("\nNotes:\n");
	printf("  (1) Switching between HDR mode (basic HDR or advanced HDR) and "
		"linear mode, sensor needs to do HW GPIO reset.\n");

	return 0;
}

static int print_qp_histogram(struct iav_qp_histogram *hist)
{
	const int entry_num = 4;
	int i, j, index;
	int mb_sum, qp_sum, mb_entry;

	printf("====== stream [%d], PTS [%d].",
		hist->id, hist->PTS);
	printf("\n====== QP:MB \n");
	mb_sum = qp_sum = 0;
	for (i = 0; i < IAV_QP_HIST_BIN_MAX_NUM / entry_num; ++i) {
		mb_entry = 0;
		printf(" [Set %d] ", i);
		for (j = 0; j < entry_num; ++j) {
			index = i * entry_num + j;
			printf("%2d:%-4d ", hist->qp[index], hist->mb[index]);
			mb_entry += hist->mb[index];
			mb_sum += hist->mb[index];
			qp_sum += hist->qp[index] * hist->mb[index];
		}
		printf("[MBs: %d].\n", mb_entry);
	}
	printf("\n====== Total MB : %d.", mb_sum);
	printf("\n====== Average QP : %d.\n\n", qp_sum / mb_sum);
	return 0;
}

static int show_qp_hist(void)
{
	struct iav_querydesc query_desc;
	int i;

	memset(&query_desc, 0, sizeof(query_desc));
	query_desc.qid = IAV_DESC_QP_HIST;

	AM_IOCTL(fd_iav, IAV_IOC_QUERY_DESC, &query_desc);
	printf("\nQP histogram for [%d] streams.\n\n", query_desc.arg.qphist.stream_num);
	for (i = 0; i < query_desc.arg.qphist.stream_num; ++i) {
		print_qp_histogram(&query_desc.arg.qphist.stream_qp_hist[i]);
	}
	return 0;
}

// rate control variables
static u32 rc_br_table[11] =
{
	256000,  512000,  768000,  1000000,
	1500000, 2000000, 3000000, 4000000,
	6000000, 8000000, 10000000,
};

static u32 rc_reso_table[11] =
{
	1920*1080, 1280*1024, 1280*960, 1280*720,
	1024*768,  800*600,   720*576,  720*480,
	640*480,   352*288,   320*240,
};

static u32 rc_qp_for_vbr_lut[11][11] =
{
	{31, 29, 27, 27, 26, 23, 23, 22, 22, 17, 16},	/* 256 kbps */
	{30, 26, 25, 25, 24, 22, 21, 20, 20, 16, 15},	/* 512 kbps */
	{28, 25, 24, 24, 23, 21, 20, 19, 19, 15, 14},	/* 768 kbps */
	{27, 24, 23, 23, 22, 20, 19, 18, 18, 14, 13},	/* 1 Mbps */
	{26, 24, 22, 22, 21, 19, 18, 17, 17, 12, 11},	/* 1.5 Mbps */
	{25, 23, 22, 21, 19, 18, 17, 16, 16, 11, 10},	/* 2 Mbps */
	{24, 22, 21, 20, 19, 17, 16, 15, 15, 9,  8},	/* 3 Mbps */
	{23, 21, 20, 19, 18, 16, 15, 14, 14, 8,  7},	/* 4 Mbps */
	{22, 20, 19, 18, 17, 15, 14, 13, 12, 5,  1},	/* 6 Mbps */
	{21, 19, 18, 17, 16, 14, 13, 12, 11, 1,  1},	/* 8 Mbps */
	{21, 18, 17, 16, 15, 13, 12, 11, 10, 1,  1},	/* 10 Mbps */
};

static int h264_calc_target_qp(u32 bitrate, u32 resolution)
{
	int i, j;

	for (i = 0; i < ARRAY_SIZE(rc_br_table); i++) {
		if (bitrate <= rc_br_table[i])
			break;
	}

	if (i == ARRAY_SIZE(rc_br_table)) {
		printf("Invalid bitrate: %d\n", bitrate);
		return -1;
	}

	for (j = 0; j < ARRAY_SIZE(rc_reso_table); j++) {
		if (resolution >= rc_reso_table[j])
			break;
	}

	if (j == ARRAY_SIZE(rc_reso_table)){
		printf("Invalid resolution: %d\n", resolution);
		return -1;
	}

	return rc_qp_for_vbr_lut[i][j];
}

static int set_h264_encode_param(int stream)
{
	struct iav_h264_cfg h264cfg;
	struct iav_stream_cfg cfg;
	struct iav_bitrate bitrate;
	struct iav_h264_gop gop;
	struct iav_stream_format format;
	u32 resolution, qp;
	h264_param_t *param = &encode_param[stream].h264_param;

	memset(&h264cfg, 0, sizeof(h264cfg));
	h264cfg.id = stream;
	AM_IOCTL(fd_iav, IAV_IOC_GET_H264_CONFIG, &h264cfg);

	if (param->h264_M_flag)
		h264cfg.M = param->h264_M;

	if (param->h264_N_flag)
		h264cfg.N = param->h264_N;

	if (param->h264_idr_interval_flag)
		h264cfg.idr_interval = param->h264_idr_interval;

	if (param->h264_gop_model_flag)
		h264cfg.gop_structure= param->h264_gop_model;

	if (param->h264_profile_level_flag)
		h264cfg.profile = param->h264_profile_level;

	if (param->h264_chrome_format_flag)
		h264cfg.chroma_format = param->h264_chrome_format;

	if (param->h264_mv_threshold_flag)
		h264cfg.mv_threshold = param->h264_mv_threshold;

	if (param->h264_flat_area_improve_flag)
		h264cfg.flat_area_improve = param->h264_flat_area_improve;

	if (param->h264_fast_seek_intvl_flag)
		h264cfg.fast_seek_intvl= param->h264_fast_seek_intvl;

	if (param->h264_multi_ref_p_flag)
		h264cfg.multi_ref_p = param->h264_multi_ref_p;

	if (param->h264_intrabias_p_flag)
		h264cfg.intrabias_p = param->h264_intrabias_p;

	if (param->h264_intrabias_b_flag)
		h264cfg.intrabias_b = param->h264_intrabias_b;

	if (param->h264_user1_intra_bias_flag)
		h264cfg.user1_intra_bias = param->h264_user1_intra_bias;

	if (param->h264_user1_direct_bias_flag)
		h264cfg.user1_direct_bias = param->h264_user1_direct_bias;

	if (param->h264_user2_intra_bias_flag)
		h264cfg.user2_intra_bias = param->h264_user2_intra_bias;

	if (param->h264_user2_direct_bias_flag)
		h264cfg.user2_direct_bias = param->h264_user2_direct_bias;

	if (param->h264_user3_intra_bias_flag)
		h264cfg.user3_intra_bias = param->h264_user3_intra_bias;

	if (param->h264_user3_direct_bias_flag)
		h264cfg.user3_direct_bias = param->h264_user3_direct_bias;

	// panic mode settings
	if (param->panic_mode_flag) {
		h264cfg.cpb_buf_idc = param->cpb_buf_idc;
		h264cfg.cpb_cmp_idc = param->cpb_cmp_idc;
		h264cfg.en_panic_rc = param->en_panic_rc;
		h264cfg.fast_rc_idc = param->fast_rc_idc;
		h264cfg.cpb_user_size = param->cpb_user_size;
	}

	// h264 syntax settings
	if (param->au_type_flag)
		h264cfg.au_type = param->au_type;

	if (param->h264_deblocking_filter_alpha_flag) {
		h264cfg.deblocking_filter_alpha = param->h264_deblocking_filter_alpha;
	}

	if (param->h264_deblocking_filter_beta_flag) {
		h264cfg.deblocking_filter_beta = param->h264_deblocking_filter_beta;
	}

	if (param->h264_deblocking_filter_enable_flag) {
		h264cfg.deblocking_filter_enable = param->h264_deblocking_filter_enable;
	}

	if (param->h264_frame_crop_left_offset_flag) {
		h264cfg.frame_crop_left_offset = param->h264_frame_crop_left_offset;
	}

	if (param->h264_frame_crop_right_offset_flag) {
		h264cfg.frame_crop_right_offset = param->h264_frame_crop_right_offset;
	}

	if (param->h264_frame_crop_top_offset_flag) {
		h264cfg.frame_crop_top_offset = param->h264_frame_crop_top_offset;
	}

	if (param->h264_frame_crop_bottom_offset_flag) {
		h264cfg.frame_crop_bottom_offset = param->h264_frame_crop_bottom_offset;
	}

	AM_IOCTL(fd_iav, IAV_IOC_SET_H264_CONFIG, &h264cfg);

	// bitrate auto sync settings
	if (param->h264_abs_br_flag) {
		memset(&cfg, 0, sizeof(cfg));
		cfg.id = stream;
		cfg.cid = IAV_H264_CFG_RC_STRATEGY;
		AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_CONFIG, &cfg);

		cfg.arg.h264_rc_strategy.abs_br_flag = param->h264_abs_br;
		AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg);
	}
	// bitrate control settings
	memset(&cfg, 0, sizeof(cfg));
	cfg.id = stream;
	cfg.cid = IAV_H264_CFG_BITRATE;
	AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_CONFIG, &cfg);
	bitrate = cfg.arg.h264_rc;

	format.id = stream;
	AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_FORMAT, &format);
	resolution = format.enc_win.width * format.enc_win.height;

	switch (param->h264_bitrate_control) {
	case IAV_CBR:
		bitrate.vbr_setting = IAV_BRC_SCBR;
		bitrate.average_bitrate = param->h264_cbr_avg_bitrate;
		bitrate.qp_min_on_I = 1;
		bitrate.qp_max_on_I = 51;
		bitrate.qp_min_on_P = 1;
		bitrate.qp_max_on_P = 51;
		bitrate.qp_min_on_B = 1;
		bitrate.qp_max_on_B = 51;
		bitrate.qp_min_on_Q = 15;
		bitrate.qp_max_on_Q = 51;
		bitrate.skip_flag = 0;
		break;

	case IAV_CBR_QUALITY_KEEPING:
		qp = h264_calc_target_qp(param->h264_cbr_avg_bitrate, resolution);
		if (qp < 0)
			return -1;
		bitrate.vbr_setting = IAV_BRC_SCBR;
		bitrate.average_bitrate = param->h264_cbr_avg_bitrate;
		bitrate.qp_min_on_I = 1;
		bitrate.qp_max_on_I = qp * 6 / 5;
		bitrate.qp_min_on_P = 1;
		bitrate.qp_max_on_P = qp * 6 / 5;
		bitrate.qp_min_on_B = 1;
		bitrate.qp_max_on_B = qp * 6 / 5;
		bitrate.qp_min_on_Q = 1;
		bitrate.qp_max_on_Q = qp * 6 / 5;
		bitrate.skip_flag = H264_WITH_FRAME_DROP; // enable frame dropping
		break;

	case IAV_VBR:
		qp = h264_calc_target_qp(param->h264_vbr_min_bitrate, resolution);
		if (qp < 0)
			return -1;
		bitrate.vbr_setting = IAV_BRC_SCBR;
		bitrate.average_bitrate = param->h264_vbr_max_bitrate;
		bitrate.qp_min_on_I = qp;
		bitrate.qp_max_on_I = 51;
		bitrate.qp_min_on_P = qp;
		bitrate.qp_max_on_P = 51;
		bitrate.qp_min_on_B = qp;
		bitrate.qp_max_on_B = 51;
		bitrate.qp_min_on_Q = qp;
		bitrate.qp_max_on_Q = 51;
		bitrate.skip_flag = 0;
		break;

	case IAV_VBR_QUALITY_KEEPING:
		qp = h264_calc_target_qp(param->h264_vbr_min_bitrate, resolution);
		if (qp < 0)
			return -1;
		bitrate.vbr_setting = IAV_BRC_SCBR;
		bitrate.average_bitrate = param->h264_vbr_max_bitrate;
		bitrate.qp_min_on_I = qp;
		bitrate.qp_max_on_I = qp;
		bitrate.qp_min_on_P = qp;
		bitrate.qp_max_on_P = qp;
		bitrate.qp_min_on_B = qp;
		bitrate.qp_max_on_B = qp;
		bitrate.qp_min_on_Q = qp;
		bitrate.qp_max_on_Q = qp;
		bitrate.skip_flag = H264_WITH_FRAME_DROP; // enable frame dropping
		break;

	case IAV_CBR2:
		bitrate.vbr_setting = IAV_BRC_CBR;
		bitrate.average_bitrate = param->h264_cbr_avg_bitrate;
		break;

	default:
		printf("Unknown rate control mode [%d] !\n", param->h264_bitrate_control);
		return -1;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = stream;
	cfg.cid = IAV_H264_CFG_BITRATE;
	cfg.arg.h264_rc = bitrate;
	// Following configurations can be changed during encoding
	if (param->h264_bitrate_control_flag
		|| param->h264_cbr_bitrate_flag
		|| param->h264_vbr_bitrate_flag) {
		AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg);
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = stream;
	cfg.cid = IAV_H264_CFG_GOP;
	if (param->h264_N_flag || param->h264_idr_interval_flag) {
		gop.id = stream;
		gop.N = h264cfg.N;
		gop.idr_interval = h264cfg.idr_interval;
		cfg.arg.h264_gop = gop;
		AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg);
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = stream;
	cfg.cid = IAV_H264_CFG_ZMV_THRESHOLD;
	if (param->h264_mv_threshold_flag) {
		cfg.arg.mv_threshold = h264cfg.mv_threshold;
		AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg);
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = stream;
	cfg.cid = IAV_H264_CFG_FLAT_AREA_IMPROVE;
	if (param->h264_flat_area_improve_flag) {
		cfg.arg.h264_flat_area_improve = h264cfg.flat_area_improve;
		AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg);
	}

	// on the fly update chrome format
	memset(&cfg, 0, sizeof(cfg));
	cfg.id = stream;
	cfg.cid = IAV_STMCFG_CHROMA;
	if (param->h264_chrome_format_flag) {
		cfg.arg.chroma = h264cfg.chroma_format;
		AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg);
	}

	// on the fly update frame drop
	memset(&cfg, 0, sizeof(cfg));
	cfg.id = stream;
	cfg.cid = IAV_H264_CFG_FRAME_DROP;
	if (param->h264_drop_frames_flag) {
		cfg.arg.h264_drop_frames = param->h264_drop_frames;
		AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg);
	}

	// on the fly update enc param
	memset(&cfg, 0, sizeof(cfg));
	cfg.id = stream;
	cfg.cid = IAV_H264_CFG_ENC_PARAM;
	if (param->h264_intrabias_p_flag ||
		param->h264_intrabias_b_flag ||
		param->h264_user1_intra_bias_flag ||
		param->h264_user1_direct_bias_flag ||
		param->h264_user2_intra_bias_flag ||
		param->h264_user2_direct_bias_flag ||
		param->h264_user3_intra_bias_flag ||
		param->h264_user3_direct_bias_flag) {
		cfg.arg.h264_enc.id = stream;
		cfg.arg.h264_enc.intrabias_p = h264cfg.intrabias_p;
		cfg.arg.h264_enc.intrabias_b = h264cfg.intrabias_b;
		cfg.arg.h264_enc.user1_intra_bias = h264cfg.user1_intra_bias;
		cfg.arg.h264_enc.user1_direct_bias = h264cfg.user1_direct_bias;
		cfg.arg.h264_enc.user2_intra_bias = h264cfg.user2_intra_bias;
		cfg.arg.h264_enc.user2_direct_bias = h264cfg.user2_direct_bias;
		cfg.arg.h264_enc.user3_intra_bias = h264cfg.user3_intra_bias;
		cfg.arg.h264_enc.user3_direct_bias = h264cfg.user3_direct_bias;
		AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg);
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.id = stream;
	cfg.cid = IAV_H264_CFG_FORCE_PSKIP;
	if (param->force_pskip_flag) {
		AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg);
		cfg.arg.h264_pskip.repeat_enable = param->force_pskip_repeat_enable;
		cfg.arg.h264_pskip.repeat_num =
			(param->force_pskip_repeat_enable ? param->force_pskip_repeat_num : 0);
		AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg);
	}

	return 0;
}

static int set_mjpeg_encode_param(int stream)
{
	struct iav_mjpeg_cfg mjpeg_cfg;
	struct iav_stream_info info;
	struct iav_stream_cfg stream_cfg;
	jpeg_param_t * param = &(encode_param[stream].jpeg_param);

	memset(&info, 0, sizeof(info));
	info.id = stream;
	AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_INFO, &info);

	// set before encoding
	if (info.state != IAV_STREAM_STATE_ENCODING) {
		mjpeg_cfg.id = stream;
		AM_IOCTL(fd_iav, IAV_IOC_GET_MJPEG_CONFIG, &mjpeg_cfg);
		if (param->quality_changed_flag)
			mjpeg_cfg.quality = param->quality;
		if (param->jpeg_chrome_format_flag)
			mjpeg_cfg.chroma_format = param->jpeg_chrome_format;
		AM_IOCTL(fd_iav, IAV_IOC_SET_MJPEG_CONFIG, &mjpeg_cfg);
	} else {
		// on the fly update quality
		if (param->quality_changed_flag) {
			memset(&stream_cfg, 0, sizeof(stream_cfg));
			stream_cfg.id = stream;
			stream_cfg.cid = IAV_MJPEG_CFG_QUALITY;
			stream_cfg.arg.mjpeg_quality = param->quality;
			AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg);
		}
		// on the fly update chrome format
		if (param->jpeg_chrome_format_flag) {
			memset(&stream_cfg, 0, sizeof(stream_cfg));
			stream_cfg.id = stream;
			stream_cfg.cid = IAV_STMCFG_CHROMA;
			stream_cfg.arg.chroma = param->jpeg_chrome_format;
			AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg);
		}
	}
	return 0;
}

static int set_encode_param(void)
{
	struct iav_stream_format format;
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (!(encode_param_changed_id & (1 << i)))
			continue;

		format.id = i;
		AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_FORMAT, &format);

		if (format.type == IAV_STREAM_TYPE_H264)
			set_h264_encode_param(i);
		else if (format.type == IAV_STREAM_TYPE_MJPEG)
			set_mjpeg_encode_param(i);
		else {
			printf("Please specify stream type (H.264 or mjpeg) first "
				"for stream %c!\n", 'A' + i);
			return -1;
		}
	}
	return 0;
}

static int set_encode_format(void)
{
	struct iav_stream_format format;
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (!(encode_format_changed_id & (1 << i)))
			continue;

		memset(&format, 0, sizeof(format));
		format.id = i;
		AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_FORMAT, &format);

		if (encode_format[i].type_changed_flag) {
			format.type = encode_format[i].type;
		}
		if (encode_format[i].resolution_changed_flag) {
			format.enc_win.width = encode_format[i].width;
			format.enc_win.height = encode_format[i].height;
		}
		if (encode_format[i].offset_changed_flag) {
			format.enc_win.x = encode_format[i].offset_x;
			format.enc_win.y = encode_format[i].offset_y;
		}
		if (encode_format[i].source_changed_flag) {
			format.buf_id = encode_format[i].source;
		}
		if (encode_format[i].duration_flag) {
			format.duration = encode_format[i].duration;
		}
		if (encode_format[i].hflip_flag) {
			format.hflip = encode_format[i].hflip;
		}
		if (encode_format[i].vflip_flag) {
			format.vflip = encode_format[i].vflip;
		}
		if (encode_format[i].rotate_flag) {
			format.rotate_cw = encode_format[i].rotate;
		}
		AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_FORMAT, &format);
	}
	return 0;
}

static int goto_idle(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_ENTER_IDLE, 0);
	printf("IAVIOC_S_IDLE done\n");
	return 0;
}

static int enable_preview(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_ENABLE_PREVIEW, 31);
	printf("enable_preview done\n");
	return 0;
}

static int start_encode(u32 streamid)
{
	struct iav_stream_info streaminfo;
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (streamid & (1 << i)) {
			streaminfo.id = i;
			AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_INFO, &streaminfo);
			if (streaminfo.state == IAV_STREAM_STATE_ENCODING) {
				streamid &= ~(1 << i);
			}
		}
	}
	if (streamid == 0) {
		printf("already in encoding, nothing to do \n");
		return 0;
	}

	AM_IOCTL(fd_iav, IAV_IOC_START_ENCODE, streamid);

	printf("Start encoding for stream 0x%x successfully\n", streamid);
	return 0;
}

//this function will get encode state, if it's encoding, then stop it, otherwise, return 0 and do nothing
static int stop_encode(u32 streamid)
{
	struct iav_stream_info streaminfo;
	int i;

	printf("Stop encoding for stream 0x%x \n", streamid);

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		if (streamid & (1 << i)) {
			streaminfo.id = i;
			AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_INFO, &streaminfo);
			if (streaminfo.state != IAV_STREAM_STATE_ENCODING) {
				streamid &= ~(1 << i);
			}
		}
	}

	if (streamid == 0)
		return 0;
	AM_IOCTL(fd_iav, IAV_IOC_STOP_ENCODE, streamid);

	return 0;
}

static int abort_encode(u32 streamid)
{
	printf("abort encoding for stream 0x%x \n", streamid);
	AM_IOCTL(fd_iav, IAV_IOC_ABORT_ENCODE, streamid);

	return 0;
}

/* some actions like dump bin may need file access, but that should be put into different unit tests */
int dump_idsp_bin(void)
{
#if 0
	int fd;
	u8* buffer;
	iav_dump_idsp_info_t dump_info;
	buffer = malloc(1024*8);
	char full_filename[256];

	dump_info.mode = 1;
	dump_info.pBuffer = buffer;
	AM_IOCTL(fd_iav, IAV_IOC_DUMP_IDSP_INFO, &dump_info);
	sleep(1);
	dump_info.mode = 2;
	AM_IOCTL(fd_iav, IAV_IOC_DUMP_IDSP_INFO, &dump_info);
	if (filename[0] == '\0')
		strcpy(filename, default_filename);

	sprintf(full_filename, "%s.bin", filename);

	if ((fd = create_file(full_filename)) < 0)
		return -1;

	if (write_data(fd, buffer, 1024*8) < 0) {
		perror("write(5)");
		return -1;
	}
	dump_info.mode = 0;
	AM_IOCTL(fd_iav, IAV_IOC_DUMP_IDSP_INFO, &dump_info);

	free(buffer);
#endif

	return 0;
}

static int change_frame_rate(void)
{
	struct iav_stream_cfg streamcfg;
	struct vindev_fps vsrc_fps;
	int i, flag = 0;
	char fps[32];
	u32 fps_hz;

	vsrc_fps.vsrc_id = 0;
	AM_IOCTL(fd_iav, IAV_IOC_VIN_GET_FPS, &vsrc_fps);
	change_fps_to_hz(vsrc_fps.fps, &fps_hz, fps);

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		memset(&streamcfg, 0, sizeof(streamcfg));
		streamcfg.id = i;
		streamcfg.cid = IAV_STMCFG_FPS;
		AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_CONFIG, &streamcfg);
		if (framerate_factor_changed_id & (1 << i)) {
			if (streamcfg.arg.fps.abs_fps_enable || stream_abs_fps_enable[i]) {
				printf("Error! cannot set frame factor for stream [%d] when abs fps enabled.\n",i);
				return -1;
			}
			if (framerate_factor[i][0] > framerate_factor[i][1]) {
				printf("Invalid frame interval value : %d/%d should be less than 1/1\n",
					framerate_factor[i][0],
					framerate_factor[i][1]);
				return -1;
			}
			streamcfg.arg.fps.fps_multi = framerate_factor[i][0];
			streamcfg.arg.fps.fps_div = framerate_factor[i][1];
			printf("The current vin fps is %s, stream [%d] encoding fps will be %s x %d/%d!\n",
				fps, i, fps, framerate_factor[i][0], framerate_factor[i][1]);
			flag = 1;
		}
		if (stream_abs_fps_enabled_id & (1 << i)) {
			if (stream_abs_fps_enable[i] && stream_abs_fps[i] == 0) {
				printf("Error! abs fps isn't set when abs fps enabled for stream [%d].\n", i);
				return -1;
			}
			streamcfg.arg.fps.abs_fps_enable = stream_abs_fps_enable[i];
			flag = 1;
		}
		if (stream_abs_fps_changed_id & (1 << i)) {
			if (streamcfg.arg.fps.abs_fps_enable || stream_abs_fps_enable[i]) {
				streamcfg.arg.fps.abs_fps = stream_abs_fps[i];
				flag = 1;
			} else {
				printf("Error! cannot set abs fps when abs fps disabled for stream[%d].\n", i);
				return -1;
			}
		}

		if (flag == 1) {
			flag = 0;
			AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &streamcfg);
		}
	}
	return 0;
}

static int sync_frame_rate(void)
{
	struct iav_stream_fps_sync fps_sync;
	int i;

	memset(&fps_sync, 0, sizeof(fps_sync));
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (framerate_factor_sync_id & (1 << i)) {
			if (framerate_factor[i][0] > framerate_factor[i][1]) {
				printf("Invalid frame interval value : %d/%d should be less than 1/1\n",
					framerate_factor[i][0],
					framerate_factor[i][1]);
				return -1;
			}
			fps_sync.enable[i] = 1;
			fps_sync.fps[i].fps_multi = framerate_factor[i][0];
			fps_sync.fps[i].fps_div = framerate_factor[i][1];
			printf("Stream [%d] sync frame interval %d/%d \n", i,
				framerate_factor[i][0],
				framerate_factor[i][1]);
		}
	}
	AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_FPS_SYNC, &fps_sync);

	return 0;
}

static int setup_resource_limit(u32 source_buffer_id, u32 stream_id)
{
	int i;
	struct iav_system_resource resource;

	memset(&resource, 0, sizeof(resource));
	if (system_resource_setup.encode_mode_flag) {
		resource.encode_mode = system_resource_setup.encode_mode;
	} else {
		resource.encode_mode = DSP_CURRENT_MODE;
	}
	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource);

	// set source buffer max size
	for (i = 0; i < MAX_SOURCE_BUFFER_NUM; i++) {
		if (source_buffer_id & (1 << i)) {
			resource.buf_max_size[i].width = system_resource_setup.source_buffer_max_size[i].width;
			resource.buf_max_size[i].height = system_resource_setup.source_buffer_max_size[i].height;
		}
		if ((i < IAV_SRCBUF_LAST) &&
			system_resource_setup.extra_dram_buf_changed_id & (1 << i)) {
			resource.extra_dram_buf[i] = system_resource_setup.extra_dram_buf[i];
		}
	}

	// set stream max size
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		if (stream_id & (1 << i)) {
			resource.stream_max_size[i].width = system_resource_setup.stream_max_size[i].width;
			resource.stream_max_size[i].height = system_resource_setup.stream_max_size[i].height;
		}
		if (system_resource_setup.max_gop_M_flag[i]  == 1) {
			resource.stream_max_M[i] = system_resource_setup.max_gop_M[i];
		}
		if (encode_param[i].h264_param.h264_N_flag == 1) {
			resource.stream_max_N[i] = encode_param[i].h264_param.h264_N;
		}
		if (system_resource_setup.stream_long_ref_enable_flag[i] == 1) {
			resource.stream_long_ref_enable[i] = system_resource_setup.stream_long_ref_enable[i];
		}
	}

	if (system_resource_setup.long_ref_b_frame_flag == 1) {
		resource.long_ref_b_frame = system_resource_setup.long_ref_b_frame;
	}

	if (system_resource_setup.rotate_possible_flag) {
		resource.rotate_enable = system_resource_setup.rotate_possible;
	}

	if (system_resource_setup.raw_capture_possible_flag) {
		resource.raw_capture_enable = system_resource_setup.raw_capture_possible;
	}

	if (system_resource_setup.hdr_exposure_num_flag) {
		resource.exposure_num = system_resource_setup.hdr_exposure_num;
	}

	if (system_resource_setup.lens_warp_flag) {
		resource.lens_warp_enable = system_resource_setup.lens_warp;
	}

	if (system_resource_setup.max_enc_num_flag) {
		resource.max_num_encode = system_resource_setup.max_enc_num;
	}

	if (system_resource_setup.enc_raw_rgb_flag) {
		resource.enc_raw_rgb = system_resource_setup.enc_raw_rgb;
	}

	if (system_resource_setup.dsp_partition_map_flag) {
		resource.dsp_partition_map = system_resource_setup.dsp_partition_map;
	}

	if (system_resource_setup.mctf_pm_flag) {
		resource.mctf_pm_enable = system_resource_setup.mctf_pm;
	}

	if (system_resource_setup.vout_swap_flag) {
		resource.vout_swap_enable = system_resource_setup.vout_swap;
	}

	if (system_resource_setup.eis_delay_count_flag) {
		resource.eis_delay_count = system_resource_setup.eis_delay_count;
	}

	if (system_resource_setup.me0_scale_flag) {
		resource.me0_scale = system_resource_setup.me0_scale;
	}

	if (system_resource_setup.enc_from_mem_flag) {
		resource.enc_from_mem = system_resource_setup.enc_from_mem;
	}

	if (system_resource_setup.efm_buf_num_flag) {
		resource.efm_buf_num = system_resource_setup.efm_buf_num;
	}

	if (system_resource_setup.raw_size.resolution_changed_flag) {
		resource.raw_size.width = system_resource_setup.raw_size.width;
		resource.raw_size.height = system_resource_setup.raw_size.height;
	}

	if (system_resource_setup.enc_raw_yuv_flag) {
		resource.enc_raw_yuv = system_resource_setup.enc_raw_yuv;
	}

	if (system_resource_setup.efm_size.resolution_changed_flag) {
		resource.efm_size.width = system_resource_setup.efm_size.width;
		resource.efm_size.height = system_resource_setup.efm_size.height;
	}

	if (system_resource_setup.max_warp_input_width_flag) {
		resource.max_warp_input_width = system_resource_setup.max_warp_input_width;
	}
	if (system_resource_setup.max_warp_input_height_flag) {
		resource.max_warp_input_height = system_resource_setup.max_warp_input_height;
	}
	if (system_resource_setup.max_warp_output_width_flag) {
		resource.max_warp_output_width = system_resource_setup.max_warp_output_width;
	}

	if (system_resource_setup.max_padding_width_flag) {
		resource.max_padding_width = system_resource_setup.max_padding_width;
	}

	if (system_resource_setup.intermediate_buf_size_flag) {
		resource.v_warped_main_max_width = system_resource_setup.v_warped_main_max_width;
		resource.v_warped_main_max_height = system_resource_setup.v_warped_main_max_height;
	}

	if (system_resource_setup.enc_dummy_latency_flag) {
		resource.enc_dummy_latency = system_resource_setup.enc_dummy_latency;
	}

	if (system_resource_setup.idsp_upsample_type_flag) {
		resource.idsp_upsample_type = system_resource_setup.idsp_upsample_type;
	}

	if (vout_flag[VOUT_0]) {
		resource.mixer_a_enable = (vout_display_input[VOUT_0] == AMBA_VOUT_INPUT_FROM_MIXER);
	}
	if (vout_flag[VOUT_1]) {
		resource.mixer_b_enable = (vout_display_input[VOUT_1] == AMBA_VOUT_INPUT_FROM_MIXER);
	}

	if (system_resource_setup.osd_mixer_changed_flag) {
		if (system_resource_setup.osd_mixer == OSD_BLENDING_FROM_MIXER_A) {
			resource.osd_from_mixer_a = 1;
			resource.osd_from_mixer_b = 0;
		} else if (system_resource_setup.osd_mixer == OSD_BLENDING_FROM_MIXER_B) {
			resource.osd_from_mixer_a = 0;
			resource.osd_from_mixer_b = 1;
		} else {
			resource.osd_from_mixer_a = 0;
			resource.osd_from_mixer_b = 0;
		}
	}

	if (system_resource_setup.vin_overflow_protection_flag) {
		resource.vin_overflow_protection= system_resource_setup.vin_overflow_protection;
	}

	/* Debug */
	if (system_resource_setup.debug_iso_type_flag) {
		if (system_resource_setup.debug_iso_type != -1) {
			resource.debug_iso_type = system_resource_setup.debug_iso_type;
			resource.debug_enable_map |= DEBUG_TYPE_ISO_TYPE;
		} else {
			resource.debug_enable_map &= ~DEBUG_TYPE_ISO_TYPE;
		}
	}
	if (system_resource_setup.debug_stitched_flag) {
		if (system_resource_setup.debug_stitched != -1) {
			resource.debug_stitched = system_resource_setup.debug_stitched;
			resource.debug_enable_map |= DEBUG_TYPE_STITCH;
		} else {
			resource.debug_enable_map &= ~DEBUG_TYPE_STITCH;
		}
	}
	if (system_resource_setup.debug_chip_id_flag) {
		if (system_resource_setup.debug_chip_id != -1) {
			resource.debug_chip_id = system_resource_setup.debug_chip_id;
			resource.debug_enable_map |= DEBUG_TYPE_CHIP_ID;
		} else {
			resource.debug_enable_map &= ~DEBUG_TYPE_CHIP_ID;
		}
	}

	if (system_resource_setup.extra_top_row_buf_flag) {
		resource.extra_top_row_buf_enable = system_resource_setup.extra_top_row_buf;
	}

	AM_IOCTL(fd_iav, IAV_IOC_SET_SYSTEM_RESOURCE, &resource);

	return 0;
}

int setup_resource_limit_if_necessary(void)
{
	struct iav_system_resource resource;

	memset(&resource, 0, sizeof(resource));
	resource.encode_mode = DSP_CURRENT_MODE;
	AM_IOCTL(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &resource);

	if (preview_buffer_format[0].resolution_changed_flag &&
		preview_buffer_format[0].height	> 720) {
		printf("Solution to restrict to encode only 1 stream if preview is HD.\n");

		resource.buf_max_size[IAV_SRCBUF_PB].width =
			preview_buffer_format[0].width;
		resource.buf_max_size[IAV_SRCBUF_PB].height =
			preview_buffer_format[0].height;

		resource.max_num_encode = 1;
		resource.max_num_cap_sources = 2;
		resource.stream_max_M[0] = 1;
		resource.stream_max_N[0] = 30;

		AM_IOCTL(fd_iav, IAV_IOC_SET_SYSTEM_RESOURCE, &resource);

		printf("Update system resource limit by current selected preview size"
			" %d x %d, max Number of streams %d \n",
			preview_buffer_format[0].width, preview_buffer_format[0].height,
			resource.max_num_encode);
	}

	return 0;
}

static int setup_source_buffer()
{
	struct iav_srcbuf_setup buf_setup;
	int i;
	int updated;

	memset(&buf_setup, 0, sizeof(buf_setup));
	AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &buf_setup);

	updated = 0;
	for (i = IAV_SRCBUF_FIRST; i < IAV_SRCBUF_LAST_PMN; i++) {
		if (source_buffer_type[i].buffer_type_changed_flag) {
			buf_setup.type[i] =	source_buffer_type[i].buffer_type;
			updated = 1;
		}

		if (source_buffer_format_changed_id & (1 << i)) {
			if (source_buffer_format[i].resolution_changed_flag) {
				buf_setup.size[i].width = source_buffer_format[i].width;
				buf_setup.size[i].height = source_buffer_format[i].height;
				if (i == IAV_SRCBUF_MN) {
					memset(&buf_setup.input, 0, sizeof(buf_setup.input));
				} else {
					memset(&buf_setup.input[i], 0, sizeof(buf_setup.input[i]));
				}
			}
			if (source_buffer_format[i].input_size_changed_flag) {
				buf_setup.input[i].width = source_buffer_format[i].input_width;
				buf_setup.input[i].height = source_buffer_format[i].input_height;
			}
			if (source_buffer_format[i].input_offset_changed_flag) {
				buf_setup.input[i].x = source_buffer_format[i].input_x;
				buf_setup.input[i].y = source_buffer_format[i].input_y;
			}
			if (source_buffer_format[i].dump_interval_flag) {
				buf_setup.dump_interval[i] = source_buffer_format[i].dump_interval;
			}
			if (source_buffer_format[i].dump_duration_flag) {
				buf_setup.dump_duration[i] = source_buffer_format[i].dump_duration;
			}
			updated = 1;
		}
	}
	if (updated) {
		AM_IOCTL(fd_iav, IAV_IOC_SET_SOURCE_BUFFER_SETUP, &buf_setup);
	}

	source_buffer_format_changed_id = 0;
	return 0;
}

static int source_buffer_format_config()
{
	struct iav_srcbuf_format format;
	int i;

	/*
	 * Generally, second, third and fourth source buffer are all downscaled
	 * from main source buffer. So their input windows are equal to the size
	 * of main source buffer. If the input window is explicitly assigned to be
	 * smaller than main source buffer, cropping is performed before downscale.*/
	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; i++) {
		memset(&format, 0, sizeof(format));
		format.buf_id = i;
		AM_IOCTL(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &format);

		if (source_buffer_format[i].input_size_changed_flag &&
				i != IAV_SRCBUF_MN) {
			format.input.width = source_buffer_format[i].input_width;
			format.input.height = source_buffer_format[i].input_height;
		}
		if (source_buffer_format[i].input_offset_changed_flag &&
				i != IAV_SRCBUF_MN) {
			format.input.x = source_buffer_format[i].input_x;
			format.input.y = source_buffer_format[i].input_y;
		}
		if (source_buffer_format[i].resolution_changed_flag) {
			format.size.width = source_buffer_format[i].width;
			format.size.height = source_buffer_format[i].height;
		}
		AM_IOCTL(fd_iav, IAV_IOC_SET_SOURCE_BUFFER_FORMAT, &format);
	}

	return 0;
}

static int force_idr_insertion(int stream)
{
	struct iav_stream_cfg stream_cfg;

	memset(&stream_cfg, 0, sizeof(stream_cfg));
	stream_cfg.id = stream;
	stream_cfg.cid = IAV_H264_CFG_FORCE_IDR;
	AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg);

	return 0;
}

static int long_ref_p_insertion(int stream)
{
	struct iav_stream_cfg stream_cfg;

	memset(&stream_cfg, 0, sizeof(stream_cfg));
	stream_cfg.id = stream;
	stream_cfg.cid = IAV_H264_CFG_LONG_REF_P;
	AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg);

	return 0;
}

static int set_force_idr(void)
{
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (force_idr_id & (1 << i)) {
			if (force_idr_insertion(i) < 0) {
				return -1;
			}
		}
	}

	return 0;
}

static int set_long_ref_p(void)
{
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (long_ref_p_id & (1 << i)) {
			if (long_ref_p_insertion(i) < 0) {
				return -1;
			}
		}
	}

	return 0;
}

static int force_fast_seek_insertion(int stream)
{
	struct iav_stream_cfg stream_cfg;

	memset(&stream_cfg, 0, sizeof(stream_cfg));
	stream_cfg.id = stream;
	stream_cfg.cid = IAV_H264_CFG_FORCE_FAST_SEEK;
	AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg);

	return 0;
}

static int set_force_fast_seek(void)
{
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (force_fast_seek_id & (1 << i)) {
			force_fast_seek_insertion(i);
		}
	}

	return 0;
}

static int change_qp_limit(int stream)
{
	qp_limit_params_t * param = &qp_limit_param[stream];
	struct iav_bitrate *bitrate;
	struct iav_stream_cfg stream_cfg;

	memset(&stream_cfg, 0, sizeof(stream_cfg));
	stream_cfg.id = stream;
	stream_cfg.cid = IAV_H264_CFG_BITRATE;
	AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_CONFIG, &stream_cfg);
	bitrate = &stream_cfg.arg.h264_rc;

	if (param->qp_i_flag) {
		bitrate->qp_min_on_I = param->qp_min_i;
		bitrate->qp_max_on_I = param->qp_max_i;
	}

	if (param->qp_p_flag) {
		bitrate->qp_min_on_P = param->qp_min_p;
		bitrate->qp_max_on_P = param->qp_max_p;
	}

	if (param->qp_b_flag) {
		bitrate->qp_min_on_B = param->qp_min_b;
		bitrate->qp_max_on_B = param->qp_max_b;
	}

	if (param->qp_q_flag) {
		bitrate->qp_min_on_Q = param->qp_min_q;
		bitrate->qp_max_on_Q = param->qp_max_q;
	}

	if (param->adapt_qp_flag)
		bitrate->adapt_qp = param->adapt_qp;

	if (param->i_qp_reduce_flag)
		bitrate->i_qp_reduce = param->i_qp_reduce;

	if (param->p_qp_reduce_flag)
		bitrate->p_qp_reduce = param->p_qp_reduce;

	if (param->q_qp_reduce_flag)
		bitrate->q_qp_reduce = param->q_qp_reduce;

	if (param->log_q_num_plus_1_flag)
		bitrate->log_q_num_plus_1 = param->log_q_num_plus_1;

	if (param->skip_frame_flag)
		bitrate->skip_flag = param->skip_frame;

	if (param->max_i_size_KB_flag)
		bitrate->max_i_size_KB = param->max_i_size_KB;

	stream_cfg.cid = IAV_H264_CFG_BITRATE;
	AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg);

	return 0;
}

static int set_qp_limit(void)
{
	int i;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (qp_limit_changed_id & (1 << i)) {
			change_qp_limit(i);
		}
	}

	return 0;
}

static int set_qp_matrix_mode(int stream)
{
	u32 *addr = NULL;
	u32 width, height, total;
	int i, j, region = 3;
	struct iav_stream_format format;

	addr = (u32 *)(G_qp_matrix_addr +
		(G_qp_matrix_size / MAX_ENCODE_STREAM_NUM * stream));

	memset(&format, 0, sizeof(format));
	format.id = stream;
	AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_FORMAT, &format);

	width = format.enc_win.width / 16;
	height = format.enc_win.height / 16;
	total = (((width * 4) * height) + 31) & (~31);
	memset(addr, 0, total);
	printf("set_qp_matrix_mode : %d.\n", qp_matrix_param[stream].matrix_mode);
	printf("width : %d, height : %d.\n", width, height);
	switch (qp_matrix_param[stream].matrix_mode) {
	case 0:
		break;
	case 1:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width / region; j++)
				addr[i * width + j] = 3;
			for (j = width / region; j < width; j++)
				addr[i * width + j] = 1;
		}
		break;
	case 2:
		for (i = 0; i < height; i++) {
			for (j = 0; j < width / region; j++)
				addr[i * width + j] = 1;
			for (j = width / region; j < width; j++)
				addr[i * width + j] = 3;
		}
		break;
	case 3:
		for (i = 0; i < height / region; i++) {
			for (j = 0; j < width; j++)
				addr[i * width + j] = 3;
		}
		for (i = height / region; i < height; i++) {
			for (j = 0; j < width; j++)
				addr[i * width + j] = 1;
		}
		break;
	case 4:
		for (i = 0; i < height / region; i++) {
			for (j = 0; j < width; j++)
				addr[i * width + j] = 1;
		}
		for (i = height / region; i < height; i++) {
			for (j = 0; j < width; j++)
				addr[i * width + j] = 3;
		}
		break;
	default:
		break;
	}

	for (i = 0; i < height; i++) {
		printf("\n");
		for (j = 0; j < width; j++) {
			printf("%d ", addr[i * width + j]);
		}
	}
	printf("\n");

	return 0;
}

static int change_qp_matrix(void)
{
	struct iav_querybuf querybuf;
	struct iav_qpmatrix *qpmatrix;
	struct iav_stream_cfg stream_cfg;
	int i, j;

	querybuf.buf = IAV_BUFFER_QPMATRIX;
	if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	G_qp_matrix_size = querybuf.length;
	G_qp_matrix_addr = mmap(NULL, G_qp_matrix_size, PROT_READ | PROT_WRITE,
		MAP_SHARED, fd_iav, querybuf.offset);
	if (G_qp_matrix_addr == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++) {
		if (qp_matrix_changed_id & (1 << i)) {
			memset(&stream_cfg, 0, sizeof(stream_cfg));
			stream_cfg.id = i;
			stream_cfg.cid = IAV_H264_CFG_QP_ROI;
			stream_cfg.arg.h264_roi.qpm_no_update = 1;
			AM_IOCTL(fd_iav, IAV_IOC_GET_STREAM_CONFIG, &stream_cfg);

			qpmatrix = &stream_cfg.arg.h264_roi;
			if (qp_matrix_param[i].delta_flag) {
				for (j = 0; j < 3; ++j)
					memcpy(qpmatrix->qp_delta[j], qp_matrix_param[i].delta,
						4 * sizeof(s8));
			}
			if (qp_matrix_param[i].matrix_mode_flag) {
				qpmatrix->enable = !!qp_matrix_param[i].matrix_mode;
				qpmatrix->qpm_no_update = 0;
				qpmatrix->qpm_no_check = 0;
				if (set_qp_matrix_mode(i) < 0) {
					printf("set_qp_matrix_mode for stream [%d] failed!\n", i);
					return -1;
				}
			}

			stream_cfg.cid = IAV_H264_CFG_QP_ROI;
			AM_IOCTL(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg);
		}
	}
	return 0;
}

static int wait_for_next_frame(void)
{
	const int times = 30;
	int i, min, max, avg, sum, value;
	struct timeval start, end;

	printf("== Start to [Wait for Next Frame] ==\n");
	min = 0x0FFFFFFF;
	max = avg = sum = value = 0;
	for (i = 0; i < times; ++i) {
		AM_IOCTL(fd_iav, IAV_IOC_WAIT_NEXT_FRAME, 0);
		gettimeofday(&start, NULL);
		AM_IOCTL(fd_iav, IAV_IOC_WAIT_NEXT_FRAME, 0);
		gettimeofday(&end, NULL);
		value = (end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec;
		if (min > value) {
			min = value;
		}
		if (max < value) {
			max = value;
		}
		sum += value;
		printf(" [%03d] Wait [%06d us] for next frame.\n", i, value);
	}
	avg = sum / times;
	printf("== Max [%06d us] Min [%06d us] Avg [%06d us] ==\n",
		max, min, avg);

	return 0;
}

static int do_show_status(void)
{
	if (show_info_flag & (1 << IAV_SHOW_SYSTEM_STATE)) {
		show_iav_state();
	}

	if (show_info_flag & (1 << IAV_SHOW_ENCODE_CFG)) {
		if (show_encode_config() < 0)
			return -1;
	}

	if (show_info_flag & (1 << IAV_SHOW_STREAM_INFO)) {
		if (show_encode_stream_info() < 0)
			return -1;
	}

	if (show_info_flag & (1 << IAV_SHOW_BUFFER_INFO)) {
		if (show_source_buffer_info() < 0)
			return -1;
	}

	if (show_info_flag & (1 << IAV_SHOW_RESOURCE_INFO)) {
		if (show_resource_limit_info() < 0)
			return -1;
	}

	if (show_info_flag & (1 << IAV_SHOW_ENCMODE_CAP)) {
		show_encode_mode_capability();
	}

	if (show_info_flag & (1 << IAV_SHOW_DRIVER_INFO)) {
		show_driver_info();
	}

	if (show_info_flag & (1 << IAV_SHOW_CHIP_INFO)) {
		show_chip_info();
	}

	if (show_info_flag & (1 << IAV_SHOW_DRAM_LAYOUT)) {
		show_dram_layout();
	}

	if (show_info_flag & (1 << IAV_SHOW_FEATURE_SET)) {
		show_feature_set();
	}

	if (show_info_flag & (1 << IAV_SHOW_CMD_EXAMPPLES)) {
		show_cmd_examples();
	}

	if (show_info_flag & (1 << IAV_SHOW_QP_HIST)) {
		show_qp_hist();
	}

	return 0;
}

static int do_debug_dump(void)
{
	//dump Image DSP setting
	if (dump_idsp_bin_flag) {
		if (dump_idsp_bin() < 0) {
			perror("dump_idsp_bin failed");
			return -1;
		}
	}

	return 0;
}

static int do_stop_encoding(void)
{
	if (stop_encode(stop_stream_id) < 0)
		return -1;
	return 0;
}

static int do_abort_encoding(void)
{
	if (abort_encode(abort_stream_id) < 0)
		return -1;
	return 0;
}

static int do_goto_idle(void)
{
	if (goto_idle() < 0)
		return -1;
	return 0;
}

static int do_vout_setup(void)
{
	if (check_vout() < 0) {
		return -1;
	}

	if (dynamically_change_vout())
		return 0;

	if (vout_flag[VOUT_0] && init_vout(VOUT_0, 0) < 0)
		return -1;
	if (vout_flag[VOUT_1] && init_vout(VOUT_1, 0) < 0)
		return -1;

	return 0;
}

static int do_vin_setup(void)
{
	// select channel: for multi channel VIN (initialize)
	if (channel >= 0) {
		if (select_channel() < 0)
			return -1;
	}

	if (init_vin(vin_mode, hdr_mode) < 0)
		return -1;

	return 0;
}

static int do_change_parameter_during_idle(void)
{
	if (system_resource_limit_changed_flag || vout_flag[VOUT_1]) {
		if (setup_resource_limit(system_resource_setup.source_buffer_max_size_changed_id,
			system_resource_setup.stream_max_size_changed_id) < 0)
			return -1;
	}

	if (source_buffer_format_changed_id || source_buffer_type_changed_flag) {
		if (setup_source_buffer() < 0)
			return -1;
	}

	setup_resource_limit_if_necessary();

	return 0;
}

static int do_enable_preview(void)
{
	if (enable_preview() < 0)
		return -1;

	return 0;
}


static int do_change_encode_format(void)
{
	if (source_buffer_format_changed_id)  {
		if (source_buffer_format_config() < 0)
			return -1;
	}

	if (set_encode_format() < 0)
		return -1;
	return 0;
}


static int do_change_encode_config(void)
{
	if (set_encode_param() < 0)
		return -1;

	return 0;
}

static int do_start_encoding(void)
{
	if (start_encode(start_stream_id) < 0)
		return -1;
	return 0;
}

static int do_real_time_change(void)
{
	if (vin_framerate_flag) {
		if (set_vin_frame_rate() < 0)
			return -1;
	}

	if (framerate_factor_changed_id ||
		stream_abs_fps_enabled_id ||
		stream_abs_fps_changed_id)  {
		if (change_frame_rate() < 0)
			return -1;
	}

	if (framerate_factor_sync_id)  {
		if (sync_frame_rate() < 0)
			return -1;
	}

	if (qp_limit_changed_id) {
		if (set_qp_limit() < 0)
			return -1;
	}

	if (qp_matrix_changed_id) {
		if (change_qp_matrix() < 0)
			return -1;
	}

	if (force_idr_id) {
		if (set_force_idr() < 0)
			return -1;
	}


	if (force_fast_seek_id) {
		if (set_force_fast_seek() < 0)
			return -1;
	}

	if (long_ref_p_id) {
		if (set_long_ref_p() < 0)
			return -1;
	}

	return 0;
}

static int do_debug_setup(void)
{
	struct iav_debug_cfg debug;

	memset(&debug, 0, sizeof(debug));

	if (debug_setup.check_disable_flag) {
		debug.id = 0;
		debug.cid = IAV_DEBUGCFG_CHECK;
		debug.arg.enable = debug_setup.check_disable;
		AM_IOCTL(fd_iav, IAV_IOC_SET_DEBUG_CONFIG, &debug);
	}
	if (debug_setup.wait_frame_flag) {
		wait_for_next_frame();
	}
	if (debug_setup.audit_int_enable_flag) {
		debug.cid = IAV_DEBUGCFG_AUDIT;
		debug.arg.audit.enable = debug_setup.audit_int_enable;
		debug.arg.audit.id = 0;
		AM_IOCTL(fd_iav, IAV_IOC_SET_DEBUG_CONFIG, &debug);
		debug.arg.audit.id = 1;
		AM_IOCTL(fd_iav, IAV_IOC_SET_DEBUG_CONFIG, &debug);
	}
	if (debug_setup.audit_id_flag) {
		debug.cid = IAV_DEBUGCFG_AUDIT;
		debug.arg.audit.id = debug_setup.audit_id;
		AM_IOCTL(fd_iav, IAV_IOC_GET_DEBUG_CONFIG, &debug);

		printf("==== [Audit info] ====\n");
		printf("     Update : %s\n", debug.arg.audit.enable ?
			"Enable" : "Disable");
		printf("     IRQ NO : %s\n", debug.arg.audit.id ? "VENC" : "VDSP");
		printf("  ISR times : %llu\n", debug.arg.audit.cnt);
		printf("   Avg (us) : %llu\n", debug.arg.audit.cnt ?
			(debug.arg.audit.sum / debug.arg.audit.cnt) : 0);
		printf("   Max (us) : %d\n", debug.arg.audit.max);
		printf("   Min (us) : %d\n", debug.arg.audit.min);
	}

	return 0;
}

static int do_dsp_clcok_set(void)
{
	AM_IOCTL(fd_iav, IAV_IOC_SET_DSP_CLOCK_STATE, dsp_clock_state_disable);
	return 0;
}

int main(int argc, char **argv)
{
	int do_show_status_flag = 0;
	int do_debug_dump_flag = 0;
	int do_stop_encoding_flag = 0;
	int do_abort_encoding_flag = 0;
	int do_goto_idle_flag = 0;
	int do_vout_setup_flag = 0;
	int do_vin_setup_flag = 0;
	int do_change_parameter_during_idle_flag  = 0;
	int do_enable_preview_flag = 0;
	int do_change_encode_format_flag = 0;
	int do_change_encode_config_flag = 0;
	int do_start_encoding_flag = 0;
	int do_real_time_change_flag = 0;
	int do_debug_setup_flag = 0;
//	int do_change_encode_mode_flag = 0;

	if (argc < 2) {
		usage();
		return -1;
	}

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	if (init_param(argc, argv) < 0)
		return -1;

/********************************************************
 *  process parameters to get flag
 *******************************************************/
	//show message flag
	if (show_info_flag) {
		do_show_status_flag  = 1;
	}

	//debug dump flag
	if (dump_idsp_bin_flag)  {
		do_debug_dump_flag = 1;
	}

	//change encode mode flag
	if (system_resource_setup.encode_mode_flag ||
		system_resource_setup.rotate_possible_flag) {
//		do_change_encode_mode_flag = 1;
	}

	//stop encoding flag
	stop_stream_id |= restart_stream_id;
	if (vin_flag || source_buffer_type_changed_flag ||
		(source_buffer_format_changed_id & (1 << IAV_SRCBUF_MN))) {
		stop_stream_id = ALL_ENCODE_STREAMS;
	}
	if (stop_stream_id)  {
		do_stop_encoding_flag = 1;
	}
	if (abort_stream_id)  {
		do_abort_encoding_flag = 1;
	}

	//go to idle (disable preview) flag
	if (channel < 0) {
		//channel is -1 means VIN is single channel
		if (vout_flag[VOUT_0] || vout_flag[VOUT_1] || vin_flag ||
			(source_buffer_format_changed_id & (1 << IAV_SRCBUF_MN)) ||
			source_buffer_type_changed_flag ||
			system_resource_limit_changed_flag ||
			system_resource_setup.osd_mixer_changed_flag ||
			idle_cmd) {
			do_goto_idle_flag = 1;
		}
	}

	//vout setup flag
	if (vout_flag[VOUT_0] || vout_flag[VOUT_1]) {
		do_vout_setup_flag = 1;
	}

	//vin setup flag
	if (vin_flag) {
		do_vin_setup_flag = 1;
	}

	//source buffer flag config during idle flag
	if (do_goto_idle_flag == 1) {
		do_change_parameter_during_idle_flag = 1;
	}

	//enable preview flag
	if (do_goto_idle_flag == 1 && !nopreview_flag) {
		do_enable_preview_flag = 1;
	}

	//set encode format flag
	if (encode_format_changed_id || source_buffer_format_changed_id) {
		do_change_encode_format_flag = 1;
	}

	//set encode param flag
	if (encode_param_changed_id) {
		do_change_encode_config_flag = 1;
	}

	//encode start flag
	start_stream_id |= restart_stream_id;
	if (start_stream_id) {
		do_start_encoding_flag = 1;
	}

	//real time change flag
	if (vin_framerate_flag ||
		framerate_factor_changed_id ||
		framerate_factor_sync_id ||
		stream_abs_fps_enabled_id ||
		stream_abs_fps_changed_id ||
		force_idr_id ||
		force_fast_seek_id ||
		qp_limit_changed_id ||
		intra_mb_rows_changed_id ||
		qp_matrix_changed_id ||
		long_ref_p_id) {
		do_real_time_change_flag = 1;
	}

	//debug setup flag
	if (debug_setup_flag) {
		do_debug_setup_flag = 1;
	}

/********************************************************
 *  check dependency base on flag
 *******************************************************/
/*	if (do_change_encode_mode_flag && !do_vout_setup_flag) {
		printf("Please specify VOUT parameter when encode mode is changed!\n");
		printf("E.g.:\n  test_encode -V 480p --hdmi --enc-mode 0\n");
		printf("  test_encode -V 480p --hdmi --mixer 0 --enc-mode 1\n");
		printf("  test_encode -V 480p --hdmi --enc-mode 3\n");
		return -1;
	}
*/
/********************************************************
 *  execution base on flag
 *******************************************************/
	//show message
	if (do_show_status_flag) {
		if (do_show_status() < 0)
			return -1;
	}

	//debug dump
	if (do_debug_dump_flag) {
		if (do_debug_dump() < 0)
			return -1;
	}

	//stop encoding
	if (do_stop_encoding_flag) {
		if (do_stop_encoding() < 0)
			return -1;
	}
	//abort encoding
	if (do_abort_encoding_flag) {
		if (do_abort_encoding() < 0)
			return -1;
	}

	//disable preview (goto idle)
	if (do_goto_idle_flag) {
		if (do_goto_idle() < 0)
			return -1;
	}

	//vout setup
	if (do_vout_setup_flag) {
		if (do_vout_setup() < 0)
			return -1;
	}

	//vin setup
	if (do_vin_setup_flag) {
		if (do_vin_setup() < 0)
			return -1;
	}

	//change source buffer paramter that needs idle state
	if (do_change_parameter_during_idle_flag) {
		if (do_change_parameter_during_idle() < 0)
			return -1;
	}

	//enable preview
	if (do_enable_preview_flag) {
		if (do_enable_preview() < 0)
			return -1;
	}

	//change encoding format
	if (do_change_encode_format_flag) {
		if (do_change_encode_format() < 0)
			return -1;
	}

	//change encoding param
	if (do_change_encode_config_flag) {
		if (do_change_encode_config() < 0)
			return -1;
	}

	//real time change encoding parameter
	if (do_real_time_change_flag) {
		if (do_real_time_change() < 0)
			return -1;
	}

	if (do_debug_setup_flag) {
		if (do_debug_setup() < 0)
			return -1;
	}

	if (dsp_clock_state_disable_flag) {
		if (do_dsp_clcok_set() < 0)
			return -1;
	}

	//start encoding
	if (do_start_encoding_flag) {
		if (do_start_encoding() < 0)
			return -1;
	}

	return 0;
}

