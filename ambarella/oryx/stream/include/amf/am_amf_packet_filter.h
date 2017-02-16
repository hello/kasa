/*******************************************************************************
 * am_amf_packet_filter.h
 *
 * History:
 *   2014-7-24 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AMF_PACKET_FILTER_H_
#define AM_AMF_PACKET_FILTER_H_

#include <atomic>
/*
 * AMPacketFilter
 */
class AMPacketFilter: public AMObject, public AMIPacketFilter
{
    typedef AMObject inherited;

  public:
    virtual void* get_interface(AM_REFIID refiid);
    virtual void destroy();
    virtual void add_ref();

  public:
    AM_STATE post_engine_msg(AmMsg& msg);
    AM_STATE post_engine_msg(uint32_t code);

  protected:
    AMPacketFilter(AMIEngine *engine);
    virtual ~AMPacketFilter();
    AM_STATE init(const char *name);

  protected:
    AMIEngine       *m_engine;
    const char      *m_name;
    std::atomic_uint m_ref;
};

/*
 * AMPacketActiveFilter
 */
class AMPacketQueueInputPin;
class AMPacketActiveFilter: public AMPacketFilter, public AMIActiveObject
{
    typedef AMPacketFilter inherited;
  public:
    virtual void* get_interface(AM_REFIID refiid);
    virtual void destroy();
    virtual AM_STATE run();
    virtual AM_STATE stop();
    virtual void get_info(INFO& info);
    virtual const char* get_name();
    virtual void on_cmd(CMD& cmd){} /* Should be implemented in sub-class */

  protected:
    bool wait_input_packet(AMPacketQueueInputPin*& pin, AMPacket*& packet);
    /* Should be implemented in sub-class */
    virtual bool process_cmd(CMD& cmd) {return false;}
    /* Should be implemented in sub-class */
    virtual void on_stop(){}

  protected:
    AMQueue* msgQ();
    void get_cmd(AMIActiveObject::CMD& cmd);
    bool peek_cmd(AMIActiveObject::CMD& cmd);
    bool peek_msg(AmMsg& msg);
    void ack(AM_STATE result);

  protected:
    AMPacketActiveFilter(AMIEngine *engine);
    virtual ~AMPacketActiveFilter();
    AM_STATE init(const char *name, bool RTPrio = false, int prio = 90);

  protected:
    AMWorkQueue *m_work_q;
};

#endif /* AM_AMF_PACKET_FILTER_H_ */
