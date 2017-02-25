/*
 * am_ipc_cmd.cpp
 *
 * History:
 *    2014/09/09 - [Louis Sun] Create
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include "am_base_include.h"
#include "am_ipc_cmd.h"
#include "am_ipc_cmd_group.h"
#include "am_define.h"
#include "am_log.h"
/*************************************************************************

 Below is AMIPCCmdSender

 **************************************************************************/

static inline int pack_msg(am_ipc_message_t *ipc_msg,
                           uint32_t msg_id,
                           void *cmd_data,
                           int cmd_max_size)
{
  int ret = 0;
  do {
    am_ipc_message_header_t *pheader = nullptr;
    struct timeval curtime;

    if (AM_UNLIKELY(ipc_msg == nullptr)) {
      ret = -1;
      break;
    }
    gettimeofday(&curtime, nullptr);
    if (AM_UNLIKELY((uint32_t)cmd_max_size >
    AM_MAX_IPC_MESSAGE_SIZE - sizeof(am_ipc_message_header_t))) {
      PRINTF("cmd arg too long %d!", cmd_max_size);
      ret = -1;
      break;
    }

    pheader = &(ipc_msg->header);
    pheader->msg_id = msg_id;
    pheader->time_stamp = curtime.tv_sec * 1000000L + curtime.tv_usec;
    pheader->header_size = sizeof(am_ipc_message_header_t);

    if (cmd_data) {
      pheader->payload_size = cmd_max_size;
      memcpy(ipc_msg->payload.data, cmd_data, cmd_max_size);
    } else {
      pheader->payload_size = 0;
    }
  } while (0);
  return ret;
}

//get msg id and msg data and pack into ipc_msg struct, just copy the msg data
//msg data should not have context related data like pointer or address
static inline int unpack_msg(am_ipc_message_t *ipc_msg,
                             uint32_t *msg_id,
                             void *cmd_data,
                             int max_cmd_data_size,
                             int *cmd_data_size)
{
  int return_size;
  am_ipc_message_header_t *pheader;

  if (AM_UNLIKELY((ipc_msg == nullptr) || (msg_id == nullptr))) {
    return -1;
  }
  pheader = &(ipc_msg->header);

  *msg_id = pheader->msg_id;
  if (cmd_data) {
    return_size = AM_MIN(pheader->payload_size, max_cmd_data_size);
    memcpy(cmd_data, (uint8_t*)pheader + pheader->header_size, return_size);
  } else {
    //cmd_data nullptr means it's a CMD-NO-RETURN or Notification when cmd arg data is optional
    return_size = 0;
  }

  if(cmd_data_size!= nullptr) {
    *cmd_data_size = return_size;
  }
  return 0;
}

AMIPCCmdSender::AMIPCCmdSender(void)
{
  mc_mutex = {0};
  sem_init(&cmd_sem, 0, 0);
  ipc_cmd = (am_ipc_message_t*)malloc(AM_MAX_IPC_MESSAGE_SIZE);
  ipc_cmd_return = (am_ipc_message_t*)malloc(AM_MAX_IPC_MESSAGE_SIZE);
  pthread_mutex_init(&mc_mutex, nullptr);
}

AMIPCCmdSender::~AMIPCCmdSender(void)
{
  destroy();
}

int AMIPCCmdSender::destroy(void)
{
  pthread_mutex_destroy(&mc_mutex);
  free(ipc_cmd_return);
  free(ipc_cmd);
  return 0;
}

int AMIPCCmdSender::register_msg_map(am_msg_handler_t *map, int num_entry)
{
  //the msg/action map will map  every CMD of method_call ,
  //to the action which is how to run the cmd.
  //it's expected after running the cmd, the return value is got.
  int ret = 0;
  do {
    if (AM_UNLIKELY(!map)) {
      PRINTF("AMIPCCmdSender::register_msg_map: null map!");
      ret = -1;
      break;
    }

    if (AM_UNLIKELY((num_entry <= 0) || (num_entry > MAX_MESSAGES_IN_MAP))) {
      PRINTF("AMIPCCmdSender::register_msg_map: invalid num_entry %d!",
             num_entry);
      ret = -1;
      break;
    }

    for (int i = 0; i < num_entry; i++) {
      ret = register_msg_proc(&map[i]);
      if (AM_UNLIKELY(ret < 0)) {
        PRINTF("AMIPCCmdSender::register_msg_map:register failed at %d!", i);
        ret = -1;
        break;
      }
    }
  } while (0);
  return ret;
}

