/*
 * iav_ioctl.h
 *
 * History:
 *	2013/03/12 - [Cao Rongrong] Created file
 *	2013/12/12 - [Jian Tang] Modified file
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

#ifndef __IAV_IOCTL_H__
#define __IAV_IOCTL_H__

#include <basetypes.h>
#include <linux/ioctl.h>

#include <iav_common.h>
#include <iav_vin_ioctl.h>
#include <iav_vout_ioctl.h>

enum {
	DSP_CURRENT_MODE = 0xFF,
	DSP_NORMAL_ISO_MODE = 0x0,
	DSP_MULTI_REGION_WARP_MODE = 0x1,
	DSP_BLEND_ISO_MODE = 0x2,
	DSP_SINGLE_REGION_WARP_MODE = 0x3,
	DSP_ADVANCED_ISO_MODE = 0x4,
	DSP_HDR_LINE_INTERLEAVED_MODE = 0x5,
	DSP_HIGH_MP_LOW_DELAY_MODE = 0x6,
	DSP_FULL_FPS_LOW_DELAY_MODE = 0x7,
	DSP_MULTI_VIN_MODE = 0x8,
	DSP_HISO_VIDEO_MODE = 0x9,
	DSP_ENCODE_MODE_TOTAL_NUM,
	DSP_ENCODE_MODE_FIRST = DSP_NORMAL_ISO_MODE,
	DSP_ENCODE_MODE_LAST = DSP_ENCODE_MODE_TOTAL_NUM,
};

enum iav_chip_id {
	IAV_CHIP_ID_UNKNOWN = -1,

	/* S2L */
	IAV_CHIP_ID_S2L_22M		= 0,
	IAV_CHIP_ID_S2L_33M		= 1,
	IAV_CHIP_ID_S2L_55M		= 2,
	IAV_CHIP_ID_S2L_99M		= 3,
	IAV_CHIP_ID_S2L_63		= 4,
	IAV_CHIP_ID_S2L_66		= 5,
	IAV_CHIP_ID_S2L_88		= 6,
	IAV_CHIP_ID_S2L_99		= 7,
	IAV_CHIP_ID_S2L_TEST		= 8,
	IAV_CHIP_ID_S2L_22		= 9,
	IAV_CHIP_ID_S2L_33MEX	= 10,
	IAV_CHIP_ID_S2L_33EX	= 11,
	IAV_CHIP_ID_S2L_LAST,
	IAV_CHIP_ID_S2LM_FIRST	= IAV_CHIP_ID_S2L_22M,
	IAV_CHIP_ID_S2LM_LAST	= IAV_CHIP_ID_S2L_99M + 1,
	IAV_CHIP_ID_S2L_FIRST	= IAV_CHIP_ID_S2L_63,
	IAV_CHIP_ID_S2L_NUM		= IAV_CHIP_ID_S2L_LAST - IAV_CHIP_ID_S2LM_FIRST,

	/* S3L */
	IAV_CHIP_ID_S3L_33M		= 0,
	IAV_CHIP_ID_S3L_55M		= 1,
	IAV_CHIP_ID_S3L_99M		= 2,
	IAV_CHIP_ID_S3L_66		= 3,
	IAV_CHIP_ID_S3L_99		= 4,
	IAV_CHIP_ID_S3L_LAST,
	IAV_CHIP_ID_S3LM_FIRST	= IAV_CHIP_ID_S3L_33M,
	IAV_CHIP_ID_S3LM_LAST	= IAV_CHIP_ID_S3L_99M + 1,
	IAV_CHIP_ID_S3L_FIRST	= IAV_CHIP_ID_S3L_66,
	IAV_CHIP_ID_S3L_NUM		= IAV_CHIP_ID_S3L_LAST - IAV_CHIP_ID_S3LM_FIRST,
};

enum {
	DSP_CLOCK_NORMAL_STATE = 0,
	DSP_CLOCK_OFF_STATE = 1,
};

struct iav_window {
	u32 width;
	u32 height;
};

struct iav_offset {
	u32 x;
	u32 y;
};

struct iav_rect {
	u32 width;
	u32 height;
	u32 x;
	u32 y;
};

struct iav_driver_version {
	int	arch;
	int	model;
	int	major;
	int	minor;
	int	patch;
	u32	mod_time;
	char	description[64];
	u32	api_version;
	u32	idsp_version;
	u32	reserved[2];
};

struct iav_driver_dsp_info {
	struct iav_driver_version drv_ver;
	u32 dspid;
	u8 dspout[4];
	u8 dspin[8];
};

struct iav_dsp_hash {
	u8 input[32];
	u8 output[4];
};

struct iav_dsplog_setup {
	int	cmd;
	u32	args[8];
};

enum iav_state {
	IAV_STATE_IDLE = 0,
	IAV_STATE_PREVIEW = 1,
	IAV_STATE_ENCODING = 2,
	IAV_STATE_STILL_CAPTURE = 3,
	IAV_STATE_DECODING = 4,
	IAV_STATE_TRANSCODING = 5,
	IAV_STATE_DUPLEX = 6,
	IAV_STATE_EXITING_PREVIEW = 7,
	IAV_STATE_INIT = 0xFF,
};

enum iav_system_params {
	IAV_STREAM_MAX_NUM_IMPL = 4,
	IAV_STREAM_MAX_NUM_EXTRA = 0,
	IAV_STREAM_MAX_NUM_ALL = (IAV_STREAM_MAX_NUM_IMPL + IAV_STREAM_MAX_NUM_EXTRA),
};

enum iav_hdr_type {
	HDR_TYPE_OFF = 0,
	HDR_TYPE_BASIC = 1,
	HDR_TYPE_ADV_WITH_CE = 2,
	HDR_TYPE_TOTAL_NUM,
};

enum iav_iso_type {
	ISO_TYPE_LOW = 0,
	ISO_TYPE_MIDDLE = 1,
	ISO_TYPE_ADVANCED = 2,
	ISO_TYPE_BLEND = 3,
	ISO_TYPE_HIGH = 4,
	ISO_TYPE_TOTAL_NUM,
};

enum iav_idsp_upsample_type {
	IDSP_UPSAMPLE_TYPE_OFF = 0,
	IDSP_UPSAMPLE_TYPE_25FPS = 1,
	IDSP_UPSAMPLE_TYPE_30FPS = 2,
	IDSP_UPSAMPLE_TYPE_TOTAL_NUM,
};

enum iav_eis_delay_count {
	EIS_DELAY_COUNT_OFF = 0,
	EIS_DELAY_COUNT_MAX = 2,
};

enum iav_me0_scale {
	ME0_SCALE_OFF = 0,
	ME0_SCALE_8X = 1,
	ME0_SCALE_16X = 2,
	ME0_SCALE_TOTAL_NUM,
};

enum iav_debug_type {
	DEBUG_TYPE_STITCH = (1 << 0),
	DEBUG_TYPE_ISO_TYPE = (1 << 1),
	DEBUG_TYPE_CHIP_ID = (1 << 2),
};

struct iav_res_s3l {
	u32 num_aaa_buf : 4;
	u32 num_vin_stats_buf : 4;
	u32 reserved : 24;
};

// Do Not change the definition order
#include <iav_types_arch.h>

enum iav_chroma_radius_num {
	CHROMA_RADIUS_32 = 0,
	CHROMA_RADIUS_64 = 1,
	CHROMA_RADIUS_128 = 2,
};

