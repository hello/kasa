/*
 * iav_dsp_cmd.c
 *
 * History:
 *	2013/08/28 - [Cao Rongrong] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <plat/ambcache.h>
#include <iav_utils.h>
#include <iav_ioctl.h>
#include <dsp_cmd_msg.h>
#include <dsp_api.h>
#include <dsp_format.h>
#include <vin_api.h>
#include <vout_api.h>
#include "iav.h"
#include "iav_dsp_cmd.h"
#include "iav_vin.h"
#include "iav_vout.h"
#include "iav_enc_utils.h"
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/clk.h>


#define GET_DSP_CMD_CODE(cmd_code, stream)	((cmd_code) | (((stream) & 0x3) << 30))
#define GET_STRAME_ID_CODE(cmd_code)	((cmd_code >> 30) & 0x3)

typedef enum {
	FRAME_RATE_SRC_VIN = 0,
	FRAME_RATE_SRC_IDSP_OUT = 1,
} FRAME_RATE_SRC;

static inline void *get_cmd(struct dsp_device *dsp, struct amb_dsp_cmd *cmd)
{
	if (!cmd) {
		cmd = dsp->get_cmd(dsp, DSP_CMD_FLAG_NORMAL);
		if (!cmd) {
			iav_error("No enough memory for DSP command!\n");
			return NULL;
		}
	}

	return &cmd->dsp_cmd;
}

static inline void put_cmd(struct dsp_device *dsp, struct amb_dsp_cmd *cmd,
	void *dsp_cmd, int cmd_type, u32 cmd_delay)
{
	if (likely(cmd)) {
		cmd->cmd_type = cmd_type;
	} else {
		cmd = container_of(dsp_cmd, struct amb_dsp_cmd, dsp_cmd);
		cmd->cmd_type = cmd_type;
		dsp->put_cmd(dsp, cmd, cmd_delay);
	}
}

static int is_vcap_interlaced(struct ambarella_iav *iav, int channel)
{
	return (AMBA_VIDEO_FORMAT_INTERLACE ==
		iav->vinc[channel]->vin_format.video_format);
}

static void init_jpeg_dqt(u8 *qtbl, int quality)
{
	static const u8 std_jpeg_qt[128] = {
		0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, 0x0E,
		0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28,
		0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 0x25,
		0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33,
		0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 0x44,
		0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57,
		0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 0x71,
		0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63,
		0x11, 0x12, 0x12, 0x18, 0x15, 0x18, 0x2F, 0x1A,
		0x1A, 0x2F, 0x63, 0x42, 0x38, 0x42, 0x63, 0x63,
		0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
		0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
		0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
		0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
		0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63,
		0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63, 0x63
	};

	int i, scale, temp;

	scale = (quality < 50) ? (5000 / quality) : (200 - quality * 2);

	for (i = 0; i < 128; i++) {
		temp = ((long) std_jpeg_qt[i] * scale + 50L) / 100L;
		/* limit the values to the valid range */
		if (temp <= 0L) temp = 1L;
		else if (temp > 255L) temp = 255L; /* max quantizer needed for baseline */
		qtbl[i] = temp;
	}
}

static void init_stream_jpeg_dqt(struct ambarella_iav *iav,
	int stream_id, int quality)
{
	static int qtbl_idx[IAV_MAX_ENCODE_STREAMS_NUM] = {0};
	struct iav_stream *stream = &iav->stream[stream_id];
	struct iav_mjpeg_config *config = &stream->mjpeg_config;
	int buf_size, curr_idx;

	curr_idx = qtbl_idx[stream_id];
	buf_size = IAV_MAX_ENCODE_STREAMS_NUM * JPEG_QT_SIZE;
	config->jpeg_quant_matrix = (void*)iav->mmap[IAV_BUFFER_QUANT].virt +
		curr_idx * buf_size + stream_id * JPEG_QT_SIZE;

	init_jpeg_dqt(config->jpeg_quant_matrix, quality);
	qtbl_idx[stream_id] = (curr_idx + 1) % JPEG_QT_BUFFER_NUM;
}

static inline int get_srcbuf_id(u8 capture_source,  enum iav_srcbuf_id *buf_id)
{
	int ret = 0;

	switch (capture_source){
	case DSP_CAPBUF_MN:
		*buf_id = IAV_SRCBUF_MN;
		break;
	case DSP_CAPBUF_PA:
		*buf_id = IAV_SRCBUF_PA;
		break;
	case DSP_CAPBUF_PB:
		*buf_id = IAV_SRCBUF_PB;
		break;
	case DSP_CAPBUF_PC:
		*buf_id = IAV_SRCBUF_PC;
		break;
	default:
		ret = -1;
		iav_error("Invalid dsp capture id\n");
		break;
	}

	return ret;
}

static inline int get_colour_primaries(struct iav_stream *stream,
	u32 * color_primaries, u32 * transfer_characteristics, u32 * matrix_coefficients)
{
	int width, height;

	width = stream->format.enc_win.width;
	height = stream->format.enc_win.height;
	if (((width == 720) && (height == 480)) ||
	   ((width == 704) && (height == 480))  ||
	   ((width == 352) && (height == 240))) {
		//NTSC
		*color_primaries = 6;
		*transfer_characteristics = 6;
		*matrix_coefficients = 6;
	} else if (((width == 720) && (height == 576)) ||
		   ((width == 704) && (height == 576)) ||
		   ((width == 352) && (height == 288))) {
		//PAL
		*color_primaries = 5;
		*transfer_characteristics = 5;
		*matrix_coefficients = 5;
	} else {
		//default
		*color_primaries = 1;
		*transfer_characteristics = 1;
		*matrix_coefficients = 1;
	}
	return 0;
}

static inline int get_aspect_ratio(struct iav_buffer *buffer,
	u16 *sar_width, u16 *sar_height)
{
	int gcd, num, den;

	num = buffer->input.width * buffer->win.height;
	den = buffer->input.height * buffer->win.width;
	gcd = get_gcd(num, den);
	num = num / gcd * (*sar_width);
	den = den / gcd * (*sar_height);
	gcd = get_gcd(num, den);
	*sar_width = num / gcd;
	*sar_height = den / gcd;

	return 0;
}

static inline int get_rounddown_height(int height)
{
	int round_h;

	if (height == 1088) {
		round_h = 1080;
	} else {
		round_h = height;
	}

	return round_h;
}

static inline void calc_roundup_size(u16 width_in,
	u16 height_in, u16 enc_type, u16 is_interlaced,
	u16 *width_out, u16 *height_out)
{
	u16 round_h;

	if (enc_type == IAV_STREAM_TYPE_H264) {
		*width_out = ALIGN(width_in, PIXEL_IN_MB);
		round_h = is_interlaced ? (PIXEL_IN_MB << 1) : PIXEL_IN_MB;
		*height_out = ALIGN(height_in, round_h);
	} else {
		*width_out = width_in;
		*height_out = height_in;
	}
}

static inline int check_idsp_upsample_factor(u8 multiplication_factor,
	u8 division_factor)
{
	if (multiplication_factor < division_factor) {
		return -1;
	}
	return 0;
}

static inline int calc_frame_rate(struct ambarella_iav *iav,
	u32 interlaced, u32 deintlc_option,
	u32 multiplication_factor, u32 division_factor,
	u32 * dsp_out_frame_rate, u8 source_fps)
{
	u8	den_encode_frame_rate = 0;
	u32	custom_encoder_frame_rate;
	u32 idsp_out_frame_rate = 0;

	if (source_fps == FRAME_RATE_SRC_IDSP_OUT) {
		if (iav_vin_get_idsp_frame_rate(iav, &idsp_out_frame_rate) < 0) {
			return -1;
		}
	} else {
		idsp_out_frame_rate = iav->vinc[0]->vin_format.frame_rate;
	}
	switch (idsp_out_frame_rate) {
	case AMBA_VIDEO_FPS_AUTO:
		iav_error("Divider of vin_frame_rate can not be zero\n");
		return -1;
	case AMBA_VIDEO_FPS_59_94:
		den_encode_frame_rate = 1;
		if (interlaced)
			custom_encoder_frame_rate = 30000;			// 59.94i --> 29.97p
		else
			custom_encoder_frame_rate = 60000;			// 59.94p
		break;
	case AMBA_VIDEO_FPS_50:
		den_encode_frame_rate = 0;
		if (interlaced)
			custom_encoder_frame_rate = 25000;			// 50i --> 25p
		else
			custom_encoder_frame_rate = 50000;			// 50p
		break;
	case AMBA_VIDEO_FPS_29_97:
		den_encode_frame_rate = 1;
		custom_encoder_frame_rate = 30000;
		break;
	case AMBA_VIDEO_FPS_23_976:
		den_encode_frame_rate = 1;
		custom_encoder_frame_rate = 24000;
		break;
	case AMBA_VIDEO_FPS_12_5:
		custom_encoder_frame_rate = 12500;
		break;
	case AMBA_VIDEO_FPS_6_25:
		custom_encoder_frame_rate = 6250;
		break;
	case AMBA_VIDEO_FPS_3_125:
		custom_encoder_frame_rate = 3125;
		break;
	case AMBA_VIDEO_FPS_7_5:
		custom_encoder_frame_rate = 7500;
		break;
	case AMBA_VIDEO_FPS_3_75:
		custom_encoder_frame_rate = 3750;
		break;
	default:
		custom_encoder_frame_rate = DIV_ROUND_CLOSEST(512000000,
			(idsp_out_frame_rate / 1000));
		break;
	}
	custom_encoder_frame_rate = custom_encoder_frame_rate *
		multiplication_factor / division_factor;
	*dsp_out_frame_rate = (0 |
		(den_encode_frame_rate << 30) |
		custom_encoder_frame_rate);

	iav_debug("idsp out fps %d (scan format: %d, den_encode_fps: %d)"
		", encode fps %d\n", idsp_out_frame_rate, 0,
		den_encode_frame_rate ? 1001 : 1000, custom_encoder_frame_rate);

	return 0;
}

static int calc_quality_model(struct iav_h264_config *h264_config)
{
	static u8 lvarr[] = {1, 2, 4, 8};
	int lv = -1;
	int i, gop;

	if (h264_config->M == 0 || h264_config->M > 8) {
		iav_error("bad M %d\n", h264_config->M);
		return -1;
	}
	if (h264_config->N == 0 || (h264_config->N % h264_config->M) != 0) {
		iav_error("bad N %d\n", h264_config->N);
		return -1;
	}
	if (h264_config->idr_interval == 0) {
		iav_error("bad idr_interval %d\n", h264_config->idr_interval);
		return -1;
	}

	switch (h264_config->gop_structure) {
	case IAV_GOP_SIMPLE:
		gop = SIMPLE_GOP;
		break;
	case IAV_GOP_SVCT_2:
		lv = 2;
		gop = (lv << 5) | HI_P_GOP;
		break;
	case IAV_GOP_SVCT_3:
		lv = 3;
		gop = (lv << 5) | HI_P_GOP;
		break;
	case IAV_GOP_NON_REF_P:
		gop = NON_REF_P_GOP;
		break;
	case IAV_GOP_HI_P:
		gop = HI_P_GOP;
		break;
	case IAV_GOP_LT_REF_P:
		gop = LT_REF_P_GOP;
		if (h264_config->multi_ref_p) {
			lv = 1;
			gop = (lv << 5) | LT_REF_P_GOP;
		}
		break;
	case IAV_GOP_ADVANCED:
		for (i = 0; i < ARRAY_SIZE(lvarr); i++) {
			if (h264_config->M == lvarr[i]) {
				lv = i + 1;
				break;
			}
		}
		if (lv < 0) {
			gop = lv;
		} else {
			gop = (lv << 5) | 4;
		}
		break;
	default:
		gop = -1;
		break;
	}

	return gop;
}

static int get_gop_struct_from_quailty(struct iav_h264_config *h264_config, int gop)
{
	switch (gop) {
	case SIMPLE_GOP:
		h264_config->gop_structure = IAV_GOP_SIMPLE;
		break;
	case ((2 << 5) | HI_P_GOP):
		h264_config->gop_structure = IAV_GOP_SVCT_2;
		break;
	case ((3 << 5) | HI_P_GOP):
		h264_config->gop_structure = IAV_GOP_SVCT_3;
		break;
	case NON_REF_P_GOP:
		h264_config->gop_structure = IAV_GOP_NON_REF_P;
		break;
	case HI_P_GOP:
		h264_config->gop_structure = IAV_GOP_HI_P;
		break;
	case ((1 << 5) | LT_REF_P_GOP):
		h264_config->multi_ref_p = 1;
		h264_config->gop_structure = IAV_GOP_LT_REF_P;
		break;
	default:
		h264_config->gop_structure = IAV_GOP_ADVANCED;
		break;
	}

	return 0;
}

static u32 get_dsp_encode_bitrate(struct iav_stream *stream)
{
	u32 ff_multi = stream->fps.fps_multi;
	u32 ff_division = stream->fps.fps_div;
	u32 full_bitrate = stream->h264_config.average_bitrate;
	u32 bitrate = 0;

	if (ff_division) {
		bitrate =  full_bitrate * ff_multi / ff_division;
	}

	return bitrate;
}

inline void get_round_encode_format(struct iav_stream *stream,
	u16* width, u16* height, s16* offset_y_shift)
{
	struct iav_stream_format *format = &stream->format;
	struct iav_rect *enc_win = &format->enc_win;
	u16 width_round, height_round;

	calc_roundup_size(enc_win->width, enc_win->height, format->type,
		is_vcap_interlaced(stream->iav, 0), &width_round, &height_round);

	if (format->rotate_cw) {
		*width = height_round;
		*height = width_round;
		*offset_y_shift =
			(format->hflip ? 0 : (enc_win->height - height_round));
	} else {
		*width = width_round;
		*height = height_round;
		*offset_y_shift =
			(format->vflip ? (enc_win->height - height_round) : 0);
	}
}

u32 get_buffer_pool_map(u32 in_map)
{
	u32 out_map = 0;

	if (in_map & (1 << IAV_DSP_SUB_BUF_RAW)) {
		out_map |= (1 << FRM_BUF_POOL_TYPE_RAW);
	}
	if ((in_map & (1 << IAV_DSP_SUB_BUF_MAIN_YUV)) ||
		(in_map & (1 << IAV_DSP_SUB_BUF_MAIN_ME1))) {
		out_map |= (1 << FRM_BUF_POOL_TYPE_MAIN_CAPTURE);
	}
	if ((in_map & (1 << IAV_DSP_SUB_BUF_PREV_A_YUV)) ||
		(in_map & (1 << IAV_DSP_SUB_BUF_PREV_A_ME1))) {
		out_map |= (1 << FRM_BUF_POOL_TYPE_PREVIEW_A);
	}
	if ((in_map & (1 << IAV_DSP_SUB_BUF_PREV_B_YUV)) ||
		(in_map & (1 << IAV_DSP_SUB_BUF_PREV_B_ME1))) {
		out_map |= (1 << FRM_BUF_POOL_TYPE_PREVIEW_B);
	}
	if ((in_map & (1 << IAV_DSP_SUB_BUF_PREV_C_YUV)) ||
		(in_map & (1 << IAV_DSP_SUB_BUF_PREV_C_ME1))) {
		out_map |= (1 << FRM_BUF_POOL_TYPE_PREVIEW_C);
	}
	if ((in_map & (1 << IAV_DSP_SUB_BUF_POST_MAIN_YUV)) ||
		(in_map & (1 << IAV_DSP_SUB_BUF_POST_MAIN_ME1))) {
		out_map |= (1 << FRM_BUF_POOL_TYPE_WARPED_MAIN_CAPTURE);
	}
	if (in_map & (1 << IAV_DSP_SUB_BUF_INT_MAIN_YUV)) {
		out_map |= (1 << FRM_BUF_POOL_TYPE_WARPED_MAIN_CAPTURE);
	}
	if ((in_map & (1 << IAV_DSP_SUB_BUF_EFM_YUV)) ||
		(in_map & (1 << IAV_DSP_SUB_BUF_EFM_ME1))) {
		out_map |= (1 << FRM_BUF_POOL_TYPE_EFM);
	}

	return out_map;
}

static inline preview_type_t buffer_format_to_preview_type(u32 type, u16 width)
{
	preview_type_t preview_type;

	switch (type) {
	case IAV_SRCBUF_TYPE_OFF:
		preview_type = OFF_PREVIEW_TYPE;
		break;
	case IAV_SRCBUF_TYPE_ENCODE:
		preview_type = CAPTURE_PREVIEW_TYPE;
		break;
	case IAV_SRCBUF_TYPE_PREVIEW:
		if (width > 720)
			preview_type = HD_PREVIEW_TYPE;
		else
			preview_type = SD_PREVIEW_TYPE;
		break;
	default:
		preview_type = OFF_PREVIEW_TYPE;
		break;
	}
	return preview_type;
}

static inline int preview_type_to_buffer_format(u32 type)
{
	enum iav_srcbuf_type srcbuf_type;

	switch (type) {
	case OFF_PREVIEW_TYPE:
		srcbuf_type = IAV_SRCBUF_TYPE_OFF;
		break;
	case CAPTURE_PREVIEW_TYPE:
		srcbuf_type = IAV_SRCBUF_TYPE_ENCODE;
		break;
	case HD_PREVIEW_TYPE:
	case SD_PREVIEW_TYPE:
		srcbuf_type = IAV_SRCBUF_TYPE_PREVIEW;
		break;
	default:
		srcbuf_type = IAV_SRCBUF_TYPE_OFF;
		break;
	}

	return srcbuf_type;
}

static u32 is_h264_frame_num_gap_allowed(struct iav_stream *stream)
{
	u32 allow_flag = 0;
	struct iav_h264_config *h264_cfg = &stream->h264_config;

	switch(h264_cfg->gop_structure) {
	case IAV_GOP_SVCT_2:
	case IAV_GOP_SVCT_3:
		allow_flag = 1;
		break;
	case IAV_GOP_LT_REF_P:
		// 2 ref without fast seek
		if ((h264_cfg->multi_ref_p == 1) &&
			(h264_cfg->long_term_intvl == 0 || h264_cfg->long_term_intvl == 63)) {
			allow_flag = 0;
		} else {
			allow_flag = 1;
		}
		break;
	default:
		allow_flag = 0;
		break;
	}

	return allow_flag;
}


