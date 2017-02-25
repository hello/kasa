/*
 * kernel/private/drivers/iav/arch_s2/iav.h
 *
 * History:
 *    2013/03/07 - [Cao Rongrong] Create
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

#ifndef __IAV_H__
#define __IAV_H__

#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/of.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <mach/hardware.h>
#include <iav_ioctl.h>
#include <iav_config.h>
#include <iav_netlink.h>
#include "amba_arch_mem.h"
#include "dsp_cmd_msg.h"

#if (CHIP_REV == S2 ||CHIP_REV == S2L)
#define VIN_INSTANCES			2
#else
#define VIN_INSTANCES			1
#endif

struct iav_nl_request
{
	u32 request_id;
	u32 responded;
	wait_queue_head_t wq_request;
};

struct iav_nl_obj
{
	s32 nl_user_pid;
	u32 nl_connected;
	s32 nl_init;
	s32 nl_port;
	s32 nl_session_count;
	s32 nl_request_count;
	struct sock *nl_sock;
	struct ambarella_iav *iav;
	struct iav_nl_request nl_requests[MAX_NETLINK_REQUESTS];
};

#define MAX_BUFCAP_DATA_NUM		(4)
#define ENCODE_DURATION_FOREVER	(0xFFFFFFFF)

struct iav_state_info {
	int	state;
	int	dsp_op_mode;
	int	dsp_encode_state;
	int	dsp_encode_mode;
	int	dsp_decode_state;
	int	decode_state;
	int	encode_timecode;
	int	encode_pts;
};

/* the limitation is different from each dsp encode mode. */
struct iav_enc_limitation
{
	/* the max/min size of main buffer */
	struct iav_window max_main;
	struct iav_window min_main;
	/* the min size of encode window */
	struct iav_window min_enc;
	u32 max_enc_num : 8;
	u32 max_chroma_radius : 2;
	u32 max_wide_chroma_radius : 2;
	u32 reserved0 : 20;
	u32 max_enc_MB;
	/* maximum capture pixel per second */
	//u32 max_pps;
	u32 rotate_supported : 1;
	u32 rawcap_supported : 1;
	u32 vout_swap_supported : 1;
	u32 lens_warp_supported : 1;
	u32 iso_supported : 3;
	u32 enc_raw_rgb_supported : 1;
	u32 high_mp_vin_supported : 1;
	u32 linear_supported : 1;
	u32 hdr_2x_supported : 2;
	u32 hdr_3x_supported : 2;
	u32 mixer_b_supported : 1;
	u32 pm_bpc_supported : 1;
	u32 pm_mctf_supported : 1;
	u32 wcr_supported : 1;
	u32 idsp_upsample_supported : 1;
	u32 me0_supported : 2;
	u32 enc_from_mem_supported : 1;
	u32 enc_raw_yuv_supported : 1;
	u32 long_ref_b_supported : 1;
	u32 reserved : 8;
};

/* the limitation for each chip ID */
struct iav_system_load
{
	#define	DESC_LENGTH		(32)
	u32	system_load;
	u32	vin_pps;
	char	desc[DESC_LENGTH];
	u8	max_enc_num;
	u8	reserved[3];
	char	vin_pps_desc[DESC_LENGTH];
};

/* the limitation of each source buffer. */
struct iav_buffer_limitation
{
	/* the max size of the buffer, it's hardware limitation */
	struct iav_window max_win;
	u32 max_zoomin_factor;
	u32 max_zoomout_factor;
};

struct iav_buffer {
	enum iav_srcbuf_id id;
	u32 id_dsp;	/* recognized by DSP cmd */
	enum iav_srcbuf_type type;
	enum iav_srcbuf_state state;
	struct iav_rect input;
	struct iav_window win;
	struct iav_window max;
	u32 ref_cnt;
	int extra_dram_buf;
	u32 dump_interval : 8;
	u32 dump_duration : 16;
	u32 unwarp : 1;
	u32 reserved : 7;
};

