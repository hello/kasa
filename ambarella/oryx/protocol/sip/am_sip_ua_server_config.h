/*******************************************************************************
 * am_sip_ua_server_config.h
 *
 * History:
 *   2015-1-28 - [Shiming Dong] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
#ifndef ORYX_SERVICES_SIP_UA_SERVER_AM_SIP_UA_SERVER_CONFIG_H_
#define ORYX_SERVICES_SIP_UA_SERVER_AM_SIP_UA_SERVER_CONFIG_H_

#include <vector>

typedef std::vector<std::string> MediaPriorityList;

struct SipUAServerConfig
{
  uint32_t    sip_retry_interval;
  uint32_t    sip_send_retry_count;
  uint32_t    sip_send_retry_interval;
  int32_t     expires;
  uint16_t    max_conn_num;
  uint16_t    server_port;
  uint16_t    sip_port;
  uint16_t    rtp_stream_port_base;
  uint16_t    frames_remain_in_jb;
  bool        jitter_buffer;
  bool        is_register;
  bool        enable_video;
  std::string sip_unix_domain_name;
  std::string sip_to_media_service_name;
  std::string sip_media_rtp_client_name;
  std::string sip_media_rtp_server_name;
  std::string username;
  std::string password;
  std::string server_url;
  MediaPriorityList audio_priority_list;
  MediaPriorityList video_priority_list;

};

class AMConfig;
class AMSipUAServerConfig
{
  public:
  AMSipUAServerConfig();
  virtual ~AMSipUAServerConfig();
  SipUAServerConfig* get_config(const std::string& conf);

  private:
  SipUAServerConfig *m_sip_config;
  AMConfig          *m_config;
};



#endif /* ORYX_SERVICES_SIP_UA_SERVER_AM_SIP_UA_SERVER_CONFIG_H_ */
