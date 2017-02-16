/*******************************************************************************
 * am_audio_service_msg_map.h
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


#ifndef AM_AUDIO_SERVICE_MSG_MAP_H_
#define AM_AUDIO_SERVICE_MSG_MAP_H_
#include "commands/am_api_cmd_audio.h"
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
void ON_AUDIO_SAMPLE_RATE_GET(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_SAMPLE_RATE_SET(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_CHANNEL_GET(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_CHANNEL_SET(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_DEVICE_VOLUME_GET_BY_INDEX(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_DEVICE_VOLUME_SET_BY_INDEX(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_DEVICE_VOLUME_GET_BY_NAME(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_DEVICE_VOLUME_SET_BY_NAME(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_DEVICE_MUTE_GET_BY_INDEX(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_DEVICE_MUTE_SET_BY_INDEX(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_DEVICE_MUTE_GET_BY_NAME(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_DEVICE_MUTE_SET_BY_NAME(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_DEVICE_INDEX_LIST_GET(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_DEVICE_NAME_GET_BY_INDEX(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
#if 0
void ON_AUDIO_INPUT_SELECT_GET(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_INPUT_SELECT_SET(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_AEC_GET(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_AEC_SET(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_EFFECT_GET(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_AUDIO_EFFECT_SET(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
#endif

BEGIN_MSG_MAP(API_PROXY_TO_AUDIO_SERVICE)
MSG_ACTION(AM_IPC_SERVICE_INIT                           , ON_SERVICE_INIT)
MSG_ACTION(AM_IPC_SERVICE_DESTROY                        , ON_SERVICE_DESTROY)
MSG_ACTION(AM_IPC_SERVICE_START                          , ON_SERVICE_START)
MSG_ACTION(AM_IPC_SERVICE_STOP                           , ON_SERVICE_STOP)
MSG_ACTION(AM_IPC_SERVICE_RESTART                        , ON_SERVICE_RESTART)
MSG_ACTION(AM_IPC_SERVICE_STATUS                         , ON_SERVICE_STATUS)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_GET           , ON_AUDIO_SAMPLE_RATE_GET)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_SET           , ON_AUDIO_SAMPLE_RATE_SET)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_CHANNEL_GET               , ON_AUDIO_CHANNEL_GET)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_CHANNEL_SET               , ON_AUDIO_CHANNEL_SET)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_INDEX, ON_AUDIO_DEVICE_VOLUME_GET_BY_INDEX)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_INDEX, ON_AUDIO_DEVICE_VOLUME_SET_BY_INDEX)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_NAME , ON_AUDIO_DEVICE_VOLUME_GET_BY_NAME)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_NAME , ON_AUDIO_DEVICE_VOLUME_SET_BY_NAME)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_INDEX  , ON_AUDIO_DEVICE_MUTE_GET_BY_INDEX)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_INDEX  , ON_AUDIO_DEVICE_MUTE_SET_BY_INDEX)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_NAME   , ON_AUDIO_DEVICE_MUTE_GET_BY_NAME)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_NAME   , ON_AUDIO_DEVICE_MUTE_SET_BY_NAME)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_DEVICE_NAME_GET_BY_INDEX  , ON_AUDIO_DEVICE_NAME_GET_BY_INDEX)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET     , ON_AUDIO_DEVICE_INDEX_LIST_GET)
#if 0
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_INPUT_SELECT_GET          , ON_AUDIO_INPUT_SELECT_GET)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_INPUT_SELECT_SET          , ON_AUDIO_INPUT_SELECT_SET)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_AEC_GET                   , ON_AUDIO_AEC_GET)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_AEC_SET                   , ON_AUDIO_AEC_SET)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_EFFECT_GET                , ON_AUDIO_EFFECT_GET)
MSG_ACTION(AM_IPC_MW_CMD_AUDIO_EFFECT_SET                , ON_AUDIO_EFFECT_SET)
#endif
END_MSG_MAP()

#endif /* AM_AUDIO_SERVICE_MSG_MAP_H_ */
