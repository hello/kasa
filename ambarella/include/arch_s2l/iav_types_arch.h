/*
 * iav_ioctl.h
 *
 * History:
 *	2015/10/14 - [Zhikan Yang] Created file
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

#ifndef __IAV_TYPE_ARCH_H__
#define __IAV_TYPE_ARCH_H__

enum iav_srcbuf_id {
	IAV_SRCBUF_MN = 0,		/* main buffer */
	IAV_SRCBUF_PC,			/* 2nd buffer */
	IAV_SRCBUF_PB,			/* 3rd buffer */
	IAV_SRCBUF_PA,			/* 4th buffer */
	IAV_SRCBUF_PMN,		/* virtual pre-main buffer, only for dewarp mode */
	IAV_SRCBUF_EFM,		/* efm buffer */
	IAV_SRCBUF_NUM,

	/* For user space convenience */
	IAV_SRCBUF_1 = IAV_SRCBUF_MN,
	IAV_SRCBUF_2 = IAV_SRCBUF_PC,
	IAV_SRCBUF_3 = IAV_SRCBUF_PB,
	IAV_SRCBUF_4 = IAV_SRCBUF_PA,

	IAV_SRCBUF_FIRST = IAV_SRCBUF_MN,
	IAV_SRCBUF_LAST = IAV_SRCBUF_PMN,
	IAV_SRCBUF_LAST_PMN = IAV_SRCBUF_PMN + 1,
	IAV_SUB_SRCBUF_FIRST = IAV_SRCBUF_PC,
	IAV_SUB_SRCBUF_LAST = IAV_SRCBUF_PMN,
};

struct iav_system_resource {
	u8 encode_mode;
	u8 max_num_encode;
	u8 max_num_cap_sources;
	u8 exposure_num;

	struct iav_window buf_max_size[IAV_SRCBUF_NUM];
	struct iav_window stream_max_size[IAV_STREAM_MAX_NUM_ALL];
	u8 stream_max_M[IAV_STREAM_MAX_NUM_ALL];
	u8 stream_max_N[IAV_STREAM_MAX_NUM_ALL];
	u8 stream_max_advanced_quality_model[IAV_STREAM_MAX_NUM_ALL];
	u8 stream_long_ref_enable[IAV_STREAM_MAX_NUM_ALL];

	/* Read only */
	u32 raw_pitch_in_bytes;
	u32 total_memory_size : 8;
	u32 hdr_type : 2;
	u32 iso_type : 3;
	u32 is_stitched : 1;
	u32 reserved1 : 18;

	/* Writable for different configuration */
	u32 rotate_enable : 1;
	u32 raw_capture_enable : 1;
	u32 vout_swap_enable : 1;
	u32 lens_warp_enable : 1;
	u32 enc_raw_rgb : 1;
	u32 mixer_a_enable : 1;
	u32 mixer_b_enable : 1;
	u32 osd_from_mixer_a : 1;
	u32 osd_from_mixer_b : 1;
	u32 idsp_upsample_type : 2;
	u32 mctf_pm_enable : 1;
	u32 me0_scale : 2;
	u32 enc_from_mem : 1;
	u32 enc_raw_yuv : 1;
	u32 eis_delay_count : 2;
	u32 vin_overflow_protection: 1;
	u32 long_ref_b_frame : 1;
	u32 extra_top_row_buf_enable : 1;
	u32 reserved2 : 11;
	u32 dsp_partition_map;

	struct iav_window raw_size;	/* Only for encode from raw (RGB/YUV) feature */
	struct iav_window efm_size;	/* Only for encode from memory feature */
	s8  extra_dram_buf[IAV_SRCBUF_NUM];
	s8  reserved3[2];
	u16 efm_buf_num;
	u16 max_warp_input_width;
	u16 max_warp_input_height;
	u16 max_warp_output_width;
	u16 max_padding_width; // For LDC stitching
	u16 v_warped_main_max_width;
	u16 v_warped_main_max_height;
	u16 enc_dummy_latency;

	/* Debug only */
	u32 debug_enable_map;
	u32 debug_stitched : 1;
	u32 debug_iso_type : 3;
	u32 debug_chip_id : 5;
	u32 reserved4 : 23;

	/* Different for ARCHs */
	u32 arch;		/* Read only */
	struct iav_res_s3l res;
};

#endif
