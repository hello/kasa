/*******************************************************************************
 * am_api_sip.h
 *
 * History:
 *   Apr 30, 2015 - [ccjing] created file
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

/*! @file am_api_sip.h
 *  @brief This file defines SIP service related data structures
 */

#ifndef AM_API_SIP_H_
#define AM_API_SIP_H_
#include "commands/am_api_cmd_sip.h"

/*! @defgroup airapi-datastructure-sip Data Structure of SIP Service
 *  @ingroup airapi-datastructure
 *  @brief All Oryx SIP Service related method call data structures
 *  @{
 */

/*!
 * @class AMIApiMediaPriorityList
 * @brief This class is used as a container to store media priority list
 *        and provide the functions to add and get medias from the list.
 */
class AMIApiMediaPriorityList
{
  public:
    /*! Create function.
     */
    static AMIApiMediaPriorityList* create();
    /*! Create function.
     * @param media_list is a pointer of AMApiMediaPriorityList object
     */
    static AMIApiMediaPriorityList* create(AMIApiMediaPriorityList* media_list);
    /*! Destructor function.
     */
    virtual void destroy() = 0;
    /*! Destructor function.
     */
    virtual ~AMIApiMediaPriorityList(){}
    /*! This function is used to add medias to the media priority list.
     * @param media specify the media which will be added to the media
     *        priority list.
     * @return true if success, otherwise return false;
     */
    virtual bool add_media_to_list(const std::string &media) = 0;
    /*! This function is used to get media name  from the media priority list.
     * @param media specify the media number which will be got from the media
     *        priority list.
     * @return the media name;
     */
    virtual std::string get_media_name(uint32_t num) = 0;
    /*! This function is used to get the media number of the priority list
     * @return the media number of the priority list
     */
    virtual uint8_t get_media_number() = 0;
    /*! This function is used to get the whole priority list
     * @return the address of priority list.
     */
    virtual char* get_media_priority_list() = 0;
    /*! This function is used to get the size of the priority list
     * @return the size of the priority list
     */
    virtual uint32_t get_media_priority_list_size() = 0;
};

/*!
 * @class AMIApiSipRegisterParameter
 * @brief This class is used as a container to store the sip registration
 *        parameters and provide the functions to set and get parameters
 */
class AMIApiSipRegisterParameter
{
  public:
    /*! Create function.
     */
    static AMIApiSipRegisterParameter* create();
    /*! Destructor function.
     */
    virtual void destroy() = 0;
    /*! Destructor function.
     */
    virtual ~AMIApiSipRegisterParameter(){}
    /*! This function is used to set username parameter
     * @param username.
     * @return true if success, otherwise return false
     */
    virtual bool set_username(const std::string &username) = 0;
    /*! This function is used to get username parameter
     * @return username.
     */
    virtual std::string get_username() = 0;
    /*! This function is used to set the password parameter
     * @param password.
     * @return true if success, otherwise return false
     */
    virtual bool set_password(const std::string &password) = 0;
    /*! This function is used to get the password parameter
     * @return password.
     */
    virtual std::string get_password() = 0;
    /*! This function is used to set the server_url parameter
     * @param server_url.
     * @return true if success, otherwise return false
     */
    virtual bool set_server_url(const std::string &server_url) = 0;
    /*! This function is used to get the server_url parameter
     * @return server_url.
     */
    virtual std::string get_server_url() = 0;
    /*! This function is used to set the server_port parameter
     * @param server_port.
     */
    virtual void set_server_port(int32_t server_port) = 0;
    /*! This function is used to get the server_port parameter
     * @return server_port.
     */
    virtual int32_t get_server_port() = 0;
    /*! This function is used to set the sip_port parameter
     * @param sip_port.
     */
    virtual void set_sip_port(int32_t sip_port) = 0;
    /*! This function is used to get the sip_port parameter
     * @return sip_port.
     */
    virtual int32_t get_sip_port() = 0;
    /*! This function is used to set the expires parameter
     * @param expires.
     */
    virtual void set_expires(int32_t expires) = 0;
    /*! This function is used to get the expires parameter
     * @return expires.
     */
    virtual int32_t get_expires() = 0;
    /*! This function is used to set the is_register parameter
     * @param is_register.
     */
    virtual void set_is_register(bool is_register) = 0;
    /*! This function is used to get the is_register parameter
     * @return is_register.
     */
    virtual bool get_is_register() = 0;
    /*! This function is used to set the is_apply parameter
     *@param is_apply.
     */
    virtual void set_is_apply(bool is_apply) = 0;
    /*! This function is used to get the whole sip register parameter
     * @return the address of sip register parameter.
     */
    virtual char* get_sip_register_parameter() = 0;
    /*! This function is used to get the size of the priority list
     * @return the size of the sip register parameter.
     */
    virtual uint32_t get_sip_register_parameter_size() = 0;

};

