/*******************************************************************************
 * dsp_platform_interface.h
 *
 * History:
 * 2012/12/07 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
 ******************************************************************************/

#ifndef __DSP_PLATFORM_INTERFACE_H__
#define __DSP_PLATFORM_INTERFACE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

typedef enum {
    EDSPPlatform_Invalid = 0,
    EDSPPlatform_AutoSelect = 0x001,

    EDSPPlatform_SimpleSW = 0x100,
    EDSPPlatform_CPUARMv7AccelSIMD,
    EDSPPlatform_CPUX86AccelSIMD,
    EDSPPlatform_GPUAccelNv,
    EDSPPlatform_GPUAccelAMD,
    EDSPPlatform_GPUAccelImg,

    EDSPPlatform_AmbaA5s = 0x200,
    EDSPPlatform_AmbaI1,
    EDSPPlatform_AmbaS2,
} EDSPPlatform;

typedef enum {
    EDSPTrickPlay_Invalid = 0,
    EDSPTrickPlay_Pause,
    EDSPTrickPlay_Resume,
    EDSPTrickPlay_Step,
    EDSPTrickPlay_FastForward,
    EDSPTrickPlay_FastBackward,
} EDSPTrickPlay;

typedef struct {
    TU8 udec_id;
    TU8 trickplay_type;

    TU8 reserved0;
    TU8 reserved1;
} SDSPTrickPlay;

//-----------------------------------------------------------------------
//
//  from dsp/udec
//
//-----------------------------------------------------------------------

enum {
    EH_DecoderType_Common = 0,
    EH_DecoderType_H264,
    EH_DecoderType_MPEG12,
    EH_DecoderType_MPEG4_HW,
    EH_DecoderType_MPEG4_Hybird,
    EH_DecoderType_VC1,
    EH_DecoderType_RV40_Hybird,
    EH_DecoderType_JPEG,
    EH_DecoderType_SW,
    EH_DecoderType_Cnt,
};

//error code's level
enum {
    EH_ErrorLevel_NoError = 0,
    EH_ErrorLevel_Warning,
    EH_ErrorLevel_Recoverable,
    EH_ErrorLevel_Fatal,
    EH_ErrorLevel_Last,
};

//error code's error type, decoder type==common
enum {
    EH_CommonErrorType_None = 0,
    EH_CommonErrorType_HardwareHang = 1,
    EH_CommonErrorType_FrameBufferLeak,
    EH_CommonErrorType_Last,
};

typedef struct _UDECErrorCodeDetail {
    TU16 error_type;
    TU8 error_level;
    TU8 decoder_type;
} UDECErrorCodeDetail;

typedef union _UDECErrorCode {
    UDECErrorCodeDetail detail;
    TU32 mu32;
} UDECErrorCode;

//mw's behavior
enum {
    MW_Bahavior_Ignore = 0,
    MW_Bahavior_Ignore_AndPostAppMsg = 1,
    MW_Bahavior_TrySeekToSkipError,
    MW_Bahavior_ExitPlayback,
    MW_Bahavior_HaltForDebug,//debug mode, do not send stop cmd to dsp, just make the env un-touched, for debug(ucode)
};

extern const char GUdecStringDecoderType[EH_DecoderType_Cnt][20];
extern const char GUdecStringErrorLevel[EH_ErrorLevel_Last][20];
extern const char GUdecStringCommonErrorType[EH_CommonErrorType_Last][20];

typedef struct {
    TU8 max_window_number;//should not be changed
    TU8 cur_window_number;
    TU8 cur_render_number;
    TU8 cur_decoder_number;

    TU8 display_mask;//should not be changed
    TU8 primary_display_index;//should not be changed
    TU8 cur_display_mask;
    TU8 mb_update_display_device;

    TU8 mb_update_window;
    TU8 mb_update_render;
    TU8 reserved0, reserved1;

    TUint display_width;
    TUint display_height;

    SDSPMUdecWindow *p_window;
    SDSPMUdecRender *p_render;
    SDSPUdecInstanceConfig *p_decoder;
} SVideoMWConfiguration;

typedef struct {
    StreamFormat codec_type;
    EDSPOperationMode dsp_mode;
    TUint dsp_version;

    TUint max_pic_width, max_pic_height;
    TUint pic_width, pic_height;

    //out
    TU8 *bits_fifo_start;
    TUint bits_fifo_size;
} SDecoderParam;