struct iav_h264_config {
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
	u32 en_panic_rc :2;
	u32 cpb_cmp_idc :2;
	u32 fast_rc_idc :4;
	u32 zmv_threshold : 8;
	u32 flat_area_improve : 1;
	u32 multi_ref_p : 1;
	u32 statis : 1;
	u32 pskip_repeat_enable : 1;
	u32 reserved1 : 4;
	u32 fast_seek_intvl : 8;

	u32 average_bitrate;
	u32 vbr_setting : 8;
	u32 qp_min_on_I : 8;
	u32 qp_max_on_I : 8;
	u32 qp_min_on_P : 8;
	u32 qp_max_on_P : 8;
	u32 qp_min_on_B : 8;
	u32 qp_max_on_B : 8;
	u32 qp_min_on_Q : 8;
	u32 qp_max_on_Q : 8;
	u32 i_qp_reduce : 8;
	u32 p_qp_reduce : 8;
	u32 q_qp_reduce : 8;
	u32 log_q_num_plus_1 : 8;
	u32 adapt_qp : 8;
	u32 skip_flag : 8;
	u32 repeat_pskip_num : 8;
	u32 intrabias_p : 16;
	u32 intrabias_b : 16;

	s8 qp_delta[3][4];
	u8 qp_roi_enable;
	u8 drop_frames;
	u8 user1_intra_bias;
	u8 user1_direct_bias;
	u8 user2_intra_bias;
	u8 user2_direct_bias;
	u8 user3_intra_bias;
	u8 user3_direct_bias;
	s8 deblocking_filter_alpha; 	/* set alpha for loop filter between -6 and 6 */
	s8 deblocking_filter_beta; 		/* set beta for loop filter between -6 and 6 */
	/* 0- disable deblocking filter
	 * 1- enable deblocking filter, use alpha & beta values specified above
	 * 2- use internally computed alpha and beta.
	 */
	u8 deblocking_filter_enable;
	u8 abs_br_flag : 1;
	u8 reserved : 7;
	u16 frame_crop_left_offset;
	u16 frame_crop_right_offset;
	u16 frame_crop_top_offset;
	u16 frame_crop_bottom_offset;
	u32 max_I_size_KB;
};

struct iav_mjpeg_config {
	u32 chroma_format : 8;
	u32 quality : 8;		/* 1 ~ 100 ,  100 is best quality */
	u32 reserved1 : 16;
	void *jpeg_quant_matrix;
};

struct iav_frame_desc {
	struct iav_framedesc desc;
	struct list_head node;
};

struct iav_stream {
	struct ambarella_iav *iav;
	wait_queue_head_t venc_wq;

	u32 id_dsp;	/* recognized by DSP cmd */
	u32 session_id;

	encode_state_t fake_dsp_state;  /* Venc interrupt send fake state to Vdsp interrupt */
	encode_state_t dsp_state;
	enum iav_stream_op op;

	u32 max_GOP_M;
	u32 max_GOP_N;
	u32 long_ref_enable;

	int prev_frmNo;

	struct iav_buffer *srcbuf;
	struct iav_window max;

	struct iav_stream_fps fps;
	struct iav_stream_format format;
	struct iav_h264_config h264_config;
	struct iav_mjpeg_config mjpeg_config;

	struct iav_overlay_insert osd;
	struct iav_fastosd_insert fastosd;
	struct mutex osd_mutex;
	struct iav_frame_desc snapshot;
};

struct iav_raw_data_info {
	u32 raw_addr;
	u32 raw_width;
	u32 raw_height;
	u32 pitch;
};

struct iav_qp_hist_info
{
	u32 data_dsp_addr_base;
	u32 data_virt_addr_base;
	u32 base_addr_set_flag;
	u32 data_dsp_addr_end;
	u32 data_dsp_addr_cur;
	u32 seq_num;
	u32 stream_num;
};