#define DEFAULT_METHOD_CALL_TIMEOUT_MS 10000
int AMIPCCmdSender::method_call(uint32_t cmd_id,
                                void *cmd_arg_s,
                                int cmd_size,
                                void *return_value_s,
                                int max_return_value_size)
{
  while(pthread_mutex_trylock(&mc_mutex) == EBUSY) {
    ERROR("Last method call is still in process, abandon this method call: 0x%x!",
          cmd_id);
    return AM_IPC_CMD_ERR_AGAIN;
  }
  uint32_t cmd_id_2;
  struct timeval time1, time2;
  int ret = AM_IPC_CMD_SUCCESS;
  gettimeofday(&time1, nullptr);
  do {
    if (AM_UNLIKELY(pack_msg(ipc_cmd, cmd_id, cmd_arg_s, cmd_size) < 0)) {
      PRINTF("AMIPCCmdSender: method_call: pack_msg failed \n");
      ret = AM_IPC_CMD_ERR_INVALID_ARG;
      break;
    }

    if (AM_UNLIKELY(send(ipc_cmd,
                         sizeof(am_ipc_message_header_t) + cmd_size) < 0)) {
      ERROR("AMIPCCmdSender: method_call: send error!");
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }

    if (AM_LIKELY(GET_IPC_MSG_NEED_RETURN(cmd_id))) {
      struct timespec ts;
      int64_t msec = DEFAULT_METHOD_CALL_TIMEOUT_MS;
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_sec += msec / 1000;
      ts.tv_nsec += (msec % 1000) * 1000000;
      if (-1 == sem_timedwait(&cmd_sem, &ts)) {
        ERROR("sem_timedwait timeout");
        ret = AM_IPC_CMD_ERR_TIMEOUT;
        break;
      }
      if (AM_UNLIKELY(unpack_msg(ipc_cmd_return, &cmd_id_2,
                                 return_value_s, max_return_value_size,
                                 nullptr) < 0)) {
        ERROR("AMIPCCmdSender: method_call: unpack_msg failed!");
        ret = AM_IPC_CMD_ERR_INTERNAL;
        break;
      }
    }

    gettimeofday(&time2, nullptr);

    //method call taking time check, warn user if method_call takes too long time
    if (AM_UNLIKELY(((time2.tv_sec-time1.tv_sec)*1000000L +
        (time2.tv_usec - time1.tv_usec)) > 2000000L)) {
      WARN("AMIPCCmdSender: method_call 0x%x takes longer than 2 second, pid = %d",
           cmd_id, getpid());
    }
  } while (0);

  pthread_mutex_unlock(&mc_mutex);
  return ret;
}

//Up notification received by Sender
int AMIPCCmdSender::no_return_cmd_proc(am_ipc_message_t *ipc_cmd)
{
  am_msg_handler_t msg_handler;
  uint32_t cmd_id;
  uint8_t notif_cmd_param_data[AM_MAX_CMD_PARAM_SIZE] = {0};
  int notif_cmd_data_size = 0;
  if (AM_UNLIKELY(unpack_msg(ipc_cmd, &cmd_id, notif_cmd_param_data,
                             AM_MAX_CMD_PARAM_SIZE, &notif_cmd_data_size) < 0)) {
    PRINTF("AMIPCCmdSender:no_return_cmd_proc:unpack_msg failed \n");
    return AM_IPC_CMD_ERR_INVALID_ARG;
  }

  //now process it by MSG/Action
  if (find_msg_handler(cmd_id, &msg_handler) == 0) { //equal 0 means success/found
    if (msg_handler.context == 0) {
      msg_handler.callback(notif_cmd_param_data, notif_cmd_data_size, nullptr, 0);
    } else {
      //when there is context, call with context
      msg_handler.callback_ct(msg_handler.context,
                              notif_cmd_param_data, notif_cmd_data_size,
                              nullptr, 0);
    }
    return AM_IPC_CMD_SUCCESS;
  } else {
    return AM_IPC_CMD_ERR_IGNORE;
  }
}