struct iav_enc_mode_cap {
	u8 encode_mode;
	u8 max_streams_num;
	u8 max_chroma_radius : 2;
	u8 max_wide_chroma_radius : 2;
	u8 reserved0 : 4;
	u8 reserved1;
	u32 max_encode_MB;
	u32 rotate_possible : 1;
	u32 raw_cap_possible : 1;
	u32 vout_swap_possible : 1;
	u32 lens_warp_possible : 1;
	u32 iso_type_possible : 3;
	u32 enc_raw_rgb_possible : 1;
	u32 high_mp_possible : 1;
	u32 linear_possible : 1;
	u32 hdr_2x_possible : 2;
	u32 hdr_3x_possible : 2;
	u32 mixer_b_possible : 1;
	u32 pm_bpc_possible : 1;
	u32 pm_mctf_possible : 1;
	u32 wcr_possible : 1;			/* wide chroma radius */
	u32 me0_possible : 2;
	u32 enc_from_mem_possible : 1;
	u32 enc_raw_yuv_possible : 1;
	u32 reserved_possible : 10;
	struct iav_window min_main;
	struct iav_window max_main;
	struct iav_window min_enc;
};

struct iav_enc_buf_cap {
	u32 buf_id;
	struct iav_window max;
	u16 max_zoom_in_factor;
	u16 max_zoom_out_factor;
};

enum iav_buffer_id {
	IAV_BUFFER_DSP = 0,
	IAV_BUFFER_BSB = 1,
	IAV_BUFFER_USR = 2,
	IAV_BUFFER_MV = 3,
	IAV_BUFFER_OVERLAY = 4,
	IAV_BUFFER_QPMATRIX = 5,
	IAV_BUFFER_WARP = 6,
	IAV_BUFFER_QUANT = 7,
	IAV_BUFFER_IMG = 8,
	IAV_BUFFER_PM_BPC = 9,
	IAV_BUFFER_BPC = 10,
	IAV_BUFFER_CMD_SYNC = 11,
	IAV_BUFFER_PM_MCTF = 12,
	IAV_BUFFER_VCA = 13,
	IAV_BUFFER_FB_DATA = 14,
	IAV_BUFFER_FB_AUDIO = 15,
	IAV_BUFFER_NUM,
	IAV_BUFFER_FIRST = IAV_BUFFER_DSP,
	IAV_BUFFER_LAST = IAV_BUFFER_NUM,
};

struct iav_querybuf {
	enum iav_buffer_id buf;		/* the buffer queryed */
	u32 length;
	u32 offset;
};

enum iav_dsp_sub_buffer_id {
	IAV_DSP_SUB_BUF_RAW = 0,
	/* For dewarp mode, this will be pre-main */
	IAV_DSP_SUB_BUF_MAIN_YUV = 1,
	IAV_DSP_SUB_BUF_MAIN_ME1 = 2,
	IAV_DSP_SUB_BUF_PREV_A_YUV = 3,
	IAV_DSP_SUB_BUF_PREV_A_ME1 = 4,
	IAV_DSP_SUB_BUF_PREV_B_YUV = 5,
	IAV_DSP_SUB_BUF_PREV_B_ME1 = 6,
	IAV_DSP_SUB_BUF_PREV_C_YUV = 7,
	IAV_DSP_SUB_BUF_PREV_C_ME1 = 8,
	IAV_DSP_SUB_BUF_POST_MAIN_YUV = 9,
	IAV_DSP_SUB_BUF_POST_MAIN_ME1 = 10,
	IAV_DSP_SUB_BUF_INT_MAIN_YUV = 11,
	IAV_DSP_SUB_BUF_EFM_YUV = 12,
	IAV_DSP_SUB_BUF_EFM_ME1 = 13,
	IAV_DSP_SUB_BUF_NUM,
	IAV_DSP_SUB_BUF_FIRST = IAV_DSP_SUB_BUF_RAW,
	IAV_DSP_SUB_BUF_LAST = IAV_DSP_SUB_BUF_NUM,
	IAV_SUB_BUF_MAX_NUM = IAV_DSP_SUB_BUF_NUM,
};

struct iav_query_subbuf {
	enum iav_buffer_id buf_id;	/* the buffer queried */
	u32 sub_buf_map;		/* the queried sub buffers */
	u32 length[IAV_SUB_BUF_MAX_NUM];
	u32 daddr[IAV_SUB_BUF_MAX_NUM];
	u32 offset[IAV_SUB_BUF_MAX_NUM];
};

struct iav_gdma_copy {
	u32 src_offset;
	u32 dst_offset;
	u16 src_pitch;
	u16 dst_pitch;
	u16 width;
	u16 height;
	u16 src_mmap_type;
	u16 dst_mmap_type;
};

enum iav_desc_id {
	IAV_DESC_FRAME = 0,
	IAV_DESC_STATIS = 1,
	IAV_DESC_RAW = 2,
	IAV_DESC_YUV = 3,
	IAV_DESC_ME1 = 4,
	IAV_DESC_ME0 = 5,
	IAV_DESC_BUFCAP = 6,
	IAV_DESC_QP_HIST = 7,
	IAV_DESC_NUM,
	IAV_DESC_FIRST = IAV_DESC_FRAME,
	IAV_DESC_LAST = IAV_DESC_NUM,
};

enum iav_stream_type {
	IAV_STREAM_TYPE_NONE = 0x0,
	IAV_STREAM_TYPE_H264,
	IAV_STREAM_TYPE_MJPEG,
	IAV_STREAM_TYPE_H265,
	IAV_STREAM_TYPE_NUM,
	IAV_STREAM_TYPE_INVALID = 0xFF,
};

enum iav_pic_type {
	IAV_PIC_TYPE_MJPEG_FRAME = 0,
	IAV_PIC_TYPE_IDR_FRAME = 1,
	IAV_PIC_TYPE_I_FRAME = 2,
	IAV_PIC_TYPE_P_FRAME = 3,
	IAV_PIC_TYPE_B_FRAME = 4,
	IAV_PIC_TYPE_P_FAST_SEEK_FRAME = 5,
};

enum iav_yuv_format {
	IAV_YUV_FORMAT_YUV422 = 0,
	IAV_YUV_FORMAT_YUV420 = 1,
	IAV_YUV_FORMAT_YUV400 = 2,
};

struct iav_framedesc {
	u32 id;				/* read bits desc for all streams if -1 */
	u32 time_ms;		/* timeout in ms; -1 means non-blocking, 0 means blocking */
	u32 data_addr_offset;
	enum iav_stream_type stream_type;
	u32 pic_type		: 3;
	u32 stream_end		: 1;
	u32 jpeg_quality	: 8;
	u32 instant_fetch 	: 1;
	u32 reserved		: 3;
	u32 bitrate_kbps	: 16;
	u32 mv_data_offset;
	u64 arm_pts;
	u64 dsp_pts;
	u32 frame_num;
	u32 session_id;
	u32 size;
	struct iav_window reso;
	u32 reserved1;
	u64 enc_done_ts;
};

/* mv descriptor reported by DSP */
struct iav_mv {
	int x : 15;			// Bit [14:0] is the x component in signed 15-bit format
	int y : 12;			// Bit [26:15] is the y component in signed 12-bit format
	int reserved : 5;
};

struct iav_statisdesc {
	u32 id;
	u32 time_ms;		/* timeout in ms; -1 means non-blocking, 0 means blocking */
	u32 data_addr_offset;
	u32 width;
	u32 height;
	u32 pitch;
	u64 arm_pts;
	u32 dsp_pts;
	u32 frame_num;
	u32 session_id;
	u32 size;
};

enum IAV_BUFCAP_FLAG {
	IAV_BUFCAP_NONBLOCK = (1 << 0),
};

struct iav_rawbufdesc {
	u32 raw_addr_offset;
	u32 width;
	u32 height;
	u32 pitch;
	u32 flag;
	u32 reserved;
	u64 mono_pts;
};

struct iav_yuvbufdesc {
	enum iav_srcbuf_id buf_id;
	u32 y_addr_offset;
	u32 uv_addr_offset;
	u32 width;
	u32 height;
	u32 pitch;
	u32 seq_num;
	u32 format;
	u32 flag;
	u32 dsp_pts;
	u64 mono_pts;
};

