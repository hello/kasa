/*******************************************************************************
 * am_video_source_config.h
 *
 * History:
 *   2014-12-11 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_RECORD_FILTERS_VIDEO_AM_VIDEO_SOURCE_CONFIG_H_
#define ORYX_STREAM_RECORD_FILTERS_VIDEO_AM_VIDEO_SOURCE_CONFIG_H_

#include "am_filter_config.h"

struct VideoSourceConfig: public AMFilterConfig
{
    int32_t query_op_timeout;
    VideoSourceConfig() :
      query_op_timeout(1000)
    {}
};

class AMConfig;
class AMVideoSource;
class AMVideoSourceConfig
{
    friend class AMVideoSource;
  public:
    AMVideoSourceConfig();
    virtual ~AMVideoSourceConfig();
    VideoSourceConfig* get_config(const std::string& conf_file);

  private:
    AMConfig          *m_config;
    VideoSourceConfig *m_video_source_config;
};

#endif /* ORYX_STREAM_RECORD_FILTERS_VIDEO_AM_VIDEO_SOURCE_CONFIG_H_ */
