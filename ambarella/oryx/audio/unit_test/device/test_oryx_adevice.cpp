/*
 * test_oryx_adevice.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 28/06/2013 [Created]
 *          11/12/2014 [Modifyed]
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
#include "am_base_include.h"
#include "am_log.h"
#include "am_audio_device_if.h"

#ifndef AUDIO_DEVICE_AMOUNT_MAX
#define AUDIO_DEVICE_AMOUNT_MAX 32
#endif

#ifndef AUDIO_DEVICE_NAME_MAX_LEN
#define AUDIO_DEVICE_NAME_MAX_LEN 256
#endif

/*
 * This test program just checks whether APIs declared
 * in audio deivce works normally. Before running this
 * program, user needs to ensure that there is sound
 * in speaker device, so features in audio device, such
 * as increasing or decreasing volume, can be tested.
 *
 * User can play an audio media using test_oryx_playback.
 */
AMIAudioDevicePtr adev;
bool              g_isrunning;
int               volume_before_mute;

static const char *dev_str[] =
{
  "mic", "speaker", "playback stream", "record stream"
};

static void
show_submenu (AM_AUDIO_DEVICE_TYPE dev)
{
  int i = 0;
  int str_len = 0;

  printf ("\n============= %s operations ==============\n\n", dev_str[dev]);
  printf (" d -- decrease volume\n");
  printf (" i -- increase volume\n");
  printf (" v -- fetch volume, display mute or not.\n");
  printf (" m -- mute\n");
  printf (" s -- sample rate\n");
  printf (" c -- channels\n");
  printf (" u -- unmute\n");
  printf (" n -- fetch name of %s\n", dev_str[dev]);
  printf (" r -- return to main menu\n");
  printf ("\n==========================================");

  str_len = strlen (dev_str[dev]);
  for (i = str_len - 2; i > 0; i--) {
    printf ("=");
  }

  printf ("\n\n");
}

