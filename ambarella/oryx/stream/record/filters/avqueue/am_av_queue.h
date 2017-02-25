/**
 * am_av_queue.h
 *
 *  History:
 *		Dec 29, 2014 - [Shupeng Ren] created file
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
 */
#ifndef _AM_AV_QUEUE_H_
#define _AM_AV_QUEUE_H_
#include <atomic>
#include "am_av_queue_if.h"
#include "am_audio_define.h"
#include <atomic>

class AMPlugin;
class AMAVQueueInput;
class AMAVQueueOutput;
class AMAVQueueConfig;
class AMAVQueuePacketPool;
struct AVQueueConfig;
struct ExPayload;

using std::map;
using std::pair;
using std::mutex;
using std::vector;

class AMAVQueue: public AMPacketActiveFilter, public AMIAVQueue
{
    typedef AMPacketActiveFilter inherited;
    friend AMAVQueueInput;

  public:
    static AMIAVQueue* create(AMIEngine *engine,
                              const std::string& config,
                              uint32_t input_num,
                              uint32_t output_num);
    void release_packet(AMPacket *packet);

  public:
    virtual uint32_t        version()                                 override;
    virtual void*           get_interface(AM_REFIID refiid)           override;
    virtual void            destroy()                                 override;
    virtual void            get_info(INFO& info)                      override;
    virtual AMIPacketPin*   get_input_pin(uint32_t index)             override;
    virtual AMIPacketPin*   get_output_pin(uint32_t index)            override;
    virtual void            on_run()                                  override;
    virtual AM_STATE        stop()                                    override;
    virtual bool            is_ready_for_event(AMEventStruct& event)  override;

  private:
    inline AM_STATE on_info(AMPacket *packet);
    inline AM_STATE on_data(AMPacket *packet);
    inline AM_STATE on_event(AMPacket *packet);
    inline AM_STATE on_eos(AMPacket *packet);
    inline AM_STATE process_packet(AMPacket *packet);
    inline AM_STATE send_normal_packet();
    inline AM_STATE send_event_packet();
    inline AM_STATE send_event_info();
    inline void     process_event();
    static void     static_event_thread(void *p);
    void            event_thread();
    bool is_h265_IDR_first_nalu(ExPayload *video_payload);

  private:
    AMAVQueue(AMIEngine *engine);
    virtual ~AMAVQueue();
    AM_STATE init(const std::string& config,
                  uint32_t input_num,
                  uint32_t output_num);
    AM_STATE check_output_pins();
    void reset();

  private:
    AM_PTS                          m_event_end_pts;
    AVQueueConfig                  *m_avqueue_config;
    AMAVQueueConfig                *m_config;
    AMAVQueueOutput                *m_direct_out; //No need to delete
    AMAVQueueOutput                *m_file_out;   //No need to delete
    AMMutex                        *m_send_mutex;
    AMCondition                    *m_send_cond;
    AMThread                       *m_event_thread;
    AMEvent                        *m_event_wait;
    AMMutex                        *m_event_send_mutex;
    AMCondition                    *m_event_send_cond;
    uint32_t                        m_input_num;
    uint32_t                        m_output_num;
    uint32_t                        m_run;
    uint32_t                        m_audio_pts_increment;
    uint32_t                        m_video_payload_count;
    uint32_t                        m_audio_payload_count;
    uint32_t                        m_event_id;
    uint32_t                        m_event_video_id;
    uint32_t                        m_event_audio_id;
    bool                            m_is_in_event;
    bool                            m_event_video_eos;
    bool                            m_event_video_block;
    bool                            m_event_audio_block;
    std::atomic_bool                m_stop;
    std::atomic_bool                m_video_come;
    vector<AMAVQueueInput*>         m_input;
    vector<AMAVQueueOutput*>        m_output;
    vector<AMAVQueuePacketPool*>    m_packet_pool;
    map<uint32_t, AM_PTS>           m_audio_last_pts;
    map<uint32_t, uint32_t>         m_send_audio_state;
    map<uint32_t, AM_VIDEO_INFO>    m_video_info;
    map<uint32_t, AM_AUDIO_INFO>    m_audio_info;
    map<uint32_t, AMRingQueue*>     m_video_queue;
    map<uint32_t, AMRingQueue*>     m_audio_queue;

    map<uint32_t, pair<bool, AM_PTS>>   m_first_video;
    map<uint32_t, pair<bool, AM_PTS>>   m_first_audio;
    map<uint32_t, uint32_t>             m_write_video_state;
    map<uint32_t, uint32_t>             m_write_audio_state;
    pair<uint32_t, bool>                m_video_send_block;
    pair<uint32_t, bool>                m_audio_send_block;
    //For Event
    AVQueueEventConfigMap               m_event_config;
    map<uint32_t, uint32_t>             m_event_audio_id_map;
    map<uint32_t, AMPacket::Payload*>   m_video_info_payload;
    map<uint32_t, AMPacket::Payload*>   m_audio_info_payload;
};

class AMAVQueueInput: public AMPacketActiveInputPin
{
    typedef AMPacketActiveInputPin inherited;
    friend AMAVQueue;

  public:
    static AMAVQueueInput* create(AMPacketFilter *filter, uint32_t num)
    {
      AMAVQueueInput *result = new AMAVQueueInput(filter, num);
      if (AM_UNLIKELY(result && (AM_STATE_OK != result->init()))) {
        delete result;
        result = nullptr;
      }
      return result;
    }

  private:
    AMAVQueueInput(AMPacketFilter *filter, uint32_t num) :
      inherited(filter),
      m_num(num)
    {}
    virtual ~AMAVQueueInput() {}
    AM_STATE process_packet(AMPacket *packet) override;
    AM_STATE init()
    {
      char name[32];
      sprintf(name, "AVQInputPin[%d]", m_num);
      return inherited::init(name);
    }

  private:
    uint32_t m_num;
};

class AMAVQueueOutput: public AMPacketOutputPin
{
    typedef AMPacketOutputPin inherited;
    friend AMAVQueue;

  public:
    static AMAVQueueOutput* create(AMPacketFilter *filter)
    {
      AMAVQueueOutput *result = new AMAVQueueOutput(filter);
      if (AM_UNLIKELY(result && (AM_STATE_OK != result->init()))) {
        delete result;
        result = NULL;
      }
      return result;
    }

  private:
    AMAVQueueOutput(AMPacketFilter *filter) :
      inherited(filter)
    {}
    virtual ~AMAVQueueOutput(){}
    AM_STATE init()
    {
      return AM_STATE_OK;
    }
};

class AMAVQueuePacketPool: public AMSimplePacketPool
{
    typedef AMSimplePacketPool inherited;
    friend AMAVQueue;

  public:
    static AMAVQueuePacketPool* create(AMAVQueue *q,
                                       const char *name,
                                       uint32_t count);

  public:
    void on_release_packet(AMPacket *packet) override;

  private:
    AMAVQueuePacketPool();
    virtual ~AMAVQueuePacketPool();
    AM_STATE init(AMAVQueue *q, const char *name, uint32_t count);

  private:
    AMAVQueue           *m_avqueue;
    AMPacket::Payload   *m_payload;
};

#endif /* _AM_AV_QUEUE_H_ */
