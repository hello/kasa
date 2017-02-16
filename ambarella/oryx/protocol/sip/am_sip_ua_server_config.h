/*******************************************************************************
 * am_sip_ua_server_config.h
 *
 * History:
 *   2015-1-28 - [Shiming Dong] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_SERVICES_SIP_UA_SERVER_AM_SIP_UA_SERVER_CONFIG_H_
#define ORYX_SERVICES_SIP_UA_SERVER_AM_SIP_UA_SERVER_CONFIG_H_

#include <vector>

typedef std::vector<std::string> MediaPriorityList;

struct SipUAServerConfig
{
  std::string sip_unix_domain_name;
  std::string sip_to_media_service_name;
  uint32_t sip_retry_interval;
  uint32_t sip_send_retry_count;
  uint32_t sip_send_retry_interval;
  std::string username;
  std::string password;
  std::string server_url;
  uint16_t    server_port;
  uint16_t    sip_port;
  int32_t     expires;
  bool        is_register;

  uint16_t    rtp_stream_port_base;
  uint32_t    max_conn_num;
  bool        enable_video;
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
