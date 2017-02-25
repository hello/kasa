/*******************************************************************************
 * am_playback_service_msg_map.h
 *
 * History:
 *   2016-04-14 - [Zhi He] created file
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


#ifndef AM_PLAYBACK_SERVICE_MSG_MAP_H_
#define AM_PLAYBACK_SERVICE_MSG_MAP_H_
#include "commands/am_api_cmd_playback.h"
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
void ON_PLAYBACK_CHECK_DSP_MODE(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_EXIT_DSP_PLAYBACK_MODE(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_START_PLAY(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_STOP_PLAY(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_PAUSE(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_RESUME(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_STEP(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_QUERY_CURRENT_POSITION(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_SEEK(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_FAST_FORWARD(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_FAST_BACKWARD(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_FAST_FORWARD_FROM_BEGIN(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_FAST_BACKWARD_FROM_END(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_CONFIGURE_VOUT(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);
void ON_PLAYBACK_HALT_VOUT(void *msg_data,
                     int  msg_data_size,
                     void *result_addr,
                     int  result_max_size);

BEGIN_MSG_MAP(API_PROXY_TO_PLAYBACK_SERVICE)
MSG_ACTION(AM_IPC_SERVICE_INIT, ON_SERVICE_INIT)
MSG_ACTION(AM_IPC_SERVICE_DESTROY, ON_SERVICE_DESTROY)
MSG_ACTION(AM_IPC_SERVICE_START, ON_SERVICE_START)
MSG_ACTION(AM_IPC_SERVICE_STOP, ON_SERVICE_STOP)
MSG_ACTION(AM_IPC_SERVICE_RESTART, ON_SERVICE_RESTART)
MSG_ACTION(AM_IPC_SERVICE_STATUS, ON_SERVICE_STATUS)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_CHECK_DSP_MODE, ON_PLAYBACK_CHECK_DSP_MODE)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_EXIT_DSP_PLAYBACK_MODE, ON_PLAYBACK_EXIT_DSP_PLAYBACK_MODE)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_START_PLAY, ON_PLAYBACK_START_PLAY)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_STOP_PLAY, ON_PLAYBACK_STOP_PLAY)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_PAUSE, ON_PLAYBACK_PAUSE)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_RESUME, ON_PLAYBACK_RESUME)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_STEP, ON_PLAYBACK_STEP)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_QUERY_CURRENT_POSITION, ON_PLAYBACK_QUERY_CURRENT_POSITION)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_SEEK, ON_PLAYBACK_SEEK)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD, ON_PLAYBACK_FAST_FORWARD)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD, ON_PLAYBACK_FAST_BACKWARD)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD_FROM_BEGIN, ON_PLAYBACK_FAST_FORWARD_FROM_BEGIN)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD_FROM_END, ON_PLAYBACK_FAST_BACKWARD_FROM_END)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_CONFIGURE_VOUT, ON_PLAYBACK_CONFIGURE_VOUT)
MSG_ACTION(AM_IPC_MW_CMD_PLAYBACK_HALT_VOUT, ON_PLAYBACK_HALT_VOUT)
END_MSG_MAP()

#endif /* AM_PLAYBACK_SERVICE_MSG_MAP_H_ */