int cmd_chip_selection(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd)
{
	chip_select_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = CHIP_SELECTION;
	dsp_cmd->chip_type = 3;	// A5

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_vin_timer_mode(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd)
{
	vin_timer_mode_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = H264_ENC_USE_TIMER;
	dsp_cmd->timer_scaler = 0;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_set_warp_control(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd)
{
	set_warp_control_t *dsp_cmd;
	struct iav_rect *vin;
	struct iav_rect *input_win;
	struct iav_warp_map *warp_map;
	u8 enc_mode = iav->encode_mode;
	u8 lens_warp_enable = iav->system_config[enc_mode].lens_warp;
	u8 expo_num = iav->system_config[enc_mode].expo_num;
	u16 crop_w, crop_h, crop_x, crop_y, vin_w, vin_h;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	iav->curr_warp_index = iav->next_warp_index + UWARP_NUM;

	dsp_cmd->cmd_code = SET_WARP_CONTROL;

	// lens warp parameters
	if (lens_warp_enable && iav->warp->area[0].enable) {
		warp_map = &iav->warp->area[0].h_map;
		if (warp_map->enable) {
			dsp_cmd->warp_control = WARP_CONTROL_ENABLE;
			dsp_cmd->grid_array_width = warp_map->output_grid_col;
			dsp_cmd->grid_array_height = warp_map->output_grid_row;
			dsp_cmd->horz_grid_spacing_exponent = warp_map->h_spacing;
			dsp_cmd->vert_grid_spacing_exponent = warp_map->v_spacing;
			dsp_cmd->warp_horizontal_table_address =
				PHYS_TO_DSP(iav->mmap[IAV_BUFFER_WARP].phys +
					WARP_VECT_PART_SIZE * (iav->curr_warp_index) +
					warp_map->data_addr_offset);
		}
		warp_map = &iav->warp->area[0].v_map;
		if (warp_map->enable) {
			dsp_cmd->vert_warp_enable = WARP_CONTROL_ENABLE;
			dsp_cmd->vert_warp_grid_array_width = warp_map->output_grid_col;
			dsp_cmd->vert_warp_grid_array_height = warp_map->output_grid_row;
			dsp_cmd->vert_warp_horz_grid_spacing_exponent = warp_map->h_spacing;
			dsp_cmd->vert_warp_vert_grid_spacing_exponent = warp_map->v_spacing;
			dsp_cmd->warp_vertical_table_address =
				PHYS_TO_DSP(iav->mmap[IAV_BUFFER_WARP].phys +
					WARP_VECT_PART_SIZE * (iav->curr_warp_index) +
					warp_map->data_addr_offset);

			// ME1 Warp for LDC
			warp_map = &iav->warp->area[0].me1_v_map;
			dsp_cmd->ME1_vwarp_grid_array_width = warp_map->output_grid_col;
			dsp_cmd->ME1_vwarp_grid_array_height = warp_map->output_grid_row;
			dsp_cmd->ME1_vwarp_horz_grid_spacing_exponent = warp_map->h_spacing;
			dsp_cmd->ME1_vwarp_vert_grid_spacing_exponent = warp_map->v_spacing;
			dsp_cmd->ME1_vwarp_table_address =
				PHYS_TO_DSP(iav->mmap[IAV_BUFFER_WARP].phys +
					WARP_VECT_PART_SIZE * (iav->curr_warp_index) +
					warp_map->data_addr_offset);
		}
	}

	if (is_warp_mode(iav) || lens_warp_enable) {
		if (lens_warp_enable && iav->warp->area[0].enable) {
			input_win = &iav->warp->area[0].input;
		} else {
			input_win = &iav->srcbuf[IAV_SRCBUF_PMN].input;
		}
	} else {
		input_win = &iav->main_buffer->input;
	}

	get_vin_win(iav, &vin, 0);
	if (expo_num >= MAX_HDR_2X) {
		crop_x = input_win->x + vin->x;
		crop_y = input_win->y + vin->y;
	} else {
		crop_x = input_win->x;
		crop_y = input_win->y;
	}
	crop_w = input_win->width;
	crop_h = input_win->height;

	// TODO: add VIN crop offset
	vin_w = vin->width;
	vin_h = vin->height;

	dsp_cmd->x_center_offset = crop_x - (vin_w- crop_w) / 2;
	dsp_cmd->y_center_offset = crop_y - (vin_h- crop_h) / 2;
	dsp_cmd->dummy_window_x_left = crop_x;
	dsp_cmd->dummy_window_y_top = crop_y;
	dsp_cmd->dummy_window_width = crop_w;
	dsp_cmd->dummy_window_height = crop_h;

	crop_w = (crop_w > MAX_WIDTH_IN_FULL_FPS) ? MAX_WIDTH_IN_FULL_FPS : crop_w;
	if (iav->encode_mode == DSP_SINGLE_REGION_WARP_MODE) {
		/* CFA pre-sampler to down scale from VIN to pre-main buffer */
		if (crop_w < iav->srcbuf[IAV_SRCBUF_MN].win.width ||
			crop_h < iav->srcbuf[IAV_SRCBUF_MN].win.height) {
			dsp_cmd->cfa_output_width = crop_w;
			dsp_cmd->cfa_output_height = crop_h;
		} else {
			dsp_cmd->cfa_output_width = iav->srcbuf[IAV_SRCBUF_PMN].win.width;
			dsp_cmd->cfa_output_height = iav->srcbuf[IAV_SRCBUF_PMN].win.height;
		}
	} else {
		dsp_cmd->cfa_output_width = crop_w;
		dsp_cmd->cfa_output_height = crop_h;
	}

	dsp_cmd->actual_left_top_x = 0;
	dsp_cmd->actual_left_top_y = 0;
	dsp_cmd->actual_right_bot_x = (crop_w << 16);
	dsp_cmd->actual_right_bot_y = (crop_h << 16);

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	//update warp index
	iav->next_warp_index = (iav->next_warp_index + 1) % IAV_PARTITION_TOGGLE_NUM;

	return 0;
}

int cmd_system_info_setup(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd,
	u32 decode_flag)
{
	system_setup_info_t *dsp_cmd = NULL;
	struct amba_video_info *vout0_info = NULL, *vout1_info = NULL;
	struct iav_buffer *buffer = NULL;
	struct iav_system_config *config = NULL;
	u16 width, enc_mode;
	int iso_type;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = SYSTEM_SETUP_INFO;

	if (unlikely(decode_flag)) {
		dsp_cmd->audio_clk_freq = 12288000;
		dsp_cmd->idsp_freq = 216000000;
		put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_DEC, 0);
	} else {
		enc_mode = iav->encode_mode;
		dsp_cmd->mode_flags = enc_mode;

		config = &iav->system_config[enc_mode];
		if (config->vout_swap) {
			vout0_info = &G_voutinfo[1].video_info;
			vout1_info = &G_voutinfo[0].video_info;
		} else {
			vout0_info = &G_voutinfo[0].video_info;
			vout1_info = &G_voutinfo[1].video_info;
		}
		width = vout0_info->rotate ? vout0_info->height : vout0_info->width;
		buffer = &iav->srcbuf[IAV_SRCBUF_PA];
		dsp_cmd->preview_A_type = buffer_format_to_preview_type(buffer->type, width);

		width = vout1_info->rotate ? vout1_info->height : vout1_info->width;
		buffer = &iav->srcbuf[IAV_SRCBUF_PB];
		dsp_cmd->preview_B_type = buffer_format_to_preview_type(buffer->type, width);

		/* 0:DV mode  1:DVR mode  2:IPCAM mode */
		dsp_cmd->sub_mode_sel.multiple_stream = IP_CAMCORDER_APP_MODE;
		/* 0: high power, 1: Low power mode */
		dsp_cmd->sub_mode_sel.power_mode = 0;
		dsp_cmd->voutA_osd_blend_enabled = iav->osd_from_mixer_a;
		dsp_cmd->voutB_osd_blend_enabled = iav->osd_from_mixer_b;
		dsp_cmd->audio_clk_freq = 0;
		/* both RGB and YUV raw */
		dsp_cmd->raw_encode_enabled = (config->enc_raw_rgb || config->enc_raw_yuv);

		dsp_cmd->hdr_num_exposures_minus_1 = config->expo_num - 1;
		dsp_cmd->hdr_preblend_from_vin = ((config->expo_num == MAX_HDR_2X) &&
			(G_encode_limit[enc_mode].hdr_2x_supported == HDR_TYPE_BASIC));

		switch (enc_mode) {
		case DSP_ADVANCED_ISO_MODE:
		case DSP_HDR_LINE_INTERLEAVED_MODE:
		case DSP_BLEND_ISO_MODE:
			iso_type = get_iso_type(iav, config->debug_enable_map);
			dsp_cmd->adv_iso_disabled =
				(iso_type != ISO_TYPE_ADVANCED && iso_type != ISO_TYPE_BLEND);
			break;
		default:
			break;
		}

		dsp_cmd->vout_swap = config->vout_swap;
		dsp_cmd->me0_scale_factor = config->me0_scale;

		dsp_cmd->core_freq = (u32)clk_get_rate(clk_get(NULL, "gclk_core"));

		put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);
	}

	return 0;
}

int cmd_set_vin_global_config(struct ambarella_iav *iav,
		struct amb_dsp_cmd *cmd)
{
	set_vin_global_config_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = SET_VIN_GLOBAL_CLK;
#ifdef VIN_USING_PIP
	dsp_cmd->main_or_pip = 1;
#else
	dsp_cmd->main_or_pip = 0;
#endif
	switch (iav->vinc[0]->vin_format.interface_type) {
	case SENSOR_SERIAL_LVDS:
		dsp_cmd->global_cfg_reg_word0 = 0x8;
		break;
	case SENSOR_PARALLEL_LVCMOS:
	case SENSOR_PARALLEL_LVDS:
		dsp_cmd->global_cfg_reg_word0 = 0x9;
		break;
	case SENSOR_MIPI:
		dsp_cmd->global_cfg_reg_word0 = 0xA;
		break;
	default:
		BUG();
		break;
	}

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_set_vin_master_config(struct ambarella_iav *iav,
		struct amb_dsp_cmd *cmd)
{
	set_vin_master_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = SET_VIN_MASTER_CLK;

	memcpy(&dsp_cmd->master_sync_reg_word0,
		iav->vinc[0]->master_config, sizeof(struct vin_master_config));

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_set_vin_config(struct ambarella_iav *iav,
		struct amb_dsp_cmd *cmd)
{
	set_vin_config_t *dsp_cmd;
	struct iav_vin_format *vin_format;
	struct iav_rect *vin_win;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	vin_format = &iav->vinc[0]->vin_format;

#ifdef VIN_USING_PIP
	dsp_cmd->cmd_code = SET_VIN_PIP_CONFIG;
#else
	dsp_cmd->cmd_code = SET_VIN_CONFIG;
#endif
	get_vin_win(iav, &vin_win, 1);
	dsp_cmd->vin_width = vin_win->width;
	dsp_cmd->vin_height = vin_win->height;
	dsp_cmd->vin_config_dram_addr = VIRT_TO_DSP(iav->vinc[0]->dsp_config);
	dsp_cmd->config_data_size = sizeof(struct vin_dsp_config);
	dsp_cmd->sensor_resolution = 0;
	dsp_cmd->sensor_bayer_pattern = 0x0;

	clean_d_cache(iav->vinc[0]->dsp_config, sizeof(struct vin_dsp_config));

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_raw_encode_video_setup(struct ambarella_iav *iav,
		struct amb_dsp_cmd *cmd)
{
	raw_encode_video_setup_cmd_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = RAW_ENCODE_VIDEO_SETUP;
	if (iav->raw_enc.raw_enc_src == RAW_ENC_SRC_IMG) {
		dsp_cmd->sensor_raw_start_daddr = PHYS_TO_DSP(
			iav->mmap[IAV_BUFFER_IMG].phys + iav->raw_enc.raw_daddr_offset);
	} else {
		dsp_cmd->sensor_raw_start_daddr = PHYS_TO_DSP(
			iav->mmap[IAV_BUFFER_USR].phys + iav->raw_enc.raw_daddr_offset);
	}
	dsp_cmd->daddr_offset = iav->raw_enc.raw_frame_size;
	dsp_cmd->dpitch = iav->raw_enc.pitch;
	dsp_cmd->num_frames = iav->raw_enc.raw_frame_num;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_video_preproc(struct ambarella_iav *iav,
		struct amb_dsp_cmd *cmd)
{
	video_preproc_t *dsp_cmd;
	u32 width, height;
	u8 enc_mode = iav->encode_mode;
	struct iav_vin_format *vin_format;
	struct iav_buffer *buffer;
	struct iav_rect *vin_win;
	struct amba_video_info *vout0_info, *vout1_info;
	struct iav_system_config *config = &iav->system_config[enc_mode];
	u8 lens_warp_enable = config->lens_warp;
	int iso_type;
	u8 vout_swap = config->vout_swap;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	vin_format = &iav->vinc[0]->vin_format;

	dsp_cmd->cmd_code = VIDEO_PREPROCESSING;
	// For YUV raw encode, the input format will be YUV422
	if (likely(!config->enc_raw_yuv)) {
		dsp_cmd->input_format = vin_format->input_format;
	} else {
		dsp_cmd->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG;
	}
	dsp_cmd->sensor_id = vin_format->sensor_id;
	dsp_cmd->bit_resolution = vin_format->bit_resolution;
	dsp_cmd->bayer_pattern = vin_format->bayer_pattern;
	dsp_cmd->vin_frame_rate = amba_iav_fps_format_to_vfr(
		vin_format->frame_rate, vin_format->video_format, 0);
	dsp_cmd->sensor_readout_mode = 0;

	get_vin_win(iav, &vin_win, 1);
	dsp_cmd->vidcap_w = vin_win->width;
	dsp_cmd->vidcap_h = vin_win->height;
	dsp_cmd->input_center_x = 0;
	dsp_cmd->input_center_y = 0;
	dsp_cmd->zoom_factor_x = 0;
	dsp_cmd->zoom_factor_y = 0;

	//skip frames to avoid bad frame capture cause DSP VIN crash
	dsp_cmd->vid_skip = 2;

	if (enc_mode == DSP_MULTI_REGION_WARP_MODE) {
		buffer = &iav->srcbuf[IAV_SRCBUF_PMN];
	} else {
		buffer = &iav->srcbuf[IAV_SRCBUF_MN];
	}
	dsp_cmd->main_w = buffer->win.width;
	dsp_cmd->main_h = buffer->win.height;
	dsp_cmd->encode_x = 0;
	dsp_cmd->encode_y = 0;
	buffer = &iav->srcbuf[IAV_SRCBUF_MN];
	dsp_cmd->encode_w = buffer->win.width;
	dsp_cmd->encode_h = buffer->win.height;

	if (vout_swap) {
		vout0_info = &G_voutinfo[1].video_info;
		vout1_info = &G_voutinfo[0].video_info;
	} else {
		vout0_info = &G_voutinfo[0].video_info;
		vout1_info = &G_voutinfo[1].video_info;
	}
	if (vout0_info->rotate) {
		width = vout0_info->height;
		height = vout0_info->width;
	} else {
		width = vout0_info->width;
		height = vout0_info->height;
	}
	dsp_cmd->preview_w_A = width;
	dsp_cmd->preview_h_A = height;
	dsp_cmd->preview_format_A = amba_iav_format_to_format(vout0_info->format);
	dsp_cmd->preview_frame_rate_A = amba_iav_fps_to_fps(vout0_info->fps);
	dsp_cmd->preview_A_en = vout_swap ?
		G_voutinfo[1].enabled : G_voutinfo[0].enabled;

	if (vout1_info->rotate) {
		width = vout1_info->height;
		height = vout1_info->width;
	} else {
		width = vout1_info->width;
		height = vout1_info->height;
	}
	dsp_cmd->preview_w_B = vout1_info->width;
	dsp_cmd->preview_h_B = vout1_info->height;
	dsp_cmd->preview_format_B = amba_iav_format_to_format(vout1_info->format);
	dsp_cmd->preview_frame_rate_B = amba_iav_fps_to_fps(vout1_info->fps);
	dsp_cmd->preview_B_en = vout_swap ?
		G_voutinfo[0].enabled : G_voutinfo[1].enabled;

	dsp_cmd->keep_states = 0;

	dsp_cmd->main_out_frame_rate = 1;
	dsp_cmd->main_out_frame_rate_ext = 0;

	dsp_cmd->Vert_WARP_enable = (lens_warp_enable ||
		(enc_mode == DSP_MULTI_REGION_WARP_MODE));

	/* It's not available in mode 3. */
	if (config->debug_enable_map & DEBUG_TYPE_STITCH) {
		dsp_cmd->force_stitching = config->debug_stitched;
	} else {
		dsp_cmd->force_stitching = is_stitched_vin(iav, enc_mode);
	}

	// EIS delay count
	dsp_cmd->EIS_delay_count = config->eis_delay_count;

	iso_type = get_iso_type(iav, config->debug_enable_map);
	dsp_cmd->still_iso_config_daddr = (iso_type == ISO_TYPE_LOW) ? 0 :
		PHYS_TO_DSP(iav->mmap[IAV_BUFFER_IMG].phys + IAV_DRAM_IMG_OFFET);

#if USE_INSTANT_CMD
	dsp_cmd->cmdReadDly = iav->cmd_read_delay;	//  & (1 << 31);
#else
	dsp_cmd->cmdReadDly = 0;
#endif
	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_sensor_config(struct ambarella_iav *iav,
		struct amb_dsp_cmd *cmd)
{
	sensor_input_setup_t *dsp_cmd;
	struct iav_vin_format *vin_format;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	vin_format = &iav->vinc[0]->vin_format;

	dsp_cmd->cmd_code = SENSOR_INPUT_SETUP;
	dsp_cmd->sensor_id = vin_format->sensor_id;
	dsp_cmd->field_format = 1;
	dsp_cmd->sensor_resolution = vin_format->bit_resolution;
	dsp_cmd->sensor_pattern = vin_format->bayer_pattern;
	dsp_cmd->first_line_field_0 = 0;
	dsp_cmd->first_line_field_1 = 0;
	dsp_cmd->first_line_field_2 = 0;
	dsp_cmd->first_line_field_3 = 0;
	dsp_cmd->first_line_field_4 = 0;
	dsp_cmd->first_line_field_5 = 0;
	dsp_cmd->first_line_field_6 = 0;
	dsp_cmd->first_line_field_7 = 0;
	dsp_cmd->sensor_readout_mode = 0;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_video_system_setup(struct ambarella_iav *iav,
		struct amb_dsp_cmd *cmd)
{
	ipcam_video_system_setup_t *dsp_cmd = NULL;
	u16 max_w, max_h;
	int enc_mode = iav->encode_mode;
	struct amba_video_info *vout0_info = NULL, *vout1_info = NULL;
	struct iav_buffer *buffer = NULL;
	struct iav_rect *vin_win = NULL;
	struct iav_window *max_win = NULL;
	struct iav_system_config *config = &iav->system_config[enc_mode];
	int max_stream_num = config->max_stream_num;
	int lens_warp_enable = config->lens_warp;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = IPCAM_VIDEO_SYSTEM_SETUP;