struct iav_srcbuf_data_info {
	u32 y_data_addr;
	u32 uv_data_addr;
	u32 pitch;
	u32 seq_num;
	u32 dsp_pts;
	u32 reserved;
	u64 mono_pts;
};

struct iav_me_data_info {
	u32 data_addr;
	u32 pitch;
	u32 seq_num;
	u32 dsp_pts;
	u64 mono_pts;
};

struct iav_bufcap_info {
	u16 write_index;
	u16 read_index;
	u32 seq_num;
	u64 mono_pts;			// capture monotonic pts
	u64 sec2_out_mono_pts;	// section out monotonic pts for dewarp mode

	struct iav_raw_data_info raw_info[MAX_BUFCAP_DATA_NUM];
	wait_queue_head_t raw_wq;
	u32 raw_data_valid;

	struct iav_srcbuf_data_info srcbuf_info[IAV_SRCBUF_NUM][MAX_BUFCAP_DATA_NUM];
	wait_queue_head_t srcbuf_wq;
	u32 srcbuf_data_valid;

	struct iav_me_data_info me1_info[IAV_SRCBUF_NUM][MAX_BUFCAP_DATA_NUM];
	wait_queue_head_t me1_wq;
	u32 me1_data_valid;

	struct iav_me_data_info me0_info[IAV_SRCBUF_NUM][MAX_BUFCAP_DATA_NUM];
	wait_queue_head_t me0_wq;
	u32 me0_data_valid;

	/* QP histogram statistics */
	struct iav_qp_hist_info qp_hist;
	wait_queue_head_t qp_hist_wq;
	u32 qp_hist_data_valid;
};

struct iav_efm_buf_pool {
	u8 req_num;									// request num at a time
	u8 req_msg_num;								// request message num at a time
	u8 req_idx;									// request index
	u8 hs_idx;									// handshake index
	u32 yuv_luma_addr[IAV_EFM_MAX_BUF_NUM];		// YUV luma data address
	u32 yuv_chroma_addr[IAV_EFM_MAX_BUF_NUM];	// YUV chroma data address
	u32 me1_addr[IAV_EFM_MAX_BUF_NUM];			// ME1 data address
	u16 yuv_fId[IAV_EFM_MAX_BUF_NUM];			// YUV buffer index in DSP
	u16 me1_fId[IAV_EFM_MAX_BUF_NUM];			// ME1 buffer index in DSP
	u16 requested[IAV_EFM_MAX_BUF_NUM];			// request frame OK
};

struct iav_efm_info {
	u8 req_buf_num;
	u8 yuv_buf_num;
	u8 me1_buf_num;
	u8 state;
	u16 yuv_pitch;
	u16 me1_pitch;
	u32 curr_pts;
	u32 valid_num;
	struct iav_window efm_size;
	struct iav_efm_buf_pool buf_pool;
	wait_queue_head_t wq;
};

struct iav_mem_block {
	unsigned long phys;
	unsigned long virt;
	u32 size;
};

struct iav_sub_mem_block {
	unsigned long phys;
	unsigned long virt;
	unsigned long offset;
	u32 size;
};

struct iav_gdma_param {
	u32 dest_phys_addr;
	u32 dest_virt_addr;
	u32 src_phys_addr;
	u32 src_virt_addr;
	u8 dest_non_cached;
	u8 src_non_cached;
	u8 reserved[2];
	u16 src_pitch;
	u16 dest_pitch;
	u16 width;
	u16 height;
};

struct iav_raw_encode {
	u32 raw_daddr_offset;
	u32 raw_frame_size;
	u32 pitch : 16;
	u32 raw_frame_num : 8;
	u32 raw_curr_num : 8;
	u32 raw_enc_src : 1;
	u32 cmd_send_flag : 1;
	u32 reserved : 30;
	u32 raw_curr_addr;
	struct iav_window raw_size;
	wait_queue_head_t raw_wq;
};

