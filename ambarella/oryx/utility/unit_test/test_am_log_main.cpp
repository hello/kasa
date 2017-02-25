/*******************************************************************************
 * test_am_log_main.cpp
 *
 * Histroy:
 *  2012-2-28 2012 - [ypchang] created file
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
