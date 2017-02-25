/*
 * iav_enc_utils.h
 *
 * History:
 *	2011/11/11 - [Jian Tang] created file
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


#ifndef __IAV_ENCODE_H__
#define __IAV_ENCODE_H__

#include "iav_config.h"

typedef enum {
	ENCODE_ASPECT_RATIO_UNSPECIFIED = 0,
	ENCODE_ASPECT_RATIO_1_1_SQUARE_PIXEL = 1,
	ENCODE_ASPECT_RATIO_PAL_4_3 = 2,
	ENCODE_ASPECT_RATIO_NTSC_4_3 = 3,
	ENCODE_ASPECT_RATIO_CUSTOM = 255,
} ENCODE_ASPECT_RATIO_TYPE;


#ifdef CONFIG_PLAT_AMBARELLA_SUPPORT_GDMA
extern int dma_pitch_memcpy(struct iav_gdma_param *params);
#else
int dma_pitch_memcpy(struct iav_gdma_param *params);
#endif


/* iav_enc_buf.c */
void inc_srcbuf_ref(struct iav_buffer *srcbuf, u32 stream_id);
void dec_srcbuf_ref(struct iav_buffer *srcbuf, u32 stream_id);

/* iav_enc_test.c */
int is_iav_no_check(void);
#define	iav_no_check()		do {		\
		if (is_iav_no_check()) {	\
			iav_debug("Skip parameter CHECK.\n");			\
			return 0;			\
		}	\
	} while (0)

/* iav_enc_utils.c */
int is_enc_work_state(struct ambarella_iav * iav);
int is_warp_mode(struct ambarella_iav *iav);
int is_stitched_vin(struct ambarella_iav *iav, u16 enc_mode);
int get_hdr_type(struct ambarella_iav *iav);
int get_iso_type(struct ambarella_iav *iav, u32 debug_enable_map);
u32 get_pm_unit(struct ambarella_iav *iav);
u32 get_pm_domain(struct ambarella_iav *iav);
u32 get_chip_id(struct ambarella_iav *iav);
int get_gcd(int a, int b);
u64 get_monotonic_pts(void);
int get_vin_win(struct ambarella_iav *iav, struct iav_rect **vin, u8 vin_win_flag);
int get_vout_win(u32 buf_id, struct iav_window *vout);
int get_stream_win_MB(struct iav_stream_format *stream_format, struct iav_window *win);
int wait_vcap_count(struct ambarella_iav *iav, u32 count);
int iav_mem_copy(struct ambarella_iav *iav, u32 buff_id, u32 src_offset,
	u32 dst_offset, u32 size, u32 pitch);
u32 get_dsp_encode_bitrate(struct iav_stream *stream);

/* iav_enc_warp.c */
int cfg_default_multi_warp(struct ambarella_iav *iav);
int clear_default_warp_dptz(struct ambarella_iav *iav, u32 buf_map);
int set_default_warp_dptz(struct ambarella_iav *iav, u32 buf_map,
	struct amb_dsp_cmd *cmd);
void update_warp_aspect_ratio(int buf_id, int area_id,
	struct iav_rect* input, struct iav_rect* output);
void get_aspect_ratio_in_warp_mode(struct iav_stream *stream,
	u8 * aspect_ratio_idc, u16 * sar_width, u16 * sar_height);

#endif	// __IAV_ENCODE_H__