struct iav_mv_dump {
	u16 enable : 1;
	u16 buf_num : 15;
	u16 width;
	u16 height;
	u16 pitch;
	u32 unit_size;
};

struct iav_cmd_sync {
	u8 idx;
	u8 qpm_idx;
	u8 qpm_flag;
	u8 reserved;
	u32 apply_pts[IAV_STREAM_MAX_NUM_ALL];
};

struct iav_sync_frame_cnt {
	u32 curr_h264_cnt;
	u32 curr_mjpeg_cnt;

	u32 total_h264_cnt;
	u32 total_mjpeg_cnt;
	u32 total_frame_cnt;

	u32 sync_delta;
	u32 free_frame_cnt;
};

struct iav_system_config {
	u8	expo_num;			/* number of exposure */
	u8	max_stream_num;		/* tell DSP how many streams in most possible. */
	u8	max_enc_buf_num;	/* max number of encode buffers */
	u8	reserved1;
	u32	max_warp_input_width;
	u32	max_warp_input_height;
	u32	max_warp_output_width;
	u32	max_padding_width;
	u32	v_warped_main_max_width;
	u32	v_warped_main_max_height;
	u32	rotatable : 1;		/* set false to save resource */
	u32	sharpen_b : 1;		/* 0: disabled; 1: enabled luma sharpen */
	u32	raw_capture: 1;		/* 0: disable (compression); 1: enable raw capture */
	u32	lens_warp : 1;		/* 0: disable; 1: enable */
	u32	vout_swap : 1;		/* 0: disable; 1: enable */
	u32	iso_type : 3;		/* 0: Low ISO; 1: Middle ISO; 2: Advanced ISO; 3: Blend ISO; 4: High ISO */
	u32	enc_raw_rgb : 1;	/* 0: disable; 1: enable */
	u32	enc_dummy_latency : 8;
	u32	mctf_pm : 1;		/* 0: disable; 1: enable */
	u32	reserved2 : 14;
	u32	debug_enable_map;
	u32	debug_stitched : 1;
	u32	debug_iso_type : 3;
	u32	debug_chip_id : 5;
	u32	idsp_upsample_type : 2;
	u32	me0_scale : 2;
	u32	enc_from_mem : 1;
	u32	enc_raw_yuv : 1;
	u32	eis_delay_count : 2;
	u32	long_ref_b_frame : 1;
	u32	extra_top_row_buf_enable : 1;
	u32	reserved3 : 13;
};

struct iav_hwtimer_pts_info
{
	u32 audio_tick;
	u32	audio_freq;
	u64	hwtimer_tick;
	u32	hwtimer_freq;
	struct file	*fp_timer;
};

struct iav_pts_info
{
	struct iav_hwtimer_pts_info hw_pts_info;
	u32 hwtimer_enabled;
	u32	reserved;
};

struct iav_system_freq
{
	u32	cortex_MHz;
	u32	idsp_MHz;
	u32	core_MHz;
	u32	dram_MHz;
};

struct iav_coeff
{
	u32 multi;
	u32 div;
};

struct iav_dram_coeff
{
	struct iav_coeff vin[2];
	struct iav_coeff dummy[2];
	struct iav_coeff preblend;
	struct iav_coeff lpf;
	struct iav_coeff vout[2];
	struct iav_coeff buf_yuv[IAV_SRCBUF_NUM];
	struct iav_coeff buf_me1[IAV_SRCBUF_NUM];
	struct iav_coeff buf_me0[IAV_SRCBUF_NUM];
	struct iav_coeff strm_yuv[IAV_STREAM_MAX_NUM_ALL];
	struct iav_coeff strm_me1[IAV_STREAM_MAX_NUM_ALL];
};

