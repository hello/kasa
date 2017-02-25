/*******************************************************************************
 * am_service_base.h
 *
 * History:
 *   2014-9-12 - [lysun] created file
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
#ifndef AM_SERVICE_BASE_H_
#define AM_SERVICE_BASE_H_
#include "am_base_include.h"
#include "commands/am_service_impl.h"
#include "am_thread.h"
#include "am_mutex.h"

#include <iostream>
#include <mutex>
#include <deque>

/*
 * Any Oryx Service must be singleton, which means, it cannot be created twice
 * within same process.  and more than that, Oryx service should not even be created
 * more than once in whole system.
 * so we use singleton to protect the class creation, and use pidlock to prevent
 * the instance being created more than once in whole system.
 */

#define AM_SERVICE_EXE_FILENAME_MAX_LENGTH 512
#define AM_SERVICE_MAX_APPS_ARGS_NUM       10

typedef int (*AM_SVC_NOTIFY_CB)(void *proxy_ptr,
                                const void *msg_data,
                                int msg_data_size);

class AMServiceBase;

typedef struct workq_ctx {
  AMServiceBase *service = nullptr;
  am_service_notify_payload payload;
} workq_ctx_t;

class AMIPCSyncCmdClient;
class AMServiceBase
{
  public:
    AMServiceBase(const char *service_name,
                  const char *full_pathname,
                  AM_SERVICE_CMD_TYPE type);
    AMServiceBase(const AMServiceBase& base) = delete;
    virtual ~AMServiceBase();

  public:
    const char* get_name();
    AM_SERVICE_CMD_TYPE get_type();
    int register_msg_map(); //register msg map to handle the notification received from real process
    int register_svc_notify_cb(void *proxy_ptr, AM_SVC_NOTIFY_CB cb);

    //init/destroy are special functrions of the service.
    virtual int init();     //service instance init, call it only once after creation
    //All IPC connections of that service are also initialized
    virtual int destroy();  //deinit process, restore to status before init called
    //should be called when service is not actively running
    //ALL IPC connections of that service are also destroyed

    virtual int start();
    virtual int stop();
    virtual int restart();
    virtual int status(AM_SERVICE_STATE  *state);

    virtual int method_call(uint32_t cmd_id,
                            void *msg_data, int msg_data_size,
                            void *result_addr, int result_max_size);

  protected:
    static void static_workq_thread(void *data);
    static void static_on_notif_callback(uintptr_t  context,
                                         void      *msg_data,
                                         int        msg_data_size,
                                         void      *result_addr,
                                         int        result_max_size);

  protected:
    void on_notif_callback(void      *msg_data,
                           int        msg_data_size,
                           void      *result_addr,
                           int        result_max_size);
    int create_process();
    int wait_process();
    int create_ipc();
    int cleanup_ipc();

  private:
    void workq_thread();
    int workq_create();
    void workq_destroy();
    int workq_task_post(void *data, size_t len);
    int workq_task_exec(void *data, size_t len);
    int workq_task_flush();
    int workq_task_quit();

  protected:
    void               *m_notify_cb_data = nullptr;
    AMIPCSyncCmdClient *m_ipc            = nullptr;
    AM_SVC_NOTIFY_CB    m_notify_cb      = nullptr;
    pid_t               m_service_pid    = -1;
    AM_SERVICE_STATE    m_state          = AM_SERVICE_STATE_NOT_INIT;
    AM_SERVICE_CMD_TYPE m_type           = AM_SERVICE_TYPE_OTHERS;
    //the service is going to run in a new process with this pid
    std::string         m_name;
    std::string         m_exe_filename;

  private:
    AMThread               *m_workq_thread = nullptr;
    int32_t                 m_workq_ctrl_fd[2] = {-1, -1};
    bool                    m_workq_loop   = false;
    std::deque<workq_ctx_t> m_workq_ctx_q;
#define WORKQ_CTRL_READ     m_workq_ctrl_fd[0]
#define WORKQ_CTRL_WRITE    m_workq_ctrl_fd[1]
};

#endif /* AM_SERVICE_BASE_H_ */
