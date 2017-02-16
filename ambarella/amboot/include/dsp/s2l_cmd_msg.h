/**
 * amboot/include/dsp/s2l_cmd_msg.h
 * sync from lib/firmware_s2l/dsp_cmd_msg.h
 *
 * History:
 *	2012/07/05 - [Jian Tang] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef	__CMD__MSG_H__
#define	__CMD__MSG_H__

#include  <basedef.h>

/* Check struct vin_video_format in private/include/vin_api.h */
struct vin_video_format {
	u32 video_mode;
	u32 device_mode;
	u32 pll_idx;	/* clock index */
	u16 width;	/* image horizontal size in unit of pixel */
	u16 height;	/* image vertical size in unit of line */

	u16 def_start_x;
	u16 def_start_y;
	u16 def_width;
	u16 def_height;
	u8 format;
	u8 type;
	u8 bits;
	u8 ratio;
	u8 bayer_pattern;
	u8 hdr_mode;
	u8 readout_mode;
	u16 sync_start;

	u32 max_fps;
	int default_fps;
	int default_agc;
	int default_shutter_time;
	int default_bayer_pattern;

	/* hdr mode related */
	u16 act_start_x;
	u16 act_start_y;
	u16 act_width;
	u16 act_height;
	u16 max_act_width;
	u16 max_act_height;
	u16 hdr_long_offset;
	u16 hdr_short1_offset;
	u16 hdr_short2_offset;
	u16 hdr_short3_offset;
	u16 dual_gain_mode;
};

#define DSP_CMD_SIZE 	128	// need to decouple if used by ARM
#define DSP_MSG_SIZE	256	// need to decouple if used by ARM

typedef struct DSP_HEADER_CMDtag{
    u32 cmd_seq_num;
    u32 num_cmds;
}DSP_HEADER_CMD;

typedef struct DSP_STATUS_MSGtag{
    u32 msg_code;
    u32 dsp_mode;
    u32 time_code;
    u32 prev_cmd_seq;
    u32 prev_cmd_echo;
    u32 prev_num_cmds;
    u32 num_msgs;
}DSP_STATUS_MSG;

typedef enum {
	DSP_ENCODE_MODE		= 0x00,
	DSP_DECODE_MODE		= 0x01,
	DSP_RESET_MODE		= 0x02,
	DSP_UNKNOWN_MODE	= 0x03
} dsp_op_mode_t;

typedef struct {
	u32	frame_num;
	u32	PTS;
	u32	start_addr;
	u32	pic_type	: 3;
	u32	level_idc	: 3;
	u32	ref_idc		: 1;
	u32	pic_struct	: 1;
	u32	pic_size	: 24;
	u32	stream_id	: 8;
	u32 session_id	: 4;
	u32	res_1		: 20;
	u32 pjpeg_start_addr;
	u32 pjpeg_size;
	u32 hw_pts;
} BIT_STREAM_HDR;

///////////////

/* General definitions */
typedef struct DSP_CMDtag{
    u32 cmd_code;
    u32 palyload[DSP_CMD_SIZE/4 -1];
}DSP_CMD;

typedef struct DSP_MSGtag{
    u32 msg_code;
    u32 palyload[DSP_MSG_SIZE/4 -1];
}DSP_MSG;

#define NUM_MSG_CAT 16

////////////////////////////////////////////////////////////////////////////////
typedef enum {
	CAT_VCAP = 0,
	CAT_ENC = 1,
	CAT_DEC = 2,
	CAT_AMB_EXP = 254,
	CAT_INVALID = 255,
} DSP_CMD_CAT;

typedef enum {
	CHIP_ID_UNKNOWN = 0xFFFF,

	/* S2L */
	CHIP_ID_S2L_33 = 0x1200,
	CHIP_ID_S2L_55 = 0x1201,
	CHIP_ID_S2L_66 = 0x1202,
	CHIP_ID_S2L_88 = 0x1203,
	CHIP_ID_S2L_99 = 0x1204,
} DSP_CHIP_ID;

/* Configuration commands */
#define NOP_CMD						0xFFFFFFFF
#define INTERRUPT_SETUP			0x00001001
#define H264_ENCODING_SETUP		0x00001002
#define JPEG_ENCODING_SETUP		0x00001003
#define H264_DECODING_SETUP		0x00001004
#define JPEG_DECODING_SETUP		0x00001005
#define RESET_OPERATION			0x00001006
#define VIDEO_OUTPUT_RESTART		0x00001007
#define H264_ENC_USE_TIMER      	0x00001008
#define CHIP_SELECTION			0x00001009
#define HD_ECHO_SETUP			0x0000100A
#define SYSTEM_SETUP_INFO		0x0000100B
#define EIS_SWITCHVOUT_DURING_ENCODE    0x0000100C
#define DSP_DEBUG_LEVEL_SETUP		0x0000100D
#define SYSTEM_PARAMETERS_SETUP		0x0000100E
#define SYSTEM_IDSP_FREQ_SETUP		0x0000100F
#define HASH_VERIFICATION               0x00001010

#define MCTF_MV_STAB_SETUP		0x0000200D
#define SET_SLOW_SHUT_UP_SAMPL_RT   	0x0000200E
#define SET_REPEAT_FRM			0x0000200F
#define MCTF_GMV_SETUP			0x00002010
#define DIS_SETUP				0x00002011	//This command is used to setup the DIS algorithm paramete and related debug stuff.
#define SET_ZOOM_FACTOR			0x00002117
#define VIDEO_PREVIEW_SETUP     0x00002119
#define ENA_SECOND_STREAM_ENCODE	0x00002125
#define MODIFY_FRAME_BUFFER     0x00002127
#define RAW_ENCODE_VIDEO_SETUP	0x00002128

// CAT_IDSP(9)

// -- 0x1001 ~ 0x1FFF   Sensor
// -- 0x2001 ~ 0x2FFF   Color
// -- 0x3001 ~ 0x3FFF   Noise
// -- 0x4001 ~ 0x4FFF   3A Statistics
// -- 0x5001 ~ 0x5FFF   Miscellaneous / Debug
// -- 0x6001 ~ 0x6FFF   Debug
#define SET_VIN_CAPTURE_WIN                 0x00091001              // 0x00091001 (0x00002100)
#define SENSOR_INPUT_SETUP                  0x00091002              // 0x00091002 (0x00002001)
#define AMPLIFIER_LINEARIZATION             0x00091003              // 0x00091003 (0x00002101)
#define PIXEL_SHUFFLE                       0x00091004              // 0x00091004 (0x00002102)
#define FIXED_PATTERN_NOISE_CORRECTION      0x00091005              // 0x00091005 (0x00002106)
#define CFA_DOMAIN_LEAKAGE_FILTER           0x00091006              // 0x00091006 (0x0000200C)
#define ANTI_ALIASING                       0x00091007              // 0x00091007 (0x0000211B)
#define SET_VIN_CONFIG                      0x00091008              // 0x00091008
#define SET_VIN_ROLLOING_SHUTTER_CONFIG     0x00091009              // 0x00091009  N/A on S2L
#define SET_VIN_CAPTURE_WIN_EXT             0x0009100A              // 0x0009100A  N/A on S2L
#define SET_VIN_PIP_CAPTURE_WIN             0x0009100B              // 0x0009100B
#define SET_VIN_PIP_CAPTURE_WIN_EXT         0x0009100C              // 0x0009100C  N/A on S2L
#define SET_VIN_PIP_CONFIG                  0x0009100D              // 0x0009100D
#define SET_VIN_MASTER_CLK                  0x0009100E              // 0x0009100E
#define SET_VIN_GLOBAL_CLK                  0x0009100F              // 0x0009100F
#define STATIC_BAD_PIXEL_CORRECTION         0x00091010


#define BLACK_LEVEL_GLOBAL_OFFSET           0x00092001              // 0x00092001 (0x0000211D)
#define BLACK_LEVEL_CORRECTION_CONFIG       0x00092002              // 0x00092002 (0x00002103)
#define BLACK_LEVEL_STATE_TABLES            0x00092003              // 0x00092003 (0x00002104)
#define BLACK_LEVEL_DETECTION_WINDOW        0x00092004              // 0x00092004 (0x00002105)
#define RGB_GAIN_ADJUSTMENT                 0x00092005              // 0x00092005 (0x00002002)
#define DIGITAL_GAIN_SATURATION_LEVEL       0x00092006              // 0x00092006 (0x00002108)
#define VIGNETTE_COMPENSATION               0x00092007              // 0x00092007 (0x00002003)
#define LOCAL_EXPOSURE                      0x00092008              // 0x00092008 (0x00002109)
#define COLOR_CORRECTION                    0x00092009              // 0x00092009 (0x0000210C)
#define RGB_TO_YUV_SETUP                    0x0009200A              // 0x0009200A (0x00002007)
#define CHROMA_SCALE                        0x0009200B              // 0x0009200B (0x0000210E)
#define RGB_TO_YUV_ADV_SETUP                0x0009200C              // 0x0009200C (0x00002012)//This command is for video fade in and out of decoder, merged from A5S

#define BLACK_LEVEL_AMPLINEAR_HDR           0x00092010              // 0x00092010 new for S2L
#define BLACK_LEVEL_GLOBAL_OFFSET_HDR       0x00092011              // 0x00092011 new for S2L

#define BAD_PIXEL_CORRECT_SETUP             0x00093001              // 0x00093001 (0x0000200A)
#define CFA_NOISE_FILTER                    0x00093002              // 0x00093002 (0x00002107) new design for A7
#define DEMOASIC_FILTER                     0x00093003              // 0x00093003 (0x0000210A)
#define CHROMA_NOISE_FILTER                 0x00093004              // 0x00093004 (0x0000210B) ported from A7L
#define STRONG_GRGB_MISMATCH_FILTER         0x00093005              // 0x00093005
#define CHROMA_MEDIAN_FILTER                0x00093006              // 0x00093006 (0x0000210D)
#define LUMA_SHARPENING                     0x00093007              // 0x00093007 (0x0000210F)
#define LUMA_SHARPENING_LINEARIZATION       0x00093008              // 0x00093008 (0x00002120) remove from A7
#define LUMA_SHARPENING_FIR_CONFIG          0x00093009              // 0x00093009 (0x00002121)
#define LUMA_SHARPENING_LNL                 0x0009300A              // 0x0009300A (0x00002122)
#define LUMA_SHARPENING_TONE                0x0009300B              // 0x0009300B (0x00002123)
#define LUMA_SHARPENING_BLEND_CONTROL       0x0009300C              // 0x0009300C (0x00002131)
#define LUMA_SHARPENING_LEVEL_CONTROL       0x0009300D              // 0x0009300D (0x00002132)

#define AAA_STATISTICS_SETUP                0x00094001              // 0x00094001 (0x00002004)
#define AAA_PSEUDO_Y_SETUP                  0x00094002              // 0x00094002 (0x00002112)
#define AAA_HISTORGRAM_SETUP                0x00094003              // 0x00094003 (0x00002113)
#define AAA_STATISTICS_SETUP1               0x00094004              // 0x00094004 (0x00002110)
#define AAA_STATISTICS_SETUP2               0x00094005              // 0x00094005 (0x00002111)
#define AAA_STATISTICS_SETUP3               0x00094006              // 0x00094006 (0x00002118)
#define AAA_EARLY_WB_GAIN                   0x00094007              // 0x00094007 (0x00002134) A5, A6, A5M, A7
#define AAA_FLOATING_TILE_CONFIG            0x00094008              // 0x00094008
#define VIN_STATISTICS_SETUP                0x00094010              // 0x00094010 S2L

#define NOISE_FILTER_SETUP                  0x00095001              // 0x00095001 (0x00002009)
#define RAW_COMPRESSION                     0x00095002              // 0x00095002 (0x00002114)
#define RAW_DECOMPRESSION                   0x00095003              // 0x00095003 (0x00002115)
#define ROLLING_SHUTTER_COMPENSATION        0x00095004              // 0x00095004 (0x00002116) new design for A7
#define FPN_CALIBRATION                     0x00095005              // 0x00095005 (0x0000211C)
#define HDR_MIXER                           0x00095006              // 0x00095006 (0x0000211F)
#define EARLY_WB_GAIN                       0x00095007              // 0x00095007 (0x0000212A) A5, A6, A7
#define SET_WARP_CONTROL                    0x00095008              // 0x00095008 (0x00002129) A5M
#define CHROMATIC_ABERRATION_WARP_CONTROL   0x00095009              // 0x00095009
#define RESAMPLER_COEFF_ADJUST              0x0009500A              // 0x0009500A

#define HDR_PREBLEND_CONFIG                 0x00095010              // new for S2L
#define HDR_PREBLEND_THRESH                 0x00095011              // new for S2L
#define HDR_PREBLEND_BLACK_LEVEL            0x00095012              // new for S2L
#define HDR_PREBLEND_EXPOSURE               0x00095013              // new for S2L
#define HDR_PREBLEND_ALPHA                  0x00095014              // new for S2L


#define DUMP_IDSP_CONFIG                    0x00096001              // 0x00096001 (0x0000ff07 AMB_DSP_DEBUG_2)
#define SEND_IDSP_DEBUG_CMD                 0x00096002              // 0x00096002 (0x0000ff08 AMB_DSP_DEBUG_3)
#define UPDATE_IDSP_CONFIG                  0x00096003              // 0x00096003 (0x0000ff09)
#define PROCESS_IDSP_BATCH_CMD              0x00096004              // 0x00096004

// H264/JPEG encoding mode commands
#define VIDEO_PREPROCESSING 		0x00003001
#define FAST_AAA_CAPTURE 		0x00003002
#define H264_ENCODE 			0x00003004
#define H264_ENCODE_FROM_MEMORY		0x00003005
#define H264_BITS_FIFO_UPDATE		0x00003006
#define ENCODING_STOP			0x00003007
#define MODIFY_CMD_DLY			0x00003008
#define VIDEO_ENCODE_ROTATE     0x00003009
#define VIDEO_HISO_CONFIG_UPDATE	0x00003014

#define STILL_CAPTURE			0x00004001
#define JPEG_ENCODE_RESCALE_FROM_MEMORY	0x00004002
#define JPEG_BITS_FIFO_UPDATE 		0x00004003
#define FREE_RAW_YUV_PIC_BUFFER		0x00004004
#define JPEG_RAW_YUV_STOP		0x00004005
#define MJPEG_ENCODE			0x00004006
#define VID_FADE_IN_OUT			0x00004007
#define MJPEG_ENCODE_WITH_H264          0x00004008
#define OSD_INSERT                      0x00004009
#define YUV422_CAPTURE 			0x00004010
#define SEND_CAVLC_RESULT		0x00004011
#define STILL_CAPTURE_IN_REC            0x00004012      // Z.ZHOU added for still capture during active recording
#define OSD_BLEND                       0x00004013
#define INTERVAL_CAPTURE		0x00004014
#define STILL_CAPTURE_ADV               0x00004015

/* H264/JPEG decode mode commnads */
#define H264_DECODE             	0x00005002
#define JPEG_DECODE             	0x00005003
#define RAW_PICTURE_DECODE      	0x00005004
#define RESCALE_POSTPROCESSING  	0x00005005
#define H264_DECODE_BITS_FIFO_UPDATE 	0x00005006
#define H264_PLAYBACK_SPEED		0x00005007
#define H264_TRICKPLAY			0x00005008
#define DECODE_STOP             	0x00005009
#define MULTI_SCENE_DECODE      	0x00005010
#define CAPTURE_VIDEO_PICTURE   	0x00005011
#define CAPTURE_STILL_PICTURE   	0x00005012
#define JPEG_FREEZE             	0x00005013
#define MULTI_SCENE_SETUP       	0x00005014
#define MULTI_SCENE_DECODE_ADV		0x00005015
#define JPEG_DECODE_THUMBNAIL_WARP	0x00005016
#define MULTI_SCENE_DECODE_ADV_2	0x00005017
#define RESCALE_DRAM_TO_DRAM            0x00005018
#define IMAGE_VIDEO_360_POSTP_ANGLE_UPDATE  0x00005019
#define MULTI_SCENE_BLENDING            0x00005020
#define STILL_DEC_ADV_PIC_ADJ           0x00005021     //Selina add for advance still image setting of IDSP
#define MULTI_SCENE_DISPLAY             0x00005022
#define JPEG_DECODE_ABORT               0x00005023      /* Add for abort the jpeg decode process for special usagem,Mars 071812 */
#define CAPTURE_VIDEO_PICTURE_TO_MJPEG  0x00005024

/* IP CAM commands */
#define IPCAM_VIDEO_PREPROCESSING               0x6001
#define IPCAM_VIDEO_CAPTURE_PREVIEW_SIZE_SETUP  0x6002
#define IPCAM_VIDEO_ENCODE_SIZE_SETUP           0x6003
#define IPCAM_REAL_TIME_ENCODE_PARAM_SETUP      0x6004
#define IPCAM_VIDEO_SYSTEM_SETUP                0x6006
#define IPCAM_OSD_INSERT                        0x6007
#define IPCAM_SET_PRIVACY_MASK                  0x6008
#define IPCAM_FAST_OSD_INSERT                   0x6009
#define IPCAM_SET_REGION_WARP_CONTROL         	0x600A
#define IPCAM_SET_HDR_PROC_CONTROL              0x600B
#define IPCAM_ENC_SYNC_CMD                      0x600C
#define IPCAM_GET_FRAME_BUF_POOL_INFO			0x600D
#define IPCAM_EFM_CREATE_FRAME_BUF_POOL			0x600E
#define IPCAM_EFM_REQ_FRAME_BUF					0x600F
#define IPCAM_EFM_HANDSHAKE						0x6010

/* VOUT commands */
#define VOUT_MIXER_SETUP		0x00007001
#define VOUT_VIDEO_SETUP		0x00007002
#define VOUT_DEFAULT_IMG_SETUP		0x00007003
#define VOUT_OSD_SETUP			0x00007004
#define VOUT_OSD_BUFFER_SETUP		0x00007005
#define VOUT_OSD_CLUT_SETUP		0x00007006
#define VOUT_DISPLAY_SETUP		0x00007007
#define VOUT_DVE_SETUP			0x00007008
#define VOUT_RESET			0x00007009
#define VOUT_DISPLAY_CSC_SETUP		0x0000700A
#define VOUT_DIGITAL_OUTPUT_MODE_SETUP	0x0000700B
#define VOUT_GAMMA_SETUP              0x0000700C

/*
 * These AMB_* commands are for ambarella experimental use only
 */
#define AMB_CFA_NOISE_FILTER		0x0000f001
#define AMB_DIGITAL_GAIN_SATURATION	0x0000f002
#define AMB_CHROMA_MEDIAN_FILTER	0x0000f003
#define AMB_LUMA_DIRECTIONAL_FILTER	0x0000f004
#define AMB_LUMA_SHARPEN		0x0000f005
#define AMB_MAIN_RESAMPLER_BANDWIDTH	0x0000f006
#define AMB_CFA_RESAMPLER_BANDWIDTH	0x0000f007

#define AMB_DSP_DEBUG_0			0x0000ff00
#define AMB_DSP_DEBUG_1			0x0000ff01
#define AMB_AAA_STATISTICS_DEBUG	0x0000ff02
#define AMB_DSP_SPECIAL			0x0000ff03
#define AMB_AAA_STATISTICS_DEBUG1	0x0000ff04
#define AMB_AAA_STATISTICS_DEBUG2	0x0000ff05
#define AMB_BAD_PIXEL_CROP		0x0000ff06
#define AMB_DSP_DEBUG_2			0x0000ff07
#define AMB_DSP_DEBUG_3			0x0000ff08
#define AMB_UPDATE_IDSP_CONFIG		0x0000ff09
#define AMB_REAL_TIME_RATE_MODIFY       0x0000ff0a
#define AMB_STOP_FATAL_UCODE		0x0000ffa0

// sensor_id for 0x2001 and 0x3001
#define MT9T001_ID      1
#define ALTA2460_ID     2
#define SONY495_ID      3
#define MT9P001_ID      4
#define OV5260_ID       5
#define MT9M002_ID      6
#define MT9P401_ID      7
#define ALTA3372_ID     8
#define ALTA5262_ID     9
#define SONYIMX017_ID   10
#define MT9N001_ID      11
#define SONYIMX039_ID   12
#define MT9J001_ID      13
#define SEMCOYHD_ID     14
#define SONYIMX044_ID   15
#define OV5633_ID       16
#define OV9710_ID       17
#define OV9810_ID       23
#define OV8810_ID       24
#define SONYIMX032_ID   25
#define MT9P014_ID      26
#define OV2710_ID       27
#define OV5653_ID       28
#define OV14810_ID      29
#define MT9F001_ID      30
#define MT9P013_ID      31
#define S5K4E1GX_ID     32
#define S5K3H1GX_ID     33
#define SONYIMX074_ID   34

/* return value for chip_id_ptr in dsp_init_data_t */
typedef enum{
	AMBA_CHIP_ID_S2L_UNKNOWN = -1,
	AMBA_CHIP_ID_S2L_22M = 0,
	AMBA_CHIP_ID_S2L_33M = 1,
	AMBA_CHIP_ID_S2L_55M = 2,
	AMBA_CHIP_ID_S2L_99M = 3,
	AMBA_CHIP_ID_S2L_63 = 4,
	AMBA_CHIP_ID_S2L_66 = 5,
	AMBA_CHIP_ID_S2L_88 = 6,
	AMBA_CHIP_ID_S2L_99 = 7,
	AMBA_CHIP_ID_S2L_TEST = 8,
} amba_chip_id_t;

