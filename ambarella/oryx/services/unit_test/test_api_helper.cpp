/*******************************************************************************
 * test_api_helper.cpp
 *
 * History:
 *   2014-9-28 - [lysun] created file
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
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include "am_base_include.h"
#include "am_log.h"
#include "am_api_helper.h"
#include "am_air_api.h"


static AMAPIHelperPtr api_helper = NULL;

static void sigstop(int arg)
{
  INFO("test api helper got signal and force quit\n");
  exit(1);
}

int main()
{
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  int count  = 1000;
  struct timeval time1, time2;
  api_helper = AMAPIHelper::get_instance();
  if (!api_helper) {
    ERROR("unable to get AMAPIHelper instance\n");
    return -1;
  }

  PRINTF("Now test image service with return value \n");
  am_service_result_t service_result;
  gettimeofday(&time1, NULL);
  api_helper->method_call(AM_IPC_MW_CMD_IMAGE_TEST, NULL, 0, &service_result,
                          sizeof(service_result));
  gettimeofday(&time2, NULL);
  PRINTF("ret = %d \n", service_result.ret);
  PRINTF("AVG method call takes %f ms\n ",
           ((time2.tv_sec - time1.tv_sec)* 1000000 +
           (time2.tv_usec - time1.tv_usec)) / count / 1000.f);

  return 0;
}
