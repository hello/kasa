/*******************************************************************************
 * am_sip_ua_server_msg_map.h
 *
 * History:
 *   2015-1-28 - [Shiming Dong] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef AM_SIP_UA_SERVER_MSG_MAP_H_
#define AM_SIP_UA_SERVER_MSG_MAP_H_

#include "commands/am_api_cmd_sip.h"

void ON_SERVICE_INIT(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size);
void ON_SERVICE_DESTROY(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size);
void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size);
void ON_SERVICE_STOP(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size);
void ON_SERVICE_RESTART(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size);
void ON_SERVICE_STATUS(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size);
void ON_AM_IPC_MW_CMD_SET_SIP_REGISTRATION(void *msg_data,
                                           int msg_data_size,
                                           void *result_addr,
                                           int result_max_size);
void ON_AM_IPC_MW_CMD_SET_SIP_CONFIGURATION(void *msg_data,
                                            int msg_data_size,
                                            void *result_addr,
                                            int result_max_size);
void ON_AM_IPC_MW_CMD_SET_SIP_MEDIA_PRIORITY(void *msg_data,
                                             int msg_data_size,
                                             void *result_addr,
                                             int result_max_size);
void ON_AM_IPC_MW_CMD_SET_SIP_CALLEE_ADDRESS(void *msg_data,
                                             int msg_data_size,
                                             void *result_addr,
                                             int result_max_size);
void ON_AM_IPC_MW_CMD_SET_SIP_HANGUP_USERNAME(void *msg_data,
                                             int msg_data_size,
                                             void *result_addr,
                                             int result_max_size);
BEGIN_MSG_MAP(API_PROXY_TO_SIP_UA_SERVER)
MSG_ACTION(AM_IPC_SERVICE_INIT, ON_SERVICE_INIT)
MSG_ACTION(AM_IPC_SERVICE_DESTROY, ON_SERVICE_DESTROY)
MSG_ACTION(AM_IPC_SERVICE_START, ON_SERVICE_START)
MSG_ACTION(AM_IPC_SERVICE_STOP, ON_SERVICE_STOP)
MSG_ACTION(AM_IPC_SERVICE_RESTART, ON_SERVICE_RESTART)
MSG_ACTION(AM_IPC_SERVICE_STATUS, ON_SERVICE_STATUS)
MSG_ACTION(AM_IPC_MW_CMD_SET_SIP_REGISTRATION,
    ON_AM_IPC_MW_CMD_SET_SIP_REGISTRATION)
MSG_ACTION(AM_IPC_MW_CMD_SET_SIP_CONFIGURATION,
    ON_AM_IPC_MW_CMD_SET_SIP_CONFIGURATION)
MSG_ACTION(AM_IPC_MW_CMD_SET_SIP_MEDIA_PRIORITY,
    ON_AM_IPC_MW_CMD_SET_SIP_MEDIA_PRIORITY)
MSG_ACTION(AM_IPC_MW_CMD_SET_SIP_CALLEE_ADDRESS,
    ON_AM_IPC_MW_CMD_SET_SIP_CALLEE_ADDRESS)
MSG_ACTION(AM_IPC_MW_CMD_SET_SIP_HANGUP_USERNAME,
    ON_AM_IPC_MW_CMD_SET_SIP_HANGUP_USERNAME)
END_MSG_MAP()

#endif /* AM_SIP_UA_SERVER_MSG_MAP_H_ */

