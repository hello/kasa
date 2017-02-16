/*******************************************************************************
 * am_media_service_msg_map.h
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


#ifndef AM_MEDIA_SERVICE_MSG_MAP_H_
#define AM_MEDIA_SERVICE_MSG_MAP_H_
#include "commands/am_api_cmd_media.h"
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
void ON_AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START(void* msg_data,
                                                  int msg_data_size,
                                                  void *result_addr,
                                                  int result_max_size);
void ON_AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE(void* msg_data,
                                           int msg_data_size,
                                           void *result_addr,
                                           int result_max_size);
void ON_AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE(void *msg_data,
                                                      int msg_data_size,
                                                      void *result_addr,
                                                      int result_max_size);
void ON_AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE(void *msg_data,
                                                      int msg_data_size,
                                                      void *result_addr,
                                                      int result_max_size);
void ON_AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE(void *msg_data,
                                                     int msg_data_size,
                                                     void *result_addr,
                                                     int result_max_size);
void ON_AM_IPC_MW_CMD_MEDIA_START_RECORDING(void *msg_data,
                                            int msg_data_size,
                                            void *result_addr,
                                            int result_max_size);
void ON_AM_IPC_MW_CMD_MEDIA_STOP_RECORDING(void *msg_data,
                                           int msg_data_size,
                                           void *result_addr,
                                           int result_max_size);
void ON_AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING(void *msg_data,
                                                 int msg_data_size,
                                                 void *result_addr,
                                                 int result_max_size);
void ON_AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING(void *msg_data,
                                                int msg_data_size,
                                                void *result_addr,
                                                int result_max_size);
BEGIN_MSG_MAP(API_PROXY_TO_MEDIA_SERVICE)
MSG_ACTION(AM_IPC_SERVICE_INIT, ON_SERVICE_INIT)
MSG_ACTION(AM_IPC_SERVICE_DESTROY, ON_SERVICE_DESTROY)
MSG_ACTION(AM_IPC_SERVICE_START, ON_SERVICE_START)
MSG_ACTION(AM_IPC_SERVICE_STOP, ON_SERVICE_STOP)
MSG_ACTION(AM_IPC_SERVICE_RESTART, ON_SERVICE_RESTART)
MSG_ACTION(AM_IPC_SERVICE_STATUS, ON_SERVICE_STATUS)
MSG_ACTION(AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START,
           ON_AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START)
MSG_ACTION(AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE,
           ON_AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE)
MSG_ACTION(AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE,
           ON_AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE)
MSG_ACTION(AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE,
           ON_AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE)
MSG_ACTION(AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE,
           ON_AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE)
MSG_ACTION(AM_IPC_MW_CMD_MEDIA_START_RECORDING,
           ON_AM_IPC_MW_CMD_MEDIA_START_RECORDING)
MSG_ACTION(AM_IPC_MW_CMD_MEDIA_STOP_RECORDING,
           ON_AM_IPC_MW_CMD_MEDIA_STOP_RECORDING)
MSG_ACTION(AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING,
           ON_AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING)
MSG_ACTION(AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING,
           ON_AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING)
END_MSG_MAP()


#endif /* AM_MEDIA_SERVICE_MSG_MAP_H_ */