/* DSP CMD/MSG protocol version */
typedef enum {
	MPV_RING_BUFFER = 0,
	MPV_FIXED_BUFFER = 1,
} DSP_MSG_PROTOCOL_VERSION;

/**
 *  DSP_INIT_DATA contains initialization related parameters
 *  passed from ARM to DSP. The location is used to store a
 *  data structure that contains various buffer pointers and
 *  size information. 128 bytes for DSP_INIT_DATA
 */
typedef struct dsp_init_data
{
	u32 *default_binary_data_ptr;
	u32 *cmd_data_ptr;
	u32 cmd_data_size;
	u32 *result_queue_ptr;
	u32 result_queue_size;
	u32 reserved_1[3];
	u32 operation_mode;
	u32 *default_config_ptr;
	u32 default_config_size;
	u32 *DSP_buf_ptr;
	u32 DSP_buf_size;
	u32 *vdsp_info_ptr;
	u32 *pjpeg_buf_ptr;
	u32 pjpeg_buf_size;
	u32 *chip_id_ptr;
	u32 chip_id_checksum;
	u32 dsp_log_buf_ptr;
	u32 prev_cmd_seq_num;
	u32 cmdmsg_protocol_version;
	u32 reserved_2[11];
}dsp_init_data_t;

// Structure that indicate the vdsp interrupt status
typedef struct vdsp_info_s
{
	u32 dsp_cmd_rx; // if DSP had read from cmd buf
	u32 dsp_msg_tx;	// if DSP had written to msg buf
	u32 num_dsp_msgs : 8;	/* number of DSP messages written to msg buf */
	u32 reserved : 24;
	u32 padding[5];
} vdsp_info_t;

/**
 *  During H264/JPEG encoding mode, ARM will receive vin interrupt.
 *  ARM should copy video input polarity information to video input
 *  register area right after input interrupt is received.
 */
typedef struct vin_shadow_registers
{
	u32 vin_shadow_register;
}vin_shadow_register_t;

/**
 *  The data structure are read by DSP at each video output frame.
 *  DSP reads the data structure after the DMA completion of the
 *  output data. ARM receives each Vout interrrupt at vsync
 *  rising edge.
 */
typedef struct vout_setup_data
{
	u8 format;
	u8 frame_rate;
	u8 Audio_Sampling_Clock_Freq;
	u8 Audio_Output_Clock_Freq;
	u16 active_width;
	u16 active_height;
	u16 video_width;
	u16 video_height;
	u32 OSD0_addr;
	u32 OSD1_addr;
	u16 OSD0_width;
	u16 OSD1_width;
	u16 OSD0_height;
	u16 OSD1_height;
	u16 OSD0_pitch;
	u16 OSD1_pitch;
	u32 default_image_y_addr;
	u32 default_image_u_addr;
	u16 default_image_pitch;
	u16 default_image_height;
	u8 polarity;
	u8 flip_control;
	u8 reset;
	u8 OSD_progressive;
}vout_setup_data_t;
/**
 * Binary data structure for encode mode
 */
typedef struct default_enc_binary_data
{
	u32 magic_number;
	u32 manufacture_id;
	u32 uCode_version;
	u32 dram_addr_idsp_default_cfg;
	u32 dram_addr_luma_huf_tab;
	u32 dram_addr_chroma_huf_tab;
	u32 dram_addr_mctf_cfg_buffer;
	u32 dram_addr_cabac_ctx_tab;
	u32 dram_addr_jpeg_out_bit_buffer;
	u32 dram_addr_cabac_out_bit_buffer;
	u32 dram_addr_bit_info_buffer;
	u32 jpeg_out_bit_buf_sz;
	u32 h264_out_bit_buf_sz;
	u32 bit_info_buf_sz;
	u32 mctf_cfg_buf_sz;
}default_enc_binary_data_t;

/**
 * Binary data structure for decode mode
 */
typedef struct default_dec_binary_data
{
	u32 magic_number;
	u32 manufacture_id;
	u32 uCode_version;
	u32 dram_addr_idsp_default_cfg;
	u32 dram_addr_cabac_ctx_tab;
	u32 dram_addr_cabac_out_bit_buffer_daddr;
	u32 dram_addr_cabac_out_bit_buffer_size;
	u32 still_picture_decode_buffer_daddr;
	u32 still_picture_decode_buffer_size;
	u32 h264_frame_buffer_daddr;
	u32 h264_frame_buffer_size;
	u32 still_frame_buffer_daddr;
	u32 still_frame_buffer_size;
	u32 still_frame_buffer_pitch;
	u32 dram_addr_luma_huf_tab;
	u32 dram_addr_chroma_huf_tab;
}default_dec_binary_data_t;

/**
 *  Configuration mode
 *-------------------------------------------------
 *  1. Interrupt setup			(0x00001001)
 *  2. H264 encoding setup 		(0x00001002)
 *  3. JPEG encoding setup 		(0x00001003)
 *  4. H264 decoding setup 		(0x00001004)
 *  5. JPEG decoding setup		(0x00001005)
 *  6. Reset operation  		(0x00001006)
 *  7. Video output restart		(0x00001007)
 *  8. Encode timer operation   	(0x00001008)
 *  9. Chip Selection			(0x00001009)
 * 10. HD echo setup			(0x0000100A)
 * 11. System setup info		(0x0000100B)
 * 12. EIS SWITCH VOUT		(0x0000100C)
 * 13. Debug level setup		(0x0000100D)
 * 14. Audio frequency setup		(0x0000100E)
 */

/**
 * Interrupt setup
 */
typedef struct interrupt_setup_s
{
	u32 cmd_code;
	u8 vin_int;
	u8 vout_int;
}interrupt_setup_t;


#define	CMD_MSG_CMD_ID(cmd_code)			(((cmd_code)&0xffffff))
/**
 * H264 encoding setup
 */
#define	H264_ENCODER_SETUP_GET_CHAN_ID(cmd_code)	(((cmd_code)>>24)&0x1f)
#define	H264_ENCODER_SETUP_GET_STREAM_ID(cmd_code)	((cmd_code)>>30)



#define NUM_PIC_TYPES           (3)

typedef enum {
	SIMPLE_GOP = 0,
	P2B2REF_GOP = 1,
	P2B3REF_GOP = 2,
	P2B3_ADV_GOP = 3,
	HI_GOP_DRAM = 4,
	HI_GOP_SMEM = 5,
	NON_REF_P_GOP = 6,
	HI_P_GOP = 7,
	LT_REF_P_GOP = 8,
	GOP_TOTAL_NUM,
	DSP_GOP_FIRST = SIMPLE_GOP,
	DSP_GOP_LAST = GOP_TOTAL_NUM,
} DSP_GOP_STRUCTURE;

typedef struct h264_encode_setup_s
{
	u32 cmd_code;
	u8 mode;
	u8 M;
	u8 N;
#define QLEVEL_MASK             (0x1f)
#define LEVEL_MASK              (0xE0)

	u8 quality;
	u32 average_bitrate;
	u32 vbr_cntl;
	u32 vbr_setting : 8;
	u32 allow_I_adv : 8;
	u32 cpb_buf_idc : 8;
	u32 en_panic_rc : 2;
	u32 cpb_cmp_idc : 2;  // cpb compliance idc
	u32 fast_rc_idc : 4;
	u32 target_storage_space;
	u32 bits_fifo_base;
	u32 bits_fifo_limit;
	u32 info_fifo_base;
	u32 info_fifo_limit;
	u8 audio_in_freq;
	u8 vin_frame_rate;
	u8 encoder_frame_rate;
	u8 frame_sync;
	u16 initial_fade_in_gain;
	u16 final_fade_out_gain;
	u32 idr_interval;
	u32 cpb_user_size;
	u8 numRef_P;
	u8 numRef_B;
	u8 vin_frame_rate_ext;
	u8 encoder_frame_rate_ext;
	u32 N_msb : 8;
	u32 fast_seek_interval : 8;
	u32 reserved2 : 16 ;
	u32 reserved3;
	u32 vbr_cbp_rate;
	u32 frame_rate_division_factor : 8 ;
	u32 force_intlc_tb_iframe : 1 ;
	u32 session_id : 4 ;
	u32 frame_rate_multiplication_factor : 8 ;
	u32 hflip : 1 ;
	u32 vflip : 1 ;
	u32 rotate : 1 ;
	u32 chroma_format : 1 ;
	u32 reserved : 7 ;
	u32 custom_encoder_frame_rate ;
}h264_encode_setup_t;

typedef h264_encode_setup_t vid_encode_setup_t;

/**
 * JPEG encoding setup
 */
typedef struct jpeg_encode_setup_s
{
	u32 cmd_code;
	u32 chroma_format;
	u32 bits_fifo_base;
	u32 bits_fifo_limit;
	u32 info_fifo_base;
	u32 info_fifo_limit;
	u32 *quant_matrix_addr;
	u32 frame_rate;
	u32 mctf_mode : 8;
	u32 is_mjpeg : 1;
	u32 frame_rate_division_factor : 8 ;
	u32 session_id : 4 ;
	u32 frame_rate_multiplication_factor : 8 ;
	u32 hflip : 1 ;
	u32 vflip : 1 ;
	u32 rotate : 1;
	u32 targ_bits_pp;
	u32 initial_ql : 8;
	u32 tolerance : 8;
	u32 max_recode_lp : 8;
	u32 total_sample_pts : 8;
	u32 rate_curve_dram_addr;
	u16 screen_thumbnail_w;
	u16 screen_thumbnail_h;
	u16 screen_thumbnail_active_w;
	u16 screen_thumbnail_active_h;
}jpeg_encode_setup_t;

/**
 * H264 decoding setup
 */
typedef struct h264_decode_setup_s
{
	u32 cmd_code;
	u32 bits_fifo_base;
	u32 bits_fifo_limit;
	u32 fade_in_pic_addr;
	u32 fade_in_pic_pitch;
	u32 fade_in_alpha_start;
	u32 fade_in_alpha_step;
	u32 fade_in_total_frames;
	u32 fade_out_pic_addr;
	u32 fade_out_pic_pitch;
	u32 fade_out_alpha_start;
	u32 fade_out_alpha_step;
	u32 fade_out_total_frames;
	u8 cabac_to_recon_delay;
	u8 forced_max_fb_size;
	u8 iv_360;             // flag to support 360 degree image/video playback
	u8 dual_lcd;  // flag to process the decoded picture in postp thread, must be set for dual LCD rotation

	//add for memory allocation size control according support range
	u32 bit_rate;
	u16 video_max_width;
	u16 video_max_height;
	u8 gop_n;
	u8 gop_m;
	u8 dpb_size;       //impliment: max is 6 of A7L
	u8 customer_mem_usage;   //On: use new memory allocation
	u8 trickplay_fw : 1;
	u8 trickplay_ff_IP : 1;
	u8 trickplay_ff_I : 1;
	u8 trickplay_bw : 1;
	u8 trickplay_fb_IP : 1;
	u8 trickplay_fb_I : 1;
	u8 trickplay_rsv : 2;

	u8 ena_stitch_flow;
	u16 vout0_win_width;
	u16 vout0_win_height;
	u16 vout1_win_width;
	u16 vout1_win_height;
}h264_decode_setup_t;

/**
 * JPEG decoding setup
 */
typedef struct jpeg_decode_setup_s
{
	u32 cmd_code;
	u32 bits_fifo_base;
	u32 bits_fifo_limit;
	u32 cross_fade_alpha_start;
	u32 cross_fade_alpha_step;
	u32 cross_fade_total_frames;
	u8 background_y;
	u8 background_cb;
	u8 background_cr;
	u8 reserved;
	u16 max_vout_width;
	u16 max_vout_height;

	// the following parameters will be used to
	// configure jpeg dram for seamless transition
	u16 vout0_win_width;
	u16 vout0_win_height;
	u16 vout1_win_width;
	u16 vout1_win_height;
	u32 capture_buffer_size;
}jpeg_decode_setup_t;

/**
 * Reset operation
 */
typedef struct reset_operation_s
{
	u32 cmd_code;
}reset_operation_t;

/**
 * Video output restart
 */
typedef struct video_output_restart_s
{
	u32 cmd_code;
	u8 vout_id;
}video_output_restart_t;

/**
 * Vin timer mode operation
 */
typedef struct vin_timer_mode_s
{
	u32 cmd_code;
	u8  timer_scaler;
	u8 display_opt;
	u8  video_term_opt; // 0 terminate with frame wait, 1 reset idsp, and terminate right away
	u8 reserved;
}vin_timer_mode_t;

/**
 * Chip selection
 */
typedef struct chip_select_s
{
	u32 cmd_code;
	u8 chip_type;
}chip_select_t;

/**
 * HD echo setup
 */
typedef struct hd_echo_setup_s
{
	u32 cmd_code;
	u8 enable;
}hd_echo_setup_t;

/**
 * System setup info
 */

typedef enum {
	CAMCORDER_APP_MODE = 0,
	IP_CAMCORDER_APP_MODE = 2,
} APP_MODE;

typedef	struct{
	u32	multiple_stream : 7;  //0:single stream, 1: multiple stream
	u32	power_mode : 1;	//0: high power, 1: low power
} applicaton_mode_t;

typedef	enum {
	OFF_PREVIEW_TYPE = 0, /* Preview unit is not used */
	SD_PREVIEW_TYPE = 1, /* Preview unit used to generate SD Preview for VOUT */
	HD_PREVIEW_TYPE = 2, /* Preview unit used to generate HD Preview for VOUT */
	CAPTURE_PREVIEW_TYPE = 3, /* Preview unit used to generate capture buffers for encoding */
	MAX_PREVIEW_TYPE = 4,
} preview_type_t;

typedef struct system_setup_info_s
{
	u32 cmd_code;

	u32 preview_A_type : 8;
	u32 preview_B_type : 8;
	u32 voutA_osd_blend_enabled : 1 ; /* if set, Mixing section of VOUTA is used for DRAM to DRAM OSD blending */
	u32 voutB_osd_blend_enabled : 1 ; /* if set, Mixing section of VOUTB is used for DRAM to DRAM OSD blending */
	/* For pipelines that support HDR, it controls where preblend is done.
	 *  If set, hdr_num_exposures_minus_1 must be 1 (num_exposures = 2)
	 */
	u32 hdr_preblend_from_vin : 1 ;
	/* for pipelines that support de-fog (num_exposures = 1) and HDR (num_exposures > 1) */
	u32 hdr_num_exposures_minus_1 : 2 ;
	u32 pip_vin_enabled : 1; /* if set, choose second VIN */
	u32 raw_encode_enabled : 1; /* if set, raw encode is enabled for the pipeline */
	u32 adv_iso_disabled : 1;  /* if set, aliso (mode 4) supports 1-pass + temporal adjust */
	u32 me0_scale_factor : 2; /* 0 - disabled; 1 - 8x; 2 - 16x */
	u32 vout_swap : 1; /* 0: prev A uses VOUT0, prev B uses VOUT1 ; 1: prev A uses VOUT1, prev B uses VOUT0 */
	u32 padding : 5;

	applicaton_mode_t  sub_mode_sel; //0: Camcorder mode (single-stream encoder) 1: DVR mode (multiple-stream encoder)
	u8  reserved1;     // number of input YUV sources muxed together.
	u8  lcd_3d;                      // need to support LCD 3D rotation
	u8  iv_360;       // need to support 360 degree image/video playback.
	u8  mode_flags; /* determines the IDSP pipeline to run */
	u32 audio_clk_freq;
	u32 idsp_freq;
	u32 core_freq;
	u16 sensor_HB_pixel;
	u16 sensor_VB_pixel;
}system_setup_info_t;

/**
 * dsp debug level setup
 */
#define	CODING_ORC_THREAD_0_PRINTF_DISABLE	0x1
#define	CODING_ORC_THREAD_1_PRINTF_DISABLE	0x2
#define	CODING_ORC_THREAD_2_PRINTF_DISABLE	0x4
#define	CODING_ORC_THREAD_3_PRINTF_DISABLE	0x8

typedef struct dsp_debug_level_setup_s{
	u32 cmd_code;
	u8 module;
	u8 debug_level;
	u8 coding_thread_printf_disable_mask;
	u8 padding;
}dsp_debug_level_setup_t;

/**
 * system audio clock frequency setup
 */
#define AUDIO_CLK_12_288_MHZ 12288000

typedef struct system_parameters_setup_s{
	u32 cmd_code;
	u32 audio_clk_freq;
	u32 idsp_freq;
	u16 sensor_HB_pixel;
	u16 sensor_VB_pixel;
}system_parameters_setup_t;

/**
 * system idsp frequency setup
 */
#define IDSP_FREQ_144_MHZ 144000000

typedef struct system_idsp_freq_setup_s{
	u32 cmd_code;
	u32 idsp_freq;
}system_idsp_freq_setup_t;

/**
 * Luma Sharpen setup
 */
typedef struct luma_sharpen_setup_s
{
	u32 cmd_code;
	u8 strength;
}luma_sharpen_setup_t;

/**
 * RGB to RGB setup
 */
typedef struct rgb_to_rgb_stup_s
{
	u32 cmd_code;
	s16 matrix_values[9];
}rgb_to_rgb_setup_t;


/**
 * Gamma curve lookup table setup
 */
typedef struct gamma_curve_setup_s
{
	u32 cmd_code;
	u32 tone_curve_addr;	// for A2 : tone_curve_addr_red
	u32 tone_curve_addr_green;
	u32 tone_curve_addr_blue;
}gamma_curve_setup_t;

/**
 * Vid fade in out setup
 */
typedef struct vid_fade_in_out_setup_s
{
	u32 cmd_code;
	u16 fade_in_duration;
	u16 fade_out_duration;
	u8 	fade_white;
}vid_fade_in_out_setup_t;

/**
 * MCTF MV_STAB setup
 */
typedef struct mctf_mv_stab_setup_s
{
	u32 cmd_code;
	u8 noise_filter_strength;
	u8 image_stabilize_strength;
	u8 still_noise_filter_strength;
	u8 reserved;
	u32 mctf_cfg_dram_addr;
}mctf_mv_stab_setup_t;

/**
 * Set slow shutter upsampling rate
 */
typedef struct set_slow_shutter_upsampling_rate_s
{
	u32 cmd_code;
	u8 slow_shutter_upsampling_rate;
}set_slow_shutter_upsampling_rate_t;

/**
 * Sensor capture repeat
 */
typedef struct sensor_cap_repeat_s
{
	u32 cmd_code;
	u32 repeat_cnt;
	u32 mode_sel;
}sensor_cap_repeat_t;

/**
 * MCTF gmv setup
 */
typedef struct mctf_gmv_setup_s
{
	u32 cmd_code;
	u32 enable_external_gmv;
	u32 external_gmv;
}mctf_gmv_setup_t;

#ifdef TYPE_DEF

/************************************************************
 * IDSP commands (Category 9)
 */

/**
 * Ser Vin Capture windows (ARCH_VER >= 3) (0x00091001)
 */

typedef union
{
    struct
        {
            u16    sw_reset    :1;
            u16    enb_vin     :1;
            u16    win_en      :1;
#define DATA_VAL_RISING_EDGE    0
#define DATA_VAL_FALLING_EDGE   1
            u16    data_edge   :1;
#define MAS_SLAV_MOD_UND    0x0
#define SLAVE_MOD           0x1
#define MAS_MOD             0x2
#define MAS_SPECIAL         0x3
            u16    mastSlav_mod:2;
#define EMB_SYNC_IN_DATA    0x1
            u16    emb_sync    :1;
#define EMB_SYNC_656        0x0
#define ALL_ZERO_IN_BLANK   0x1
            u16    emb_sync_mode:1;
#define EMB_SYNC_IN_LOW_PIX 0x0;
#define EMB_SYNC_IN_UP_PIX  0x1
#define EMB_SYNC_IN_BOTH    0x2
            u16    emb_sync_loc:2;
#define VSYNC_POL_ACT_H     0x0
#define VSYNC_POL_ACT_L     0x1
            u16    vs_pol      :1;
#define HSYNC_POL_ACT_H     0x0
#define HSYNC_POL_ACT_L     0x1
            u16    hs_pol      :1;
            u16    field_pol   :1;
#define FLD_MOD_NORMAL  0
#define FLD_MOD_SONY    1
            u16    sony_fld_mod:1;
            u16    ec_enb      :1;
            u16    reserved    :1;
        } s_cntl_bit_fld;
    u16 s_control;
} S_CTRL_REG;



