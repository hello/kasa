/*******************************************************************************
 * am_muxer_config.h
 *
 * History:
 *   2014-12-26 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_CONFIG_H_
#define ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_CONFIG_H_

#include "am_filter_config.h"

struct MuxerConfig: public AMFilterConfig
{
    uint32_t media_type_num;
    std::string *media_type;
    AM_MUXER_TYPE type;
    MuxerConfig() :
      media_type_num(0),
      media_type(nullptr),
      type(AM_MUXER_TYPE_NONE)
    {}
    ~MuxerConfig()
    {
      delete[] media_type;
    }
};

class AMConfig;
class AMMuxerConfig
{

  public:
    AMMuxerConfig();
    virtual ~AMMuxerConfig();
    MuxerConfig* get_config(const std::string& conf_file);

  private:
    AMConfig    *m_config;
    MuxerConfig *m_muxer_config;
};


#endif /* ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_CONFIG_H_ */
