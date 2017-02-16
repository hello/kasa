/*******************************************************************************
 * test_oryx_data_export.cpp
 *
 * History:
 *   2015-01-04 - [Zhi He]      created file
 *   2015-04-02 - [Shupeng Ren] modified  file
 *
 * Copyright (C) 2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_export_if.h"

#include <signal.h>

#define DMAX_STREAM_NUMBER 8

struct unittest_content {
  char    output_filenamebase[512];
  uint8_t b_no_dump;
  uint8_t reserved0;
  uint8_t reserved1;
  uint8_t reserved2;

  uint8_t b_video_output_file_opened[DMAX_STREAM_NUMBER];
  uint8_t b_audio_output_file_opened[DMAX_STREAM_NUMBER];

  FILE*   p_video_output_file[DMAX_STREAM_NUMBER];
  FILE*   p_audio_output_file[DMAX_STREAM_NUMBER];
};

static AMIExportClient* g_client = nullptr;
static bool running_flag = true;

static void __sigstop(int i)
{
  running_flag = false;
}

static void show_usage()
{
  printf("test_oryx_data_export usage:\n");
  printf("\t-f [%%s]: specify output filename base, "
      "final video file name will be 'filename_video_%%d.h264', "
      "same with '--filename'\n");
  printf("\t--filename [%%s]: specify output filename base, "
      "final video file name will be 'filename_video_%%d.h264', "
      "same with '-f'\n");
  printf("\t--nodump: will not save file, print only\n");
  printf("\t--help: show usage\n");
}

static int init_params(int argc, char **argv, unittest_content* content)
{
  int i = 0;

  for (i = 1; i < argc; i ++) {
    if (!strcmp("--filename", argv[i])) {
      if (((i + 1) < argc)) {
        snprintf(content->output_filenamebase, 512, "%s", argv[i + 1]);
        printf("[input argument]: '--filename': (%s).\n",
               content->output_filenamebase);
      } else {
        printf("[input argument error]: '--filename', "
            "should follow with output filename base, argc %d, i %d.\n",
            argc, i);
        return (-1);
      }
      i ++;
    } else if (!strcmp("-f", argv[i])) {
      if (((i + 1) < argc)) {
        snprintf(content->output_filenamebase, 512, "%s", argv[i + 1]);
        printf("[input argument]: '-f': (%s).\n",
               content->output_filenamebase);
      } else {
        printf("[input argument error]: '-f', "
            "should follow with output filename base, argc %d, i %d.\n",
            argc, i);
        return (-2);
      }
      i ++;
    } else if (!strcmp("--nodump", argv[i])) {
      content->b_no_dump = 1;
    } else if (!strcmp("--help", argv[i])) {
      show_usage();
      return 1;
    } else {
      printf("[input argument error]: unknwon input params, [%d][%s]\n",
             i, argv[i]);
      return (-20);
    }
  }

  return 0;
}

static void open_video_output_file(unittest_content* content,
                                   uint32_t stream_index,
                                   uint8_t packet_format)
{
  char filename[512] = {0};

  switch (packet_format) {
    case AM_EXPORT_PACKET_FORMAT_AVC:
      snprintf(filename, 512, "%s_video_%d.h264",
               content->output_filenamebase, stream_index);
      content->p_video_output_file[stream_index] = fopen(filename, "wb+");
      break;
    case AM_EXPORT_PACKET_FORMAT_HEVC:
      snprintf(filename, 512, "%s_video_%d.h265",
               content->output_filenamebase, stream_index);
      content->p_video_output_file[stream_index] = fopen(filename, "wb+");
      break;
    case AM_EXPORT_PACKET_FORMAT_MJPEG:
      snprintf(filename, 512, "%s_video_%d.mjpeg",
               content->output_filenamebase, stream_index);
      content->p_video_output_file[stream_index] = fopen(filename, "wb+");
      break;
    default:
      ERROR("BAD video format %d", packet_format);
      break;
  }
}

static void open_audio_output_file(unittest_content* content,
                                   uint32_t stream_index,
                                   uint8_t packet_format)
{
  char filename[512] = {0};

  switch (packet_format) {
    case AM_EXPORT_PACKET_FORMAT_AAC:
      snprintf(filename, 512, "%s_audio_%d.aac",
               content->output_filenamebase, stream_index);
      content->p_audio_output_file[stream_index] = fopen(filename, "wb+");
      break;
    case AM_EXPORT_PACKET_FORMAT_G711MuLaw:
      snprintf(filename, 512, "%s_audio_g711mu_%d.g711",
               content->output_filenamebase, stream_index);
      content->p_audio_output_file[stream_index] = fopen(filename, "wb+");
      break;
    case AM_EXPORT_PACKET_FORMAT_G711ALaw:
      snprintf(filename, 512, "%s_audio_g711a_%d.g711",
               content->output_filenamebase, stream_index);
      content->p_audio_output_file[stream_index] = fopen(filename, "wb+");
      break;
    case AM_EXPORT_PACKET_FORMAT_G726_40:
      snprintf(filename, 512, "%s_audio_g726_40_%d.g726",
               content->output_filenamebase, stream_index);
      content->p_audio_output_file[stream_index] = fopen(filename, "wb+");
      break;
    case AM_EXPORT_PACKET_FORMAT_G726_32:
      snprintf(filename, 512, "%s_audio_g726_32_%d.g726",
               content->output_filenamebase, stream_index);
      content->p_audio_output_file[stream_index] = fopen(filename, "wb+");
      break;
    case AM_EXPORT_PACKET_FORMAT_G726_24:
      snprintf(filename, 512, "%s_audio_g726_24_%d.g726",
               content->output_filenamebase, stream_index);
      content->p_audio_output_file[stream_index] = fopen(filename, "wb+");
      break;
    case AM_EXPORT_PACKET_FORMAT_G726_16:
      snprintf(filename, 512, "%s_audio_g726_16_%d.g726",
               content->output_filenamebase, stream_index);
      content->p_audio_output_file[stream_index] = fopen(filename, "wb+");
      break;
    case AM_EXPORT_PACKET_FORMAT_PCM:
      snprintf(filename, 512, "%s_audio_%d.pcm",
               content->output_filenamebase, stream_index);
      content->p_audio_output_file[stream_index] = fopen(filename, "wb+");
      break;
    case AM_EXPORT_PACKET_FORMAT_OPUS:
      snprintf(filename, 512, "%s_audio_%d.opus",
               content->output_filenamebase, stream_index);
      content->p_audio_output_file[stream_index] = fopen(filename, "wb+");
      break;
    case AM_EXPORT_PACKET_FORMAT_BPCM:
      snprintf(filename, 512, "%s_audio_%d.bpcm",
               content->output_filenamebase, stream_index);
      content->p_audio_output_file[stream_index] = fopen(filename, "wb+");
      break;
    case AM_EXPORT_PACKET_FORMAT_SPEEX:
      snprintf(filename, 512, "%s_audio_%d.speex",
               content->output_filenamebase, stream_index);
      content->p_audio_output_file[stream_index] = fopen(filename, "wb+");
      break;
    default:
      ERROR("BAD audio format %d", packet_format);
      break;
  }
}

static void write_packet(unittest_content* content, AMExportPacket* packet)
{
  if (AM_LIKELY(DMAX_STREAM_NUMBER > packet->stream_index)) {
    switch (packet->packet_type) {
      case AM_EXPORT_PACKET_TYPE_VIDEO_DATA:
        if (AM_UNLIKELY(!content->b_video_output_file_opened[packet->stream_index])) {
          open_video_output_file(content, packet->stream_index,
                                 packet->packet_format);
          content->b_video_output_file_opened[packet->stream_index] = 1;
        }
        if (AM_LIKELY(content->p_video_output_file[packet->stream_index])) {
          fwrite(packet->data_ptr, 1, packet->data_size,
                 content->p_video_output_file[packet->stream_index]);
        }
        break;

      case AM_EXPORT_PACKET_TYPE_AUDIO_DATA:
        if (AM_UNLIKELY(!content->b_audio_output_file_opened[packet->stream_index])) {
          open_audio_output_file(content, packet->stream_index,
                                 packet->packet_format);
          content->b_audio_output_file_opened[packet->stream_index] = 1;
        }
        if (AM_LIKELY(content->p_audio_output_file[packet->stream_index])) {
          fwrite(packet->data_ptr, 1, packet->data_size,
                 content->p_audio_output_file[packet->stream_index]);
        }
        break;

      default:
        NOTICE("discard non-video-audio packet here\n");
        break;
    }
  } else {
    ERROR("BAD stream index %d", packet->stream_index);
  }
}

int main(int argc, char *argv[])
{
  if (AM_UNLIKELY(2 > argc)) {
    show_usage();
    return (-10);
  }

  int ret = 0;
  AMExportPacket packet;
  AMExportConfig config = {0};
  unittest_content content;
  memset(&content, 0x0, sizeof(unittest_content));

  if ((ret = init_params(argc, argv, &content)) < 0) {
    show_usage();
    return (-1);
  } else if (ret) {
    return ret;
  }

  do {
    if (!(g_client = am_create_export_client(AM_EXPORT_TYPE_UNIX_DOMAIN_SOCKET,
                                             &config))) {
      ERROR("am_create_export_client() failed");
      ret = (-3);
      break;
    }

    signal(SIGINT, __sigstop);
    signal(SIGQUIT, __sigstop);
    signal(SIGTERM, __sigstop);

    if (!g_client->connect_server(DEXPORT_PATH)) {
      ERROR("p_client->connect() failed");
      ret = (-4);
      break;
    }

    NOTICE("read packet loop start");
    while (running_flag) {
      if (g_client->receive(&packet)) {
        switch (packet.packet_type) {
          case AM_EXPORT_PACKET_TYPE_VIDEO_INFO: {
            AMExportVideoInfo *video_info = (AMExportVideoInfo*)packet.data_ptr;
            printf("Video INFO[%d]: "
                "width: %d, height: %d, framerate factor: %d/%d\n",
                 packet.stream_index,
                 video_info->width, video_info->height,
                 video_info->framerate_num, video_info->framerate_den);
          } break;
          case AM_EXPORT_PACKET_TYPE_AUDIO_INFO: {
            AMExportAudioInfo *audio_info = (AMExportAudioInfo*)packet.data_ptr;
            printf("Audio INFO[%d]: "
                "samplerate: %d, frame size: %d, "
                "bitrate: %d, channel: %d, sample size: %d\n",
                packet.stream_index,
                audio_info->samplerate, audio_info->frame_size,
                audio_info->bitrate, audio_info->channels,
                audio_info->sample_size);
          } break;
          case AM_EXPORT_PACKET_TYPE_VIDEO_DATA:
          case AM_EXPORT_PACKET_TYPE_AUDIO_DATA: {
            if (!content.b_no_dump) {
              write_packet(&content, &packet);
            } else {
              printf("receive a packet, stream index %d, format %d, size %d\n",
                     packet.stream_index, packet.packet_type, packet.data_size);
            }
          } break;
          default:
            break;
        }
        g_client->release(&packet);
      } else {
        running_flag = false;
        WARN("receive_packet failed, server shut down");
        break;
      }
    }
    NOTICE("read packet loop end");
  } while (0);

  if (g_client) {
    g_client->destroy();
  }

  for (ret = 0; ret < DMAX_STREAM_NUMBER; ret ++) {
    if (content.p_video_output_file[ret]) {
      fclose(content.p_video_output_file[ret]);
      content.p_video_output_file[ret] = nullptr;
    }

    if (content.p_audio_output_file[ret]) {
      fclose(content.p_audio_output_file[ret]);
      content.p_audio_output_file[ret] = nullptr;
    }
  }

  return ret;
}
