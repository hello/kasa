/**
 * am_amba_dsp.h
 *
 * History:
 *    2015/07/31 - [Zhi He] create file
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
 */

#ifndef __AM_AMBA_DSP_H__
#define __AM_AMBA_DSP_H__

#define DAMBADSP_MAX_DECODER_NUMBER 1

#define DAMBADSP_MAX_INTRA_DECODE_CMD_NUMBER 4
#define DAMBADSP_MAX_INTRA_FB_NUMBER 4
#define DAMBADSP_MAX_INTRA_YUV2YUV_DST_FB_NUMBER 3

#define DAMBA_H264_GOP_HEADER_LENGTH 22
#define DAMBA_H265_GOP_HEADER_LENGTH 23
#define DAMBA_MAX_GOP_HEADER_LENGTH 24

#define DAMBA_RESERVED_SPACE 32

#define DDECODER_STOPPED (-1678)

enum {
  EAMDSP_VIDEO_CODEC_TYPE_INVALID = 0x00,
  EAMDSP_VIDEO_CODEC_TYPE_H264 = 0x01,
  EAMDSP_VIDEO_CODEC_TYPE_H265 = 0x02,
  EAMDSP_VIDEO_CODEC_TYPE_MJPEG = 0x03,
};

enum {
  EAMDSP_BUFFER_PIX_FMT_420 = 1,
  EAMDSP_BUFFER_PIX_FMT_422 = 2,
};

enum {
  EAMDSP_TRICK_PLAY_PAUSE = 0,
  EAMDSP_TRICK_PLAY_RESUME = 1,
  EAMDSP_TRICK_PLAY_STEP = 2,
};

enum {
  EAMDSP_PB_DIRECTION_FW = 0,
  EAMDSP_PB_DIRECTION_BW = 1,
};

enum {
  EAMDSP_PB_SCAN_MODE_ALL_FRAMES = 0,
  EAMDSP_PB_SCAN_MODE_I_ONLY = 1,
};

enum {
  EAMDSP_ENC_STREAM_STATE_IDLE = 0,
  EAMDSP_ENC_STREAM_STATE_STARTING = 1,
  EAMDSP_ENC_STREAM_STATE_ENCODING = 2,
  EAMDSP_ENC_STREAM_STATE_STOPPING = 3,
  EAMDSP_ENC_STREAM_STATE_UNKNOWN = 4,
  EAMDSP_ENC_STREAM_STATE_ERROR = 5,
};

enum {
  EAMDSP_ENC_STREAM_SOURCE_BUFFER_MAIN = 0,
  EAMDSP_ENC_STREAM_SOURCE_BUFFER_SECOND = 1,
  EAMDSP_ENC_STREAM_SOURCE_BUFFER_THIRD = 2,
  EAMDSP_ENC_STREAM_SOURCE_BUFFER_FOURTH = 3,
  EAMDSP_ENC_STREAM_SOURCE_BUFFER_FIFTH = 4,
  EAMDSP_ENC_STREAM_SOURCE_BUFFER_EFM = 5,

  EAMDSP_ENC_STREAM_SOURCE_BUFFER_INVALID = 255,
};

typedef struct {
  TU32 dsp_mode;
} SAmbaDSPMode;

typedef struct {
  TU8 b_support_ff_fb_bw;
  TU8 debug_max_frame_per_interrupt;
  TU8 debug_use_dproc;
  TU8 num_decoder;

  TU16 max_gop_size;
  TU8 vout_mask;
  TU8 reserved0;

  TU32 max_frm_width;
  TU32 max_frm_height;

  TU32 max_vout0_width;
  TU32 max_vout0_height;

  TU32 max_vout1_width;
  TU32 max_vout1_height;
} SAmbaDSPDecodeModeConfig;

typedef struct {
  TU8 vout_id;
  TU8 enable;
  TU8 flip;
  TU8 rotate;

  TU16    target_win_offset_x;
  TU16    target_win_offset_y;

  TU16    target_win_width;
  TU16    target_win_height;

  TU32    zoom_factor_x;
  TU32    zoom_factor_y;
  TU32    vout_mode;
} SAmbaDSPDecVoutConfig;

