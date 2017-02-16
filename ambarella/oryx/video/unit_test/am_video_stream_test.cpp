/*******************************************************************************
 * am_video_stream_test.cpp
 *
 * History:
 *   2014-8-26 - [lysun] created file
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
#include "am_video_reader_if.h"
#include "am_log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

//C++ 11 thread
#include <thread>

// options and usage
#define NO_ARG    0
#define HAS_ARG   1

enum numeric_short_options {
  DUMP_VIDEO  = 'z' + 1,
  DUMP_YUV    = 'z' + 2,
  DUMP_LUMA   = 'z' + 3,
  DUMP_RAW    = 'z' + 4
};

static struct option long_options[]={
  {"filename_base", HAS_ARG, 0, 'f'},
  {"video", NO_ARG, 0, DUMP_VIDEO },
  {"yuv", HAS_ARG, 0, DUMP_YUV },
  {"luma", HAS_ARG,  0, DUMP_LUMA},
  {"raw", NO_ARG,  0, DUMP_RAW},
  {"verbose", NO_ARG,   0,  'v'},
  {0, 0, 0, 0}
};

static const char *short_options = "f:v";


static char file_name_base[512] = "";
static int file_name_base_flag = 0;


static int file_fd[AM_STREAM_MAX_NUM] = {-1, -1, -1, -1 };
static int verbose_flag = 0;
static int dump_video_flag = 0;
static int dump_yuv_flag = 0;
static int dump_yuv_buffer_id = 0;
static int dump_luma_flag = 0;
static int dump_luma_buffer_id = 0;
static int dump_raw_flag = 0;

static bool quit_flag = false;

struct hint_s {
  const char *arg;
  const char *str;
};

static const struct hint_s hint[] = {
  {"filename_base", "specify filename with path"},
  {"", "\t\tdump encoded video stream(s) to file(s)"},
  {"0~3", "\t\tbuffer_id, dump yuv of this buffer to file"},
  {"0~3", "\t\tbuffer_id, dump me1 (luma) of this buffer to file"},
  {"", "\t\tdump Bayer pattern RAW format to file"},
  {"", "\t\tprint more information"},

};

void usage(void)
{
  uint32_t i;
  printf("test_stream usage:\n");
  for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
    if (isalpha(long_options[i].val))
      printf("-%c ", long_options[i].val);
    else
      printf("   ");
    printf("--%s", long_options[i].name);
    if (hint[i].arg[0] != 0)
      printf(" [%s]", hint[i].arg);
    printf("\t%s\n", hint[i].str);
  }
  printf("\n");
}


int init_param(int argc, char **argv)
{

  int ch;
  int option_index = 0;

  opterr = 0;
  while ((ch = getopt_long(argc,
                           argv,
                           short_options,
                           long_options,
                           &option_index)) != -1) {
    switch (ch) {
      case 'f':
        strcpy(file_name_base, optarg);
        file_name_base_flag = 1;
        break;
      case 'v':
        verbose_flag = 1;
        break;

      case DUMP_VIDEO:
        dump_video_flag = 1;
        break;

      case DUMP_YUV:
        dump_yuv_buffer_id = atoi(optarg);
        dump_yuv_flag = 1;
        break;

      case DUMP_LUMA:
        dump_luma_buffer_id = atoi(optarg);
        dump_luma_flag = 1;
        break;

      case DUMP_RAW:
        dump_raw_flag = 1;
        break;

      default:
        printf("unknown command %s \n", optarg);
        return -1;
        break;

    }

  }

  if (!file_name_base_flag) {
    ERROR("please enter a file base name by '-f'\n");
    return -1;

  }

  return 0;
}

void sigstop(int)
{
  INFO("Force quit by signal\n");
  quit_flag  = true;
}


static int dump_video()
{
  AM_RESULT result = AM_RESULT_OK;
  uint32_t stream_id;
  uint8_t *frame_start;
  int32_t frame_size;
  char file_fullname[512];
  char extension[16];
  uint32_t i;
  int ret = 0;

  AMQueryDataFrameDesc frame_desc;
  AMMemMapInfo bsb_mem;

  AMIVideoReaderPtr reader = AMIVideoReader::get_instance();
  if (!reader) {
    result = AM_RESULT_ERR_INVALID;
    return -1;
  }

  //try to get bsb mem
  result = reader->get_bsb_mem(&bsb_mem);
  if (result != AM_RESULT_OK) {
    ERROR("get bsb mem failed \n");
    return -2;
  }

  while (!quit_flag) {
    result = reader->query_video_frame(&frame_desc, 2000/* timeout in 2 sec */);
    if (result == AM_RESULT_ERR_AGAIN) {
      //DSP state not ready to query video, small pause and query state again
      usleep(100*1000);
      continue;
    }else if (result != AM_RESULT_OK) {
      ERROR("query video frame failed \n");
      ret = -3;
      break;
    }
    stream_id = frame_desc.video.stream_id;

    //create new file for new session, otherwise, use existing file_fd
    if (reader->is_new_video_session(stream_id)) {
      if (frame_desc.video.video_type == AM_VIDEO_FRAME_TYPE_MJPEG){
        strcpy(extension, "mjpeg");
      } else {
        strcpy(extension, "h264");
      }
      //just to create a unique file name so that it won't overwrite existing
      sprintf(file_fullname, "%s_%d_%u.%s", file_name_base,
              stream_id, frame_desc.video.session_id,
              extension);
      if (file_fd[stream_id] < 0) {
        file_fd[stream_id] = open(file_fullname, O_CREAT | O_RDWR, 0644); //with right mode 0644
      } else {
        ERROR("Unexpected: file_fd for stream %d not closed when there is new session, will append write \n");
        ret = -4;
        break;
      }
    }

    if (file_fd[stream_id] < 0) {
      ERROR("file_fd wrong for stream %d\n", stream_id);
      ret = -5;
      break;
    }

    frame_start = bsb_mem.addr + frame_desc.video.data_addr_offset;
    frame_size = frame_desc.video.data_size;

    //check stream end flag, if it's , close the file.
    if (frame_desc.video.stream_end_flag) {
      INFO("stream %d now ends \n", stream_id);
      close(file_fd[stream_id]);
      file_fd[stream_id] = -1;
    } else {
      //otherwise, write the video data
      if (write(file_fd[stream_id], frame_start, frame_size) != frame_size) {
        ERROR("write file error \n");
        ret = -6;
        break;
      }
    }

    if (quit_flag) {
      ret = 0;
      break;
    }
  } //end while

  //close unclosed files.
  for (i = 0; i < AM_STREAM_MAX_NUM; i ++) {
    if (file_fd[i] > 0) {
      close(file_fd[i]);
      file_fd[i] = -1;
    }
  }

  return ret;
}

