/**
 * am_av_queue_builder.h
 *
 *  History:
 *		Dec 31, 2014 - [Shupeng Ren] created file
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
#ifndef _AM_AV_QUEUE_BUILDER_H_
#define _AM_AV_QUEUE_BUILDER_H_

struct ExPayload: public AMPacket::Payload
{
  int32_t           m_normal_ref;
  int32_t           m_event_ref;
  explicit ExPayload():
    Payload(),
    m_normal_ref(0),
    m_event_ref(0)
  {}
  void operator=(const AMPacket::Payload& payload)
  {
    m_header = payload.m_header;
    m_data = payload.m_data;
    m_normal_ref = 0;
    m_event_ref = 0;
  }
  virtual ~ExPayload(){}
};

enum class queue_mode {normal = 0, event = 1};

class AMRingQueue
{
  public:
    static AMRingQueue* create(uint32_t count);
    void destroy();

  public:
    bool write(const AMPacket::Payload *payload, queue_mode mode);
    ExPayload* get();
    ExPayload* event_get();
    ExPayload* event_get_prev();

    void read_pos_inc(queue_mode mode);
    void event_backtrack();
    void normal_release(ExPayload *payload);
    void event_release(ExPayload *payload);

    void reset();
    void event_reset();

    bool in_use();
    bool normal_in_use();
    bool event_in_use();

    bool full(queue_mode mode);
    bool empty(queue_mode mode);
    bool event_empty();
    bool about_to_full(queue_mode mode);
    bool about_to_empty(queue_mode mode);

    int32_t get_free_payload_num();

  private:
    AMRingQueue();
    AM_STATE construct(uint32_t count);
    ~AMRingQueue();

  private:
    ExPayload          *m_payload;
    int32_t             m_payload_num;

    int32_t             m_read_pos;
    int32_t             m_read_pos_e;
    int32_t             m_write_pos;
    int32_t             m_write_count;

    int32_t             m_in_use_payload_num;
    int32_t             m_in_use_payload_num_e;
    int32_t             m_readable_payload_num;
    int32_t             m_readable_payload_num_e;

    std::mutex          m_mutex;
};

#endif /* _AM_AV_QUEUE_BUILDER_H_ */