typedef struct {
  TU8 decoder_id;
  TU8 decoder_type;
  TU8 num_vout;
  TU8 b_use_addr;

  TU32    width;
  TU32    height;

  SAmbaDSPDecVoutConfig vout_configs[DAMBADSP_MAX_VOUT_NUMBER];

  TU32    bsb_start_offset;
  TU32    bsb_size;
} SAmbaDSPDecoderInfo;

typedef struct {
  TU8 auto_map_bsb;
  TU8 rendering_monitor_mode;
  TU8 reserved0;
  TU8 reserved1;
} SAmbaDSPQueryDecodeConfig;

typedef struct {
  TU8  decoder_id;
  TU8  num_frames;
  TU8  reserved1;
  TU8  reserved2;

  TU32 start_ptr_offset;
  TU32 end_ptr_offset;

  TU32 first_frame_display;
} SAmbaDSPDecode;

typedef struct {
  TInt sink_id;
  TInt sink_type;
  TInt source_id;

  TInt rotate;
  TInt flip;
  TInt offset_x;
  TInt offset_y;
  TInt width;
  TInt height;
  TU32 mode;
} SAmbaDSPVoutInfo;

typedef struct {
  TU32 width;
  TU32 height;
  TU32 fps;

  TU32 fr_num;
  TU32 fr_den;

  TU8  format;
  TU8  type;
  TU8  bits;
  TU8  ratio;

  TU8  system;
  TU8  flip;
  TU8  rotate;
  TU8  pattern;
} SAmbaDSPVinInfo;

typedef struct {
  TU16 framefactor_num;
  TU16 framefactor_den;
} SAmbaDSPStreamFramefactor;

typedef struct {
  TU8 b_two_times;
  TU8 b_enable_read;
  TU8 b_enable_write;
  TU8 reserved0;

  void *base;
  TU32 size;
} SAmbaDSPMapBSB;

typedef struct {
  void *base;
  TU32 size;
} SAmbaDSPMapDSP;

typedef struct {
  void *base;
  TU32 size;
} SAmbaDSPMapIntraPB;

typedef struct {
  TU32 yuv_buf_num;
  TU32 yuv_pitch;
  TU32 me1_buf_num;
  TU32 me1_pitch;

  TU32 yuv_buf_width;
  TU32 yuv_buf_height;
  TU32 me1_buf_width;
  TU32 me1_buf_height;

  TU8 b_use_addr;
  TU8 reserved0;
  TU8 reserved1;
  TU8 reserved2;
} SAmbaEFMPoolInfo;

typedef struct {
  TU32 frame_idx;
  TPointer yuv_luma_offset;
  TPointer yuv_chroma_offset;
  TPointer me1_offset;
  TPointer me0_offset;
} SAmbaEFMFrame;

typedef struct {
  TU32 stream_id;
  TU32 frame_idx;
  TU32 frame_pts;

  TU8 b_not_wait_next_interrupt;
  TU8 is_last_frame;
  TU8 use_hw_pts;
  TU8 reserved2;

  TU32 buffer_width;
  TU32 buffer_height;
} SAmbaEFMFinishFrame;

typedef struct {
  TU32 stream_id;

  TU32 offset;
  TU32 size; // 0 means stream end
  TU64 pts;

  TU32 video_width;
  TU32 video_height;

  TU32 stream_type;

  TU8 slice_id;
  TU8 slice_num;
  TU8 hint_frame_type;
  TU8 hint_is_keyframe;

  TU32 timeout_ms;
} SAmbaDSPReadBitstream;

typedef struct {
  TU8  decoder_id;
  TU8  max_num;
  TU8  reserved0;
  TU8  reserved1;
} SAmbaDSPIntraplayResetBuffers;

