/*
 * iav_dsp_cmd.h
 *
 * History:
 *	2013/04/24 - [Cao Rongrong] created file
 *	2013/12/12 - [Jian Tang] Modified file
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


#ifndef __IAV_DSP_CMD__
#define __IAV_DSP_CMD__

struct amb_dsp_cmd;

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
int cmd_apply_frame_sync_cmd(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, struct iav_apply_frame_sync * apply, u32 stream_map);

int cmd_get_frm_buf_pool_info(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u32 buf_map);
int cmd_efm_creat_frm_buf_pool(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);
int cmd_efm_req_frame_buf(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);
int cmd_efm_handshake(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u8 is_last_frame);

int cmd_hash_verification(struct ambarella_iav *iav, u8* input);
int cmd_hash_verification_long(struct ambarella_iav *iav, u8* input);

int cmd_reset_operation(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd);

int cmd_h264_decoder_setup(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u8 decoder_id);
int cmd_decode_stop(struct ambarella_iav *iav, u8 decoder_id, u8 stop_flag);
int cmd_vout_video_setup(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd,
	struct iav_decode_vout_config *p_vout_config, u8 src, u8 is_interlace);
int cmd_rescale_postp(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd,
	u8 decoder_id, u16 center_x, u16 center_y);
int cmd_playback_speed(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd,
	u8 decoder_id, u16 speed, u8 scan_mode, u8 direction);
int cmd_h264_decode(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd,
	u8 decoder_id, u32 bits_fifo_start, u32 bits_fifo_end, u32 num_frames, u32 first_frame_display);
int cmd_h264_decode_fifo_update(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd,
	u8 decoder_id, u32 bits_fifo_start, u32 bits_fifo_end, u32 num_frames);
int cmd_h264_trick_play(struct ambarella_iav *iav, struct amb_dsp_cmd *cmd, u8 decoder_id, u8 mode);

/* used for Fastboot case */
int parse_dsp_cmd_to_iav(struct ambarella_iav *iav);

#endif

