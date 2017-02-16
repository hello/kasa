/*
 * am_ipc_sync_cmd.h
 *
 * History:
 *    2013/3/13 - [Louis Sun] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

/*! @file am_ipc_sync_cmd.h
 *  @brief This file defines IPC basic sender and receiver class.
 *
 *  AMIPCSyncCmdClient/AMIPCSyncCmdServer need to be created in pair in
 *  different processes. it's defined for AMIPCSyncCmdServer to do creation
 *  first, after it's done, it must call explicitly to inform it has been
 *  finished, so that AMIPCSyncCmdClient can be created and binded.\n
 *  The purpose is to make the object creation dependency to be more simple.
 *  because of this, AMIPCSyncCmdServer is created with two msg queues,
 *  and AMIPCSyncCmdClient is created with NULL msg queue and it will bind
 *  AMIPCSyncCmdServer's msg queues.\n
 *  User process can create either process first, but AMIPCSyncCmdClient will
 *  always wait AMIPCSyncCmdServer to create and finish, then it starts to
 *  create. so that user application can fork/exec the two processes without
 *  extra synchronizations.
 */
#ifndef  AM_IPC_SYNC_CMD_H__
#define  AM_IPC_SYNC_CMD_H__
#include "am_ipc_cmd.h"
#include <semaphore.h>

#define AM_IPC_SYNC_CMD_MAX_DELAY 20
#define AM_MAX_SEM_NAME_LENGTH 250

/*! @class AMIPCSyncCmdClient
 *  @brief IPC sync CMD client, which is also the CMD sender.
 */
class AMIPCSyncCmdClient : public AMIPCCmdSender
{
  public:
    AMIPCSyncCmdClient ();
    virtual ~AMIPCSyncCmdClient();
    int32_t create(const char *unique_identifier);
    virtual int32_t method_call(uint32_t cmd_id,
                            void *cmd_arg_s,
                            int32_t cmd_size,
                            void *return_value_s,
                            int32_t max_return_value_size);
    static int32_t cleanup(const char*unique_identifier);

  protected:
    int32_t bind(const char *unique_identifier);
    int32_t send_connection_state(int32_t state);
    virtual int32_t destroy();

  protected:
    sem_t *m_create_sem;
    bool m_connected_flag;
    char m_sem_name[AM_MAX_SEM_NAME_LENGTH +1];
};

/*! @class AMIPCSyncCmdServer
 *  @brief IPC sync CMD server, which is also the CMD receiver.
 */
class AMIPCSyncCmdServer : public AMIPCCmdReceiver
{
  public:
    /*!
     *  Constructor
     */
    AMIPCSyncCmdServer();

    /*!
     *  Destructor
     */
    virtual ~AMIPCSyncCmdServer();
    int32_t create(const char *unique_identifier);
    /*! @brief Tell AMIPCSyncCmdClient that init is done by post semaphore
     *
     * notify_init_complete can be called by APP to notify AMIPCSyncCmdClient
     * that init is done. AMIPCSyncCmdClient's normal operation should be to
     * call "bind" to setup connection.
     * @return 0 on success, value less than 0 indicates an error.
     */
    int32_t complete();
    virtual int32_t notify(uint32_t cmd_id, void *cmd_arg_s , int32_t cmd_size);

  protected:
    virtual int32_t destroy();
    static void connection_callback(uint32_t context,
                                   void *msg_data,
                                   int32_t msg_data_size,
                                   void *result_addr,
                                   int32_t result_max_size);

  protected:
    sem_t *m_create_sem;
    int32_t m_connection_state;
    char m_sem_name[AM_MAX_SEM_NAME_LENGTH +1];
};

#endif //AM_IPC_SYNC_CMD_H__