typedef struct {
  TU8  buffer_id;
  TU8  ch_fmt;

  TU16 buf_pitch;

  TU16 buf_width;
  TU16 buf_height;

  TU32 lu_buf_offset;
  TU32 ch_buf_offset;

  TU16 img_width;
  TU16 img_height;
  TU16 img_offset_x;
  TU16 img_offset_y;

  TU32 buffer_size;
} SAmbaDSPIntraplayBuffer;

typedef struct {
  TU8  vout_id;
  TU8  vid_win_update;
  TU8  vid_win_rotate;
  TU8  vid_flip;

  TU16 vid_win_width;
  TU16 vid_win_height;
  TU16 vid_win_offset_x;
  TU16 vid_win_offset_y;
} SAmbaDSPIntraplayDisplayDesc;

typedef struct {
  TU32 bits_fifo_start;
  TU32 bits_fifo_end;
} SAmbaDSPIntraplayBitstream;

typedef struct {
  TU8  decoder_id;
  TU8  num;
  TU8  decode_type;
  TU8  reserved2;

  SAmbaDSPIntraplayBitstream bitstreams[DAMBADSP_MAX_INTRA_DECODE_CMD_NUMBER];
  SAmbaDSPIntraplayBuffer buffers[DAMBADSP_MAX_INTRA_DECODE_CMD_NUMBER];
} SAmbaDSPIntraplayDecode;

typedef struct {
  TU8  decoder_id;
  TU8  num;
  TU8  reserved0;
  TU8  reserved1;

  TU8  rotate;
  TU8  flip;
  TU8  luma_gain;
  TU8  reserved2;

  SAmbaDSPIntraplayBuffer src_buf;
  SAmbaDSPIntraplayBuffer dst_buf[DAMBADSP_MAX_INTRA_YUV2YUV_DST_FB_NUMBER];
} SAmbaDSPIntraplayYUV2YUV;

typedef struct {
  TU8  decoder_id;
  TU8  num;
  TU8  reserved1;
  TU8  reserved2;

  SAmbaDSPIntraplayBuffer buffers[DAMBADSP_MAX_INTRA_DECODE_CMD_NUMBER];
  SAmbaDSPIntraplayDisplayDesc desc[DAMBADSP_MAX_INTRA_DECODE_CMD_NUMBER];
} SAmbaDSPIntraplayDisplay;

typedef struct {
  TU8 decoder_id;
  TU8 reserved0;
  TU8 reserved1;
  TU8 reserved2;

  TU32  start_offset;
  TU32  room;

  TU32  dsp_read_offset;
  TU32  free_room;
} SAmbaDSPBSBStatus;

typedef struct {
  TU8 decoder_id;
  TU8 reserved0;
  TU8 is_started;
  TU8 is_send_stop_cmd;

  TU32  last_pts;

  TU32  decode_state;
  TU32  error_status;
  TU32  total_error_count;
  TU32  decoded_pic_number;

  TU32  write_offset;
  TU32  room;
  TU32  dsp_read_offset;
  TU32  free_room;

  TU32  irq_count;
  TU32  yuv422_y_addr;
  TU32  yuv422_uv_addr;
} SAmbaDSPDecodeStatus;

typedef struct {
  TU8 decoder_id;
  TU8 reserved0;
  TU8 reserved1;
  TU8 reserved2;

  TU32 last_pts_high;
  TU32 last_pts_low;
} SAmbaDSPDecodeEOSTimestamp;

typedef struct {
  TU8 buf_id;
  TU8 reserved0;
  TU8 reserved1;
  TU8 reserved2;

  TU32 size_width;
  TU32 size_height;

  TU32 crop_size_x;
  TU32 crop_size_y;
  TU32 crop_pos_x;
  TU32 crop_pos_y;
} SAmbaDSPSourceBufferInfo;

typedef struct {
  TU8 buf_id;
  TU8 non_block_flag;
  TU8 vca_flag;
  TU8 reserved0;

  TU32 y_addr_offset;
  TU32 uv_addr_offset;
  TU32 width;
  TU32 height;
  TU32 pitch;
  TU32 seq_num;
  TU32 format;

  TU32 dsp_pts;
  TU64 mono_pts;
} SAmbaDSPQueryYUVBuffer;