typedef struct {
    TUint codec_type;
    TUint dsp_mode, dsp_version;
    TUint main_width, main_height;
    TUint enc_width, enc_height;
    TUint enc_offset_x, enc_offset_y;

    TU8 profile, level;
    TU8 M, N;
    TU8 gop_structure;
    TU8 numRef_P;
    TU8 numRef_B;
    TU8 use_cabac;
    TUint idr_interval;
    TUint bitrate;
    TUint framerate_num, framerate_den;

    TU16 quality_level;
    TU8 vbr_setting;
    TU8 calibration;
    TU8 vbr_ness;
    TU8 min_vbr_rate_factor;
    TU8 max_vbr_rate_factor;

    TU8 second_stream_enabled;
    TU8 dsp_piv_enabled;
    TU8 reserved1[2];
    TUint second_bitrate;
    TUint second_enc_width, second_enc_height;
    TUint second_enc_offset_x, second_enc_offset_y;

    TU16 dsp_jpeg_active_win_w, dsp_jpeg_active_win_h;
    TU16 dsp_jpeg_dram_w, dsp_jpeg_dram_h;

    //out
    TU8 *bits_fifo_start;
    TUint bits_fifo_size;
} SEncoderParam;

typedef struct {
    //TUint codec_type;
    //TUint dsp_mode, dsp_version;
    //TUint main_width, main_height;
    TUint enc_width, enc_height;
    //TUint enc_offset_x, enc_offset_y;
    //TUint second_stream_enabled;
    TUint second_enc_width, second_enc_height;
    //TUint second_enc_offset_x, second_enc_offset_y;
} SPrepareEncodingParam;

typedef struct {
    TU8 *pstart;
    TUint size;
    TU64 pts;

    TU8 pic_type;
    //need here?
    TU8 enc_id;
    TU16 fb_id;

    TUint frame_number;
} SBitDesc;

//same with driver
#define DMaxDescNumber 8
#define INVALID_FD_ID 0xffff
#define DFlagLastFrame (1<<0)
#define DFlagNeedReleaseFrame (1<<1)
typedef struct {
    TU16 tot_desc_number;
    TU16 status;
    SBitDesc desc[DMaxDescNumber];
} SBitDescs;

#if 0
typedef struct {
    TInt enable;//video layer
    TInt osd_disable;//osd layer
    TInt sink_id;
    TInt width, height;//the size of device
    TInt pos_x, pos_y;
    TInt size_x, size_y;//target_win_size
    TU32 zoom_factor_x, zoom_factor_y;
    TInt input_center_x, input_center_y;
    TInt flip, rotate;
    TInt failed;//checking/opening failed
    TInt vout_mode;// 1: interlace_mode

    TInt vout_id;//debug use
} SVoutConfig;
#endif

typedef struct {
    TU8 module_type;
    TU8 error_level;
    TU16 error_type;

    TU32 error_code;

    TTime last_pts;

    TU8 dec_state;
    TU8 vout_state;
    TU8 reserved0;
    TU8 reserved1;

    TU32 free_zoom;
    TU32 fullness;
} SDecStatus;

class IDSPDecAPI
{
public:
    virtual EECode InitDecoder(TUint &dec_id, SDecoderParam *param, TUint vout_start_index, TUint number_vout, const volatile SDSPVoutConfig *vout_config) = 0;
    virtual EECode ReleaseDecoder(TUint dec_id) = 0;

    virtual EECode RequestBitStreamBuffer(TUint dec_id, TU8 *pstart, TUint room) = 0;
    virtual EECode Decode(TUint dec_id, TU8 *pstart, TU8 *pend, TU32 number_of_frames = 1) = 0;
    virtual EECode Stop(TUint dec_id, TUint stop_flag) = 0;

    virtual EECode PbSpeed(TUint dec_id, TU8 direction, TU8 feeding_rule, TU16 speed) = 0;

    virtual EECode QueryStatus(TUint dec_id, SDecStatus *status) = 0;

    virtual EECode TuningPB(TUint dec_id, TU8 fw, TU16 frame_tick) = 0;

    virtual ~IDSPDecAPI() {}
};

class IDSPEncAPI
{
public:
    virtual EECode InitEncoder(TUint &enc_id, SEncoderParam *param) = 0;
    virtual EECode ReleaseEncoder(TUint enc_id) = 0;

    virtual EECode GetBitStreamBuffer(TUint enc_id, SBitDescs *p_desc) = 0;
    virtual EECode Start(TUint enc_id) = 0;
    virtual EECode Stop(TUint enc_id, TUint stop_flag) = 0;

    virtual ~IDSPEncAPI() {}
};