typedef union
{
    struct
    {
#define LVCMOS  0x0
#define LVDS    0x1
        u16    pad_type    :1;
#define SDR     0x0
#define DDR     0x1
        u16    data_rate   :1;
#define ONE_PIX_WIDE    0
#define TWO_PIX_WIDE    1
        u16                :1;
#define LVDS_SRC    0x0
#define GPIO_SRC    0x1
        u16    inp_src     :1;
#define RGB_IN      0x0
#define YUV_IN      0x1
        u16    inp_src_ty  :1;
        u16    src_pix_data_width:2;
#define CrYCbY_1PIX 0x0
#define CbYCrY_1PIX 0x1
#define YCrYCb_1PIX 0x2
#define YCbYCr_1PIX 0x3
#define YCrYCb_2PIX 0x0
#define YCbYCr_2PIX 0x1
#define CrYCbY_2PIX 0x2
#define CbYCrY_2PIX 0x3
        u16    yuv_inp_ord :2;
		u16    reserved    :4;
		u16	non_mipi_input:1; // New elemented added for a5s here
        u16    reserved2   :2;
    } s_inputCfg_bit_fld;
    u16    s_inputCfg;
} S_INPUT_CONFIG_REG;
#endif

typedef struct
{
	u16  s_ctrl_reg;
	u16  s_inpcfg_reg;
	u16  s_status_reg;
	u16  s_v_width_reg;
	u16  s_h_width_reg;
	u16  s_h_offset_top_reg;
	u16  s_h_offset_bot_reg;
	u16  s_v_reg;
	u16  s_h_reg;
	u16  s_min_v_reg;
	u16  s_min_h_reg;
	u16  s_trigger_0_start_reg;
	u16  s_trigger_0_end_reg;
	u16  s_trigger_1_start_reg;
	u16  s_trigger_1_end_reg;
	u16  s_vout_start_0_reg;
	u16  s_vout_start_1_reg;
	u16  s_cap_start_v_reg;
	u16  s_cap_start_h_reg;
	u16  s_cap_end_v_reg;
	u16  s_cap_end_h_reg;
	u16  s_blank_leng_h_reg;
	u32  vsync_timeout;
	u32  hsync_timeout;

} vin_cap_reg_t;

typedef struct vin_cap_win_s
{
    u32 cmd_code;
    u16 s_ctrl_reg;
    u16 s_inp_cfg_reg;
    u16 reserved;
    u16 s_v_width_reg;
    u16 s_h_width_reg;
    u16 s_h_offset_top_reg;
    u16 s_h_offset_bot_reg;
    u16 s_v_reg;
    u16 s_h_reg;
    u16 s_min_v_reg;
    u16 s_min_h_reg;
    u16 s_trigger_0_start_reg;
    u16 s_trigger_0_end_reg;
    u16 s_trigger_1_start_reg;
    u16 s_trigger_1_end_reg;
    u16 s_vout_start_0_reg;
    u16 s_vout_start_1_reg;
    u16 s_cap_start_v_reg;
    u16 s_cap_start_h_reg;
    u16 s_cap_end_v_reg;
    u16 s_cap_end_h_reg;
    u16 s_blank_leng_h_reg;
    u32 vsync_timeout;
    u32 hsync_timeout;
}vin_cap_win_t;
typedef vin_cap_win_t   VIN_SETUP_CMD;

/**
 * Sensor Input setup (0x00091002)
 */
typedef struct sensor_input_setup_s
{
    u32 cmd_code;
    u8 sensor_id;
    u8 field_format;
    u8 sensor_resolution;
    u8 sensor_pattern;
    u8 first_line_field_0;
    u8 first_line_field_1;
    u8 first_line_field_2;
    u8 first_line_field_3;
    u8 first_line_field_4;
    u8 first_line_field_5;
    u8 first_line_field_6;
    u8 first_line_field_7;
    u32 sensor_readout_mode;
}sensor_input_setup_t;

/**
 * Amplifier linearization (0x00091003)
 * updated on S2L
 */
typedef struct amplifier_linear_s
{
  u32 cmd_code;

  u8 amplinear_enable;
  u8 hdr_amplinear_enable;
  u8 hdrblend_enable;
  u8 hdr_mode;

  u8 hdr_stream0_sub;
  u8 hdr_stream1_sub;
  u8 hdr_stream1_shift;
  u8 hdr_alpha_mode;

  u16 amplinear_exp_table[4];
  u16 hdrblend_exp_table[4];
  u32 amplinear_lookup_table_addr[4];
  u32 hdrblend_lookup_table_addr[4];
}amplifier_linear_t;

/**
 * Pixel Shuffle (0x00091004)
 */
typedef struct pixel_shuffle_s
{
    u32 cmd_code;
    u32 enable;
    u32 reorder_mode;
    u32 input_width;
    u16 start_index[8];
    u16 pitch[8];
}pixel_shuffle_t;

/**
 * Fixed pattern noise correction (0x00091005)
 */
typedef struct fixed_pattern_noise_correct_s
{
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
} fixed_pattern_noise_correct_t;

/**
 * CFA Domain Leakage Filter setup (0x00091006)
 */
typedef struct cfa_domain_leakage_filter_s
{
    u32 cmd_code;
    u32 enable;
    u8 alpha_rr;
    u8 alpha_rb;
    u8 alpha_br;
    u8 alpha_bb;
    u16 saturation_level;
}cfa_domain_leakage_filter_t;

/**
 * Anti-Aliasing Filter setup (0x00091007)
 */
typedef struct anti_aliasing_filter_s
{
    u32 cmd_code;
    u32 enable;
    u32 threshold;
    u32 shift;
}anti_aliasing_filter_t;

/**
 * Set VIN Configuration (0x00091008)
 */
typedef struct set_vin_config_s
{
    u32 cmd_code;
    u16 vin_width;
    u16 vin_height;
    u32 vin_config_dram_addr;
    u16 config_data_size;
    u8  sensor_resolution;
    u8  sensor_bayer_pattern;
    u32 vin_set_dly;
    u8  vin_enhance_mode;   // valid only on A7S
    u8  vin_panasonic_mode; // valid only on A7S
    u8  reserved[2];
}set_vin_config_t;

/**
 * Set VIN Rolling Shutter Configuration (0x00091009)
 */
typedef struct set_vin_rolling_shutter_config_s
{
	u32 cmd_code;
	u32 rolling_shutter_config_dram_addr; // 0x28
	u16 vertical_enable:1;                // 0x2A
	u16 horizontal_enable:1;
	u16 arm_interrupt_enable:1;
	u16 register_config_mask:1;
	u16 use_software_sample:1;
	u16 reserved:11;
	u16 interrupt_delay;                  // 0x2B
	u16 horizontal_software_gyro_sample;
	u16 horizontal_offset;
	u16 horizontal_scale;
	u16 vertical_software_gyro_sample;
	u16 vertical_offset;
	u16 vertical_scale;                   // 0x31
}set_vin_rolling_shutter_config_t;

/**
 * Set Master clock Configuration (0x0009100E)
 */
typedef struct set_vin_master_s
{
	u32 cmd_code;
	u16 master_sync_reg_word0;
	u16 master_sync_reg_word1;
	u16 master_sync_reg_word2;
	u16 master_sync_reg_word3;
	u16 master_sync_reg_word4;
	u16 master_sync_reg_word5;
	u16 master_sync_reg_word6;
	u16 master_sync_reg_word7;
}set_vin_master_t;

/**
 * Set Global Vin Configuration (0x0009100F)
 */
typedef struct set_vin_global_config_s
{
	u32 cmd_code;
	u8  main_or_pip;          // 0: man; 1: pip
	u8  reserved[3];
	u32 global_cfg_reg_word0; // word0 of the register values
} set_vin_global_config_t;

typedef struct static_bad_pixel_correction_s {
	u32 cmd_code;
	u32 sbp_map_addr;
	u16 num_of_rows;
	u16 num_of_cols;
	u16 sbp_map_pitch;
	u8  correction_mode; // 0 disable, 1 enable, 2 visualize
	u8 reserved;
} static_bad_pixel_correction_t;

/**
 * Black Level Global Offset (0x00092001)
 */
typedef struct black_level_global_offset_s
{
	u32 cmd_code;
	u32 global_offset_ee;
	u32 global_offset_eo;
	u32 global_offset_oe;
	u32 global_offset_oo;
	u16 black_level_offset_red;
	u16 black_level_offset_green;
	u16 black_level_offset_blue;
	u16 gain_depedent_offset_red;
	u16 gain_depedent_offset_green;
	u16 gain_depedent_offset_blue;
	u16 def_blk1_red;   // for S2L
	u16 def_blk1_green; // for S2L
	u16 def_blk1_blue;  // for S2L
}black_level_global_offset_t;

/**
 * Black level correction (0x00092002)
 */
typedef struct black_level_correcttion_s
{
    u32 cmd_code;
    u32 column_enable;
    u32 row_enable;
    u32 black_level_mode;
    u32 bad_pixel_mode_column;
    u32 bad_pixel_mode_row;
    u32 cold_pixel_thresh;
    u32 hot_pixel_thresh;
    u32 center_offset;
    u32 column_replace;
    u32 column_invalid_replace;
    u32 column_invalid_thresh;
    u32 row_invalid_thresh;
    u32 column_average_k_frames;
    u32 row_average_k_frames;
    u32 column_black_gain;
    u32 row_black_gain;
    u32 column_bad_pixel_subtract;
    u32 global_offset_ee;
    u32 global_offset_eo;
    u32 global_offset_oe;
    u32 global_offset_oo;
}black_level_correcttion_t;

/**
 * Black level state tables (0x00092003)
 */
typedef struct black_level_state_table_s
{
    u32 cmd_code;
    u32 num_columns;
    u32 column_frame_acc_addr;
    u32 column_average_acc_addr;
    u32 num_rows;
    u32 row_fixed_offset_addr;
    u32 row_average_acc_addr;
}black_level_state_table_t;

/**
 * Black level detection window (0x00092004)
 */
typedef struct black_level_detect_win_s
{
    u32 cmd_code;
    u8 top_black_present;
    u8 bottom_black_present;
    u8 left_black_present;
    u8 right_black_present;
    u32 top_black_start;
    u32 top_black_end;
    u32 bottom_black_start;
    u32 bottom_black_end;
    u32 left_black_start;
    u32 left_black_end;
    u32 right_black_start;
    u32 right_black_end;
}black_level_detect_win_t;

/**
 * RGB gain adjustment (0x00092005)
 */
typedef struct rgb_gain_adjust_s
{
    u32 cmd_code;
    u32 r_gain;
    u32 g_even_gain;
    u32 g_odd_gain;
    u32 b_gain;
}rgb_gain_adjust_t;

/**
 * Digital gain saturation level (0x00092006)
 */
typedef struct digital_gain_level_s
{
    u32 cmd_code;
    u32 level_red;
    u32 level_green_even;
    u32 level_green_odd;
    u32 level_blue;
}digital_gain_level_t;

/**
 * Vignette compensation (0x00092007)
 */
typedef struct vignette_compensation_s
{
    u32 cmd_code;
    u8 enable;
    u8 gain_shift;
    u16 reserved;
    u32 tile_gain_addr;
    u32 tile_gain_addr_green_even;
    u32 tile_gain_addr_green_odd;
    u32 tile_gain_addr_blue;
}vignette_compensation_t;

/**
 * Local exposure (0x00092008)
 */
typedef struct local_exposure_s
{
    u32 cmd_code;
    u8 group_index;
    u8 enable;
    u16 radius;
    u8 luma_weight_red;
    u8 luma_weight_green;
    u8 luma_weight_blue;
    u8 luma_weight_sum_shift;
    u32 gain_curve_table_addr;
    u16 luma_offset;
    u16 reserved;
}local_exposure_t;

/**
 * Color correction (0x00092009)
 */
typedef struct color_correction_s
{
    u32 cmd_code;
    u32 group_index;
    u8 enable;
    u8 no_interpolation;
    u8 yuv422_foramt;
    u8 uv_center;
    u32 multi_red;
    u32 multi_green;
    u32 multi_blue;
    u32 in_lookup_table_addr;
    u32 matrix_addr;
    u32 output_lookup_bypass;
    u32 out_lookup_table_addr;
}color_correction_t;

/**
 * RGB to YUV setup (0x0009200A)
 */
typedef struct rgb_to_yuv_stup_s
{
    u32 cmd_code;
    u32 group_index;
    u16 matrix_values[9];
    s16 y_offset;
    s16 u_offset;
    s16 v_offset;
}rgb_to_yuv_setup_t;

/**
 * RGB to YUV setup of Fade in and Fade out
 */
typedef struct fade_in_fade_out_s {
  u16 matrix_values_start[9];
  u16 matrix_values_end[9];
  s16 y_offset_start;
  s16 u_offset_start;
  s16 v_offset_start;
  s16 y_offset_end;
  s16 u_offset_end;
  s16 v_offset_end;
  u32 start_pts;
  u32 duration;
}fade_in_fade_out_t;

/**
 * Video Fade in and Fade out setup (0x0009200C)
 */
typedef struct rgb_to_yuv_adv_setup_s  {
  u32 cmd_code;
  fade_in_fade_out_t effect[2];

}rgb_to_yuv_adv_setup_t;


/**
 * Black Level Ampifiler Linearization HDR (0x00092010) - new for a12
 */
typedef struct black_level_amplinear_s {
  u32 cmd_code;

  u32 amplinear_offset_ee;
  u32 amplinear_offset_eo;
  u32 amplinear_offset_oe;
  u32 amplinear_offset_oo;

  u32 amp_hdr_offset_ee;
  u32 amp_hdr_offset_eo;
  u32 amp_hdr_offset_oe;
  u32 amp_hdr_offset_oo;

  u32 hdrblend_offset_ee;
  u32 hdrblend_offset_eo;
  u32 hdrblend_offset_oe;
  u32 hdrblend_offset_oo;

} black_level_amplinear_t;


/**
 * Black Level Global Offset for HDR (0x00092011) - new for S2L
 */
typedef struct black_level_hdr_s {
  u32 cmd_code;

  u32 hdr_offset_ee;
  u32 hdr_offset_eo;
  u32 hdr_offset_oe;
  u32 hdr_offset_oo;

} black_level_hdr_t;

/**
 * Thumbnail Blending
 */
typedef struct thumbnail_blending_s {
  u32 src1;
  u32 src2;
  u32 target;
  u32 alpha;
  u16 width;
  u16 height;
  u16 pitch;
}thumbnail_blending_t;

/**
 * Thumbnail Blending setup
 */
typedef struct multi_scene_blending_s {
  u32 cmd_code;
  u8 total;

  thumbnail_blending_t scene[5];

}multi_scene_blending_t;

/**
 * Chroma scale (0x0009200B)
 */
typedef struct chroma_scale_s
{
    u32 cmd_code;
    u16 group_index;
    u16 enable;
    u32 make_legal;
    s16 u_weight_0;
    s16 u_weight_1;
    s16 u_weight_2;
    s16 v_weight_0;
    s16 v_weight_1;
    s16 v_weight_2;
    u32 gain_curver_addr;
}chroma_scale_t;

/**
 * Bad pixel correct setup (0x00093001)
 */
typedef struct bad_pixel_correct_setup
{
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
}bad_pixel_correct_setup_t;

/**
 * CFA noise filter (0x00093002)
 */
typedef struct cfa_noise_filter_s
{
    u32 cmd_code;
    u8 enable:2;
    u8 mode:2;
    u8 shift_coarse_ring1:2;
    u8 shift_coarse_ring2:2;
    u8 shift_fine_ring1:2;
    u8 shift_fine_ring2:2;
    u8 shift_center_red:4;
    u8 shift_center_green:4;
    u8 shift_center_blue:4;
    u8 target_coarse_red;
    u8 target_coarse_green;
    u8 target_coarse_blue;
    u8 target_fine_red;
    u8 target_fine_green;
    u8 target_fine_blue;
    u8 cutoff_red;
    u8 cutoff_green;
    u8 cutoff_blue;
    u16 thresh_coarse_red;
    u16 thresh_coarse_green;
    u16 thresh_coarse_blue;
    u16 thresh_fine_red;
    u16 thresh_fine_green;
    u16 thresh_fine_blue;
}cfa_noise_filter_t;

/**
 * Demoasic Filter (0x00093003)
 */
typedef struct demoasic_filter_s
{
    u32 cmd_code;
    u8 group_index;
    u8 enable;
    u8 clamp_directional_candidates;
    u8 activity_thresh;
    u16 activity_difference_thresh;
    u16 grad_clip_thresh;
    u16 grad_noise_thresh;
    u16 grad_noise_difference_thresh;
    u16 zipper_noise_difference_add_thresh;
    u8 zipper_noise_difference_mult_thresh;
    u8 max_const_hue_factor;
}demoasic_filter_t;

/**
 * Chroma noise filter (0x00093004)
 */
typedef struct chroma_noise_filter_s {
  u32 cmd_code;
  u8  enable;
  u8  radius;
  u16 mode;
  u16 thresh_u;
  u16 thresh_v;
  u16 shift_center_u;
  u16 shift_center_v;
  u16 shift_ring1;
  u16 shift_ring2;
  u16 target_u;
  u16 target_v;
}chroma_noise_filter_t;

/**
 * Strong GrGb mismatch filter (0x00093005)
 */
typedef struct strong_grgb_mismatch_filter_s
{
  u32 cmd_code;
  u16 enable_correction;
  u16 grgb_mismatch_wide_thresh;
  u16 grgb_mismatch_reject_thresh;
  u16 reserved;
}strong_grgb_mismatch_filter_t;

/**
 * Chroma media filter (0x00093006)
 */
typedef struct chroma_median_filter_info_s
{
    u32 cmd_code;
    u16 group_index;
    u16 enable;
    u32 k0123_table_addr;
    u16 u_sat_t0;
    u16 u_sat_t1;
    u16 v_sat_t0;
    u16 v_sat_t1;
    u16 u_act_t0;
    u16 u_act_t1;
    u16 v_act_t0;
    u16 v_act_t1;
}chroma_median_filter_info_t;

/**
 * Luma sharpening (0x00093007)
 */
typedef struct luma_sharpening_s
{
    u32 cmd_code;
    u8  group_index:1;
    u8  enable:1;
    u8  abs:1;
    u8  yuv:2;
#if 1
    u8  coring_control:2;
    u8  add_in_low_pass:1;
    u8  second_input_signed:1;
    u8  second_input_shift:3;
    u8  output_signed:1;
    u8  output_shift:3;
#else
    u8  use_generated_low_pass:3; // A7 only
    u8  input_B_enable:1; // A7 only
    u8  input_C_enable:1; // A7 only
    u8  FIRs_input_from_B_minus_C:1; // A7 only
    u8  coring1_input_from_B_minus_C:1; // A7 only
    u8  reserved:4;
#endif
    u8  clip_low;
    u8  clip_high;

    u8  max_change_down;
    u8  max_change_up;
    u8  max_change_down_center; // A7 only
    u8  max_change_up_center; // A7 only

    // alpha control
    u8  grad_thresh_0;
    u8  grad_thresh_1;
    u8  smooth_shift;
    u8  edge_shift;
    u32 alpha_table_addr;

    // edge control
    u8  wide_weight:4;
    u8  narrow_weight:4;
    u8  edge_threshold_multiplier;
    u16 edge_thresh;
}luma_sharpening_t;

/**
 * Luma Sharpening FIR Config (0x00093009)
 */
typedef struct luma_sharpening_FIR_config_s
{
	u32 cmd_code;
	u8  enable_FIR1:2;
	u8  enable_FIR2:1;
	u16 fir1_clip_low:13;
	u16 coring_mul_shift:2; // for S2L only
	u16 reserved:1;
	u16 fir1_clip_high:13;
	u16 fir2_clip_low;
	u16 fir2_clip_high;
	u32 coeff_FIR1_addr;
	u32 coeff_FIR2_addr;
	u32 coring_table_addr;
}luma_sharpening_FIR_config_t;

/**
 * Luma Sharpening Low Noise Luma (0x0009300A)
 */
typedef struct luma_sharpening_LNL_s
{
    u32 cmd_code;
    u8  group_index;
    u8  enable:1;
    u8  output_normal_luma_size_select:1;
    u8  output_low_noise_luma_size_select:1;
    u8  reserved:5;
    u8  input_weight_red;
    u8  input_weight_green;
    u8  input_weight_blue;
    u8  input_shift_red;
    u8  input_shift_green;
    u8  input_shift_blue;
    u16 input_clip_red;
    u16 input_clip_green;
    u16 input_clip_blue;
    u16 input_offset_red;
    u16 input_offset_green;
    u16 input_offset_blue;
    u8  output_normal_luma_weight_a;
    u8  output_normal_luma_weight_b;
    u8  output_normal_luma_weight_c;
    u8  output_low_noise_luma_weight_a;
    u8  output_low_noise_luma_weight_b;
    u8  output_low_noise_luma_weight_c;
    u8  output_combination_min;
    u8  output_combination_max;
    u32 tone_curve_addr;
}luma_sharpening_LNL_t;

/**
 * Luma Sharpening Tone Control (0x0009300B)
 */
typedef struct luma_sharpening_tone_control_s
{
    u32 cmd_code;
    u8  group_index:4;
    u8  shift_y:4;
    u8  shift_u:4;
    u8  shift_v:4;
    u8  offset_y;
    u8  offset_u;
    u8  offset_v;
    u8  bits_u:4;
    u8  bits_v:4;
    u16 reserved;
    u32 tone_based_3d_level_table_addr;
} luma_sharpening_tone_control_t;

/**
 * Luma Sharpening Blend Config (0x0009300C)
 */