static void
show_main_menu ()
{
  printf ("\n=============== oryx audio device test ==================\n\n");
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
  unsigned char cur_volume = 0;
  unsigned char new_volume = 0;
  unsigned int sample_rate = 0;
  unsigned char channels = 0;
  int index_arr[AUDIO_DEVICE_AMOUNT_MAX];
  int arr_len = AUDIO_DEVICE_AMOUNT_MAX;
  int dev_name_len = AUDIO_DEVICE_NAME_MAX_LEN;
  char dev_name[AUDIO_DEVICE_NAME_MAX_LEN];
  AM_AUDIO_DEVICE_TYPE dev = AM_AUDIO_DEVICE_MIC;
  bool sub_running = true;
  bool mute_or_not = true;
  int i, dev_index = 0;

  g_isrunning = true;
  show_main_menu();
  while (g_isrunning) {
    switch (getchar ()) {
    case 'm':
    case 'M': {
      dev = AM_AUDIO_DEVICE_MIC;
    } break;

    case 's':
    case 'S': {
      dev = AM_AUDIO_DEVICE_SPEAKER;
    } break;

    case 'r':
    case 'R': {
      dev = AM_AUDIO_DEVICE_SOURCE_OUTPUT;
    } break;

    case 'p':
    case 'P': {
      dev = AM_AUDIO_DEVICE_SINK_INPUT;
    } break;

    case 'q':
    case 'Q': {
      g_isrunning = false;
    } break;

    case '\n': {
      continue;
    } break;

    default: {
      PRINTF ("No such audio device, try again!");
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
    arr_len = AUDIO_DEVICE_AMOUNT_MAX;
    if (!adev->get_index_list(dev, index_arr, &arr_len)) {
      ERROR ("Failed to get index list of %ss", dev_str[dev]);
      sub_running = false;
    } else {
      sub_running = true;
      if (arr_len == 1) {
        /* Currently, there is just one device or stream on board */
        dev_index = index_arr[0];
      } else {
        do {
          printf ("\n\nThere are two or more %ss on borad: \n\n", dev_str[dev]);
          printf ("choice index name\n");
          for (i = 0; i < arr_len; i++) {
            if (!adev->get_name_by_index(dev,
                                         index_arr[i],
                                         dev_name,
                                         dev_name_len)) {
              ERROR ("Failed to get device name!");
              sub_running = false; break;
            }

            printf ("%6d %5d %s\n", i + 1, index_arr[i], dev_name);
          }

          if (sub_running) {
            printf ("\nyour choice[1 - %d]: ", arr_len);
            scanf ("%d", &i);
            if (i <= 0 || i > arr_len) {
              PRINTF ("\nWrong choice, try again.!");
            } else {
              dev_index = index_arr[i - 1];
              break;
            }
          } else {
            break;
          }
        } while (true);
      }
    }

    if (sub_running) {
      show_submenu (dev);
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
          if (!adev->get_volume_by_index(dev,
                                         &cur_volume,
                                         dev_index)) {
            ERROR ("Failed to get volume of %s!", dev_str[dev]);
            sub_running = false;
          } else {
            if (cur_volume > 5) {
              new_volume = cur_volume - 5;
            } else {
              new_volume = 0;
            }

            if (!adev->set_volume_by_index(dev,
                                           new_volume,
                                           dev_index)) {
              ERROR ("Failed to decrease volume of %s!", dev_str[dev]);
              sub_running = false;
            }
          }
        } break;

        case 'I':
        case 'i': {
          /* Like decreasing volume, speaker's volume will be increased by 5%.*/
          if (!adev->get_volume_by_index(dev,
                                         &cur_volume,
                                         dev_index)) {
            ERROR ("Failed to get volume of %s!", dev_str[dev]);
            sub_running = false;
          } else {
            if (cur_volume >= 95) {
              new_volume = 100;
            } else {
              new_volume = cur_volume + 5;
            }

            if (!adev->set_volume_by_index(dev,
                                           new_volume,
                                           dev_index)) {
              ERROR ("Failed to decrease volume of %s!", dev_str[dev]);
              sub_running = false;
            }
          }
        } break;

        case 'v':
        case 'V': {
          if (!adev->get_volume_by_index (dev,
                                          &cur_volume,
                                          dev_index)) {
            ERROR ("Failed to get volume of %s!", dev_str[dev]);
            sub_running = false;
          } else {
            if (!adev->is_muted_by_index (dev,
                                         &mute_or_not,
                                         dev_index)) {
              ERROR ("Failed to check whether %s is mute or not.", dev_str[dev]);
              sub_running = false;
            } else {
              PRINTF ("current volume of %s: %d%, %s\n", dev_str[dev], cur_volume,
                                                         mute_or_not ? "mute" : "unmute");
            }
          }
        } break;

        case 'M':
        case 'm': {
          if (!adev->mute_by_index(dev, dev_index)) {
            ERROR ("Failed to let %s to be silent!", dev_str[dev]);
            sub_running = false;
          }
        } break;

        case 'u':
        case 'U': {
          if (!adev->unmute_by_index(dev, dev_index)) {
            ERROR ("Failed to restore %s!", dev_str[dev]);
            sub_running = false;
          }
        } break;

        case 'S':
        case 's': {
          if (!adev->get_sample_rate(&sample_rate)) {
            ERROR ("Falied to get sample rate!");
          } else {
            PRINTF ("Sample rate: %u", sample_rate);
          }
        } break;

        case 'C':
        case 'c': {
          if (!adev->get_channels(&channels)) {
            ERROR ("Falied to get channels");
          } else {
            PRINTF ("Channels: %u", channels);
          }
        } break;

        case 'n':
        case 'N': {
          if (!adev->get_name_by_index(dev,
                                       dev_index,
                                       dev_name,
                                       dev_name_len)) {
            ERROR ("Failed to get name of %s", dev_str[dev]);
            sub_running = false;
          } else {
            PRINTF ("%s's name: %s", dev_str[dev], dev_name);
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

      show_submenu (dev);
    }

    show_main_menu ();
  }
}

int
main (int argc, char **argv)
{
  if ((adev = create_audio_device ("pulse")) == nullptr) {
    ERROR ("Failed to create an instance of AmAudioDevice!");
    return -1;
  }

  mainloop ();
  return 0;
}