static int dump_yuv()
{
  int ret = 0;
  AM_RESULT result = AM_RESULT_OK;
  AMQueryDataFrameDesc frame_desc;
  AMMemMapInfo dsp_mem;
  int32_t width, height, pitch;
  char file_fullname[512];
  char extension[16];
  uint8_t *y_start;
  uint8_t *uv_start;
  int32_t uv_height;
  int32_t i;

  AMIVideoReaderPtr reader = AMIVideoReader::get_instance();
  if (!reader) {
    result = AM_RESULT_ERR_INVALID;
    return -1;
  }

  //try to get bsb mem
  result = reader->get_dsp_mem(&dsp_mem);
  if (result != AM_RESULT_OK) {
    ERROR("get bsb mem failed \n");
    return -2;
  }

  while (!quit_flag) {
    result =
        reader->query_yuv_frame(&frame_desc,
                                AM_ENCODE_SOURCE_BUFFER_ID(dump_yuv_buffer_id),
                                false);
    if (result == AM_RESULT_ERR_AGAIN) {
      //DSP state not ready to query video, small pause and query state again
      usleep(100 * 1000);
      continue;
    } else if (result != AM_RESULT_OK) {
      ERROR("query yuv frame failed \n");
      ret = -3;
      break;
    }

    width = frame_desc.yuv.width;
    height = frame_desc.yuv.height;
    pitch = frame_desc.yuv.pitch;
    strcpy(extension, "yuv");
    //just to create a unique file name so that it won't overwrite existing
    sprintf(file_fullname,
            "%s_%dx%d.%s",
            file_name_base,
            width,
            height,
            extension);
    //create the file only when needed
    if (file_fd[0] < 0) {
      file_fd[0] = open(file_fullname, O_CREAT | O_RDWR, 0644);
    }
    //check again
    if (file_fd[0] < 0) {
      ERROR("file_fd wrong for yuv dump\n");
      ret = -4;
      break;
    }

    y_start = dsp_mem.addr + frame_desc.yuv.y_addr_offset;
    //note that this is mem dump, so use pitch instead of width

    printf("start offset 0x%x, seq num %d \n", frame_desc.yuv.y_addr_offset,
               frame_desc.yuv.seq_num);

    for (i = 0; i < height ; i++) {
      if (write(file_fd[0], y_start + i * pitch, width) != width) {
        ERROR("write y file error \n");
        ret = -5;
        break;
      }
    }

    uv_start = dsp_mem.addr + frame_desc.yuv.uv_addr_offset;
    if (frame_desc.yuv.format == AM_ENCODE_CHROMA_FORMAT_YUV420) {
      uv_height = height / 2;
    } else if (frame_desc.yuv.format == AM_ENCODE_CHROMA_FORMAT_YUV422) {
      uv_height = height;
    } else {
      ERROR("not supported chroma format in YUV dump\n");
      ret = -6;
      break;
    }

    for (i = 0 ; i < uv_height; i ++ )
    if (write(file_fd[0], uv_start + i * pitch, width) != width) {
          ERROR("write uv file error \n");
          ret = -7;
          break;
    }

    if (quit_flag) {
      ret = 0;
      break;
    }

  } //end while

  if (file_fd[0] > 0) {
    close(file_fd[0]);
    file_fd[0] = -1;
  }

  return ret;
}

