/*******************************************************************************
 * am_avqueue.h
 *
 * History:
 *   Sep 21, 2016 - [Shupeng Ren] created file
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
#ifndef ORYX_STREAM_RECORD_FILTERS_LIBAVQUEUE_AM_AVQUEUE_H_
#define ORYX_STREAM_RECORD_FILTERS_LIBAVQUEUE_AM_AVQUEUE_H_

#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <condition_variable>

#include "am_sei_define.h"
#include "am_audio_define.h"
#include "am_muxer_codec_info.h"
#include "am_event.h"
#include "am_mutex.h"
#include "am_queue.h"
using std::map;
using std::pair;
using std::vector;

struct ExPayload;
class AMAVQueue;
class AMRingQueue;
class AMAVQueuePacketPool;
struct AMEventStruct;
struct AM_VIDEO_INFO;

struct AVQueueConfig
{
    uint32_t      pkt_pool_size                     = 0;
    uint32_t      video_id                          = 0;
    //map<audio_type, vector<sample_rate>>
    map<AM_AUDIO_TYPE, vector<uint32_t>> audio_types;
    bool          event_enable                      = false;
    bool          event_gsensor_enable              = false;
    //pair<audio_type, sample_rate>
    pair<AM_AUDIO_TYPE, uint32_t> event_audio       = {AM_AUDIO_AAC, 48000};
    uint32_t      event_max_history_duration        = 0;
};

enum AM_AVQUEUE_MEDIA_TYPE{
  AM_AVQUEUE_MEDIA_NULL = -1,
  AM_AVQUEUE_MEDIA_VIDEO,
  AM_AVQUEUE_MEDIA_AUDIO,
  AM_AVQUEUE_MEDIA_GSENSOR,
};

typedef std::function<void(AMPacket *packet)> recv_cb_t;

typedef std::shared_ptr<AMAVQueue> AMAVQueuePtr;
class AMAVQueue final
{
    friend AMAVQueuePacketPool;
    typedef AMSafeDeque<AMPacket*> packet_queue;
  public:
    static AMAVQueuePtr create(AVQueueConfig &config,
                               std::string name,
                               bool start_send_normal_pkt,
                               recv_cb_t callback);

  public:
    AM_STATE process_packet(AMPacket *packet);
    bool is_ready_for_event(AMEventStruct& event);
    bool is_sending_normal_pkt();
    bool start_send_normal_pkt();
  private:
    inline AM_STATE on_info(AMPacket *packet);
    inline AM_STATE on_data(AMPacket *packet);
    inline AM_STATE on_event(AMPacket *packet);
    inline AM_STATE on_eos(AMPacket *packet);

    inline AM_STATE send_normal_packet();
    inline AM_STATE send_event_packet();
    inline AM_STATE send_event_info();
    inline AM_STATE send_normal_info(AM_AVQUEUE_MEDIA_TYPE type, uint32_t stream_id);
    AM_STATE process_info_pkt(AMPacket *packet);
    void release_packet(AMPacket *packet);
    static void     static_send_normal_packet_thread(void *p);
    static void     static_send_event_packet_thread(void *p);
    void            send_normal_packet_thread();
    void            send_event_packet_thread();
    bool is_h265_IDR_first_nalu(ExPayload *video_payload);
    inline AM_STATE process_h26x_event(AMPacket *packet);
  private:
    AMAVQueue();
    ~AMAVQueue();
    AM_STATE construct(AVQueueConfig &config,
                       std::string name,
                       bool start_send_normal_pkt,
                       recv_cb_t callback);

  private:
    AMEvent                            *m_event_wait             = nullptr;
    AMEvent                            *m_send_normal_pkt_wait   = nullptr;
    AMEvent                            *m_info_pkt_process_event = nullptr;
    AMThread                           *m_send_normal_pkt_thread = nullptr;
    AMThread                           *m_send_event_pkt_thread  = nullptr;
    AMAVQueuePacketPool                *m_packet_pool            = nullptr;
    recv_cb_t                           m_recv_cb                = nullptr;
    AM_PTS                              m_event_end_pts          = 0;
    uint32_t                            m_run                    = 0;
    uint32_t                            m_audio_pts_increment    = 0;
    uint32_t                            m_gsensor_pts_increment  = 0;
    uint32_t                            m_event_video_id         = 0;
    uint32_t                            m_event_audio_id         = 0;
    uint32_t                            m_event_gsensor_id       = 0;
    bool                                m_event_video_eos        = false;
    bool                                m_event_video_block      = false;
    bool                                m_event_audio_block      = false;
    bool                                m_event_gsensor_block    = false;
    std::atomic_bool                    m_event_stop             = {false};
    std::atomic_bool                    m_is_sending_normal_pkt  = {true};
    std::atomic_bool                    m_in_event               = {false};
    std::atomic_bool                    m_stop                   = {false};
    std::atomic_bool                    m_video_come             = {false};
    AVQueueConfig                       m_config;
    std::mutex                          m_send_mutex;
    std::condition_variable             m_send_cond;
    map<uint32_t, AM_PTS>               m_audio_last_pts;
    map<uint32_t, AM_PTS>               m_gsensor_last_pts;
    map<uint32_t, uint32_t>             m_send_audio_state;
    map<uint32_t, uint32_t>             m_send_gsensor_state;
    map<uint32_t, AM_VIDEO_INFO>        m_video_info;
    map<uint32_t, AM_AUDIO_INFO>        m_audio_info;
    map<uint32_t, AM_GSENSOR_INFO>      m_gsensor_info;
    map<uint32_t, AMRingQueue*>         m_video_queue;
    map<uint32_t, AMRingQueue*>         m_audio_queue;
    map<uint32_t, AMRingQueue*>         m_gsensor_queue;
    vector<uint32_t>                    m_audio_ids;
    map<uint32_t, pair<bool, AM_PTS>>   m_first_video;
    map<uint32_t, pair<bool, AM_PTS>>   m_first_audio;
    map<uint32_t, pair<bool, AM_PTS>>   m_first_gsensor;
    /*map<stream_id, info_sent>*/
    map<uint32_t, bool>                 m_video_info_sent;
    map<uint32_t, bool>                 m_audio_info_sent;
    map<uint32_t, bool>                 m_gsensor_info_sent;
    map<uint32_t, uint32_t>             m_write_video_state;
    map<uint32_t, uint32_t>             m_write_audio_state;
    map<uint32_t, uint32_t>             m_write_gsensor_state;
    pair<uint32_t, bool>                m_video_send_block = {0, false};
    pair<uint32_t, bool>                m_audio_send_block = {0, false};
    pair<uint32_t, bool>                m_gsensor_send_block = {0, false};
    std::mutex                          m_event_send_mutex;
    std::condition_variable             m_event_send_cond;
    map<uint32_t, AMPacket::Payload>    m_video_info_payload;
    map<uint32_t, AMPacket::Payload>    m_audio_info_payload;
    map<uint32_t, AMPacket::Payload>    m_gsensor_info_payload;
    packet_queue                        m_info_pkt_queue;
    std::string                         m_name;
};

