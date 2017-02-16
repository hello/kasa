/*******************************************************************************
 * am_rtsp_config.h
 *
 * History:
 *   2015-1-20 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_SERVICES_RTSP_SERVER_AM_RTSP_SERVER_CONFIG_H_
#define ORYX_SERVICES_RTSP_SERVER_AM_RTSP_SERVER_CONFIG_H_

#include <vector>

struct RtspServerStreamDefine
{
    std::string video;
    std::string audio;
};
typedef std::vector<RtspServerStreamDefine> RtspStreamList;

struct RtspServerConfig
{
    std::string rtsp_server_name;
    std::string rtsp_unix_domain_name;
    std::string rtsp_server_user_db;
    uint32_t rtsp_retry_interval;
    uint32_t rtsp_server_port;
    uint32_t rtsp_send_retry_count;
    uint32_t rtsp_send_retry_interval;
    uint32_t rtp_stream_port_base;
    bool     need_auth;
    uint32_t max_client_num;
    RtspStreamList stream;
};

class AMConfig;
class AMRtspServerConfig
{
  public:
    AMRtspServerConfig();
    virtual ~AMRtspServerConfig();
    RtspServerConfig* get_config(const std::string& conf);

  private:
    RtspServerConfig *m_rtsp_config;
    AMConfig         *m_config;
};

#endif /* ORYX_SERVICES_RTSP_SERVER_AM_RTSP_SERVER_CONFIG_H_ */