struct iav_mebufdesc {
	enum iav_srcbuf_id buf_id;
	u32 data_addr_offset;
	u32 width;
	u32 height;
	u32 pitch;
	u32 seq_num;
	u32 flag;
	u32 dsp_pts;
	u64 mono_pts;
};

struct iav_yuv_cap {
	u32 y_addr_offset;
	u32 uv_addr_offset;
	u32 width;
	u32 height;
	u32 pitch;
	u32 seq_num;
	u32 format;
	u32 dsp_pts;
	u64 mono_pts;
};

struct iav_me_cap {
	u32 data_addr_offset;
	u32 width;
	u32 height;
	u32 pitch;
	u32 seq_num;
	u32 dsp_pts;
	u64 mono_pts;
};

struct iav_bufcapdesc {
	u32	flag;
	struct iav_yuv_cap yuv[IAV_SRCBUF_NUM];
	struct iav_me_cap me1[IAV_SRCBUF_NUM];
	struct iav_me_cap me0[IAV_SRCBUF_NUM];
};

/* Total number of bins for QP histogram */
#define	IAV_QP_HIST_BIN_MAX_NUM	(16)
struct iav_qp_histogram {
	u32 id;
	u32 PTS;
	u8 qp[IAV_QP_HIST_BIN_MAX_NUM];	// QP value for each bin
	u16 mb[IAV_QP_HIST_BIN_MAX_NUM];	// Macroblocks used per each bin
};

struct iav_qphistdesc {
	struct iav_qp_histogram stream_qp_hist[IAV_STREAM_MAX_NUM_IMPL];
	u32 stream_num;
	u32 seq_num;
};

struct iav_querydesc {
	enum iav_desc_id qid;	/* query desc id */
	union {
		struct iav_framedesc frame;
		struct iav_statisdesc statis;
		struct iav_rawbufdesc raw;
		struct iav_yuvbufdesc yuv;
		struct iav_mebufdesc me1;
		struct iav_mebufdesc me0;
		struct iav_bufcapdesc bufcap;
		struct iav_qphistdesc qphist;
	} arg;
};

enum iav_srcbuf_type {
	IAV_SRCBUF_TYPE_OFF = 0,
	IAV_SRCBUF_TYPE_ENCODE = 1,
	IAV_SRCBUF_TYPE_PREVIEW = 2,
	IAV_SRCBUF_TYPE_VCA = 3,
};

enum iav_srcbuf_state {
	IAV_SRCBUF_STATE_UNKNOWN = 0,
	IAV_SRCBUF_STATE_IDLE = 1,
	IAV_SRCBUF_STATE_BUSY = 2,
	IAV_SRCBUF_STATE_ERROR = 255,
};

#define MAX_NUM_VCA_DUMP_DURATION (32)

struct iav_srcbuf_setup {
	struct iav_window size[IAV_SRCBUF_NUM];
	struct iav_rect input[IAV_SRCBUF_NUM];
	enum iav_srcbuf_type type[IAV_SRCBUF_NUM];

	/* Following fields only work for dewarp mode */
	u8 unwarp[IAV_SRCBUF_NUM];

	/* Following fields only work for vca */
	u8 dump_interval[IAV_SRCBUF_NUM];
	u16 dump_duration[IAV_SRCBUF_NUM];
};

struct iav_srcbuf_format {
	u32 buf_id;
	struct iav_window size;
	struct iav_rect input;
};

struct iav_digital_zoom {
	u32 buf_id;
	struct iav_rect input;
};

struct iav_srcbuf_info {
	u32 buf_id;
	enum iav_srcbuf_state state;
};

enum iav_stream_state {
	IAV_STREAM_STATE_IDLE = 0,
	IAV_STREAM_STATE_STARTING,
	IAV_STREAM_STATE_ENCODING,
	IAV_STREAM_STATE_STOPPING,
	IAV_STREAM_STATE_UNKNOWN,
};

enum iav_stream_op {
	IAV_STREAM_OP_STOP = 0,
	IAV_STREAM_OP_START,
};

enum iav_chroma_format {
	H264_CHROMA_YUV420 = 0,
	H264_CHROMA_MONO = 1,

	JPEG_CHROMA_YUV422 = 0,
	JPEG_CHROMA_YUV420 = 1,
	JPEG_CHROMA_MONO = 2,
};

struct iav_stream_fps {
	u32 fps_multi;
	u32 fps_div;
	u32 abs_fps : 8;
	u32 abs_fps_enable : 1;
	u32 reserved : 23;
};

struct iav_stream_format {
	u32 id;
	enum iav_stream_type type;
	enum iav_srcbuf_id buf_id;
	struct iav_rect enc_win;
	u32 hflip : 1;
	u32 vflip : 1;
	u32 rotate_cw : 1;
	u32 duration : 16;
	u32 reserved : 13;
};

struct iav_h264_gop {
	u32 id;
	u32 N;
	u32 idr_interval;
};

struct iav_bitrate {
	u32 id;
	u32 average_bitrate;
	u32 vbr_setting : 8;
	u32 qp_min_on_I : 8;
	u32 qp_max_on_I : 8;
	u32 qp_min_on_P : 8;
	u32 qp_max_on_P : 8;
	u32 qp_min_on_B : 8;
	u32 qp_max_on_B : 8;
	u32 i_qp_reduce : 8;
	u32 p_qp_reduce : 8;
	u32 adapt_qp : 8;
	u32 skip_flag : 8;
	u32 log_q_num_plus_1 : 8;
	u32 max_i_size_KB;
	u32 qp_min_on_Q : 8;
	u32 qp_max_on_Q : 8;
	u32 q_qp_reduce : 8;
	u32 reserved1 : 8;
};

struct iav_rc_strategy {
	u32 abs_br_flag : 1;
	u32 reserved : 31;
};

typedef enum {
	QP_FRAME_I = 0,
	QP_FRAME_P = 1,
	QP_FRAME_B = 2,
	QP_FRAME_TYPE_NUM = 3,
} QP_FRAME_TYPES;

struct iav_qproi_data {
	u8 qp_quality;
	s8 qp_offset;
	u8 zmv_threshold;
	u8 reserved;
};

struct iav_qpmatrix {
	u32 id;
	u32 enable : 1;				/* whether qp matrix is enabled */
	u32 qpm_no_update : 1;		/* no qp matrix update for get/set op */
	u32 qpm_no_check : 1;		/* no qp matrix check for set op */
	u32 reserved : 29;
	char qp_delta[QP_FRAME_TYPE_NUM][4];
	u32 size;
	u32 data_offset;			/* qp matrix data offset */
};

struct iav_h264_enc_param {
	u32 id;
	u16 intrabias_p;
	u16 intrabias_b;
	u8 user1_intra_bias;
	u8 user1_direct_bias;
	u8 user2_intra_bias;
	u8 user2_direct_bias;
	u8 user3_intra_bias;
	u8 user3_direct_bias;
	u16 reserved;
};

struct iav_h264_pskip {
	u32 repeat_enable : 1;
	u32 reserved1 : 15;
	u32 repeat_num : 8;
	u32 reserved2 : 8;
};

enum iav_streamcfg_id {
	/* Stream config for H264 & H265 & MJPEG (0x0000 ~ 0x0FFF) */
	IAV_STMCFG_FPS = 0x0000,
	IAV_STMCFG_OFFSET = 0x0001,
	IAV_STMCFG_CHROMA = 0x0002,
	IAV_STMCFG_NUM,
	IAV_STMCFG_FIRST = IAV_STMCFG_FPS,
	IAV_STMCFG_LAST = IAV_STMCFG_NUM,