int AMIPCCmdSender::process_msg(void)
{
  uint32_t msg_id;
  am_ipc_message_t *ipc_cmd;
  ipc_cmd = (am_ipc_message_t*)m_receive_buffer;
  msg_id = ipc_cmd->header.msg_id;
  if (AM_UNLIKELY(GET_IPC_MSG_DIRECTION(msg_id) == 0)) {
    PRINTF("AMIPCCmdSender: wrong msg direction!");
    return AM_IPC_CMD_ERR_INVALID_IPC;
  }
  if (GET_IPC_MSG_NEED_RETURN(msg_id) == 1) {
    memcpy(ipc_cmd_return, m_receive_buffer,
           sizeof(am_ipc_message_header_t) + ipc_cmd->header.payload_size);
    sem_post(&cmd_sem);
  } else {
    INFO("AMIPCCmdSender gets UP notif!");
    return no_return_cmd_proc(ipc_cmd);
  }
  return AM_IPC_CMD_SUCCESS;
}

/*******************************************************************************

 Below is AMIPCCmdReceiver

 ******************************************************************************/

AMIPCCmdReceiver::AMIPCCmdReceiver(void)
{
  ipc_cmd = (am_ipc_message_t*)malloc(AM_MAX_IPC_MESSAGE_SIZE);
  ipc_cmd_return = (am_ipc_message_t*)malloc(AM_MAX_IPC_MESSAGE_SIZE);
}

AMIPCCmdReceiver::~AMIPCCmdReceiver(void)
{
  destroy();
}

int AMIPCCmdReceiver::destroy(void)
{
  free(ipc_cmd_return);
  free(ipc_cmd);
  return 0;
}

int AMIPCCmdReceiver::process_msg(void)
{
  uint32_t msg_id;
  int need_return;
  am_msg_handler_t msg_handler;
  am_ipc_message_t *ipc_cmd;

  char *receive_buffer = get_receive_buffer();
  ipc_cmd = (am_ipc_message_t*)receive_buffer;
  msg_id = ipc_cmd->header.msg_id;

  if (AM_UNLIKELY(GET_IPC_MSG_DIRECTION(msg_id) == 1)) {
    ERROR("wrong msg direction!");
    return -1;
  }

  need_return = GET_IPC_MSG_NEED_RETURN(msg_id);
  if (AM_UNLIKELY(unpack_msg(ipc_cmd,
                             &last_cmd_id, last_cmd_param_data,
                             AM_MAX_CMD_PARAM_SIZE, nullptr) < 0)) {
    ERROR("AMIPCCmdReceiver:process_msg:unpack_msg failed!");
    return -1;
  }

  //Check whether this is a group cmd (batch processing cmd)
  if ((GET_IPC_MSG_IS_GROUP(last_cmd_id))) {
    PRINTF("AMIPCCmdReceiver:: GROUP cmd received!");
    if (process_group_cmd() < 0) {
      ERROR("AMIPCCmdReceiver: unsupported group cmd id 0x%x!", last_cmd_id);
      return -1;
    }
  } else {
    //SIMPLE CMD
    if (find_msg_handler(last_cmd_id, &msg_handler) == 0) {
      if (msg_handler.context == 0) {
        msg_handler.callback(last_cmd_param_data, m_receive_buffer_bytes,
                             last_cmd_result_data, AM_MAX_CMD_RESULT_SIZE);
      } else {
        //when there is context, call with context
        msg_handler.callback_ct(msg_handler.context,
                                last_cmd_param_data, m_receive_buffer_bytes,
                                last_cmd_result_data, AM_MAX_CMD_RESULT_SIZE);
      }
    } else {
      char cmdline[256];
      debug_print_process_name(cmdline, sizeof(cmdline));
      ERROR("Process %s, AMIPCCmdReceiver: unsupported/unregistered cmd id 0x%x, return error code %d!",
             cmdline, last_cmd_id, ERROR_NOT_SUPPORTED_CMD);
      int *ret_value = (int*)last_cmd_result_data;
      *ret_value  = ERROR_NOT_SUPPORTED_CMD;
    }
  }

  if (need_return) {
    if (AM_UNLIKELY(pack_msg(ipc_cmd_return, SET_IPC_MSG_UP_DIRECTION(last_cmd_id), last_cmd_result_data, AM_MAX_CMD_RESULT_SIZE) < 0)) { //pack the cmd id and argument into a IPC CMD
      ERROR("AMIPCCmdReceiver:process_msg, pack_msg failed!");
      return -1;
    }

    send(ipc_cmd_return, AM_MAX_IPC_MESSAGE_SIZE);
  }

  return 0;
}

