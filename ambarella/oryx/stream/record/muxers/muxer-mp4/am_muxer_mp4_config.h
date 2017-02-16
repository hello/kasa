/*******************************************************************************
 * am_muxer_mp4_config.h
 *
 * History:
 *   2014-12-23 - [ccjing] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_MUXER_MP4_CONFIG_H_
#define AM_MUXER_MP4_CONFIG_H_
#include "am_audio_define.h"

struct AMMuxerCodecMp4Config
{
    std::string    file_name_prefix;
    std::string    file_location;
    uint64_t       file_duration;
    uint32_t       max_file_number;
    uint32_t       video_id;
    uint32_t       smallest_free_space;
    AM_AUDIO_TYPE  audio_type;
    AM_MUXER_ATTR  muxer_attr;
    bool            hls_enable;
    bool            auto_file_writing;
    bool            file_location_auto_parse;
};

class AMConfig;
class AMMuxerMp4Config
{
  public:
    AMMuxerMp4Config();
    virtual ~AMMuxerMp4Config();
    AMMuxerCodecMp4Config* get_config(const std::string& config_file);
    //write config information into config file
    bool set_config(AMMuxerCodecMp4Config* config);


  private:
    AMConfig              *m_config;
    AMMuxerCodecMp4Config *m_muxer_mp4_config;
};

#endif /* AM_MUXER_MP4_CONFIG_H_ */