struct iav_dram_buf_num
{
	int vin;
	int dummy;
	int preblend;
	int lpf;
	int vout[2];
	int buf_yuv[IAV_SRCBUF_NUM];
	int buf_me1[IAV_SRCBUF_NUM];
	int buf_me0[IAV_SRCBUF_NUM];
	int strm_yuv[IAV_STREAM_MAX_NUM_ALL];
	int strm_col[IAV_STREAM_MAX_NUM_ALL];
};

struct iav_decoder_current_status {
	u32 current_bsb_addr_dsp_base;
	u32 current_bsb_addr_phy_base;
	u32 current_bsb_addr_virt_base;
	u32 current_bsb_addr_virt_end;
	u32 current_bsb_size;

	u32 current_bsb_write_offset;
	u32 current_bsb_read_offset;
	u32 current_bsb_remain_free_size;

	int num_waiters;
	struct semaphore sem;

	u16 speed;
	u8 scan_mode;
	u8 direction;

	u8 accumulated_dec_cmd_number;
	u8 current_trickplay;
	u8 is_started : 1;
	u8 b_send_first_decode_cmd : 1;
	u8 b_send_stop_cmd : 1;
	u8 reserved0 : 5;
	u8 reserved1;

	u32 last_pts;

	u32 decode_state;
	u32 error_status;
	u32 total_error_count;
	u32 decoded_pic_number;

	u32 irq_count;
	u32 yuv422_y_addr;
	u32 yuv422_uv_addr;
};

struct iav_decode_context {
	u8 b_interlace_vout;
	u8 reserved0;
	u8 reserved1;
	u8 reserved2;

	struct iav_decode_mode_config mode_config;
	struct iav_decoder_info decoder_config[DIAV_MAX_DECODER_NUMBER];

	struct iav_decoder_current_status decoder_current_status[DIAV_MAX_DECODER_NUMBER];
};

struct ambarella_iav {
	struct cdev iav_cdev;
	struct device_node *of_node;
	struct dsp_device *dsp;
	struct mutex iav_mutex;
	struct mutex enc_mutex;
	spinlock_t iav_lock;

	u32 dsp_enc_state;		/* DSP encode state */
	u32 state;				/* current IAV state */
	u32 probe_state;		/* IAV state in module probe param */
	u32 encode_mode;		/* current encode mode */
	u32 cmd_read_delay;		/* cmd read latency in audio clock unit */
	u16 resume_flag;		/* resume from suspend */
	u16 fast_resume;		/* fast resume from suspend */

	struct iav_decode_context decode_context;

	struct iav_system_config *system_config;

	/* MMAP for memory partitions */
	struct iav_mem_block mmap[IAV_BUFFER_NUM];
	struct iav_sub_mem_block mmap_dsp[IAV_DSP_SUB_BUF_NUM];
	u32 mmap_base;
	u32 mmap_dsp_base;
	u32 dsp_partition_mapped;
	u32 dsp_partition_to_map;
	u32 dsp_map_updated;
	wait_queue_head_t dsp_map_wq;

	/* These 5 buffers are allocated and handled by DSP, but
	 * we need to provide enough information such as geometry. */
	struct iav_buffer srcbuf[IAV_SRCBUF_NUM];
	/* main_buffer always points to srcbuf[0] */
	struct iav_buffer *main_buffer;

	/* Stream */
	struct iav_stream stream[IAV_MAX_ENCODE_STREAMS_NUM];
	u32 stream_num;			/* actual alive stream number.*/
	struct iav_pts_info pts_info;

	struct iav_sync_frame_cnt frame_cnt;

	/* Used to capture raw/yuv/me1 data before encoding */
	struct iav_bufcap_info buf_cap;

	/* Encode from memory */
	struct iav_efm_info efm;

	/* Warp */
	struct iav_warp_main* warp;
	struct iav_warp_dptz warp_dptz[IAV_SRCBUF_NUM];
	u16 curr_warp_index;
	u16 next_warp_index;

	/*Privacy mask*/
	u8 curr_pm_index;
	u8 next_pm_index;
	u8 pm_bpc_enable;
	u8 bpc_enable;
	struct iav_privacy_mask pm;