/*!
 * @class AMApiSipConfigParameter
 * @brief This class is used as a container to store the sip configuration
 *        parameters and provide the functions to set parameters
 */
class AMIApiSipConfigParameter
{
  public:
    /*! Create function.
     */
    static AMIApiSipConfigParameter* create();
    /*! Destructor function.
     */
    virtual void destroy() = 0;
    /*! Destructor function.
     */
    virtual ~AMIApiSipConfigParameter(){}
    /*! This function is used to set the rtp_stream_port_base parameter
     * @param rtp_port.
     * @return true if success, otherwise return false.
     */
    virtual bool set_rtp_stream_port_base(int32_t rtp_port) = 0;
    /*! This function is used to get the rtp_stream_port_base parameter
     * @return rtp_stream_port_base.
     */
    virtual int32_t get_rtp_stream_port_base() = 0;
    /*! This function is used to set the max_conn_num parameter
     * @param max_conn_num specify the max_conn_num to modify
     * @return true if success, otherwise return false.
     */
    virtual bool set_max_conn_num(uint32_t max_conn_num) = 0;
    /*! This function is used to get the max_conn_num parameter
     * @return max_conn_num.
     */
    virtual uint32_t get_max_conn_num() = 0;
    /*! This function is used to set the m_enable_video parameter
     * @param enable_video specify the m_enable_video to modify
     */
    virtual void set_enable_video(bool enable_video) = 0;
    /*! This function is used to get the enable_video parameter
     * @param enable_video specify the enable_video to modify
     */
    virtual bool get_enable_video() = 0;
    /*! This function is used to get the whole sip config parameter
     * @return the address of sip config parameter.
     */
    virtual char* get_sip_config_parameter() = 0;
    /*! This function is used to get the size of the sip config parameter
     * @return the size of the sip config parameter.
     */
    virtual uint32_t get_sip_config_parameter_size() = 0;
};

/*!
 * @class AMIApiSipCalleeAddress
 * @brief This class is not only used as a container to store the sip callee
 *  address and provide the function to set it but also able to hang up calls.
 */
class AMIApiSipCalleeAddress
{
  public:
    /*! Create function.
     */
    static AMIApiSipCalleeAddress* create();
    /*! Destructor function.
     */
    virtual void destroy() = 0;
    /*! Destructor function.
     */
    virtual ~AMIApiSipCalleeAddress() {}
    /*! This function is used to set the m_callee_address parameter
     * @param sip callee address
     * @return true if success, otherwise return false.
     */
    virtual bool set_callee_address(const std::string &address) = 0;
    /*! This function is used to get the whole sip callee parameter
     * @return the address of sip callee parameter.
     */
    virtual char* get_sip_callee_parameter() = 0;
    /*! This function is used to get the size of the sip callee parameter
     * @return the size of the sip callee parameter.
     */
    virtual uint32_t get_sip_callee_parameter_size() = 0;
    /*! This function is used to set the hangup username
     * @param specify the username to hang up
     * @return true if success, otherwise return false.
     */
    virtual bool set_hangup_username(const std::string &username) = 0;
    /*! This function is used to get the username to hangup
     * @return the username for hanging up
     */
    virtual char* get_hangup_username() = 0;
    /*! This function is used to get the size of the username
     * @return the size of the username
     */
    virtual uint32_t get_hangup_username_size() = 0;

};

/*!
 * @}
 */
#endif /* AM_API_SIP_H_ */
