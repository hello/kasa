/*******************************************************************************
 * am_audio_codec_g726_config.h
 *
 * History:
 *   2014-11-12 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_CODEC_G726_CONFIG_H_
#define AM_AUDIO_CODEC_G726_CONFIG_H_

#include "am_audio_define.h"

enum G726_RATE
{
  G726_16K = 16,
  G726_24K = 24,
  G726_32K = 32,
  G726_40K = 40,
};

struct AudioCodecG726Config
{
    AM_AUDIO_SAMPLE_FORMAT decode_law;
    G726_RATE              encode_rate;
    uint32_t               encode_frame_time_length;
    AudioCodecG726Config() :
      decode_law(AM_SAMPLE_S16LE),
      encode_rate(G726_32K),
      encode_frame_time_length(80)
    {}
};

class AMConfig;
class AMAudioCodecG726Config
{
  public:
    AMAudioCodecG726Config();
    virtual ~AMAudioCodecG726Config();
    AudioCodecG726Config* get_config(const std::string& conf_file);

  private:
    AMConfig             *m_config;
    AudioCodecG726Config *m_g726_config;
};

#endif /* AM_AUDIO_CODEC_G726_CONFIG_H_ */
