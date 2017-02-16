/*******************************************************************************
 * am_amf_packet_pin.h
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
#ifndef AM_AMF_PACKET_PIN_H_
#define AM_AMF_PACKET_PIN_H_

/*
 * AMPacketPin
 */
class AMPacketPin: public AMObject, public AMIPacketPin
{
    typedef AMObject inherited;
  public:
    virtual void* get_interface(AM_REFIID refiid);
    virtual void destroy();

    virtual AM_STATE connect(AMIPacketPin *peer);
    virtual void disconnect();
    virtual void receive(AMPacket *packet);
    virtual void purge();
    virtual void enable(bool enabled);
    virtual bool is_enable();
    virtual AMIPacketPin* get_peer();
    virtual AMIPacketFilter* get_filter();
    virtual bool is_connected();
    virtual const char* filter_name();

  protected:
    virtual AM_STATE on_connect(AMIPacketPin *peer) = 0;
    virtual void on_disconnect();
    void set_packet_pool(AMIPacketPool *packetPool);
    void release_packet_pool();

  protected:
    AM_STATE post_engine_msg(AmMsg& msg);
    AM_STATE post_engine_msg(uint32_t code);
    AM_STATE post_engine_error_msg(AM_STATE result);

  protected:
    AMPacketPin(AMPacketFilter *filter);
    virtual ~AMPacketPin();

  protected:
    bool            m_is_external_packet_pool;
    AMPacketFilter *m_filter;
    AMIPacketPin   *m_peer;
    AMIPacketPool  *m_packet_pool;
};

/*
 * AMPacketInputPin
 */
class AMPacketInputPin: public AMPacketPin
{
    typedef AMPacketPin inherited;
  public:
    virtual AM_STATE connect(AMIPacketPin *peer);
    virtual void disconnect();

  protected:
    virtual AM_STATE on_connect(AMIPacketPin *peer);

  protected:
    AMPacketInputPin(AMPacketFilter *filter) :
      inherited(filter)
    {}
    ~AMPacketInputPin(){}
};

/*
 * AMPacketQueueInputPin
 */
class AMPacketQueueInputPin: public AMPacketInputPin
{
    typedef AMPacketInputPin inherited;
    enum INPUT_PIN_QUEUE_LENGTH {
      INPUT_PIN_Q_LEN = 16
    };
  public:
    virtual void receive(AMPacket *packet);
    virtual void purge();

  public:
    bool peek_packet(AMPacket*& packet);
    virtual void enable(bool enabled);
    uint32_t get_avail_packet_num();

  protected:
    AMPacketQueueInputPin(AMPacketFilter *filter);
    virtual ~AMPacketQueueInputPin();
    AM_STATE init(AMQueue *msgQ);

  protected:
    AMQueue* m_packet_q;
};

/*
 * AMPacketActiveInputPin
 */
class AMPacketActiveInputPin: public AMPacketQueueInputPin,
                              public AMIActiveObject
{
    typedef AMPacketQueueInputPin inherited;
  public:
    virtual void* get_interface(AM_REFIID refiid);
    virtual void destroy();
    virtual const char* get_name();
    virtual void on_run();
    virtual void on_cmd(CMD& cmd){}

  public:
    AM_STATE run();
    AM_STATE stop();

  protected:
    AMQueue* msgQ();
    void get_cmd(AMIActiveObject::CMD& cmd);
    bool peek_cmd(AMIActiveObject::CMD& cmd);
    void ack(AM_STATE result);

  protected:
    virtual AM_STATE process_packet(AMPacket *packet) = 0;

  protected:
    AMPacketActiveInputPin(AMPacketFilter *filter);
    virtual ~AMPacketActiveInputPin();
    AM_STATE init(const char *name);

  protected:
    AMWorkQueue *m_work_q;
    const char  *m_name;
};

/*
 * AMPacketOutputPin
 */
class AMPacketOutputPin: public AMPacketPin
{
    typedef AMPacketPin inherited;
  public:
    virtual AM_STATE connect(AMIPacketPin *peer);
    virtual void disconnect();

  public:
    bool alloc_packet(AMPacket*& packet, uint32_t size = 0);
    void send_packet(AMPacket *packet);
    uint32_t get_avail_packet_num();

  protected:
    virtual AM_STATE on_connect(AMIPacketPin *peer);

  protected:
    AMPacketOutputPin(AMPacketFilter *filter) :
      inherited(filter)
    {}
    virtual ~AMPacketOutputPin(){}
};

/*
 * AMPacketActiveOutputPin
 */
class AMPacketActiveOutputPin: public AMPacketOutputPin, public AMIActiveObject
{
    typedef AMPacketOutputPin inherited;

  public:
    virtual void* get_interface(AM_REFIID refiid);
    virtual void destroy();
    virtual const char* get_name();

  public:
    virtual AM_STATE run();
    virtual AM_STATE stop();

  protected:
    virtual void on_cmd(CMD& cmd){}
    AMQueue* msgQ();
    void get_cmd(AMIActiveObject::CMD& cmd);
    bool peek_cmd(AMIActiveObject::CMD& cmd);
    void ack(AM_STATE result);

  protected:
    AMPacketActiveOutputPin(AMPacketFilter *filter);
    virtual ~AMPacketActiveOutputPin();
    AM_STATE init(const char *name);

  protected:
    AMWorkQueue *m_work_q;
    const char  *m_name;
};
#endif /* AM_AMF_PACKET_PIN_H_ */