static int dump_luma()
{
    int ret = 0;
    AM_RESULT result = AM_RESULT_OK;
    AMQueryDataFrameDesc frame_desc;
    AMMemMapInfo dsp_mem;
    int32_t width, height, pitch;
    char file_fullname[512];
    char extension[16];
    uint8_t *y_start;
    int32_t i;

    AMIVideoReaderPtr reader = AMIVideoReader::get_instance();
    if (!reader) {
      result = AM_RESULT_ERR_INVALID;
      return -1;
    }

    //try to get bsb mem
    result = reader->get_dsp_mem(&dsp_mem);
    if (result != AM_RESULT_OK) {
      ERROR("get bsb mem failed \n");
      return -2;
    }

    while (!quit_flag) {
      result =
          reader->query_luma_frame(
              &frame_desc,
              AM_ENCODE_SOURCE_BUFFER_ID(dump_luma_buffer_id),
              false);
      if (result == AM_RESULT_ERR_AGAIN) {
        //DSP state not ready to query video, small pause and query state again
        usleep(100 * 1000);
        continue;
      } else if (result != AM_RESULT_OK) {
        ERROR("query luma frame failed \n");
        ret = -3;
        break;
      }

      width = frame_desc.luma.width;
      height = frame_desc.luma.height;
      pitch = frame_desc.luma.pitch;
      strcpy(extension, "y");
      //just to create a unique file name so that it won't overwrite existing
      sprintf(file_fullname,
              "%s_%dx%d.%s",
              file_name_base,
              width,
              height,
              extension);
      //create the file only when needed
      if (file_fd[0] < 0) {
        file_fd[0] = open(file_fullname, O_CREAT | O_RDWR, 0644);
      }
      //check again
      if (file_fd[0] < 0) {
        ERROR("file_fd wrong for yuv dump\n");
        ret = -4;
        break;
      }

      y_start = dsp_mem.addr + frame_desc.luma.data_addr_offset;
      //note that this is mem dump, so use pitch instead of width

      printf("start offset 0x%x, seq num %d \n",
             frame_desc.luma.data_addr_offset,
             frame_desc.luma.seq_num);

      for (i = 0; i < height ; i++) {
        if (write(file_fd[0], y_start + i * pitch, width) != width) {
          ERROR("write y file error \n");
          ret = -5;
          break;
        }
      }

      if (quit_flag) {
        ret = 0;
        break;
      }

    } //end while

    if (file_fd[0] > 0) {
      close(file_fd[0]);
      file_fd[0] = -1;
    }

   return ret;

}

