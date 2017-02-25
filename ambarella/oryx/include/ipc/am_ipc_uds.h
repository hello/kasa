/*******************************************************************************
 * am_ipc_uds.h
 *
 * History:
 *   Jul 28, 2016 - [Shupeng Ren] created file
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
#ifndef ORYX_INCLUDE_IPC_AM_IPC_UDS_H_
#define ORYX_INCLUDE_IPC_AM_IPC_UDS_H_

#include "am_result.h"
#include "am_mutex.h"
#include <map>
#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <condition_variable>

class AMIPCServerUDS;
class AMIPCClientUDS;

typedef void (*notify_cb_t)(int32_t context, int fd);

typedef std::shared_ptr<AMIPCServerUDS> AMIPCServerUDSPtr;
class AMIPCServerUDS
{
  public:
    static AMIPCServerUDSPtr create(const char *path);

    AM_RESULT send(int fd, const uint8_t *data, int32_t size);
    AM_RESULT send_to_all(const uint8_t *data, int32_t size);
    AM_RESULT recv(int fd, uint8_t *data, int32_t &size);
    AM_RESULT register_connect_cb(notify_cb_t cb, int32_t context = 0);
    AM_RESULT register_disconnect_cb(notify_cb_t cb, int32_t context = 0);
    AM_RESULT register_receive_cb(notify_cb_t cb, int32_t context = 0);

  private:
    int m_sockfd = -1;
    int m_socketpair_fd[2] = {-1, -1};
    std::vector<int> m_connected_fds;
    std::string m_sock_path;
    std::mutex m_recv_mutex;
    std::condition_variable m_recv_cond;
    std::map<int, std::atomic_bool> m_reading;
    std::atomic_bool wait_recv_done = {false};
    std::shared_ptr<std::thread> m_thread = nullptr;
    AMMemLock m_connect_cb_lock;
    AMMemLock m_disconnect_cb_lock;
    AMMemLock m_receive_cb_lock;
    int32_t m_connect_callback_context = 0;
    int32_t m_disconnect_callback_context = 0;
    int32_t m_receive_callback_context = 0;
    notify_cb_t connect_callback = nullptr;
    notify_cb_t disconnect_callback = nullptr;
    notify_cb_t receive_callback = nullptr;

  private:
    static void static_socket_process(AMIPCServerUDS *p);
    void socket_process();
    void close_and_remove_clientfd(int fd);
    bool check_clientfd(int fd);
    AM_RESULT construct(const char *path);
    AMIPCServerUDS();
    AMIPCServerUDS(AMIPCServerUDS const &copy) = delete;
    AMIPCServerUDS& operator=(AMIPCServerUDS const &copy) = delete;
    virtual ~AMIPCServerUDS();
};

typedef std::shared_ptr<AMIPCClientUDS> AMIPCClientUDSPtr;
class AMIPCClientUDS
{
  public:
    static AMIPCClientUDSPtr create();

    AM_RESULT connect(const char *path,
                      int32_t retry = 10, int32_t retry_delay_ms = 100);
    AM_RESULT distconnect();

    AM_RESULT send(const uint8_t *data, int32_t size);
    AM_RESULT recv(uint8_t *data, int32_t &size);

    AM_RESULT register_receive_cb(notify_cb_t cb, int32_t context = 0);

  private:
    int m_sockfd = -1;
    int m_epoll_fd = -1;
    int m_socketpair_fd[2] = {-1, -1};
    AMMemLock m_connect_lock;
    AMMemLock m_receive_cb_lock;
    int32_t m_receive_callback_context = 0;
    notify_cb_t receive_callback = nullptr;
    std::mutex m_recv_mutex;
    std::condition_variable m_recv_cond;
    std::shared_ptr<std::thread> m_thread = nullptr;
    std::atomic_bool wait_recv_done = {false};
    std::atomic_bool m_read_available = {false};

  private:
    static void static_socket_process(AMIPCClientUDS *p);
    void socket_process();
    AM_RESULT construct();
    AMIPCClientUDS();
    AMIPCClientUDS(AMIPCClientUDS const &copy) = delete;
    AMIPCClientUDS& operator=(AMIPCClientUDS const &copy) = delete;
    virtual ~AMIPCClientUDS();
};

#endif /* ORYX_INCLUDE_IPC_AM_IPC_UDS_H_ */
