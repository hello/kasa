/*******************************************************************************
 * am_amf_base.h
 *
 * History:
 *   2014-7-23 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AMF_BASE_H_
#define AM_AMF_BASE_H_

/*
 * AMObject
 */
class AMObject: public AMIInterface
{
  public:
    virtual void* get_interface(AM_REFIID refiid);
    virtual void destroy();
    virtual ~AMObject(){}
};

/*
 * AMWorkQueue
 */
class AMQueue;
class AMThread;
class AMWorkQueue
{
  public:
    static AMWorkQueue* create(AMIActiveObject *activeObj,
                               bool RTPrio = false,
                               int  prio = 99);
    void destroy();

  public:
    AM_STATE send_cmd(uint32_t cmd_id, void *data = NULL);
    void get_cmd(AMIActiveObject::CMD& cmd);
    bool peek_cmd(AMIActiveObject::CMD& cmd);
    bool peek_msg(AmMsg& msg);
    AM_STATE run();
    AM_STATE stop();
    void ack(AM_STATE result);
    AMQueue* msgQ();
    AMQueue::QTYPE wait_data_msg(void *msg,
                                 uint32_t msgSize,
                                 AMQueue::WaitResult *result);

  private:
    AMWorkQueue(AMIActiveObject *activeObj);
    virtual ~AMWorkQueue();
    AM_STATE init(bool RTPrio, int prio);

  private:
    void main_loop();
    static void static_thread_entry(void *data);
    void terminate();

  private:
    AMIActiveObject *m_active_obj;
    AMQueue         *m_msg_q;
    AMThread        *m_thread;
};

/*
 * AMBaseEngine
 */
class AMMsgSys;
class AMSpinLock;
class AMEngineMsgProxy;
class AMBaseEngine: public AMObject,
                    public AMIEngine,
                    public AMIMsgSink,
                    public AMIMediaControl
{
    typedef AMObject inherited;
    friend class AMEngineMsgProxy;

  public:
    virtual void* get_interface(AM_REFIID refiid);
    virtual void destroy();

    virtual AM_STATE post_engine_msg(uint32_t code);
    virtual AM_STATE post_engine_msg(AmMsg& msg);
    virtual AM_STATE set_app_msg_sink(AMIMsgSink *appMsgSink);
    virtual AM_STATE set_app_msg_callback(MsgProcType msgProc, void *data);

  protected:
    AM_STATE post_app_msg(AmMsg& msg);
    AM_STATE post_app_msg(uint32_t code);

    bool is_session_msg(AmMsg& msg);

  protected:
    AMBaseEngine();
    virtual ~AMBaseEngine();
    AM_STATE init();

  private:
    void on_app_msg(AmMsg& msg);

  protected:
    uint32_t          m_session_id;
    AMSpinLock       *m_mutex;
    AMMsgSys         *m_msg_sys;
    AMIMsgSink       *m_app_msg_sink;
    AMIMsgPort       *m_filter_msg_port;
    AMEngineMsgProxy *m_msg_proxy;
    void             *m_app_msg_data;
    MsgProcType       m_app_msg_callback;
};

/*
 * AMEngineMsgProxy
 */
class AMEngineMsgProxy: public AMObject, public AMIMsgSink
{
    typedef AMObject inherited;
  public:
    static AMEngineMsgProxy* create(AMBaseEngine* engine);

  public:
    virtual void* get_interface(AM_REFIID refiid);
    virtual void destroy();
    virtual void msg_proc(AmMsg& msg);

    AMIMsgPort* msg_port();

  protected:
    AMEngineMsgProxy(AMBaseEngine *engine);
    virtual ~AMEngineMsgProxy();
    AM_STATE init();

  private:
    AMBaseEngine *m_engine;
    AMIMsgPort   *m_msg_port;
};

#endif /* AM_AMF_BASE_H_ */