typedef struct luma_sharpening_blend_control_s
{
    u32 cmd_code;
    u32 group_index;
    u16 enable;
    u8  edge_threshold_multiplier;
    u8  iso_threshold_multiplier;
    u16 edge_threshold0;
    u16 edge_threshold1;
    u16 dir_threshold0;
    u16 dir_threshold1;
    u16 iso_threshold0;
    u16 iso_threshold1;
} luma_sharpening_blend_control_t;

/**
 * Luma Sharpening Level Control (0x0009300D)
 */
typedef struct luma_sharpening_level_control_s
{
    u32 cmd_code;
    u32 group_index;
    u32 select;
    u8  low;
    u8  low_0;
    u8  low_delta;
    u8  low_val;
    u8  high;
    u8  high_0;
    u8  high_delta;
    u8  high_val;
    u8  base_val;
    u8  area;
    u16 level_control_clip_low;
    u16 level_control_clip_low2;
    u16 level_control_clip_high;
    u16 level_control_clip_high2;
} luma_sharpening_level_control_t;

/**
 * AAA statistics setup (0x00094001)
 */
typedef struct aaa_statistics_setup_s
{
    u32 cmd_code;
    u32 on : 8;
    u32 auto_shift : 8;
    u32 data_fifo_no_reset : 1;
    u32 data_fifo2_no_reset : 1;
    u32 reserved : 14;
    u32 data_fifo_base;
    u32 data_fifo_limit;
    u32 data_fifo2_base;
    u32 data_fifo2_limit;
    u16 awb_tile_num_col;
    u16 awb_tile_num_row;
    u16 awb_tile_col_start;
    u16 awb_tile_row_start;
    u16 awb_tile_width;
    u16 awb_tile_height;
    u16 awb_tile_active_width;
    u16 awb_tile_active_height;
    u16 awb_pix_min_value;
    u16 awb_pix_max_value;
    u16 ae_tile_num_col;
    u16 ae_tile_num_row;
    u16 ae_tile_col_start;
    u16 ae_tile_row_start;
    u16 ae_tile_width;
    u16 ae_tile_height;
    u16 ae_pix_min_value;
    u16 ae_pix_max_value;
    u16 af_tile_num_col;
    u16 af_tile_num_row;
    u16 af_tile_col_start;
    u16 af_tile_row_start;
    u16 af_tile_width;
    u16 af_tile_height;
    u16 af_tile_active_width;
    u16 af_tile_active_height;
}aaa_statistics_setup_t;

/**
 * AAA pseudo Y setup (0x00094002)
 */
typedef struct aaa_pseudo_y_s
{
    u32 cmd_code;
    u32 mode;
    u32 sum_shift;
    u8 pixel_weight[4];
    u8 tone_curve[32];
}aaa_pseudo_y_t;

#define AAA_FILTER_SELECT_BOTH 0
#define AAA_FILTER_SELECT_CFA  1
#define AAA_FILTER_SELECT_YUV  2

/**
 * AAA histogram setup (0x00094003)
 */
typedef struct aaa_histogram_s
{
    u32 cmd_code;
    u32 mode;
    u16 ae_file_mask[8];
}aaa_histogram_t;

/**
 * AAA statistics setup 1 (0x00094004)
 */

typedef struct aaa_statistics_setup1_s
{
    u32 cmd_code;
    u8 af_horizontal_filter1_mode:4;
    u8 af_filter1_select:4;
    u8 af_horizontal_filter1_stage1_enb;
    u8 af_horizontal_filter1_stage2_enb;
    u8 af_horizontal_filter1_stage3_enb;
    u16 af_horizontal_filter1_gain[7];
    u16 af_horizontal_filter1_shift[4];
    u16 af_horizontal_filter1_bias_off;
    u16 af_horizontal_filter1_thresh;
    u16 af_vertical_filter1_thresh;
    u16 af_tile_fv1_horizontal_shift;
    u16 af_tile_fv1_vertical_shift;
    u16 af_tile_fv1_horizontal_weight;
    u16 af_tile_fv1_vertical_weight;
}aaa_statistics_setup1_t;

/**
 * AAA statistics setup 2 (0x00094005)
 */
 typedef struct aaa_statistics_setup2_s
{
    u32 cmd_code;
    u8 af_horizontal_filter2_mode;
    u8 af_horizontal_filter2_stage1_enb;
    u8 af_horizontal_filter2_stage2_enb;
    u8 af_horizontal_filter2_stage3_enb;
    u16 af_horizontal_filter2_gain[7];
    u16 af_horizontal_filter2_shift[4];
    u16 af_horizontal_filter2_bias_off;
    u16 af_horizontal_filter2_thresh;
    u16 af_vertical_filter2_thresh;
    u16 af_tile_fv2_horizontal_shift;
    u16 af_tile_fv2_vertical_shift;
    u16 af_tile_fv2_horizontal_weight;
    u16 af_tile_fv2_vertical_weight;
}aaa_statistics_setup2_t;

/**
 * AAA statistics setup 3 (0x00094006)
 */
typedef struct aaa_statistics_setup3_s
{
    u32 cmd_code;
    u16 awb_tile_rgb_shift;
    u16 awb_tile_y_shift;
    u16 awb_tile_min_max_shift;
    u16 ae_tile_y_shift;
    u16 ae_tile_linear_y_shift;
    u16 ae_tile_min_max_shift;
    u16 af_tile_cfa_y_shift;
    u16 af_tile_y_shift;
}aaa_statistics_setup3_t;

/*
 * AAA early WB gain (0x00094007)
 */
typedef struct aaa_early_wb_gain_s
{
    u32 cmd_code;
    u32 red_multiplier;
    u32 green_multiplier_even;
    u32 green_multiplier_odd;
    u32 blue_multiplier;
    u8  enable_ae_wb_gain;
    u8  enable_af_wb_gain;
    u8  enable_histogram_wb_gain;
    u8  reserved;
    u32 red_wb_multiplier;
    u32 green_wb_multiplier_even;
    u32 green_wb_multiplier_odd;
    u32 blue_wb_multiplier;
} aaa_early_wb_gain_t;

/*
 * AAA floating tile configuration (0x00094008)
 */
typedef struct aaa_floating_tile_config_s
{
    u32 cmd_code;
    u16 frame_sync_id;
    u16 number_of_tiles;
    u32 floating_tile_config_addr;
} aaa_floating_tile_config_t;

/*
 * AAA tile configuration (internal use only)
 */
typedef struct aaa_tile_config_s
{
    // 1 u32
    u16 awb_tile_col_start;
    u16 awb_tile_row_start;
    // 2 u32
    u16 awb_tile_width;
    u16 awb_tile_height;
    // 3 u32
    u16 awb_tile_active_width;
    u16 awb_tile_active_height;
    //4 u32
    u16 awb_rgb_shift;
    u16 awb_y_shift;
    //5 u32
    u16 awb_min_max_shift;
    u16 ae_tile_col_start;
    //6 u32
    u16 ae_tile_row_start;
    u16 ae_tile_width;
    //7 u32
    u16 ae_tile_height;
    u16 ae_y_shift;
    //8 u32
    u16 ae_linear_y_shift;
    u16 ae_min_max_shift;

    //9 u32
    u16 af_tile_col_start;
    u16 af_tile_row_start;

    //10 u32
    u16 af_tile_width;
    u16 af_tile_height;

    //11 u32
    u16 af_tile_active_width;
    u16 af_tile_active_height;

    //12 u32
    u16 af_y_shift;
    u16 af_cfa_y_shift;

    //13 u32
    u8  awb_tile_num_col;
    u8  awb_tile_num_row;
    u8  ae_tile_num_col;
    u8  ae_tile_num_row;

    //14 u32
    u8  af_tile_num_col;
    u8  af_tile_num_row;
    u8 total_exposures;
    u8 exposure_index;

    //15 u32
    u8 total_slices_x;
    u8 total_slices_y;
    u8 slice_index_x;
    u8 slice_index_y;

    //16 u32
    u16 slice_width;
    u16 slice_height;

    //17 u32
    u16 slice_start_x;
    u16 slice_start_y;

    //18 u32
    u16 black_red;
    u16 black_green;

    //19 u32
    u16 black_blue;
}aaa_tile_config_t;

/**
 * VIN_STAT(main or HDR) (0x00094010)
 */
typedef struct vin_stats_setup_s
{
  u32 cmd_code;

  u8  reserved_0[4];

  u8  vin_stats_main_on;
  u8  vin_stats_hdr_on;
  u8  total_exposures;
  u8  total_slice_in_x;

  u32 main_data_fifo_base;
  u32 main_data_fifo_limit;
  u32 hdr_data_fifo_base;
  u32 hdr_data_fifo_limit;

} vin_stats_setup_t;

// VIN_STAT header
typedef struct vin_stats_tile_config_s
{
  u8  vin_stats_type; // 0: main; 1: hdr
  u8  reserved_0;
  u8  total_exposures;
  u8  blend_index;    // exposure no.

  u8  total_slice_in_x;
  u8  slice_index_x;
  u8  total_slice_in_y;
  u8  slice_index_y;

  u16 vin_stats_slice_left;
  u16 vin_stats_slice_width;

  u16 vin_stats_slice_top;
  u16 vin_stats_slice_height;

  u32 reserved[28]; // pad to 128B

} vin_stats_tile_config_t;


/**
 * Noise filter setup (0x00095001)
 */
typedef struct noise_filter_setup_s
{
    u32 cmd_code;
    u32 strength;
}noise_filter_setup_t;

/**
 * Raw compression (0x00095002)
 */
typedef struct raw_compression_s
{
    u32 cmd_code;
    u32 enable;
    u32 lossy_mode;
    u32 vic_mode;
}raw_compression_t;

/**
 * Raw decompression (0x00095003)
 */
typedef struct raw_decompression_s
{
    u32 cmd_code;
    u32 enable;
    u32 lossy_mode;
    u32 vic_mode;
    u32 image_width;
    u32 image_height;
}raw_decompression_t;

/**
 * Rolling shutter compensation (0x00095004)
 */
typedef struct rolling_shutter_compensation_s
{
    u32 cmd_code;
    u32 enable;
    u32 skew_init_phase_horizontal;
    u32 skew_phase_incre_horizontal;
    u32 skew_phase_incre_vertical;
}rolling_shutter_compensation_t;

/**
 * FPN Calibration (0x00095005)
 */
typedef struct fpn_calibration_s
{
    u32 cmd_code;
    u32 dram_addr;
    u32 width;
    u32 height;
    u32 num_of_frames;
}fpn_calibration_t;

/**
 * HDR Mixer (0x00095006)
 */
typedef struct hdr_mixer_s
{
    u32 cmd_code;
    u16 mixer_mode;
    u8  radius;
    u8  luma_weight_red;
    u8  luma_weight_green;
    u8  luma_weight_blue;
    u16 threshold;
    u8  thresh_delta;
    u8  long_exposure_shift;
    u16 luma_offset;
}hdr_mixer_t;

/*
 * early WB gain (0x00095007)
 */
typedef struct early_wb_gain_s
{
    u32 cmd_code;
    u32 cfa_early_red_multiplier;
    u32 cfa_early_green_multiplier_even;
    u32 cfa_early_green_multiplier_odd;
    u32 cfa_early_blue_multiplier;
    u32 aaa_early_red_multiplier;
    u32 aaa_early_green_multiplier_even;
    u32 aaa_early_green_multiplier_odd;
    u32 aaa_early_blue_multiplier;
} early_wb_gain_t;

/*
 * Set Warp Control (0x00095008)
 */
typedef	struct	set_warp_control_s
{
#define	WARP_CONTROL_DISABLE	0
#define	WARP_CONTROL_ENABLE	1
	u32 cmd_code;
	u32 warp_control;
	u32 warp_horizontal_table_address;
	u32 warp_vertical_table_address;
	u32 actual_left_top_x;
	u32 actual_left_top_y;
	u32 actual_right_bot_x;
	u32 actual_right_bot_y;
	u32 zoom_x;
	u32 zoom_y;
	u32 x_center_offset;
	u32 y_center_offset;

	u8  grid_array_width;
	u8  grid_array_height;
	u8  horz_grid_spacing_exponent;
	u8  vert_grid_spacing_exponent;

	u8  vert_warp_enable;
	u8  vert_warp_grid_array_width;
	u8  vert_warp_grid_array_height;
	u8  vert_warp_horz_grid_spacing_exponent;

	u8  vert_warp_vert_grid_spacing_exponent;
	u8  force_v4tap_disable;
        u8  group_index;  //for multi_idsp_cfg_vin_crop flow
        u8  reserved;
	s32  hor_skew_phase_inc;

	/*
                This one is used for ARM to calcuate the
                dummy window for Ucode, these fields should be
                zero for turbo command in case of EIS. could be
                non-zero valid value only when this warp command is send
                in non-turbo command way.
	*/

	u16 dummy_window_x_left;
	u16 dummy_window_y_top;
	u16 dummy_window_width;
	u16 dummy_window_height;

	/*
                This field is used for ARM to calculate the
                cfa prescaler zoom factor which will affect
                the warping table value. this should also be zeor
                during the turbo command sending.Only valid on the
                non-turbo command time.
	*/
	u16 cfa_output_width;
	u16 cfa_output_height;

	u32 extra_sec2_vert_out_vid_mode;

	/* vertical warp on main ME1 */
	u8 ME1_vwarp_grid_array_width;
	u8 ME1_vwarp_grid_array_height;
	u8 ME1_vwarp_horz_grid_spacing_exponent;
	u8 ME1_vwarp_vert_grid_spacing_exponent;
	u32 ME1_vwarp_table_address;
} set_warp_control_t;

/*
 * Set Chromatic Aberration Warp Control (0x00095009)
 */
typedef	struct	set_chroma_aberration_warp_control_s
{
	u32 cmd_code;
    u16 horz_warp_enable;
    u16 vert_warp_enable;

    u8  horz_pass_grid_array_width;
    u8  horz_pass_grid_array_height;
    u8  horz_pass_horz_grid_spacing_exponent;
    u8  horz_pass_vert_grid_spacing_exponent;
    u8  vert_pass_grid_array_width;
    u8  vert_pass_grid_array_height;
    u8  vert_pass_horz_grid_spacing_exponent;
    u8  vert_pass_vert_grid_spacing_exponent;

    u16 red_scale_factor;
    u16 blue_scale_factor;
    u32 warp_horizontal_table_address;
    u32 warp_vertical_table_address;
} set_chromatic_aberration_warp_control_t;

/**
 * Resampler Coefficient Adjust (0x0009500A)
 */
// control_flag:
#define RESAMP_COEFF_M2        0x1
#define RESAMP_COEFF_M4        0x2
#define RESAMP_COEFF_M8        0x4
#define RESAMP_COEFF_ADJUST    0x7
#define RESAMP_COEFF_RECTWIN   0x8
#define RESAMP_COEFF_LP_STRONG 0x10
#define RESAMP_COEFF_LP_MEDIUM 0x20

// resampler_select
#define RESAMP_SELECT_CFA      0x1
#define RESAMP_SELECT_MAIN     0x2
#define RESAMP_SELECT_PRV_A    0x4
#define RESAMP_SELECT_PRV_B    0x8
#define RESAMP_SELECT_PRV_C    0x10
#define RESAMP_SELECT_CFA_V    0x20
#define RESAMP_SELECT_MAIN_V   0x40
#define RESAMP_SELECT_PRV_A_V  0x80
#define RESAMP_SELECT_PRV_B_V  0x100
#define RESAMP_SELECT_PRV_C_V  0x200

// mode:
#define RESAMP_COEFF_VIDEO     0x0
#define RESAMP_COEFF_ONE_FRAME 0x1

typedef struct resampler_coeffcient_adjust_s
{
    u32 cmd_code;
    u32 control_flag;     // b0: blackman/rectwin, b1: M-2, b2: M-4, b3: medium LP, b4: strong LP
    u16 resampler_select; // b0-5: cfa, main, a, b, c
    u16 mode;             // 0:video (always), 1: still (one-frame)
}resampler_coefficient_adjust_t;

// General HDR Preblend configuration (0x00095010)
typedef struct hdr_preblend_config_s
{
  u32 cmd_code;
  u8  avg_radius;
  u8  avg_method;
  u8  saturation_num_nei;
  u8  reserved;
  u8  luma_avg_weight0;
  u8  luma_avg_weight1;
  u8  luma_avg_weight2;
  u8  luma_avg_weight3;

} hdr_preblend_config_t;


// Preblend threshold configuration (0x00095011)
typedef struct hdr_preblend_threshold_s
{
  u32 cmd_code;

  u16 saturation_threshold0;
  u16 saturation_threshold1;
  u16 saturation_threshold2;
  u16 saturation_threshold3;

} hdr_preblend_threshold_t;

// Preblend blacklevel configuration (0x00095012)
typedef struct hdr_preblend_blacklevel_s
{
  u32 cmd_code;

  u16 bl_offset_ee;
  u16 bl_offset_eo;
  u16 bl_offset_oe;
  u16 bl_offset_oo;

} hdr_preblend_blacklevel_t;

// Preblend exposure (0x00095013)
typedef struct hdr_preblend_exposure_s
{
  u32 cmd_code;

  u16 input_row_offset;
  u16 input_row_height;
  u16 input_row_skip;
  u16 reserved;

} hdr_preblend_exposure_t;

// Preblend alpha (0x00095014)
typedef struct hdr_preblend_alpha_s
{
  u32 cmd_code;
  u8  pre_alpha_mode;
  u8  reserved[3];
  u32 alpha_table_addr; // in dram

} hdr_preblend_alpha_t;




/*
 * Dump IDSP config (0x00096001)
 */
typedef struct dump_idsp_config_s
{
    u32 cmd_code;
    u32 dram_addr;
    u32 dram_size;
    u32 mode;
}dump_idsp_config_t;

//typedef amb_dsp_debug_2_t dump_idsp_config_t;

/*
 * Send IDSP debug command (0x00096002)
 */
typedef struct send_idsp_debug_cmd_s
{
    u32 cmd_code;
    u32 mode;
    u32 param1;
    u32 param2;
    u32 param3;
    u32 param4;
    u32 param5;
    u32 param6;
    u32 param7;
    u32 param8;
}send_idsp_debug_cmd_t;

//typedef amb_dsp_debug_3_t send_idsp_debug_cmd_t;

/*
 * Update IDSP config (0x00096003)
 */
typedef struct update_idsp_config_s
{
    u32 cmd_code;
    u16 section_id;
    u8  mode;
    u8  table_sel;
    u32 dram_addr;
    u32 data_size;
} update_idsp_config_t;

/*
 *  Propcess IDSP Batch Command (0x00096004)
 */
typedef struct process_idsp_batch_cmd_t_s
{
    u32 cmd_code;
    u32 group_index;
    u32 idsp_cmd_buf_addr;
    u32 cmd_buf_size;
} process_idsp_batch_cmd_t;


/**
 * Set zoom factor
 */
typedef struct zoom_factor_s
{
	u32 cmd_code;
	u32 zoom_x;
	u32 zoom_y;
	u32 x_center_offset;
	u32 y_center_offset;
}zoom_factor_t;

/**
 * Set video preview for mctf/mcts configuration
 */

typedef struct video_preview_s
{
	u32 cmd_code;
#define	PREVIEW_ID_A	0
#define	PREVIEW_ID_B	1
	u8 preview_id;
	u8 preview_format;
	u16 preview_w;
	u16 preview_h;
	u8 preview_frame_rate;
	u8 preview_en;
/*
?* new fields for preview source window parameters of preview A and B
?* x/y offset is relative to the upper left
?* corner of the Main window.
?*/
	u16 preview_src_w;
	u16 preview_src_h;
	u16 preview_src_x_offset;
	u16 preview_src_y_offset;

	u32 preview_freeze_enabled:1;
	u32 preview_freeze_offset_x:10;
	u32 preview_freeze_offset_y:10;
	u32 resv:11;

}video_preview_t;

/**
 * multi stream video preview setup
 */
typedef struct multi_stream_preview_s
{
	u32 cmd_code;
	u16 preview_id;
	u16 num_preview_ins;
	u32 preview_src : 8;
	u32 dis_dev_x_loc : 12;
	u32 dis_dev_y_loc : 12;
	u16 preview_width;
	u16 preview_height;
}multi_stream_preview_t;

/**
 * enable second stream
 */
typedef struct ena_second_stream_s
{
	u32 cmd_code;
	u32 primary_stream_channel      : 24;
	u32 frame_rate             :  8;
	u16 output_width;
	u16 output_height;
	u16 encode_width;
	u16 encode_height;
}ena_second_stream_t;

/*
 * set active win ctr offset
 */
typedef struct set_ative_win_ctr_ofs_s
{
	u32 cmd_code;
	s32 x_offset;
	s32 y_offset;
	s32 x_frm_mv;
	s32 y_frm_mv;
} set_ative_win_ctr_ofs_t;

