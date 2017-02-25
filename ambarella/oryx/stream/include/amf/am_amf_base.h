/*******************************************************************************
 * am_amf_base.h
 *
 * History:
 *   2014-7-23 - [ypchang] created file
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
#ifndef AM_AMF_BASE_H_
#define AM_AMF_BASE_H_

#include "am_mutex.h"

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
    void put_msg(void *msg, uint32_t msgSize);

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
    AMMsgSys         *m_msg_sys;
    AMIMsgSink       *m_app_msg_sink;
    AMIMsgPort       *m_filter_msg_port;
    AMEngineMsgProxy *m_msg_proxy;
    void             *m_app_msg_data;
    MsgProcType       m_app_msg_callback;
    uint32_t          m_session_id;
    AMMemLock         m_mutex;
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
