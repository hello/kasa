/*******************************************************************************
 * am_sip_ua_server_data_structure.h
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
#ifndef AM_SIP_UA_SERVER_DATA_STRUCTURE_H_
#define AM_SIP_UA_SERVER_DATA_STRUCTURE_H_

#include "am_api_sip.h"
#include "am_sip_ua_server_protocol_data_structure.h"

class AMApiMediaPriorityList : public AMIApiMediaPriorityList
{
  public:
    AMApiMediaPriorityList();
    AMApiMediaPriorityList(AMIApiMediaPriorityList* media_list);
    virtual void destroy() {delete this;}
    virtual ~AMApiMediaPriorityList(){}
    virtual bool add_media_to_list(const std::string &media);
    virtual std::string get_media_name(uint32_t num);
    virtual uint8_t get_media_number();
    virtual char* get_media_priority_list();
    virtual uint32_t get_media_priority_list_size();
  private:
    AMMediaPriorityList m_list;
};

class AMApiSipRegisterParameter : public AMIApiSipRegisterParameter
{
  public:
    AMApiSipRegisterParameter();
    virtual ~AMApiSipRegisterParameter(){}
    virtual void destroy() {delete this;}
    virtual bool set_username(const std::string &username);
    virtual std::string get_username();
    virtual bool set_password(const std::string &password);
    virtual std::string get_password();
    virtual bool set_server_url(const std::string &server_url);
    virtual std::string get_server_url();
    virtual void set_server_port(int32_t server_port);
    virtual int32_t get_server_port();
    virtual void set_sip_port(int32_t sip_port);
    virtual int32_t get_sip_port();
    virtual void set_expires(int32_t expires);
    virtual int32_t get_expires();
    virtual void set_is_register(bool is_register);
    virtual bool get_is_register();
    virtual void set_is_apply(bool is_apply);
    virtual char* get_sip_register_parameter();
    virtual uint32_t get_sip_register_parameter_size();
  private:
    AMSipRegisterParameter m_register;
};

class AMApiSipConfigParameter : public AMIApiSipConfigParameter
{
  public:
    AMApiSipConfigParameter();
    virtual ~AMApiSipConfigParameter(){}
    virtual void destroy() {delete this;}
    virtual bool set_rtp_stream_port_base(int32_t rtp_port);
    virtual int32_t get_rtp_stream_port_base();
    virtual bool set_max_conn_num(uint32_t max_conn_num);
    virtual uint32_t get_max_conn_num();
    virtual void set_enable_video(bool enable_video);
    virtual bool get_enable_video();
    virtual char* get_sip_config_parameter();
    virtual uint32_t get_sip_config_parameter_size();
  private:
    AMSipConfigParameter m_config;
};

class AMApiSipCalleeAddress : public AMIApiSipCalleeAddress
{
  public:
    AMApiSipCalleeAddress();
    virtual ~AMApiSipCalleeAddress(){}
    virtual void destroy() {delete this;}
    virtual bool set_callee_address(const std::string &address);
    virtual char* get_sip_callee_parameter();
    virtual uint32_t get_sip_callee_parameter_size();
    virtual bool set_hangup_username(const std::string &username);
    virtual char* get_hangup_username();
    virtual uint32_t get_hangup_username_size();

  private:
    AMSipCalleeAddress m_callee_address;
    AMSipHangupUsername m_username;
};

#endif /* AM_SIP_UA_SERVER_DATA_STRUCTURE_H_ */