	/* Resize maximum of the main buffer */
	buffer = &iav->srcbuf[IAV_SRCBUF_MN];
	calc_roundup_size(buffer->max.width, buffer->max.height,
		IAV_STREAM_TYPE_H264, 0, &max_w, &max_h);
	switch (enc_mode) {
	case DSP_MULTI_REGION_WARP_MODE:
		dsp_cmd->warped_main_max_width = max_w;
		dsp_cmd->warped_main_max_height = max_h;
		dsp_cmd->max_warp_region_input_width = config->max_warp_input_width;
		dsp_cmd->max_warp_region_input_height = config->max_warp_input_height;
		dsp_cmd->max_warp_region_output_width = config->max_warp_output_width;
		dsp_cmd->v_warped_main_max_width =
			ALIGN(config->v_warped_main_max_width, PIXEL_IN_MB);
		dsp_cmd->v_warped_main_max_height =
			ALIGN(config->v_warped_main_max_height, PIXEL_IN_MB);

		buffer = &iav->srcbuf[IAV_SRCBUF_PMN];
		calc_roundup_size(buffer->win.width, buffer->win.height,
			IAV_STREAM_TYPE_H264, 0, &max_w, &max_h);

		dsp_cmd->vwarp_blk_h = VWARP_BLOCK_HEIGHT_MAX;
		break;

	/* Specify PADDING Width for LDC Stitching */
	case DSP_ADVANCED_ISO_MODE:
		dsp_cmd->max_padding_width = config->max_padding_width;
		dsp_cmd->vwarp_blk_h = lens_warp_enable ?
			VWARP_BLOCK_HEIGHT_MAX_LDC : VWARP_BLOCK_HEIGHT_MIN;
		break;

	default:
		dsp_cmd->vwarp_blk_h = VWARP_BLOCK_HEIGHT_MIN;
		break;
	}
	dsp_cmd->main_max_width = max_w;
	dsp_cmd->main_max_height = max_h;

	dsp_cmd->enc_rotation_possible = config->rotatable;

	/* Resize maximum of the second buffer */
	buffer = &iav->srcbuf[IAV_SRCBUF_PC];
	switch (buffer->type) {
	case IAV_SRCBUF_TYPE_OFF:
		max_w = max_h = 0;
		break;
	case IAV_SRCBUF_TYPE_ENCODE:
		max_win = &buffer->max;
		calc_roundup_size(max_win->width, max_win->height,
			IAV_STREAM_TYPE_H264, 0, &max_w, &max_h);
		break;
	default:
		BUG();
		break;
	}
	dsp_cmd->preview_C_max_width = max_w;
	dsp_cmd->preview_C_max_height = max_h;

	/* Resize maximum of the third buffer */
	buffer = &iav->srcbuf[IAV_SRCBUF_PB];
	switch (buffer->type) {
	case IAV_SRCBUF_TYPE_OFF:
		max_w = max_h = 0;
		break;
	case IAV_SRCBUF_TYPE_ENCODE:
		max_win = &buffer->max;
		calc_roundup_size(max_win->width, max_win->height,
			IAV_STREAM_TYPE_H264, 0, &max_w, &max_h);
		break;
	case IAV_SRCBUF_TYPE_PREVIEW:
		if (config->vout_swap) {
			vout1_info = &G_voutinfo[0].video_info;
		} else {
			vout1_info = &G_voutinfo[1].video_info;
		}
		max_w = vout1_info->rotate ? vout1_info->height : vout1_info->width;
		max_h = vout1_info->rotate ? vout1_info->width : vout1_info->height;
		break;
	default:
		BUG();
		break;
	}
	dsp_cmd->preview_B_max_width = max_w;
	dsp_cmd->preview_B_max_height = max_h;

	/* Resize maximum of the fourth buffer */
	buffer = &iav->srcbuf[IAV_SRCBUF_PA];
	switch (buffer->type) {
	case IAV_SRCBUF_TYPE_OFF:
		max_w = max_h = 0;
		break;
	case IAV_SRCBUF_TYPE_ENCODE:
		max_win = &buffer->max;
		calc_roundup_size(max_win->width, max_win->height,
			IAV_STREAM_TYPE_H264, 0, &max_w, &max_h);
		break;
	case IAV_SRCBUF_TYPE_PREVIEW:
		if (config->vout_swap) {
			vout0_info = &G_voutinfo[1].video_info;
		} else {
			vout0_info = &G_voutinfo[0].video_info;
		}
		max_w = vout0_info->rotate ? vout0_info->height : vout0_info->width;
		max_h = vout0_info->rotate ? vout0_info->width : vout0_info->height;
		break;
	default:
		BUG();
		break;
	}
	dsp_cmd->preview_A_max_width = max_w;
	dsp_cmd->preview_A_max_height = max_h;

	dsp_cmd->stream_0_max_GOP_M = iav->stream[0].max_GOP_M;
	dsp_cmd->stream_1_max_GOP_M = iav->stream[1].max_GOP_M;
	dsp_cmd->stream_2_max_GOP_M = iav->stream[2].max_GOP_M;
	dsp_cmd->stream_3_max_GOP_M = iav->stream[3].max_GOP_M;

	/* Resize maximum of the streams */
	max_win = &iav->stream[0].max;
	calc_roundup_size(max_win->width, max_win->height,
		IAV_STREAM_TYPE_H264, 0, &max_w, &max_h);
	dsp_cmd->stream_0_max_width = max_w;
	dsp_cmd->stream_0_max_height = max_h;

	max_win = &iav->stream[1].max;
	calc_roundup_size(max_win->width, max_win->height,
		IAV_STREAM_TYPE_H264, 0, &max_w, &max_h);
	dsp_cmd->stream_1_max_width = max_w;
	dsp_cmd->stream_1_max_height = max_h;

	max_win = &iav->stream[2].max;
	calc_roundup_size(max_win->width, max_win->height,
		IAV_STREAM_TYPE_H264, 0, &max_w, &max_h);
	dsp_cmd->stream_2_max_width = max_w;
	dsp_cmd->stream_2_max_height = max_h;

	max_win = &iav->stream[3].max;
	calc_roundup_size(max_win->width, max_win->height,
		IAV_STREAM_TYPE_H264, 0, &max_w, &max_h);
	dsp_cmd->stream_3_max_width = max_w;
	dsp_cmd->stream_3_max_height = max_h;

	dsp_cmd->MCTF_possible = 1;
	dsp_cmd->max_num_streams = max_stream_num;
	dsp_cmd->max_num_cap_sources = 2;
	dsp_cmd->use_1Gb_DRAM_config = 0;
	dsp_cmd->raw_compression_disabled = config->raw_capture;
	dsp_cmd->extra_buf_main = iav->srcbuf[IAV_SRCBUF_MN].extra_dram_buf;
	dsp_cmd->extra_buf_preview_a = iav->srcbuf[IAV_SRCBUF_PA].extra_dram_buf;
	dsp_cmd->extra_buf_preview_b = iav->srcbuf[IAV_SRCBUF_PB].extra_dram_buf;
	dsp_cmd->extra_buf_preview_c = iav->srcbuf[IAV_SRCBUF_PC].extra_dram_buf;

	get_vin_win(iav, &vin_win, 1);
	dsp_cmd->raw_max_width = vin_win->width;
	dsp_cmd->raw_max_height = vin_win->height;

	dsp_cmd->enc_dummy_latency = config->enc_dummy_latency;

	dsp_cmd->stream_0_LT_enable = iav->stream[0].long_ref_enable;
	dsp_cmd->stream_1_LT_enable = iav->stream[1].long_ref_enable;
	dsp_cmd->stream_2_LT_enable = iav->stream[2].long_ref_enable;
	dsp_cmd->stream_3_LT_enable = iav->stream[3].long_ref_enable;

	/* Add 16 lines more to avoid roundup issue in dewarp mode.
		It's to fix the 16 lines alignment for each warp region.
	*/
	if (enc_mode == DSP_MULTI_REGION_WARP_MODE) {
		dsp_cmd->main_max_height += PIXEL_IN_MB;
		dsp_cmd->v_warped_main_max_height += PIXEL_IN_MB;
		dsp_cmd->warped_main_max_height += PIXEL_IN_MB;
		if (dsp_cmd->preview_C_max_height > 0) {
			dsp_cmd->preview_C_max_height += PIXEL_IN_MB;
		}
		if (dsp_cmd->preview_A_max_height > 0) {
			dsp_cmd->preview_A_max_height += PIXEL_IN_MB;
		}
	}

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_capture_buffer_default_setup(struct ambarella_iav *iav,
	struct amb_dsp_cmd *cmd, int id)
{
	ipcam_capture_preview_size_setup_t *dsp_cmd;
	struct iav_buffer *buffer = &iav->srcbuf[id];
	struct iav_buffer *main_buf = &iav->srcbuf[0];

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = IPCAM_VIDEO_CAPTURE_PREVIEW_SIZE_SETUP;
	dsp_cmd->preview_id = buffer->id_dsp;
	dsp_cmd->disabled = 0;
	dsp_cmd->cap_width = buffer->max.width;
	dsp_cmd->cap_height = buffer->max.height;
	dsp_cmd->output_scan_format = 0;
	dsp_cmd->deinterlace_mode = 0;
	dsp_cmd->input_win_width = main_buf->win.width;
	dsp_cmd->input_win_height = main_buf->win.height;
	dsp_cmd->input_win_offset_x = 0;
	dsp_cmd->input_win_offset_y = 0;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_capture_buffer_setup(struct ambarella_iav *iav,
	struct amb_dsp_cmd *cmd, int id)
{
	ipcam_capture_preview_size_setup_t *dsp_cmd;
	struct iav_buffer *buffer = &iav->srcbuf[id];

	if (is_invalid_win(&buffer->win) || is_invalid_input_win(&buffer->input))
		return -EINVAL;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = IPCAM_VIDEO_CAPTURE_PREVIEW_SIZE_SETUP;

