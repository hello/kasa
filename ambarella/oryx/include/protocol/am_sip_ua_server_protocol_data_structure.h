/*******************************************************************************
 * am_sip_ua_server_protocol_data_structure.h
 *
 * History:
 *   Apr 30, 2015 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_SIP_UA_SERVER_PROTOCOL_DATA_STRUCTURE_H_
#define AM_SIP_UA_SERVER_PROTOCOL_DATA_STRUCTURE_H_

struct AMMediaPriorityList
{
  enum {
    MAX_MEDIA_NUMBER   = 10,
    MAX_MEDIA_NAME_LEN = 16,
  };

  bool add_media_to_list(const std::string &media);

  uint8_t m_media_num;
  char    m_media_list[MAX_MEDIA_NUMBER][MAX_MEDIA_NAME_LEN];
};

struct AMSipRegisterParameter
{
  enum {
    MAX_USERNAME   = 32,
    MAX_PASSWORD   = 32,
    MAX_SERVER_URL = 64
  };
  enum {
    USERNAME_FLAG = 0,
    PASSWORD_FLAG,
    SERVER_URL_FLAG,
    SERVER_PORT_FLAG,
    SIP_PORT_FLAG,
    EXPIRES_FLAG,
    IS_REGISTER_FLAG,
  };

  bool set_username(const std::string &username);
  bool set_password(const std::string &password);
  bool set_server_url(const std::string &server_url);
  void set_server_port(int32_t server_port);
  void set_sip_port(int32_t sip_port);
  void set_expires(int32_t expires);
  void set_is_register(bool is_register);
  void set_is_apply(bool is_apply);

  char     m_username[MAX_USERNAME];
  char     m_password[MAX_PASSWORD];
  char     m_server_url[MAX_SERVER_URL];
  int32_t  m_server_port;
  int32_t  m_sip_port;
  int32_t  m_expires;
  uint16_t m_modify_flag;
  bool     m_is_register;
  bool     m_is_apply;
};

struct AMSipConfigParameter
{
  enum {
    RTP_PORT_FLAG = 0,
    MAX_CONN_NUM_FLAG,
    ENABLE_VIDEO_FLAG
  };

  bool set_rtp_stream_port_base(int32_t rtp_port);
  bool set_max_conn_num(uint32_t max_conn_num);
  void set_enable_video(bool enable_video);

  int32_t  m_rtp_stream_port_base;
  uint32_t m_max_conn_num;
  uint16_t m_modify_flag;
  bool     m_enable_video;

};

struct AMSipCalleeAddress
{
  enum {
    MAX_ADDRESS_LENGTH = 64
  };

  bool set_callee_address(const std::string &address);

  char callee_address[MAX_ADDRESS_LENGTH];
};

struct AMSipHangupUsername
{
  enum {
    MAX_NAME_LENGTH = 32
  };

  bool set_hangup_username(const std::string &username);

  char user_name[MAX_NAME_LENGTH];
};

#endif /* AM_SIP_UA_SERVER_PROTOCOL_DATA_STRUCTURE_H_ */