int AMIPCCmdReceiver::register_msg_map(am_msg_handler_t *map, int num_entry)
{
  //the msg/action map will map  every CMD of method_call ,
  //to the action which is how to run the cmd.
  //it's expected after running the cmd, the return value is got.
  int ret = 0;
  do {
    if (AM_UNLIKELY(map == nullptr)) {
      PRINTF("AMIPCCmdReceiver::register_msg_map: null map \n");
      ret = -1;
      break;
    }

    if (AM_UNLIKELY((num_entry <= 0) || (num_entry > MAX_MESSAGES_IN_MAP))) {
      PRINTF("AMIPCCmdReceiver::register_msg_map:invalid num_entry %d \n",
             num_entry);
      ret = -1;
      break;
    }

    for (int i = 0; i < num_entry; i ++) {
      ret = register_msg_proc(&map[i]);
      if (AM_UNLIKELY(ret < 0)) {
        ERROR("AMIPCCmdReceiver::register_msg_map:register failed at %d!", i);
        ret = -1;
        break;
      }
    }
  } while (0);

  return ret;
}

int AMIPCCmdReceiver::notify(uint32_t cmd_id, void *cmd_arg_s, int cmd_size)
{
  if (AM_UNLIKELY(pack_msg(ipc_cmd, cmd_id, cmd_arg_s, cmd_size) < 0)) {
    PRINTF("AMIPCCmdReceiver: notify: pack_msg failed \n");
    return AM_IPC_CMD_ERR_INVALID_ARG;
  }

  if (AM_UNLIKELY(send(ipc_cmd,
                       sizeof(am_ipc_message_header_t) +
                       ipc_cmd->header.payload_size) < 0)) {
    ERROR("AMIPCCmdReceiver: notify: send error!");
    return AM_IPC_CMD_ERR_INVALID_IPC;
  }

  return AM_IPC_CMD_SUCCESS;
}

int AMIPCCmdReceiver::process_group_cmd(void)
{
  uint8_t *p_group_data;
  cmd_group_pack_header_t *p_pack_header =
      (cmd_group_pack_header_t*)last_cmd_param_data;
  cmd_group_result_pack_header_t *p_result_header = nullptr;
  cmd_pack_header_t *p_cmd_header = nullptr;

  //  SYS_IPC_MSG_CALLBACK msg_cb;
  am_msg_handler_t  msg_handler;

  uint8_t *ret_result_data = last_cmd_result_data;

  //for debug, check magic string
  if (AM_UNLIKELY(strncmp(p_pack_header->magic_string,
                          GROUP_PACK_MAGIC_STRING, 3))) {
    ERROR("AMIPCCmdReceiver:check group cmd magic string failed!");
    return -1;
  }

  if (AM_UNLIKELY(p_pack_header->cmd_num == 0)) {
    PRINTF("AMIPCCmdReceiver:Null cmds to process!");
    return -1;
  }

  //prepare for output
  p_result_header = (cmd_group_result_pack_header_t*)last_cmd_result_data;
  p_group_data = (uint8_t*)p_pack_header + sizeof(cmd_group_pack_header_t);

  for (int i = 0; i < p_pack_header->cmd_num; i ++) {
    p_cmd_header = (cmd_pack_header_t*)p_group_data;
    if (find_msg_handler(p_cmd_header->msg_id, &msg_handler) == 0) {// equal 0 means success/found
      if (msg_handler.context == 0) {
        msg_handler.callback (p_group_data + sizeof(cmd_pack_header_t),
                              AM_MAX_CMD_PARAM_SIZE, ret_result_data,
                              AM_MAX_CMD_RESULT_SIZE);
      } else {
        msg_handler.callback_ct(msg_handler.context,
                                p_group_data + sizeof(cmd_pack_header_t),
                                AM_MAX_CMD_PARAM_SIZE,
                                ret_result_data, AM_MAX_CMD_RESULT_SIZE);
      }
    } else {
      ERROR("AMIPCCmdReceiver: unsupported/unregistered cmd id 0x%x!",
             last_cmd_id);
      return -1;
    }
    //move to next cmd
    p_group_data += sizeof(cmd_pack_header_t) + p_cmd_header->input_size;

    //move result pointer
    ret_result_data += p_cmd_header->output_size;

    //update result header payload size
    p_result_header->payload_size += p_cmd_header->output_size;
  }

  if (AM_LIKELY(p_pack_header->cmd_num)) {
    //update result header total cmd num
    p_result_header->cmd_num = p_pack_header->cmd_num;
    strcpy(p_result_header->magic_string, GROUP_PACK_MAGIC_STRING);
  }

  return 0;
}