/**
 *  H264/JPEG encoding mode
 *-------------------------------------------------
 *  1. Video Preprocessing		(0x00003001)
 *  2. Fast AAA capture 		(0x00003002)
 *  3. H264 encode 			(0x00003004)
 *  4. H264 encode from memory 		(0x00003005) invalid
 *  5. H264 bits FIFO update 		(0x00003006)
 *  6. H264 encoding stop		(0x00003007)

 *  7. Still capture 			(0x00004001)
 *  8. JPEG encode/rescale from memory  (0x00004002)
 *  9. JPEG bits FIFO update 		(0x00004003)
 * 10. Free RAW/YUV422 pictures buffers (0x00004004)
 * 11. JPEG/RQW/YUV/422 Stop 		(0x00004005)
 * 12. Vid Fade In Out			(0x00004007)
 * 13. MJPEG encode with h264		(0x00004008)
 * 14. OSD insert in MJPEG and H264	(0x00004009)
 * 15. YUV422 capture			(0x00004010)
 * 16. Send cavlc result			(0x00004011)
 */

/**
 * modify cmd proc delay.
 */
typedef struct cmd_proc_delay_s
{
	u32 cmd_code;
	u32 api_proc_delay;
} cmd_proc_delay_t;

typedef cmd_proc_delay_t vid_cmd_proc_delay_t;

/*
 *
 */
typedef struct raw_encode_video_setup_cmd_s
{
	u32 cmd_code;
	u32 sensor_raw_start_daddr;		/* frm 0 start address */
	u32 daddr_offset;	/* offset from sensor_raw_start_daddr to the start address of the next frame in DRAM */
	u32 dpitch : 16;		/* buffer pitch in bytes */
	u32 num_frames : 8;	/* num of frames stored in DRAM starting from sensor_raw_start_daddr */
	u32 reserved : 8;
} raw_encode_video_setup_cmd_t;

/**
 * Video Preprocessing (0x3001)
 */
#define RGB_RAW         0
#define YUV_422_INTLC   1
#define YUV_422_PROG    2

typedef enum
{
    APTINA_AR0331 = 0xe,
} dsp_sensor_id_t;

typedef struct video_preproc_s
{
	u32 cmd_code;
	u32 input_format : 8;
	u32 sensor_id : 8;
	u32 keep_states : 8;
	u32 vin_frame_rate : 8;

	u16 vidcap_w;
	u16 vidcap_h;

	u16 main_w;
	u16 main_h;

	u16 encode_w;
	u16 encode_h;

	u16 encode_x;
	u16 encode_y;

	u16 preview_w_A;
	u16 preview_h_A;

	u32 input_center_x;
	u32 input_center_y;
	u32 zoom_factor_x;
	u32 zoom_factor_y;
	u32 aaa_data_fifo_start;
	u32 sensor_readout_mode;
	u8 noise_filter_strength;
	u8 bit_resolution;
	u8 bayer_pattern;

#define	PREVIEW_FORMAT_PROGRESS			0
#define	PREVIEW_FORMAT_INTERLACE		1
#define	PREVIEW_FORMAT_DEFAULT_IMAGE_PROGRESS	2
#define	PREVIEW_FORMAT_DEFAULT_IMAGE_INTERLACE	3
#define	PREVIEW_FORMAT_PROGRESS_TOP_FIELD	4
#define	PREVIEW_FORMAT_PROGRESS_BOT_FIELD	5
#define	PREVIEW_FORMAT_NO_OUTPUT		6

	u8 preview_format_A : 4;
	u8 preview_format_B : 3;
	u8 no_pipelineflush : 1;

	u16 preview_w_B;
	u16 preview_h_B;

	u8 preview_frame_rate_A;
	u8 preview_frame_rate_B;
	u8 preview_A_en : 4;
	u8 preview_B_en : 4;
	u8 vin_frame_rate_ext;

	u8 vdsp_int_factor;
	u8 main_out_frame_rate;
	u8 main_out_frame_rate_ext;
	u8 vid_skip;

	u16 EIS_delay_count:2;           //Used to indicate that delay for EIS
	u16 Vert_WARP_enable:1;          //Used to indicate that Vertical  WARP will be used
	u16 no_vin_reset_exiting:1;      //Used to indicate that we do not need to resetting vin and need to wait
									//out the vin before exiting VIDEO mode to TIMER mode
									//This one is only useful for non DIS pipeline
	u16 force_stitching:1;     // when set to 1, oversampling is disabled
	u16 reserved:11;
	u16 reserved_2;

	u32 cmdReadDly;					//Used to indicate the turbo command time related to normal interrrupts.
									//First bit indicate its direction,
									//1: before the VDSP interrupt, i.e., turbo command deadline is the
									//absolute value of cmdReadDly's audio clk before the next normal interrupts.
									//0: after the VDSP interrutps, i.e., turbo command deadline is the
									//absolute value of cmdReadDly's audio clk after the next normal interrutps.
/*
?* new fields for preview source window parameters of preview A and B
?* x/y offset is relative to the upper left
?* corner of the Main window.
?*/
	u16 preview_src_w_A;
	u16 preview_src_h_A;
	u16 preview_src_x_offset_A;
	u16 preview_src_y_offset_A;
	u16 preview_src_w_B;
	u16 preview_src_h_B;
	u16 preview_src_x_offset_B;
	u16 preview_src_y_offset_B;
	u32 still_iso_config_daddr;	/* DRAM address containing still ISO config param as defined */
}video_preproc_t;

/**
 * Fast AAA capture
 */
typedef struct fast_aaa_capture_s
{
	u32 cmd_code;
	u16 input_image_width;
	u16 input_image_height;
	u32 start_record_id;
}fast_aaa_capture_t;

/**
 * H264 encode
 */
typedef struct h264_encode_s
{
	u32 cmd_code;
	u32 bits_fifo_next;
	u32 info_fifo_next;
	u32 start_encode_frame_no;
	u32 encode_duration;
	u8 is_flush;
	u8 enable_slow_shutter;
	u8 res_rate_min;		// between 0 and 100
	s8 alpha;			// between -6 and 6
	s8 beta;			// between -6 and 6
	u8 en_loop_filter;		// 1 enable loop filtering.
	u8 max_upsampling_rate;
	u8 slow_shutter_upsampling_rate;

	// SPS
	u8 frame_cropping_flag;
	/* 0 -- Main Profile with CABAC;
	 * 1 -- High Profile;
	 * 2 -- Main Profile with CAVLC;
	 * 3 -- Baseline Profile
	 */
	u8 profile   :2;
	u8 reserved2 :6;
	u16 frame_crop_left_offset;

	u16 frame_crop_right_offset;
	u16 frame_crop_top_offset;

	u16 frame_crop_bottom_offset;
	u8 num_ref_frame;
	u8 log2_max_frame_num_minus4;

	u8 log2_max_pic_order_cnt_lsb_minus4;
	u8 sony_avc : 1;
	u8 gaps_in_frame_num_value_allowed_flag : 1;
	u8 reserved : 6;
	u16 height_mjpeg_h264_simultaneous;

	u16 width_mjpeg_h264_simultaneous;
	u16 vui_enable : 1;
	u16 aspect_ratio_info_present_flag : 1;
	u16 overscan_info_present_flag : 1;
	u16 overscan_appropriate_flag : 1;
	u16 video_signal_type_present_flag : 1;
	u16 video_full_range_flag : 1;
	u16 colour_description_present_flag : 1;
	u16 chroma_loc_info_present_flag : 1;
	u16 timing_info_present_flag : 1;
	u16 fixed_frame_rate_flag : 1;
	u16 nal_hrd_parameters_present_flag : 1;
	u16 vcl_hrd_parameters_present_flag : 1;
	u16 low_delay_hrd_flag : 1;
	u16 pic_struct_present_flag : 1;
	u16 bitstream_restriction_flag : 1;
	u16 motion_vectors_over_pic_boundaries_flag : 1;
																// aspect_ratio_info_present_flag

	u16 SAR_width;
	u16 SAR_height;

	// video_signal_type_present_flag
	u8 video_format;
	// colour_description_present_flag
	u8 colour_primaries;
	u8 transfer_characteristics;
	u8 matrix_coefficients;

	// chroma_loc_info_present_flag
	u8 chroma_sample_loc_type_top_field : 4;
	u8 chroma_sample_loc_type_bottom_field : 4;
	u8 aspect_ratio_idc;
	u8 reserved3;

	// bitstream_restriction_flag
	u32 max_bytes_per_pic_denom : 8;
	u32 max_bits_per_mb_denom : 8;
	u32 log2_max_mv_length_horizontal : 8;
	u32 log2_max_mv_length_vertical	: 8;

	u16 num_reorder_frames;
	u16 max_dec_frame_buffering;

	u32 I_IDR_sp_rc_handle_mask	:8;
	u32 IDR_QP_adj_str		:8;
	u32 IDR_class_adj_limit		:8;
	u32 reserved_1			:8;
	u32 I_QP_adj_str		:8;
	u32 I_class_adj_limit		:8;
	u32 firstGOPstartB		:8;
	u32 au_type			:8;
}h264_encode_t;

typedef h264_encode_t vid_encode_t;

/**
 * H264 encode from memory
 */
typedef struct h264_encode_from_memory_param_s
{
	u32 cmd_code;
	u16 vidcap_w;
	u16 vidcap_h;
	u16 vidcap_pitch;
	u16 main_w;
	u16 main_h;
	u16 encode_w;
	u16 encode_h;
	u16 encode_x;
	u16 encode_y;
	u16 preview_w;
	u16 preview_h;
	u32 input_center_x;
	u32 input_center_y;
	u32 zoom_factor_x;
	u32 zoom_factor_y;
	u8 num_images;
	u32 h264_bits_fifo_start;
	u32 h264_info_fifo_start;
	u32 input_image_addr_Y_UV[8];
}h264_encode_from_memory_param_t;

typedef h264_encode_from_memory_param_t vid_encode_from_memory_param_t;

/**
 * H264 bits FIFO update
 */
typedef struct h264_encode_bits_fifo_update_s
{
	u32 cmd_code;
	u32 bits_output_fifo_end;
}h264_encode_bits_fifo_update_t;

typedef h264_encode_bits_fifo_update_t vid_encode_bits_fifo_update_t;

/**
 * H264/JPEG Encoding stop
 */
typedef struct h264_encode_stop_s
{
	u32 cmd_code;
#define	H264_STOP_IMMEDIATELY       0
#define H264_STOP_ON_NEXT_IP        1
#define H264_STOP_ON_NEXT_I         2
#define H264_STOP_ON_NEXT_IDR       3
#define EMERG_STOP                  0xff
	u32 stop_method;
}h264_encode_stop_t;

typedef h264_encode_stop_t vid_encode_stop_t;

/**
 * Video Encode Rotate
 */
typedef struct encode_rotate_s
{
	u32 cmd_code;
	u32 hflip : 1 ;
	u32 vflip : 1 ;
	u32 rotate : 1 ;
	u32	vert_black_bar_en:1;
	u32 vert_black_bar_MB_width:8;
}encode_rotate_t;

typedef encode_rotate_t vid_encode_rotate_t;

/**
 * Encode Sync Cmd (0x0000600C)
 */
typedef struct enc_sync_cmd_s
{
	u32 cmd_code;
	u32 target_pts;
	u32 cmd_daddr;
	u32 enable_flag : 4;
	u32 force_update_flag : 4;
	u32 reserved : 24;
}enc_sync_cmd_t;

/**
 *  ** Video HISO Config Update
 *   **/
/**
 *  ** Video HISO Config Update
 *   **/
/**
 *  ** Video HISO Config Update
 *   **/
typedef struct video_hiso_config_update_flag_s
{
	u32 hiso_config_common_update : 1 ;
	u32 hiso_config_color_update : 1 ;
	u32 hiso_config_mctf_update : 1 ;
	u32 hiso_config_step1_update : 1 ;
	u32 hiso_config_step2_update : 1 ;
	u32 hiso_config_step3_update : 1 ;
	u32 hiso_config_step4_update : 1 ;
	u32 hiso_config_step5_update : 1 ;
	u32 hiso_config_step6_update : 1 ;
	u32 hiso_config_step7_update : 1 ;
	u32 hiso_config_step8_update : 1 ;
	u32 hiso_config_step9_update : 1 ;
	u32 hiso_config_step10_update : 1 ;
	u32 hiso_config_step11_update : 1 ;
	u32 hiso_config_step15_update : 1 ;
	u32 hiso_config_step16_update : 1 ;
	u32 hiso_config_vwarp_update : 1 ;
	u32 hiso_config_flow_update : 1 ;
	u32 hiso_config_aaa_update : 1 ; // AAA setup update
	u32 reserved : 13 ;
} video_hiso_config_update_flag_t;

typedef struct video_hiso_config_update_s
{
	u32 cmd_code;
	u32 hiso_param_daddr;

	// config update flags
	union
	{
		video_hiso_config_update_flag_t flag;
		u32 word;
	} loadcfg_type;
} video_hiso_config_update_t;

typedef struct hash_verfication_s
{
	u32 cmd_code;
	u8 hash_input[8];
} hash_verification_t;

/* use for returning results for command HASH_VERIFICATION */
typedef struct hash_verification_msg_s
{
	u32 msg_code ;
	u8 hash_output[4];
}hash_verification_msg_t;

/**
 * Still capture
 */
#define	JPEG_OUTPUT_SELECT_MAIN_JPEG_BITS	0x1
#define	JPEG_OUTPUT_SELECT_THUMB_JPEG_BITS	0x4
#define	JPEG_OUTPUT_SELECT_MAIN_YUV			0x8
#define	JPEG_OUTPUT_SELECT_PREVIEW_YUV		0x10

typedef struct still_capture_s
{
	u32 cmd_code;
	u8 output_select;
	u8 input_format;
	u8 vsync_skip;
	u8 resume;
	u32 number_frames_to_capture;
	u16 vidcap_w;
	u16 vidcap_h;
	u16 main_w;
	u16 main_h;
	u16 encode_w;
	u16 encode_h;
	u16 encode_x;
	u16 encode_y;
	u16 preview_w;
	u16 preview_h;
	u16 thumbnail_w;
	u16 thumbnail_h;
	u32 input_center_x;
	u32 input_center_y;
	u32 zoom_factor_x;
	u32 zoom_factor_y;
	u32 jpeg_bits_fifo_start;
	u32 jpeg_info_fifo_start;
	u32 sensor_readout_mode;
	u8 sensor_id;
	u8 field_format;
	u8 sensor_resolution;
	u8 sensor_pattern;
	u8 first_line_field_0;
	u8 first_line_field_1;
	u8 first_line_field_2;
	u8 first_line_field_3;
	u8 first_line_field_4;
	u8 first_line_field_5;
	u8 first_line_field_6;
	u8 first_line_field_7;
	u16 preview_w_B;
	u16 preview_h_B;
	u16 raw_cap_cntl;

/*	Added by Colin chen for the High ISO mode of Still processing */
#define	STILL_PROCESS_MODE_NORMAL       0
#define	STILL_PROCESS_MODE_HIGH_ISO	1
#define STILL_PROCESS_MODE_LOW_ISO      2
	u8 still_process_mode;
#define YUV_START_NO_CNTRL  (0)
#define YUV_START_EXP_MODE  (1)
#define YUV_START_SLW_MODE  (2)
#define YUV_START_SLW_STATS_MODE  (3)
	u8 yuv_proc_mode;
	u32 still_process_data_dram_addr;
	u32 raw_cap_hw_rsc_ptr;

	u8  disable_quickview_HDMI:1;
	u8  disable_quickview_LCD:1;
	u8  no_vin_reset_exiting:1;  //Used to indicate that we do not need to resetting vin and need to wait
									//out the vin before exiting RJPEG mode to TIMER mode
	u8  reserved_1:5;
	u8  jpg_enc_cntrl;
	u16 thumbnail_active_h;
}still_capture_t;

/**
 * Still capture in record
 */
typedef struct still_cap_in_rec_s
{
	u32 cmd_code;			// should be STILL_CAPTURE_IN_REC
	u16 main_jpg_w;
	u16 main_jpg_h;
	u16 encode_w;
	u16 encode_h;
	u16 encode_x;
	u16 encode_y;
	u16 blank_period_duration;	// absolute duration, time unit: 1/60000 second.
	u8 is_use_compaction;		// if compaction is needed
	u8 is_thumbnail_ena;
} still_cap_in_rec_t;


/**
 * Still processing from memory
 */
typedef struct still_proc_from_memory_s
{
	u32 cmd_code;
	u8  output_select;
	u8  input_format;
	u8  bayer_pattern;
	u8  resolution;
	u32 input_address;
	u32 input_chroma_address;
	u16 input_pitch;
	u16 input_chroma_pitch;
	u16 input_h;
	u16 input_w;
	u16 main_w;
	u16 main_h;
	u16 encode_w;
	u16 encode_h;
	u16 encode_x;
	u16 encode_y;
	u16 preview_w_A;
	u16 preview_h_A;
	u16 thumbnail_w;
	u16 thumbnail_h;
	u32 input_center_x;
	u32 input_center_y;
	u32 zoom_factor_x;
	u32 zoom_factor_y;
	u32 jpeg_bits_fifo_start;
	u32 jpeg_info_fifo_start;
	u16 preview_w_B;
	u16 preview_h_B;

	u16 cap_cntl;
/*
	#define	STILL_PROCESS_MODE_NORMAL   0
	#define	STILL_PROCESS_MODE_HIGH_ISO	1
	are defined above.
	Added by Colin chen for the High ISO mode of Still processing
*/
	u8 still_process_mode;
	u8 still_process_mode_padding;
	u32 still_process_data_dram_addr;

	u8 disable_quickview_HDMI:1;
	u8 disable_quickview_LCD:1;
	u8 no_vin_reset_exiting:1;   	//Used to indicate that we do not need to resetting vin and need to wait
									//out the vin before exiting RJPEG mode to TIMER mode
	u8  reserved_1:5;
	u8 reserved;
	u16 thumbnail_active_h;
}still_proc_from_memory_t;

//STILL_DEC_ADV_PIC_ADJ_CMD (cmd code 0x0000401C)
typedef struct STILL_DEC_ADV_PIC_ADJ_CMDtag{
    u32 cmd_code;

    u32 enable : 8;//0:default;1:depend on user; when use has set the enable bit, no way to back default, user need to control them to default by themselves
    u32 reserved : 24;
} still_dec_adv_pic_adj_t;



/**
 *
 * Set ALPHA CHANNEL Here
 */
typedef	struct	set_alpha_channel_s
{
	u32	cmd_code;

	u32	luma_alpha_addr;
	u32	chroma_alpha_addr;

	u16	luma_alpha_pitch;
	u16	luma_alpha_width;
	u16	luma_alpha_height;

	u16	chroma_alpha_pitch;
	u16	chroma_alpha_width;
	u16	chroma_alpha_height;
} set_alpha_channel_t;

/**
 *
 * Modify Frame Buffer
 * Used to get some DRAM from DSP in some mode
 * and return it back to DSP when finished.
 * User only send this command when the DSP is in IDEL State.
 */

typedef	struct	modify_frame_buffer_s
{
	u32	cmd_code;
	u32	dramStart; //32 bits DRAM ADDRSS of the Frame Buffer. Must be 32 bits aligned
	u32	dramSize;  //32 bits Size of the DRAM for DSP.
} modify_frame_buffer_t;

/**
 * JPEG bits FIFO update
 */
typedef struct jpeg_bits_fifo_update_s
{
	u32 cmd_code;
	u32 bits_fifo_end;
}jpeg_bits_fifo_update_t;

/**
 * Free RAW/YUV422 pictures buffers
 */
typedef struct free_raw_yuv_picture_buffer_s
{
	u32 cmd_code;
	u32 number_of_raw_pic_consumed;
	u32 raw_last_addr_consumed;
	u32 number_of_thumbnail_pic_consumed;
	u32 thumbnail_last_addr_consumed;
	u32 number_of_encode_YUV_pic_consumed;
	u32 encode_yuv_last_addr_consumed;
}free_raw_yuv_picture_buffer_t;

/**
 * MJPEG capture
 */
typedef struct mjpeg_capture_s
{
	u32 cmd_code;
	u32 bits_fifo_next;
	u32 info_fifo_next;
	u32 start_encode_frame_no;
	u32 encode_duration;
	u8  framerate_control_M;
	u8  framerate_control_N;
	u16 reserve;
}mjpeg_capture_t;

/**
 * MJPEG capture with 264
 */
typedef struct mjpeg_capture_with_264_s
{
	u32 cmd_code;
	u32 enable;
}mjpeg_capture_with_264_t;

/**
 * OSD insert in MJPEG and H264
 */
typedef struct osd_insert_s
{
	u32 cmd_code;
	u32 enable;
	u32 y_osd_addr_h264;
	u32 uv_osd_addr_h264;// 420
	u16 osd_width_h264;
	u16 osd_pitch_h264;
	u16 osd_height_h264;
	u16 osd_vertical_position_h264;
	u32 y_osd_addr_mjpeg;
	u32 uv_osd_addr_mjpeg;//422
	u16 osd_width_mjpeg;
	u16 osd_pitch_mjpeg;
	u16 osd_height_mjpeg;
	u16 osd_vertical_position_mjpeg;
}osd_insert_t;

/**
 * Enhanced OSD insert
 */