	/* H264 config (0x1000 ~ 0x1FFF) */
	IAV_H264_CFG_GOP = 0x1000,
	IAV_H264_CFG_BITRATE = 0x1001,
	IAV_H264_CFG_FORCE_IDR = 0x1002,
	IAV_H264_CFG_QP_LIMIT = 0x1003,
	IAV_H264_CFG_ENC_PARAM = 0x1004,
	IAV_H264_CFG_QP_ROI = 0x1005,
	IAV_H264_CFG_ZMV_THRESHOLD = 0x1006,
	IAV_H264_CFG_FLAT_AREA_IMPROVE = 0x1007,
	IAV_H264_CFG_FORCE_FAST_SEEK = 0x1008,
	IAV_H264_CFG_FRAME_DROP = 0x1009,
	IAV_H264_CFG_STATIS = 0x100A,
	IAV_H264_CFG_LONG_REF_P = 0x100B,
	IAV_H264_CFG_RC_STRATEGY = 0x100C,
	IAV_H264_CFG_FORCE_PSKIP = 0x100D,
	IAV_H264_CFG_NUM,
	IAV_H264_CFG_FIRST = IAV_H264_CFG_GOP,
	IAV_H264_CFG_LAST = IAV_H264_CFG_NUM,

	/* MJPEG config (0x2000 ~ 0x2FFF) */
	IAV_MJPEG_CFG_QUALITY = 0x2000,
	IAV_MJPEG_CFG_NUM,
	IAV_MJPEG_CFG_FIRST = IAV_MJPEG_CFG_QUALITY,
	IAV_MJPEG_CFG_LAST = IAV_MJPEG_CFG_NUM,
};

struct iav_stream_cfg {
	u32 id;
	enum iav_streamcfg_id cid;
	union {
		struct iav_stream_fps fps;
		struct iav_offset enc_offset;
		enum iav_chroma_format chroma;
		u32 mv_threshold;

		struct iav_h264_gop h264_gop;
		struct iav_bitrate h264_rc;
		struct iav_rc_strategy h264_rc_strategy;
		struct iav_qpmatrix h264_roi;
		struct iav_h264_enc_param h264_enc;
		struct iav_h264_pskip h264_pskip;
		int h264_force_idr;
		u32 h264_flat_area_improve;
		u32 h264_force_fast_seek;
		u32 h264_drop_frames;
		u32 h264_statis;

		u32 mjpeg_quality;
	} arg;
	u32 dsp_pts;
};

struct iav_stream_fps_sync {
	int enable[IAV_STREAM_MAX_NUM_ALL];
	struct iav_stream_fps fps[IAV_STREAM_MAX_NUM_ALL];
};

struct iav_stream_info {
	u32 id;
	enum iav_stream_state state;
};

enum iav_h264_profile {
	H264_PROFILE_BASELINE = 0,
	H264_PROFILE_MAIN = 1,
	H264_PROFILE_HIGH = 2,
	H264_PROFILE_NUM,
	H264_PROFILE_FIRST = 0,
	H264_PROFILE_LAST = H264_PROFILE_NUM,
};

enum iav_h264_au_type {
	H264_NO_AUD_NO_SEI = 0,
	H264_AUD_BEFORE_SPS_WITH_SEI = 1,
	H264_AUD_AFTER_SPS_WITH_SEI = 2,
	H264_NO_AUD_WITH_SEI = 3,
	H264_AU_TYPE_NUM,
};

struct iav_pic_info {
	u32	rate;
	u32	scale;
	u16	width;
	u16	height;
};

struct iav_h264_cfg {
	u32 id;
	u32 gop_structure : 8;
	u32 M : 8;
	u32 N : 16;
	u32 idr_interval : 8;
	u32 profile : 8;
	u32 au_type : 8;
	u32 chroma_format : 8;
	u32 cpb_buf_idc : 8;
	u32 reserved0 : 24;
	u32 cpb_user_size;
	u32 en_panic_rc : 2;
	u32 cpb_cmp_idc : 2;
	u32 fast_rc_idc : 4;
	u32 mv_threshold : 8;
	u32 flat_area_improve : 1;
	u32 multi_ref_p : 1;
	u32 reserved1 : 6;
	u32 fast_seek_intvl : 8;
	u32 intrabias_p : 16;
	u32 intrabias_b : 16;
	u8 user1_intra_bias;
	u8 user1_direct_bias;
	u8 user2_intra_bias;
	u8 user2_direct_bias;
	u8 user3_intra_bias;
	u8 user3_direct_bias;
	s8 deblocking_filter_alpha;
	s8 deblocking_filter_beta;
	u8 deblocking_filter_enable;
	u8 reserved2[3];
	struct iav_pic_info pic_info;
	u16 frame_crop_left_offset;
	u16 frame_crop_right_offset;
	u16 frame_crop_top_offset;
	u16 frame_crop_bottom_offset;
};

/* GOP model */
enum {
	/* simpe GOP, MPEG2 alike */
	IAV_GOP_SIMPLE = 0,
	/* hierachical GOP */
	IAV_GOP_ADVANCED = 1,
	/* 2 level SVCT */
	IAV_GOP_SVCT_2 = 2,
	/* 3 level SVCT */
	IAV_GOP_SVCT_3 = 3,
	/* B has 3 reference, P has 2 reference, advanced mode */
	IAV_GOP_P2B3_ADV = 4,
	/* P refers from I, may drop P in GOP. enable fast reverse play */
	IAV_GOP_NON_REF_P = 6,
	/* 2 level hierarchical P GOP, Ref and Non-Ref frames are interleaved */
	IAV_GOP_HI_P = 7,
	/* Long reference P GOP */
	IAV_GOP_LT_REF_P = 8,
};

enum iav_bitrate_control_params {
	/* QP limit parameters */
	H264_AQP_MAX = 4,
	H264_QP_MAX = 51,
	H264_QP_MIN = 0,
	H264_I_QP_REDUCE_MAX = 10,
	H264_I_QP_REDUCE_MIN = 1,
	H264_P_QP_REDUCE_MAX = 5,
	H264_P_QP_REDUCE_MIN = 1,
	H264_Q_QP_REDUCE_MAX = 10,
	H264_Q_QP_REDUCE_MIN = 1,
	/* skip frame flag */
	H264_WITHOUT_FRAME_DROP = 0,
	H264_WITH_FRAME_DROP = 6,
	H264_I_SIZE_KB_MAX = 8192,
	H264_LOG_Q_NUM_PLUS_1_MAX = 4,
};

enum {
	/* bitrate control */
	IAV_BRC_CBR = 1,
	IAV_BRC_PCBR = 2,
	IAV_BRC_VBR = 3,
	IAV_BRC_SCBR = 4,	/* smart CBR */
};

struct iav_statiscfg {
	u32 id;
	u32 enable;
	u32 mvdump_div;
};

struct iav_mjpeg_cfg {
	u32 id;
	u32 chroma_format : 8;
	u32 quality : 8;		/* 1 ~ 100 ,  100 is best quality */
	u32 reserved1 : 16;
};

enum {
	RAW_ENC_SRC_IMG = 0,	/* raw encode from image memory */
	RAW_ENC_SRC_USR = 1,	/* raw encode from user memory */
};

struct iav_raw_enc_setup {
	u32 raw_daddr_offset;
	u32 raw_frame_size;
	u32 pitch : 16;
	u32 raw_frame_num : 8;
	u32 raw_enc_src : 1;
	u32 reserved : 7;
};

struct iav_overlay_area {
	u16 enable;
	u16 width;
	u16 pitch;
	u16 height;
	u32 total_size;
	u16 start_x;
	u16 start_y;
	u32 clut_addr_offset;
	u32 data_addr_offset;
};

#define MAX_NUM_OVERLAY_AREA	(4)
struct iav_overlay_insert {
	u32 id;
	u32 enable: 1;
	u32 osd_insert_always: 1;
	u32 reserved : 30;
	struct iav_overlay_area area[MAX_NUM_OVERLAY_AREA];
};

