/*******************************************************************************
 * am_api_proxy.h
 *
 * History:
 *   Jul 29, 2016 - [Shupeng Ren] created file
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
#ifndef ORYX_SERVICES_INCLUDE_AM_API_PROXY_H_
#define ORYX_SERVICES_INCLUDE_AM_API_PROXY_H_

#include "am_mutex.h"
#include "am_ipc_uds.h"

#include <memory>

class AMAPIProxy;

typedef std::shared_ptr<AMAPIProxy> AMAPIProxyPtr;
class AMAPIProxy
{
  public:
    static AMAPIProxyPtr get_instance();
    int register_notify_cb(AM_SERVICE_CMD_TYPE type);
    static int on_notify(void *api_ipc, const void *msg_data, int msg_data_size);

  private:
    static void on_static_receive_from_app(int32_t context, int fd);
    static void on_static_client_disconnect(int32_t context, int fd);
    void on_receive_from_app(int fd, const uint8_t *data, int32_t size);
    void on_client_disconnect(int fd);

    AM_RESULT construct();
    AMAPIProxy();
    virtual ~AMAPIProxy();

  private:
    std::vector<int> m_notify_fds;
    static AMAPIProxy *m_instance;
    static AMMemLock m_lock;
    AMIPCServerUDSPtr m_server = nullptr;
};

#endif /* ORYX_SERVICES_INCLUDE_AM_API_PROXY_H_ */
