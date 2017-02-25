/*******************************************************************************
 * am_export_client_uds.h
 *
 * History:
 *   2015-01-04 - [Zhi He] created file
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

#ifndef __AM_EXPORT_CLIENT_UDS_H__
#define __AM_EXPORT_CLIENT_UDS_H__

#include <list>
#include <sys/un.h>
#include <atomic>

using std::list;
class AMExportClientUDS: public AMIExportClient
{
  protected:
    AMExportClientUDS();
    AMExportClientUDS(AMExportConfig *config);
    virtual ~AMExportClientUDS();

  public:
    static AMExportClientUDS* create(AMExportConfig *config);
    virtual void destroy()                          override;

  public:
    virtual AM_RESULT connect_server(const char *url)  override;
    virtual void disconnect_server()                override;
    virtual AM_RESULT receive(AMExportPacket *packet)  override;
    virtual AM_RESULT set_config(AMExportConfig *config) override;
    virtual void release(AMExportPacket *packet)    override;

  public:
    AM_RESULT init();
  private:
    static void thread_entry(void *arg);
    bool get_data();
    bool check_sort_mode();

  private:
    AMEvent             *m_event = nullptr;
    AMMutex             *m_mutex = nullptr;
    AMCondition         *m_cond = nullptr;
    AMThread            *m_recv_thread = nullptr;
    int                  m_iav_fd = -1;
    int                  m_socket_fd = -1;
    int                  m_max_fd = -1;
    int                  m_control_fd[2] = {-1, -1};
    std::atomic_bool     m_block = {false};
    std::atomic_bool     m_is_connected = {false};
    std::atomic_bool     m_run = {false};
    fd_set               m_all_set;
    fd_set               m_read_set;
    list<AMExportPacket> m_packet_queue;
    AMExportConfig       m_config = {0};
    AMIVideoAddressPtr   m_video_address = nullptr;
    sockaddr_un          m_addr = {0};
};

#endif