typedef struct osd_blend_s
{
	u32 cmd_code;
	u8 chan_id;
	u8 stream_id;
	u8 enable;
	u8 still_osd;

	u32 osd_addr_y;
	u32 osd_addr_uv;

	u32 alpha_addr_y;  // alpha mask must be the same size as osd, (osd_width*osd_height), value 0~0xff
	u32 alpha_addr_uv;

	u16 osd_width;
	u16 osd_pitch;
	u16 osd_height;
	u16 osd_start_x;  // (start_x, start_y)=(0,0) refers to top-left pixel of main image.
	u16 osd_start_y;
	u16 reserved_2;

	u8 blend_area2_enable;
	u8 reserved_3;
	u16 blend_area2_width;

	u16 blend_area2_pitch;
	u16 blend_area2_height;

	u16 blend_area2_start_x;
	u16 blend_area2_start_y;
	u32 blend_area2_y_addr;
	u32 blend_area2_uv_addr;
	u32 blend_area2_alpha_addr;

	u8 blend_area3_enable;
	u8 reserved_4;
	u16 blend_area3_width;

	u16 blend_area3_pitch;
	u16 blend_area3_height;

	u16 blend_area3_start_x;
	u16 blend_area3_start_y;
	u32 blend_area3_y_addr;
	u32 blend_area3_uv_addr;
	u32 blend_area3_alpha_addr;
}osd_blend_t;

/**
 * Interval capture
 */
typedef struct interval_cap_s
{
	u32 cmd_code;
#define IDLE		(0)
#define INITIATE	(1)
#define CAPTURE		(2)
#define TERMINATE	(3)
	u32 action;
	u32 num_of_frame;
}interval_cap_t;

/**
 * YUV422 capture
 */
typedef struct yuv422_capture_s
{
	u32 cmd_code;
	u32 num2Capture;
}yuv422_capture_t;

/**
 * JPEG/RQW/YUV/422 Stop
 */
typedef struct jpeg_stop_s
{
	u32 cmd_code;
}jpeg_stop_t;

/**
 * Vid Fade In Out
 * 0: fade in start, 1: fade in stop, 2: fade out start, 3: fade out stop
 */
typedef struct vid_fade_in_out_s
{
	u32 cmd_code;
	u8  cmd;
}vid_fade_in_out_t;

/**
 * Send cavlc result
 */
typedef struct cavlc_result_s
{
	u32 cmd_code;
	u32 num_of_cavlc_results : 8;
	u32 pjpg_rd_size : 24;
	u32 cavlcBits_A[6];
	u32 cavlcBits_B[6];
	u32 cavlcBits_C[6];
	u32 cavlcBits_D[6];
	u32 totCavlcBits[6];
}cavlc_result_t;

/**
 * DIS algorithm parameter
 */
typedef struct dis_algo_params_s {
	u32 cmd_code;	//

	u8 x_pan_thd;
	u8 y_pan_thd;
	u8 x_dis_strength  : 7;
	u8 flg_mc_enabled  : 1;
	u8 y_dis_strength  : 7;
	u8 flg_rsc_enabled : 1;

	u8 x_rsc_strength  : 7;
	u8 flg_dis_enabled : 1;
	u8 y_rsc_strength  : 7;
	u8 blk_select      : 1;
	u16 focal_length;

	u16 sensor_cell_width           : 12;
	u16 x_win_pullback_speed_if_pan : 4;
	u16 sensor_cell_height          : 12;
	u16 y_win_pullback_speed_if_pan : 4;

	u32* dis_hist_dram_addr;
	u32 dis_hist_dram_size;

	u16 me_y_top_blk_start_pos : 12;
	s16 me_x_res               : 4;
	u16 me_y_bot_blk_start_pos : 12;
	s16 me_y_res               : 4;

	u16 me_y_top_blk_height : 12;
	u16 me_x_bin            : 4;
	u16 me_y_bot_blk_height : 12;
	u16 me_y_bin            : 4;

	u32 me_lamda        : 8;
	u32 sensor_row_time : 17;
	u32 mc_beta         : 7;

	s32 x_mov : 16;
	s32 y_mov : 16;

	s32 x_skew : 16;
	s32 y_skew : 16;
} dis_algo_params_t;

/**
 * DIS histogram information
 */
typedef	struct dis_hist_header_s {
	u32 seq_nbr : 8;
	u32 sem_id : 2;
	u32 hist_x_precision_in_quater_pixel : 3;
	u32 hist_y_precision_in_quater_pixel : 3;
	u32 reserved : 16;

	u32 hist_x_hist_max_item_nbr : 16; //For example, the range of the hist x dimension would be: 128: [-64,63]
	u32 hist_y_hist_max_item_nbr : 16; //As above

	u32 hist_start_mb_nbr : 16;//  This one and the following one are used to specify the block area in the picture for the ME histogram
	u32 hist_total_mb_nbr : 16;//
} dis_hist_header_t;

/**
 *  H264/JPEG decoding mode
 *-------------------------------------------------
 *  1. H264 decode		 	(0x00005002)
 *  2. JPEG decode		 	(0x00005003)
 *  3. RAW picture decode	 	(0x00005004)
 *  4. Rescale Postprocessing	 	(0x00005005)
 *  5. H264 decode bits FIFO update	(0x00005006)
 *  6. H264 playback speed	 	(0x00005007)
 *  7. H264 trickplay		 	(0x00005008)
 *  8. Decode stop		 	(0x00005009)
 *  9. Multi-scene decode	 	(0x00005010)
 * 10. Capture video picture	 	(0x00005011)
 */

/**
 * H264 decode
 */
typedef struct h264_decode_s
{
	u32 cmd_code;
	u32 bits_fifo_start;
	u32 bits_fifo_end;
	u32 num_pics;
	u32 num_frame_decode;
	u32 first_frame_display;
	u32 fade_in_on;
	u32 fade_out_on;
}h264_decode_t;

/**
 * JPEG decode
 */
typedef struct jpeg_decode_s
{
	u32 cmd_code;
	u32 bits_fifo_start;
	u32 bits_fifo_end;
	u8 main_rotation;
	u8 ycbcr_position;
	u16 reserved;
	u32 frame_duration;
	u32 num_frame_decode;
	u8 already_decoded;
	u8 sec_rotation;
	u8 image_360;
}jpeg_decode_t;

/**
 * RAW picture decode
 */
typedef struct raw_pic_decode_s
{
	u32 cmd_code;
	u32 raw_pic_addr;
	u16 input_width;
	u16 input_height;
	u16 input_pitch;
	u8 rotation;
	u8 bayer_pattern;
	u8 resolution;
	u8 already_decoded;
	u16 reserved;
}raw_pic_decode_t;

/**
 * Rescale postprocessing
 */
typedef struct rescale_postproc_s
{
  u32 cmd_code;
  u16 input_center_x;
  u16 input_center_y;
  u16 display_win_offset_x;
  u16 display_win_offset_y;
  u16 display_win_width;
  u16 display_win_height;
  u32 zoom_factor_x;
  u32 zoom_factor_y;
  u8 apply_yuv;
  u8 apply_luma;
  u8 apply_noise;
  u8 pip_enable;
  u16 pip_x_offset;
  u16 pip_y_offset;
  u16 pip_x_size;
  u16 pip_y_size;
  u16 sec_display_win_offset_x;
  u16 sec_display_win_offset_y;
  u16 sec_display_win_width;
  u16 sec_display_win_height;
  u32 sec_zoom_factor_x;
  u32 sec_zoom_factor_y;

  u32 reserved                : 12;
  u32 postp_start_degree     : 9; // indicate start degree in 360 degree postp. (0-360)
  u32 postp_end_degree      : 9; // indicate end degree in 360 degree postp. (0-360)
  u32 horz_warp_enable : 1;
  u32 vert_warp_enable  : 1;

  u32 warp_horizontal_table_address;
  u32 warp_vertical_table_address;

  u8  grid_array_width;
  u8  grid_array_height;
  u8  horz_grid_spacing_exponent;
  u8  vert_grid_spacing_exponent;
  u8  vert_warp_grid_array_width;
  u8  vert_warp_grid_array_height;
  u8  vert_warp_horz_grid_spacing_exponent;
  u8  vert_warp_vert_grid_spacing_exponent;

  // the following parameters will be used to set vout
  // window for seamless transition
  u8  vout0_flip;
  u8  vout0_rotate;
  u16 vout0_win_offset_x;
  u16 vout0_win_offset_y;
  u16 vout0_win_width;
  u16 vout0_win_height;

  u8  vout1_flip;
  u8  vout1_rotate;
  u16 vout1_win_offset_x;
  u16 vout1_win_offset_y;
  u16 vout1_win_width;
  u16 vout1_win_height;

  // the following parameters will be used for 3d playback zooming
  u16 left_input_offset_x;
  u16 left_input_offset_y;
  u16 left_input_width;
  u16 left_input_height;
  u16 left_output_offset_x;
  u16 left_output_offset_y;
  u16 left_output_width;
  u16 left_output_height;

  u16 right_input_offset_x;
  u16 right_input_offset_y;
  u16 right_input_width;
  u16 right_input_height;
  u16 right_output_offset_x;
  u16 right_output_offset_y;
  u16 right_output_width;
  u16 right_output_height;

  u16 postp_start_radius; // start radius in 360 degree postp. (0-half of video height)
  u16 postp_end_radius; // end radius in 360 degree postp. (0-half of video height)
}rescale_postproc_t;

/**
 * Universal dram to dram rescale
 */
typedef struct rescale_dram_to_dram_s {
  u32 cmd_code;
  u32 input_y_daddr;
  u32 input_uv_daddr;
  u16 input_offset_x;
  u16 input_offset_y;
  u16 input_pic_width;
  u16 input_pic_height;
  u16 input_pic_dpitch;
  u8 input_ch_fmt;
  u8 input_rotate_direction;
  u32 output_y_daddr;
  u32 output_uv_daddr;
  u16 output_offset_x;
  u16 output_offset_y;
  u16 output_pic_width;
  u16 output_pic_height;
  u16 output_pic_dpitch;
  u8 output_ch_fmt;
}rescale_dram_to_dram_t;

/**
 * Update post processing angle in image/video 360 degree playback
 */
typedef struct image_video_360_postp_angle_update_s {
  u32 cmd_code;
  u16 postp_start_degree; // start degree, range 0-360
  u16 postp_end_degree; // end degree, range 0-360
}image_video_360_postp_angle_update_t;

/**
 * H264 decode bits FIFO update
 */
typedef struct h264_decode_bits_fifo_update_s
{
	u32 cmd_code;
	u32 bits_fifo_start;
	u32 bits_fifo_end;
	u32 num_pics;
}h264_decode_bits_fifo_update_t;

/**
 * H264 playback speed
 */
typedef struct h264_playback_speed_s
{
	u32 cmd_code;
	u16 speed;
	u8 scan_mode;
	u8 direction;
}h264_playback_speed_t;

/**
 * H264 trickplay
 */
typedef struct h264_trickplay_s
{
	u32 cmd_code;
	u8 mode;
}h264_trickplay_t;

/**
 * H264 stop
 */
typedef struct h264_decode_stop_s
{
	u32 cmd_code;
	u8 stop_flag;
}h264_decode_stop_t;

typedef enum {
    JPEG_THUMB,
    H264_I_ONLY,
    RAW_PIC,
    YUV_PIC,
    JPEG_MAIN,
} SCENE_TYPE;

typedef enum {
    NORMAL,
    FLIPPING_BLOCK,
    FLIPPING_PAGE,
    ALBUM_MIRROR,
} EFFECT_TYPE;

/**
 * Multi-scene-setup
 */
typedef struct multi_scene_setup_s
{
    u32 cmd_code;
    u8   if_save_thumbnail; /* indicate if we need to allocate a regular thumbnail buffer*/
    u8   total_thumbnail;
    u16 saving_thumbnail_width;
    u16 saving_thumbnail_height;
    u8   if_save_large_thumbnail; /* indicate if we need to allocate a large size thumbnail buffer*/
    u8   total_large_thumbnail;
    u16 saving_large_thumbnail_width;
    u16 saving_large_thumbnail_height;
    u8   if_capture_large_size_thumbnail; /* indicate if we need to capture a large size multi scene picture */
    u8 reserved;
    u16 large_thumbnail_pic_width;  /* picture width of large size multi-scene */
    u16 large_thumbnail_pic_height; /* picture height of large size multi-scene */
    u8   visual_effect_type; /* special effect type */
    u8 reserved1;
    u16 extra_total_thumbnail; /*number of thumnails if total > 1000 */
    u16 reserved2;
    u32 second_decoded_buffer_size;
}multi_scene_setup_t;

/**
 * Multi-scene decode
 */
typedef struct scene_structure_s
{
    s16 offset_x; //for TV
    s16 offset_y;
    u16 width;
    u16 height;
    s16 sec_offset_x; //for LCD
    s16 sec_offset_y;
    u16 sec_width;
    u16 sec_height;

    u32 source_base;
    u32 source_size : 24;
    u32 source_type  :  4;
    u32 rotation     :  4; // for main rotation
    u32 thumbnail_id :  8;
    u32 decode_only  :  4;
    u32 sec_rotation     :  4; // for LCD rotation
    u32 reserved     : 16;
}scene_structure_t;

typedef struct multi_scene_decode_s
{
    u32 cmd_code;

    u32 total_scenes    :  8;
    u32 start_scene_num :  8;
    u32 scene_num       :  8;
    u32 end             :  4;
    u32 fast_mode       :  4;

    scene_structure_t scene[4];

}multi_scene_decode_t;

typedef enum {
    SMALL_SIZE,
    LARGE_SIZE,
    MAIN_SIZE,
} THUMBNAIL_TYPE;

/* Multi scene decode adv scene structure */
typedef struct scene_structure_adv_s
{
    s16 input_offset_x; //for input cropping
    s16 input_offset_y;
    u16 input_width;
    u16 input_height;

    s16 main_output_offset_x; //for TV
    s16 main_output_offset_y;
    u16 main_output_width;
    u16 main_output_height;
    s16 sec_output_offset_x; //for LCD
    s16 sec_output_offset_y;
    u16 sec_output_width;
    u16 sec_output_height;

    u32 source_base;
    u32 source_size     : 24;
    u32 source_type     :  4;
    u32 rotation        :  4;
    u32 thumbnail_id    :  8;
    u32 decode_only     :  4;
    u32 luma_gain       :  8; // for luma scaling
    u32 thumbnail_type  :  2; // indicate different thumbnail size, 0: small size thumbnail 1: large size thumbnail.
    u32 reserved        : 10;
}scene_structure_adv_t;

/* Multi scene decode adv command */
typedef struct multi_scene_decode_adv_s
{
	u32 cmd_code;
	u32 total_scenes                :  8;
	u32 start_scene_num         :  8;
	u32 scene_num                  :  8;
	u32 end                             :  4;
	u32 buffer_source_only      :  2; // indicate if the scene source is TYPE3=YUV_PIC only
	u32 fast_mode                   :  2;

	scene_structure_adv_t scene[3];
}multi_scene_decode_adv_t;

/* Multi scene decode adv scene structure for 1000 thumbnail feature*/
typedef struct scene_structure_adv_2_s
{
  s16 input_offset_x; //for input cropping
  s16 input_offset_y;
  u16 input_width;
  u16 input_height;

  s16 main_output_offset_x; //for TV
  s16 main_output_offset_y;
  u16 main_output_width;
  u16 main_output_height;
  s16 sec_output_offset_x; //for LCD
  s16 sec_output_offset_y;
  u16 sec_output_width;
  u16 sec_output_height;

  u32 source_base;
  u32 source_size     : 24;
  u32 source_type     :  4;
  u32 main_rotation        :  4;
  u32 sec_rotation       :  4;
  u32 thumbnail_id    :  11; // to support 2048 thumbnails
  u32 decode_only     :  1;
  u32 luma_gain       :  8; // for luma scaling
  u32 thumbnail_type  :  2; // indicate different thumbnail size, 0: small size thumbnail 1: large size thumbnail.
  u32 thumb_crop_flag :  2;
  u32 decode_buf_flag :  1;
  u32 reserved        :  3;
}scene_structure_adv_2_t;

/* Multi scene decode adv command for 1000 thumbnail feature */
typedef struct multi_scene_decode_adv_2_s
{
  u32 cmd_code;
  u32 total_scenes                :  8;
  u32 start_scene_num         :  8;
  u32 scene_num                  :  8;
  u32 end                             :  1;
  u32 update_lcd_only          :  1; //indicate if we only update lcd
  u32 buffer_source_only      :  1; // indicate if the scene source is TYPE3=YUV_PIC only
  u32 fast_mode                   :  1;
  u32 use_single_prev_filter  :  1; // indicate if using only 1 preview filter
  u32 reserved                     :  3;

  scene_structure_adv_2_t scene[3];

  /* osd syn feature only support at adv2 multis commandi, for backward
   * compatable, put at end of the structure */
  u32 sec_osd_daddr;                  /* if !Null->do the osd update with "end == 1",LCD */
  u32 main_osd_daddr;                 /* if !Null->do the osd update with "end == 1",TV */
  u32 rst_num_daddr;                   /* if Null->normal multi-scene command, else run the partial decode flow and the restart_interval numbers needs to be bigger than one*/
}multi_scene_decode_adv_2_t;

/* Multi scene decode adv scene structure for 1000 thumbnail feature*/
typedef struct scene_structure_display_s {
  s16 input_offset_x; //for input cropping
  s16 input_offset_y;
  u16 input_width;
  u16 input_height;

  s16 main_output_offset_x; //for TV
  s16 main_output_offset_y;
  u16 main_output_width;
  u16 main_output_height;
  s16 sec_output_offset_x; //for LCD
  s16 sec_output_offset_y;
  u16 sec_output_width;
  u16 sec_output_height;

  u32 main_rotation        :  4;
  u32 sec_rotation       :  4;
  u32 thumbnail_id    :  11; // to support 2048 thumbnails
  u32 luma_gain       :  8; // for luma scaling
  u32 thumbnail_type  :  2; // indicate different thumbnail size, 0: small size thumbnail 1: large size thumbnail.
  u32 reserved        : 3;
}scene_structure_display_t;

/* Multi scene decode adv command for 1000 thumbnail feature */
typedef struct multi_scene_display_s {
  u32 cmd_code;
  u32 total_scenes                :  8;
  u32 start_scene_num         :  8;
  u32 scene_num                  :  8;
  u32 end                             :  1;
  u32 update_lcd_only          :  1; // indicate if we only update lcd
  u32 use_single_prev_filter  :  1; // indicate if using only 1 preview filter
  u32 reserved                     :  5;

  scene_structure_display_t scene[4];

  /* osd syn feature only support at adv2 multis commandi, for backward
   * compatable, put at end of the structure */
  u32 sec_osd_daddr;                  /* if !Null->do the osd update with "end == 1",LCD */
  u32 main_osd_daddr;                 /* if !Null->do the osd update with "end == 1",TV */
}multi_scene_display_t;

/* jpeg decode thumbnail warping structure */
typedef struct  jpeg_decode_thumbnail_warp_s {
    u32 cmd_code;
    // src and dst thumbnail id
    u8 src_thm_id;
    u8 dst_thm_id;
    u16 origin_height;
    u16 mirror_height;
    u16 reserved;

    // if draw border on src thumbnail buffer
    u8 if_draw_border;
    u8 border_y;
    u8 border_u;
    u8 border_v;

    // mirror
    u8 if_mirror_effect;
    u8 mirror_luma_gain;

    //warp related field
    u8 horz_warp_control;
    u8 vert_warp_control;
    u32 warp_horizontal_table_address;
    u32 warp_vertical_table_address;
    u8 grid_array_width;
    u8 grid_array_height;
    u8 horz_grid_spacing_exponent;
    u8 vert_grid_spacing_exponent;
    u8 vert_warp_grid_array_width;
    u8 vert_warp_grid_array_height;
    u8 vert_warp_horz_grid_spacing_exponent;
    u8 vert_warp_vert_grid_spacing_exponent;
} jpeg_decode_thumbnail_warp_t;


/**
 * Capture video picture
 */
typedef struct capture_video_pic_s
{
	u32 cmd_code;
	u32 coded_pic_base;
	u32 coded_pic_limit;
	u32 thumbnail_pic_base;
	u32 thumbnail_pic_limit;
	u16 thumbnail_width;
	u16 thumbnail_height;
	u16 thumbnail_letterbox_strip_width;
	u16 thumbnail_letterbox_strip_height;
	u8 thumbnail_letterbox_strip_y;
	u8 thumbnail_letterbox_strip_cb;
	u8 thumbnail_letterbox_strip_cr;
	u8 reserved0;
	u32 quant_matrix_addr;
	u16 target_pic_width;
	u16 target_pic_height;
	u32 pic_structure : 8;
	u32 reserved1 : 24;
	u32 screennail_pic_base;
	u32 screennail_pic_limit;
	u16 screennail_width;
	u16 screennail_height;
	u16 screennail_letterbox_strip_width;
	u16 screennail_letterbox_strip_height;
	u8 screennail_letterbox_strip_y;
	u8 screennail_letterbox_strip_cb;
	u8 screennail_letterbox_strip_cr;
	u8 reserved2;
	u32 quant_matrix_addr_thumbnail;
	u32 quant_matrix_addr_screennail;
	u32 captured_luma_addr;
	u32 captured_chroma_addr;
	u8  capture_yuv;
	u8  resured3[3];
}capture_video_pic_t;

/**
 * Capture video picture to MJpeg
 */