static int dump_raw()
{
    int ret = 0;
     AM_RESULT result = AM_RESULT_OK;
     AMQueryDataFrameDesc frame_desc;
     AMMemMapInfo dsp_mem;
     int32_t width, height, pitch;
     char file_fullname[512];
     char extension[16];
     uint8_t *data_start;
     int32_t raw_size;

     AMIVideoReaderPtr reader = AMIVideoReader::get_instance();
     if (!reader) {
       result = AM_RESULT_ERR_INVALID;
       return -1;
     }

     //try to get bsb mem
     result = reader->get_dsp_mem(&dsp_mem);
     if (result != AM_RESULT_OK) {
       ERROR("get bsb mem failed \n");
       return -2;
     }

     while (1) {
       result = reader->query_bayer_raw_frame(&frame_desc);

       if (result == AM_RESULT_ERR_AGAIN) {
         //DSP state not ready to query video, small pause and query state again
         usleep(100 * 1000);
         continue;
       } else if (result != AM_RESULT_OK) {
         ERROR("query raw frame failed \n");
         ret = -3;
         break;
       }

       width = frame_desc.bayer.width;
       height = frame_desc.bayer.height;
       pitch = frame_desc.bayer.pitch;
       strcpy(extension, "raw");
       //just to create a unique file name so that it won't overwrite existing
       sprintf(file_fullname,
               "%s_%dx%d.%s",
               file_name_base,
               width,
               height,
               extension);
       //create the file only when needed
       if (file_fd[0] < 0) {
         file_fd[0] = open(file_fullname, O_CREAT | O_RDWR, 0644);
       }
       //check again
       if (file_fd[0] < 0) {
         ERROR("file_fd wrong for raw dump\n");
         ret = -4;
         break;
       }

       data_start = dsp_mem.addr + frame_desc.bayer.data_addr_offset;
       //note that this is mem dump, so use pitch instead of width
       raw_size = pitch * height;

       printf("start offset 0x%x, PTS= %llu \n", frame_desc.bayer.data_addr_offset,
              frame_desc.mono_pts);

       if (write(file_fd[0], data_start ,  raw_size) != raw_size) {
           ERROR("write raw file error \n");
           ret = -5;
           break;
        }

       if (quit_flag) {
         ret = 0;
         break;
       }

     } //end while

     if (file_fd[0] > 0) {
       close(file_fd[0]);
       file_fd[0] = -1;
     }

    return ret;
}



int main(int argc, char **argv)
{
  int ret = 0;
  AM_RESULT result;
  int count;
  AMIVideoReaderPtr reader = NULL;
  std::thread *thread = NULL;
  //register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  do {
    if (argc < 2) {
      usage();
      ret = -1;
      break;
    }

    if (init_param(argc, argv) < 0) {
      ERROR("init param failed \n");
      ret = -3;
      break;
    }

    reader = AMIVideoReader::get_instance();
    if (!reader) {
      ERROR("unable to get AMVideoReader instance \n");
      ret = -1;
      break;
    }

    result = reader->init();
    if (result != AM_RESULT_OK) {
      ERROR("unable to init AMVideoReader\n");
      ret = -2;
      break;
    }

    count = dump_video_flag + dump_yuv_flag + dump_raw_flag + dump_luma_flag;
    if (count > 1) {
      ERROR("please capture one type of data at one time\n");
      ret = -2;
      break;
    }

    if (dump_video_flag) {
      thread = new std::thread(dump_video);
      if (!thread) {
        ERROR("unable to create thread, failed \n");
        ret = -6;
        break;
      }
    }

    if (dump_yuv_flag) {
      thread = new std::thread(dump_yuv);
      if (!thread) {
        ERROR("unable to create thread, failed \n");
        ret = -6;
        break;
      }
    }

    if (dump_luma_flag) {
      thread = new std::thread(dump_luma);
      if (!thread) {
        ERROR("unable to create thread, failed \n");
        ret = -6;
        break;
      }
    }

    if (dump_raw_flag) {
      thread = new std::thread(dump_raw);
      if (!thread) {
        ERROR("unable to create thread, failed \n");
        ret = -6;
        break;
      }
    }

  } while (0);


  if (thread) {
    thread->join();
    delete thread;
  }

  return ret;
}
