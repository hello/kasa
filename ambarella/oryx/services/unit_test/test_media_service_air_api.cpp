/*******************************************************************************
 * test_playback_audio_file.cpp
 *
 * History:
 *   2015-2-27 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#include <signal.h>
#include <limits.h>

#include "am_base_include.h"
#include "am_log.h"

#include "am_api_helper.h"
#include "am_api_media.h"

int ctrl[2];

#define CTRL_READ ctrl[0]
#define CTRL_WRITE ctrl[1]

static void show_menu()
{
  printf("\n===========test_media_service=================\n");
  printf("  r -- start media recording\n");
  printf("  s -- stop media recording\n");
  printf("  y -- start audio file playback\n");
  printf("  p -- pause audio file playback\n");
  printf("  n -- stop audio file playback\n");
  printf("  d -- start file recording\n");
  printf("  b -- stop file recording\n");
  printf("  e -- send event\n");
  printf("  q -- Quit the program\n");
  printf("\n===============================================\n\n");
}

static void sigstop(int sig_num)
{
  PRINTF("sigstop.\n");
  write(CTRL_WRITE, "e", 1);
}

int main(int argc, char **argv)
{
  int ret = 0;
  int result = 0;//method call result
  bool running = true;
  fd_set allset;
  fd_set fdset;
  int maxfd = -1;
  FD_ZERO(&allset);
  FD_SET(STDIN_FILENO, &allset);
  FD_SET(CTRL_READ, &allset);
  maxfd = (STDIN_FILENO > CTRL_READ)? STDIN_FILENO : CTRL_READ;
  do {
    if (argc < 2) {
      PRINTF("If you want to playback audio file, please Usage: \n "
          "%s mediafile1 [mediafile2 | ...],\n"
          "if you want to control media recording or file recording, "
          "please continue.", argv[0]);
    }
    if (pipe(ctrl) < 0) {
      ERROR("Failed to create pipe.\n");
      ret = -1;
      break;
    }
    signal(SIGINT, sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);
    AMIApiPlaybackAudioFileList* audio_file =
                               AMIApiPlaybackAudioFileList::create();
    if(!audio_file) {
      ERROR("Failed to create AMIApiPlaybackAudioFileList");
      ret = -1;
      break;
    }
    show_menu();
    while (running) { //main loop
      char ch = '\n';
      fdset = allset;
      if (select(maxfd + 1, &fdset, NULL, NULL, NULL) > 0) {
        if (FD_ISSET(STDIN_FILENO, &fdset)) {
          if (read(STDIN_FILENO, &ch, 1) < 0) {
            ERROR("Read error.\n");
            running = false;
            ret = -1;
            continue;
          }
          if((ch == 'q') || (ch == 'Q')) {
            running = false;
            INFO("Quit!");
            continue;
          }
        } else if (FD_ISSET(CTRL_READ, &fdset)) {
          char cmd[1] = { 0 };
          if (read(CTRL_READ, cmd, sizeof(cmd)) < 0) {
            ERROR("Read error.\n");
            running = false;
            ret = -1;
            continue;
          } else if(cmd[0] == 'e') {
            running = false;
            continue;
          }
        }
      } else {
        if(errno != EINTR) {
          ERROR("select error.\n");
          ret = -1;
          running = false;
        }
        continue;
      }
      AMAPIHelperPtr g_api_helper = NULL;
      if ((g_api_helper = AMAPIHelper::get_instance()) == NULL) {
        ERROR("CCWAPIConnection::get_instance failed.\n");
        ret = -1;
        break;
      }
      switch (ch) {
        case 'R' :
        case 'r' : {
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_START_RECORDING,
                                    nullptr, 0,
                                    &result, sizeof(int));
          if (result < 0) {
            ERROR("Failed to start recording!");
            ret = -1;
            running = false;
            continue;
          }
          show_menu();
        }break;
        case 'S' :
        case 's' : {
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_STOP_RECORDING,
                                    nullptr, 0,
                                    &result, sizeof(int));
          if (result < 0) {
            ERROR("Failed to stop recording!");
            ret = -1;
            running = false;
            continue;
          }
          show_menu();
        }break;
        case 'Y' :
        case 'y' : {
          for (int i = 0; i < argc - 1; ++ i) {
            char abs_path[PATH_MAX] = {0};
            if ((NULL != realpath(argv[i + 1], abs_path)) &&
                !audio_file->add_file(std::string(abs_path))) {
              ERROR("Failed to add file %s to file list, "
                     "file name maybe too long, drop it.");
            }
            if (audio_file->is_full()) {
              g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE,
                                        audio_file->get_file_list(),
                                        audio_file->get_file_list_size(),
                                        &result,
                                        sizeof(int));
              if (result < 0) {
                ERROR("Failed to add audio file.");
                ret = -1;
                running = false;
                continue;
              }
              audio_file->clear_file();
            }
          }
          if (audio_file->get_file_number() > 0) {
            g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE,
                                      audio_file->get_file_list(),
                                      audio_file->get_file_list_size(),
                                      &result,
                                      sizeof(int));
            if (result < 0) {
              ERROR("Failed to add audio file.");
              ret = -1;
            }
            audio_file->clear_file();
          }
          g_api_helper->method_call(
          AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE, NULL, 0,
                                    &result, sizeof(int));
          if(result < 0) {
            ERROR("Failed to start playback audio file.");
            ret = -1;
            running = false;
            continue;
          }
          show_menu();
        } break;
        case 'p':
        case 'P': {
          g_api_helper->method_call(
              AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE, NULL, 0,
                               &result, sizeof(int));
          if(result < 0) {
            ERROR("Failed to pause playback audio file.");
            ret = -1;
            running = false;
            continue;
          }
          show_menu();
        } break;
        case 'q':
        case 'Q': {
          INFO("Quit!\n");
          running = false;
          continue;
        } break;
        case 'N':
        case 'n': {
          g_api_helper->method_call(
              AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE, NULL, 0,
                               &result, sizeof(int));
          if(result < 0) {
            ERROR("Failed to stop playback audio file.");
            ret = -1;
            running = false;
            continue;
          }
          show_menu();
        } break;
        case 'D' :
        case 'd' : {
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING,
                                    nullptr, 0, &result, sizeof(int));
          if (result < 0) {
            ERROR("Failed to start file recording!");
            ret = -1;
            running = false;
            continue;
          }
          show_menu();
        }break;
        case 'B' :
        case 'b' : {
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING,
                                    nullptr, 0, &result, sizeof(int));
          if (result < 0) {
            ERROR("Failed to stop file recording!");
            ret = -1;
            running = false;
            continue;
          }
          show_menu();
        }break;
        case 'E' :
        case 'e' : {
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START,
                                    nullptr, 0,
                                    &result, sizeof(int));
          if (result < 0) {
            ERROR("Failed to start event recording!");
            ret = -1;
            running = false;
            continue;
          }
          show_menu();
        }break;
        case '\n':
          continue;
        default:
          break;
      }
    } //while(running)
    delete audio_file;
    audio_file = NULL;
  } while (0);
  return ret;
}

