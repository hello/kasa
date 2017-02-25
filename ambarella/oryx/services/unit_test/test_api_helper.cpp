/*******************************************************************************
 * test_api_helper.cpp
 *
 * History:
 *   Aug 9, 2016 - [Shupeng Ren] created file
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

#include <stdio.h>
#include <signal.h>
#include <iostream>

#include "am_base_include.h"
#include "uuid.h"
#include "am_log.h"

#include "am_api_helper.h"
#include "am_api_video.h"

#define MAX_STREAM_NUM 4
static void show_main_menu(bool prompt)
{
  if (!prompt) {
    printf("> ");
    return;
  }

  PRINTF("\n");
  PRINTF("================================================");
  PRINTF("  l -- Lock stream");
  PRINTF("  u -- Unlock stream");
  PRINTF("  g -- Get streams lock state");
  PRINTF("  q -- Quit");
  PRINTF("================================================\n\n");
  printf("> ");
}

static void signal_handler(int sig)
{

}

int main(int argc, char **argv)
{
  signal(SIGINT, signal_handler);
  signal(SIGQUIT, signal_handler);
  signal(SIGTERM, signal_handler);

  int ret = 0;
  char ch = 0;
  bool prompt = true;
  bool run_flag = true;
  bool show_menu = true;
  timeval time1 = {0}, time2 = {0};
  AMAPIHelperPtr helper = nullptr;

  do {
    if (!(helper = AMAPIHelper::get_instance())) {
      ret = -1;
      ERROR("Failed to get AMAPIHelper instance!");
      break;
    }

    while (run_flag) {
      if (show_menu) {
        show_main_menu(prompt);
        prompt = true;
      } else {
        show_menu = true;
      }

      switch (ch = getchar()) {
        case 'l': {
          am_service_result_t service_result = {0};
          am_stream_lock_t lock_cfg = {0};
          lock_cfg.operation = 1;
          PRINTF("Please input stream ID[0 - 3] you want to lock:");
          scanf("%d", &lock_cfg.stream_id);
          if (lock_cfg.stream_id < 0 || lock_cfg.stream_id > 3) {
            ERROR("Invalid stream ID: %d", lock_cfg.stream_id);
            break;
          }
          std::string in_str;
          PRINTF("\nPlease input UUID string:");
          std::cin >> in_str;
          if (in_str.length() != 36) {
            ERROR("Invalid UUID string: %s", in_str.c_str());
            break;
          }
          uuid_parse(in_str.c_str(), lock_cfg.uuid);

          gettimeofday(&time1, nullptr);
          helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK,
                              &lock_cfg, sizeof(lock_cfg),
                              &service_result, sizeof(service_result));
          gettimeofday(&time2, nullptr);
          PRINTF("method_call() takes %.3f ms\n ",
                 ((time2.tv_sec - time1.tv_sec) * 1000000 +
                     (time2.tv_usec - time1.tv_usec)) / 1000.f);
          am_stream_lock_t *lock_result = (am_stream_lock_t*)service_result.data;
          if ((ret = service_result.ret) == -1) {
            ERROR("Failed to do stream lock operation!");
          } else if (ret == -2 || ret == -3) {
            run_flag = false;
            break;
          } else {
            if (!lock_result->op_result) {
              PRINTF("\nStream[%d] lock operation is OK!\n\n",
                     lock_result->stream_id);
            } else {
              PRINTF("\nStream[%d] is occupied by another APP!\n\n",
                     lock_result->stream_id);
            }
          }
        } break;
        case 'u': {
          am_service_result_t service_result = {0};
          am_stream_lock_t lock_cfg = {0};
          lock_cfg.operation = 0;
          PRINTF("Please input stream ID[0 - 3] you want to unlock:");
          scanf("%d", &lock_cfg.stream_id);
          if (lock_cfg.stream_id < 0 || lock_cfg.stream_id > 3) {
            ERROR("Invalid stream ID: %d", lock_cfg.stream_id);
            break;
          }
          std::string in_str;
          PRINTF("\nPlease input UUID string:");
          std::cin >> in_str;
          if (in_str.length() != 36) {
            ERROR("Invalid UUID string: %s", in_str.c_str());
            break;
          }
          uuid_parse(in_str.c_str(), lock_cfg.uuid);
          gettimeofday(&time1, nullptr);
          helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK,
                              &lock_cfg, sizeof(lock_cfg),
                              &service_result, sizeof(service_result));
          gettimeofday(&time2, nullptr);
          PRINTF("method_call() takes %.3f ms\n ",
                 ((time2.tv_sec - time1.tv_sec) * 1000000 +
                     (time2.tv_usec - time1.tv_usec)) / 1000.f);
          am_stream_lock_t *lock_result = (am_stream_lock_t*)service_result.data;
          if ((ret = service_result.ret) == -1) {
            ERROR("Failed to do stream lock operation!");
          } else if (ret == -2 || ret == -3) {
            run_flag = false;
            break;
          } else {
            if (!lock_result->op_result) {
              PRINTF("\nStream[%d] lock operation is OK!\n\n",
                     lock_result->stream_id);
            } else {
              PRINTF("\nStream[%d] is occupied by another APP!\n\n",
                     lock_result->stream_id);
            }
          }
        } break;
        case 'g': {
          am_service_result_t service_result = {0};
          gettimeofday(&time1, nullptr);
          helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK_STATE_GET,
                              nullptr, 0,
                              &service_result, sizeof(service_result));
          gettimeofday(&time2, nullptr);
          PRINTF("method_call() takes %.3f ms\n ",
                 ((time2.tv_sec - time1.tv_sec) * 1000000 +
                     (time2.tv_usec - time1.tv_usec)) / 1000.f);
          if ((ret = service_result.ret) == -1) {
            ERROR("Failed to do stream lock operation!");
          } else if (ret == -2 || ret == -3) {
            run_flag = false;
            break;
          }
          uint32_t *lock_bitmap = (uint32_t*)service_result.data;

          for (uint32_t i = 0; i < MAX_STREAM_NUM; ++i) {
            if (*lock_bitmap & (1 << i)) {
              PRINTF("Stream[%d] lock state: locked.", i);
            } else {
              PRINTF("Stream[%d] lock state: free.", i);
            }
          }
        } break;
        case 'q': {
          run_flag = false;
        } break;
        case '\n': {
          prompt = false;
        } break;
        default: {
          show_menu = false;
        } break;
      }
      if (run_flag && prompt) {
        getchar();
      }
    }
  } while (0);

  return ret;
}