typedef struct {
  TU8 id;
  TU8 state; // EAMDSP_ENC_STREAM_STATE_xx
  TU8 reserved1;
  TU8 reserved2;
} SAmbaDSPEncStreamInfo;

typedef struct {
  TU8 id;
  TU8 codec; // EAMDSP_VIDEO_CODEC_TYPE_xx
  TU8 source_buffer; // EAMDSP_ENC_STREAM_SOURCE_BUFFER_xx
  TU8 reserved2;
} SAmbaDSPEncStreamFormat;

typedef enum {
  EAmbaBufferType_DSP = 0,
  EAmbaBufferType_BSB = 1,
  EAmbaBufferType_USR = 2,
  EAmbaBufferType_MV = 3,
  EAmbaBufferType_OVERLAY = 4,
  EAmbaBufferType_QPMATRIX = 5,
  EAmbaBufferType_WARP = 6,
  EAmbaBufferType_QUANT = 7,
  EAmbaBufferType_IMG = 8,
  EAmbaBufferType_PM_IDSP = 9,
  EAmbaBufferType_CMD_SYNC = 10,
  EAmbaBufferType_FB_DATA = 11,
  EAmbaBufferType_FB_AUDIO = 12,
  EAmbaBufferType_QPMATRIX_RAW = 13,
  EAmbaBufferType_INTRA_PB = 14,
  EAmbaBufferType_SBP = 15,
  EAmbaBufferType_MULTI_CHAN = 16,
} EAmbaBufferType;

typedef struct {
  TU32 src_offset;
  TU32 dst_offset;
  TU32 src_pitch;
  TU32 dst_pitch;
  TU32 width;
  TU32 height;
  TU16 src_mmap_type;
  TU16 dst_mmap_type;
} SAmbaGDMACopy;

typedef TInt(*TFDSPGetDSPMode)(TInt iav_fd, SAmbaDSPMode *mode);

typedef TInt(*TFDSPEnterDecodeMode)(TInt iav_fd, SAmbaDSPDecodeModeConfig *mode_config);
typedef TInt(*TFDSPLeaveDecodeMode)(TInt iav_fd);
typedef TInt(*TFDSPCreateDecoder)(TInt iav_fd, SAmbaDSPDecoderInfo *p_decoder_info);
typedef TInt(*TFDSPDestroyDecoder)(TInt iav_fd, TU8 decoder_id);
typedef TInt(*TFDSPQueryDecodeConfig)(int iav_fd, SAmbaDSPQueryDecodeConfig *config);

typedef TInt(*TFDSPDecodeTrickPlay)(int iav_fd, TU8 decoder_id, TU8 trick_play);
typedef TInt(*TFDSPDecodeStart)(int iav_fd, TU8 decoder_id);
typedef TInt(*TFDSPDecodeStop)(int iav_fd, TU8 decoder_id, TU8 stop_flag);
typedef TInt(*TFDSPDecodeSpeed)(int iav_fd, TU8 decoder_id, TU16 speed, TU8 scan_mode, TU8 direction);
typedef TInt(*TFDSPDecodeRequestBitsFifo)(int iav_fd, int decoder_id, TU32 size, void *cur_pos_offset);

typedef TInt(*TFDSPDecode)(int iav_fd, SAmbaDSPDecode *dec);

typedef TInt(*TFDSPDecodeQueryBSBAndPrint)(int iav_fd, TU8 decoder_id);
typedef TInt(*TFDSPDecodeQueryStatusAndPrint)(int iav_fd, TU8 decoder_id);
typedef TInt(*TFDSPDecodeQueryBSB)(int iav_fd, SAmbaDSPBSBStatus *status);
typedef TInt(*TFDSPDecodeQueryStatus)(int iav_fd, SAmbaDSPDecodeStatus *status);

