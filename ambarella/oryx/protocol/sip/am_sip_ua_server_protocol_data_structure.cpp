/*******************************************************************************
 * am_sip_ua_server_protocol_data_structure.cpp
 *
 * History:
 *   May 7, 2015 - [ccjing] created file
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
#include "am_base_include.h"
#include "am_log.h"
#include "am_sip_ua_server_protocol_data_structure.h"

bool AMMediaPriorityList::add_media_to_list(const std::string &media)
{
  bool ret = true;
   if (media.empty()) {
     ERROR("Invalid parameter, media is empty.");
     ret = false;
   } else if (m_media_num >= MAX_MEDIA_NUMBER) {
     ERROR("The media priority list is full, the max number is %d.",
           MAX_MEDIA_NUMBER);
     ret = false;
   } else if (media.size() >= MAX_MEDIA_NAME_LEN) {
     ERROR("Media name is too long, the max length is %d" ,
       MAX_MEDIA_NAME_LEN - 1);
     ret = false;
   } else {
     strcpy(m_media_list[m_media_num], media.c_str());
     m_media_num++;
   }
   return ret;
}

bool AMSipRegisterParameter::set_username(const std::string &username)
{
  bool ret = true;
  if (username.empty()) {
    ERROR("Invalid parameter, username is empty.");
    ret = false;
  } else if (username.size() >= MAX_USERNAME) {
    ERROR("User name  is too long, the max length is %d" , MAX_USERNAME - 1);
    ret = false;
  } else {
    strcpy(m_username, username.c_str());
    m_modify_flag |= (1 << AMSipRegisterParameter::USERNAME_FLAG);
  }
  return ret;
}

bool AMSipRegisterParameter::set_password(const std::string &password)
{
  bool ret = true;
  if (password.empty()) {
    ERROR("Invalid parameter, password is empty.");
    ret = false;
  } else if (password.size() >= MAX_PASSWORD) {
    ERROR("Password is too long, the max length is %d", MAX_PASSWORD - 1);
    ret = false;
  } else {
    strcpy(m_password, password.c_str());
    m_modify_flag |= (1 << PASSWORD_FLAG);
  }
  return ret;
}

bool AMSipRegisterParameter::set_server_url(const std::string &server_url)
{
  bool ret = true;
  if (server_url.empty()) {
    ERROR("Invalid parameter, server_url is empty.");
    ret = false;
  } else if (server_url.size() >= MAX_SERVER_URL) {
    ERROR("server_url is too long, the max length is %d",
          MAX_SERVER_URL - 1);
    ret = false;
  } else {
    strcpy(m_server_url, server_url.c_str());
    m_modify_flag |= (1 << SERVER_URL_FLAG);
  }
  return ret;
}

void AMSipRegisterParameter::set_server_port(int32_t server_port)
{
  m_server_port = server_port;
  m_modify_flag |= (1 << SERVER_PORT_FLAG);
}

void AMSipRegisterParameter::set_sip_port(int32_t sip_port)
{
  m_sip_port = sip_port;
  m_modify_flag |= (1 << SIP_PORT_FLAG);
}

void AMSipRegisterParameter::set_expires(int32_t expires)
{
  m_expires = expires;
  m_modify_flag |= (1 << EXPIRES_FLAG);
}

void AMSipRegisterParameter::set_is_register(bool is_register)
{
  m_is_register = is_register;
  m_modify_flag |= (1 << IS_REGISTER_FLAG);
}

void AMSipRegisterParameter::set_is_apply(bool is_apply)
{
  m_is_apply = is_apply;
}

bool AMSipConfigParameter::set_rtp_stream_port_base(int32_t rtp_port)
{
  bool ret = true;
  if (rtp_port > 0) {
    m_rtp_stream_port_base = rtp_port;
    m_modify_flag |= (1 << RTP_PORT_FLAG);
  } else {
    ret = false;
    ERROR("Invalid parameter!");
  }
  return ret;
}

bool AMSipConfigParameter::set_max_conn_num(uint32_t max_conn_num)
{
  bool ret = true;
  if (max_conn_num > 0) {
    m_max_conn_num = max_conn_num;
    m_modify_flag |= (1 << MAX_CONN_NUM_FLAG);
  } else {
    ret = false;
    ERROR("Invalid parameter!");
  }
  return ret;
}

void AMSipConfigParameter::set_enable_video(bool enable_video)
{
  m_enable_video = enable_video;
  m_modify_flag |= (1 << ENABLE_VIDEO_FLAG);
}

bool AMSipCalleeAddress::set_callee_address(const std::string &address)
{
  bool ret = true;
  if (address.empty()) {
    ERROR("Invalid parameter, address is empty.");
    ret = false;
  } else if (address.size() >= MAX_ADDRESS_LENGTH) {
    ERROR("Callee address is too long, the max length is %d" ,
           MAX_ADDRESS_LENGTH - 1);
    ret = false;
  } else {
    strcpy(callee_address, address.c_str());
  }
  return ret;
}

bool AMSipHangupUsername::set_hangup_username(const std::string &username)
{
  bool ret = true;
  if (username.empty()) {
    ERROR("Invalid parameter, username is empty.");
    ret = false;
  } else if (username.size() >= MAX_NAME_LENGTH) {
    ERROR("Username is too long, the max length is %d", MAX_NAME_LENGTH - 1);
    ret = false;
  } else {
    strcpy(user_name, username.c_str());
  }
  return ret;
}