#define MAX_NUM_FASTOSD_STRING_AREA (2)
#define MAX_NUM_FASTOSD_OSD_AREA (2)

struct iav_fastosd_string_area {
	u8 enable;
	u8 string_length;
	u16 string_output_pitch;
	u16 offset_x;
	u16 offset_y;
	u32 overall_string_width; //if each char's width is const, overall_string_width = num_of_char_in_string * width_of_char_font
	u32 string_output_offset;
	u32 string_offset;
	u32 clut_offset;
};

struct iav_fastosd_osd_area {
	u8 enable;
	u8 reserved0;
	u16 osd_buf_pitch;
	u16 offset_x;
	u16 offset_y;
	u16 win_width;
	u16 win_height;
	u32 buffer_offset;
	u32 clut_offset;
};

struct iav_fastosd_insert {
	u8 enable;
	u8 id;
	u8 string_num_region;
	u8 osd_num_region;

	u32 font_index_offset;
	u32 font_map_offset;
	u16 font_map_pitch;
	u16 font_map_height;

	struct iav_fastosd_string_area string_area[MAX_NUM_FASTOSD_STRING_AREA];
	struct iav_fastosd_osd_area osd_area[MAX_NUM_FASTOSD_OSD_AREA];
};

enum iav_privacy_mask_unit {
	IAV_PM_UNIT_PIXEL = 0,		/* 1 bit per pixel, VIN must be 32 pixels aligned. */
	IAV_PM_UNIT_MB = 1,			/* 4 bytes per MB, VIN must be 16 pixels aligned. */
};

enum iav_privacy_mask_domain {
	IAV_PM_DOMAIN_MAIN = 0,
	IAV_PM_DOMAIN_VIN = 1,
	IAV_PM_DOMAIN_PREMAIN_INPUT = 2,
};

struct iav_privacy_mask_info {
	u16 unit;
	u16 domain;
	u16 buffer_pitch;
	u16 pixel_width;		/* only used in pixel mask unit */
};

struct iav_privacy_mask {
	u8 enable;	/* 0: disable, 1: enable */
	u8 y;
	u8 u;
	u8 v;
	u16 buf_pitch;
	u16 buf_height;
	u32 data_addr_offset;
};

struct iav_badpixel_correction {
	u8 enable;	/* 0: disable, 1: enable */
};

struct iav_digital_zoom_privacy_mask {
	struct iav_digital_zoom zoom;
	struct iav_privacy_mask privacy_mask;
};

#define	BPC_TABLE_SIZE		((3)*(128)*(2))

struct iav_bpc_setup {
	u32 cmd_code;
	u8 dynamic_bad_pixel_detection_mode;
	u8 dynamic_bad_pixel_correction_method;
	u16 correction_mode;
	u32 hot_pixel_thresh_addr;
	u32 dark_pixel_thresh_addr;
	u16 hot_shift0_4;
	u16 hot_shift5;
	u16 dark_shift0_4;
	u16 dark_shift5;
};

struct iav_bpc_fpn {
	u32 cmd_code;
	u32 fpn_pixel_mode;
	u32 row_gain_enable;
	u32 column_gain_enable;
	u32 num_of_rows;
	u16 num_of_cols;
	u16 fpn_pitch;
	u32 fpn_pixels_addr;
	u32 fpn_pixels_buf_size;
	u32 intercept_shift;
	u32 intercepts_and_slopes_addr;
	u32 row_gain_addr;
	u32 column_gain_addr;
	u8  bad_pixel_a5m_mode;
};

enum iav_warp_params {
	MAX_NUM_WARP_AREAS = 8,
	MAX_GRID_WIDTH = 32,
	MAX_GRID_HEIGHT = 48,
	MAX_WARP_TABLE_SIZE = (MAX_GRID_WIDTH * MAX_GRID_HEIGHT),
	MAX_GRID_WIDTH_LDC = MAX_GRID_WIDTH * 2,
	MAX_WARP_TABLE_SIZE_LDC = (MAX_GRID_WIDTH_LDC * MAX_GRID_HEIGHT),

	GRID_SPACING_PIXEL_16 = 0,
	GRID_SPACING_PIXEL_32 = 1,
	GRID_SPACING_PIXEL_64 = 2,
	GRID_SPACING_PIXEL_128 = 3,
	GRID_SPACING_PIXEL_256 = 4,
	GRID_SPACING_PIXEL_512 = 5,

	WARP_AREA_H_OFFSET = 0,
	WARP_AREA_V_OFFSET = 1,
	WARP_AREA_ME1_V_OFFSET = 2,
	WARP_AREA_VECTOR_NUM,
};

struct iav_warp_map {
	u8 enable;
	u8 output_grid_col;
	u8 output_grid_row;
	u8 h_spacing : 4;
	u8 v_spacing : 4;
	u32 data_addr_offset;
};

struct iav_warp_area {
	u8 enable;
	u8 rotate_flip;
	u8 reserved[2];
	struct iav_rect input;
	struct iav_rect intermediate;
	struct iav_rect output;
	struct iav_warp_map h_map;
	struct iav_warp_map v_map;
	struct iav_warp_map me1_v_map;
};

struct iav_warp_main {
	u8 keep_dptz[IAV_SRCBUF_NUM];
	struct iav_warp_area area[MAX_NUM_WARP_AREAS];
};

struct iav_warp_dptz {
	u32 buf_id;
	struct iav_rect input[MAX_NUM_WARP_AREAS];
	struct iav_rect output[MAX_NUM_WARP_AREAS];
};

enum iav_warp_ctrl_id {
	IAV_WARP_CTRL_MAIN = 0,
	IAV_WARP_CTRL_DPTZ,
	IAV_WARP_CTRL_ENCOFS,
	IAV_WARP_CTRL_NUM,
	IAV_WARP_CTRL_FIRST = IAV_WARP_CTRL_MAIN,
	IAV_WARP_CTRL_LAST = IAV_WARP_CTRL_NUM,
};

struct iav_warp_ctrl {
	enum iav_warp_ctrl_id cid;
	union {
		struct iav_warp_main main;
		struct iav_warp_dptz dptz;
	} arg;
};

struct iav_mctf_filter {
	u32 mask_privacy : 1;
	u32 mask_overlay : 1;
	u32 reserved :30;
};

enum iav_video_proc_id {
	IAV_VIDEO_PROC_DPTZ = 0x00,
	IAV_VIDEO_PROC_PM = 0x01,
	IAV_VIDEO_PROC_CAWARP = 0x02,
	IAV_VIDEO_PROC_NUM,
	IAV_VIDEO_PROC_FIRST = IAV_VIDEO_PROC_DPTZ,
	IAV_VIDEO_PROC_LAST = IAV_VIDEO_PROC_NUM
};

enum iav_cawarp_params {
	CA_TABLE_COL_NUM = 32,
	CA_TABLE_ROW_NUM = 48,
	CA_TABLE_MAX_SIZE = (CA_TABLE_COL_NUM * CA_TABLE_ROW_NUM),

	CA_GRID_SPACING_16 = 0,
	CA_GRID_SPACING_32 = 1,
	CA_GRID_SPACING_64 = 2,
	CA_GRID_SPACING_128 = 3,
	CA_GRID_SPACING_256 = 4,
	CA_GRID_SPACING_512 = 5,
};

struct iav_ca_warp {
	struct iav_warp_map hor_map;
	struct iav_warp_map ver_map;
	u16 red_scale_factor;
	u16 blue_scale_factor;
};

struct iav_video_proc {
	int cid;
	union {
		struct iav_digital_zoom	dptz;
		struct iav_privacy_mask	pm;
		struct iav_ca_warp	cawarp;
	} arg;
};

struct iav_apply_flag {
	int apply;
	u32 param;
};

