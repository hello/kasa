/*******************************************************************************
 * am_sip_ua_server_data_structure.cpp
 *
 * History:
 *   May 7, 2015 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#include "am_base_include.h"
#include "am_sip_ua_server_data_structure.h"

AMIApiMediaPriorityList* AMIApiMediaPriorityList::create()
{
  AMApiMediaPriorityList* m_list = new AMApiMediaPriorityList();
  return m_list ? (AMIApiMediaPriorityList*)m_list : NULL;
}

AMIApiMediaPriorityList* AMIApiMediaPriorityList::create(
    AMIApiMediaPriorityList* media_list)
{
  AMApiMediaPriorityList* m_list = new AMApiMediaPriorityList(media_list);
  return m_list ? (AMIApiMediaPriorityList*)m_list : NULL;
}

AMApiMediaPriorityList::AMApiMediaPriorityList()
{
  memset(&m_list, 0, sizeof(AMMediaPriorityList));
}

AMApiMediaPriorityList::AMApiMediaPriorityList(
    AMIApiMediaPriorityList* media_list)
{
  memcpy(&m_list, media_list->get_media_priority_list(),
         media_list->get_media_priority_list_size());
}

bool AMApiMediaPriorityList::add_media_to_list(const std::string &media)
{
  return m_list.add_media_to_list(media);
}

std::string AMApiMediaPriorityList::get_media_name(uint32_t num)
{
  return (num <= m_list.m_media_num) ? std::string(m_list.m_media_list[num]) :
      std::string("");
}

uint8_t AMApiMediaPriorityList::get_media_number()
{
  return m_list.m_media_num;
}

char* AMApiMediaPriorityList::get_media_priority_list()
{
  return (char*)&m_list;
}

uint32_t AMApiMediaPriorityList::get_media_priority_list_size()
{
  return sizeof(AMMediaPriorityList);
}

AMIApiSipRegisterParameter*::AMIApiSipRegisterParameter::create()
{
  AMApiSipRegisterParameter* m_register = new AMApiSipRegisterParameter();
  return m_register ? (AMIApiSipRegisterParameter*)m_register : NULL;
}

AMApiSipRegisterParameter::AMApiSipRegisterParameter()
{
  memset(&m_register, 0, sizeof(AMSipRegisterParameter));
}

bool AMApiSipRegisterParameter::set_username(const std::string &username)
{
  return m_register.set_username(username);
}

std::string AMApiSipRegisterParameter::get_username()
{
  return std::string(m_register.m_username);
}

bool AMApiSipRegisterParameter::set_password(const std::string &password)
{
  return m_register.set_password(password);
}

std::string AMApiSipRegisterParameter::get_password()
{
  return std::string(m_register.m_password);
}

bool AMApiSipRegisterParameter::set_server_url(const std::string &server_url)
{
  return m_register.set_server_url(server_url);
}

std::string AMApiSipRegisterParameter::get_server_url()
{
  return std::string(m_register.m_server_url);
}

void AMApiSipRegisterParameter::set_server_port(int32_t server_port)
{
  m_register.set_server_port(server_port);
}

int32_t AMApiSipRegisterParameter::get_server_port()
{
  return m_register.m_server_port;
}

void AMApiSipRegisterParameter::set_sip_port(int32_t sip_port)
{
  m_register.set_sip_port(sip_port);
}

int32_t AMApiSipRegisterParameter::get_sip_port()
{
  return m_register.m_sip_port;
}

void AMApiSipRegisterParameter::set_expires(int32_t expires)
{
  m_register.set_expires(expires);
}

int32_t AMApiSipRegisterParameter::get_expires()
{
  return m_register.m_expires;
}

void AMApiSipRegisterParameter::set_is_register(bool is_register)
{
  m_register.set_is_register(is_register);
}

bool AMApiSipRegisterParameter::get_is_register()
{
  return m_register.m_is_register;
}

void AMApiSipRegisterParameter::set_is_apply(bool is_apply)
{
  m_register.set_is_apply(is_apply);
}

char* AMApiSipRegisterParameter::get_sip_register_parameter()
{
  return (char*)&m_register;
}

uint32_t AMApiSipRegisterParameter::get_sip_register_parameter_size()
{
  return sizeof(AMSipRegisterParameter);
}

AMIApiSipConfigParameter* AMIApiSipConfigParameter::create()
{
  AMApiSipConfigParameter* m_config = new AMApiSipConfigParameter();
  return m_config ? (AMIApiSipConfigParameter*)m_config : NULL;
}

AMApiSipConfigParameter::AMApiSipConfigParameter()
{
  memset(&m_config, 0, sizeof(AMSipConfigParameter));
}

bool AMApiSipConfigParameter::set_rtp_stream_port_base(int32_t rtp_port)
{
  return m_config.set_rtp_stream_port_base(rtp_port);
}

int32_t AMApiSipConfigParameter::get_rtp_stream_port_base()
{
  return m_config.m_rtp_stream_port_base;
}

bool AMApiSipConfigParameter::set_max_conn_num(uint32_t max_conn_num)
{
  return m_config.set_max_conn_num(max_conn_num);
}

uint32_t AMApiSipConfigParameter::get_max_conn_num()
{
  return m_config.m_max_conn_num;
}

void AMApiSipConfigParameter::set_enable_video(bool enable_video)
{
  m_config.set_enable_video(enable_video);
}

bool AMApiSipConfigParameter::get_enable_video()
{
  return m_config.m_enable_video;
}

char* AMApiSipConfigParameter::get_sip_config_parameter()
{
  return (char*)&m_config;
}

uint32_t AMApiSipConfigParameter::get_sip_config_parameter_size()
{
  return sizeof(AMSipConfigParameter);
}

AMIApiSipCalleeAddress* AMIApiSipCalleeAddress::create()
{
  AMApiSipCalleeAddress *address = new AMApiSipCalleeAddress;
  return address ? (AMIApiSipCalleeAddress*)address : nullptr;
}

AMApiSipCalleeAddress::AMApiSipCalleeAddress()
{
  memset(&m_callee_address, 0, sizeof(AMSipCalleeAddress));
}

bool AMApiSipCalleeAddress::set_callee_address(const std::string &address)
{
  return m_callee_address.set_callee_address(address);
}

char* AMApiSipCalleeAddress::get_sip_callee_parameter()
{
  return (char*)&m_callee_address;
}

uint32_t AMApiSipCalleeAddress::get_sip_callee_parameter_size()
{
  return sizeof(AMSipCalleeAddress);
}

bool AMApiSipCalleeAddress::set_hangup_username(const std::string &username)
{
  return m_username.set_hangup_username(username);
}

char* AMApiSipCalleeAddress::get_hangup_username()
{
  return (char*)&m_username;
}

uint32_t AMApiSipCalleeAddress::get_hangup_username_size()
{
  return sizeof(AMSipHangupUsername);
}
