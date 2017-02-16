/*******************************************************************************
 * am_amf_queue.h
 *
 * History:
 *   2014-7-22 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AMF_QUEUE_H_
#define AM_AMF_QUEUE_H_

class AMMutex;
class AMCondition;

class AMQueue
{
  public:
    enum QTYPE
    {
      AM_Q_MSG,
      AM_Q_DATA
    };
    struct WaitResult
    {
        AMQueue *dataQ;
        void    *owner;
        uint32_t block_size;
    };
  public:
    static AMQueue* create(AMQueue *mainQ,
                           void    *owner,
                           uint32_t blockSize,
                           uint32_t reservedSlots);
    void destroy();

  public:
    AM_STATE post_msg(const void *msg, uint32_t msgSize);
    AM_STATE send_msg(const void *msg, uint32_t msgSize);

    void get_msg(void *msg, uint32_t msgSize);
    bool get_msg_non_block(void *msg, uint32_t msgSize);
    bool peek_msg(void *msg, uint32_t msgSize);
    void reply(AM_STATE result);

    void enable(bool enabled = true);
    bool is_enable();

    AM_STATE put_data(const void *buffer, uint32_t size);

    QTYPE wait_data_msg(void *msg, uint32_t msgSize, WaitResult *result);
    bool peek_data(void *buffer, uint32_t size);
    uint32_t get_available_data_number();

  public:
    bool is_main();
    bool is_sub();

  private:
    AMQueue(AMQueue *mainQ, void *owner);
    virtual ~AMQueue();
    AM_STATE init(uint32_t blockSize, uint32_t reservedSlots);

  private:
    struct List
    {
      List *next;
      bool allocated;
      void destroy();
    };

  private:
    void copy(void *to, const void *from, uint32_t bytes);
    List* alloc_node();
    void write_data(List *node, const void *buffer, uint32_t size);
    void read_data(void *buffer, uint32_t size);
    void wakeup_any_reader();

  private:
    bool         m_is_disabled;
    uint32_t     m_num_get;
    uint32_t     m_num_send_msg;
    uint32_t     m_num_data;
    uint32_t     m_block_size;
    uint8_t     *m_reserved_mem;
    AM_STATE    *m_msg_result;
    void        *m_owner;
    List        *m_head;
    List        *m_tail;
    List        *m_free_list;
    List        *m_send_buffer;
    AMQueue     *m_main_q;
    AMQueue     *m_prev_q;
    AMQueue     *m_next_q;
    AMMutex     *m_mutex;
    AMCondition *m_cond_reply;
    AMCondition *m_cond_get;
    AMCondition *m_cond_send_msg;
};

#endif /* AM_AMF_QUEUE_H_ */