struct iav_efm_get_pool_info {
	u32 yuv_buf_num;
	u32 yuv_pitch;
	u32 me1_buf_num;
	u32 me1_pitch;
	struct iav_window yuv_size;
	struct iav_window me1_size;
};

struct iav_efm_request_frame {
	u32 frame_idx;
	u32 yuv_luma_offset;
	u32 yuv_chroma_offset;
	u32 me1_offset;
};

struct iav_efm_handshake_frame {
	u32 frame_idx;
	u32 frame_pts;

	u8 is_last_frame;
	u8 use_hw_pts;//only apply to real time's case
	u8 reserved1;
	u8 reserved2;
};

struct iav_apply_frame_sync {
	u32 dsp_pts;
	u32 force_update : 4;
	u32 reserved : 28;
};


enum iav_nalu_type {
	NT_NON_IDR = 1,
	NT_IDR = 5,
	NT_SEI = 6,
	NT_SPS = 7,
	NT_PPS = 8,
	NT_AUD = 9,
};

enum iav_debugcfg_module {
	IAV_DEBUG_IAV = (1 << 0),
	IAV_DEBUG_DSP = (1 << 1),
	IAV_DEBUG_VIN = (1 << 2),
	IAV_DEBUG_VOUT = (1 << 3),
	IAV_DEBUG_AAA = (1 << 4),
};

enum iav_debugcfg_id {
	IAV_DEBUGCFG_CHECK = 0x0000,
	IAV_DEBUGCFG_MODULE = 0x0001,
	IAV_DEBUGCFG_AUDIT = 0x0002,
	IAV_DEBUGCFG_NUM,
	IAV_DEBUGCFG_FIRST = IAV_DEBUGCFG_CHECK,
	IAV_DEBUGCFG_LAST = IAV_DEBUGCFG_NUM,
};

struct iav_debug_module {
	u8	enable;
	u8	reserved[3];
	u32	flags;
	u32	args[4];
};

struct iav_debug_audit {
	u32	id;
	u32	enable : 1;
	u32	reserved : 31;
	u64	cnt;
	u64	sum;
	u32	max;
	u32	min;
};

struct iav_debug_cfg {
	u32	id;
	enum iav_debugcfg_id cid;
	union {
		u32	enable;
		struct iav_debug_module module;
		struct iav_debug_audit audit;
	} arg;
};

#define DIAV_MAX_DECODER_NUMBER 1
#define DIAV_MAX_DECODE_VOUT_NUMBER 2

enum {
	IAV_DECODER_TYPE_INVALID = 0x00,

	IAV_DECODER_TYPE_H264 = 0x01,
	IAV_DECODER_TYPE_H265 = 0x02,

	IAV_DECODER_TYPE_MJPEG = 0x06,
};

enum {
	IAV_TRICK_PLAY_PAUSE = 0,
	IAV_TRICK_PLAY_RESUME = 1,
	IAV_TRICK_PLAY_STEP = 2,
};

enum {
	IAV_PB_DIRECTION_FW = 0,
	IAV_PB_DIRECTION_BW = 1,
};

enum {
	IAV_PB_SCAN_MODE_ALL_FRAMES = 0,
	IAV_PB_SCAN_MODE_I_ONLY = 1,
};

struct iav_decoder_config {
	u8	max_frm_num;
	u8	b_support_ff;
	u8	b_support_fb;
	u8	b_support_bw;

	u16	max_frm_width;
	u16	max_frm_height;

	u32	max_bitrate;
};

struct iav_decode_vout_config {
	u8	vout_id;
	u8	enable;
	u8	flip;
	u8	rotate;

	u16	target_win_offset_x;
	u16	target_win_offset_y;

	u16	target_win_width;
	u16	target_win_height;

	u32 zoom_factor_x;
	u32 zoom_factor_y;

	u32 vout_mode;
};

struct iav_decode_mode_config {
	u8	b_support_ff_fb_bw;
	u8	debug_max_frame_per_interrupt;
	u8	debug_use_dproc;
	u8	num_decoder;

	//for resource allocation inside dsp
	u32	max_frm_width;
	u32	max_frm_height;

	u32	max_vout0_width;
	u32	max_vout0_height;

	u32	max_vout1_width;
	u32	max_vout1_height;
};

struct iav_decoder_info {
	u8	decoder_id;
	u8	decoder_type;
	u8	num_vout;
	u8	setup_done;

	u32	width;
	u32	height;

	struct iav_decode_vout_config vout_configs[DIAV_MAX_DECODE_VOUT_NUMBER];

	u32	bsb_start_offset;
	u32	bsb_size;
};

struct iav_decode_video {
	u8	decoder_id;
	u8	num_frames;
	u8	reserved1;
	u8	reserved2;

	u32	start_ptr_offset;
	u32	end_ptr_offset;

	u32	first_frame_display;
};

struct iav_decode_jpeg {
	u8	decoder_id;
	u8	reserved0;
	u8	reserved1;
	u8	reserved2;

	u32	start_offset;
	u32	size;
};

struct iav_decode_bsb {
	u8	decoder_id;
	u8	reserved0;
	u8	reserved1;
	u8	reserved2;

	u32		start_offset;
	u32		room; // minus 256 byte for avoid read_offset == write_offset

	//read only
	u32		dsp_read_offset;
	u32		free_room;
};

struct iav_decode_stop {
	u8	decoder_id;
	u8	stop_flag;
	u8	reserved1;
	u8	reserved2;
};

struct iav_decode_trick_play {
	u8		decoder_id;
	u8		trick_play;
	u8		reserved0;
	u8		reserved1;
};

struct iav_decode_speed {
	u8		decoder_id;
	u8		reserved0;
	u8		reserved1;
	u8		reserved2;

	u8		direction; // 0: forward, 1: backward
	u8		scan_mode;
	u16		speed;
};

struct iav_decode_status {
	u8		decoder_id;
	u8		reserved0;
	u8		is_started;
	u8		is_send_stop_cmd;

	u32		last_pts;

	u32		decode_state;
	u32		error_status;
	u32		total_error_count;
	u32		decoded_pic_number;

	//bit stream buffer
	u32		write_offset;
	u32		room; // minus 256 byte for avoid read_offset == write_offset
	u32		dsp_read_offset;
	u32		free_room;

	//debug
	u32		irq_count;
	u32		yuv422_y_addr;
	u32		yuv422_uv_addr;
};

/*
 *	GENERAL   APIs
 */

#define IAVENCIOC_MAGIC			'V'
#define IAVENC_IO(nr)			_IO(IAVENCIOC_MAGIC, nr)
#define IAVENC_IOR(nr, size)		_IOR(IAVENCIOC_MAGIC, nr, size)
#define IAVENC_IOW(nr, size)		_IOW(IAVENCIOC_MAGIC, nr, size)
#define IAVENC_IOWR(nr, size)		_IOWR(IAVENCIOC_MAGIC, nr, size)

#define IAVDECIOC_MAGIC			'D'
#define IAVDEC_IO(nr)			_IO(IAVDECIOC_MAGIC, nr)
#define IAVDEC_IOR(nr, size)		_IOR(IAVDECIOC_MAGIC, nr, size)
#define IAVDEC_IOW(nr, size)		_IOW(IAVDECIOC_MAGIC, nr, size)
#define IAVDEC_IOWR(nr, size)		_IOWR(IAVDECIOC_MAGIC, nr, size)

