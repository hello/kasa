/*
 * test_audio_service.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 19/12/2014 [Created]
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
#include <signal.h>
#include "am_base_include.h"
#include "am_log.h"

#include "am_api_helper.h"
#include "am_api_audio.h"
#include "am_audio_device_if.h"

static bool g_isrunning = true;

static const char *dev_str[] =
{
  "mic", "speaker", "playback stream", "record stream"
};

static void
sigstop (int32_t arg)
{
  INFO ("Signal has been captured, test_audio_device quits!");
  g_isrunning = false;
  exit (1);
}

static void
show_submenu (AM_AUDIO_DEVICE_TYPE type)
{
  int i = 0;
  int str_len = 0;

  printf ("\n============= %s operations ==============\n\n", dev_str[type]);
  printf (" d -- decrease volume\n");
  printf (" i -- increase volume\n");
  printf (" v -- fetch current volume\n");
  printf (" m -- mute\n");
  printf (" s -- sample rate\n");
  printf (" c -- channels\n");
  printf (" u -- unmute\n");
  printf (" n -- fetch name of %s\n", dev_str[type]);
  printf (" r -- return to main menu\n");
  printf ("\n==========================================");

  str_len = strlen (dev_str[type]);
  for (i = str_len - 2; i > 0; i--) {
    printf ("=");
  }

  printf ("\n\n");
}

static void
show_main_menu ()
{
  printf ("\n=============== oryx audio service test =================\n\n");
  printf (" m -- fetch or change properties of mic\n");
  printf (" s -- fetch or change properties of speaker\n");
  printf (" r -- fetch or change properties of recording stream\n");
  printf (" p -- fetch or change properties of playback streams\n");
  printf (" q -- quit\n");
  printf ("\n=========================================================\n\n");
}

static void
mainloop ()
{
  am_audio_device_t audio_device;
  am_audio_format_t audio_format;
  am_service_result_t service_result;
  int index_arr[AM_AUDIO_DEVICE_NUM_MAX];
  int arr_len = AM_AUDIO_DEVICE_NUM_MAX;
  bool sub_running = true;
  int i = 0;

  g_isrunning = true;
  show_main_menu();
  while (g_isrunning) {
    switch (getchar ()) {
    case 'm':
    case 'M': {
      audio_device.type = (uint8_t)AM_AUDIO_DEVICE_MIC;
    } break;

    case 's':
    case 'S': {
      audio_device.type = (uint8_t)AM_AUDIO_DEVICE_SPEAKER;
    } break;

    case 'r':
    case 'R': {
      audio_device.type = (uint8_t)AM_AUDIO_DEVICE_SOURCE_OUTPUT;
    } break;

    case 'p':
    case 'P': {
      audio_device.type = (uint8_t)AM_AUDIO_DEVICE_SINK_INPUT;
    } break;

    case 'q':
    case 'Q': {
      g_isrunning = false;
    } break;

    case '\n': {
      continue;
    } break;

    default: {
      INFO ("No such audio device, try again!");
      continue;
    } break;
    }

    if (!g_isrunning) {
      break;
    }

    /*
     * As we don't know how many audio device or stream on borad, we
     * need to get index list of audio device or stream. If there are
     * two or more devices or streams with the same type, we display
     * them on screen and let user choose it.
     */
    {
      AMAPIHelperPtr g_api_helper = nullptr;
      if ((g_api_helper = AMAPIHelper::get_instance ()) == nullptr) {
        ERROR ("Failed to get an instance of AMAPIHelper!");
        break;
      }
      g_api_helper->method_call(
          AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET,
          &audio_device,
          sizeof (am_audio_device_t),
          &service_result,
          sizeof (am_service_result_t));
    }
    if ((arr_len = service_result.ret) < 0) {
      ERROR ("Failed to get index list of %ss", dev_str[audio_device.type]);
      sub_running = false;
    } else {
      sub_running = true;
      memcpy (index_arr, service_result.data, arr_len * sizeof (int));

      if (arr_len == 1) {
        /* Currently, there is just one device or stream on board */
        audio_device.index = index_arr[0];
      } else {
        do {
          printf ("\n\nThere are two or more %ss on borad: \n\n",
                  dev_str[audio_device.type]);
          printf ("choice index name\n");
          AMAPIHelperPtr g_api_helper = nullptr;
          if ((g_api_helper = AMAPIHelper::get_instance ()) == nullptr) {
            ERROR ("Failed to get an instance of AMAPIHelper!");
            break;
          }
          for (i = 0; i < arr_len; i++) {
            audio_device.index = index_arr[i];
            g_api_helper->method_call(
                AM_IPC_MW_CMD_AUDIO_DEVICE_NAME_GET_BY_INDEX,
                &audio_device,
                sizeof (am_audio_device_t),
                &service_result,
                sizeof (am_service_result_t));
            if (service_result.ret < 0) {
              ERROR ("Failed to get device name!");
              sub_running = false; break;
            } else {
              memcpy (&audio_device,
                      service_result.data,
                      sizeof (am_audio_device_t));
              printf ("%6d %5d %s\n", i + 1, index_arr[i], audio_device.name);
            }

          }
          if (sub_running) {
            printf ("\nyour choice[1 - %d]: ", arr_len);
            scanf ("%d", &i);
            if (i <= 0 || i > arr_len) {
              PRINTF ("\nWrong choice, try again.!");
            } else {
              audio_device.index = index_arr[i - 1];
              break;
            }
          } else {
            break;
          }
        } while (true);
      }
    }

    if (sub_running) {
      show_submenu ((AM_AUDIO_DEVICE_TYPE)audio_device.type);
    }

    while (sub_running) {
      switch (getchar ()) {
        case 'D':
        case 'd': {
          /* When user want to decrease the volume of one device or stream,
           * current volume of this device or stream needs to read. Then
           * decrease the volume of this device or stream by 5%. If current
           * volume is equal to or less than 5%, 0 will be set.
           */
          AMAPIHelperPtr g_api_helper = nullptr;
          if ((g_api_helper = AMAPIHelper::get_instance ()) == nullptr) {
            ERROR ("Failed to get an instance of AMAPIHelper!");
            break;
          }
          g_api_helper->method_call(
              AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_INDEX,
              &audio_device,
              sizeof (am_audio_device_t),
              &service_result,
              sizeof (am_service_result_t));

          if (service_result.ret < 0) {
            ERROR ("Failed to get volume of %s!", dev_str[audio_device.type]);
            sub_running = false;
          } else {
            memcpy (&audio_device, service_result.data,
                    sizeof (am_audio_device_t));
            if (audio_device.volume > 5) {
              audio_device.volume -= 5;
            } else {
              audio_device.volume = 0;
            }

            g_api_helper->method_call(
                AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_INDEX,
                &audio_device,
                sizeof (am_audio_device_t),
                &service_result,
                sizeof (am_service_result_t));

            if (service_result.ret < 0) {
              ERROR ("Failed to decrease volume of %s!",
                     dev_str[audio_device.type]);
              sub_running = false;
            }
          }
        } break;

        case 'I':
        case 'i': {
          /* Like decreasing volume, speaker's volume will be increased by 5%. */
          AMAPIHelperPtr g_api_helper = nullptr;
          if ((g_api_helper = AMAPIHelper::get_instance ()) == nullptr) {
            ERROR ("Failed to get an instance of AMAPIHelper!");
            break;
          }
          g_api_helper->method_call(
              AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_INDEX,
              &audio_device,
              sizeof (am_audio_device_t),
              &service_result,
              sizeof (am_service_result_t));

          if (service_result.ret < 0) {
            ERROR ("Failed to get volume of %s!", dev_str[audio_device.type]);
            sub_running = false;
          } else {
            memcpy (&audio_device, service_result.data,
                    sizeof(am_audio_device_t));
            if (audio_device.volume < 95) {
              audio_device.volume += 5;
            } else {
              audio_device.volume = 100;
            }

            g_api_helper->method_call(
                AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_INDEX,
                &audio_device,
                sizeof (am_audio_device_t),
                &service_result,
                sizeof (am_service_result_t));

            if (service_result.ret < 0) {
              ERROR ("Failed to increase volume of %s!",
                     dev_str[audio_device.type]);
              sub_running = false;
            }
          }
        } break;

        case 'v':
        case 'V': {
          AMAPIHelperPtr g_api_helper = nullptr;
          if ((g_api_helper = AMAPIHelper::get_instance ()) == nullptr) {
            ERROR ("Failed to get an instance of AMAPIHelper!");
            break;
          }
          g_api_helper->method_call(
              AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_INDEX,
              &audio_device,
              sizeof (am_audio_device_t),
              &service_result,
              sizeof (am_service_result_t));

          if (service_result.ret < 0) {
            ERROR ("Failed to get volume of %s!", dev_str[audio_device.type]);
            sub_running = false;
          } else {
            memcpy (&audio_device,
                    service_result.data,
                    sizeof (am_audio_device_t));
            PRINTF ("current volume of %s: %d%\n", dev_str[audio_device.type],
                                                   audio_device.volume);
          }
        } break;

        case 'M':
        case 'm': {
          AMAPIHelperPtr g_api_helper = nullptr;
          if ((g_api_helper = AMAPIHelper::get_instance ()) == nullptr) {
            ERROR ("Failed to get an instance of AMAPIHelper!");
            break;
          }
          audio_device.mute = 1;
          g_api_helper->method_call(
              AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_INDEX,
              &audio_device,
              sizeof (am_audio_device_t),
              &service_result,
              sizeof (am_service_result_t));

          if (service_result.ret < 0) {
            ERROR ("Failed to mute %s!", dev_str[audio_device.type]);
            sub_running = false;
          }
        } break;

        case 'u':
        case 'U': {
          AMAPIHelperPtr g_api_helper = nullptr;
          if ((g_api_helper = AMAPIHelper::get_instance ()) == nullptr) {
            ERROR ("Failed to get an instance of AMAPIHelper!");
            break;
          }
          audio_device.mute = 0;
          g_api_helper->method_call(
              AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_INDEX,
              &audio_device,
              sizeof (am_audio_device_t),
              &service_result,
              sizeof (am_service_result_t));

          if (service_result.ret < 0) {
            ERROR ("Failed to store %s!", dev_str[audio_device.type]);
            sub_running = false;
          }
        } break;

        case 'S':
        case 's': {
          AMAPIHelperPtr g_api_helper = nullptr;
          if ((g_api_helper = AMAPIHelper::get_instance ()) == nullptr) {
            ERROR ("Failed to get an instance of AMAPIHelper!");
            break;
          }
          g_api_helper->method_call (AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_GET,
                                     nullptr,
                                     0,
                                     &service_result,
                                     sizeof (am_service_result_t));

          if (service_result.ret < 0) {
            ERROR ("Failed to get sample rate!");
            sub_running = false;
          } else {
            memcpy (&audio_format, service_result.data,
                    sizeof(am_audio_format_t));
            PRINTF ("Sample rate: %u", audio_format.sample_rate);
          }
        } break;

        case 'C':
        case 'c': {
          AMAPIHelperPtr g_api_helper = nullptr;
          if ((g_api_helper = AMAPIHelper::get_instance ()) == nullptr) {
            ERROR ("Failed to get an instance of AMAPIHelper!");
            break;
          }
          g_api_helper->method_call (AM_IPC_MW_CMD_AUDIO_CHANNEL_GET,
                                     nullptr,
                                     0,
                                     &service_result,
                                     sizeof (am_service_result_t));

          if (service_result.ret < 0) {
            ERROR ("Failed to get sample rate!");
            sub_running = false;
          } else {
            memcpy (&audio_format, service_result.data,
                    sizeof(am_audio_format_t));
            PRINTF ("Channels: %u", audio_format.channels);
          }
        } break;

        case 'n':
        case 'N': {
          AMAPIHelperPtr g_api_helper = nullptr;
          if ((g_api_helper = AMAPIHelper::get_instance ()) == nullptr) {
            ERROR ("Failed to get an instance of AMAPIHelper!");
            break;
          }
          g_api_helper->method_call(
              AM_IPC_MW_CMD_AUDIO_DEVICE_NAME_GET_BY_INDEX,
              &audio_device,
              sizeof (am_audio_device_t),
              &service_result,
              sizeof (am_service_result_t));

          if (service_result.ret < 0) {
            ERROR ("Failed to get device name!");
            sub_running = false; break;
          } else {
            memcpy (&audio_device,
                    service_result.data,
                    sizeof (am_audio_device_t));
            PRINTF ("Device name: %s", audio_device.name);
          }
        } break;

        case 'r':
        case 'R': {
          sub_running = false;
        } break;

        case '\n': {
          continue;
        } break;

        default:
          break;
      }

      show_submenu ((AM_AUDIO_DEVICE_TYPE)audio_device.type);
    }

    show_main_menu ();
  }
}

int
main (int argc, char **argv)
{
  signal (SIGINT , sigstop);
  signal (SIGTERM, sigstop);
  signal (SIGQUIT, sigstop);

  mainloop ();

  return 0;
}
