/*******************************************************************************
 * am_service_frame.h
 *
 * History:
 *   2015-1-26 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_SERVICES_SERVICE_FRAME_AM_SERVICE_FRAME_H_
#define ORYX_SERVICES_SERVICE_FRAME_AM_SERVICE_FRAME_H_

#include "am_service_frame_if.h"
#include <semaphore.h>

enum AM_SERVICE_FRAME_CMD
{
  SERVICE_CMD_NULL  = 0,
  SERVICE_CMD_QUIT  = 'e',
  SERVICE_CMD_ABORT = 'a',
};
class AMThread;
class AMServiceFrame: public AMIServiceFrame
{
  public:
    static AMIServiceFrame* create(const std::string& name);

  public:
    virtual void destroy();
    virtual void run();
    virtual bool quit();
    virtual void set_user_input_callback(AMServiceFrameCallback cb);
    virtual uint32_t version();

  protected:
    AMServiceFrame(const std::string& name);
    virtual ~AMServiceFrame();
    bool init();

  private:
    bool send_ctrl_cmd(AM_SERVICE_FRAME_CMD cmd);
    bool init_semaphore(const char* sem_name);
    static void send_heartbeat_to_watchdog(void* data);

  private:
    AMServiceFrameCallback m_user_input_callback;
    AMThread              *m_thread_feed_dog;
    sem_t                 *m_semaphore;
    std::string            m_name;
    std::string            m_sem_name;
    bool                   m_run;
    bool                   m_thread_running;
    bool                   m_thread_created;
    int                    m_ctrl_sock[2];

#define CTRL_SOCK_R m_ctrl_sock[0]
#define CTRL_SOCK_W m_ctrl_sock[1]
};

#endif /* ORYX_SERVICES_SERVICE_FRAME_AM_SERVICE_FRAME_H_ */
