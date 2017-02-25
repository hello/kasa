/*******************************************************************************
 * am_system_service_msg_map.h
 *
 * History:
 *   2014-9-17 - [lysun] created file
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
#ifndef AM_SYSTEM_SERVICE_MSG_MAP_H_
#define AM_SYSTEM_SERVICE_MSG_MAP_H_
#include "commands/am_api_cmd_system.h"
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
void ON_FIRMWARE_VERSION_GET(void *msg_data,
                             int msg_data_size,
                             void *result_addr,
                             int result_max_size);
void ON_SYSTEM_DATETIME_GET(void *msg_data,
                            int msg_data_size,
                            void *result_addr,
                            int result_max_size);
void ON_SYSTEM_DATETIME_SET(void *msg_data,
                            int msg_data_size,
                            void *result_addr,
                            int result_max_size);
void ON_NTP_GET(void *msg_data,
                int msg_data_size,
                void *result_addr,
                int result_max_size);
void ON_NTP_SET(void *msg_data,
                int msg_data_size,
                void *result_addr,
                int result_max_size);
void ON_LED_INDICATOR_GET(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size);
void ON_LED_INDICATOR_SET(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size);
void ON_LED_INDICATOR_UNINIT(void *msg_data,
                             int msg_data_size,
                             void *result_addr,
                             int result_max_size);
void ON_LED_INDICATOR_UNINIT_ALL(void *msg_data,
                                 int msg_data_size,
                                 void *result_addr,
                                 int result_max_size);
void ON_FIRMWARE_UPGRADE_SET(void *msg_data,
                             int msg_data_size,
                             void *result_addr,
                             int result_max_size);
void ON_UPGRADE_STATUS_GET(void *msg_data,
                           int msg_data_size,
                           void *result_addr,
                           int result_max_size);

BEGIN_MSG_MAP(API_PROXY_TO_SYSTEM_SERVICE)
MSG_ACTION(AM_IPC_SERVICE_INIT, ON_SERVICE_INIT)
MSG_ACTION(AM_IPC_SERVICE_DESTROY, ON_SERVICE_DESTROY)
MSG_ACTION(AM_IPC_SERVICE_START, ON_SERVICE_START)
MSG_ACTION(AM_IPC_SERVICE_STOP, ON_SERVICE_STOP)
MSG_ACTION(AM_IPC_SERVICE_RESTART, ON_SERVICE_RESTART)
MSG_ACTION(AM_IPC_SERVICE_STATUS, ON_SERVICE_STATUS)
MSG_ACTION(AM_IPC_MW_CMD_SYSTEM_FIRMWARE_VERSION_GET, ON_FIRMWARE_VERSION_GET)
MSG_ACTION(AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_GET, ON_SYSTEM_DATETIME_GET)
MSG_ACTION(AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_SET, ON_SYSTEM_DATETIME_SET)
MSG_ACTION(AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_GET, ON_NTP_GET)
MSG_ACTION(AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_SET, ON_NTP_SET)
MSG_ACTION(AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_GET, ON_LED_INDICATOR_GET)
MSG_ACTION(AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_SET, ON_LED_INDICATOR_SET)
MSG_ACTION(AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT, ON_LED_INDICATOR_UNINIT)
MSG_ACTION(AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT_ALL,
           ON_LED_INDICATOR_UNINIT_ALL)
MSG_ACTION(AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_SET, ON_FIRMWARE_UPGRADE_SET)
MSG_ACTION(AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_STATUS_GET,
           ON_UPGRADE_STATUS_GET)

END_MSG_MAP()


#endif /* AM_SYSTEM_SERVICE_MSG_MAP_H_ */