typedef enum {
	/* For DSP & Driver (0x00 ~ 0x0F) */
	IOC_DRV_INFO = 0x00,
	IOC_DSP_LOG = 0x01,
	IOC_DSP_CFG = 0x02,
	IOC_DRV_DSP_INFO = 0x03,
	IOC_DSP_CLOCK = 0x04,
	IOC_GET_DSP_HASH = 0x05,

	/* For system (0x10 ~ 0x2F) */
	IOC_STATE = 0x10,
	IOC_ENTER_IDLE = 0x11,
	IOC_SET_PREVIEW = 0x12,
	IOC_ENCODE_START = 0x13,
	IOC_ENCODE_STOP = 0x14,
	IOC_SYSTEM_RESOURCE = 0x15,
	IOC_ENC_MODE_CAPABILITY = 0x16,
	IOC_SRC_BUF_CAPABILITY = 0x17,
	IOC_QUERY_BUF = 0x18,
	IOC_QUERY_DESC = 0x19,
	IOC_GDMA_COPY = 0x1A,
	IOC_QUERY_SUB_BUF = 0x1B,

	/* For encode control (0x30 ~ 0x5F) */
	IOC_SRC_BUF_SETUP = 0x30,
	IOC_SRC_BUF_FORMAT = 0x31,
	IOC_SRC_BUF_DPTZ = 0x32,
	IOC_SRC_BUF_INFO = 0x33,
	IOC_STREAM_FORMAT = 0x34,
	IOC_STREAM_INFO = 0x35,
	IOC_STREAM_CFG = 0x36,
	IOC_STREAM_FPS_SYNC = 0x37,
	IOC_H264_CFG = 0x38,
	IOC_MJPEG_CFG = 0x39,
	IOC_RAW_ENC = 0x3A,
	IOC_FRAME_SYNC_PROC = 0x3B,

	/* For other encode control (0x60 ~ 0x6F) */
	IOC_OVERLAY_INSERT = 0x60,
	IOC_PRIVACY_MASK = 0x61,
	IOC_PRIVACY_MASK_INFO = 0x62,
	IOC_WARP_CTRL = 0x63,
	IOC_BADPIXEL_CORRECTION = 0x64,
	IOC_FASTOSD_INSERT = 0x65,
	IOC_VIDEO_PROC = 0x66,
	IOC_EFM_PROC = 0x67,

	/* For Misc setting (0xD0 ~ 0xEF) */
	IOC_TEST = 0xD0,
	IOC_DEBUG = 0xD1,

	/* Reserved (0xF0 ~ 0xFF) */
	IOC_CUSTOM = 0xF0,
} IAV_ENC_IOC;

typedef enum {
	/* For decode control (0x00 ~ 0x1F) */
	IOC_ENTER_DECODE_MODE = 0x00,
	IOC_LEAVE_DECODE_MODE = 0x01,
	IOC_CREATE_DECODER = 0x02,
	IOC_DESTROY_DECODER = 0x03,
	IOC_DECODE_START = 0x04,
	IOC_DECODE_STOP = 0x05,
	IOC_DECODE_VIDEO = 0x06,
	IOC_DECODE_JPEG = 0x07,

	IOC_WAIT_DECODE_BSB = 0x08,

	IOC_DECODE_TRICK_PLAY = 0x10,
	IOC_DECODE_SPEED = 0x11,

	/* For decode status (0x30 ~ 0x3F) */
	IOC_QUERY_DECODE_STATUS = 0x30,
	IOC_QUERY_DECODE_BSB = 0x31,
} IAV_DEC_IOC;

/* DSP & driver ioctl */
#define IAV_IOC_GET_CHIP_ID		IAVENC_IOR(IOC_DRV_INFO, u8)
#define IAV_IOC_GET_DRIVER_INFO	IAVENC_IOR(IOC_DRV_INFO, u16)
#define IAV_IOC_SET_DSP_LOG			IAVENC_IOW(IOC_DSP_LOG, struct iav_dsplog_setup *)
#define IAV_IOC_DUMP_DSP_CFG			IAVENC_IOR(IOC_DSP_CFG)
#define IAV_IOC_DRV_DSP_INFO	IAVENC_IOWR(IOC_DRV_DSP_INFO, struct iav_driver_dsp_info *)
#define IAV_IOC_SET_DSP_CLOCK_STATE	IAVENC_IOW(IOC_DSP_CLOCK, u32)
#define IAV_IOC_GET_DSP_HASH	IAVENC_IOWR(IOC_GET_DSP_HASH, struct iav_dsp_hash *)

/* state ioctl */
#define IAV_IOC_WAIT_NEXT_FRAME		IAVENC_IOR(IOC_STATE, u8)
#define IAV_IOC_GET_IAV_STATE		IAVENC_IOR(IOC_STATE, u32)
#define IAV_IOC_ENTER_IDLE		IAVENC_IO(IOC_ENTER_IDLE)
#define IAV_IOC_ENABLE_PREVIEW	IAVENC_IOW(IOC_SET_PREVIEW, int)
#define IAV_IOC_START_ENCODE			IAVENC_IOW(IOC_ENCODE_START, u32)
#define IAV_IOC_STOP_ENCODE			IAVENC_IOW(IOC_ENCODE_STOP, u32)
#define IAV_IOC_ABORT_ENCODE		IAVENC_IOW(IOC_ENCODE_STOP, u8)

/* system ioctl */
#define IAV_IOC_SET_SYSTEM_RESOURCE	IAVENC_IOW(IOC_SYSTEM_RESOURCE, struct iav_system_resource *)
#define IAV_IOC_GET_SYSTEM_RESOURCE	IAVENC_IOWR(IOC_SYSTEM_RESOURCE, struct iav_system_resource *)
#define IAV_IOC_QUERY_ENC_MODE_CAP	IAVENC_IOWR(IOC_ENC_MODE_CAPABILITY, struct iav_enc_mode_cap *)
#define IAV_IOC_QUERY_ENC_BUF_CAP		IAVENC_IOWR(IOC_SRC_BUF_CAPABILITY, struct iav_enc_buf_cap *)

/* gdma copy */
#define IAV_IOC_GDMA_COPY			IAVENC_IOW(IOC_GDMA_COPY, struct iav_gdma_copy *)

/* read data ioctl */
#define IAV_IOC_QUERY_BUF			IAVENC_IOWR(IOC_QUERY_BUF, struct iav_querybuf *)
#define IAV_IOC_QUERY_DESC			IAVENC_IOWR(IOC_QUERY_DESC, struct iav_querydesc *)
#define IAV_IOC_QUERY_SUB_BUF		IAVENC_IOWR(IOC_QUERY_SUB_BUF, struct iav_query_subbuf *)

/* source buffer ioctl */
#define IAV_IOC_SET_SOURCE_BUFFER_SETUP	IAVENC_IOW(IOC_SRC_BUF_SETUP, struct iav_srcbuf_setup *)
#define IAV_IOC_GET_SOURCE_BUFFER_SETUP	IAVENC_IOWR(IOC_SRC_BUF_SETUP, struct iav_srcbuf_setup *)
#define IAV_IOC_SET_SOURCE_BUFFER_FORMAT	IAVENC_IOW(IOC_SRC_BUF_FORMAT, struct iav_srcbuf_format *)
#define IAV_IOC_GET_SOURCE_BUFFER_FORMAT	IAVENC_IOWR(IOC_SRC_BUF_FORMAT, struct iav_srcbuf_format *)
#define IAV_IOC_SET_DIGITAL_ZOOM		IAVENC_IOW(IOC_SRC_BUF_DPTZ, struct iav_digital_zoom *)
#define IAV_IOC_GET_DIGITAL_ZOOM		IAVENC_IOWR(IOC_SRC_BUF_DPTZ, struct iav_digital_zoom *)
#define IAV_IOC_GET_SOURCE_BUFFER_INFO		IAVENC_IOWR(IOC_SRC_BUF_INFO, struct iav_srcbuf_info *)