typedef enum {
    //MUDEC related
    EDSPControlType_UDEC_pb_still_capture = 0,
    EDSPControlType_UDEC_trickplay,
    EDSPControlType_UDEC_zoom,
    EDSPControlType_UDEC_stream_switch,
    EDSPControlType_UDEC_render_config,
    EDSPControlType_UDEC_window_config,
    EDSPControlType_UDEC_one_window_config,
    EDSPControlType_UDEC_wait_switch_done,
    EDSPControlType_UDEC_update_multiple_window_display,

    //UDEC related
    EDSPControlType_UDEC_wake_vout,
    EDSPControlType_UDEC_wait_vout_dormant,
    EDSPControlType_UDEC_get_status,
    EDSPControlType_UDEC_clear,

    //DUPLEX related

    //ENCODE related
    EDSPControlType_UDEC_TRANSCODER_update_params,
    EDSPControlType_UDEC_TRANSCODER_update_bitrate,
    EDSPControlType_UDEC_TRANSCODER_update_framerate,
    EDSPControlType_UDEC_TRANSCODER_update_gop,
    EDSPControlType_UDEC_TRANSCODER_demand_IDR,

    //TRANSCODE related
} EDSPControlType;

typedef struct {
    TU32 u32_param[4];
    TU16 u16_param[4];
    TU8 u8_param[4];
    void *p_param0[4];
} SDSPControlParams;

typedef struct {
    TU32 decoder_id;
    TU32 error_code;
    TU64 last_pts;
    TU32 dec_state;
    TU32 vout_state;

    //usr space
    TU8 *dsp_current_read_bitsfifo_addr;
    TU8 *arm_last_write_bitsfifo_addr;

    //physical addr
    TU8 *dsp_current_read_bitsfifo_addr_phys;
    TU8 *arm_last_write_bitsfifo_addr_phys;

    //bit-stream fifo info
    TU32 bits_fifo_total_size;
    TU32 bits_fifo_free_size;
    TU32 bits_fifo_phys_start;

    //decode cmd count
    TU32 tot_decode_cmd_cnt;
    TU32 tot_decode_frame_cnt;
} SDSPControlStatus;

typedef struct {
    TU32 decoder_id;
    TU32 flag;
} SDSPControlClear;

class IDSPAPI
{
public:
    virtual EECode OpenDevice(TInt &fd) = 0;
    virtual EECode CloseDevice() = 0;

    virtual EECode QueryVoutSettings(volatile SDSPConfig *config) const = 0;
    virtual EECode DSPModeConfig(const volatile SPersistMediaConfig *config) = 0;
    virtual EECode EnterDSPMode(TUint mode) = 0;

    virtual EECode DSPControl(EDSPControlType type, void *p_params) = 0;

    virtual EDSPOperationMode QueryCurrentDSPMode() const = 0;
    virtual EDSPPlatform QueryDSPPlatform() const = 0;

    virtual ~IDSPAPI() {}
};

extern IDSPAPI *gfDSPAPIFactory(const volatile SPersistMediaConfig *p_config, EDSPPlatform dsp_platform = EDSPPlatform_AutoSelect);
extern IDSPDecAPI *gfDSPDecAPIFactory(TInt iav_fd, EDSPOperationMode dsp_mode, StreamFormat codec_type, const volatile SPersistMediaConfig *p_config, EDSPPlatform dsp_platform = EDSPPlatform_AutoSelect);
extern IDSPEncAPI *gfDSPEncAPIFactory(TInt iav_fd, EDSPOperationMode dsp_mode, StreamFormat codec_type, const volatile SPersistMediaConfig *p_config, EDSPPlatform dsp_platform = EDSPPlatform_AutoSelect);

//utils function
#define DUDEC_SEQ_HEADER_LENGTH 20
#define DUDEC_SEQ_HEADER_EX_LENGTH 24
#define DUDEC_PES_HEADER_LENGTH 24

#define DUDEC_SEQ_STARTCODE_H264 0x7D
#define DUDEC_SEQ_STARTCODE_MPEG4 0xC4
#define DUDEC_SEQ_STARTCODE_VC1WMV3 0x71

#define DUDEC_PES_STARTCODE_H264 0x7B
#define DUDEC_PES_STARTCODE_MPEG4 0xC5
#define DUDEC_PES_STARTCODE_VC1WMV3 0x72
#define DUDEC_PES_STARTCODE_MPEG12 0xE0

#define DPRIVATE_GOP_NAL_HEADER_LENGTH 22

const TChar *gfGetDSPPlatformString(EDSPPlatform platform);

//little endian
TUint gfFillUSEQHeader(TU8 *pHeader, StreamFormat vFormat, TU32 timeScale, TU32 tickNum, TU32 is_mp4s_flag, TS32 vid_container_width, TS32 vid_container_height);
void gfInitUPESHeader(TU8 *pHeader, StreamFormat vFormat);
TUint gfFillPESHeader(TU8 *pHeader, TU32 ptsLow, TU32 ptsHigh, TU32 auDatalen, TUint hasPTS, TUint isPTSJump);
TUint gfFillPrivateGopNalHeader(TU8 *pHeader, TU32 timeScale, TU32 tickNum, TUint m, TUint n, TU32 pts_high, TU32 pts_low);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

