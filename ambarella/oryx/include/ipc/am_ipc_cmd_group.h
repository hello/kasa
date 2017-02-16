/*
 * ipc_cmd_group.h
 *
 * History:
 *    2013/2/22 - [Louis Sun] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

/*! @file am_ipc_cmd_group.h
 *  @brief This file defines AMIPCCmdGroupClient.
 */
#ifndef  AM_IPC_CMD_GROUP_H__
#define  AM_IPC_CMD_GROUP_H__

#include "am_ipc_cmd.h"
/*
ipc_cmd_header  will use following payload

AMIPCCmdGroupClient  defines the cmd payload, this class aggregate the cmd_id and arg
into its payload. and the IPC message AIR_MW_GROUP_CMD_GENERIC.

 the cmd input arg payload is like below:
#########################################################
#                                        # cmd_pack_header#cmd_pack_header#cmd_header #
#cmd_group_pack_header_t # cmd_arg1            # cmd_arg2           #cmd_arg3    #  ...
#                                        #                           #                          #                   #
#########################################################

 the cmd output result payload is like below:  (no cmd_header any more in results)
######################################################
#                                        #                  #                   #                 #
#cmd_group_pack_header_t # cmd_result1# cmd_result2#cmd_result3#  ...
#                                        #                   #                  #                 #
######################################################
 */

#define AM_MAX_NUM_CMDS_IN_GROUP 8 //max num of groups to put in single GROUP

typedef struct cmd_group_pack_s{
    int8_t *start;
    int8_t *current;//(current - start) is current packed size
    int32_t cmd_num;
} cmd_group_pack_t;

typedef struct cmd_ret_item_s {
    void *output_arg;
    int32_t     output_size;
} cmd_ret_item_t;

typedef struct cmd_pack_header_s {
    uint32_t msg_id; //got from macro BUILD_CMD
    uint16_t input_size;
    uint16_t output_size;
} cmd_pack_header_t;

typedef struct cmd_group_ret_result_s {
    int32_t cmd_ret_total_size;
    cmd_ret_item_t cmd_ret_item[AM_MAX_NUM_CMDS_IN_GROUP];
    int8_t * result_buffer;
} cmd_group_ret_result_t;

typedef enum{
  AM_CMD_GROUP_NO_ERROR = 0,
  AM_CMD_GROUP_ERROR = -1,
  AM_CMD_GROUP_INPUT_EXCEEED_LIMIT = -2,
  AM_CMD_GROUP_OUTPUT_EXCEEED_LIMIT = -3,
  AM_CMD_GROUP_CMD_NUM_EXCEEED_LIMIT = -4,
  AM_CMD_GROUP_INVALID_ARG = -5,
} AM_CMD_GROUP_RETURN_VALUE;

#define GROUP_PACK_MAGIC_STRING "PAC"
typedef struct cmd_group_pack_header_s {
    int32_t cmd_num;
    int32_t payload_size;
    char   magic_string[4]; /* PAC */   //in order to do simple check about the return value
} cmd_group_pack_header_t;

typedef struct cmd_group_pack_header_s cmd_group_result_pack_header_t;

class AMIPCSyncCmdClient;
class AMIPCCmdGroupClient {
  private:
    //ipc_message_header_t ipc_cmd_header;

    static const int32_t cmd_group_pack_maxsize = sizeof(am_ipc_message_payload_t);
    static const int32_t cmd_group_ret_maxsize = sizeof(am_ipc_message_payload_t);

    cmd_group_pack_t  cmd_group;
    cmd_group_ret_result_t  cmd_group_result;

    int32_t prepare_cmd_pack();
    int32_t update_cmd_pack();
    int32_t check_cmd(int32_t function_id,
                  void *input_arg,
                  int32_t input_size,
                  void *output_arg,
                  int32_t output_maxsize);
    int32_t parse_return_result();

    AMIPCSyncCmdClient *cmd_sender;

    int32_t batch_state; //0: not in batch   1: in batch processing

  public:
    AMIPCCmdGroupClient();
    ~AMIPCCmdGroupClient();

    int32_t create();   //real constructor, can only be called once
    int32_t set_cmd_sender(AMIPCSyncCmdClient *p_cmd_sender);   //associate the cmd sender to it. can change it after set.


    int32_t batch_cmd_begin();
    int32_t batch_cmd_add(int32_t  function_id,
                      void *input_arg,
                      int32_t input_size,
                      void *output_arg,
                      int32_t output_maxsize);
    int32_t batch_cmd_exec();
};

#endif //AM_IPC_CMD_GROUP_H__
