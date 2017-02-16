/*******************************************************************************
 * test_am_log_main.cpp
 *
 * Histroy:
 *  2012-2-28 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include <pthread.h>

#include "test_am_log.h"

int main(int argc, char *argv[])
{
  if (argc != 3) {
    fprintf(stderr,
            "Usage: test_amlog [debug | error | warn | info]"
            " [stderr | syslog | null | file:filename]\n");
    ERROR("Parameter error!");
    exit(0);
  }
  if (is_str_equal(argv[1], "debug") ||
      is_str_equal(argv[1], "error") ||
      is_str_equal(argv[1], "warn")  ||
      is_str_equal(argv[1], "notice")||
      is_str_equal(argv[1], "info")) {
    if (false == set_log_level(argv[1])) {
      ERROR("Failed to set log level: %s!\n", argv[1]);
      exit(0);
    }
  } else {
    ERROR("Unrecognized log level %s\n", argv[1]);
    exit(0);
  }
  if (is_str_equal(argv[2], "syslog") ||
      is_str_equal(argv[2], "stderr") ||
      is_str_equal(argv[2], "null")   ||
      is_str_start_with(argv[2], "file:")) {
    if (false == set_log_target(argv[2])) {
      ERROR("Failed to set log target: %s!\n", argv[2]);
      exit (0);
    }
  } else {
    ERROR("Unrecognized log target %s\n", argv[2]);
    exit (0);
  }

  TestAmlogA A;
  TestAmlogB B;
  pthread_t threada;
  pthread_t threadb;
  if (0 != pthread_create(&threada,
                          NULL,
                          TestAmlogA::thread_entry,
                          (void *)&A)) {
    ERROR("Failed to create thread TestAmlogA");
    exit (0);
  }
  INFO("TestAmlogA thread created!");

  if (0 != pthread_create(&threadb,
                          NULL,
                          TestAmlogB::thread_entry,
                          (void *)&B)) {
    ERROR("Failed to create thread TestAmlogB");
    exit (0);
  }
  INFO("TestAmlogB thread created!");

  sleep(2);
  INFO("Before main loop!");
  for (int i = 0; i < 50; ++ i) {
    DEBUG("This is debug!");
    INFO("This is info!");
    NOTICE("This is notice!");
    ERROR("This is warning!");
    ERROR("This is error!");
  }
  INFO("After main loop!");
  INFO("Wait for thread A!");
  pthread_join(threada, NULL) == 0 ? INFO("Thread A exited successfully!") :
      ERROR("Wait for thread A failed: %s", strerror(errno));
  INFO("Wait for thread B!");
  pthread_join(threadb, NULL) == 0 ? INFO("Thread B exited successfully!") :
      ERROR("Wait for thread B failed: %s", strerror(errno));

  INFO("test_amlog exit!");

  return 0;
}
