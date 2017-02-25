
/*
 * am_ipc_cmd_group.cpp
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
#include <string.h>
#include "am_base_include.h"
#include "am_ipc_cmd_group.h"
#include "am_define.h"
#include "am_log.h"
#include "am_ipc_types.h"
#include "am_ipc_sync_cmd.h"

AMIPCCmdGroupClient::AMIPCCmdGroupClient()
{}

AMIPCCmdGroupClient::~AMIPCCmdGroupClient()
{
  delete[] cmd_group.start;
  delete[] cmd_group_result.result_buffer;
}

int32_t AMIPCCmdGroupClient::create()
{
  int32_t ret = 0;
  do {
    if (cmd_group.start) {
      PRINTF("AMIPCCmdGroupClient: already created, cannot create again!");
      ret = -1;
      break;
    }

    if (!(cmd_group.start = new int8_t[cmd_group_pack_maxsize])) {
      ERROR("AMIPCCmdGroupClient: create cmd group failed!");
      ret = -1;
      break;
    }

    cmd_group_result.result_buffer = new int8_t[cmd_group_ret_maxsize];
    if (!cmd_group_result.result_buffer) {
      ERROR("AMIPCCmdGroupClient: create cmd result failed!");
      ret = -1;
      break;
    }
  } while (0);

  return ret;
}

int32_t AMIPCCmdGroupClient::check_cmd(int32_t function_id,
                                       void *input_arg, int32_t input_size,
                                       void *output_arg, int32_t output_maxsize)
{
  int32_t ret = 0;
  do {
    if (AM_UNLIKELY(!GET_IPC_MSG_NEED_RETURN(function_id))) {
      ERROR("AMIPCCmdGroupClient: cannot add cmd without return!");
      ret = AM_CMD_GROUP_INVALID_ARG;
      break;
    }

    if (AM_UNLIKELY(!cmd_group.start)) {
      ERROR("AMIPCCmdGroupClient: group not created yet. cannot add!");
      ret = AM_CMD_GROUP_ERROR;
      break;
    }

    if (AM_UNLIKELY(((!input_arg) && (input_size)) || ((input_arg) && (!input_size)))) {
      ERROR("AMIPCCmdGroupClient: invalid argument!");
      ret = AM_CMD_GROUP_INVALID_ARG;
      break;
    }

    if (AM_UNLIKELY(((!output_arg) && (output_maxsize)) || ((output_arg) && (!output_maxsize)))) {
      ERROR("AMIPCCmdGroupClient: invalid argument!");
      ret = AM_CMD_GROUP_INVALID_ARG;
      break;
    }

    if (AM_UNLIKELY((input_size & 0x3) ||(output_maxsize & 0x3) || ((uint32_t)input_arg & 0x3) || ((uint32_t)output_arg & 0x3))) {
      ERROR("AMIPCCmdGroupClient: input size %d or output size %d or input_arg/output_arg addr not aligned to 32-bit!",
            input_size, output_maxsize);
      ret = AM_CMD_GROUP_INVALID_ARG;
      break;
    }

    if (AM_UNLIKELY((cmd_group.cmd_num >= AM_MAX_NUM_CMDS_IN_GROUP))) {
      ERROR("AMIPCCmdGroupClient: group full, current cmd num is %d!",
            cmd_group.cmd_num);
      ret = AM_CMD_GROUP_CMD_NUM_EXCEEED_LIMIT;
      break;
    }

    if (AM_UNLIKELY(cmd_group.current + input_size > cmd_group.start + cmd_group_pack_maxsize)) {
      ERROR("AMIPCCmdGroupClient: cmd input value size exceeds limit, total size reach %d!",
            cmd_group.current + input_size - cmd_group.start );
      ret = AM_CMD_GROUP_INPUT_EXCEEED_LIMIT;
      break;
    }

    if (AM_UNLIKELY(cmd_group_result.cmd_ret_total_size + output_maxsize >  cmd_group_ret_maxsize)) {
      ERROR("AMIPCCmdGroupClient: cmd ret(output) value size exceeds limit , total size reach %d!",
            cmd_group_result.cmd_ret_total_size + output_maxsize);
      ret = AM_CMD_GROUP_OUTPUT_EXCEEED_LIMIT;
      break;
    }
  } while (0);

  return ret;
}

int32_t AMIPCCmdGroupClient::batch_cmd_add(int32_t function_id,
                                           void *input_arg, int32_t input_size,
                                           void *output_arg, int32_t output_maxsize)
{
  cmd_pack_header_t *p_msg_header = nullptr;
  int32_t ret = AM_CMD_GROUP_NO_ERROR;

  do {
    if (!batch_state) {
      ERROR("AMIPCCmdGroupClient: not in batch, should call batch_cmd_begin in advance!");
      ret = -1;
      break;
    }

    if ((ret = check_cmd(function_id,
                         input_arg, input_size,
                         output_arg, output_maxsize)) < 0) {
      break;
    }

    //add cmd_pack_header_t first (please note that this is not ipc header
    p_msg_header = (cmd_pack_header_t*)cmd_group.current;
    p_msg_header->msg_id = function_id;
    p_msg_header->input_size = input_size;
    p_msg_header->output_size = output_maxsize;

    //advance pointer and add possible "cmd payload"
    cmd_group.current += sizeof(cmd_pack_header_t);
    if (p_msg_header->input_size > 0) {
      memcpy(cmd_group.current, input_arg, input_size);
      cmd_group.current += p_msg_header->input_size;
    }

    //process the output pointers, backup the output addr into cmd_ret_data array,
    cmd_group_result.cmd_ret_item[cmd_group.cmd_num].output_arg = output_arg;
    cmd_group_result.cmd_ret_item[cmd_group.cmd_num].output_size = output_maxsize;
    //update the total output size
    cmd_group_result.cmd_ret_total_size += output_maxsize;

    //now all is done, update counter
    cmd_group.cmd_num++;
  } while (0);
  return ret;
}

int32_t AMIPCCmdGroupClient::set_cmd_sender(AMIPCSyncCmdClient *p_cmd_sender)
{
  return p_cmd_sender ? (cmd_sender = p_cmd_sender, 0) : -1;
}

int32_t AMIPCCmdGroupClient::parse_return_result()
{
  int32_t ret = 0;
  int8_t *p_return_data = nullptr;
  do {
    cmd_group_result_pack_header_t *p_result_pack_header =
        (cmd_group_result_pack_header_t*)cmd_group_result.result_buffer;
    if (p_result_pack_header->cmd_num!= cmd_group.cmd_num) {
      ERROR("AMIPCCmdGroupClient::parse_return_result:cmd mismatch!");
      ret = -1;
      break;
    }

    //check magic string  (usually used for debugging)
    if (strncmp(p_result_pack_header->magic_string,
                GROUP_PACK_MAGIC_STRING,
                sizeof(p_result_pack_header->magic_string))) {
      ERROR("AMIPCCmdGroupClient::parse_return_result:magic string mismatch!");
      ret = -1;
      break;
    }

    p_return_data = cmd_group_result.result_buffer +
        sizeof(cmd_group_result_pack_header_t);

    //copy the returned data structure to original address
    for (int32_t i = 0; i < p_result_pack_header->cmd_num; i++) {
      memcpy(cmd_group_result.cmd_ret_item[i].output_arg,
             p_return_data,
             cmd_group_result.cmd_ret_item[i].output_size);
      p_return_data += cmd_group_result.cmd_ret_item[i].output_size;
    }
  } while (0);
  return ret;
}

int32_t AMIPCCmdGroupClient::batch_cmd_begin()
{
  int32_t ret = 0;
  do {
    if (batch_state) {
      PRINTF("AMIPCCmdGroupClient: already in batch!");
      ret = -1;
      break;
    }

    if ((!cmd_group.start) || (!cmd_group_result.result_buffer)) {
      ERROR("AMIPCCmdGroupClient: group not created yet. cannot reset!");
      ret = -1;
      break;
    }

    //input
    cmd_group.cmd_num = 0;
    memset(cmd_group.start, 0, cmd_group_pack_maxsize);
    cmd_group.current = cmd_group.start;

    //ouput
    cmd_group_result.cmd_ret_total_size = 0;
    memset(cmd_group_result.result_buffer, 0, cmd_group_ret_maxsize);
    memset(cmd_group_result.cmd_ret_item, 0,
           AM_MAX_NUM_CMDS_IN_GROUP* sizeof(cmd_ret_item_t));

    //now add CMD header for aggregation.
    prepare_cmd_pack();

    batch_state = 1;  //now it's in batch state
  } while (0);
  return ret;
}

int32_t AMIPCCmdGroupClient::batch_cmd_exec()
{
  int32_t ret = 0;
  do {
    if (!batch_state) {
      PRINTF("AMIPCCmdGroupClient: not in batch, should call batch_cmd_begin in advance!");
      ret = -1;
      break;
    }

    if (!cmd_group.start) {
      PRINTF("AMIPCCmdGroupClient: group not created yet. cannot exec!");
      ret = -1;
      break;
    }

    if (!cmd_sender) {
      PRINTF("AMIPCCmdGroupClient::exec: should set_cmd_sender in advance!");
      ret = -1;
      break;
    }

    if (!cmd_group.cmd_num) {
      PRINTF("AMIPCCmdGroupClient::exec: zero cmds to exec in batch \n");
      ret = -1;
      break;
    }

    //update header to reflect the all of cmds added
    update_cmd_pack();
    //now process it.
    if(cmd_sender->method_call(AM_IPC_CMD_ID_GROUP_CMD_GENERIC, cmd_group.start,
                               cmd_group.current - cmd_group.start,
                               cmd_group_result.result_buffer,
                               cmd_group_result.cmd_ret_total_size) < 0) {
      ERROR("AMIPCCmdGroupClient::exec method_call failed!");
      ret = -1;
      break;
    }

    //fill return values
    if (parse_return_result() < 0) {
      ERROR("AMIPCCmdGroupClient::exec parse_return_results failed!");
      ret = -1;
      break;
    }

    //if everything is done, clear batch state
    batch_state = 0;
  } while (0);
  return ret;
}

int32_t AMIPCCmdGroupClient::prepare_cmd_pack()
{
  cmd_group_pack_header_t *group_pack_header;

  //here is cmd pack header ( cmd pack header and cmd pack are "payload" of cmd)
  group_pack_header = (cmd_group_pack_header_t*)cmd_group.start;
  strcpy(group_pack_header->magic_string, GROUP_PACK_MAGIC_STRING);
  //advance pointer
  cmd_group.current += sizeof(cmd_group_pack_header_t);

  return 0;
}

int32_t AMIPCCmdGroupClient::update_cmd_pack()
{
  cmd_group_pack_header_t *group_pack_header = (cmd_group_pack_header_t*)cmd_group.start;

  //update cmd pack header (cmd pack header and cmd pack are "payload" of cmd)
  group_pack_header->cmd_num = cmd_group.cmd_num;
  //the first cmd which is GROUP CMD's payload is all aggregated cmds below
  group_pack_header->payload_size =
      (cmd_group.current - cmd_group.start) - sizeof(cmd_group_pack_header_t);
  return 0;
}
