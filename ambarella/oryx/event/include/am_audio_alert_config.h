/*
 * am_audio_alert_config.h
 *
 * Histroy:
  *  2014-11-19 [Dongge Wu] create file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef AM_AUDIO_ALERT_CONFIG_H
#define AM_AUDIO_ALERT_CONFIG_H

#include "am_configure.h"
#include "am_audio_define.h"

struct AudioAlertConfig
{
    bool enable_alert_detect;
    int32_t channel_num;
    int32_t sample_rate;
    int32_t chunk_bytes;
    AM_AUDIO_SAMPLE_FORMAT sample_format;
    int32_t sensitivity;
    int32_t threshold;
    int32_t direction;
    AudioAlertConfig() :
        enable_alert_detect(false),
        channel_num(2),
        sample_rate(48000),
        chunk_bytes(2048),
        sample_format(AM_SAMPLE_S16LE),
        sensitivity(50),
        threshold(50),
        direction(0)
    {
    }
};

class AMConfig;
class AMAudioAlertConfig
{
  public:
    AMAudioAlertConfig();
    virtual ~AMAudioAlertConfig();
    AudioAlertConfig* get_config(const std::string& config);
    bool set_config(AudioAlertConfig *alert_config, const std::string& config);

  private:
    AMConfig            *m_config;
    AudioAlertConfig *m_alert_config;
};

#endif
