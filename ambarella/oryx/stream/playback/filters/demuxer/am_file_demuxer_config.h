/*******************************************************************************
 * am_file_demuxer_config.h
 *
 * History:
 *   2014-9-5 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_FILE_DEMUXER_CONFIG_H_
#define AM_FILE_DEMUXER_CONFIG_H_

#include "am_filter_config.h"
struct FileDemuxerConfig: public AMFilterConfig
{
    uint32_t packet_size;
    uint32_t wait_count;
    uint32_t file_empty_timeout;
    uint32_t packet_empty_timeout;
    FileDemuxerConfig() :
      packet_size(0),
      wait_count(0),
      file_empty_timeout(0),
      packet_empty_timeout(0)
    {}
};

class AMConfig;
class AMFileDemuxer;
class AMFileDemuxerConfig
{
    friend class AMFileDemuxer;
  public:
    AMFileDemuxerConfig();
    virtual ~AMFileDemuxerConfig();
    FileDemuxerConfig* get_config(const std::string& config);

  private:
    AMConfig          *m_config;
    FileDemuxerConfig *m_demuxer_config;
};

#endif /* AM_FILE_DEMUXER_CONFIG_H_ */
