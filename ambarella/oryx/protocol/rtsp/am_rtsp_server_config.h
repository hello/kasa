/*******************************************************************************
 * am_rtsp_config.h
 *
 * History:
 *   2015-1-20 - [ypchang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
    uint32_t rtsp_retry_interval;
    uint32_t rtsp_server_port;
    uint32_t rtsp_send_retry_count;
    uint32_t rtsp_send_retry_interval;
    uint32_t rtp_stream_port_base;
    uint32_t max_client_num;
    bool     need_auth;
    bool     use_epoll;
    std::string rtsp_server_name;
    std::string rtsp_unix_domain_name;
    std::string rtsp_server_user_db;
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