typedef struct capture_video_pic_to_mjpeg_s {
  u32 cmd_code;
  u32 coded_pic_num;    //buffer number
  u32 coded_pic_base;    //buffer start address
  u32 coded_pic_max_size;    //each buffer size
  u32 coded_pic_pts_base;    // address point to u32[coded_pic_num] the indicate pic pts of each pic
  u32 coded_pic_size_base;   // address point to u32[coded_pic_num] the indicate pic size of each pic
  u32 quant_matrix_addr;
  u16 target_pic_width;    //same of pitch?
  u16 target_pic_height;
}capture_video_pic_to_mjpeg_t;

/**
 * Capture still picture
 */
typedef struct capture_still_pic_s
{
  u32 cmd_code; /* 0x00005012 */
  u32 coded_pic_base; /*DRAM address to hold JPEG to encode */
  u32 coded_pic_limit;
  u32 thumbnail_pic_base; /*DRAM address to store JPEG in thumbnail form */
  u32 thumbnail_pic_limit;
  u16 thumbnail_width;
  u16 thumbnail_height;
  u16 thumbnail_letterbox_strip_width; /*will change order when it's finalized */
  u16 thumbnail_letterbox_strip_height;
  u8 thumbnail_letterbox_strip_y; /*Y value for painting letterbox */
  u8 thumbnail_letterbox_strip_cb; /*Cb value for painting letterbox */
  u8 thumbnail_letterbox_strip_cr; /*Cr value for painting letterbox */
  u8 capture_multi_scene; /* capture the multi scene picture */
  u32 quant_matrix_addr; /*DRAM address to hold quant matrix, refer to capture_video_pic_s*/
  u32 screennail_pic_base; /*DRAM address to store JPEG in screennail form */
  u32 screennail_pic_limit;
  u16 screennail_width;
  u16 screennail_height;
  u16 screennail_letterbox_strip_width; /*will change order when it's finalized */
  u16 screennail_letterbox_strip_height;
  u8 screennail_letterbox_strip_y; /*Y value for painting letterbox */
  u8 screennail_letterbox_strip_cb; /*Cb value for painting letterbox */
  u8 screennail_letterbox_strip_cr; /*Cr value for painting letterbox */
  u8 rotate_direction;
  u16 input_offset_x; /* offset x to crop the input picture */
  u16 input_offset_y; /* offset y to crop the input picture */
  u16 input_width; /* default=0: capture original size */
  u16 input_height; /* default=0 */
  u16 target_pic_width; /* regular capture width */
  u16 target_pic_height; /* regular capture height */
  u32 quant_matrix_addr_thumbnail;
  u32 quant_matrix_addr_screennail;
  u32 input_luma_addr;
  u32 input_chroma_addr;
  u16 input_orig_pic_width;
  u16 input_orig_pic_height;
  u16 input_pitch;
  u8 input_chroma_format;
  u8 yuv_capture_on_flag;
}capture_still_pic_t;

/**
 * JPEG freeze
 */
typedef struct jpeg_freeze_s
{
	u32 cmd_code;
	u8  freeze_state;
	u8  reserved[3];
}jpeg_freeze_t;

/**
 * JPEG DECODE ABORT
 */
typedef struct jpeg_decode_abort_s {
	u32 cmd_code;
	u8  jpeg_decode_abort;    /* this is a edge trigger and pull down when break out from jpeg decode */
	u8  reserved[3];
}jpeg_decode_abort_t;


/*********************************************************************/
/* MESSAGE */

/**
 *  The length of a record is fixed at 128 bytes.
 *  DSP firmware continuously writes result record into the queue.
 *  ARM reads the result record out and both advance their buffer
 *  pointer. DSP doesn't handshake with ARM on result queue usage.
 */
typedef struct dsp_result_queue
{
	u32 time_stamp;
	u32 frame_number;
	u32 params[30];
} dsp_result_queue_t;

/**
 * H264/JPEG encoding message
 */
typedef struct dis_status_s {
	u8	enable : 1;
	u8	rsc_enable : 1;
	u8	proc : 1;
	u8	reserved : 5;
	s8	x_angular_velocity;
	s8	y_angular_velocity;
	u8	max_angular_velocity;
} dis_status_t;

typedef enum {
	STOP_V_ENC = 0,
	TIMER_MODE = 1,
	VIDEO_MODE = 2,
	SJPEG_MODE = 3,
	MJPEG_MODE = 4,
	RJPEG_MODE = 5,
	ENC_UNKNOWN_MODE = 7,
} encode_operation_mode_t;

typedef enum {
	ENC_IDLE_STATE = 0x00,
	ENC_BUSY_STATE = 0x01,
	ENC_PAUSE_STATE = 0x02,
	ENC_FLUSH_STATE = 0x03,
	ENC_UNKNOWN_STATE = 0xFF,
} encode_state_t;

typedef enum {
	CAPTURE_STATE_IDLE = 0,
	CAPTURE_STATE_RUNNING = 1,
} capture_state_t;

typedef enum {
	ERR_CODE_NO_ERROR = 0,
	ERR_CODE_VSYNC_LOSS = 1,
} err_code_t;

typedef enum {
	DSP_STATUS_MSG_ENCODE = 0x00000000,
	DSP_STATUS_MSG_DECODE = 0x00000001,
	DSP_STATUS_MSG_FBP_INFO = 0x00000002,
	DSP_STATUS_MSG_EFM_CREATE_FBP = 0x00000003,
	DSP_STATUS_MSG_EFM_REQ_FB = 0x00000004,
	DSP_STATUS_MSG_EFM_HANDSHAKE = 0x00000005,
	DSP_STATUS_MSG_HASH_VERIFICATION  = 0x00000006,
} DSP_STATUS_MSG_CODE;

typedef struct encode_msg_s
{
	u32 dsp_operation_mode;	/* DSP_STATUS_MSG */
	u32 timecode;
	u32 prev_cmd_seq;
	u32 prev_num_cmds;
	u16 encode_operation_mode   :4;
	u16 encode_state    :4;
	u16 capture_state   :4;
	u16 enc_sync_cmd_update_fail :1;
	u16 reserved1    :3;
	u16 raw_pic_pitch;
	u16 main_y_pitch;
	u16 main_me1_pitch;
	u16 preview_A_y_pitch;
	u16 preview_A_me1_pitch;
	u16 preview_B_y_pitch;
	u16 preview_B_me1_pitch;
	u16 preview_C_y_pitch;
	u16 preview_C_me1_pitch;
	u32 vin_main_data_fifo_next;
	u32 vin_hdr_data_fifo_next;
	u32 aaa_data_fifo_next;
	u32 yuv_aaa_data_fifo_next;
	u32 raw_pic_addr;
	u32 main_y_addr;
	u32 main_uv_addr;
	u32 main_me1_addr;
	u32 preview_A_y_addr;
	u32 preview_A_uv_addr;
	u32 preview_A_me1_addr;
	u32 preview_B_y_addr;
	u32 preview_B_uv_addr;
	u32 preview_B_me1_addr;
	u32 preview_C_y_addr ;
	u32 preview_C_uv_addr ;
	u32 preview_C_me1_addr ;
	u32 total_pic_encoded_h264_mode;
	u32 total_pic_encoded_mjpeg_mode;
	u32 raw_cap_cnt;
	u32 yuv_pic_cnt;
	u32 sw_pts;
	u32 hw_pts;
	u32 vcap_audio_clk_counter;
	u32 qp_histo_addr;
	u8  encode_stream_state[8];
	u32 err_code;
	u32 vsync_cnt;
	u16 int_y_pitch;		/* Pitch for the intermediate Y of dewarp mode */
	u16 reserved2;
	u32 int_y_addr;
	u32 int_uv_addr;
	u32 main_me0_addr;
	u32 flexible_dram_used;
	u32 flexible_dram_free;
} encode_msg_t;

/*
 * H264/JPEG decoding message
 */
typedef struct decode_msg_s
{
	u32 dsp_operation_mode;	/* DSP_STATUS_MSG */
	u32 timecode;
	u32 prev_cmd_seq;
	u32 prev_num_cmds;
	u32 latest_clock_counter;
	u32 latest_pts;
	u32 jpeg_frame_count;
	u32 yuv422_y_addr;
	u32 yuv422_uv_addr;
	u32 h264_bits_fifo_next;
	u32 jpeg_bits_fifo_next;
	u32 decode_state;
	u32 error_status;
	u32 total_error_count;
	u32 decoded_pic_number;
	u32 jpeg_thumbnail_size;
	u32 jpeg_rescale_buf_pitch; /* pitch of scaled JPEG after decoding */
	u16 jpeg_rescale_buf_width;
	u16 jpeg_rescale_buf_height;
	u32 jpeg_rescale_buf_address_y; /* Y address of scaled JPEG after decoding */
	u32 jpeg_rescale_buf_address_uv;
	u32 second_rescale_buf_pitch; /* pitch of scaled JPEG after decoding */
	u16 second_rescale_buf_width;
	u16 second_rescale_buf_height;
	u32 second_rescale_buf_address_y; /* Y address of second scaled after decoding */
	u32 second_rescale_buf_address_uv;
	u32 jpeg_y_addr; /*Y address of decoded JPEG */
	u32 jpeg_uv_addr;
	u32 jpeg_pitch; /* DRAM pitch of decoded JPEG */
	u32 jpeg_width; /* width of decoded JPEG */
	u32 jpeg_height;
	u32 jpeg_capture_count;
	u32 jpeg_screennail_size;
	u32 jpeg_thumbnail_buf_dbase; /*multi scene thumbnail buffer base address*/
	u32 jpeg_thumbnail_buf_pitch; /*buffer pitch of each thumbnail buffer */
	u32 jpeg_large_thumbnail_buf_dbase; /*multi scene large thumbnail buffer base address*/
	u32 jpeg_large_thumbnail_buf_pitch; /*buffer pitch of each large thumbnail buffer */
	u16 jpeg_cabac_message_queue_fullness; /* fullness of cabac message queue */
	u16 jpeg_rescale_message_queue_fullness; /* fullness of rescale message queue */
	u32 thumbnail_status_dbase; /* thumbnail status base address */
	u8  thumbnail_blend_done_num;  /* thumbnail blend done number */
	u8  rescale_done_num;  /* rescale done number */
	u8  osd_upd_idx;    /* osd sync feature, report the idx, TV | LCD(each LSB 4 bits from dram address) */
	u32 jpeg_y_addr_2nd; /*Y address of decoded JPEG */
	u32 jpeg_uv_addr_2nd;
}decode_msg_t;

/*
 * Use for returning results for IPCAM_GET_FRAME_BUF_POOL_INFO
 */
typedef struct
{
	u32 msg_code;	/* DSP_STATUS_MESSAGE */
	u32 raw_start_daddr;
	u32 raw_size;
	u32 main_cap_start_daddr;
	u32 main_cap_size;
	u32 main_cap_me1_start_daddr;
	u32 main_cap_me1_size;
	u32 prev_A_start_daddr;
	u32 prev_A_size;
	u32 prev_A_me1_start_daddr;
	u32 prev_A_me1_size;
	u32 prev_B_start_daddr;
	u32 prev_B_size;
	u32 prev_B_me1_start_daddr;
	u32 prev_B_me1_size;
	u32 prev_C_start_daddr;
	u32 prev_C_size;
	u32 prev_C_me1_start_daddr;
	u32 prev_C_me1_size;
	u32 warped_main_cap_start_daddr;
	u32 warped_main_cap_size;
	u32 warped_main_cap_me1_start_daddr;
	u32 warped_main_cap_me1_size;
	u32 warped_int_cap_start_daddr;
	u32 warped_int_main_cap_size;
	u32 efm_start_daddr;
	u32 efm_size;
	u32 efm_me1_start_daddr;
	u32 efm_me1_size;
	u32 reserved[3];
} frame_buf_info_msg_t;

/*
 * Use for returning results for IPCAM_EFM_CREATE_FRAME_BUF_POOL
 */
typedef struct efm_creat_frm_buf_pool_msg_s
{
	u32 msg_code;	/* DSP_STATUS_MESSAGE */
	u32 err_code;	/* 0: success; 1: fail */
	u32 max_num_yuv	: 8;
	u32 max_num_me1 : 8;
	u32 yuv_pitch : 16;
	u32 me1_pitch : 16;
	u32 reserved : 16;
} efm_creat_frm_buf_pool_msg_t;

/*
 * Use for returning results for IPCAM_EFM_REQ_FRAME_BUF
 */
typedef struct efm_req_frm_buf_msg_s
{
	u32 msg_code;	/* DSP_STATUS_MESSAGE */
	u32 err_code;
	u32 y_addr;
	u32 uv_addr;
	u32 me1_addr;
	u32 yuv_fId : 16;
	u32 me1_fId : 16;
} efm_req_frm_buf_msg_t;

/*
 * Use for returning results for IPCAM_EFM_HANDSHAKE
 */
typedef struct efm_handshake_msg_s
{
	u32 msg_code;	/* DSP_STATUS_MESSAGE */
	u32 err_code;
} efm_handshake_msg_t;


/*
 * Ambarealla experimental commands
 */
typedef struct amb_digital_gain_saturation_s
{
	u32 cmd_code;
	u32 level;
}amb_digital_gain_saturation_t;

typedef struct amb_chroma_median_filter_s
{
	u32 cmd_code;
	u32 enable;
}amb_chroma_median_filter_t;

typedef struct amb_luma_directional_filter_s
{
	u32 cmd_code;
	u32 strength;
}amb_luma_directional_filter_t;

typedef struct amb_luma_sharpen_s
{
	u32 cmd_code;
	u32 enable;
	u32 lp_coef[6];
	u32 alpha_max_pos;
	u32 alpha_max_neg;
	u32 thresh_gradient;
}amb_luma_sharpen_t;

typedef struct amb_main_resampler_s
{
	u32 cmd_code;
	u8 strength_luma;
	u8 strength_chroma;
}amb_main_resampler_t;

typedef struct amb_cfa_resampler_bandwidth_s
{
	u32 cmd_code;
	u8 strength;
}amb_cfa_resampler_bandwidth_t;

typedef struct amb_dsp_debug_0_s
{
	u32 cmd_code;
	u32 dram_addr;
}amb_dsp_debug_0_t;

typedef struct amb_dsp_debug_1_s
{
	u32 cmd_code;
	u32 dram_addr;
}amb_dsp_debug_1_t;

typedef struct amb_aaa_statistics_debug_s
{
	u32 cmd_code;
	u32 on : 8;
	u32 reserved : 24;
	u32 data_fifo_base;
	u32 data_fifo_limit;
	u16 ae_awb_tile_num_col;
	u16 ae_awb_tile_num_row;
	u16 ae_awb_tile_col_start;
	u16 ae_awb_tile_row_start;
	u16 ae_awb_tile_width;
	u16 ae_awb_tile_height;
	u32 ae_awb_pix_min_value;
	u32 ae_awb_pix_max_value;
	u16 ae_awb_tile_rgb_shift;
	u16 ae_awb_tile_y_shift;
	u16 ae_awb_tile_min_max_shift;
	u16 af_tile_num_col;
	u16 af_tile_num_row;
	u16 af_tile_col_start;
	u16 af_tile_row_start;
	u16 af_tile_width;
	u16 af_tile_height;
	u16 af_tile_active_width;
	u16 af_tile_active_height;
	u16 af_tile_focus_value_shift;
	u16 af_tile_y_shift;
	u8 af_horizontal_filter1_mode;
	u8 af_horizontal_filter1_stage1_enb;
	u8 af_horizontal_filter1_stage2_enb;
	u8 af_horizontal_filter1_stage3_enb;
	u16 af_horizontal_filter1_gain[7];
	u16 af_horizontal_filter1_shift[4];
	u16 af_horizontal_filter1_bias_off;
	u16 af_vertical_filter1_thresh;
	u8 af_horizontal_filter2_mode;
	u8 af_horizontal_filter2_stage1_enb;
	u8 af_horizontal_filter2_stage2_enb;
	u8 af_horizontal_filter2_stage3_enb;
	u16 af_horizontal_filter2_gain[7];
	u16 af_horizontal_filter2_shift[4];
	u16 af_horizontal_filter2_bias_off;
	u16 af_vertical_filter2_thresh;
	u16 af_tile_fv1_horizontal_shift;
	u16 af_tile_fv1_vertical_shift;
	u16 af_tile_fv1_horizontal_weight;
	u16 af_tile_fv1_vertical_weight;
	u16 af_tile_fv2_horizontal_shift;
	u16 af_tile_fv2_vertical_shift;
	u16 af_tile_fv2_horizontal_weight;
	u16 af_tile_fv2_vertical_weight;
}amb_aaa_statistics_debug_t;

typedef struct amb_aaa_statistics_debug1_s
{
	u32 cmd_code;
	u32 on : 8;
	u32 reserved : 24;
	u32 data_fifo_base;
	u32 data_fifo_limit;
	u16 ae_awb_tile_num_col;
	u16 ae_awb_tile_num_row;
	u16 ae_awb_tile_col_start;
	u16 ae_awb_tile_row_start;
	u16 ae_awb_tile_width;
	u16 ae_awb_tile_height;
	u32 ae_awb_pix_min_value;
	u32 ae_awb_pix_max_value;
	u16 ae_awb_tile_rgb_shift;
	u16 ae_awb_tile_y_shift;
	u16 ae_awb_tile_min_max_shift;
	u16 af_tile_num_col;
	u16 af_tile_num_row;
	u16 af_tile_col_start;
	u16 af_tile_row_start;
	u16 af_tile_width;
	u16 af_tile_height;
	u16 af_tile_active_width;
	u16 af_tile_active_height;
	u16 af_tile_focus_value_shift;
	u16 af_tile_y_shift;
	u8 af_horizontal_filter1_mode;
	u8 af_horizontal_filter1_stage1_enb;
	u8 af_horizontal_filter1_stage2_enb;
	u8 af_horizontal_filter1_stage3_enb;
	u16 af_horizontal_filter1_gain[7];
	u16 af_horizontal_filter1_shift[4];
	u16 af_horizontal_filter1_bias_off;
	u16 af_vertical_filter1_thresh;
	u16 af_tile_fv1_horizontal_shift;
	u16 af_tile_fv1_vertical_shift;
	u16 af_tile_fv1_horizontal_weight;
	u16 af_tile_fv1_vertical_weight;
}amb_aaa_statistics_debug1_t;

typedef struct amb_aaa_statistics_debug2_s
{
	u32 cmd_code;
	u32 on : 8;
	u32 reserved : 24;
	u32 data_fifo_base;
	u32 data_fifo_limit;
	u16 ae_awb_tile_num_col;
	u16 ae_awb_tile_num_row;
	u16 ae_awb_tile_col_start;
	u16 ae_awb_tile_row_start;
	u16 ae_awb_tile_width;
	u16 ae_awb_tile_height;
	u32 ae_awb_pix_min_value;
	u32 ae_awb_pix_max_value;
	u16 ae_awb_tile_rgb_shift;
	u16 ae_awb_tile_y_shift;
	u16 ae_awb_tile_min_max_shift;
	u16 af_tile_num_col;
	u16 af_tile_num_row;
	u16 af_tile_col_start;
	u16 af_tile_row_start;
	u16 af_tile_width;
	u16 af_tile_height;
	u16 af_tile_active_width;
	u16 af_tile_active_height;
	u16 af_tile_focus_value_shift;
	u16 af_tile_y_shift;
	u8 af_horizontal_filter2_mode;
	u8 af_horizontal_filter2_stage1_enb;
	u8 af_horizontal_filter2_stage2_enb;
	u8 af_horizontal_filter2_stage3_enb;
	u16 af_horizontal_filter2_gain[7];
	u16 af_horizontal_filter2_shift[4];
	u16 af_horizontal_filter2_bias_off;
	u16 af_vertical_filter2_thresh;
	u16 af_tile_fv2_horizontal_shift;
	u16 af_tile_fv2_vertical_shift;
	u16 af_tile_fv2_horizontal_weight;
	u16 af_tile_fv2_vertical_weight;
}amb_aaa_statistics_debug2_t;

typedef struct amb_dsp_special_s{
	u32 cmd_code;
	u32 p0;
	u32 p1;
	u32 p2;
}amb_dsp_special_t;

typedef struct amb_bad_pixel_crop_s{
	u32 cmd_code;
	u8 enable;
	u16 offset_horiz;
	u16 offset_vert;
}amb_bad_pixel_crop_t;

//#ifdef TYPE_DEF

typedef enum {
	VOUT_ID_A           = 0,
	VOUT_ID_B           = 1,
} vout_id_t;

typedef enum {
	VOUT_SRC_DEFAULT_IMG = 0,
	VOUT_SRC_BACKGROUND  = 1,
	VOUT_SRC_ENC         = 2,
	VOUT_SRC_DEC         = 3,
	VOUT_SRC_H264_DEC    = 3,
	VOUT_SRC_MPEG2_DEC   = 5,
	VOUT_SRC_MPEG4_DEC   = 6,
	VOUT_SRC_MIXER_A     = 7,
	VOUT_SRC_VCAP        = 8,
	VOUT_SRC_VOUTPIC     = 9,
} vout_src_t;

typedef enum {
	VOUT_DATA_SRC_NORMAL = 0,
	VOUT_DATA_SRC_DRAM = 1,
} vout_data_src_t;

