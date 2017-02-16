/*******************************************************************************
 * am_network_service_msg_action.cpp
 *
 * History:
 *   2014-9-12 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "am_base_include.h"
#include "am_log.h"
#include "commands/am_service_impl.h"
#include "am_network_service_priv.h"


void ON_SERVICE_INIT(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  //add real service init here



  //put init result to result_addr
  am_service_result_t *service_result = (am_service_result_t *)result_addr;
  //if it's init done, then
  service_result->ret = 0;
  service_result->state = g_service_state;
  if (service_result->state == AM_SERVICE_STATE_INIT_DONE)
    INFO("Network Service Init Done \n");
  else
    INFO("Network Service is doing init... \n");
}


void ON_SERVICE_DESTROY(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  INFO("video service destroy, cleanup\n");
  clean_up();
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{

}

void ON_SERVICE_STOP(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
}

void ON_SERVICE_RESTART(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  INFO("video service restart\n");
}
void ON_SERVICE_STATUS(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  INFO("video service get status\n");
}

