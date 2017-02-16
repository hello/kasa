/*
 * iav_dsp_cmd.h
 *
 * History:
 *	2013/04/24 - [Cao Rongrong] created file
 *	2013/12/12 - [Jian Tang] Modified file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_DSP_CMD__
#define __IAV_DSP_CMD__

struct amb_dsp_cmd;

enum {
	REALTIME_PARAM_CBR_MODIFY_BIT = (1 << 0),
	REALTIME_PARAM_CUSTOM_FRAME_RATE_BIT = (1 << 1),
	REALTIME_PARAM_QP_LIMIT_BIT = (1 << 2),
	REALTIME_PARAM_GOP_BIT = (1 << 3),
	REALTIME_PARAM_CUSTOM_VIN_FPS_BIT = (1 << 4),
	REALTIME_PARAM_INTRA_MB_ROWS_BIT = (1 << 5),
	REALTIME_PARAM_PREVIEW_A_FRAME_RATE_BIT = (1 << 6),
	REALTIME_PARAM_QP_ROI_MATRIX_BIT = (1 << 7),
	REALTIME_PARAM_PANIC_MODE_BIT = (1 << 8),
	REALTIME_PARAM_QUANT_MATRIX_BIT = (1 << 9),
	REALTIME_PARAM_MONOCHROME_BIT = (1 << 10),
	REALTIME_PARAM_INTRA_BIAS_BIT = (1 << 11),
	REALTIME_PARAM_SCENE_DETECT_BIT = (1 << 12),
	REALTIME_PARAM_FORCE_IDR_BIT = (1 << 13),
	REALTIME_PARAM_ZMV_THRESHOLD_BIT = (1 << 14),
	REALTIME_PARAM_FLAT_AREA_BIT = (1 << 15),
	REALTIME_PARAM_FORCE_FAST_SEEK_BIT = (1 << 16),
	REALTIME_PARAM_FRAME_DROP_BIT = (1 << 17),
};

int cmd_chip_selection(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);
int cmd_vin_timer_mode(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);
int cmd_set_vin_global_config(struct ambarella_iav *iav,
		struct amb_dsp_cmd *cmd);
int cmd_set_vin_config(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);
int cmd_set_vin_master_config(struct ambarella_iav *iav,
		struct amb_dsp_cmd *cmd);
int cmd_raw_encode_video_setup(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);
int cmd_set_warp_control(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);
int cmd_system_info_setup(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u32 decode_flag);
int cmd_video_system_setup(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);
int cmd_video_preproc(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);
int cmd_sensor_config(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);
int cmd_capture_buffer_default_setup(struct ambarella_iav *iav,
		struct amb_dsp_cmd *cmd, int id);
int cmd_capture_buffer_setup(struct ambarella_iav *iav,
		struct amb_dsp_cmd *cmd, int id);
int cmd_encode_size_setup(struct iav_stream *stream, struct amb_dsp_cmd *cmd);
int cmd_h264_encode_setup(struct iav_stream *stream, struct amb_dsp_cmd *cmd);
int cmd_h264_encode_start(struct iav_stream *stream, struct amb_dsp_cmd *cmd);
int cmd_jpeg_encode_setup(struct iav_stream *stream, struct amb_dsp_cmd *cmd);
int cmd_jpeg_encode_start(struct iav_stream *stream, struct amb_dsp_cmd *cmd);
int cmd_encode_stop(struct iav_stream *stream, struct amb_dsp_cmd *cmd);
int cmd_update_encode_params(struct iav_stream *stream,
		struct amb_dsp_cmd *cmd, u32 flags);
// used for frame sync case
int update_sync_encode_params(struct iav_stream *stream,
		struct amb_dsp_cmd *cmd, u32 flags);
int cmd_update_idsp_factor(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);

int cmd_overlay_insert(struct iav_stream *stream, struct amb_dsp_cmd *cmd,
		struct iav_overlay_insert *info);
int cmd_fastosd_insert(struct iav_stream *stream, struct amb_dsp_cmd *cmd,
	struct iav_fastosd_insert *info);

int cmd_dsp_set_debug_level(struct ambarella_iav *iav,
		struct amb_dsp_cmd *cmd, u32 module, u32 debug_level, u32 mask);
int cmd_set_region_warp_control(struct ambarella_iav *iav, int area_id, int active_rid);
int cmd_set_warp_batch_cmds(struct ambarella_iav *iav, int active_num, struct amb_dsp_cmd *cmd);

int cmd_set_pm_mctf(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u32 delay);
int cmd_set_pm_bpc(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);
int cmd_bpc_setup(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, struct iav_bpc_setup *bpc_setup);
int cmd_static_bpc(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, struct iav_bpc_fpn *bpc_fpn);
int cmd_apply_frame_sync_cmd(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, struct iav_apply_frame_sync * apply);

int cmd_get_frm_buf_pool_info(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u32 buf_map);
int cmd_efm_creat_frm_buf_pool(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);
int cmd_efm_req_frame_buf(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);
int cmd_efm_handshake(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);

int cmd_hash_verification(struct ambarella_iav *iav, u8* input);

int cmd_reset_operation(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);

int cmd_h264_decoder_setup(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u8 decoder_id);
int cmd_decode_stop(struct ambarella_iav *iav, u8 decoder_id, u8 stop_flag);
int cmd_vout_video_setup(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u16 vout_id, u8 en, u8 src, u8 flip, u8 rotate, u16 offset_x, u16 offset_y, u16 width, u16 height);
int cmd_rescale_postp(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u8 decoder_id, u16 center_x, u16 center_y);
int cmd_playback_speed(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u8 decoder_id, u16 speed, u8 scan_mode, u8 direction);
int cmd_h264_decode(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u8 decoder_id, u32 bits_fifo_start, u32 bits_fifo_end, u32 num_frames, u32 first_frame_display);
int cmd_h264_decode_fifo_update(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u8 decoder_id, u32 bits_fifo_start, u32 bits_fifo_end, u32 num_frames);
int cmd_h264_trick_play(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u8 decoder_id, u8 mode);

/* used for Fastboot case */
int parse_dsp_cmd_to_iav(struct ambarella_iav *iav);

#endif

