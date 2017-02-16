/*******************************************************************************
 * am_record_engine.h
 *
 * History:
 *   2014-12-30 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_RECORD_ENGINE_AM_RECORD_ENGINE_H_
#define ORYX_STREAM_RECORD_ENGINE_AM_RECORD_ENGINE_H_

struct EngineConfig;
struct EngineFilter;
struct ConnectionConfig;

class AMEvent;
class AMThread;
class AMSpinLock;
class AMRecordEngineConfig;
class AMIMuxer;

class AMRecordEngine: public AMEngineFrame, public AMIRecordEngine
{
    typedef AMEngineFrame inherited;

    enum AM_RECORD_ENGINE_CMD
    {
      AM_ENGINE_CMD_START = 's',
      AM_ENGINE_CMD_STOP  = 'q',
      AM_ENGINE_CMD_ABORT = 'a',
      AM_ENGINE_CMD_EXIT  = 'e',
    };
  public:
    static AMRecordEngine* create(const std::string& config);

  public:
    virtual void* get_interface(AM_REFIID ref_iid);
    virtual void destroy();

  public:
    virtual AMIRecordEngine::AM_RECORD_ENGINE_STATUS get_engine_status();
    virtual bool create_graph();
    virtual bool record();
    virtual bool stop();
    virtual bool start_file_recording();
    virtual bool stop_file_recording();
    virtual bool send_event();
    virtual bool is_ready_for_event();
    virtual void set_app_msg_callback(AMRecordCallback callback, void *data);

  private:
    static void static_app_msg_callback(void *context, AmMsg& msg);
    void app_msg_callback(void *context, AmMsg& msg);
    virtual void msg_proc(AmMsg& msg);
    bool change_engine_status(AMIRecordEngine::AM_RECORD_ENGINE_STATUS tStatus);

  private:
    AMRecordEngine();
    virtual ~AMRecordEngine();
    AM_STATE init(const std::string& config);
    AM_STATE load_all_filters();
    AMIPacketFilter* get_filter_by_name(std::string& name);
    AMIMuxer* get_muxer_filter_by_name(std::string& name);
    const char* get_filter_name_by_pointer(AMIInterface *filter);
    ConnectionConfig* get_connection_conf_by_name(std::string& name);
    void* get_filter_by_iid(AM_REFIID iid);
    static void static_mainloop(void *data);
    void mainloop();
    bool send_engine_cmd(AM_RECORD_ENGINE_CMD cmd, bool block = true);

  private:
    AMSpinLock             *m_lock;
    AMRecordEngineConfig   *m_config;
    EngineConfig           *m_engine_config; /* No need to delete */
    EngineFilter           *m_engine_filter;
    void                   *m_app_data;
    AMThread               *m_thread;
    AMEvent                *m_event;
    AMEvent                *m_sem;
    AMRecordCallback        m_app_callback;
    AMRecordMsg             m_app_msg;
    AM_RECORD_ENGINE_STATUS m_status;
    bool                    m_graph_created;
    bool                    m_mainloop_run;
    int                     m_msg_ctrl[2];
#define MSG_R m_msg_ctrl[0]
#define MSG_W m_msg_ctrl[1]
};

#endif /* ORYX_STREAM_RECORD_ENGINE_AM_RECORD_ENGINE_H_ */
