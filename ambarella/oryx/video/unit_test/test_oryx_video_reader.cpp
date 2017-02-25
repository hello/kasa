/**
 * test_oryx_video_reader.cpp
 *
 *  History:
 *    Aug 11, 2015 - [Shupeng Ren] created file
 *    Dec 19, 2016 - [Ning Zhang] modified file
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
#include "am_define.h"
#include "am_log.h"
#include <signal.h>
#include <getopt.h>
#include "am_log.h"
#include "am_video_reader_if.h"
#include "am_video_address_if.h"
#include <time.h>
#include <sys/uio.h>
#include <map>

#define NO_ARG    0
#define HAS_ARG   1

using std::map;
static map<uint32_t, int32_t> video_file;

static bool file_name_base_flag = false;
static std::string file_name_base;
static bool dump_video_flag = false;
static bool dump_yuv_flag = false;
static uint32_t dump_yuv_buffer_id = 0;
static bool dump_luma_flag = false;
static uint32_t dump_luma_buffer_id = 0;
static bool dump_raw_flag = false;
static uint32_t dump_count = 0;
static bool quit_flag = false;

static AMIVideoReaderPtr greader  = nullptr;
static AMIVideoAddressPtr gaddress = nullptr;

enum numeric_short_options {
  DUMP_VIDEO  = 'v',
  DUMP_YUV    = 'y',
  DUMP_LUMA   = 'l',
  DUMP_RAW    = 'r',
  COUNT       = 'c'
};

static const char *short_options = "f:vy:l:rc:";
static struct option long_options[] = {
  {"filename_base",   HAS_ARG,  0, 'f'},
  {"video",           NO_ARG,   0, DUMP_VIDEO},
  {"yuv",             HAS_ARG,  0, DUMP_YUV},
  {"luma",            HAS_ARG,  0, DUMP_LUMA},
  {"raw",             NO_ARG,   0, DUMP_RAW},
  {"count",           HAS_ARG,  0, COUNT},
  {0, 0, 0, 0}
};

struct hint_s {
  const char *arg;
  const char *str;
};

static const struct hint_s hint[] = {
  {"", "\t specify filename with path"},
  {"", "\t\t dump encoded video stream(s) to file(s)"},
  {"0~3", "\t\t buffer_id, dump yuv of this buffer to file"},
  {"0~3", "\t\t buffer_id, dump me1 (luma) of this buffer to file"},
  {"", "\t\t dump Bayer pattern RAW format to file"},
  {"", "\t\t number of dumping pictures"},
};

static void usage(int argc, char **argv)
{
  printf("%s usage:\n", argv[0]);
  for (uint32_t i = 0; i < sizeof(long_options) /
              sizeof(long_options[0]) - 1; i++) {
    if (isalpha(long_options[i].val)) {
      printf("-%c ", long_options[i].val);
    } else {
      printf("   ");
    }
    printf("--%s", long_options[i].name);
    if (hint[i].arg[0] != 0) {
      printf(" [%s]", hint[i].arg);
    }
    printf("\t%s\n", hint[i].str);
  }
  printf("\n");
}

static int init_param(int argc, char **argv)
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
      case 'f': {
        std::string tmp = optarg;
        file_name_base += optarg;
        file_name_base_flag = true;
      } break;

      case DUMP_VIDEO:
        dump_video_flag = true;
        break;

      case DUMP_YUV: {
        dump_yuv_buffer_id = atoi(optarg);
        dump_yuv_flag = true;
      } break;

      case DUMP_LUMA: {
        dump_luma_buffer_id = atoi(optarg);
        dump_luma_flag = true;
      } break;

      case DUMP_RAW:
        dump_raw_flag = true;
        break;
      case COUNT:
        dump_count = atoi(optarg);
        break;

      default: {
        printf("unknown command %s \n", optarg);
        return -1;
       } break;
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

static void open_video_file(uint32_t stream_index, AM_STREAM_TYPE stream_type)
{
  char filename[512] = {0};
  char file_time[64] = {0};
  //get the system time
  time_t system_time = time(0);
  strftime(file_time, sizeof(file_time), "%Y%m%d%H%M%S", localtime(&system_time));
  switch (stream_type) {
    case AM_STREAM_TYPE_H264: {
      snprintf(filename, 512, "%s_video_%d_%s.h264",
               file_name_base.c_str(), stream_index, file_time);
      if ((video_file[stream_index] =
          open(filename, O_WRONLY|O_CREAT, 0644)) < 0) {
        ERROR("Failed to create the file %s.", filename);
      }
    } break;
    case AM_STREAM_TYPE_H265: {
      snprintf(filename, 512, "%s_video_%d_%s.h265",
               file_name_base.c_str(), stream_index, file_time);
      if ((video_file[stream_index] =
          open(filename, O_WRONLY|O_CREAT, 0644)) < 0) {
        ERROR("Failed to create the file %s.", filename);
      }
    } break;
    case AM_STREAM_TYPE_MJPEG: {
      snprintf(filename, 512, "%s_video_%d_%s.mjpeg",
               file_name_base.c_str(), stream_index, file_time);
      if ((video_file[stream_index] =
          open(filename, O_WRONLY|O_CREAT, 0644)) < 0) {
        ERROR("Failed to create the file %s.", filename);
      }
    } break;
    default:
      break;
  }
}

static AM_RESULT save_video()
{
  AM_RESULT result = AM_RESULT_OK;
  AMQueryFrameDesc frame_desc;
  AMAddress video_addr = {0};

  do {
    while (!quit_flag) {
      result = greader->query_video_frame(frame_desc, 0);
      if ((result != AM_RESULT_OK) && (result != AM_RESULT_ERR_AGAIN)) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("Failed to query video frame.");
        break;
      } else if (result == AM_RESULT_ERR_AGAIN) {
        result = AM_RESULT_OK;
        break;
      }

      if ((video_file.find(frame_desc.video.stream_id)) == video_file.end()) {
        open_video_file(frame_desc.video.stream_id, frame_desc.video.stream_type);
      }
      if ((result = gaddress->video_addr_get(frame_desc, video_addr))
          != AM_RESULT_OK) {
        ERROR("Failed to get the address.");
        break;
      }
      if ((write(video_file[frame_desc.video.stream_id], video_addr.data,
          frame_desc.video.data_size)) != int(frame_desc.video.data_size)) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("Failed to write.");
        break;
      }
    }
    for (auto &m : video_file) {
      if (m.second >= 0) {
        close(m.second);
        m.second = -1;
      }
    }
  } while (0);
  return result;
}

static AM_RESULT dump_yuv(uint32_t buf_id, uint32_t count)
{
  AM_RESULT result = AM_RESULT_OK;
  int file_fd = -1;
  AMQueryFrameDesc frame_desc;
  struct iovec y = {0};
  struct iovec uv = {0};
  AMAddress yaddr = {0};
  AMAddress uvaddr = {0};
  uint8_t *y_buf = nullptr;
  uint8_t *uv_buf = nullptr;
  uint8_t *y_ptr = nullptr;
  uint8_t *uv_ptr = nullptr;
  int32_t uv_height = 0;
  struct iovec iov[2] = {0};
  char file_time[64] = {0};
  char filename[512] = {0};

  do {
    for (uint32_t j = 0; j < count; j++) {
      //get the system time
      time_t system_time = time(0);
      strftime(file_time, sizeof(file_time), "%Y%m%d%H%M%S",
               localtime(&system_time));
      if (AM_RESULT_OK != greader->query_yuv_frame(frame_desc,
                          AM_SOURCE_BUFFER_ID(buf_id), false)) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("Query yuv frame failed.");
        break;
      }
      snprintf(filename, 512, "%s_yuv_%d_%d_%d_%s_%d",file_name_base.c_str(),
               buf_id, frame_desc.yuv.width, frame_desc.yuv.height, file_time, j);

      if (AM_RESULT_OK != gaddress->yuv_y_addr_get(frame_desc, yaddr)) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("Failed to get y address!");
        break;
      }
      if (AM_RESULT_OK != gaddress->yuv_uv_addr_get(frame_desc, uvaddr)) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("Failed to get uv address!");
        break;
      }

      if (frame_desc.yuv.format == AM_CHROMA_FORMAT_YUV420) {
        uv_height = frame_desc.yuv.height / 2;
        INFO("The yuv format is 420.");
      } else if (frame_desc.yuv.format == AM_CHROMA_FORMAT_YUV422) {
        uv_height = frame_desc.yuv.height;
        INFO("The yuv format is 422.");
      } else {
        result = AM_RESULT_ERR_INVALID;
        ERROR("Unsupported chroma format in YUV dump.");
        break;
      }

      delete[] y_buf;
      y_buf = nullptr;
      delete[] uv_buf;
      uv_buf = nullptr;
      //copy y buffer
      y.iov_len = frame_desc.yuv.width * frame_desc.yuv.height;
      y_buf = new uint8_t[y.iov_len];
      if (!y_buf) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("New y buffer failed!");
        break;
      }
      y.iov_base = y_buf;
      y_ptr = y_buf;
      if (frame_desc.yuv.pitch == frame_desc.yuv.width) {
        memcpy(y.iov_base, yaddr.data, y.iov_len);
      } else {
        for (uint32_t i = 0; i < frame_desc.yuv.height ; i++) {
          memcpy(y_ptr, yaddr.data + i * frame_desc.yuv.pitch,
                 frame_desc.yuv.width);
          y_ptr += frame_desc.yuv.width;
        }
        memcpy(y.iov_base, y_ptr - y.iov_len, y.iov_len);
      }
      //copy uv buffer
      uv.iov_len = frame_desc.yuv.width * uv_height;
      uv_buf  = new uint8_t[uv.iov_len];
      if (!uv_buf) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("New uv buffer failed.");
        break;
      }
      uv.iov_base = uv_buf;
      uv_ptr = uv_buf;
      if (frame_desc.yuv.pitch == frame_desc.yuv.width) {
        memcpy(uv.iov_base, uvaddr.data, uv.iov_len);
      } else {
        for (int32_t i = 0; i < uv_height; i++) {
          memcpy(uv_ptr, uvaddr.data + i * frame_desc.yuv.pitch,
                 frame_desc.yuv.width);
          uv_ptr += frame_desc.yuv.width;
        }
        memcpy(uv.iov_base, uv_ptr - uv.iov_len, uv.iov_len);
      }
      iov[0] = y;
      iov[1] = uv;
      if ((file_fd = open(filename, O_WRONLY|O_CREAT, 0644)) < 0) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("Failed to create the file %s.", filename);
        break;
      }
      if (writev(file_fd, iov, 2) != int32_t(y.iov_len + uv.iov_len)) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("Failed to write data to files.");
      }
      close(file_fd);
      file_fd = -1;
    }
  } while (0);
  if (file_fd  > 0) {
    close(file_fd);
  }
  delete[] y_buf;
  delete[] uv_buf;
  return result;
}

static AM_RESULT dump_me1(uint32_t buf_id, uint32_t count)
{
  AM_RESULT result = AM_RESULT_OK;
  int file_fd = -1;
  AMQueryFrameDesc frame_desc;
  AMAddress me1addr = {0};
  uint8_t *me1_ptr = nullptr;
  uint8_t *me1_buf = nullptr;
  int32_t data_size = 0;
  char file_time[64] = {0};
  char filename[512] = {0};

  do {
    for (uint32_t j = 0; j < count; j++) {
      time_t system_time = time(0);
      strftime(file_time, sizeof(file_time), "%Y%m%d%H%M%S",
               localtime(&system_time));

      if (AM_RESULT_OK != greader->query_me1_frame(frame_desc,
                        AM_SOURCE_BUFFER_ID(buf_id), false)) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("Query me1 frame failed.");
        break;
      }

      snprintf(filename, 512, "%s_me1_%d_%d_%d_%s_%d",file_name_base.c_str(),
               buf_id, frame_desc.me.width, frame_desc.me.height, file_time, j);

      if (AM_RESULT_OK != gaddress->me1_addr_get(frame_desc, me1addr)) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("Failed to get me1 address!");
        break;
      }

      data_size = frame_desc.me.width * frame_desc.me.height;
      delete[] me1_buf;
      me1_buf = nullptr;
      me1_buf = new uint8_t[data_size];
      if (!me1_buf) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("New me1 buffer failed.");
        break;
      }
      me1_ptr = me1_buf;
      if ((file_fd = open(filename, O_WRONLY|O_CREAT, 0644)) < 0) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("Failed to create the file %s.", filename);
        break;
      }
      if (frame_desc.me.pitch == frame_desc.me.width) {
        if (write(file_fd, me1addr.data, data_size) != data_size) {
          result = AM_RESULT_ERR_INVALID;
          ERROR("Failed to write data to file.");
          break;
        }
      } else {
        for (uint32_t i = 0; i < frame_desc.me.height; i++) {
          memcpy(me1_ptr, me1addr.data + i * frame_desc.me.pitch,
                 frame_desc.me.width);
          me1_ptr += frame_desc.me.width;
        }
        if (write(file_fd, me1_ptr - data_size, data_size) != data_size) {
          result = AM_RESULT_ERR_INVALID;
          ERROR("Failed to write data to file.");
          break;
        }
      }
      close (file_fd);
      file_fd = -1;
    }
  } while(0);
  if (file_fd > 0) {
    close (file_fd);
  }
  delete[] me1_buf;
  return result;
}

static AM_RESULT dump_raw(uint32_t count)
{
  AM_RESULT result = AM_RESULT_OK;
  AMQueryFrameDesc frame_desc;
  AMAddress raw_addr = {0};
  int file_fd = -1;
  int32_t data_size = 0;
  char file_time[64] = {0};
  char filename[512] = {0};

  do {
    for (uint32_t j = 0; j < count; j++) {
      time_t system_time = time(0);
      strftime(file_time, sizeof(file_time), "%Y%m%d%H%M%S",
               localtime(&system_time));

      if ((result = greader->query_raw_frame(frame_desc)) != AM_RESULT_OK) {
        ERROR("Failed to query raw frame.");
        break;
      }
      snprintf(filename, 512, "%s_raw_%d_%d_%s_%d.raw", file_name_base.c_str(),
               frame_desc.raw.width, frame_desc.raw.height, file_time, j);
      data_size = frame_desc.raw.pitch * frame_desc.raw.height;

      if ((result = gaddress->raw_addr_get(frame_desc, raw_addr))!= AM_RESULT_OK) {
        ERROR("Failed to get the address.");
        break;
      }

      if ((file_fd = open(filename, O_WRONLY|O_CREAT, 0644)) < 0) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("Failed to create the file %s.", filename);
        break;
      }
      if (write(file_fd, raw_addr.data, data_size) != data_size) {
        result = AM_RESULT_ERR_INVALID;
        ERROR("Failed to write data to file.");
        break;
      }
      close(file_fd);
      file_fd = -1;
    }
  } while(0);
  if (file_fd > 0) {
    close(file_fd);
  }
  return result;
}

int main(int argc, char **argv)
{
  AM_RESULT result = AM_RESULT_OK;
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  do {
    if (argc < 2) {
      usage(argc, argv);
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (init_param(argc, argv) < 0) {
      ERROR("Failed to init param!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (!(greader = AMIVideoReader::get_instance())) {
      ERROR("Failed to create greader!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (!(gaddress = AMIVideoAddress::get_instance())) {
      ERROR("Failed to get instance of VideoAddress!");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (dump_video_flag) {
      if ((result = save_video()) != AM_RESULT_OK) {
        ERROR("Failed to save video.");
      }
    } else if (dump_yuv_flag) {
      if ((result = dump_yuv(dump_yuv_buffer_id, dump_count))
          != AM_RESULT_OK) {
        ERROR("Failed to dump yuv.");
      }
    } else if (dump_luma_flag) {
      if ((result = dump_me1(dump_luma_buffer_id, dump_count))
          != AM_RESULT_OK) {
        ERROR("Failed to dump me1.");
      }
    } else if (dump_raw_flag) {
      if ((result = dump_raw(dump_count)) != AM_RESULT_OK) {
        ERROR("Failed to dump raw.");
      }
    }
  } while (0);
  return int(result);
}
