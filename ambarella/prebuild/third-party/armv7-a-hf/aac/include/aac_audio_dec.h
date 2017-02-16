
#ifndef AAC_AUDIO_DEC_H
#define AAC_AUDIO_DEC_H

#include "basetypes.h"

#ifndef AAC_AUDIO_ENC_H
typedef struct audio_lib_info_s {
    u8  svnrev[8];
    u8  svnhttp[128];
} audio_lib_info_t;

typedef enum {
  AACPLAIN,         /* aac encoding low complexity */
  AACPLUS,          /* aacPlus encoding */
  AACPLUS_SPEECH,   /* Not support */
  AACPLUS_PS,       /* aacPlus encoding with parametric stereo */
  AACPLUS_SPEECH_PS /* Not support */
} AAC_MODE;
#endif

enum audio_fs {
  AUDIO_FS_96000 = 96000,
  AUDIO_FS_88200 = 88200,
  AUDIO_FS_64000 = 64000,
  AUDIO_FS_48000 = 48000,
  AUDIO_FS_44100 = 44100,
  AUDIO_FS_32000 = 32000,
  AUDIO_FS_24000 = 24000,
  AUDIO_FS_22050 = 22050,
  AUDIO_FS_16000 = 16000,
  AUDIO_FS_12000 = 12000,
  AUDIO_FS_11025 = 11025,
  AUDIO_FS_8000 = 8000
};

enum {
  MAIN_OK = 0x0,
  MAIN_UCI_OUT_OF_MEMORY = 0x100,
  MAIN_UCI_HELP_MODE_ACTIV,
  MAIN_OPEN_BITSTREAM_FILE_FAILED,
  MAIN_OPEN_16_BIT_PCM_FILE_FAILED,
  MAIN_FRAME_COUNTER_REACHED_STOP_FRAME,
  MAIN_TERMINATED_BY_ESC
};

typedef enum {
  AAC_DEC_OK = 0x0,
  AAC_DEC_ADTS_SYNC_ERROR = 0x1000,
  AAC_DEC_LOAS_SYNC_ERROR,
  AAC_DEC_ADTS_SYNCWORD_ERROR,
  AAC_DEC_LOAS_SYNCWORD_ERROR,
  AAC_DEC_ADIF_SYNCWORD_ERROR,
  AAC_DEC_UNSUPPORTED_FORMAT,
  AAC_DEC_DECODE_FRAME_ERROR,
  AAC_DEC_CRC_CHECK_ERROR,
  AAC_DEC_INVALID_CODE_BOOK,
  AAC_DEC_UNSUPPORTED_WINOW_SHAPE,
  AAC_DEC_PREDICTION_NOT_SUPPORTED_IN_LC_AAC,
  AAC_DEC_UNIMPLEMENTED_CCE,
  AAC_DEC_UNIMPLEMENTED_GAIN_CONTROL_DATA,
  AAC_DEC_UNIMPLEMENTED_EP_SPECIFIC_CONFIG_PARSE,
  AAC_DEC_UNIMPLEMENTED_CELP_SPECIFIC_CONFIG_PARSE,
  AAC_DEC_UNIMPLEMENTED_HVXC_SPECIFIC_CONFIG_PARSE,
  AAC_DEC_OVERWRITE_BITS_IN_INPUT_BUFFER,
  AAC_DEC_CANNOT_REACH_BUFFER_FULLNESS,
  AAC_DEC_TNS_RANGE_ERROR,
  AAC_DEC_NEED_MORE_DATA,
  AAC_DEC_INSUFFICIENT_BACKUP_MEMORY
} AAC_DEC_Status;

typedef struct au_aacdec_config_s {
    u32 sample_freq;        // sample frequency
    s8  syncState;          // sync status
    s8  channelMode;        // channel mode
    s32 bsFormat;           // bit stream format (adif, adts, loas)
    s32 streamType;         // stream type
    s32 objType;            // audio object type
    s32 profile;            // bit stream profile
    s32 useHqSbr;           // high quality SBR
    s32 decMode;            // decode mode (AAC, AAC+, AAC++)
    s32 frameSize;          // frame size
    s32 srcNumCh;           // input channel number
    s32 outNumCh;           // output channel number
    s32 Out_res;            // output PCM resolution, 16 bit or 32 bit
    s32 averageBitrate;           // average bit rate
    s32 numberOfRawDataBlocks;    // number of raw data blocks in frames
    s32 bBitstreamDownMix;        // stereo to mono down mix flag
    s32 externalBitrate;          // external bit rate for loading the input buffer
    s32 externalSamplingRate;     // external sampling rate for raw decoding
    s32 ErrorStatus;              // error status of decode process
    s32 fs_out;                   // sample frequency of bit stream
    s32 fs_core;                  // core sample frequency for AAC+ bit stream
    s32 frameCounter;             // frame counter of decode
    s32 errorCounter;             // error counter of decode
    s8  bDownSample;              // down sampled SBR flag
    u32 consumedByte;             // consumed byte of decoding one frame
    u32 interBufSize;             // internal buffer size
    s32 *dec_wptr;                // write pointer of output data
    u8  *dec_rptr;                // read pointer of input bit stream
    u8  has_dec_out;              // decode output flag
    u32 *codec_lib_mem_addr;      // address of library working memory
    u8  need_more;                //
    audio_lib_info_t lib_info;    // revision info of audio library
    void *memory_map_ptr;         //pointer of internal memory map
} au_aacdec_config_t;


#define MPEG4_ADDITION_BYTES  (4+3)   /*!< 4 bytes loas header bits + 3 bytes bitstream routines */
#define SYNCH_BUF_SIZE_LOAS (8192+MPEG4_ADDITION_BYTES) /*!< Size of Input buffer */

#define ADTS_ADDITION_BYTES (7+3)     /*!< 7 bytes adts header bits + 3 bytes bitstream routines */
#define SYNCH_BUF_SIZE_ADTS (1536+ADTS_ADDITION_BYTES)	/*!< Size of Input buffer */

#define fract long
#define dfract long

enum {
  ByteSize      = 8,
  ShortSize     = ByteSize*sizeof(short),
  LongSize      = ByteSize*sizeof(long),
  Log2ByteSize  = 2+sizeof(char)
};

#define SAMPLES_PER_FRAME	(4096)

typedef enum {
  RAW_BSFORMAT = 0x0,
  ADIF_BSFORMAT,
  ADTS_BSFORMAT,
  LOAS_BSFORMAT
} AAC_DEC_BSFORMAT;

#ifdef __cplusplus
extern "C" {
#endif

void aacdec_decode(au_aacdec_config_t *);
void aacdec_setup(au_aacdec_config_t *);
void aacdec_set_bitstream_rp(au_aacdec_config_t *);
void aacdec_open(au_aacdec_config_t *);
void aacdec_close();
#ifdef __cplusplus
}
#endif

#endif //AAC_AUDIO_DEC_H
