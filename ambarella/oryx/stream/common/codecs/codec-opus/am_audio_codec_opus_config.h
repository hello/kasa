/*******************************************************************************
 * am_audio_codec_opus_config.h
 *
 * History:
 *   2014-11-10 - [ccjing] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_CODEC_OPUS_CONFIG_H_
#define AM_AUDIO_CODEC_OPUS_CONFIG_H_

struct OpusEncode
{
    uint32_t opus_complexity;
    uint32_t opus_avg_bitrate;
    uint32_t sample_rate;
    uint32_t enc_output_size;
    uint32_t enc_frame_time_length;
    uint32_t output_channel;
    OpusEncode() :
        opus_complexity(1),
        opus_avg_bitrate(48000),
        sample_rate(48000),
        enc_output_size(4096),
        enc_frame_time_length(20),
        output_channel(2)
    {
    }
};

struct OpusDecode
{
    uint32_t dec_output_max_time_length;
    uint32_t dec_output_sample_rate;
    uint32_t dec_output_channel;
    OpusDecode() :
        dec_output_max_time_length(120),
        dec_output_sample_rate(48000),
        dec_output_channel(2)
    {
    }
};

struct AudioCodecOpusConfig
{
    OpusEncode encode;
    OpusDecode decode;
};

class AMConfig;
class AMAudioCodecOpus;
class AMAudioCodecOpusConfig
{
    friend class AMAudioCodecOpus;
  public:
    AMAudioCodecOpusConfig();
    virtual ~AMAudioCodecOpusConfig();
    AudioCodecOpusConfig* get_config(const std::string& config);

  private:
    AMConfig *m_config;
    AudioCodecOpusConfig *m_opus_config;
};

#endif /* AM_AUDIO_CODEC_OPUS_CONFIG_H_ */
