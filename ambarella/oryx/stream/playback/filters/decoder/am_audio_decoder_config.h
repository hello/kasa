/*******************************************************************************
 * am_audio_decoder_config.h
 *
 * History:
 *   2014-9-24 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_DECODER_CONFIG_H_
#define AM_AUDIO_DECODER_CONFIG_H_

#include "am_filter_config.h"
struct AudioDecoderConfig: public AMFilterConfig
{
    uint32_t decode_error_threshold;
    AudioDecoderConfig() :
      decode_error_threshold(30)
    {}
};

class AMConfig;
class AMAudioDecoder;
class AMAudioDecoderConfig
{
    friend class AMAudioDecoder;
  public:
    AMAudioDecoderConfig();
    virtual ~AMAudioDecoderConfig();
    AudioDecoderConfig* get_config(const std::string& config);

  private:
    AMConfig           *m_config;
    AudioDecoderConfig *m_decoder_config;
};

#endif /* AM_AUDIO_DECODER_CONFIG_H_ */
