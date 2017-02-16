/**
 * am_av_queue_builder.h
 *
 *  History:
 *		Dec 31, 2014 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _AM_AV_QUEUE_BUILDER_H_
#define _AM_AV_QUEUE_BUILDER_H_

struct ExPayload: public AMPacket::Payload
{
  bool              m_normal_use;
  bool              m_event_use;
  uint8_t          *m_mem_end;
  std::atomic_int   m_ref;
  explicit ExPayload():
    Payload(),
    m_normal_use(false),
    m_event_use(false),
    m_mem_end(nullptr),
    m_ref(0)
  {}
  void operator=(const AMPacket::Payload& payload)
  {
    memcpy(&m_header, &(payload.m_header), sizeof(m_header));
    memcpy(&m_data, &(payload.m_data), sizeof(m_data));
    m_normal_use = false;
    m_event_use = false;
    m_ref = 0;
  }
  virtual ~ExPayload(){}
};

enum class queue_mode {
  normal = 0,
  event = 1
};

class AMRingQueue
{
  public:
    static AMRingQueue* create(uint32_t count, uint32_t size);
    void destroy();

  public:
    void write(AMPacket::Payload *payload, queue_mode mode);
    ExPayload* get();
    ExPayload* event_get();
    ExPayload* event_get_prev();

    void read_pos_inc(queue_mode mode);
    void event_backtrack();
    void release(ExPayload *payload);
    void drop();

    void reset();
    void event_reset();

    bool is_in_use();
    bool is_full(queue_mode mode, uint32_t payload_size = 0);
    bool is_empty(queue_mode mode);
    bool about_to_full(queue_mode mode);
    bool about_to_empty(queue_mode mode);
    bool is_event_sync();

    uint32_t get_free_mem_size();
    uint32_t get_free_payload_num();

  private:
    AMRingQueue();
    AM_STATE construct(uint32_t count, uint32_t size);
    ~AMRingQueue();

  private:
    uint8_t            *m_mem;
    uint8_t            *m_mem_end;
    uint8_t            *m_current;
    int32_t             m_free_mem;
    int32_t             m_reserved_mem_size;
    int32_t             m_datasize;
    int32_t             m_read_pos;
    int32_t             m_read_pos_e;
    int32_t             m_write_pos;
    ExPayload          *m_payload;
    int32_t             m_payload_num;

    int32_t             m_in_use_payload_num;
    int32_t             m_in_use_payload_num_e;
    int32_t             m_readable_payload_num;
    int32_t             m_readable_payload_num_e;

    std::mutex          m_mutex;
};

#endif /* _AM_AV_QUEUE_BUILDER_H_ */
