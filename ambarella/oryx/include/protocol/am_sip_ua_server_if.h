/*******************************************************************************
 * am_sip_ua_server_if.h
 *
 * History:
 *   2015-1-27 - [Shiming Dong] created file
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
#ifndef ORYX_INCLUDE_PROTOCOL_AM_SIP_UA_SERVER_IF_H_
#define ORYX_INCLUDE_PROTOCOL_AM_SIP_UA_SERVER_IF_H_
/*!
 * @mainpage Oryx SIP Protocol
 * @section Introduction
 * Oryx SIP Protocol module is used to handle SIP service,
 * which supports register to SIP server and bi-directional
 * audio communication.
 */

/*!
 * @file am_sip_ua_server_if.h
 * @brief defines interfaces to control sip_ua_server
 */
#include "am_pointer.h"
#include "version.h"
#include "am_sip_ua_server_protocol_data_structure.h"
class AMISipUAServer;
/*!
 * @typedef AMISipUAServerPtr
 * @brief Smart pointer type to handle AMISipUAServer
 */
typedef AMPointer<AMISipUAServer> AMISipUAServerPtr;

/*!
 * @class AMISipUAServer
 * @brief AMISipUAServer is a interface class to control sip service
 *     This class can not only create, start and stop the SIP service but
 *     also modify the SIP configuration file.
 */
class AMISipUAServer
{
  friend AMISipUAServerPtr;

  public:
    /*!
     * Constructor function
     * @return AMISipUAServerPtr
     */
    static AMISipUAServerPtr create();
    /*!
     * Start function
     * @return bool true on success, false on failure
     */
    virtual bool start()       = 0;
    /*!
     * Stop function
     * @return bool true on success, false on failure
     */
    virtual void stop()        = 0;
    /*!
     * get version function
     * @return uint32_t
     */
    virtual uint32_t version() = 0;
    /*!
     * This api can modify the SIP registration parameters
     * @param regis_para specify the parameters to modify
     * @return bool true on success, false on failure
     */
    virtual bool set_sip_registration_parameter(AMSipRegisterParameter
                                                *regis_para)  = 0;
    /*!
     * This api can modify the SIP configuration parameters
     * @param config_para specify the parameters to modify
     * @return bool true on success, false on failure
     */
    virtual bool set_sip_config_parameter(AMSipConfigParameter
                                          *config_para)  = 0;
    /*!
     *This api can modify the SIP media priority list which used to
     *determine the priority of which media type to use
     *@param media_prio specify the media priority list
     *@return bool true on success, false on failure
     */
    virtual bool set_sip_media_priority_list(AMMediaPriorityList
                                             *media_prio)  = 0;
    /*!
     *This api can initiate a SIP invite call
     *@param address specify the call address
     *@return bool true on success, false on failure
     */
    virtual bool initiate_sip_call(AMSipCalleeAddress *address) = 0;

    /*!
     *This api not only can hang up specific call but also disconnect
     *all connections.
     *@param name specify the username to hangup
     *@return bool true on success, false on failure
     */
    virtual bool hangup_sip_call(AMSipHangupUsername *name) = 0;

  protected:
    virtual void inc_ref()     = 0;
    virtual void release()     = 0;
    virtual ~AMISipUAServer(){}
};

#endif /* ORYX_INCLUDE_PROTOCOL_AM_SIP_UA_SERVER_IF_H_ */