typedef TInt(*TFDSPDecodeWaitVoutDormant)(int iav_fd, TU8 decoder_id);
typedef TInt(*TFDSPDecodeWakeVout)(int iav_fd, TU8 decoder_id);
typedef TInt(*TFDSPDecodeWaitEOSFlag)(int iav_fd, SAmbaDSPDecodeEOSTimestamp *eos_timestamp);
typedef TInt(*TFDSPDecodeWaitEOS)(int iav_fd, SAmbaDSPDecodeEOSTimestamp *eos_timestamp);

typedef TInt(*TFDSPConfigureVout)(int iav_fd, SVideoOutputConfigure *vout_config);

typedef TInt(*TFDSPGetVoutInfo)(int iav_fd, int index, int type, SAmbaDSPVoutInfo *voutinfo);
typedef TInt(*TFDSPGetVinInfo)(int iav_fd, SAmbaDSPVinInfo *vintinfo);

typedef TInt(*TFDSPGetStreamFrameFactor)(int iav_fd, int index, SAmbaDSPStreamFramefactor *framefactor);

typedef TInt(*TFDSPMapBSB)(int iav_fd, SAmbaDSPMapBSB *map_bsb);
typedef TInt(*TFDSPMapDSP)(int iav_fd, SAmbaDSPMapDSP *map_dsp);
typedef TInt(*TFDSPMapDSPIntraPB)(int iav_fd, SAmbaDSPMapIntraPB *map_intrapb);

typedef TInt(*TFDSPUnmapBSB)(int iav_fd, SAmbaDSPMapBSB *map_bsb);
typedef TInt(*TFDSPUnmapDSP)(int iav_fd, SAmbaDSPMapDSP *map_dsp);
typedef TInt(*TFDSPUnmapDSPIntraPB)(int iav_fd, SAmbaDSPMapIntraPB *map_intrapb);

typedef TInt(*TFDSPEFMGetBufferPoolInfo)(int iav_fd, SAmbaEFMPoolInfo *buffer_pool_info);
typedef TInt(*TFDSPEFMRequestFrame)(int iav_fd, SAmbaEFMFrame *frame);
typedef TInt(*TFDSPEFMFinishFrame)(int iav_fd, SAmbaEFMFinishFrame *finish_frame);

typedef TInt(*TFDSPReadBitstream)(int iav_fd, SAmbaDSPReadBitstream *read_bitstream);
typedef TInt(*TFDSPIsReadyForReadBitstream)(int iav_fd);

typedef TInt(*TFDSPIntraplayResetBuffers)(int iav_fd, SAmbaDSPIntraplayResetBuffers *reset_buffers);
typedef TInt(*TFDSPIntraplayRequestBuffer)(int iav_fd, SAmbaDSPIntraplayBuffer *buffer);
typedef TInt(*TFDSPIntraplayDecode)(int iav_fd, SAmbaDSPIntraplayDecode *decode);
typedef TInt(*TFDSPIntraplayYUV2YUV)(int iav_fd, SAmbaDSPIntraplayYUV2YUV *yuv2yuv);
typedef TInt(*TFDSPInytaplayDisplay)(int iav_fd, SAmbaDSPIntraplayDisplay *display);

typedef TInt(*TFDSPEncodeStart)(int iav_fd, TU32 mask);
typedef TInt(*TFDSPEncodeStop)(int iav_fd, TU32 mask);

typedef TInt(*TFDSPQueryEncodeStreamInfo)(int iav_fd, SAmbaDSPEncStreamInfo *info);
typedef TInt(*TFDSPQueryEncodeStreamFormat)(int iav_fd, SAmbaDSPEncStreamFormat *fmt);

typedef TInt(*TFDSPQuerySourceBufferInfo)(int iav_fd, SAmbaDSPSourceBufferInfo *info);
typedef TInt(*TFDSPQueryYUVBuffer)(int iav_fd, SAmbaDSPQueryYUVBuffer *yuv_buffer);

typedef TInt(*TFDSPGDMACopy)(int iav_fd, SAmbaGDMACopy *copy);

typedef TInt(*TFDSPEnterIdleMode)(int iav_fd);