class AMAVQueuePacketPool final: public AMSimplePacketPool
{
    typedef AMSimplePacketPool inherited;

  public:
    static AMAVQueuePacketPool* create(AMAVQueue *q,
                                       const char *name,
                                       uint32_t count)
    {
      AMAVQueuePacketPool *result = new AMAVQueuePacketPool();
      if (result && (AM_STATE_OK != result->init(q, name, count))) {
        delete result;
        result = NULL;
      }
      return result;
    }

    bool alloc_packet(AMPacket*& packet, uint32_t size = 0) override
    {
      return AMSimplePacketPool::alloc_packet(packet, size);
    }

  public:
    void on_release_packet(AMPacket *packet) override
    {
      if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA ||
          packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOS) {
        m_avqueue->release_packet(packet);
      }
    }

  private:
    AMAVQueuePacketPool(){};
    virtual ~AMAVQueuePacketPool(){};
    AM_STATE init(AMAVQueue *q, const char *name, uint32_t count)
    {
      AM_STATE state = inherited::init(name, count, sizeof(AMPacket));
      if (AM_STATE_OK == state) {
        m_avqueue = q;
      }
      return state;
    }

  private:
    AMAVQueue           *m_avqueue = nullptr;
    AMPacket::Payload   *m_payload = nullptr;
};

#endif /* ORYX_STREAM_RECORD_FILTERS_LIBAVQUEUE_AM_AVQUEUE_H_ */
