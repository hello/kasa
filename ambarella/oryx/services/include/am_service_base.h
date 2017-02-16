/*******************************************************************************
 * am_service_base.h
 *
 * History:
 *   2014-9-12 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef AM_SERVICE_BASE_H_
#define AM_SERVICE_BASE_H_
#include "am_base_include.h"
#include "commands/am_service_impl.h"
//Any Oryx Service must be singleton, which means, it cannot be created twice
//within same process.  and more than that, Oryx service should not even be created
//more than once in whole system.
//so we use singleton to protect the class creation, and use pidlock to prevent
//the instance being created more than once in whole system
#define AM_SERVICE_EXE_FILENAME_MAX_LENGTH 512
#define AM_SERVICE_MAX_APPS_ARGS_NUM       10

class AMIPCSyncCmdClient;
class AMServiceBase
{
  public:
    virtual ~AMServiceBase();
    AMServiceBase(const char *service_name,
                  const char *full_pathname,
                  AM_SERVICE_CMD_TYPE type);

    static void on_notif_callback(uint32_t   context,
                                  void      *msg_data,
                                  int        msg_data_size,
                                  void      *result_addr,
                                  int        result_max_size);
    int get_name(char *service_name, int max_len); //report its name
    AM_SERVICE_CMD_TYPE get_type();
    //int set_name(const char *service_name);   //service_name does not have to be same as executable filename
    //int set_exe_filename(const char *full_pathname);

    int register_msg_map();   //register msg map to handle the notification received from real process

    //init/destroy are special functrions of the service.
    virtual int init();     //service instance init, call it only once after creation
    //All IPC connections of that service are also initialized
    virtual int destroy();  //deinit process, restore to status before init called
    //should be called when service is not actively running
    //ALL IPC connections of that service are also destroyed

    //start/stop/restart/status are the main normal functions of that service
    virtual int start();    //start service's real function,
    virtual int stop();     //stop service's real function
    virtual int restart();  //restart service's real function
    virtual int status(AM_SERVICE_STATE  *state);   //report status

    virtual int method_call(uint32_t cmd_id, void *msg_data,
                            int msg_data_size,
                            void *result_addr,
                            int result_max_size);
  protected:
    int create_process();
    int wait_process();
    int create_ipc();
    int cleanup_ipc();
    int m_service_pid;  //the service is going to run in a new process with this pid

    AM_SERVICE_STATE  m_state;

    AMIPCSyncCmdClient *m_ipc;
    AM_SERVICE_CMD_TYPE m_type;
    char m_name[AM_SERVICE_NAME_MAX_LENGTH];
    char m_exe_filename[AM_SERVICE_EXE_FILENAME_MAX_LENGTH];
};

#endif /* AM_SERVICE_BASE_H_ */