typedef struct {
  TFDSPGetDSPMode f_get_dsp_mode;

  TFDSPEnterDecodeMode f_enter_mode;
  TFDSPLeaveDecodeMode f_leave_mode;
  TFDSPCreateDecoder f_create_decoder;
  TFDSPDestroyDecoder f_destroy_decoder;
  TFDSPQueryDecodeConfig f_query_decode_config;

  TFDSPDecodeTrickPlay f_trickplay;
  TFDSPDecodeStart f_start;
  TFDSPDecodeStop f_stop;
  TFDSPDecodeSpeed f_speed;
  TFDSPDecodeRequestBitsFifo f_request_bsb;

  TFDSPDecode f_decode;

  TFDSPDecodeQueryBSBAndPrint f_query_print_decode_bsb_status;
  TFDSPDecodeQueryStatusAndPrint f_query_print_decode_status;
  TFDSPDecodeQueryBSB f_query_decode_bsb_status;
  TFDSPDecodeQueryStatus f_query_decode_status;

  TFDSPDecodeWaitVoutDormant f_decode_wait_vout_dormant;
  TFDSPDecodeWakeVout f_decode_wake_vout;
  TFDSPDecodeWaitEOSFlag f_decode_wait_eos_flag;
  TFDSPDecodeWaitEOS f_decode_wait_eos;

  TFDSPConfigureVout f_configure_vout;

  TFDSPGetVoutInfo f_get_vout_info;
  TFDSPGetVinInfo f_get_vin_info;

  TFDSPGetStreamFrameFactor f_get_stream_framefactor;

  TFDSPMapBSB f_map_bsb;
  TFDSPMapDSP f_map_dsp;
  TFDSPMapDSPIntraPB f_map_intrapb;

  TFDSPUnmapBSB f_unmap_bsb;
  TFDSPUnmapDSP f_unmap_dsp;
  TFDSPUnmapDSPIntraPB f_unmap_intrapb;

  TFDSPEFMGetBufferPoolInfo f_efm_get_buffer_pool_info;
  TFDSPEFMRequestFrame f_efm_request_frame;
  TFDSPEFMFinishFrame f_efm_finish_frame;

  TFDSPReadBitstream f_read_bitstream;
  TFDSPIsReadyForReadBitstream f_is_ready_for_read_bitstream;

  TFDSPIntraplayResetBuffers f_intraplay_reset_buffers;
  TFDSPIntraplayRequestBuffer f_intraplay_request_buffer;
  TFDSPIntraplayDecode f_intraplay_decode;
  TFDSPIntraplayYUV2YUV f_intraplay_yuv2yuv;
  TFDSPInytaplayDisplay f_intraplay_display;

  TFDSPEncodeStart f_encode_start;
  TFDSPEncodeStop f_encode_stop;

  TFDSPQueryEncodeStreamInfo f_query_encode_stream_info;
  TFDSPQueryEncodeStreamFormat f_query_encode_stream_fmt;

  TFDSPQuerySourceBufferInfo f_query_source_buffer_info;
  TFDSPQueryYUVBuffer f_query_yuv_buffer;

  TFDSPGDMACopy f_gdma_copy;

  TFDSPEnterIdleMode f_enter_idle_mode;
} SFAmbaDSPDecAL;

extern void gfSetupDSPAlContext(SFAmbaDSPDecAL *al);

extern EECode gfDSPConfigureVOUT(SVideoOutputConfigures *vouts);

void gfFillAmbaH264GopHeader(TU8 *p_gop_header, TU32 frame_tick, TU32 time_scale, TU32 pts, TU8 gopsize, TU8 m);
void gfUpdateAmbaH264GopHeader(TU8 *p_gop_header, TU32 pts, TU8 gopsize);

void gfFillAmbaH265GopHeader(TU8 *p_gop_header, TU32 frame_tick, TU32 time_scale, TU32 pts, TU8 gopsize, TU8 m);
void gfUpdateAmbaH265GopHeader(TU8 *p_gop_header, TU32 pts, TU8 gopsize);

#endif

