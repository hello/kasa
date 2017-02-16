/*******************************************************************************
 * am_audio_codec_speex_config.h
 *
 * History:
 *   Jul 24, 2015 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_CODEC_SPEEX_CONFIG_H_
#define AM_AUDIO_CODEC_SPEEX_CONFIG_H_

struct SpeexEncode
{
    uint32_t sample_rate;
    uint32_t enc_frame_time_length;
    uint32_t spx_complexity;
    uint32_t spx_avg_bitrate;
    uint32_t spx_vbr_max_bitrate;
    uint32_t spx_quality;
    bool     spx_vbr;
    bool     spx_vad;
    bool     spx_dtx;
    bool     spx_highpass;
    SpeexEncode() :
      sample_rate(16000),
      enc_frame_time_length(40),
      spx_complexity(1),
      spx_avg_bitrate(32000),
      spx_vbr_max_bitrate(48000),
      spx_quality(1),
      spx_vbr(true),
      spx_vad(true),
      spx_dtx(false),
      spx_highpass(false)
    {}
};

struct SpeexDecode
{
    uint32_t    spx_output_channel;
    bool        spx_perceptual_enhance;
};

struct AudioCodecSpeexConfig
{
    SpeexEncode encode;
    SpeexDecode decode;
};

class AMConfig;
class AMAudioCodecSpeex;
class AMAudioCodecSpeexConfig
{
    friend class AMAudioCodecSpeex;
  public:
    AMAudioCodecSpeexConfig();
    virtual ~AMAudioCodecSpeexConfig();
    AudioCodecSpeexConfig* get_config(const std::string& config);

  private:
    AMConfig              *m_config;
    AudioCodecSpeexConfig *m_speex_config;
};

#endif /* AM_AUDIO_CODEC_SPEEX_CONFIG_H_ */
