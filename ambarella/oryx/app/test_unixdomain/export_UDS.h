/*******************************************************************************
 * export_UDS.h
 *
 * History:
 *   2016-07-18 - [Guohua Zheng] created file
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

#ifndef __AM_MUXER_EXPORT_UDS_H__
#define __AM_MUXER_EXPORT_UDS_H__
#include <map>
#include <queue>
#include <list>
#include <memory>
#include <condition_variable>

#include "am_event.h"
#include "am_thread.h"
#include "am_amf_types.h"

#include <sys/un.h>

#define DMAX_CACHED_PACKET_NUMBER 64

enum
{
  EExportState_not_inited           = 0x0,
  EExportState_no_client_connected  = 0x1,
  EExportState_running              = 0x2,
  EExportState_error                = 0x3,
  EExportState_halt                 = 0x4,
};

using std::map;
using std::queue;
using std::pair;
using std::list;


class AMMuxerExportUDS
{
  public:
    static AMMuxerExportUDS* create();
    void destroy();

  public:
    virtual AM_STATE start()                ;
    virtual AM_STATE stop()                 ;
    virtual bool is_running()               ;
  public:
    AMMuxerExportUDS();
    virtual ~AMMuxerExportUDS();

  public:
    bool init();
    bool send_info(int client_fd);
    bool send_packet(int client_fd);
    void monitor_loop();
    void data_export_loop();
    void clean_resource();
    void reset_resource();
    static void thread_entry(void *arg);
    static void thread_monitor_entry(void *arg);
    static void thread_export_entry(void *arg);

  private:
    AMEvent       *m_thread_monitor_wait = nullptr;
    AMEvent       *m_thread_export_wait  = nullptr;
    AMThread      *m_monitor_thread      = nullptr;
    AMThread      *m_export_thread       = nullptr;
    int            m_export_state        = EExportState_not_inited;
    int            m_max_fd              = -1;
    int            m_max_clients_fd      = -1;
    int            m_socket_fd           = -1;
    int            m_connect_fd          = -1;
    uint32_t       m_video_map           = 0;
    uint32_t       m_audio_map           = 0;
    uint32_t       m_audio_pts_increment = 0;
    int            m_control_fd[2]       = {-1, -1};
    bool           m_running             = false;
    bool           m_monitor_running     = false;
    bool           m_export_running      = false;
    bool           m_thread_exit         = false;
    bool           m_thread_monitor_exit = false;
    bool           m_thread_export_exit  = false;
    bool           m_client_connected    = false;
    bool           m_send_block          = false;
    volatile bool  m_export_send_block   = false;
    fd_set         m_all_set;
    fd_set         m_read_set;
    fd_set         m_clients_set;
    list<struct pollfd> m_clients_fds;
    sockaddr_un    m_addr                = {0};
    std::mutex     m_send_mutex;
    std::mutex     m_clients_mutex;
    std::condition_variable           m_send_cond;
    std::condition_variable           m_export_cond;
};
#endif
