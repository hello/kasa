/*******************************************************************************
 * am_audio_codec.h
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
#ifndef AM_AUDIO_CODEC_H_
#define AM_AUDIO_CODEC_H_

enum AudioInfoType
{
  AUDIO_INFO_ENCODE = 0,
  AUDIO_INFO_DECODE = 1,
  AUDIO_INFO_NUM
};

struct AM_AUDIO_INFO;
class AMAudioCodec: public AMIAudioCodec
{
  public:
    virtual void destroy();
    virtual const std::string& get_codec_name();
    virtual AM_AUDIO_CODEC_TYPE get_codec_type();
    virtual bool is_initialized();
    virtual bool initialize(AM_AUDIO_INFO *srcAudioInfo,
                            AM_AUDIO_CODEC_MODE mode);
    virtual bool finalize();
    virtual AM_AUDIO_INFO* get_codec_audio_info();
    virtual uint32_t get_codec_output_size();
    virtual bool get_encode_required_src_parameter(AM_AUDIO_INFO &info);
    virtual uint32_t encode(uint8_t *input,  uint32_t in_data_size,
                            uint8_t *output, uint32_t *out_data_size);
    virtual uint32_t decode(uint8_t *input,  uint32_t in_data_size,
                            uint8_t *output, uint32_t *out_data_size);

  protected:
    AMAudioCodec(AM_AUDIO_CODEC_TYPE type, const std::string& name);
    virtual ~AMAudioCodec();
    bool init();
    std::string codec_mode_string();

  protected:
    AM_AUDIO_INFO      *m_src_audio_info;
    AM_AUDIO_INFO      *m_audio_info;
    AM_AUDIO_CODEC_TYPE m_codec_type;
    bool                m_is_initialized;
    std::string         m_name;
    AM_AUDIO_CODEC_MODE m_mode;
};

#endif /* AM_AUDIO_CODEC_H_ */