/* stream ioctl */
#define IAV_IOC_SET_STREAM_FORMAT	IAVENC_IOW(IOC_STREAM_FORMAT, struct iav_stream_format *)
#define IAV_IOC_GET_STREAM_FORMAT	IAVENC_IOWR(IOC_STREAM_FORMAT, struct iav_stream_format *)
#define IAV_IOC_GET_STREAM_INFO		IAVENC_IOWR(IOC_STREAM_INFO, struct iav_stream_info *)
#define IAV_IOC_SET_STREAM_CONFIG		IAVENC_IOW(IOC_STREAM_CFG, struct iav_stream_cfg *)
#define IAV_IOC_GET_STREAM_CONFIG		IAVENC_IOWR(IOC_STREAM_CFG, struct iav_stream_cfg *)
#define IAV_IOC_SET_STREAM_FPS_SYNC	IAVENC_IOW(IOC_STREAM_FPS_SYNC, struct iav_stream_fps_sync *)
#define IAV_IOC_SET_H264_CONFIG		IAVENC_IOW(IOC_H264_CFG, struct iav_h264_cfg *)
#define IAV_IOC_GET_H264_CONFIG		IAVENC_IOWR(IOC_H264_CFG, struct iav_h264_cfg *)
#define IAV_IOC_SET_MJPEG_CONFIG		IAVENC_IOW(IOC_MJPEG_CFG, struct iav_mjpeg_cfg *)
#define IAV_IOC_GET_MJPEG_CONFIG		IAVENC_IOWR(IOC_MJPEG_CFG, struct iav_mjpeg_cfg *)

/* frame sync ioctl */
#define IAV_IOC_CFG_FRAME_SYNC_PROC		IAVENC_IOW(IOC_FRAME_SYNC_PROC, struct iav_stream_cfg *)
#define IAV_IOC_GET_FRAME_SYNC_PROC		IAVENC_IOWR(IOC_FRAME_SYNC_PROC, struct iav_stream_cfg *)
#define IAV_IOC_APPLY_FRAME_SYNC_PROC	IAVENC_IOW(IOC_FRAME_SYNC_PROC, u8)

/* encode ioctl */
#define IAV_IOC_SET_RAW_ENCODE		IAVENC_IOW(IOC_RAW_ENC, struct iav_raw_enc_setup *)
#define IAV_IOC_GET_RAW_ENCODE		IAVENC_IOR(IOC_RAW_ENC, struct iav_raw_enc_setup *)
#define IAV_IOC_WAIT_RAW_ENCODE		IAVENC_IOW(IOC_RAW_ENC, u8)

/* overlay ioctl */
#define IAV_IOC_SET_OVERLAY_INSERT	IAVENC_IOW(IOC_OVERLAY_INSERT, struct iav_overlay_insert *)
#define IAV_IOC_GET_OVERLAY_INSERT	IAVENC_IOWR(IOC_OVERLAY_INSERT, struct iav_overlay_insert *)

/* fastosd ioctl */
#define IAV_IOC_SET_FASTOSD_INSERT	IAVENC_IOW(IOC_FASTOSD_INSERT, struct iav_fastosd_insert *)
#define IAV_IOC_GET_FASTOSD_INSERT	IAVENC_IOWR(IOC_FASTOSD_INSERT, struct iav_fastosd_insert *)

/* privacy mask ioctl */
#define IAV_IOC_GET_PRIVACY_MASK_INFO	IAVENC_IOR(IOC_PRIVACY_MASK_INFO, struct iav_privacy_mask_info *)
#define IAV_IOC_SET_PRIVACY_MASK		IAVENC_IOW(IOC_PRIVACY_MASK, struct iav_privacy_mask *)
#define IAV_IOC_GET_PRIVACY_MASK		IAVENC_IOR(IOC_PRIVACY_MASK, struct iav_privacy_mask *)
#define IAV_IOC_SET_DIGITAL_ZOOM_PRIVACY_MASK	IAVENC_IOW(IOC_PRIVACY_MASK, u8)

/* vproc control ioctl */
#define IAV_IOC_GET_VIDEO_PROC		IAVENC_IOWR(IOC_VIDEO_PROC, struct iav_video_proc *)
#define IAV_IOC_CFG_VIDEO_PROC		IAVENC_IOW(IOC_VIDEO_PROC, struct iav_video_proc *)
#define IAV_IOC_APPLY_VIDEO_PROC	IAVENC_IOW(IOC_VIDEO_PROC, u8)

/* encode from memory ioctl */
#define IAV_IOC_EFM_GET_POOL_INFO		IAVENC_IOR(IOC_EFM_PROC, struct iav_efm_get_pool_info *)
#define IAV_IOC_EFM_REQUEST_FRAME		IAVENC_IOWR(IOC_EFM_PROC, struct iav_efm_request_frame *)
#define IAV_IOC_EFM_HANDSHAKE_FRAME		IAVENC_IOW(IOC_EFM_PROC, struct iav_efm_handshake_frame *)

/* warp control ioctl */
#define IAV_IOC_CFG_WARP_CTRL		IAVENC_IOW(IOC_WARP_CTRL, struct iav_warp_ctrl *)
#define IAV_IOC_GET_WARP_CTRL		IAVENC_IOWR(IOC_WARP_CTRL, struct iav_warp_ctrl *)
#define IAV_IOC_APPLY_WARP_CTRL		IAVENC_IOW(IOC_WARP_CTRL, u8)

/* bad pixel correction ioctl */
#define IAV_IOC_CFG_BPC_SETUP		IAVENC_IOW(IOC_BADPIXEL_CORRECTION, u8)
#define IAV_IOC_APPLY_STATIC_BPC		IAVENC_IOW(IOC_BADPIXEL_CORRECTION, u16)

/* IAV test for debug */
#define IAV_IOC_SET_DEBUG_CONFIG			IAVENC_IOW(IOC_DEBUG, struct iav_debug_cfg *)
#define IAV_IOC_GET_DEBUG_CONFIG			IAVENC_IOR(IOC_DEBUG, struct iav_debug_cfg *)

/* Customized IAV IOCTL */
#define IAV_IOC_CUSTOM_CMDS		IAVENC_IOWR(IOC_CUSTOM, u8)

/* decode ioctl */
#define IAV_IOC_ENTER_DECODE_MODE    IAVDEC_IOWR(IOC_ENTER_DECODE_MODE, struct iav_decode_mode_config *)
#define IAV_IOC_LEAVE_DECODE_MODE    IAVDEC_IO(IOC_LEAVE_DECODE_MODE)

#define IAV_IOC_CREATE_DECODER    IAVDEC_IOWR(IOC_CREATE_DECODER, struct iav_decoder_info *)
#define IAV_IOC_DESTROY_DECODER    IAVDEC_IOW(IOC_DESTROY_DECODER, u8)

#define IAV_IOC_DECODE_START    IAVDEC_IOW(IOC_DECODE_START, u8)
#define IAV_IOC_DECODE_STOP    IAVDEC_IOW(IOC_DECODE_STOP, struct iav_decode_stop *)

#define IAV_IOC_DECODE_VIDEO    IAVDEC_IOW(IOC_DECODE_VIDEO, struct iav_decode_video *)
#define IAV_IOC_DECODE_JPEG    IAVDEC_IOW(IOC_DECODE_JPEG, struct iav_decode_jpeg *)

#define IAV_IOC_WAIT_DECODE_BSB    IAVDEC_IOWR(IOC_WAIT_DECODE_BSB, struct iav_decode_bsb *)

#define IAV_IOC_DECODE_TRICK_PLAY    IAVDEC_IOW(IOC_DECODE_TRICK_PLAY, struct iav_decode_trick_play *)
#define IAV_IOC_DECODE_SPEED    IAVDEC_IOW(IOC_DECODE_SPEED, struct iav_decode_speed *)

#define IAV_IOC_QUERY_DECODE_STATUS    IAVDEC_IOR(IOC_QUERY_DECODE_STATUS, struct iav_decode_status *)
#define IAV_IOC_QUERY_DECODE_BSB    IAVDEC_IOR(IOC_QUERY_DECODE_BSB, struct iav_decode_bsb *)

#endif