typedef enum {
	OSD_SRC_MAPPED_IN    = 0,
	OSD_SRC_DIRECT_IN    = 1,
} osd_src_t;

typedef enum {
	OSD_MODE_UYV565    = 0,
	OSD_MODE_AYUV4444  = 1,
	OSD_MODE_AYUV1555  = 2,
	OSD_MODE_YUV1555   = 3,
} osd_dir_mode_t;

typedef enum {
	CSC_DIGITAL	= 0,
	CSC_ANALOG	= 1,
	CSC_HDMI	= 2,
} csc_type_t;

//#endif

// (cmd code 0x00007001)
typedef struct vout_mixer_setup_s {
  u32 cmd_code;
  u16 vout_id;
  u8  interlaced;
  u8  frm_rate;
  u16 act_win_width;
  u16 act_win_height;
  u8  back_ground_v;
  u8  back_ground_u;
  u8  back_ground_y;
  u8  mixer_444;
  u8  highlight_v;
  u8  highlight_u;
  u8  highlight_y;
  u8  highlight_thresh;
  u8  reverse_en;
  u8  csc_en;
  u8  reserved[2];
  u32 csc_parms[9];
} vout_mixer_setup_t;

// (cmd code 0x00007002)
typedef struct vout_video_setup_s {
  u32 cmd_code;
  u16 vout_id;
  u8  en;
  u8  src;
  u8  flip;
  u8  rotate;
  u8  data_src;
  u8  reserved;
  u16 win_offset_x;
  u16 win_offset_y;
  u16 win_width;
  u16 win_height;
  u32 default_img_y_addr;
  u32 default_img_uv_addr;
  u16 default_img_pitch;
  u8  default_img_repeat_field;
  u8  reserved2;
} vout_video_setup_t;

// (cmd code 0x00007003)
typedef struct vout_default_img_setup_s {
  u32 cmd_code;
  u16 vout_id;
  u16 reserved;
  u32 default_img_y_addr;
  u32 default_img_uv_addr;
  u16 default_img_pitch;
  u8  default_img_repeat_field;
  u8  reserved2;
} vout_default_img_setup_t;

// (cmd code 0x00007004)
typedef struct vout_osd_setup_s {
  u32 cmd_code;
  u16 vout_id;
  u8  en;
  u8  src;
  u8  flip;
  u8  rescaler_en;
  u8  premultiplied;
  u8  global_blend;
  u16 win_offset_x;
  u16 win_offset_y;
  u16 win_width;
  u16 win_height;
  u16 rescaler_input_width;
  u16 rescaler_input_height;
  u32 osd_buf_dram_addr;
  u16 osd_buf_pitch;
  u8  osd_buf_repeat_field;
  u8  osd_direct_mode;
  u16 osd_transparent_color;
  u8  osd_transparent_color_en;
  u8  osd_swap_bytes;
  u32 osd_buf_info_dram_addr;

  u8  osd_video_en;     // for VOUT0 and VOUT1
  u8  osd_video_mode;   //0 for YUV422, 1 for YUV420
  u16 osd_video_win_offset_x;
  u16 osd_video_win_offset_y;
  u16 osd_video_win_width;
  u16 osd_video_win_height;
  u16 osd_video_buf_pitch;
  u32 osd_video_buf_dram_addr;

  u32 osd_buf_dram_sync_addr;  //for seamless DEC<->ENC osd sync
} vout_osd_setup_t;

// (cmd code 0x00007005)
typedef struct vout_osd_buf_setup_s {
  u32 cmd_code;
  u16 vout_id;
  u16 reserved;
  u32 osd_buf_dram_addr;
  u16 osd_buf_pitch;
  u8  osd_buf_repeat_field;
  u8  reserved2;
} vout_osd_buf_setup_t;

typedef struct {
  u32 osd_buf_dram_addr;
  u16 osd_buf_pitch;
  u16 osd_buf_repeat:1;
  u16 reserved:15;
  u32 osd_video_buf_dram_addr;
  u16 osd_video_buf_pitch;
  u16 reserved_2;
} dram_osd_t;

// (cmd code 0x00007006)
typedef struct vout_osd_clut_setup_s {
  u32 cmd_code;
  u16 vout_id;
  u16 reserved;
  u32 clut_dram_addr;
} vout_osd_clut_setup_t;

// (cmd code 0x00007007)
typedef struct vout_display_setup_s {
  u32 cmd_code;
  u16 vout_id;
  u16 dual_vout_vysnc_delay_ms_x10;
  u32 disp_config_dram_addr;
} vout_display_setup_t;

// (cmd code 0x00007008)
typedef struct vout_dve_setup_s {
  u32 cmd_code;
  u16 vout_id;
  u16 reserved;
  u32 dve_config_dram_addr;
} vout_dve_setup_t;

// (cmd code 0x00007009)
typedef struct vout_reset_s {
  u32 cmd_code;
  u16 vout_id;
  u8  reset_mixer;
  u8  reset_disp;
} vout_reset_t;

// (cmd code 0x0000700a)
typedef struct vout_display_csc_setup_s {
  u32 cmd_code;
  u16 vout_id;
  u16 csc_type; // 0: digital; 1: analog; 2: hdmi
  u32 csc_parms[9];
} vout_display_csc_setup_t;

// (cmd code 0x0000700b)
typedef struct vout_digital_output_mode_s {
  u32 cmd_code;
  u16 vout_id;
  u16 reserved;
  u32 output_mode;
} vout_digital_output_mode_setup_t;

// (cmd code 0x0000700c)
typedef struct vout_gamma_setup_s {
  u32 cmd_code;
  u16 vout_id;
  u8  enable;
  u8  setup_gamma_table;
  u32 gamma_dram_addr;
} vout_gamma_setup_t;

/**
 * real time rate modify - vbr
 */
typedef struct amb_real_time_rate_modify_s
{
	u32 cmd_code;
	u32 real_time_rate_modify;
	u32 chan_id;
}amb_real_time_rate_modify_t;

typedef struct real_time_cbr_modify_s
{
	u32 cmd_code;
	u32 real_time_cbr_modify;
	u32 chan_id;
}real_time_cbr_modify_t;

/* 0x6002 */
typedef struct ipcam_capture_preview_size_setup_s
{
	u32 cmd_code;
    u32 preview_id : 2 ;
    u32 output_scan_format : 1 ;
    u32 deinterlace_mode : 2 ;
    u32 disabled : 1 ;
    u32 Reserved1 : 26 ;
    u16 cap_width ;
    u16 cap_height ;
    u16 input_win_offset_x ;
    u16 input_win_offset_y ;
    u16 input_win_width ;
    u16 input_win_height ;
} ipcam_capture_preview_size_setup_t ;

/* for capture_source in ipcam_video_encode_size_setup_t */
typedef enum {
	DSP_CAPBUF_MN = 0,
	DSP_CAPBUF_PA = 1,
	DSP_CAPBUF_PB = 2,
	DSP_CAPBUF_PC = 3,
	DSP_CAPBUF_WARP = 4,
	DSP_CAPBUF_EFM = 5,
	DSP_CAPBUF_TOTAL_NUM,
} DSP_CAPBUF_ID;

/* 0x6003 */
typedef struct ipcam_video_encode_size_setup_s
{
	u32 cmd_code;
	u32 capture_source : 3;
	u32 Reserved1 : 29;
	s16 enc_x;
	s16 enc_y;
	u16 enc_width;
	u16 enc_height;
} ipcam_video_encode_size_setup_t;

/* 0x6006 */
typedef struct ipcam_video_system_setup_s
{
	u32 cmd_code;
	u16 main_max_width ;
	u16 main_max_height ;
	u16 preview_A_max_width ;
	u16 preview_A_max_height ;
	u16 preview_B_max_width ;
	u16 preview_B_max_height ;
	u16 preview_C_max_width ;
	u16 preview_C_max_height ;
	u8 stream_0_max_GOP_M ;
	u8 stream_1_max_GOP_M ;
	u8 stream_2_max_GOP_M ;
	u8 stream_3_max_GOP_M ;
	u16 stream_0_max_width ;
	u16 stream_0_max_height ;
	u16 stream_1_max_width ;
	u16 stream_1_max_height ;
	u16 stream_2_max_width ;
	u16 stream_2_max_height ;
	u16 stream_3_max_width ;
	u16 stream_3_max_height ;
	u32 MCTF_possible : 1 ;
	u32 max_num_streams : 3 ;
	u32 max_num_cap_sources : 2 ;
	u32 use_1Gb_DRAM_config : 1 ;
	u32 raw_compression_disabled : 1 ;
	u32 vwarp_blk_h : 8;
	u32 extra_buf_main : 4;
	u32 extra_buf_preview_a : 4;
	u32 extra_buf_preview_b : 4;
	u32 extra_buf_preview_c : 4;
	u16 raw_max_width ;
	u16 raw_max_height ;
	u16 warped_main_max_width ;
	u16 warped_main_max_height ;
	u16 v_warped_main_max_width ;
	u16 v_warped_main_max_height ;
	u16 max_warp_region_input_width;
	u16 max_warp_region_output_width;
	u16 max_padding_width;
	u16 enc_rotation_possible : 1;
	u16 enc_dummy_latency : 7;
	u16 stream_0_LT_enable : 1;
	u16 stream_1_LT_enable : 1;
	u16 stream_2_LT_enable : 1;
	u16 stream_3_LT_enable : 1;
	u16 reserved1 : 4;
	u16 max_warp_region_input_height;
	u16 reserved2;
}ipcam_video_system_setup_t ;

/* 0x6004 */
typedef struct ipcam_real_time_encode_param_setup_s
{
	u32 cmd_code;
	u32 enable_flags ;
	u32 cbr_modify ;
	u32 custom_encoder_frame_rate;
	u8 frame_rate_division_factor ;
	u8 qp_min_on_I ;
	u8 qp_max_on_I ;
	u8 qp_min_on_P ;
	u8 qp_max_on_P ;
	u8 qp_min_on_B ;
	u8 qp_max_on_B ;
	u8 aqp ;
	u8 frame_rate_multiplication_factor ;
	u8 i_qp_reduce ;
	u8 skip_flags ;
	u8 M ;
	u8 N ;
	u8 p_qp_reduce ;
	u8 intra_refresh_num_mb_row ;
	u8 preview_A_frame_rate_divison_factor ;
	u32 idr_interval ;
	u32 custom_vin_frame_rate ;
	u32 roi_daddr;
	s8 roi_delta[NUM_PIC_TYPES][4]; /* 3 num pic types and 4 categories */
	u32 panic_div : 8 ;
	u32 is_monochrome : 1 ;
	u32 scene_change_detect_on : 1;
	u32 flat_area_improvement_on : 1;
	u32 drop_frame : 8;
	u32 reserved0 : 13 ;
	u32 pic_size_control;
	u32 quant_matrix_addr ;
	u16 P_IntraBiasAdd;
	u16 B_IntraBiasAdd;
	u8 zmv_threshold;
	u8 N_msb;
	u8 idsp_frame_rate_M; /* idsp_frame_rate_multiplication_factor */
	u8 idsp_frame_rate_N; /* idsp_frame_rate_division_factor */
	u32 roi_daddr_p;
	u32 roi_daddr_b;
	u8 user1_intra_bias;
	u8 user1_direct_bias;
	u8 user2_intra_bias;
	u8 user2_direct_bias;
	u8 user3_intra_bias;
	u8 user3_direct_bias;
	u8 reserved1[2];
} ipcam_real_time_encode_param_setup_t ;

/* 0x6007 */
typedef struct ipcam_osd_insert_s
{
	u32 cmd_code;
	u32 vout_id : 1 ;
	u32 osd_enable : 1 ;
	u32 osd_num_regions : 2 ;
	u32 osd_enable_ex : 1 ; /* if set, osd_enable must be set also */
	u32 osd_num_regions_ex : 2 ;
	u32 reserved1 : 25 ;
	u32 osd_clut_dram_address[3] ;
	u32 osd_buf_dram_address[3] ;
	u16 osd_buf_pitch[3] ;
	u16 osd_win_offset_x[3] ;
	u16 osd_win_offset_y[3] ;
	u16 osd_win_w[3] ;
	u16 osd_win_h[3] ;
	u16 osd_buf_pitch_ex[3] ;
	u16 osd_win_offset_x_ex[3] ;
	u16 osd_win_offset_y_ex[3] ;
	u16 osd_win_w_ex[3] ;
	u16 osd_win_h_ex[3] ;
	u32 osd_clut_dram_address_ex[3] ;
	u32 osd_buf_dram_address_ex[3] ;
} ipcam_osd_insert_t ;

/* 0x6008 */
typedef struct ipcam_set_privacy_mask_s
{
	u32 cmd_code;
	u32 enabled_flags_dram_address ;
	u8 Y ;
	u8 U ;
	u8 V ;
} ipcam_set_privacy_mask_t ;

/* 0x6009 */
typedef struct ipcam_fast_osd_insert_s
{
	u32 cmd_code;
	u32 vout_id : 1;
	u32 fast_osd_enable : 1;
	u32 string_num_region : 2;
	u32 osd_num_region : 2;
	u32 reserved1 : 26;

	/* For time string */
	u32 font_index_addr;
	u32 font_map_addr;
	u16 font_map_pitch;
	u16 font_map_h;
	u32 string_addr[2];
	u8  string_len[2];
	u16 reserved2;
	u32 string_output_addr[2];
	u16 string_output_pitch[2];
	u32 clut_addr[2];
	u16 string_win_offset_x[2];
	u16 string_win_offset_y[2];

	/* For logo picture */
	u32 osd_clut_addr[2];
	u32 osd_buf_addr[2];
	u16 osd_buf_pitch[2];
	u16 osd_win_offset_x[2];
	u16 osd_win_offset_y[2];
	u16 osd_win_w[2];
	u16 osd_win_h[2];
} ipcam_fast_osd_insert_t;


/* 0x600A */
typedef struct ipcam_set_region_warp_control_batch_s
{
	u32 cmd_code ;
	u32 num_regions_minus_1 : 4 ; /* in case we need to support 16 warp regions */
	u32 reserved : 28 ;
	u32 warp_region_ctrl_daddr ; /* batch commands for ipcam_set_region_warp_control_t */
} ipcam_set_region_warp_control_batch_t;

typedef struct
{
	u16 x ;
	u16 y ;
	u16 w ;
	u16 h ;
} WARP_RECT_t ;

typedef struct ipcam_set_region_warp_control_s
{
	WARP_RECT_t input_win ;
	WARP_RECT_t output_win ;

	u32 region_id : 4 ; /* 0..VCAP_MAX_NUM_WARP_REGION-1 */
	u32 reserved1 : 4 ;
	u32 rotate_flip_input : 3 ;
	u32 h_warp_enabled : 1 ;
	u32 h_warp_grid_w : 7 ; /* <= 32 */
	u32 h_warp_grid_h : 7 ; /* <= 48 */
	u32 h_warp_h_grid_spacing_exponent : 3 ; // 2^(4+grid_spacing) => 0:16 1:32 2:64 3:128 4:256 5:512
	u32 h_warp_v_grid_spacing_exponent : 3 ; // 2^(4+grid_spacing) => 0:16 1:32 2:64 3:128 4:256 5:512

	u32 v_warp_enabled : 1 ;
	u32 v_warp_grid_w : 7 ; /* <= 32 */
	u32 v_warp_grid_h : 7 ; /* <= 48 */
	u32 v_warp_h_grid_spacing_exponent : 3 ; // 2^(4+grid_spacing) => 0:16 1:32 2:64 3:128 4:256 5:512
	u32 v_warp_v_grid_spacing_exponent : 3 ; // 2^(4+grid_spacing) => 0:16 1:32 2:64 3:128 4:256 5:512
	u32 reserved2 : 11 ;

	u32 h_warp_grid_addr ;
	u32 v_warp_grid_addr ;

	WARP_RECT_t prev_a_input_win ;
	WARP_RECT_t prev_a_output_win ;
	WARP_RECT_t prev_c_input_win ;
	WARP_RECT_t prev_c_output_win ;
	WARP_RECT_t intermediate_win ;

	u32 me1_vwarp_enabled : 1;
	u32 me1_vwarp_grid_w : 7;	/* <= 32 */
	u32 me1_vwarp_grid_h : 7;	/* <= 48 */
	u32 me1_vwarp_h_grid_spacing_exponent : 3; // 2^(4+grid_spacing) => 0:16 1:32 2:64 3:128 4:256 5:512
	u32 me1_vwarp_v_grid_spacing_exponent : 3; // 2^(4+grid_spacing) => 0:16 1:32 2:64 3:128 4:256 5:512
	u32 reserved3 : 11;

	u32 me1_vwarp_grid_addr;
} ipcam_set_region_warp_control_t;

/* 0x600B */
/* Up to 4 exposures */
typedef struct ipcam_set_hdr_proc_control_s
{
	u32 cmd_code ;
	u32 blend_0_updated : 1 ; /* exp 0 + 1 */
	u32 blend_1_updated : 1 ; /* exp 0 + 1 + 2 */
	u32 blend_2_updated : 1 ; /* exp 0 + 1 + 2 + 3 */
	u32 exp_0_start_offset_updated : 1 ;
	u32 exp_1_start_offset_updated : 1 ;
	u32 exp_2_start_offset_updated : 1 ;
	u32 exp_3_start_offset_updated : 1 ;
	u32 low_pass_filter_radius : 2;	/* 0: 16x, 1: 32x, 2: 64x */
	u32 contrast_enhance_gain : 12;
	u32 reserved : 11;
	u32 preblend_cmds_daddr[3] ; /* batch commands for preblend */
	u32 blend_cmds_daddr[3]; /* batch commands for blending */
	u32 raw_daddr_start_offset_y[4] ;
} ipcam_set_hdr_proc_control_t ;

/* 0x600D */
typedef enum
{
	FRM_BUF_POOL_TYPE_RAW = 0,
	FRM_BUF_POOL_TYPE_MAIN_CAPTURE = 1,
	FRM_BUF_POOL_TYPE_PREVIEW_A = 2,
	FRM_BUF_POOL_TYPE_PREVIEW_B = 3,
	FRM_BUF_POOL_TYPE_PREVIEW_C = 4,
	FRM_BUF_POOL_TYPE_WARPED_MAIN_CAPTURE = 5, /* post-main, for dewarp pipeline only */
	FRM_BUF_POOL_TYPE_WARPED_INT_MAIN_CAPTURE = 6, /* intermediate-main, for dewarp pipeline only */
	FRM_BUF_POOL_TYPE_EFM = 7,
	FRM_BUF_POOL_TYPE_NUM = 8,
} FRM_BUF_POOL_TYPE ;

typedef struct ipcam_get_frame_buf_pool_info_s
{
	u32 cmd_code ;
	u32 frm_buf_pool_info_req_bit_flag;
} ipcam_get_frame_buf_pool_info_t;

typedef enum
{
	EFM_BUF_POOL_TYPE_ME1 = 0,
	EFM_BUF_POOL_TYPE_YUV420 = 1,
	EFM_BUF_POOL_TYPE_NUM = 2,
} EFM_BUF_POOL_TYPE ;

/* 0x600E */
typedef struct
{
	u32 cmd_code ;
	u32 buf_width : 16;
	u32 buf_height : 16;
	u32 fbp_type : 8; 		// bit0 -- ME1; bit1 -- YUV420
	u32 max_num_yuv : 8;
	u32 max_num_me1 : 8;
	u32 reserved : 8;
} ipcam_efm_creat_frm_buf_pool_cmd_t;

/* 0x600F */
typedef struct
{
	u32 cmd_code ;
	u32 fbp_type : 8; 		// bit0 -- ME1; bit1 -- YUV420
	u32 reserved : 24;
} ipcam_efm_req_frm_buf_cmd_t;

/* 0x6010 */
typedef struct
{
	u32 cmd_code ;
	u32 yuv_fId : 16;
	u32 me1_fId : 16;
	u32 pts;
} ipcam_efm_handshake_cmd_t;

/* Adding Macros defined in the interface between ARM and VDSP */

//following three configuration is predefined inside the coding module.
//and shared between the ARM and DSP.
//Used to describe the layout of the Coding CTX information in the DRAM sent from
//ARM to DSP.
#define CTX_WIDTH   		(416)
#define I_CTX_SZ		(52*CTX_WIDTH)
#define FLD_CTX_OFS 		(104*CTX_WIDTH)
#define CABAC_CTX_MEM_SIZE	(CTX_WIDTH*(104*2))

#define JPG_LU_HUFF_DRAM_SIZE	(256)
#define JPG_CH_HUFF_DRAM_SIZE	(256)
#define MCTF_CFG_SZ 		(22528)

typedef struct bit_stream_hdr_s
{
	u32    frmNo;
	u32    pts;
	u32    start_addr;
	u32    frameTy     : 3;
	u32    levelIDC    : 3;
	u32    refIDC      : 1;
	u32    picStruct   : 1;
	u32    length      :24;
	u32    streamID	: 8;
	u32    sessionID	: 4;
	u32    res_1		:20;
	u32    pjpg_start_addr;
	u32    pjpg_size;
	u32    hw_pts;
} bit_stream_hdr_t ;

#endif /* __CMD__MSG_H__ */

