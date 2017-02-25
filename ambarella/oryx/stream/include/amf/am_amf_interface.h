/*******************************************************************************
 * am_amf_interface.h
 *
 * History:
 *   2014-7-22 - [ypchang] created file
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
/*! @mainpage Oryx Stream AMF
 *  @section Introduction
 *  AMF as known as Ambarella Media Framework, is the base infrastructure
 *  of Oryx Stream Module.
 *  Record module and Playback module are all based on AMF
 */

/*! @file am_amf_interface.h
 *  @brief This file defines the basic AMF interfaces.
 */
#ifndef AM_AMF_INTERFACE_H_
#define AM_AMF_INTERFACE_H_

extern const AM_IID IID_AMIInterface;
extern const AM_IID IID_AMIMsgPort;
extern const AM_IID IID_AMIMsgSink;
extern const AM_IID IID_AMIMediaControl;
extern const AM_IID IID_AMIActiveObject;
extern const AM_IID IID_AMIEngine;
extern const AM_IID IID_AMIPacketPin;
extern const AM_IID IID_AMIPacketPool;
extern const AM_IID IID_AMIPacketFilter;

class IAMInterface;
class IAMMsgSink;
class IAMMediaControl;

struct AmMsg
{
    uint32_t code;
    uint32_t session_id;
    int_ptr  p0;
    int_ptr  p1;
    AmMsg() :
      code(0),
      session_id(0),
      p0(0),
      p1(0){}
    AmMsg(uint32_t msg_code) :
      code(msg_code),
      session_id(0),
      p0(0),
      p1(0){}
};

#define DECLARE_INTERFACE(ifname, refiid) \
  static ifname* get_interface_from(AMIInterface *interface) \
  { \
    return (ifname*)interface->get_interface(refiid); \
  }

/*
 * IAMIInterface
 */
class AMIInterface
{
  public:
    virtual void* get_interface(AM_REFIID refiid) = 0;
    virtual void  destroy() = 0;
    virtual ~AMIInterface() {}
};

/*
 * AMIMsgPort
 */
class AMIMsgPort: public AMIInterface
{
  public:
    DECLARE_INTERFACE(AMIMsgPort, IID_AMIMsgPort);
    virtual AM_STATE post_msg(AmMsg& msg) = 0;
};

/*
 * AMIMsgSink
 */
class AMIMsgSink: public AMIInterface
{
  public:
    DECLARE_INTERFACE(AMIMsgSink, IID_AMIMsgSink);
    virtual void msg_proc(AmMsg& msg) = 0;
};

/*
 * AMIMediaControl
 */
typedef void (*MsgProcType)(void*, AmMsg&);

class AMIMediaControl: public AMIInterface
{
  public:
    DECLARE_INTERFACE(AMIMediaControl, IID_AMIMediaControl);
    virtual AM_STATE set_app_msg_sink(AMIMsgSink *appMsgSink) = 0;
    virtual AM_STATE set_app_msg_callback(MsgProcType callback, void *data) = 0;
};

/*
 * AMIActiveObject
 */
class AMIActiveObject: public AMIInterface
{
  public:
    enum ACTIVE_OBJ_CMD
    {
      CMD_TERMINATE,
      CMD_RUN,
      CMD_STOP,
      CMD_LAST
    };

    struct CMD
    {
        uint32_t code;
        void    *data;
        CMD(uint32_t cmd_id) :
          code(cmd_id),
          data(nullptr)
        {}
        CMD() :
          code(0),
          data(nullptr)
        {}
    };

  public:
    DECLARE_INTERFACE(AMIActiveObject, IID_AMIActiveObject);
    virtual const char* get_name() = 0;
    virtual void on_run() = 0;
    virtual void on_cmd(CMD& cmd) = 0;
    virtual ~AMIActiveObject(){}
};

/*
 * AMIEngine
 */
class AMIEngine: public AMIInterface
{
  public:
    enum ENGINE_MSG
    {
      ENG_MSG_ERROR,
      ENG_MSG_EOS,
      ENG_MSG_ABORT,
      ENG_MSG_OK,
      ENG_MSG_EOF,
      ENG_MSG_EOL,
      ENG_MSG_LAST
    };

  public:
    DECLARE_INTERFACE(AMIEngine, IID_AMIEngine);
    virtual AM_STATE post_engine_msg(uint32_t code) = 0;
    virtual AM_STATE post_engine_msg(AmMsg& msg) = 0;
};

/*
 * AMIPacketPin
 */
class AMPacket;
class AMIPacketFilter;
class AMIPacketPin: public AMIInterface
{
  public:
    DECLARE_INTERFACE(AMIPacketPin, IID_AMIPacketPin);
    virtual AM_STATE connect(AMIPacketPin *peer) = 0;
    virtual void disconnect() = 0;
    virtual void receive(AMPacket *packet) = 0;
    virtual void purge() = 0;
    virtual void enable(bool enabled) = 0;
    virtual bool is_enable() = 0;
    virtual AMIPacketPin* get_peer() = 0;
    virtual AMIPacketFilter* get_filter() = 0;
    virtual bool is_connected() = 0;
    virtual const char* filter_name() = 0;
};

/*
 * AMIPacketPool
 */
class AMIPacketPool: public AMIInterface
{
  public:
    DECLARE_INTERFACE(AMIPacketPool, IID_AMIPacketPool);
    virtual const char* get_name() = 0;
    virtual void enable(bool enabled = true) = 0;
    virtual bool is_enable() = 0;
    virtual bool alloc_packet(AMPacket*& packet, uint32_t size) = 0;
    virtual uint32_t get_avail_packet_num() = 0;
    virtual void add_ref(AMPacket *packet) = 0;
    virtual void add_ref() = 0;
    virtual void release(AMPacket *packet) = 0;
    virtual void release() = 0;
};

/*
 * AMIPacketFilter
 */
class AMIPacketFilter: public AMIInterface
{
  public:
    struct INFO
    {
      uint32_t num_in;
      uint32_t num_out;
      const char *name;
    };

  public:
    DECLARE_INTERFACE(AMIPacketFilter, IID_AMIPacketFilter);
    virtual AM_STATE run()  = 0;
    virtual AM_STATE stop() = 0;

    virtual void purge()                                 = 0;
    virtual void add_ref()                               = 0;
    virtual void get_info(INFO& info)                    = 0;
    virtual AMIPacketPin* get_input_pin(uint32_t index)  = 0;
    virtual AMIPacketPin* get_output_pin(uint32_t index) = 0;
    /*
    virtual AM_STATE add_output_pin(uint32_t& index)     = 0;
    virtual AM_STATE add_input_pin(uint32_t& index, uint32_t type) = 0;
    */
};

/*
 * Filter's get object function
 */
typedef AMIInterface* (*GetFilterObject)(AMIEngine*, const char*, \
                                         uint32_t, uint32_t);
#define GET_FILTER_OBJ ((const char*)"create_filter")

#endif /* AM_AMF_INTERFACE_H_ */