	/* Encoder */
	BIT_STREAM_HDR *bsh_virt;	/* bit stream header pool */
	dma_addr_t bsh_phys;
	u32 bsh_size;
	BIT_STREAM_HDR *bsh;		/* current bit stream header */
	wait_queue_head_t frame_wq;
	wait_queue_head_t mv_wq;
	struct list_head frame_queue;
	struct list_head frame_free;
	struct iav_frame_desc *frame_queue_virt;  /* frame queue header */

	u32 bsb_free_bytes;
	u32 bsb_mmap_count;
	default_enc_binary_data_t *dsp_enc_data_virt;
	default_dec_binary_data_t *dsp_dec_data_virt;

	/* cmd sync*/
	struct iav_cmd_sync cmd_sync;

	/* Encode from RAW (RGB/YUV)*/
	struct iav_raw_encode raw_enc;

	/* Netlink */
	struct iav_nl_obj nl_obj[NL_OBJ_MAX_NUM];

	/* motion vector dump */
	//ENCODER_STATISTIC *esh_virt;	/* encoder statistics header pool */
	//dma_addr_t esh_phys;
	//u32 esh_size;
	//ENCODER_STATISTIC *esh;	/* current encoder statistics header */
	//wait_queue_head_t statis_wq;
	//struct list_head statis_queue;
	//struct list_head statis_free;
	//u32 statis_free_bytes;

	/* vin & vout information */
	struct vin_video_format *vin_probe_format;
	struct vin_controller *vinc[VIN_INSTANCES];
	struct amba_iav_vout_info *pvoutinfo[2];
	u32 vin_enabled : 1;
	u32 mixer_a_enable : 1;
	u32 mixer_b_enable : 1;
	u32 osd_from_mixer_a : 1;
	u32 osd_from_mixer_b : 1;
	u32 vin_overflow_protection : 1;
	u32 reserved1 : 26;

	/* hash cmd */
	struct mutex hash_mutex; // seperate mutex, not do impact to encoding
	wait_queue_head_t hash_wq;
	u32 wait_hash_msg;
	u32 hash_msg_cnt;
	u8 hash_output[4];

	/* Error handling */
	u32 err_vsync_handling : 1;
	u32 err_vsync_again : 1;
	u32 err_vsync_lost : 1;
	u32 reserved2 : 29;
	wait_queue_head_t err_wq;

	/* Debug tool */
	struct iav_debug_cfg *debug;
	u32 dsp_used_dram;

	/* dsplog setup */
	struct iav_dsplog_setup dsplog_setup;
};

#define	get_next_cmd(cmd, first)			do {		\
			list_for_each_entry(cmd, &first->head, node) {	\
				if (cmd->dsp_cmd.cmd_code == 0)			\
					break;						\
			}	\
		} while (0)

static inline int is_invalid_win(struct iav_window *win)
{
	int rval = 0;

	if (win->width > 0xffff || win->height > 0xffff)
		rval = 1;

	return rval;
}

static inline int is_invalid_input_win(struct iav_rect *input_win)
{
	int rval = 0;

	if (input_win->x > 0xffff || input_win->y > 0xffff
		|| input_win->width > 0xffff || input_win->height > 0xffff
		|| input_win->width == 0 || input_win->height == 0)
		rval = 1;

	return rval;
}


static inline int get_MB_num(int pixel)
{
	return ALIGN(pixel, PIXEL_IN_MB) / PIXEL_IN_MB;
}

static inline int get_row_pitch_in_MB(int MB_count_in_row)
{
	return ALIGN(MB_count_in_row * sizeof(u32), 32);
}

extern struct iav_buffer_limitation G_buffer_limit[];
extern struct iav_enc_limitation G_encode_limit[];
extern struct iav_system_config G_system_config[];
extern struct iav_system_load G_system_load[];
extern struct ambpriv_device *iav_device;

#endif

