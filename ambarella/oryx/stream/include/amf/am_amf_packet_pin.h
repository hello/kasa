/*******************************************************************************
 * am_amf_packet_pin.h
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
    AMPacketFilter *m_filter;
    AMIPacketPin   *m_peer;
    AMIPacketPool  *m_packet_pool;
    bool            m_is_external_packet_pool;
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
