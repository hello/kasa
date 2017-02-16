
/*
 * am_ipc_cmd_group.cpp
 *
 * History:
 *    2014/09/09 - [Louis Sun] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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
{
  //cmd input group pack
  memset(&cmd_group, 0, sizeof(cmd_group));

  //cmd result
  memset(&cmd_group_result, 0, sizeof(cmd_group_result));

  cmd_sender = NULL;

  batch_state = 0;
}

AMIPCCmdGroupClient::~AMIPCCmdGroupClient()
{
  delete[]   cmd_group.start;
  delete[]  cmd_group_result.result_buffer;
}

int32_t AMIPCCmdGroupClient::create()
{
  if (cmd_group.start) {
    PRINTF("AMIPCCmdGroupClient: already created, cannot create again \n");
    return -1;
  }

  cmd_group.start = new int8_t[cmd_group_pack_maxsize];
  if (!cmd_group.start) {
    PRINTF("AMIPCCmdGroupClient: create cmd group failed \n");
    return -1;
  }

  cmd_group_result.result_buffer = new int8_t[cmd_group_ret_maxsize];
  if (!cmd_group_result.result_buffer) {
    PRINTF("AMIPCCmdGroupClient: create cmd result failed \n");
    return -1;
  }

  return 0;
}

int32_t AMIPCCmdGroupClient::check_cmd(int32_t  function_id,  void *input_arg,  int32_t input_size,  void *output_arg, int32_t output_maxsize)
{
  if (AM_UNLIKELY(!GET_IPC_MSG_NEED_RETURN(function_id))) {
    PRINTF("AMIPCCmdGroupClient: cannot add cmd without return \n");
    return AM_CMD_GROUP_INVALID_ARG;
  }

  if (AM_UNLIKELY( !cmd_group.start)) {
    PRINTF("AMIPCCmdGroupClient: group not created yet. cannot add \n");
    return AM_CMD_GROUP_ERROR;
  }

  if (AM_UNLIKELY(((!input_arg) && (input_size)) ||  ((input_arg) && (!input_size)))) {
    PRINTF("AMIPCCmdGroupClient: invalid argument\n");
    return AM_CMD_GROUP_INVALID_ARG;
  }

  if  (AM_UNLIKELY(((!output_arg) && (output_maxsize)) ||  ((output_arg) && (!output_maxsize)))) {
    PRINTF("AMIPCCmdGroupClient: invalid argument\n");
    return AM_CMD_GROUP_INVALID_ARG;
  }

  if   (AM_UNLIKELY((input_size & 0x3) ||(output_maxsize & 0x3) || ((uint32_t)input_arg & 0x3) || ((uint32_t)output_arg & 0x3))) {
    PRINTF("AMIPCCmdGroupClient: input size %d or output size %d or input_arg/output_arg addr not aligned to 32-bit\n", input_size, output_maxsize);
    return AM_CMD_GROUP_INVALID_ARG;
  }

  if (AM_UNLIKELY((cmd_group.cmd_num >= AM_MAX_NUM_CMDS_IN_GROUP))) {
    PRINTF("AMIPCCmdGroupClient: group full, current cmd num is %d \n", cmd_group.cmd_num);
    return AM_CMD_GROUP_CMD_NUM_EXCEEED_LIMIT;
  }

  if (AM_UNLIKELY(cmd_group.current + input_size > cmd_group.start + cmd_group_pack_maxsize)) {
    PRINTF("AMIPCCmdGroupClient: cmd input value size exceeds limit , total size reach %d \n",  cmd_group.current + input_size - cmd_group.start );
    return AM_CMD_GROUP_INPUT_EXCEEED_LIMIT;
  }

  if (AM_UNLIKELY(cmd_group_result.cmd_ret_total_size + output_maxsize >  cmd_group_ret_maxsize)) {
    PRINTF("AMIPCCmdGroupClient: cmd ret(output) value size exceeds limit , total size reach %d \n", cmd_group_result.cmd_ret_total_size + output_maxsize);
    return AM_CMD_GROUP_OUTPUT_EXCEEED_LIMIT;
  }

  return 0;
}

int32_t AMIPCCmdGroupClient::batch_cmd_add(int32_t  function_id,  void *input_arg,  int32_t input_size,  void *output_arg, int32_t output_maxsize)
{
  cmd_pack_header_t *p_msg_header ;
  int32_t ret;

  if (!batch_state) {
    PRINTF("AMIPCCmdGroupClient: not in batch, should call batch_cmd_begin in advance\n");
    return -1;
  }

  //check the cmd
  if ((ret = check_cmd(function_id, input_arg, input_size, output_arg, output_maxsize)) < 0) {
    return ret;
  }

  //add cmd_pack_header_t first (please note that this is not ipc header
  p_msg_header = (cmd_pack_header_t *)cmd_group.current;
  p_msg_header->msg_id = function_id;
  p_msg_header->input_size = input_size;
  p_msg_header->output_size = output_maxsize;

  //PRINTF("batch_cmd_add: msg_id =0x%x,  input_size = %d,  output_size =%d \n",  function_id, input_size, output_maxsize);

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

  //for debug
  //PRINTF("AMIPCCmdGroupClient: num of cmds %d, total input size %d,  total ret size %d\n",  cmd_group.cmd_num, cmd_group.current - cmd_group.start,
  //cmd_group_result.cmd_ret_total_size );

  return AM_CMD_GROUP_NO_ERROR;
}

int32_t AMIPCCmdGroupClient::set_cmd_sender(AMIPCSyncCmdClient *p_cmd_sender)
{
  if (!p_cmd_sender)
    return -1;
  cmd_sender = p_cmd_sender;
  return 0;
}

int32_t AMIPCCmdGroupClient::parse_return_result()
{
  int32_t i;
  int8_t *p_return_data;
  cmd_group_result_pack_header_t  * p_result_pack_header =  (cmd_group_result_pack_header_t  *)cmd_group_result.result_buffer;
  if (p_result_pack_header->cmd_num!= cmd_group.cmd_num) {
    PRINTF("AMIPCCmdGroupClient::parse_return_result:cmd mismatch \n");
    return -1;
  }

  //check magic string  (usually used for debugging)
  if (strncmp(p_result_pack_header->magic_string, GROUP_PACK_MAGIC_STRING,
  	sizeof(p_result_pack_header->magic_string))) {
    PRINTF("AMIPCCmdGroupClient::parse_return_result:magic string mismatch \n");
    return -1;
  }

  p_return_data = cmd_group_result.result_buffer + sizeof(cmd_group_result_pack_header_t);

  //copy the returned data structure to original address
  for (i= 0; i<p_result_pack_header->cmd_num; i++) {
    memcpy(cmd_group_result.cmd_ret_item[i].output_arg, p_return_data,
           cmd_group_result.cmd_ret_item[i].output_size);
    p_return_data += cmd_group_result.cmd_ret_item[i].output_size;
  }

  return 0;
}

int32_t AMIPCCmdGroupClient::batch_cmd_begin()
{
  if (batch_state) {
    PRINTF("AMIPCCmdGroupClient: already in batch \n");
    return -1;
  }

  if  ((!cmd_group.start) || (!cmd_group_result.result_buffer)) {
    PRINTF("AMIPCCmdGroupClient: group not created yet. cannot reset \n");
    return -1;
  }

  //input
  cmd_group.cmd_num = 0;
  memset(cmd_group.start, 0, cmd_group_pack_maxsize);
  cmd_group.current = cmd_group.start;

  //ouput
  cmd_group_result.cmd_ret_total_size = 0;
  memset(cmd_group_result.result_buffer, 0, cmd_group_ret_maxsize);
  memset(cmd_group_result.cmd_ret_item, 0, AM_MAX_NUM_CMDS_IN_GROUP* sizeof(cmd_ret_item_t));

  //now add CMD header for aggregation.
  prepare_cmd_pack();

  batch_state = 1;  //now it's in batch state

  return 0;
}

int32_t AMIPCCmdGroupClient::batch_cmd_exec()
{

  if (!batch_state) {
    PRINTF("AMIPCCmdGroupClient: not in batch, should call batch_cmd_begin in advance\n");
    return -1;
  }

  if (!cmd_group.start) {
    PRINTF("AMIPCCmdGroupClient: group not created yet. cannot exec \n");
    return -1;
  }

  if (!cmd_sender) {
    PRINTF("AMIPCCmdGroupClient::exec: should set_cmd_sender in advance \n");
    return -1;
  }

  if (!cmd_group.cmd_num) {
    PRINTF("AMIPCCmdGroupClient::exec: zero cmds to exec in batch \n");
    return -1;
  }

  //update header to reflect the all of cmds added
  update_cmd_pack();
  //now process it.

  if(cmd_sender->method_call(AM_IPC_CMD_ID_GROUP_CMD_GENERIC, cmd_group.start,
                             cmd_group.current - cmd_group.start,
                             cmd_group_result.result_buffer,
                             cmd_group_result.cmd_ret_total_size) < 0) {
    PRINTF("AMIPCCmdGroupClient::exec method_call failed \n");
    return -1;
  }

  //fill return values
  if (parse_return_result() < 0) {
    PRINTF("AMIPCCmdGroupClient::exec  parse_return_results failed \n");
    return -1;
  }

  //if everything is done, clear batch state
  batch_state = 0;

  return 0;
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
  cmd_group_pack_header_t *group_pack_header = (cmd_group_pack_header_t *)cmd_group.start;

  //update cmd pack header  ( cmd pack header and cmd pack are "payload" of cmd)
  group_pack_header->cmd_num = cmd_group.cmd_num;
  //the first cmd which is GROUP CMD's payload is all aggregated cmds below
  group_pack_header->payload_size = (cmd_group.current - cmd_group.start) - sizeof(cmd_group_pack_header_t);

  //for debug
  //PRINTF("AMIPCCmdGroupClient::update_pack_header: total num packed %d,  payload size %d \n",
  //group_pack_header->cmd_num, group_pack_header->payload_size);
  return 0;
}
