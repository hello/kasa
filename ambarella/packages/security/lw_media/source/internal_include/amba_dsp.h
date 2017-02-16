/*
 * amba_dsp.h
 *
 * History:
 *    2015/07/31 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __AMBA_DSP_H__
#define __AMBA_DSP_H__

#define DAMBADSP_MAX_DECODER_NUMBER 1
#define DAMBADSP_MAX_DECODE_VOUT_NUMBER 4

#define DAMBA_GOP_HEADER_LENGTH 22
//for deliemiter, eos, etc
#define DAMBA_RESERVED_SPACE 32

enum {
    EAMDSP_VOUT_TYPE_INVALID = 0x00,
    EAMDSP_VOUT_TYPE_DIGITAL = 0x01,
    EAMDSP_VOUT_TYPE_HDMI = 0x02,
    EAMDSP_VOUT_TYPE_CVBS = 0x03,
};

enum {
    EAMDSP_VIDEO_CODEC_TYPE_INVALID = 0x00,
    EAMDSP_VIDEO_CODEC_TYPE_H264 = 0x01,
    EAMDSP_VIDEO_CODEC_TYPE_H265 = 0x02,
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

typedef struct {
    TU8 max_frm_num;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;

    TU16    max_frm_width;
    TU16    max_frm_height;
} SAmbaDSPDecoderConfig;

typedef struct {
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
    TU8 num_decoder;

    SAmbaDSPDecoderConfig decoder_configs[DAMBADSP_MAX_DECODER_NUMBER];
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
    TU8 setup_done;

    TU32    width;
    TU32    height;

    SAmbaDSPDecVoutConfig vout_configs[DAMBADSP_MAX_DECODE_VOUT_NUMBER];

    TU32    bsb_start_offset;
    TU32    bsb_size;
} SAmbaDSPDecoderInfo;

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
    int sink_id;
    int sink_type;
    int source_id;

    int rotate;
    int flip;
    int offset_x;
    int offset_y;
    int width;
    int height;
    unsigned int mode;
} SAmbaDSPVoutInfo;

typedef struct {
    TU8 b_two_times;
    TU8 b_enable_read;
    TU8 b_enable_write;
    TU8 reserved0;

    void *base;
    TU32 size;
} SAmbaDSPMapBSB;

typedef struct {
    TU32 stream_id;
    TU32 offset;
    TU32 size;
    TU64 pts;

    TU32 video_width;
    TU32 video_height;
} SAmbaDSPReadBitstream;

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

typedef TInt(*TFDSPEnterDecodeMode)(TInt iav_fd, SAmbaDSPDecodeModeConfig *mode_config);
typedef TInt(*TFDSPLeaveDecodeMode)(TInt iav_fd);
typedef TInt(*TFDSPCreateDecoder)(TInt iav_fd, SAmbaDSPDecoderInfo *p_decoder_info);
typedef TInt(*TFDSPDestroyDecoder)(TInt iav_fd, TU8 decoder_id);

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

typedef TInt(*TFDSPGetVoutInfo)(int iav_fd, int index, int type, SAmbaDSPVoutInfo *voutinfo);

typedef TInt(*TFDSPMapBSB)(int iav_fd, SAmbaDSPMapBSB *map_bsb);

typedef TInt(*TFDSPReadBitstream)(int iav_fd, SAmbaDSPReadBitstream *read_bitstream);

typedef struct {
    TFDSPEnterDecodeMode f_enter_mode;
    TFDSPLeaveDecodeMode f_leave_mode;
    TFDSPCreateDecoder f_create_decoder;
    TFDSPDestroyDecoder f_destroy_decoder;

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

    TFDSPGetVoutInfo f_get_vout_info;

    TFDSPMapBSB f_map_bsb;

    TFDSPReadBitstream f_read_bitstream;
} SFAmbaDSPDecAL;

extern void gfSetupDSPAlContext(SFAmbaDSPDecAL *al);

void gfFillAmbaGopHeader(TU8 *p_gop_header, TU32 frame_tick, TU32 time_scale, TU32 pts, TU8 gopsize, TU8 m);
void gfUpdateAmbaGopHeader(TU8 *p_gop_header, TU32 pts);

#endif

