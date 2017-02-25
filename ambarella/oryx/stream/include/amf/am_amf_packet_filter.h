/*******************************************************************************
 * am_amf_packet_filter.h
 *
 * History:
 *   2014-7-24 - [ypchang] created file
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
    virtual void purge();
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
