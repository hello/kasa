/*******************************************************************************
 * am_muxer_export_uds.h
 *
 * History:
 *   2015-01-04 - [Zhi He] created file
 *   2016-07-26 - [Guohua Zheng] modified file
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
#include <memory>
#include <condition_variable>
#include <atomic>
#include "am_muxer_codec_info.h"
#include "am_queue.h"

#define DMAX_CACHED_PACKET_NUMBER 64
#define MAX_EPOLL_FDS 10
#define MAX_CLIENTS_NUM 5

enum ExportAudioState
{
  EExportAudioState_Normal = 0, // Normal
  EExportAudioState_Try    = 1, // Try to get audio
  EExportAudioState_Wait   = 2, // Need block to wait for audio packet
};

typedef AMSafeQueue<AMPacket*>                AMPacketQueue;
typedef std::map<uint32_t, AMPacketQueue>     AMPacketQueueMap;
typedef std::map<uint32_t, AMExportConfig>    AMExportConfigMap;
typedef std::map<uint32_t, AM_VIDEO_INFO>     VideoInfoMap;
typedef std::map<uint32_t, AM_AUDIO_INFO>     AudioInfoMap;
typedef std::map<uint32_t, AMExportVideoInfo> AMExportVideoInfoMap;
typedef std::map<uint32_t, AMExportAudioInfo> AMExportAudioInfoMap;
typedef std::map<uint32_t, AMExportPacket>    AMExportPacketMap;
typedef std::map<uint32_t, bool>              AMFlagMap;
typedef std::pair<uint32_t, bool>             AMFlagpair;
typedef std::map<uint32_t, AM_PTS>            AMPtsMap;
typedef std::map<uint32_t, ExportAudioState>  AMAudioStateMap;
typedef std::map<uint32_t, uint32_t>          AudioIncrementPts;

class AMMuxerExportUDS final: public AMIMuxerCodec
{
  public:
    static AMMuxerExportUDS* create();
    void destroy();

  public:
    virtual AM_STATE start()                                           override;
    virtual AM_STATE stop()                                            override;
    virtual const char* name()                                         override;
    virtual void* get_muxer_codec_interface(AM_REFIID refiid)          override;
    virtual bool is_running()                                          override;
    virtual AM_STATE set_config(AMMuxerCodecConfig *config)            override;
    virtual AM_STATE get_config(AMMuxerCodecConfig *config)            override;
    virtual AM_MUXER_ATTR get_muxer_attr()                             override;
    virtual uint8_t get_muxer_codec_stream_id()                        override;
    virtual uint32_t get_muxer_id()                                    override;
    virtual AM_MUXER_CODEC_STATE get_state()                           override;
    virtual void feed_data(AMPacket *packet)                           override;

  protected:
    AMMuxerExportUDS();
    virtual ~AMMuxerExportUDS();

  private:
    bool init();
    static void thread_export_entry(void *arg);
    void save_info(AMPacket *packet);
    void data_export_loop();
    bool fill_export_packet(int client_fd, AMPacket *packet_fill,
                            AMExportPacket *export_packet);
    bool send_info(int client_fd);
    AMPacket* get_export_packet();
    bool send_packet(int client_fd, AMPacket *packet_send);
    bool read_config(int fd, AMExportConfig *m_config);
    void clean_resource();
    void reset_resource();

  private:
    AMEvent                *m_thread_export_wait  = nullptr;
    AMThread               *m_export_thread       = nullptr;
    int                     m_socket_fd           = -1;
    int                     m_connect_fd          = -1;
    int                     m_epoll_fd            = -1;
    uint32_t                m_video_map           = 0;
    uint32_t                m_audio_map           = 0;
    int                     m_control_fd[2]       = {-1, -1};
    std::atomic<bool>       m_running             = {false};
    std::atomic<bool>       m_export_running      = {false};
    std::atomic<bool>       m_thread_exit         = {false};
    std::atomic<bool>       m_thread_export_exit  = {false};
    std::atomic<bool>       m_client_connected    = {false};
    std::atomic<bool>       m_send_block          = {false};
    std::atomic<int>        m_connect_num         = {0};
    std::condition_variable m_send_cond;
    std::mutex              m_send_mutex;
    std::mutex              m_clients_mutex;
    AMExportConfigMap       m_config_map;
    AMPacketQueue           m_packet_queue;
    AMPacketQueueMap        m_video_queue;
    AMPacketQueueMap        m_audio_queue;
    VideoInfoMap            m_video_infos;
    AudioInfoMap            m_audio_infos;
    AMExportVideoInfoMap    m_video_export_infos;
    AMExportAudioInfoMap    m_audio_export_infos;
    AMExportPacketMap       m_video_export_packets;
    AMExportPacketMap       m_audio_export_packets;
    AMFlagMap               m_video_info_send_flag;
    AMFlagMap               m_audio_info_send_flag;
    AMFlagMap               m_client_info_send_flag;
    AMFlagMap               m_export_update_send_flag;
    AMFlagpair              m_video_send_block  = {0, false};
    AMFlagpair              m_audio_send_block  = {0, false};
    AMFlagpair              m_sort_mode = {0, false};
    AMPtsMap                m_audio_last_pts;
    AM_PTS                  m_predict_pts = INT64_MAX;
    AMAudioStateMap         m_audio_state;
    AMIVideoReaderPtr       m_video_reader = nullptr;
    AudioIncrementPts       m_audio_pts_increment;
};
#endif
