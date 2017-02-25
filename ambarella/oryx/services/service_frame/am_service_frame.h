/*******************************************************************************
 * am_service_frame.h
 *
 * History:
 *   2015-1-26 - [ypchang] created file
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
#ifndef ORYX_SERVICES_SERVICE_FRAME_AM_SERVICE_FRAME_H_
#define ORYX_SERVICES_SERVICE_FRAME_AM_SERVICE_FRAME_H_

#include "am_service_frame_if.h"
#include <semaphore.h>

enum AM_SERVICE_FRAME_CMD
{
  SERVICE_CMD_NULL  = 0,
  SERVICE_CMD_QUIT  = 'e',
  SERVICE_CMD_ABORT = 'a',
};
class AMThread;
class AMServiceFrame: public AMIServiceFrame
{
  public:
    static AMIServiceFrame* create(const std::string& name);

  public:
    virtual void destroy();
    virtual void run();
    virtual bool quit();
    virtual void set_user_input_callback(AMServiceFrameCallback cb);
    virtual uint32_t version();

  protected:
    AMServiceFrame(const std::string& name);
    virtual ~AMServiceFrame();
    bool init();

  private:
    bool send_ctrl_cmd(AM_SERVICE_FRAME_CMD cmd);
    bool send_wd_ctrl_cmd(AM_SERVICE_FRAME_CMD cmd);
    bool init_semaphore(const char* sem_name);
    static void static_send_heartbeat_to_watchdog(void *data);
    void send_heartbeat_to_watchdog();

  private:
    AMServiceFrameCallback m_user_input_callback;
    AMThread              *m_thread_feed_dog;
    sem_t                 *m_semaphore;
    int                    m_ctrl_sock[2];
    int                    m_wd_ctrl_sock[2];
    std::string            m_name;
    std::string            m_sem_name;
    bool                   m_run;
    bool                   m_thread_running;
    bool                   m_thread_created;

#define CTRL_SOCK_R    m_ctrl_sock[0]
#define CTRL_SOCK_W    m_ctrl_sock[1]
#define WD_CTRL_SOCK_R m_wd_ctrl_sock[0]
#define WD_CTRL_SOCK_W m_wd_ctrl_sock[1]
};

#endif /* ORYX_SERVICES_SERVICE_FRAME_AM_SERVICE_FRAME_H_ */
