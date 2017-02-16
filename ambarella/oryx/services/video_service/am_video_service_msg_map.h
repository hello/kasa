/*******************************************************************************
 * am_video_service_msg_map.h
 *
 * History:
 *   2014-9-17 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef AM_VIDEO_SERVICE_MSG_MAP_H_
#define AM_VIDEO_SERVICE_MSG_MAP_H_
#include "commands/am_api_cmd_video.h"
#include "commands/am_api_cmd_common.h"

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

void ON_VIN_GET(void *msg_data,
                int msg_data_size,
                void *result_addr,
                int result_max_size);
void ON_VIN_SET(void *msg_data,
                int msg_data_size,
                void *result_addr,
                int result_max_size);

void ON_STREAM_FMT_GET(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size);
void ON_STREAM_FMT_SET(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size);

void ON_STREAM_CFG_GET(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size);
void ON_STREAM_CFG_SET(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size);

void ON_BUFFER_FMT_GET(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size);

void ON_BUFFER_FMT_SET(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size);

void ON_BUFFER_ALLOC_STYLE_GET(void *msg_data,
                               int msg_data_size,
                               void *result_addr,
                               int result_max_size);

void ON_BUFFER_ALLOC_STYLE_SET(void *msg_data,
                               int msg_data_size,
                               void *result_addr,
                               int result_max_size);

void ON_STREAM_STATUS_GET(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size);

void ON_DPTZ_WARP_SET(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size);

void ON_DPTZ_WARP_GET(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size);

void ON_ENCODE_H264_LBR_SET(void *msg_data,
                            int msg_data_size,
                            void *result_addr,
                            int result_max_size);

void ON_ENCODE_H264_LBR_GET(void *msg_data,
                            int msg_data_size,
                            void *result_addr,
                            int result_max_size);

void ON_VIDEO_ENCODE_START(void *msg_data,
                           int msg_data_size,
                           void *result_addr,
                           int result_max_size);
void ON_VIDEO_ENCODE_STOP(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size);

void ON_VIDEO_FORCE_IDR(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size);

void ON_COMMON_GET_EVENT(void *msg_data,
                         int msg_data_size,
                         void *result_addr,
                         int result_max_size);

void ON_VIDEO_OVERLAY_GET_MAX_NUM(void *msg_data,
                                  int msg_data_size,
                                  void *result_addr,
                                  int result_max_size);

void ON_VIDEO_OVERLAY_DESTROY(void *msg_data,
                              int msg_data_size,
                              void *result_addr,
                              int result_max_size);

void ON_VIDEO_OVERLAY_SAVE(void *msg_data,
                           int msg_data_size,
                           void *result_addr,
                           int result_max_size);

void ON_VIDEO_OVERLAY_ADD(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size);

void ON_VIDEO_OVERLAY_SET(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size);

void ON_VIDEO_OVERLAY_GET(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size);

BEGIN_MSG_MAP(API_PROXY_TO_VIDEO_SERVICE)
MSG_ACTION(AM_IPC_SERVICE_INIT, ON_SERVICE_INIT)
MSG_ACTION(AM_IPC_SERVICE_DESTROY, ON_SERVICE_DESTROY)
MSG_ACTION(AM_IPC_SERVICE_START, ON_SERVICE_START)
MSG_ACTION(AM_IPC_SERVICE_STOP, ON_SERVICE_STOP)
MSG_ACTION(AM_IPC_SERVICE_RESTART, ON_SERVICE_RESTART)
MSG_ACTION(AM_IPC_SERVICE_STATUS, ON_SERVICE_STATUS)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_VIN_GET, ON_VIN_GET)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_VIN_SET, ON_VIN_SET)

MSG_ACTION(AM_IPC_MW_CMD_VIDEO_STREAM_FMT_GET, ON_STREAM_FMT_GET)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_STREAM_FMT_SET, ON_STREAM_FMT_SET)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_STREAM_CFG_GET, ON_STREAM_CFG_GET)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_STREAM_CFG_SET, ON_STREAM_CFG_SET)

MSG_ACTION(AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_GET, ON_BUFFER_FMT_GET)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_SET, ON_BUFFER_FMT_SET)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_GET,
    ON_BUFFER_ALLOC_STYLE_GET)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_BUFFER_ALLOC_STYLE_SET,
    ON_BUFFER_ALLOC_STYLE_SET)

MSG_ACTION(AM_IPC_MW_CMD_VIDEO_STREAM_STATUS_GET, ON_STREAM_STATUS_GET)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_SET, ON_DPTZ_WARP_SET)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_GET, ON_DPTZ_WARP_GET)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_SET, ON_ENCODE_H264_LBR_SET)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_ENCODE_H264_LBR_GET, ON_ENCODE_H264_LBR_GET)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_ENCODE_START, ON_VIDEO_ENCODE_START)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_ENCODE_STOP, ON_VIDEO_ENCODE_STOP)

MSG_ACTION(AM_IPC_MW_CMD_VIDEO_ENCODE_FORCE_IDR, ON_VIDEO_FORCE_IDR)

MSG_ACTION(AM_IPC_MW_CMD_COMMON_GET_EVENT, ON_COMMON_GET_EVENT)

MSG_ACTION(AM_IPC_MW_CMD_VIDEO_OVERLAY_GET_MAX_NUM, ON_VIDEO_OVERLAY_GET_MAX_NUM)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_OVERLAY_DESTROY, ON_VIDEO_OVERLAY_DESTROY)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_OVERLAY_SAVE, ON_VIDEO_OVERLAY_SAVE)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD, ON_VIDEO_OVERLAY_ADD)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_OVERLAY_SET, ON_VIDEO_OVERLAY_SET)
MSG_ACTION(AM_IPC_MW_CMD_VIDEO_OVERLAY_GET, ON_VIDEO_OVERLAY_GET)

END_MSG_MAP()

#endif /* AM_VIDEO_SERVICE_MSG_MAP_H_ */
