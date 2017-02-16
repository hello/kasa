/*******************************************************************************
 * am_audio_codec_g711_config.h
 *
 * History:
 *   2015-1-28 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_COMMON_CODECS_CODEC_G711_AM_AUDIO_CODEC_G711_CONFIG_H_
#define ORYX_STREAM_COMMON_CODECS_CODEC_G711_AM_AUDIO_CODEC_G711_CONFIG_H_

#include "am_audio_define.h"

struct AudioCodecG711Config
{
    AM_AUDIO_SAMPLE_FORMAT encode_law;
    uint32_t               encode_rate;
    uint32_t               encode_frame_time_length;
    AudioCodecG711Config() :
      encode_law(AM_SAMPLE_ALAW),
      encode_rate(16000),
      encode_frame_time_length(80)
    {}
};

class AMConfig;
class AMAudioCodecG711Config
{
  public:
    AMAudioCodecG711Config();
    virtual ~AMAudioCodecG711Config();
    AudioCodecG711Config* get_config(const std::string& conf_file);

  private:
    AMConfig             *m_config;
    AudioCodecG711Config *m_g711_config;
};

#endif /* ORYX_STREAM_COMMON_CODECS_CODEC_G711_AM_AUDIO_CODEC_G711_CONFIG_H_ */
