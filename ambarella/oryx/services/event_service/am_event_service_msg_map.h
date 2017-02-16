/*******************************************************************************
 * am_event_service_msg_map.h
 *
 * History:
 *   2014-10-21 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_EVENT_SERVICE_MSG_MAP_H_
#define AM_EVENT_SERVICE_MSG_MAP_H_
#include "commands/am_api_cmd_event.h"

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

void ON_SERVICE_REGISTER_MODULES(void *msg_data,
                                 int msg_data_size,
                                 void *result_addr,
                                 int result_max_size);
void ON_SERVICE_IS_MODULE_RESGISTER(void *msg_data,
                                    int msg_data_size,
                                    void *result_addr,
                                    int result_max_size);
void ON_SERVICE_START_ALL_EVENT_MOUDLE(void *msg_data,
                                       int msg_data_size,
                                       void *result_addr,
                                       int result_max_size);
void ON_SERVICE_STOP_ALL_EVENT_MOUDLE(void *msg_data,
                                      int msg_data_size,
                                      void *result_addr,
                                      int result_max_size);
void ON_SERVICE_DESTROY_ALL_EVENT_MOUDLE(void *msg_data,
                                         int msg_data_size,
                                         void *result_addr,
                                         int result_max_size);
void ON_SERVICE_START_EVENT_MOUDLE(void *msg_data,
                                   int msg_data_size,
                                   void *result_addr,
                                   int result_max_size);
void ON_SERVICE_STOP_EVENT_MOUDLE(void *msg_data,
                                  int msg_data_size,
                                  void *result_addr,
                                  int result_max_size);
void ON_SERVICE_DESTROY_EVENT_MOUDLE(void *msg_data,
                                     int msg_data_size,
                                     void *result_addr,
                                     int result_max_size);
void ON_SERVICE_SET_EVENT_MOTION_DETECT_CONFIG(void *msg_data,
                                               int msg_data_size,
                                               void *result_addr,
                                               int result_max_size);
void ON_SERVICE_SET_EVENT_AUDIO_ALERT_CONFIG(void *msg_data,
                                             int msg_data_size,
                                             void *result_addr,
                                             int result_max_size);
void ON_SERVICE_SET_EVENT_KEY_INPUT_CONFIG(void *msg_data,
                                           int msg_data_size,
                                           void *result_addr,
                                           int result_max_size);
void ON_SERVICE_GET_EVENT_MOTION_DETECT_CONFIG(void *msg_data,
                                               int msg_data_size,
                                               void *result_addr,
                                               int result_max_size);
void ON_SERVICE_GET_EVENT_AUDIO_ALERT_CONFIG(void *msg_data,
                                             int msg_data_size,
                                             void *result_addr,
                                             int result_max_size);
void ON_SERVICE_GET_EVENT_KEY_INPUT_CONFIG(void *msg_data,
                                           int msg_data_size,
                                           void *result_addr,
                                           int result_max_size);

BEGIN_MSG_MAP(API_PROXY_TO_EVENT_SERVICE)
MSG_ACTION(AM_IPC_SERVICE_INIT, ON_SERVICE_INIT)
MSG_ACTION(AM_IPC_SERVICE_DESTROY, ON_SERVICE_DESTROY)
MSG_ACTION(AM_IPC_SERVICE_START, ON_SERVICE_START)
MSG_ACTION(AM_IPC_SERVICE_STOP, ON_SERVICE_STOP)
MSG_ACTION(AM_IPC_SERVICE_RESTART, ON_SERVICE_RESTART)
MSG_ACTION(AM_IPC_SERVICE_STATUS, ON_SERVICE_STATUS)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_REGISTER_MODULE_SET, ON_SERVICE_REGISTER_MODULES)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_CHECK_MODULE_REGISTER_GET, ON_SERVICE_IS_MODULE_RESGISTER)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_START_ALL_MODULE, ON_SERVICE_START_ALL_EVENT_MOUDLE)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_STOP_ALL_MODULE, ON_SERVICE_STOP_ALL_EVENT_MOUDLE)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_DESTROY_ALL_MODULE, ON_SERVICE_DESTROY_ALL_EVENT_MOUDLE)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_START_MODULE, ON_SERVICE_START_EVENT_MOUDLE)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_STOP_MODULE, ON_SERVICE_STOP_EVENT_MOUDLE)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_DESTROY_MODULE, ON_SERVICE_DESTROY_EVENT_MOUDLE)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_MOTION_DETECT_CONFIG_SET, ON_SERVICE_SET_EVENT_MOTION_DETECT_CONFIG)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_AUDIO_ALERT_CONFIG_SET, ON_SERVICE_SET_EVENT_AUDIO_ALERT_CONFIG)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_KEY_INPUT_CONFIG_SET, ON_SERVICE_SET_EVENT_KEY_INPUT_CONFIG)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_MOTION_DETECT_CONFIG_GET, ON_SERVICE_GET_EVENT_MOTION_DETECT_CONFIG)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_AUDIO_ALERT_CONFIG_GET, ON_SERVICE_GET_EVENT_AUDIO_ALERT_CONFIG)
MSG_ACTION(AM_IPC_MW_CMD_EVENT_KEY_INPUT_CONFIG_GET, ON_SERVICE_GET_EVENT_KEY_INPUT_CONFIG)
END_MSG_MAP()

#endif /* AM_EVENT_SERVICE_MSG_MAP_H_ */
