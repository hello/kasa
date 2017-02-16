
#ifndef AAC_AUDIO_ENC_H
#define AAC_AUDIO_ENC_H

#include "basetypes.h"

typedef enum {
  ENCODE_OK,                      /*!< No error */
  ENCODE_INVALID_POINTER,         /*!< Pointer invalid */
  ENCODE_FAILED,                  /*!< Encoding failed */
  ENCODE_UNSUPPORTED_SAMPLE_RATE, /*!< Given samplerate not supported */
  ENCODE_UNSUPPORTED_CH_CFG,      /*!< Given channel configuration not supported */
  ENCODE_UNSUPPORTED_BIT_RATE,    /*!< Given bitrate not supported */
  ENCODE_UNSUPPORTED_MODE         /*!< Given mode not supported */
} AACPLUS_ERROR;

#ifndef AAC_AUDIO_DEC_H
typedef struct audio_lib_info_s {
    u8  svnrev[8];
    u8  svnhttp[128];
} audio_lib_info_t;

typedef enum {
  AACPLAIN,           /* aac encoding low complexity */
  AACPLUS,            /* aacPlus encoding */
  AACPLUS_SPEECH,     /* Not support */
  AACPLUS_PS,         /* aacPlus encoding with parametric stereo */
  AACPLUS_SPEECH_PS,  /* Not support */
} AAC_MODE;
#endif

typedef struct au_aacenc_config_s {
    u32 sample_freq;
    u32 coreSampleRate; /* != samplerate for aac+ */
    s32 Src_numCh;
    s32 Out_numCh;
    u32 bitRate;
    u8  ena_adjbr;
    u32 adj_bitRate;
    u32 quantizerQuality;
    u32 tns;    /* default is tns on */
    s8  ffType; /* LOAS */
    s32 enc_mode;
    u32 frameCounter;
    u32 ErrorStatus;
    u32 nBitsInRawDataBlock;
    u8  *enc_wptr;
    s32 *enc_rptr;
    u32 *codec_lib_mem_adr;
    audio_lib_info_t lib_info;
    /* thread safe global variables */
    void *hEnc;
    void *bufSize;
    void *bsDesc;
    void *hFileformat;
    void *calib_config;
    u32 cnt_for_bitrate;
    u32 bitrate_period;
    s32 initheader;
    s32 maxShift;

    u32 calibre_proc_on;   //1: calib on, 0: calib off
    u32 calibre_proc_mode; //for different resolution
    u32 calibre_apply;     //under calibre_proc_on=0, 1: apply the calib curve
    //to current encoding, 0: do nothing
    s8 *calibre_curve_addr;
} au_aacenc_config_t;

#ifdef __cplusplus
extern "C" {
#endif
void AacEncClose();
void aacenc_setup(au_aacenc_config_t *pau_aacenc_config);
void aacenc_open(au_aacenc_config_t *pau_aacenc_config);
void aacenc_encode(au_aacenc_config_t *pau_aacenc_config);
#define aacenc_close AacEncClose

#ifdef __cplusplus
}
#endif

#endif //AAC_AUDIO_ENC_H