	dsp_cmd->preview_id = buffer->id_dsp;
	if (buffer->win.width == 0 && buffer->win.height == 0) {
		dsp_cmd->disabled = 1;
		dsp_cmd->cap_width = buffer->max.width;
		dsp_cmd->cap_height = buffer->max.height;
	} else {
		dsp_cmd->disabled = 0;
		dsp_cmd->cap_width = buffer->win.width;
		dsp_cmd->cap_height = buffer->win.height;
	}
	dsp_cmd->output_scan_format = 0;
	dsp_cmd->deinterlace_mode = 0;
	dsp_cmd->input_win_width = buffer->input.width;
	dsp_cmd->input_win_height = buffer->input.height;
	dsp_cmd->input_win_offset_x = buffer->input.x;
	dsp_cmd->input_win_offset_y = buffer->input.y;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_encode_size_setup(struct iav_stream *stream, struct amb_dsp_cmd *cmd)
{
	ipcam_video_encode_size_setup_t *dsp_cmd;
	struct iav_stream_format *format;
	u16 width, height;
	s16 offset_y;

	format = &stream->format;

	dsp_cmd = get_cmd(stream->iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = GET_DSP_CMD_CODE(IPCAM_VIDEO_ENCODE_SIZE_SETUP,
		stream->format.id);

	if (stream->iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) {
		switch (format->buf_id) {
		case IAV_SRCBUF_MN:
			dsp_cmd->capture_source = DSP_CAPBUF_WARP;
			break;
		case IAV_SRCBUF_PMN:
			dsp_cmd->capture_source = DSP_CAPBUF_MN;
			break;
		default:
			dsp_cmd->capture_source = stream->srcbuf->id_dsp;
			break;
		}
	} else {
		dsp_cmd->capture_source = stream->srcbuf->id_dsp;
	}

	/* Use origin encode width, height, VIN info and encode type to
	 * calculate possible encode size alignment.
	 */
	get_round_encode_format(stream, &width, &height, &offset_y);
	dsp_cmd->enc_width = width;
	dsp_cmd->enc_height = height;
	dsp_cmd->enc_x = format->enc_win.x;
	dsp_cmd->enc_y = format->enc_win.y + offset_y;

	put_cmd(stream->iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_ENC, 0);

	return 0;
}

// stream: 0~ 3 for 4 streams (IPCAM ucode only)
int cmd_h264_encode_setup(struct iav_stream *stream, struct amb_dsp_cmd *cmd)
{
	h264_encode_setup_t *dsp_cmd;
	struct ambarella_iav *iav = stream->iav;
	struct iav_h264_config *h264_config;
	struct iav_vin_format *vin_format;
	struct iav_stream_format *stream_format;
	u32 encoder_frame_rate;

	dsp_cmd = get_cmd(stream->iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	h264_config = &stream->h264_config;
	stream_format = &stream->format;
	vin_format = &iav->vinc[0]->vin_format;

	dsp_cmd->cmd_code = GET_DSP_CMD_CODE(H264_ENCODING_SETUP,
		stream->format.id);

	dsp_cmd->mode = 1;
	dsp_cmd->M = h264_config->M;
	dsp_cmd->N = h264_config->N & 0xFF;
	dsp_cmd->N_msb = h264_config->N >> 8;

	dsp_cmd->idr_interval = h264_config->idr_interval;
	dsp_cmd->quality = calc_quality_model(h264_config);
	dsp_cmd->average_bitrate = h264_config->average_bitrate;
	dsp_cmd->vbr_setting = h264_config->vbr_setting;
	dsp_cmd->vbr_cntl = 0;

	dsp_cmd->hflip = stream_format->hflip;
	dsp_cmd->vflip = stream_format->vflip;
	dsp_cmd->rotate = stream_format->rotate_cw;
	dsp_cmd->chroma_format = h264_config->chroma_format;

	dsp_cmd->audio_in_freq = 2;	// todo,   48KHz

	dsp_cmd->vin_frame_rate = 0;
	dsp_cmd->vin_frame_rate_ext  = 0; //set to 0 temporarily
	dsp_cmd->session_id = 0;

	if (calc_frame_rate(iav, 0, 0, stream->fps.fps_multi, stream->fps.fps_div,
		&encoder_frame_rate, FRAME_RATE_SRC_IDSP_OUT) < 0) {
		iav_error("frame rate calculation for H.264 encode setup failed,"
			" frame rate 0x%x.\n", vin_format->frame_rate);
		return -EINVAL;
	}

	dsp_cmd->custom_encoder_frame_rate = encoder_frame_rate;

	dsp_cmd->frame_rate_division_factor = stream->fps.fps_div;
	dsp_cmd->frame_rate_multiplication_factor = stream->fps.fps_multi;
	dsp_cmd->force_intlc_tb_iframe = 0;

	dsp_cmd->cpb_buf_idc = h264_config->cpb_buf_idc;
	dsp_cmd->cpb_cmp_idc = h264_config->cpb_cmp_idc;
	dsp_cmd->en_panic_rc = h264_config->en_panic_rc;
	dsp_cmd->fast_rc_idc = h264_config->fast_rc_idc;
	dsp_cmd->cpb_user_size = h264_config->cpb_user_size;

	dsp_cmd->fast_seek_interval = h264_config->long_term_intvl;

	put_cmd(stream->iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_ENC, 0);

	return 0;
}

int cmd_h264_encode_start(struct iav_stream *stream, struct amb_dsp_cmd *cmd)
{
	h264_encode_t *dsp_cmd;
	struct iav_h264_config *h264_config;
	struct iav_vin_format *vin_format;
	int stream_height, round_factor, round_height, margin;

	u8 enc_mode = stream->iav->encode_mode;
	u8 lens_warp_enable = stream->iav->system_config[enc_mode].lens_warp;

	h264_config = &stream->h264_config;
	vin_format = &stream->iav->vinc[0]->vin_format;

	dsp_cmd = get_cmd(stream->iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = GET_DSP_CMD_CODE(H264_ENCODE, stream->format.id);
	dsp_cmd->start_encode_frame_no = 0xffffffff;
	dsp_cmd->encode_duration = (stream->format.duration ?
		stream->format.duration : ENCODE_DURATION_FOREVER);
	dsp_cmd->is_flush = 1;
	dsp_cmd->enable_slow_shutter = 0;
	dsp_cmd->res_rate_min = 40;

	dsp_cmd->en_loop_filter = 2;
	dsp_cmd->max_upsampling_rate = 1;
	dsp_cmd->slow_shutter_upsampling_rate = 0;
	dsp_cmd->au_type = h264_config->au_type;
	dsp_cmd->gaps_in_frame_num_value_allowed_flag =
		is_h264_frame_num_gap_allowed(stream);

	/* VUI */
	{
		dsp_cmd->vui_enable = 1;
		dsp_cmd->timing_info_present_flag = 1;
		dsp_cmd->fixed_frame_rate_flag = 1;
		dsp_cmd->pic_struct_present_flag = 1;
		dsp_cmd->nal_hrd_parameters_present_flag = 1;
		dsp_cmd->vcl_hrd_parameters_present_flag = 1;
		dsp_cmd->video_signal_type_present_flag = 1;
		dsp_cmd->video_full_range_flag = 1;
		dsp_cmd->video_format = 5;
	}

	/* color primaries */
	{
		u32 color_primaries = 1;
		u32 transfer_characteristics = 1;
		u32 matrix_coefficients = 1;
		get_colour_primaries(stream, &color_primaries,
			&transfer_characteristics, &matrix_coefficients);
		dsp_cmd->colour_description_present_flag = 1;
		dsp_cmd->colour_primaries = color_primaries;
		dsp_cmd->transfer_characteristics = transfer_characteristics;
		dsp_cmd->matrix_coefficients = matrix_coefficients;
	}

	/* crop pictures when needed for height alignment */
	{
		/* crop pictures when needed for height alignment
		 * hardware needs encoding height to be 16 aligned,
		 * for interlaced video, each field needs to be 16 aligned in height */
		stream_height = stream->format.enc_win.height;
		round_factor = is_vcap_interlaced(stream->iav, 0) ?
			(PIXEL_IN_MB << 1) : PIXEL_IN_MB;
		dsp_cmd->frame_cropping_flag = 1;
		dsp_cmd->frame_crop_left_offset = 0;
		dsp_cmd->frame_crop_right_offset = 0;
		dsp_cmd->frame_crop_top_offset = 0;
		round_height = ALIGN(stream_height, round_factor);
		margin = round_height - stream_height;
		if (is_vcap_interlaced(stream->iav, 0))
			margin >>= 2;
		else
			margin >>= 1;

		if (stream->format.rotate_cw) {
			/* Use minus height offset to compensate left cropping flag, so use
			 * right cropping flag all the time.
			 */
			dsp_cmd->frame_crop_right_offset = margin;
		} else {
			/* Use minus height offset to compensate top cropping flag, so
			 * use bottom cropping flag all the time.
			 */
			dsp_cmd->frame_crop_bottom_offset = margin;
		}
	}

	/* Calculate aspect ratio info in SPS */
	{
		u8 aspect_ratio_idc = 0;
		u16 sar_width = 1;
		u16 sar_height = 1;
		u8 is_warp_mode = ((enc_mode == DSP_MULTI_REGION_WARP_MODE) ||
			(enc_mode == DSP_ADVANCED_ISO_MODE && lens_warp_enable));
		u8 is_buf_warp;
		u16 temp;

		if (stream->srcbuf->id != IAV_SRCBUF_EFM) {
			is_buf_warp = (is_warp_mode && !stream->srcbuf->unwarp);
			if (is_buf_warp) {
				get_aspect_ratio_in_warp_mode(stream, &aspect_ratio_idc,
					&sar_width, &sar_height);
			} else {
				get_aspect_ratio(stream->srcbuf, &sar_width, &sar_height);
				if (stream->srcbuf->id != IAV_SRCBUF_MN) {
					get_aspect_ratio(
						is_warp_mode ?
							&stream->iav->srcbuf[IAV_SRCBUF_PMN] :
							stream->iav->main_buffer,
						&sar_width, &sar_height);
				}
			}
		}
		if (sar_width == sar_height) {
			aspect_ratio_idc = ENCODE_ASPECT_RATIO_1_1_SQUARE_PIXEL;
			sar_width = sar_height = 0;
		} else {
			aspect_ratio_idc = ENCODE_ASPECT_RATIO_CUSTOM;
			if (stream->format.rotate_cw) {
				temp = sar_width;
				sar_width = sar_height;
				sar_height = temp;
			}
		}
		dsp_cmd->aspect_ratio_idc = aspect_ratio_idc;
		dsp_cmd->SAR_width = sar_width;
		dsp_cmd->SAR_height = sar_height;
		dsp_cmd->aspect_ratio_info_present_flag = 1;
		switch (h264_config->profile) {
		case H264_PROFILE_HIGH:
			dsp_cmd->profile = 1;
			break;
		case H264_PROFILE_MAIN:
			dsp_cmd->profile = 0;
			break;
		case H264_PROFILE_BASELINE:
			dsp_cmd->profile = 3;
			break;
		default:
			BUG();
			break;
		}
	}

	put_cmd(stream->iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_ENC, 0);

	return 0;
}

int cmd_encode_stop(struct iav_stream *stream, struct amb_dsp_cmd *cmd)
{
	h264_encode_stop_t *dsp_cmd;

	dsp_cmd = get_cmd(stream->iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = GET_DSP_CMD_CODE(ENCODING_STOP, stream->format.id);
	// Stop on next P or I picture after stop command has been received.
	dsp_cmd->stop_method = H264_STOP_ON_NEXT_IP;

	put_cmd(stream->iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_ENC, 0);

	return 0;
}

//stream 0 ~ 3 multi stream
int cmd_jpeg_encode_setup(struct iav_stream *stream, struct amb_dsp_cmd *cmd)
{
	struct iav_mjpeg_config *mjpeg_config = &stream->mjpeg_config;
	struct ambarella_iav *iav = stream->iav;
	struct iav_vin_format *vin_format;
	struct iav_stream_format *stream_format;
	jpeg_encode_setup_t *dsp_cmd;
	u32 encoder_frame_rate, offset;

	mjpeg_config = &stream->mjpeg_config;
	stream_format = &stream->format;
	vin_format = &stream->iav->vinc[0]->vin_format;

	dsp_cmd = get_cmd(stream->iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = GET_DSP_CMD_CODE(JPEG_ENCODING_SETUP,
		stream->format.id);

	dsp_cmd->chroma_format = mjpeg_config->chroma_format;
	dsp_cmd->is_mjpeg = 1;

	init_stream_jpeg_dqt(iav, stream->format.id, mjpeg_config->quality);
	offset = (u32)mjpeg_config->jpeg_quant_matrix -
		iav->mmap[IAV_BUFFER_QUANT].virt;
	dsp_cmd->quant_matrix_addr =
		(u32*)PHYS_TO_DSP(iav->mmap[IAV_BUFFER_QUANT].phys + offset);

	if (calc_frame_rate(iav,
		0, 0, stream->fps.fps_multi, stream->fps.fps_div, &encoder_frame_rate,
		FRAME_RATE_SRC_IDSP_OUT) < 0) {
		iav_error("frame rate calculation for jpeg encode setup failed,"
			" frame rate 0x%x.\n", vin_format->frame_rate);
		return -EINVAL;
	}
	dsp_cmd->frame_rate = encoder_frame_rate;   //29.97 progressive
	dsp_cmd->frame_rate_division_factor = stream->fps.fps_div;
	dsp_cmd->frame_rate_multiplication_factor = stream->fps.fps_multi;
	dsp_cmd->session_id = 0;
	dsp_cmd->hflip = stream_format->hflip;
	dsp_cmd->vflip = stream_format->vflip;
	dsp_cmd->rotate = stream_format->rotate_cw;

	put_cmd(stream->iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_ENC, 0);

	return 0;
}

int cmd_jpeg_encode_start(struct iav_stream *stream, struct amb_dsp_cmd *cmd)
{
	mjpeg_capture_t *dsp_cmd;

	dsp_cmd = get_cmd(stream->iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = GET_DSP_CMD_CODE(MJPEG_ENCODE, stream->format.id);
	dsp_cmd->start_encode_frame_no = -1;
	dsp_cmd->encode_duration = (stream->format.duration ?
		stream->format.duration : ENCODE_DURATION_FOREVER);

	put_cmd(stream->iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_ENC, 0);

	return 0;
}

// 0x6004 command
int cmd_update_encode_params(struct iav_stream *stream,
		struct amb_dsp_cmd *cmd, u32 flags)
{
	ipcam_real_time_encode_param_setup_t *dsp_cmd;
	struct ambarella_iav *iav = stream->iav;
	struct iav_h264_config *h264_config;
	struct iav_mjpeg_config *mjpeg_config;
	struct iav_vin_format *vin_format;
	u32 dsp_vin_fps, dsp_encoder_fps, i, offset;
	u32 idsp_out_frame_rate = 0;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	h264_config = &stream->h264_config;
	mjpeg_config = &stream->mjpeg_config;
	vin_format = &iav->vinc[0]->vin_format;

	dsp_cmd->cmd_code = GET_DSP_CMD_CODE(
		IPCAM_REAL_TIME_ENCODE_PARAM_SETUP, stream->format.id);

	if (flags & REALTIME_PARAM_CBR_MODIFY_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_CBR_MODIFY_BIT;
		dsp_cmd->cbr_modify = get_dsp_encode_bitrate(stream);
	}
	if (flags & REALTIME_PARAM_CUSTOM_FRAME_RATE_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_CUSTOM_FRAME_RATE_BIT;
		if (calc_frame_rate(iav,
			0, 0, stream->fps.fps_multi, stream->fps.fps_div,
			&dsp_encoder_fps, FRAME_RATE_SRC_IDSP_OUT) < 0) {
			iav_error("frame rate calculation for realtime encode param"
				" update failed, frame rate 0x%x.\n",
				vin_format->frame_rate);
			return -EINVAL;
		}
		dsp_cmd->custom_encoder_frame_rate = dsp_encoder_fps;
		dsp_cmd->frame_rate_division_factor = stream->fps.fps_div;
		dsp_cmd->frame_rate_multiplication_factor = stream->fps.fps_multi;
	}
	if (flags & REALTIME_PARAM_QP_LIMIT_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_QP_LIMIT_BIT;
		dsp_cmd->qp_max_on_I = h264_config->qp_max_on_I;
		dsp_cmd->qp_max_on_P = h264_config->qp_max_on_P;
		dsp_cmd->qp_max_on_B = h264_config->qp_max_on_B;
		dsp_cmd->qp_min_on_I = h264_config->qp_min_on_I;
		dsp_cmd->qp_min_on_P = h264_config->qp_min_on_P;
		dsp_cmd->qp_min_on_B = h264_config->qp_min_on_B;
		dsp_cmd->aqp = h264_config->adapt_qp;
		dsp_cmd->i_qp_reduce = h264_config->i_qp_reduce;
		dsp_cmd->p_qp_reduce = h264_config->p_qp_reduce;
		dsp_cmd->skip_flags = h264_config->skip_flag;
	}
	if (flags & REALTIME_PARAM_GOP_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_GOP_BIT;
		dsp_cmd->N = h264_config->N & 0xFF;
		dsp_cmd->N_msb = h264_config->N >> 8;
		dsp_cmd->idr_interval = h264_config->idr_interval;
	}
	if (flags & REALTIME_PARAM_CUSTOM_VIN_FPS_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_CUSTOM_VIN_FPS_BIT;
		if (calc_frame_rate(iav,
			0, 0, 1, 1, &dsp_vin_fps, FRAME_RATE_SRC_VIN) < 0) {
			iav_error("frame rate calculation for realtime encode param"
				"update failed, frame rate 0x%x.\n",
				vin_format->frame_rate);
			return -EINVAL;
		}
		dsp_cmd->custom_vin_frame_rate = dsp_vin_fps;
		if (iav_vin_get_idsp_frame_rate(iav, &idsp_out_frame_rate) < 0) {
			return -EINVAL;
		}
		dsp_cmd->idsp_frame_rate_M = (u8)DIV_ROUND_CLOSEST(FPS_Q9_BASE,
			idsp_out_frame_rate);
		dsp_cmd->idsp_frame_rate_N = (u8)DIV_ROUND_CLOSEST(FPS_Q9_BASE,
			iav->vinc[0]->vin_format.frame_rate);
		if (check_idsp_upsample_factor(dsp_cmd->idsp_frame_rate_M,
			dsp_cmd->idsp_frame_rate_N) < 0) {
			iav_error("IDSP multiplication_factor can not be smaller than division_factor!\n");
			return -EINVAL;
		}
	}
	if (flags & REALTIME_PARAM_INTRA_MB_ROWS_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_INTRA_MB_ROWS_BIT;
		dsp_cmd->intra_refresh_num_mb_row = 0;
	}
	if (flags & REALTIME_PARAM_PREVIEW_A_FRAME_RATE_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_PREVIEW_A_FRAME_RATE_BIT;
		dsp_cmd->preview_A_frame_rate_divison_factor = 1;
	}
	if (flags & REALTIME_PARAM_QP_ROI_MATRIX_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_QP_ROI_MATRIX_BIT;
		if (h264_config->qp_roi_enable) {
			dsp_cmd->roi_daddr = PHYS_TO_DSP(
				iav->mmap[IAV_BUFFER_QPMATRIX].phys +
				stream->format.id * STREAM_QP_MATRIX_SIZE);
#ifdef CONFIG_AMBARELLA_IAV_QP_OFFSET_IPB
			dsp_cmd->roi_daddr_p = dsp_cmd->roi_daddr + SINGLE_QP_MATRIX_SIZE;
			dsp_cmd->roi_daddr_b = dsp_cmd->roi_daddr_p + SINGLE_QP_MATRIX_SIZE;
#else
			dsp_cmd->roi_daddr_p = dsp_cmd->roi_daddr;
			dsp_cmd->roi_daddr_b = dsp_cmd->roi_daddr;
#endif
		} else {
			dsp_cmd->roi_daddr = 0;
			dsp_cmd->roi_daddr_p = 0;
			dsp_cmd->roi_daddr_b = 0;
		}

		for (i = 0; i < NUM_PIC_TYPES; ++i) {
			dsp_cmd->roi_delta[i][0] = h264_config->qp_delta[i][0];
			dsp_cmd->roi_delta[i][1] = h264_config->qp_delta[i][1];
			dsp_cmd->roi_delta[i][2] = h264_config->qp_delta[i][2];
			dsp_cmd->roi_delta[i][3] = h264_config->qp_delta[i][3];
		}
		dsp_cmd->user1_intra_bias = h264_config->user1_intra_bias;
		dsp_cmd->user1_direct_bias = h264_config->user1_direct_bias;
		dsp_cmd->user2_intra_bias = h264_config->user2_intra_bias;
		dsp_cmd->user2_direct_bias = h264_config->user2_direct_bias;
		dsp_cmd->user3_intra_bias = h264_config->user3_intra_bias;
		dsp_cmd->user3_direct_bias = h264_config->user3_direct_bias;
	}
	if (flags & REALTIME_PARAM_PANIC_MODE_BIT) {
		BUG();
	}
	if (flags & REALTIME_PARAM_QUANT_MATRIX_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_QUANT_MATRIX_BIT;
		init_stream_jpeg_dqt(iav, stream->format.id,
			mjpeg_config->quality);
		offset = (u32)mjpeg_config->jpeg_quant_matrix -
			iav->mmap[IAV_BUFFER_QUANT].virt;
		dsp_cmd->quant_matrix_addr =
			PHYS_TO_DSP(iav->mmap[IAV_BUFFER_QUANT].phys + offset);
	}
	if (flags & REALTIME_PARAM_MONOCHROME_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_MONOCHROME_BIT;
		if (stream->format.type == IAV_STREAM_TYPE_H264) {
			dsp_cmd->is_monochrome = h264_config->chroma_format;
		} else {
			dsp_cmd->is_monochrome =
				(mjpeg_config->chroma_format == JPEG_CHROMA_MONO);
		}
	}
	if (flags & REALTIME_PARAM_INTRA_BIAS_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_INTRA_BIAS_BIT;
		dsp_cmd->P_IntraBiasAdd = h264_config->intrabias_p;
		dsp_cmd->B_IntraBiasAdd = h264_config->intrabias_b;
	}
	if (flags & REALTIME_PARAM_FORCE_IDR_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_FORCE_IDR_BIT;
	}
	if (flags & REALTIME_PARAM_ZMV_THRESHOLD_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_ZMV_THRESHOLD_BIT;
		dsp_cmd->zmv_threshold = h264_config->zmv_threshold;
	}
	if (flags & REALTIME_PARAM_FLAT_AREA_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_FLAT_AREA_BIT;
		dsp_cmd->flat_area_improvement_on = h264_config->enc_improve;
	}
	if (flags & REALTIME_PARAM_FORCE_FAST_SEEK_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_FORCE_FAST_SEEK_BIT;
	}
	if (flags & REALTIME_PARAM_FRAME_DROP_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_FRAME_DROP_BIT;
		dsp_cmd->drop_frame = h264_config->drop_frames;
	}

	put_cmd(stream->iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_ENC, 0);

	return 0;
}

// 0x6004 command
int cmd_update_idsp_factor(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd)
{
	ipcam_real_time_encode_param_setup_t *dsp_cmd;
	u32 dsp_vin_fps = 0;
	u32 idsp_out_frame_rate = 0;
	int rval = 0;

	mutex_lock(&iav->iav_mutex);

	do {
		if (iav->state != IAV_STATE_PREVIEW && iav->state != IAV_STATE_ENCODING) {
			break;
		}

		dsp_cmd = get_cmd(iav->dsp, cmd);
		if (!dsp_cmd) {
			rval = -ENOMEM;
			break;
		}
		if (calc_frame_rate(iav,
			0, 0, 1, 1, &dsp_vin_fps, FRAME_RATE_SRC_VIN) < 0) {
			iav_error("frame rate calculation for realtime encode param"
				"update failed, frame rate 0x%x.\n", dsp_vin_fps);
			rval = -EINVAL;
			break;
		}
		if (iav_vin_get_idsp_frame_rate(iav, &idsp_out_frame_rate) < 0) {
			rval = -EINVAL;
			break;
		}

		dsp_cmd->cmd_code = IPCAM_REAL_TIME_ENCODE_PARAM_SETUP;
		dsp_cmd->enable_flags |= REALTIME_PARAM_CUSTOM_VIN_FPS_BIT;
		dsp_cmd->custom_vin_frame_rate = dsp_vin_fps;
		dsp_cmd->idsp_frame_rate_M = (u8)DIV_ROUND_CLOSEST(FPS_Q9_BASE,
			idsp_out_frame_rate);
		dsp_cmd->idsp_frame_rate_N = (u8)DIV_ROUND_CLOSEST(FPS_Q9_BASE,
			iav->vinc[0]->vin_format.frame_rate);

		if (check_idsp_upsample_factor(dsp_cmd->idsp_frame_rate_M,
			dsp_cmd->idsp_frame_rate_N) < 0) {
			iav_error("IDSP multiplication_factor can not be smaller"
				" than division_factor!\n");
			rval = -EINVAL;
			break;
		}

		put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_ENC, 0);
	} while (0);

	mutex_unlock(&iav->iav_mutex);

	return rval;
}

int update_sync_encode_params(struct iav_stream *stream,
	struct amb_dsp_cmd *cmd, u32 flags)
{
	ipcam_real_time_encode_param_setup_t *dsp_cmd;
	struct ambarella_iav *iav = stream->iav;
	struct iav_h264_config *h264_config;
	struct iav_mjpeg_config *mjpeg_config;
	struct iav_vin_format *vin_format;
	u32 dsp_vin_fps, dsp_encoder_fps, offset;
	u8 *addr;
	int i, qpm_idx;
	u32 idsp_out_frame_rate = 0;

	addr = (u8 *)iav->mmap[IAV_BUFFER_CMD_SYNC].virt +
		iav->cmd_sync_idx * CMD_SYNC_SIZE +
		stream->format.id * DSP_ENC_CMD_SIZE;
	dsp_cmd = (ipcam_real_time_encode_param_setup_t *)addr;

	h264_config = &stream->h264_config;
	mjpeg_config = &stream->mjpeg_config;
	vin_format = &iav->vinc[0]->vin_format;

	dsp_cmd->cmd_code = GET_DSP_CMD_CODE(
		IPCAM_REAL_TIME_ENCODE_PARAM_SETUP, stream->format.id);

	if (flags & REALTIME_PARAM_CBR_MODIFY_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_CBR_MODIFY_BIT;
		dsp_cmd->cbr_modify = get_dsp_encode_bitrate(stream);
	}
	if (flags & REALTIME_PARAM_CUSTOM_FRAME_RATE_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_CUSTOM_FRAME_RATE_BIT;
		if (calc_frame_rate(iav,
			0, 0, stream->fps.fps_multi, stream->fps.fps_div,
			&dsp_encoder_fps, FRAME_RATE_SRC_IDSP_OUT) < 0) {
			iav_error("frame rate calculation for realtime encode param"
				" update failed, frame rate 0x%x.\n",
				vin_format->frame_rate);
			return -EINVAL;
		}
		dsp_cmd->custom_encoder_frame_rate = dsp_encoder_fps;
		dsp_cmd->frame_rate_division_factor = stream->fps.fps_div;
		dsp_cmd->frame_rate_multiplication_factor = stream->fps.fps_multi;
	}
	if (flags & REALTIME_PARAM_QP_LIMIT_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_QP_LIMIT_BIT;
		dsp_cmd->qp_max_on_I = h264_config->qp_max_on_I;
		dsp_cmd->qp_max_on_P = h264_config->qp_max_on_P;
		dsp_cmd->qp_max_on_B = h264_config->qp_max_on_B;
		dsp_cmd->qp_min_on_I = h264_config->qp_min_on_I;
		dsp_cmd->qp_min_on_P = h264_config->qp_min_on_P;
		dsp_cmd->qp_min_on_B = h264_config->qp_min_on_B;
		dsp_cmd->aqp = h264_config->adapt_qp;
		dsp_cmd->i_qp_reduce = h264_config->i_qp_reduce;
		dsp_cmd->p_qp_reduce = h264_config->p_qp_reduce;
		dsp_cmd->skip_flags = h264_config->skip_flag;
	}
	if (flags & REALTIME_PARAM_GOP_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_GOP_BIT;
		dsp_cmd->N = h264_config->N & 0xFF;
		dsp_cmd->N_msb = h264_config->N >> 8;
		dsp_cmd->idr_interval = h264_config->idr_interval;
	}
	if (flags & REALTIME_PARAM_CUSTOM_VIN_FPS_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_CUSTOM_VIN_FPS_BIT;
		if (calc_frame_rate(iav,
			0, 0, 1, 1, &dsp_vin_fps, FRAME_RATE_SRC_VIN) < 0) {
			iav_error("frame rate calculation for realtime encode param"
				"update failed, frame rate 0x%x.\n",
				vin_format->frame_rate);
			return -EINVAL;
		}
		dsp_cmd->custom_vin_frame_rate = dsp_vin_fps;
		if (iav_vin_get_idsp_frame_rate(iav, &idsp_out_frame_rate) < 0) {
			return -EINVAL;
		}
		dsp_cmd->idsp_frame_rate_M = (u8)DIV_ROUND_CLOSEST(FPS_Q9_BASE,
			idsp_out_frame_rate);
		dsp_cmd->idsp_frame_rate_N = (u8)DIV_ROUND_CLOSEST(FPS_Q9_BASE,
			iav->vinc[0]->vin_format.frame_rate);
		if (check_idsp_upsample_factor(dsp_cmd->idsp_frame_rate_M,
			dsp_cmd->idsp_frame_rate_N) < 0) {
			iav_error("IDSP multiplication_factor can not be smaller than division_factor!\n");
			return -EINVAL;
		}
	}
	if (flags & REALTIME_PARAM_INTRA_MB_ROWS_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_INTRA_MB_ROWS_BIT;
		dsp_cmd->intra_refresh_num_mb_row = 0;
	}
	if (flags & REALTIME_PARAM_PREVIEW_A_FRAME_RATE_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_PREVIEW_A_FRAME_RATE_BIT;
		dsp_cmd->preview_A_frame_rate_divison_factor = 1;
	}
	if (flags & REALTIME_PARAM_QP_ROI_MATRIX_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_QP_ROI_MATRIX_BIT;
		if (h264_config->qp_roi_enable) {
			/* use previous qp matrix if not updated */
			if (!iav->cmd_sync_qpm_flag) {
				qpm_idx = (iav->cmd_sync_qpm_idx + ENC_CMD_TOGGLED_NUM - 2) %
					ENC_CMD_TOGGLED_NUM + 1;
			} else {
				qpm_idx = iav->cmd_sync_qpm_idx;
			}
			dsp_cmd->roi_daddr = PHYS_TO_DSP(
				iav->mmap[IAV_BUFFER_QPMATRIX].phys +
				qpm_idx * QP_MATRIX_SIZE +
				stream->format.id * STREAM_QP_MATRIX_SIZE);
#ifdef CONFIG_AMBARELLA_IAV_QP_OFFSET_IPB
			dsp_cmd->roi_daddr_p = dsp_cmd->roi_daddr + SINGLE_QP_MATRIX_SIZE;
			dsp_cmd->roi_daddr_b = dsp_cmd->roi_daddr_p + SINGLE_QP_MATRIX_SIZE;
#else
			dsp_cmd->roi_daddr_p = dsp_cmd->roi_daddr;
			dsp_cmd->roi_daddr_b = dsp_cmd->roi_daddr;
#endif
		} else {
			dsp_cmd->roi_daddr = 0;
			dsp_cmd->roi_daddr_p = 0;
			dsp_cmd->roi_daddr_b = 0;
		}
		for (i = 0; i < NUM_PIC_TYPES; ++i) {
			dsp_cmd->roi_delta[i][0] = h264_config->qp_delta[i][0];
			dsp_cmd->roi_delta[i][1] = h264_config->qp_delta[i][1];
			dsp_cmd->roi_delta[i][2] = h264_config->qp_delta[i][2];
			dsp_cmd->roi_delta[i][3] = h264_config->qp_delta[i][3];
		}
		dsp_cmd->user1_intra_bias = h264_config->user1_intra_bias;
		dsp_cmd->user1_direct_bias = h264_config->user1_direct_bias;
		dsp_cmd->user2_intra_bias = h264_config->user2_intra_bias;
		dsp_cmd->user2_direct_bias = h264_config->user2_direct_bias;
		dsp_cmd->user3_intra_bias = h264_config->user3_intra_bias;
		dsp_cmd->user3_direct_bias = h264_config->user3_direct_bias;
	}
	if (flags & REALTIME_PARAM_PANIC_MODE_BIT) {
		BUG();
	}
	if (flags & REALTIME_PARAM_QUANT_MATRIX_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_QUANT_MATRIX_BIT;
		init_stream_jpeg_dqt(iav, stream->format.id,
			mjpeg_config->quality);
		offset = (u32)mjpeg_config->jpeg_quant_matrix -
			iav->mmap[IAV_BUFFER_QUANT].virt;
		dsp_cmd->quant_matrix_addr =
			PHYS_TO_DSP(iav->mmap[IAV_BUFFER_QUANT].phys + offset);
	}
	if (flags & REALTIME_PARAM_MONOCHROME_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_MONOCHROME_BIT;
		if (stream->format.type == IAV_STREAM_TYPE_H264) {
			dsp_cmd->is_monochrome = h264_config->chroma_format;
		} else {
			dsp_cmd->is_monochrome =
				(mjpeg_config->chroma_format == JPEG_CHROMA_MONO);
		}
	}
	if (flags & REALTIME_PARAM_INTRA_BIAS_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_INTRA_BIAS_BIT;
		dsp_cmd->P_IntraBiasAdd = h264_config->intrabias_p;
		dsp_cmd->B_IntraBiasAdd = h264_config->intrabias_b;
	}
	if (flags & REALTIME_PARAM_FORCE_IDR_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_FORCE_IDR_BIT;
	}
	if (flags & REALTIME_PARAM_ZMV_THRESHOLD_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_ZMV_THRESHOLD_BIT;
		dsp_cmd->zmv_threshold = h264_config->zmv_threshold;
	}
	if (flags & REALTIME_PARAM_FLAT_AREA_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_FLAT_AREA_BIT;
		dsp_cmd->flat_area_improvement_on = h264_config->enc_improve;
	}
	if (flags & REALTIME_PARAM_FORCE_FAST_SEEK_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_FORCE_FAST_SEEK_BIT;
	}
	if (flags & REALTIME_PARAM_FRAME_DROP_BIT) {
		dsp_cmd->enable_flags |= REALTIME_PARAM_FRAME_DROP_BIT;
		dsp_cmd->drop_frame = h264_config->drop_frames;
	}

	return 0;
}

// 0x6007 command
int cmd_overlay_insert(struct iav_stream *stream, struct amb_dsp_cmd *cmd,
	struct iav_overlay_insert *info)
{
	struct iav_overlay_area area;
	ipcam_osd_insert_t *dsp_cmd;
	unsigned long phys_addr = 0;
	int i, active_areas, area_index = 0;
	u16 width, height;
	s16 offset_y;

	dsp_cmd = get_cmd(stream->iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = GET_DSP_CMD_CODE(IPCAM_OSD_INSERT, stream->format.id);

	if (info->enable) {
		if (stream->iav->osd_from_mixer_a)
			dsp_cmd->vout_id = 0;
		else if (stream->iav->osd_from_mixer_b)
			dsp_cmd->vout_id = 1;
		else {
			iav_error("OSD mixer should be configured to support overlay!\n");
			return -EINVAL;
		}

		get_round_encode_format(stream, &width, &height, &offset_y);
		phys_addr = stream->iav->mmap[IAV_BUFFER_OVERLAY].phys;
		for (i = 0, active_areas = 0; i < MAX_NUM_OVERLAY_AREA; ++i) {
			area = info->area[i];
			if (area.enable) {
				if (active_areas < MAX_NUM_OSD_AERA_NO_EXTENSION) {
					dsp_cmd->osd_clut_dram_address[active_areas] =
						PHYS_TO_DSP(phys_addr + area.clut_addr_offset);
					dsp_cmd->osd_buf_dram_address[active_areas] =
						PHYS_TO_DSP(phys_addr + area.data_addr_offset);
					dsp_cmd->osd_buf_pitch[active_areas] = area.pitch;
					dsp_cmd->osd_win_w[active_areas] = area.width;
					dsp_cmd->osd_win_h[active_areas] = area.height;
					dsp_cmd->osd_win_offset_x[active_areas] = area.start_x;
					dsp_cmd->osd_win_offset_y[active_areas] =
						area.start_y - offset_y;
					dsp_cmd->osd_enable = 1;
				} else {
					area_index = active_areas - MAX_NUM_OSD_AERA_NO_EXTENSION;
					dsp_cmd->osd_clut_dram_address_ex[area_index] =
						PHYS_TO_DSP(phys_addr + area.clut_addr_offset);
					dsp_cmd->osd_buf_dram_address_ex[area_index] =
						PHYS_TO_DSP(phys_addr + area.data_addr_offset);
					dsp_cmd->osd_buf_pitch_ex[area_index] = area.pitch;
					dsp_cmd->osd_win_w_ex[area_index] = area.width;
					dsp_cmd->osd_win_h_ex[area_index] = area.height;
					dsp_cmd->osd_win_offset_x_ex[area_index] = area.start_x;
					dsp_cmd->osd_win_offset_y_ex[area_index] =
						area.start_y - offset_y;
					dsp_cmd->osd_enable_ex = 1;
				}
				++active_areas;
			}
		}
		if (active_areas <= MAX_NUM_OSD_AERA_NO_EXTENSION)
			dsp_cmd->osd_num_regions = active_areas;
		else {
			dsp_cmd->osd_num_regions = MAX_NUM_OSD_AERA_NO_EXTENSION;
			dsp_cmd->osd_num_regions_ex = active_areas -
				MAX_NUM_OSD_AERA_NO_EXTENSION;
		}
	}

	put_cmd(stream->iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_ENC, 0);

	return 0;
}

int cmd_set_pm_mctf(struct ambarella_iav *iav,struct amb_dsp_cmd *cmd,
	u32 delay)
{
	ipcam_set_privacy_mask_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = IPCAM_SET_PRIVACY_MASK;

	dsp_cmd->enabled_flags_dram_address = iav->mmap[IAV_BUFFER_PM_MCTF].phys
		+ iav->pm.data_addr_offset;
	dsp_cmd->Y = iav->pm.y;
	dsp_cmd->U = iav->pm.u;
	dsp_cmd->V = iav->pm.v;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, delay);

	return 0;
}

// 0x6009 command
int cmd_fastosd_insert(struct iav_stream *stream, struct amb_dsp_cmd *cmd,
	struct iav_fastosd_insert *info)
{
	struct iav_fastosd_string_area* string_area;
	struct iav_fastosd_osd_area* osd_area;
	ipcam_fast_osd_insert_t *dsp_cmd;
	unsigned long phys_addr = 0;
	int i, active_areas;

	dsp_cmd = get_cmd(stream->iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = GET_DSP_CMD_CODE(IPCAM_FAST_OSD_INSERT,
		stream->format.id);

	if (info->enable) {
		dsp_cmd->fast_osd_enable = 1;

		if (stream->iav->osd_from_mixer_a)
			dsp_cmd->vout_id = 0;
		else if (stream->iav->osd_from_mixer_b)
			dsp_cmd->vout_id = 1;
		else {
			iav_error("OSD mixer should be configured to support overlay!\n");
			return -EINVAL;
		}

		phys_addr = stream->iav->mmap[IAV_BUFFER_OVERLAY].phys;
		dsp_cmd->string_num_region = info->string_num_region;
		dsp_cmd->osd_num_region = info->osd_num_region;
		dsp_cmd->font_index_addr = PHYS_TO_DSP(phys_addr + info->font_index_offset);
		dsp_cmd->font_map_addr = PHYS_TO_DSP(phys_addr + info->font_map_offset);
		dsp_cmd->font_map_pitch = info->font_map_pitch;
		dsp_cmd->font_map_h = info->font_map_height;

		for (i = 0, active_areas = 0; i < dsp_cmd->string_num_region; ++i) {
			string_area = &info->string_area[i];
			if (string_area->enable) {
				dsp_cmd->clut_addr[active_areas] =
					PHYS_TO_DSP(phys_addr + string_area->clut_offset);
				dsp_cmd->string_output_addr[active_areas] =
					PHYS_TO_DSP(phys_addr + string_area->string_output_offset);
				dsp_cmd->string_addr[active_areas] =
					PHYS_TO_DSP(phys_addr + string_area->string_offset);
				dsp_cmd->string_len[active_areas] = string_area->string_length;
				dsp_cmd->string_output_pitch[active_areas] = string_area->string_output_pitch;
				dsp_cmd->string_win_offset_x[active_areas] = string_area->offset_x;
				dsp_cmd->string_win_offset_y[active_areas] = string_area->offset_y;
				++ active_areas;
			}
		}

		for (i = 0, active_areas = 0; i < dsp_cmd->string_num_region; ++i) {
			osd_area = &info->osd_area[i];
			if (osd_area->enable) {
				dsp_cmd->osd_clut_addr[active_areas] =
					PHYS_TO_DSP(phys_addr + osd_area->clut_offset);
				dsp_cmd->osd_buf_addr[active_areas] =
					PHYS_TO_DSP(phys_addr + osd_area->buffer_offset);
				dsp_cmd->osd_buf_pitch[active_areas] = osd_area->osd_buf_pitch;
				dsp_cmd->osd_win_offset_x[active_areas] = osd_area->offset_x;
				dsp_cmd->osd_win_offset_y[active_areas] = osd_area->offset_y;
				dsp_cmd->osd_win_w[active_areas] = osd_area->win_width;
				dsp_cmd->osd_win_h[active_areas] = osd_area->win_height;
				++ active_areas;
			}
		}

	}

	put_cmd(stream->iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_ENC, 0);

	return 0;
}

int cmd_dsp_set_debug_level(struct ambarella_iav *iav,
	struct amb_dsp_cmd *cmd, u32 module, u32 debug_level, u32 mask)
{
	dsp_debug_level_setup_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = DSP_DEBUG_LEVEL_SETUP;
	dsp_cmd->module = module;
	dsp_cmd->debug_level = debug_level;
	dsp_cmd->coding_thread_printf_disable_mask = mask;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_set_pm_bpc(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd)
{
	fixed_pattern_noise_correct_t *dsp_cmd;
	struct iav_rect *vin_win = NULL;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	get_pm_vin_win(iav, &vin_win);

	dsp_cmd->cmd_code = FIXED_PATTERN_NOISE_CORRECTION;
	dsp_cmd->fpn_pixel_mode = 3;
	dsp_cmd->num_of_cols = vin_win->width;
	dsp_cmd->fpn_pitch =  ALIGN(dsp_cmd->num_of_cols / 8, 32);
	dsp_cmd->fpn_pixels_buf_size = dsp_cmd->fpn_pitch * vin_win->height;
	dsp_cmd->bad_pixel_a5m_mode = 1;
	dsp_cmd->fpn_pixels_addr = PHYS_TO_DSP(iav->mmap[IAV_BUFFER_PM_BPC].phys +
			PM_BPC_PARTITION_SIZE * (iav->curr_pm_index));

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_bpc_setup(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd,
	struct iav_bpc_setup *bpc_setup)
{
	bad_pixel_correct_setup_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	if (bpc_setup) {
		memcpy(dsp_cmd, bpc_setup, sizeof(bad_pixel_correct_setup_t));
		dsp_cmd->correction_mode |= iav->pm_bpc_enable;
	} else {
		dsp_cmd->correction_mode = 3; //both static and dynamic
		dsp_cmd->hot_shift0_4 = 13449;
		dsp_cmd->hot_shift5 = 4;
		dsp_cmd->dark_shift0_4 = 13449;
		dsp_cmd->dark_shift5 = 4;
		dsp_cmd->dynamic_bad_pixel_detection_mode = 4;
		dsp_cmd->dynamic_bad_pixel_correction_method = 0;
	}

	dsp_cmd->cmd_code = BAD_PIXEL_CORRECT_SETUP;
	dsp_cmd->hot_pixel_thresh_addr = PHYS_TO_DSP(iav->mmap[IAV_BUFFER_BPC].phys);
	dsp_cmd->dark_pixel_thresh_addr = PHYS_TO_DSP(iav->mmap[IAV_BUFFER_BPC].phys +
		BPC_TABLE_SIZE);

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_static_bpc(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd,
	struct iav_bpc_fpn *bpc_fpn)
{
	fixed_pattern_noise_correct_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	memcpy(dsp_cmd, bpc_fpn, sizeof(fixed_pattern_noise_correct_t));

	dsp_cmd->cmd_code = FIXED_PATTERN_NOISE_CORRECTION;
	dsp_cmd->fpn_pixels_addr = PHYS_TO_DSP(iav->mmap[IAV_BUFFER_PM_BPC].phys +
		PM_BPC_PARTITION_SIZE * (iav->curr_pm_index));
	dsp_cmd->intercepts_and_slopes_addr = PHYS_TO_DSP(
		iav->mmap[IAV_BUFFER_BPC].phys +
		BPC_TABLE_SIZE * 2);
	dsp_cmd->intercept_shift = 3;
	dsp_cmd->row_gain_enable = 0;
	dsp_cmd->row_gain_addr = 0;
	dsp_cmd->column_gain_enable = 0;//mw_cmd.column_gain_enable;
	dsp_cmd->column_gain_addr = 0;//VIRT_TO_DSP(column_offset);
	dsp_cmd->bad_pixel_a5m_mode = 1;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

static void debug_print_warp_region(struct ambarella_iav *iav,
	ipcam_set_region_warp_control_t *dsp_cmd, int active_rid)
{
	#define warp_region_print_dec(arg)	DRV_PRINT(KERN_DEBUG "    "#arg" = %d\n", (u32)dsp_cmd->arg)
	#define warp_region_print_hex(arg)	DRV_PRINT(KERN_DEBUG "    "#arg" = 0x%08x\n", (u32)dsp_cmd->arg)

	/* Print cmd */
	DRV_PRINT(KERN_DEBUG "Warp Region cmd[%d] Addr: 0x%08x\n", active_rid,
		(u32)(iav->mmap[IAV_BUFFER_WARP].phys +
		WARP_BATCH_CMDS_OFFSET + iav->next_warp_index * WARP_BATCH_PART_SIZE +
		active_rid * DSP_CMD_SIZE));
	warp_region_print_dec(region_id);
	warp_region_print_dec(input_win.x);
	warp_region_print_dec(input_win.y);
	warp_region_print_dec(input_win.w);
	warp_region_print_dec(input_win.h);
	warp_region_print_dec(intermediate_win.x);
	warp_region_print_dec(intermediate_win.y);
	warp_region_print_dec(intermediate_win.w);
	warp_region_print_dec(intermediate_win.h);
	warp_region_print_dec(output_win.x);
	warp_region_print_dec(output_win.y);
	warp_region_print_dec(output_win.w);
	warp_region_print_dec(output_win.h);
	warp_region_print_dec(rotate_flip_input);
	warp_region_print_dec(h_warp_enabled);
	warp_region_print_dec(h_warp_grid_w);
	warp_region_print_dec(h_warp_grid_h);
	warp_region_print_dec(h_warp_h_grid_spacing_exponent);
	warp_region_print_dec(h_warp_v_grid_spacing_exponent);
	warp_region_print_dec(v_warp_enabled);
	warp_region_print_dec(v_warp_grid_w);
	warp_region_print_dec(v_warp_grid_h);
	warp_region_print_dec(v_warp_h_grid_spacing_exponent);
	warp_region_print_dec(v_warp_v_grid_spacing_exponent);
	warp_region_print_dec(me1_vwarp_enabled);
	warp_region_print_dec(me1_vwarp_grid_w);
	warp_region_print_dec(me1_vwarp_grid_h);
	warp_region_print_dec(me1_vwarp_h_grid_spacing_exponent);
	warp_region_print_dec(me1_vwarp_v_grid_spacing_exponent);
	warp_region_print_hex(h_warp_grid_addr);
	warp_region_print_hex(v_warp_grid_addr);
	warp_region_print_hex(me1_vwarp_grid_addr);
	warp_region_print_dec(prev_a_input_win.x);
	warp_region_print_dec(prev_a_input_win.y);
	warp_region_print_dec(prev_a_input_win.w);
	warp_region_print_dec(prev_a_input_win.h);
	warp_region_print_dec(prev_a_output_win.x);
	warp_region_print_dec(prev_a_output_win.y);
	warp_region_print_dec(prev_a_output_win.w);
	warp_region_print_dec(prev_a_output_win.h);
	warp_region_print_dec(prev_c_input_win.x);
	warp_region_print_dec(prev_c_input_win.y);
	warp_region_print_dec(prev_c_input_win.w);
	warp_region_print_dec(prev_c_input_win.h);
	warp_region_print_dec(prev_c_output_win.x);
	warp_region_print_dec(prev_c_output_win.y);
	warp_region_print_dec(prev_c_output_win.w);
	warp_region_print_dec(prev_c_output_win.h);
}

int cmd_set_region_warp_control(struct ambarella_iav *iav,
	int area_id, int active_rid)
{
	ipcam_set_region_warp_control_t* dsp_cmd;
	WARP_RECT_t *dsp_input, *dsp_output;
	struct iav_warp_area *area = &iav->warp->area[area_id];
	struct iav_warp_map *map;
	struct iav_rect* dptz_input, *dptz_output;
	int i;

	dsp_cmd = (ipcam_set_region_warp_control_t *)(iav->mmap[IAV_BUFFER_WARP].virt +
		WARP_BATCH_CMDS_OFFSET + iav->next_warp_index * WARP_BATCH_PART_SIZE +
		active_rid * DSP_CMD_SIZE);

	memset(dsp_cmd, 0, sizeof(ipcam_set_region_warp_control_t));
	dsp_cmd->input_win.x = area->input.x;
	dsp_cmd->input_win.y = area->input.y;
	dsp_cmd->input_win.w = area->input.width;
	dsp_cmd->input_win.h = area->input.height;
	dsp_cmd->output_win.x = area->output.x;
	dsp_cmd->output_win.y = area->output.y;
	dsp_cmd->output_win.w = area->output.width;
	dsp_cmd->output_win.h = area->output.height;
	dsp_cmd->region_id =  active_rid;
	dsp_cmd->rotate_flip_input = area->rotate_flip;

	// Intermediate buffer
	dsp_cmd->intermediate_win.x = area->intermediate.x;
	dsp_cmd->intermediate_win.y = area->intermediate.y;
	dsp_cmd->intermediate_win.w = area->intermediate.width;
	dsp_cmd->intermediate_win.h = area->intermediate.height;

	map = &area->h_map;
	dsp_cmd->h_warp_enabled = map->enable;
	if (map->enable) {
		dsp_cmd->h_warp_grid_w = map->output_grid_col;
		dsp_cmd->h_warp_grid_h = map->output_grid_row;
		dsp_cmd->h_warp_h_grid_spacing_exponent = map->h_spacing;
		dsp_cmd->h_warp_v_grid_spacing_exponent =  map->v_spacing;
		dsp_cmd->h_warp_grid_addr = PHYS_TO_DSP(iav->mmap[IAV_BUFFER_WARP].phys +
			WARP_VECT_PART_SIZE * (iav->curr_warp_index) +
			map->data_addr_offset);
	}

	map = &area->v_map;
	dsp_cmd->v_warp_enabled = map->enable;
	if (map->enable) {
		dsp_cmd->v_warp_grid_w = map->output_grid_col;
		dsp_cmd->v_warp_grid_h = map->output_grid_row;
		dsp_cmd->v_warp_h_grid_spacing_exponent = map->h_spacing;
		dsp_cmd->v_warp_v_grid_spacing_exponent =  map->v_spacing;
		dsp_cmd->v_warp_grid_addr = PHYS_TO_DSP(iav->mmap[IAV_BUFFER_WARP].phys +
			WARP_VECT_PART_SIZE * (iav->curr_warp_index) +
			map->data_addr_offset);
	}

	map = &area->me1_v_map;
	dsp_cmd->me1_vwarp_enabled = map->enable;
	if (map->enable) {
		dsp_cmd->me1_vwarp_grid_w = map->output_grid_col;
		dsp_cmd->me1_vwarp_grid_h = map->output_grid_row;
		dsp_cmd->me1_vwarp_h_grid_spacing_exponent = map->h_spacing;
		dsp_cmd->me1_vwarp_v_grid_spacing_exponent =  map->v_spacing;
		dsp_cmd->me1_vwarp_grid_addr = PHYS_TO_DSP(iav->mmap[IAV_BUFFER_WARP].phys +
			WARP_VECT_PART_SIZE * (iav->curr_warp_index) +
			map->data_addr_offset);
	}

	for (i = IAV_SUB_SRCBUF_FIRST; i < IAV_SUB_SRCBUF_LAST; i++) {
		dptz_input = &iav->warp_dptz[i].input[area_id];
		dptz_output = &iav->warp_dptz[i].output[area_id];
		switch (i) {
			case IAV_SRCBUF_PA:
				dsp_input = &dsp_cmd->prev_a_input_win;
				dsp_output = &dsp_cmd->prev_a_output_win;
				break;
			case IAV_SRCBUF_PC:
				dsp_input = &dsp_cmd->prev_c_input_win;
				dsp_output = &dsp_cmd->prev_c_output_win;
				break;
			default:
				continue;
		}

		dsp_input->x = dptz_input->x;
		dsp_input->y = dptz_input->y;
		dsp_input->w = dptz_input->width;
		dsp_input->h = dptz_input->height;
		dsp_output->x = dptz_output->x;
		dsp_output->y = dptz_output->y;
		dsp_output->w = dptz_output->width;
		dsp_output->h = dptz_output->height;
	}

	debug_print_warp_region(iav, dsp_cmd, active_rid);

	return 0;
}

// 0x600A command
int cmd_set_warp_batch_cmds(struct ambarella_iav *iav, int active_num,
	struct amb_dsp_cmd *cmd)
{
	ipcam_set_region_warp_control_batch_t *warp_batch_cmd;

	warp_batch_cmd = get_cmd(iav->dsp, cmd);
	if (!warp_batch_cmd) {
		return -ENOMEM;
	}
	warp_batch_cmd->cmd_code = IPCAM_SET_REGION_WARP_CONTROL;
	warp_batch_cmd->num_regions_minus_1 = active_num - 1;
	warp_batch_cmd->warp_region_ctrl_daddr = PHYS_TO_DSP(
		iav->mmap[IAV_BUFFER_WARP].phys +
		WARP_BATCH_CMDS_OFFSET + iav->next_warp_index *
		WARP_TABLE_AREA_MAX_NUM * WARP_CMD_SIZE);

	put_cmd(iav->dsp, cmd, warp_batch_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

int cmd_apply_frame_sync_cmd(struct ambarella_iav *iav,
	struct amb_dsp_cmd *cmd, struct iav_apply_frame_sync *apply)
{
	enc_sync_cmd_t *dsp_cmd;
	ipcam_real_time_encode_param_setup_t *cmd_data;
	u8 *addr = NULL;
	u16 enable_flag = 0;
	int i;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = IPCAM_ENC_SYNC_CMD;
	dsp_cmd->target_pts = apply->dsp_pts;

	addr = (u8 *)iav->mmap[IAV_BUFFER_CMD_SYNC].phys +
		iav->cmd_sync_idx * CMD_SYNC_SIZE;
	dsp_cmd->cmd_daddr = PHYS_TO_DSP(addr);

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		addr = (u8 *)iav->mmap[IAV_BUFFER_CMD_SYNC].virt +
			iav->cmd_sync_idx * CMD_SYNC_SIZE + i * DSP_ENC_CMD_SIZE;
		cmd_data = (ipcam_real_time_encode_param_setup_t *)addr;
		if (cmd_data->cmd_code) {
			enable_flag |= (1 << i);
		}
	}

	dsp_cmd->enable_flag = enable_flag;
	dsp_cmd->force_update_flag = (enable_flag & apply->force_update);

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_ENC, 0);

	return 0;
}

// 0x600C command
int cmd_get_frm_buf_pool_info(struct ambarella_iav *iav,
	struct amb_dsp_cmd *cmd, u32 buf_map)
{
	ipcam_get_frame_buf_pool_info_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	memset(dsp_cmd, 0, sizeof(ipcam_get_frame_buf_pool_info_t));
	dsp_cmd->cmd_code = IPCAM_GET_FRAME_BUF_POOL_INFO;
	dsp_cmd->frm_buf_pool_info_req_bit_flag = get_buffer_pool_map(buf_map);

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

// 0x600E command
int cmd_efm_creat_frm_buf_pool(struct ambarella_iav *iav,
	struct amb_dsp_cmd *cmd)
{
	ipcam_efm_creat_frm_buf_pool_cmd_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	memset(dsp_cmd, 0, sizeof(ipcam_efm_creat_frm_buf_pool_cmd_t));
	dsp_cmd->cmd_code = IPCAM_EFM_CREATE_FRAME_BUF_POOL;
	dsp_cmd->buf_width = iav->efm.efm_size.width;
	dsp_cmd->buf_height = iav->efm.efm_size.height;
	dsp_cmd->fbp_type = (1 << EFM_BUF_POOL_TYPE_ME1) |
		(1 << EFM_BUF_POOL_TYPE_YUV420);
	dsp_cmd->max_num_yuv = iav->efm.req_buf_num;
	dsp_cmd->max_num_me1 = iav->efm.req_buf_num;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

// 0x600F command
int cmd_efm_req_frame_buf(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd)
{
	ipcam_efm_req_frm_buf_cmd_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	memset(dsp_cmd, 0, sizeof(ipcam_efm_req_frm_buf_cmd_t));
	dsp_cmd->cmd_code = IPCAM_EFM_REQ_FRAME_BUF;
	dsp_cmd->fbp_type = (1 << EFM_BUF_POOL_TYPE_ME1) |
		(1 << EFM_BUF_POOL_TYPE_YUV420);

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

// 0x6010 command
int cmd_efm_handshake(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd)
{
	ipcam_efm_handshake_cmd_t *dsp_cmd;
	struct iav_efm_buf_pool *pool = &iav->efm.buf_pool;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	memset(dsp_cmd, 0, sizeof(ipcam_efm_handshake_cmd_t));
	dsp_cmd->cmd_code = IPCAM_EFM_HANDSHAKE;
	dsp_cmd->yuv_fId = pool->yuv_fId[pool->hs_idx];
	dsp_cmd->me1_fId = pool->me1_fId[pool->hs_idx];
	dsp_cmd->pts = iav->efm.curr_pts;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

// 0x1010 command
int cmd_hash_verification(struct ambarella_iav *iav, u8* input)
{
	hash_verification_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, NULL);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = HASH_VERIFICATION;
	memcpy(dsp_cmd->hash_input, input, 8);

	put_cmd(iav->dsp, NULL, dsp_cmd, DSP_CMD_TYPE_NORMAL, 0);

	return 0;
}

// 0x1006 command
int cmd_reset_operation(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd)
{
	reset_operation_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	dsp_cmd->cmd_code = RESET_OPERATION;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_DEC, 0);

	return 0;
}

int cmd_h264_decoder_setup(struct ambarella_iav *iav,
	struct amb_dsp_cmd *cmd, u8 decoder_id)
{
	h264_decode_setup_t *dsp_cmd;
	struct iav_decoder_info *decoder = &iav->decode_context.decoder_config[decoder_id];

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	memset(dsp_cmd, 0x0, sizeof(h264_decode_setup_t));
	dsp_cmd->cmd_code = H264_DECODING_SETUP;

	dsp_cmd->bits_fifo_base = PHYS_TO_DSP(iav->mmap[IAV_BUFFER_BSB].phys);
	dsp_cmd->bits_fifo_limit = dsp_cmd->bits_fifo_base + iav->mmap[IAV_BUFFER_BSB].size - 1;

	dsp_cmd->cabac_to_recon_delay = 0x6;
	dsp_cmd->forced_max_fb_size = 0x0;

	dsp_cmd->gop_n = 0xf;
	dsp_cmd->gop_m = 0x1;
	dsp_cmd->dpb_size = 0x6;
	dsp_cmd->customer_mem_usage = 1;
	dsp_cmd->trickplay_fw = 1;
	dsp_cmd->trickplay_ff_IP = 1;
	dsp_cmd->trickplay_ff_I = 1;
	dsp_cmd->trickplay_bw = 1;
	dsp_cmd->trickplay_fb_IP = 1;
	dsp_cmd->trickplay_fb_I = 1;
	dsp_cmd->trickplay_rsv = 0;
	dsp_cmd->bit_rate = (12 << 20);

	//if (!iav->decode_context.b_interlace_vout) {
	if (0 == decoder->vout_configs[0].vout_id) {
		dsp_cmd->vout0_win_width = decoder->vout_configs[0].target_win_width;
		dsp_cmd->vout0_win_height = decoder->vout_configs[0].target_win_height;
	} else if (1 == decoder->vout_configs[0].vout_id) {
		dsp_cmd->vout1_win_width = decoder->vout_configs[0].target_win_width;
		dsp_cmd->vout1_win_height = decoder->vout_configs[0].target_win_height;
	}
	//}

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_DEC, 0);

	return 0;
}

int cmd_decode_stop(struct ambarella_iav *iav, u8 decoder_id, u8 stop_flag)
{
	h264_decode_stop_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, NULL);
	if (!dsp_cmd)
		return -ENOMEM;

	memset(dsp_cmd, 0x0, sizeof(h264_decode_stop_t));

	dsp_cmd->cmd_code = DECODE_STOP;
	dsp_cmd->stop_flag = stop_flag;

	put_cmd(iav->dsp, NULL, dsp_cmd, DSP_CMD_TYPE_DEC, 0);

	return 0;
}

int cmd_vout_video_setup(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd,
	u16 vout_id, u8 en, u8 src, u8 flip, u8 rotate,
	u16 offset_x, u16 offset_y, u16 width, u16 height)
{
	vout_video_setup_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	memset(dsp_cmd, 0x0, sizeof(vout_video_setup_t));
	dsp_cmd->cmd_code = VOUT_VIDEO_SETUP;
	dsp_cmd->vout_id = vout_id;
	dsp_cmd->en = en;
	dsp_cmd->src = src;
	dsp_cmd->flip = flip;
	dsp_cmd->rotate = rotate;
	dsp_cmd->win_offset_x = offset_x;
	dsp_cmd->win_offset_y = offset_y;
	dsp_cmd->win_width = width;
	dsp_cmd->win_height = height;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_DEC, 0);

	return 0;
}

int cmd_rescale_postp(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd,
	u8 decoder_id, u16 center_x, u16 center_y)
{
	rescale_postproc_t *dsp_cmd;
	struct iav_decode_vout_config *vout = NULL;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	memset(dsp_cmd, 0, sizeof(rescale_postproc_t));
	dsp_cmd->cmd_code = RESCALE_POSTPROCESSING;
	dsp_cmd->input_center_x = center_x;
	dsp_cmd->input_center_y = center_y;

	vout = &iav->decode_context.decoder_config[0].vout_configs[0];
	if (1 == vout->vout_id) {
		dsp_cmd->display_win_offset_x = vout->target_win_offset_x;
		dsp_cmd->display_win_offset_y = vout->target_win_offset_y;
		dsp_cmd->display_win_width = vout->target_win_width;
		dsp_cmd->display_win_height = vout->target_win_height;
		dsp_cmd->zoom_factor_x = vout->zoom_factor_x;
		dsp_cmd->zoom_factor_y = vout->zoom_factor_y;
		dsp_cmd->vout1_flip = vout->flip;
		dsp_cmd->vout1_rotate = vout->rotate;
		dsp_cmd->vout1_win_offset_x = vout->target_win_offset_x;
		dsp_cmd->vout1_win_offset_y = vout->target_win_offset_y;
		dsp_cmd->vout1_win_width = vout->target_win_width;
		if (!iav->decode_context.b_interlace_vout) {
			dsp_cmd->vout1_win_height = vout->target_win_height;
		} else {
			dsp_cmd->vout1_win_height = vout->target_win_height / 2;
		}
	} else if (0 == vout->vout_id) {
		dsp_cmd->sec_display_win_offset_x = vout->target_win_offset_x;
		dsp_cmd->sec_display_win_offset_y = vout->target_win_offset_y;
		dsp_cmd->sec_display_win_width = vout->target_win_width;
		dsp_cmd->sec_display_win_height = vout->target_win_height;
		dsp_cmd->sec_zoom_factor_x = vout->zoom_factor_x;
		dsp_cmd->sec_zoom_factor_y = vout->zoom_factor_y;
		dsp_cmd->vout0_flip = vout->flip;
		dsp_cmd->vout0_rotate = vout->rotate;
		dsp_cmd->vout0_win_offset_x = vout->target_win_offset_x;
		dsp_cmd->vout0_win_offset_y = vout->target_win_offset_y;
		dsp_cmd->vout0_win_width = vout->target_win_width;
		if (!iav->decode_context.b_interlace_vout) {
			dsp_cmd->vout0_win_height = vout->target_win_height;
		} else {
			dsp_cmd->vout0_win_height = vout->target_win_height / 2;
		}
	}

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_DEC, 0);

	return 0;
}

int cmd_playback_speed(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd,
	u8 decoder_id, u16 speed, u8 scan_mode, u8 direction)
{
	h264_playback_speed_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	memset(dsp_cmd, 0x0, sizeof(h264_playback_speed_t));
	dsp_cmd->cmd_code = H264_PLAYBACK_SPEED;
	dsp_cmd->speed = speed;
	dsp_cmd->scan_mode = scan_mode;
	dsp_cmd->direction = direction;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_DEC, 0);

	return 0;
}

int cmd_h264_decode(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd,
	u8 decoder_id, u32 bits_fifo_start, u32 bits_fifo_end,
	u32 num_frames, u32 first_frame_display)
{
	h264_decode_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	memset(dsp_cmd, 0x0, sizeof(h264_decode_t));
	dsp_cmd->cmd_code = H264_DECODE;
	dsp_cmd->bits_fifo_start = bits_fifo_start;
	dsp_cmd->bits_fifo_end = bits_fifo_end;
	dsp_cmd->num_pics = num_frames;
	dsp_cmd->first_frame_display = first_frame_display;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_DEC, 0);

	return 0;
}

int cmd_h264_decode_fifo_update(struct ambarella_iav *iav,
	struct amb_dsp_cmd *cmd, u8 decoder_id,
	u32 bits_fifo_start, u32 bits_fifo_end, u32 num_frames)
{
	h264_decode_bits_fifo_update_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	memset(dsp_cmd, 0x0, sizeof(h264_decode_bits_fifo_update_t));
	dsp_cmd->cmd_code = H264_DECODE_BITS_FIFO_UPDATE;
	dsp_cmd->bits_fifo_start = bits_fifo_start;
	dsp_cmd->bits_fifo_end = bits_fifo_end;
	dsp_cmd->num_pics = num_frames;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_DEC, 0);

	return 0;
}

int cmd_h264_trick_play(struct ambarella_iav *iav,
	struct amb_dsp_cmd *cmd, u8 decoder_id, u8 mode)
{
	h264_trickplay_t *dsp_cmd;

	dsp_cmd = get_cmd(iav->dsp, cmd);
	if (!dsp_cmd)
		return -ENOMEM;

	memset(dsp_cmd, 0x0, sizeof(h264_trickplay_t));
	dsp_cmd->cmd_code = H264_TRICKPLAY;
	dsp_cmd->mode = mode;

	put_cmd(iav->dsp, cmd, dsp_cmd, DSP_CMD_TYPE_DEC, 0);

	return 0;
}

static int parse_cmd_system_info_setup(struct ambarella_iav *iav, DSP_CMD *cmd)
{
	system_setup_info_t *dsp_cmd = (system_setup_info_t *)cmd;

	iav->encode_mode = dsp_cmd->mode_flags;
	iav->srcbuf[IAV_SRCBUF_PA].type = preview_type_to_buffer_format(dsp_cmd->preview_A_type);
	iav->srcbuf[IAV_SRCBUF_PB].type = preview_type_to_buffer_format(dsp_cmd->preview_B_type);

	iav->osd_from_mixer_a = dsp_cmd->voutA_osd_blend_enabled;
	iav->osd_from_mixer_b = dsp_cmd->voutB_osd_blend_enabled;
	iav->system_config[iav->encode_mode].enc_raw_rgb = dsp_cmd->raw_encode_enabled;
	iav->system_config[iav->encode_mode].expo_num = dsp_cmd->hdr_num_exposures_minus_1 + 1;
	iav->system_config[iav->encode_mode].vout_swap = dsp_cmd->vout_swap;
	iav->system_config[iav->encode_mode].me0_scale = dsp_cmd->me0_scale_factor;
	/* TODO: dsp_cmd->adv_iso_disabled */

	iav_debug ("cmd_code = 0x%08X, Encode mode [%u]\n", dsp_cmd->cmd_code, iav->encode_mode);

	return 0;
}

static int parse_cmd_set_vin_global_config(struct ambarella_iav *iav, DSP_CMD *cmd)
{
	int rval = 0;
	set_vin_global_config_t *dsp_cmd = (set_vin_global_config_t *)cmd;

	switch (dsp_cmd->global_cfg_reg_word0) {
	case 0x8:
		iav->vinc[0]->vin_format.interface_type = SENSOR_SERIAL_LVDS;
		break;
	case 0x9:
		/* FIXME: Since S2L doesn't support parallel LVDS interface,
		 * here interface_type = SENSOR_PARALLEL_LVCMOS.
		 * Otherwise it has to judge between SENSOR_PARALLEL_LVDS
		 * and SENSOR_PARALLEL_LVCMOS. */
		iav->vinc[0]->vin_format.interface_type = SENSOR_PARALLEL_LVCMOS;
		break;
	case 0xA:
		iav->vinc[0]->vin_format.interface_type = SENSOR_MIPI;
		break;
	default:
		rval = -1;
		iav_error("Invalid vin interface type\n");
		break;
	}

	iav_debug ("cmd_code = 0x%08X\n", dsp_cmd->cmd_code);

	return rval;
}

static int parse_cmd_video_preproc(struct ambarella_iav *iav, DSP_CMD *cmd)
{
	video_preproc_t *dsp_cmd = (video_preproc_t *)cmd;
	struct amba_video_info *vout0_info, *vout1_info;

	iav->vinc[0]->vin_format.input_format = dsp_cmd->input_format;
	iav->vinc[0]->vin_format.sensor_id = dsp_cmd->sensor_id;
	/* FIXME: iav_vin.c have done these.
	iav->vinc[0]->vin_format.bit_resolution = dsp_cmd->bit_resolution;
	iav->vinc[0]->vin_format.bayer_pattern = dsp_cmd->bayer_pattern;
	iav->vinc[0]->vin_format.vin_win.width = dsp_cmd->vidcap_w;
	iav->vinc[0]->vin_format.vin_win.height = dsp_cmd->vidcap_h; */

	if (iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) {
		iav->srcbuf[IAV_SRCBUF_PMN].win.width = dsp_cmd->main_w;
		iav->srcbuf[IAV_SRCBUF_PMN].win.height= dsp_cmd->main_h;
	} else {
		iav->srcbuf[IAV_SRCBUF_MN].win.width = dsp_cmd->main_w;
		iav->srcbuf[IAV_SRCBUF_MN].win.height = dsp_cmd->main_h;
	}
	if (iav->system_config[iav->encode_mode].vout_swap) {
		vout0_info = &G_voutinfo[1].video_info;
		vout1_info = &G_voutinfo[0].video_info;
	} else {
		vout0_info = &G_voutinfo[0].video_info;
		vout1_info = &G_voutinfo[1].video_info;
	}
	/* FIXEME: DSP CMD don't contain rotate, use IAV default value */
	if (vout0_info->rotate) {
		vout0_info->height = dsp_cmd->preview_w_A;
		vout0_info->width = dsp_cmd->preview_h_A;
	} else {
		vout0_info->width = dsp_cmd->preview_w_A;
		vout0_info->height = dsp_cmd->preview_h_A;
	}
	if (vout1_info->rotate) {
		vout1_info->height = dsp_cmd->preview_w_B;
		vout1_info->width = dsp_cmd->preview_h_B;
	} else {
		vout1_info->width = dsp_cmd->preview_w_B;
		vout1_info->height = dsp_cmd->preview_h_B;
	}

	vout0_info->format = dsp_format_to_amba_iav_format(dsp_cmd->preview_format_A);
	vout1_info->format = dsp_format_to_amba_iav_format(dsp_cmd->preview_format_B);
	if (iav->system_config[iav->encode_mode].vout_swap) {
		G_voutinfo[0].enabled = dsp_cmd->preview_B_en;
		G_voutinfo[1].enabled = dsp_cmd->preview_A_en;
	} else {
		G_voutinfo[0].enabled = dsp_cmd->preview_A_en;
		G_voutinfo[1].enabled = dsp_cmd->preview_B_en;
	}
	iav->cmd_read_delay = dsp_cmd->cmdReadDly;
	/*TODO: dsp_cmd->Vert_WARP_enable
			dsp_cmd->force_stitching
			dsp_cmd->still_iso_config_daddr */

	iav_debug ("cmd_code = 0x%08X\n", dsp_cmd->cmd_code);

	return 0;
}

static int parse_set_warp_control(struct ambarella_iav *iav, DSP_CMD *cmd)
{
	struct iav_rect *vin;
	struct iav_rect input_win;
	u8 enc_mode = iav->encode_mode;
	u8 expo_num = iav->system_config[enc_mode].expo_num;
	set_warp_control_t *dsp_cmd = (set_warp_control_t *)cmd;

	input_win.width = dsp_cmd->dummy_window_width;
	input_win.height = dsp_cmd->dummy_window_height;

	get_vin_win(iav, &vin, 0);

	if (expo_num >= MAX_HDR_2X) {
		/* FIXME: this entrance has not been implimented yet, because
		     for the moment fastboot doesn't use HDR mode.
		     Also, if entered this entrance, it's unable to get the value of vin->x and vin->y,
		     which is got from iav->vinc[0]->vin_format.act_win or vin_win, since these
		     values are restored after the sensor driver is probed, which takes place after
		     parse functions. */
		input_win.x = dsp_cmd->dummy_window_x_left - vin->x;
		input_win.y = dsp_cmd->dummy_window_y_top - vin->y;
	} else {
		input_win.x = dsp_cmd->dummy_window_x_left;
		input_win.y = dsp_cmd->dummy_window_y_top;
	}

	if (dsp_cmd->warp_control == WARP_CONTROL_ENABLE ||
		dsp_cmd->vert_warp_enable == WARP_CONTROL_ENABLE) {
		if (dsp_cmd->warp_control == WARP_CONTROL_ENABLE) {
			iav->warp->area[0].h_map.enable = 1;
			iav->warp->area[0].h_map.output_grid_col = dsp_cmd->grid_array_width;
			iav->warp->area[0].h_map.output_grid_row = dsp_cmd->grid_array_height;
			iav->warp->area[0].h_map.h_spacing = dsp_cmd->horz_grid_spacing_exponent;
			iav->warp->area[0].h_map.v_spacing = dsp_cmd->vert_grid_spacing_exponent;
		}
		if (dsp_cmd->vert_warp_enable == WARP_CONTROL_ENABLE) {
			iav->warp->area[0].v_map.enable = 1;
			iav->warp->area[0].v_map.output_grid_col = dsp_cmd->vert_warp_grid_array_width;
			iav->warp->area[0].v_map.output_grid_row = dsp_cmd->vert_warp_grid_array_height;
			iav->warp->area[0].v_map.h_spacing = dsp_cmd->vert_warp_horz_grid_spacing_exponent;
			iav->warp->area[0].v_map.v_spacing = dsp_cmd->vert_warp_vert_grid_spacing_exponent;

			iav->warp->area[0].me1_v_map.output_grid_col = dsp_cmd->ME1_vwarp_grid_array_width;
			iav->warp->area[0].me1_v_map.output_grid_row = dsp_cmd->ME1_vwarp_grid_array_height;
			iav->warp->area[0].me1_v_map.h_spacing = dsp_cmd->ME1_vwarp_horz_grid_spacing_exponent;
			iav->warp->area[0].me1_v_map.v_spacing = dsp_cmd->ME1_vwarp_vert_grid_spacing_exponent;
		}
		iav->warp->area[0].enable = 1;
		iav->system_config[enc_mode].lens_warp = 1;
	}

	if (is_warp_mode(iav) || iav->system_config[enc_mode].lens_warp) {
		if (iav->system_config[enc_mode].lens_warp) {
			iav->warp->area[0].input.width = input_win.width;
			iav->warp->area[0].input.height = input_win.height;
			iav->warp->area[0].input.x = input_win.x;
			iav->warp->area[0].input.y = input_win.y;

			iav->main_buffer->input.width = iav->warp->area[0].input.width;
			iav->main_buffer->input.height = iav->warp->area[0].input.height;
			iav->main_buffer->input.x = iav->warp->area[0].input.x;
			iav->main_buffer->input.y = iav->warp->area[0].input.y;
		} else {
			iav->srcbuf[IAV_SRCBUF_PMN].input.width = input_win.width;
			iav->srcbuf[IAV_SRCBUF_PMN].input.height = input_win.height;
			iav->srcbuf[IAV_SRCBUF_PMN].input.x = input_win.x;
			iav->srcbuf[IAV_SRCBUF_PMN].input.y = input_win.y;

			if (iav->encode_mode == DSP_SINGLE_REGION_WARP_MODE) {
				if ((dsp_cmd->actual_right_bot_x >> 16) >= iav->srcbuf[IAV_SRCBUF_MN].win.width &&
					(dsp_cmd->actual_right_bot_y >> 16) >= iav->srcbuf[IAV_SRCBUF_MN].win.height) {
					iav->srcbuf[IAV_SRCBUF_PMN].win.width = dsp_cmd->cfa_output_width;
					iav->srcbuf[IAV_SRCBUF_PMN].win.height = dsp_cmd->cfa_output_height;
				}
			}

			iav->main_buffer->input.width = iav->srcbuf[IAV_SRCBUF_PMN].win.width;
			iav->main_buffer->input.height = iav->srcbuf[IAV_SRCBUF_PMN].win.height;
			iav->main_buffer->input.x = 0;
			iav->main_buffer->input.y = 0;
		}
	} else {
		iav->main_buffer->input.width = input_win.width;
		iav->main_buffer->input.height = input_win.height;
		iav->main_buffer->input.x = input_win.x;
		iav->main_buffer->input.y = input_win.y;
	}

	iav_debug ("cmd_code = 0x%08X\n", dsp_cmd->cmd_code);
	return 0;
}

static int parse_cmd_video_system_setup(struct ambarella_iav *iav, DSP_CMD *cmd)
{
	ipcam_video_system_setup_t *dsp_cmd = (ipcam_video_system_setup_t *)cmd;

	if (iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) {
		iav->system_config[iav->encode_mode].max_warp_input_width = dsp_cmd->max_warp_region_input_width;
		iav->system_config[iav->encode_mode].max_warp_input_height = dsp_cmd->max_warp_region_input_height;
		iav->system_config[iav->encode_mode].max_warp_output_width = dsp_cmd->max_warp_region_output_width;
		iav->system_config[iav->encode_mode].v_warped_main_max_width = dsp_cmd->v_warped_main_max_width;
		iav->system_config[iav->encode_mode].v_warped_main_max_height = dsp_cmd->v_warped_main_max_height;
		iav->srcbuf[IAV_SRCBUF_MN].max.width = dsp_cmd->warped_main_max_width;
		iav->srcbuf[IAV_SRCBUF_MN].max.height = get_rounddown_height(dsp_cmd->warped_main_max_height);
		iav->srcbuf[IAV_SRCBUF_PMN].win.width = dsp_cmd->main_max_width;
		iav->srcbuf[IAV_SRCBUF_PMN].win.height =  get_rounddown_height(dsp_cmd->main_max_height);
	} else {
		iav->srcbuf[IAV_SRCBUF_MN].max.width = dsp_cmd->main_max_width;
		iav->srcbuf[IAV_SRCBUF_MN].max.height = get_rounddown_height(dsp_cmd->main_max_height);
	}
	iav->system_config[iav->encode_mode].max_padding_width = dsp_cmd->max_padding_width;
	iav->system_config[iav->encode_mode].rotatable = dsp_cmd->enc_rotation_possible;

	iav->srcbuf[IAV_SRCBUF_PC].max.width = dsp_cmd->preview_C_max_width;
	iav->srcbuf[IAV_SRCBUF_PC].max.height = get_rounddown_height(dsp_cmd->preview_C_max_height);
	iav->srcbuf[IAV_SRCBUF_PB].max.width = dsp_cmd->preview_B_max_width;
	iav->srcbuf[IAV_SRCBUF_PB].max.height = get_rounddown_height(dsp_cmd->preview_B_max_height);
	iav->srcbuf[IAV_SRCBUF_PA].max.width = dsp_cmd->preview_A_max_width;
	iav->srcbuf[IAV_SRCBUF_PA].max.height = get_rounddown_height(dsp_cmd->preview_A_max_height);

	iav->stream[0].max_GOP_M = dsp_cmd->stream_0_max_GOP_M;
	iav->stream[1].max_GOP_M = dsp_cmd->stream_1_max_GOP_M;
	iav->stream[2].max_GOP_M = dsp_cmd->stream_2_max_GOP_M;
	iav->stream[3].max_GOP_M = dsp_cmd->stream_3_max_GOP_M;

	iav->stream[0].max.width = dsp_cmd->stream_0_max_width;
	iav->stream[0].max.height = get_rounddown_height(dsp_cmd->stream_0_max_height);
	iav->stream[1].max.width = dsp_cmd->stream_1_max_width;
	iav->stream[1].max.height = get_rounddown_height(dsp_cmd->stream_1_max_height);
	iav->stream[2].max.width = dsp_cmd->stream_2_max_width;
	iav->stream[2].max.height = get_rounddown_height(dsp_cmd->stream_2_max_height);
	iav->stream[3].max.width = dsp_cmd->stream_3_max_width;
	iav->stream[3].max.height = get_rounddown_height(dsp_cmd->stream_3_max_height);

	iav->system_config[iav->encode_mode].max_stream_num = dsp_cmd->max_num_streams;
	iav->system_config[iav->encode_mode].raw_capture = dsp_cmd->raw_compression_disabled;

	iav->srcbuf[IAV_SRCBUF_MN].extra_dram_buf = dsp_cmd->extra_buf_main;
	iav->srcbuf[IAV_SRCBUF_PA].extra_dram_buf = dsp_cmd->extra_buf_preview_a;
	iav->srcbuf[IAV_SRCBUF_PB].extra_dram_buf = dsp_cmd->extra_buf_preview_b;
	iav->srcbuf[IAV_SRCBUF_PC].extra_dram_buf = dsp_cmd->extra_buf_preview_c;
	iav->system_config[iav->encode_mode].enc_dummy_latency = dsp_cmd->enc_dummy_latency;

	iav->stream[0].long_ref_enable = dsp_cmd->stream_0_LT_enable;
	iav->stream[1].long_ref_enable = dsp_cmd->stream_1_LT_enable;
	iav->stream[2].long_ref_enable = dsp_cmd->stream_2_LT_enable;
	iav->stream[3].long_ref_enable = dsp_cmd->stream_3_LT_enable;
	/* TODO: dsp_cmd->vwarp_blk_h */

	iav_debug ("cmd_code = 0x%08X\n", dsp_cmd->cmd_code);

	return 0;
}

static int parse_cmd_capture_buffer_setup(struct ambarella_iav *iav, DSP_CMD *cmd)
{
	int rval = 0;
	ipcam_capture_preview_size_setup_t *dsp_cmd = (ipcam_capture_preview_size_setup_t *)cmd;

	enum iav_srcbuf_id buf_id = IAV_SRCBUF_FIRST;
	rval = get_srcbuf_id(dsp_cmd->preview_id, &buf_id);
	if (dsp_cmd->disabled) {
		iav->srcbuf[buf_id].max.width = dsp_cmd->cap_width;
		iav->srcbuf[buf_id].max.height = dsp_cmd->cap_height;
	} else {
		iav->srcbuf[buf_id].win.width = dsp_cmd->cap_width;
		iav->srcbuf[buf_id].win.height = dsp_cmd->cap_height;
	}
	iav->srcbuf[buf_id].input.width = dsp_cmd->input_win_width;
	iav->srcbuf[buf_id].input.height = dsp_cmd->input_win_height;
	iav->srcbuf[buf_id].input.x = dsp_cmd->input_win_offset_x;
	iav->srcbuf[buf_id].input.y = dsp_cmd->input_win_offset_y;

	iav_debug ("cmd_code = 0x%08X, Buffer ID [%d]\n", dsp_cmd->cmd_code, buf_id);

	return rval;
}

static int parse_cmd_encode_size_setup(struct ambarella_iav *iav, DSP_CMD *cmd)
{
	int id = 0;
	int rval = 0;
	ipcam_video_encode_size_setup_t *dsp_cmd = (ipcam_video_encode_size_setup_t *)cmd;
	id = GET_STRAME_ID_CODE(dsp_cmd->cmd_code);

	if (iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) {
		switch (dsp_cmd->capture_source) {
		case DSP_CAPBUF_WARP:
			iav->stream[id].format.buf_id = IAV_SRCBUF_MN;
			break;
		case DSP_CAPBUF_MN:
			iav->stream[id].format.buf_id = IAV_SRCBUF_PMN;
			break;
		default:
			rval = get_srcbuf_id(dsp_cmd->capture_source, &iav->stream[id].format.buf_id);
			break;
		}
	} else {
		 rval = get_srcbuf_id(dsp_cmd->capture_source, &iav->stream[id].format.buf_id);
	}

	iav->stream[id].format.enc_win.width = dsp_cmd->enc_width;
	iav->stream[id].format.enc_win.height = get_rounddown_height(dsp_cmd->enc_height);
	iav->stream[id].format.enc_win.x = dsp_cmd->enc_x;
	iav->stream[id].format.enc_win.y = dsp_cmd->enc_y;

	iav_debug ("cmd_code = 0x%08X, Stream ID [%c]\n", dsp_cmd->cmd_code, 'A' + id);

	return rval;
}

static int parse_cmd_h264_encode_setup(struct ambarella_iav *iav, DSP_CMD *cmd)
{
	int id = 0;
	h264_encode_setup_t *dsp_cmd = (h264_encode_setup_t *)cmd;
	id = GET_STRAME_ID_CODE(dsp_cmd->cmd_code);

	iav->stream[id].h264_config.M = dsp_cmd->M;
	iav->stream[id].h264_config.N = (dsp_cmd->N_msb << 8) | dsp_cmd->N;
	get_gop_struct_from_quailty(&iav->stream[id].h264_config, dsp_cmd->quality);
	iav->stream[id].h264_config.idr_interval = dsp_cmd->idr_interval;
	iav->stream[id].h264_config.average_bitrate = dsp_cmd->average_bitrate;
	iav->stream[id].h264_config.vbr_setting = dsp_cmd->vbr_setting;
	iav->stream[id].format.hflip = dsp_cmd->hflip;
	iav->stream[id].format.vflip = dsp_cmd->vflip;
	iav->stream[id].format.rotate_cw = dsp_cmd->rotate;
	iav->stream[id].h264_config.chroma_format = dsp_cmd->chroma_format;
	iav->stream[id].fps.fps_div = dsp_cmd->frame_rate_division_factor;
	iav->stream[id].fps.fps_multi = dsp_cmd->frame_rate_multiplication_factor;
	iav->stream[id].h264_config.cpb_buf_idc = dsp_cmd->cpb_buf_idc;
	iav->stream[id].h264_config.cpb_cmp_idc = dsp_cmd->cpb_cmp_idc;
	iav->stream[id].h264_config.en_panic_rc = dsp_cmd->en_panic_rc;
	iav->stream[id].h264_config.fast_rc_idc = dsp_cmd->fast_rc_idc;
	iav->stream[id].h264_config.cpb_user_size = dsp_cmd->cpb_user_size;
	/* TODO: dsp_cmd->fast_seek_interval */;

	iav_debug ("cmd_code = 0x%08X, Stream ID [%c]\n", dsp_cmd->cmd_code, 'A' + id);

	return 0;
}

static int parse_cmd_h264_encode_start(struct ambarella_iav *iav, DSP_CMD *cmd)
{
	int id = 0;
	int rval = 0;
	h264_encode_t *dsp_cmd = (h264_encode_t *)cmd;
	id = GET_STRAME_ID_CODE(dsp_cmd->cmd_code);

	if (dsp_cmd->encode_duration != ENCODE_DURATION_FOREVER) {
		iav->stream[id].format.duration = dsp_cmd->encode_duration;
	}
	iav->stream[id].h264_config.au_type = dsp_cmd->au_type;
	switch (dsp_cmd->profile) {
	case 1:
		iav->stream[id].h264_config.profile = H264_PROFILE_HIGH;
		break;
	case 0:
		iav->stream[id].h264_config.profile = H264_PROFILE_MAIN;
		break;
	case 3:
		iav->stream[id].h264_config.profile = H264_PROFILE_BASELINE;
		break;
	default:
		rval = -1;
		iav_error("Invalid H264 profile\n");
		break;
	}

	iav->stream[id].dsp_state = ENC_BUSY_STATE;
	iav->stream[id].op = IAV_STREAM_OP_START;
	iav->stream[id].format.type = IAV_STREAM_TYPE_H264;
	inc_srcbuf_ref(iav->stream[id].srcbuf);

	/*TODO: dsp_cmd->frame_crop_right_offset
			dsp_cmd->frame_crop_bottom_offset
			dsp_cmd->aspect_ratio_idc
			dsp_cmd->SAR_width
			dsp_cmd->SAR_height */

	iav_debug ("cmd_code = 0x%08X, Stream ID [%c]\n", dsp_cmd->cmd_code, 'A' + id);

	return rval;
}

static int parse_cmd_jpeg_encode_setup(struct ambarella_iav *iav, DSP_CMD *cmd)
{
	int id = 0;
	jpeg_encode_setup_t *dsp_cmd = (jpeg_encode_setup_t *)cmd;
	id = GET_STRAME_ID_CODE(dsp_cmd->cmd_code);

	iav->stream[id].mjpeg_config.chroma_format = dsp_cmd->chroma_format;
	iav->stream[id].fps.fps_div = dsp_cmd->frame_rate_division_factor;
	iav->stream[id].fps.fps_multi = dsp_cmd->frame_rate_multiplication_factor;
	iav->stream[id].format.hflip = dsp_cmd->hflip;
	iav->stream[id].format.vflip = dsp_cmd->vflip;
	iav->stream[id].format.rotate_cw = dsp_cmd->rotate;

	iav_debug ("cmd_code = 0x%08X, Stream ID [%c]\n", dsp_cmd->cmd_code, 'A' + id);

	return 0;
}

static int parse_cmd_jpeg_encode_start(struct ambarella_iav *iav, DSP_CMD *cmd)
{
	int id = 0;
	mjpeg_capture_t *dsp_cmd = (mjpeg_capture_t *)cmd;
	id = GET_STRAME_ID_CODE(dsp_cmd->cmd_code);

	if (dsp_cmd->encode_duration != ENCODE_DURATION_FOREVER) {
		iav->stream[id].format.duration = dsp_cmd->encode_duration;
	}
	iav->stream[id].dsp_state = ENC_BUSY_STATE;
	iav->stream[id].op = IAV_STREAM_OP_START;
	iav->stream[id].format.type = IAV_STREAM_TYPE_MJPEG;
	inc_srcbuf_ref(iav->stream[id].srcbuf);

	iav_debug ("cmd_code = 0x%08X, Stream ID [%c]\n", dsp_cmd->cmd_code, 'A' + id);

	return 0;
}

static int parse_cmd_encode_param(struct ambarella_iav *iav, DSP_CMD *cmd)
{
	int i;
	int id = 0;
	ipcam_real_time_encode_param_setup_t *dsp_cmd = (ipcam_real_time_encode_param_setup_t *)cmd;
	id = GET_STRAME_ID_CODE(dsp_cmd->cmd_code);

	if (dsp_cmd->enable_flags & REALTIME_PARAM_QP_LIMIT_BIT) {
		iav->stream[id].h264_config.qp_max_on_I = dsp_cmd->qp_max_on_I;
		iav->stream[id].h264_config.qp_max_on_P = dsp_cmd->qp_max_on_P;
		iav->stream[id].h264_config.qp_max_on_B = dsp_cmd->qp_max_on_B;
		iav->stream[id].h264_config.qp_min_on_I = dsp_cmd->qp_min_on_I;
		iav->stream[id].h264_config.qp_min_on_P = dsp_cmd->qp_min_on_P;
		iav->stream[id].h264_config.qp_min_on_B = dsp_cmd->qp_min_on_B;
		iav->stream[id].h264_config.adapt_qp = dsp_cmd->aqp;
		iav->stream[id].h264_config.i_qp_reduce = dsp_cmd->i_qp_reduce;
		iav->stream[id].h264_config.p_qp_reduce = dsp_cmd->p_qp_reduce;
		iav->stream[id].h264_config.skip_flag = dsp_cmd->skip_flags;
	}
	if (dsp_cmd->enable_flags & REALTIME_PARAM_GOP_BIT) {
		iav->stream[id].h264_config.idr_interval = dsp_cmd->idr_interval;
	}
	if (dsp_cmd->enable_flags & REALTIME_PARAM_QP_ROI_MATRIX_BIT) {
		if ((dsp_cmd->roi_daddr) &&
			(dsp_cmd->roi_daddr) &&
			(dsp_cmd->roi_daddr)) {
			iav->stream[id].h264_config.qp_roi_enable = 1;
		}
		for (i = 0; i < NUM_PIC_TYPES; ++i) {
			iav->stream[id].h264_config.qp_delta[i][0] = dsp_cmd->roi_delta[i][0];
			iav->stream[id].h264_config.qp_delta[i][1] = dsp_cmd->roi_delta[i][1];
			iav->stream[id].h264_config.qp_delta[i][2] = dsp_cmd->roi_delta[i][2];
			iav->stream[id].h264_config.qp_delta[i][3] = dsp_cmd->roi_delta[i][3];
		}
		iav->stream[id].h264_config.user1_intra_bias = dsp_cmd->user1_intra_bias;
		iav->stream[id].h264_config.user1_direct_bias = dsp_cmd->user1_direct_bias;
		iav->stream[id].h264_config.user2_intra_bias = dsp_cmd->user2_intra_bias;
		iav->stream[id].h264_config.user2_direct_bias = dsp_cmd->user2_direct_bias;
		iav->stream[id].h264_config.user3_intra_bias = dsp_cmd->user3_intra_bias;
		iav->stream[id].h264_config.user3_direct_bias = dsp_cmd->user3_direct_bias;
	}
	if (dsp_cmd->enable_flags & REALTIME_PARAM_MONOCHROME_BIT) {
		if (iav->stream[id].format.type == IAV_STREAM_TYPE_H264) {
			iav->stream[id].h264_config.chroma_format = dsp_cmd->is_monochrome;
		} else {
			if (dsp_cmd->is_monochrome) {
				iav->stream[id].mjpeg_config.chroma_format = JPEG_CHROMA_MONO;
			}
		}
	}
	if (dsp_cmd->enable_flags & REALTIME_PARAM_INTRA_BIAS_BIT) {
		iav->stream[id].h264_config.intrabias_p = dsp_cmd->P_IntraBiasAdd;
		iav->stream[id].h264_config.intrabias_b = dsp_cmd->B_IntraBiasAdd;
	}
	if (dsp_cmd->enable_flags & REALTIME_PARAM_ZMV_THRESHOLD_BIT) {
		iav->stream[id].h264_config.zmv_threshold = dsp_cmd->zmv_threshold;
	}
	if (dsp_cmd->enable_flags & REALTIME_PARAM_FLAT_AREA_BIT) {
		iav->stream[id].h264_config.enc_improve = dsp_cmd->flat_area_improvement_on;
	}
	if (dsp_cmd->enable_flags & REALTIME_PARAM_FRAME_DROP_BIT) {
		iav->stream[id].h264_config.drop_frames = dsp_cmd->drop_frame;
	}
	/* TODO: dsp_cmd->custom_encoder_frame_rate
			 dsp_cmd->custom_vin_frame_rate
			 dsp_cmd->idsp_frame_rate_M
			 dsp_cmd->idsp_frame_rate_N */

	iav_debug ("cmd_code = 0x%08X, Stream ID [%c]\n", dsp_cmd->cmd_code, 'A' + id);

	return 0;
}

/* store dsp cmd in dsp init data on amboot, restore it after enter Linux IAV */
int parse_dsp_cmd_to_iav(struct ambarella_iav *iav)
{
	int i = 0;
	int rval = 0;
	u32 cmd_data_phys = 0;
	u8 *cmd_data_virt = NULL;
	DSP_HEADER_CMD *cmd_hdr = NULL;
	DSP_CMD *cmd = NULL;
	dsp_init_data_t *dsp_init_data = NULL;

	dsp_init_data = iav->dsp->get_dsp_init_data(iav->dsp);
	cmd_data_phys = DSP_TO_PHYS(dsp_init_data->cmd_data_ptr);
	cmd_data_virt = ioremap_nocache(cmd_data_phys, DSP_CMD_BUF_SIZE);
	if (!cmd_data_virt) {
		iav_error("Failed to call ioremap() for dsp_cmd_data.\n");
		return -ENOMEM;
	}
	cmd_hdr = (DSP_HEADER_CMD *)cmd_data_virt;
	cmd = (DSP_CMD *)(cmd_data_virt + sizeof(DSP_HEADER_CMD));

	for (i = 0; i < cmd_hdr->num_cmds; i++, cmd++) {
		switch (cmd->cmd_code & 0x3FFFFFFF) {
		case SYSTEM_SETUP_INFO:
			rval = parse_cmd_system_info_setup(iav, cmd);
			break;
		case SET_VIN_GLOBAL_CLK:
			rval = parse_cmd_set_vin_global_config(iav, cmd);
			break;
		case VIDEO_PREPROCESSING:
			rval = parse_cmd_video_preproc(iav, cmd);
			break;
		case SET_WARP_CONTROL:
			rval = parse_set_warp_control(iav, cmd);
			break;
		case IPCAM_VIDEO_SYSTEM_SETUP:
			rval = parse_cmd_video_system_setup(iav, cmd);
			break;
		case IPCAM_VIDEO_CAPTURE_PREVIEW_SIZE_SETUP:
			rval = parse_cmd_capture_buffer_setup(iav, cmd);
			break;
		case IPCAM_VIDEO_ENCODE_SIZE_SETUP:
			rval = parse_cmd_encode_size_setup(iav, cmd);
			break;
		case H264_ENCODING_SETUP:
			rval = parse_cmd_h264_encode_setup(iav, cmd);
			break;
		case H264_ENCODE:
			rval = parse_cmd_h264_encode_start(iav, cmd);
			break;
		case JPEG_ENCODING_SETUP:
			rval = parse_cmd_jpeg_encode_setup(iav, cmd);
			break;
		case MJPEG_ENCODE:
			rval = parse_cmd_jpeg_encode_start(iav, cmd);
			break;
		case IPCAM_REAL_TIME_ENCODE_PARAM_SETUP:
			rval = parse_cmd_encode_param(iav, cmd);
			break;
		default:
			break;
		}

		if (rval < 0) {
			iav_error("Parse dsp cmd to iav failed\n");
			break;
		}
	}

	if (cmd_data_virt) {
		iounmap((void __iomem *)cmd_data_virt);
		cmd_data_virt = NULL;
	}
	return rval;
}

