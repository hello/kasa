/*******************************************************************************
 * test_oryx_data_export.cpp
 *
 * History:
 *   2015-01-04 - [Zhi He]      created file
 *   2015-04-02 - [Shupeng Ren] modified  file
 *   2016-07-08 - [Guohua Zheng] modified  file
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

#include <signal.h>
#include <map>

#include <getopt.h>
#include <condition_variable>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_export_if.h"
#include "am_thread.h"
#include "am_event.h"

#define NO_ARG 0
#define HAS_ARG 1

using std::map;

std::string namebase;
bool write_flag = 0;

map<uint8_t, int8_t> video_output_file;
map<uint8_t, map <uint8_t, int8_t>> audio_output_file;

static AMIExportClient* g_client = nullptr;
static bool running_flag = true;
static uint32_t video_choice = ~0;
static uint64_t audio_choice = ~0;
static uint32_t mode = 0;

AMThread *data_process_thread = nullptr;
AMEvent  *monitor_wait = nullptr;

AMExportConfig m_config;
AM_RESULT ret = AM_RESULT_OK;

static struct option long_options[]=
{
 {"help",            NO_ARG,   0, 'h'},
 {"mode",            HAS_ARG,  0, 'm'},
 {"video",           HAS_ARG,  0, 'v'},
 {"audio",           HAS_ARG,  0, 'a'},
 {"nodump",          NO_ARG,   0, 'n'},
 {"filename",        HAS_ARG,  0, 'f'},
};

static const char *short_options = "hm:v:a:nf:";

static void show_usage()
{
  PRINTF("test_oryx_data_export usage:\n");
  PRINTF("\t-f [%%s]: specify output filename base, "
      "when write the files, users could set data export dynamic"
      "final video file name will be 'filename_video_%%d.h264', "
      "same with '--filename'\n");
  PRINTF("\t-m [%%d]: specify sort mode of packet export,"
      "0: no-sort mode, 1: sort mode,"
      "same with '--mode'\n");
  PRINTF("\t-v [%%d]: specify the video stream to be exported,"
      "set by bit-mask, i.g. stream 0 + stream 1: -v 3(11)"
      "same with '--video'\n");
  PRINTF("\t-a [%%d]: specify the audio stream to be exported,"
      "set by bit-mask, i.g. stream 0 + stream 1: -a 3(11)"
      "same with '--audio'\n");
  PRINTF("\t--nodump: will not save file, print only, "
      "same with '-n'\n ");
  PRINTF("\t--help: show usage, same with -h\n");
}

static void show_interactive_usage()
{
  PRINTF("\n==========================================\n");
  PRINTF("b----------------begin set\n");
  PRINTF("a----------------audio map(bit mask)\n");
  PRINTF("v----------------video map(bit mask)\n");
  PRINTF("r----------------reset the export data\n");
  PRINTF("u----------------go back to previous menu\n");
  PRINTF("e----------------exit\n");
  PRINTF("==========================================\n");
  PRINTF("> ");

}
static AM_RESULT init_params(int argc, char **argv) {
  int32_t ch;
  int32_t option_index = 0;
  AM_RESULT ret = AM_RESULT_OK;
  while ((ch = getopt_long(argc, argv, short_options, long_options,
                           &option_index)) != -1) {
    switch (ch) {
      case 'h':
        ret = AM_RESULT_ERR_INVALID;
        break;
      case 'm':
        mode = atoi(optarg);
        break;
      case 'n':
        write_flag = 1;
        break;
      case 'v':
        video_choice = atoi(optarg);
        break;
      case 'a':
        audio_choice = atoll(optarg);
        break;
      case 'f':
        namebase = optarg;
        if (namebase.c_str() == nullptr) {
          ERROR("input filename error");
          ret = AM_RESULT_ERR_INVALID;
        }
        break;
      default:
        ERROR("unknown options");
        ret = AM_RESULT_ERR_INVALID;
        break;
    }
  }
  return ret;
}

static void __sigstop(int i)
{
  running_flag = false;
  monitor_wait->signal();
}

static void open_video_output_file(uint32_t stream_index,
                                   uint8_t packet_format)
{
  char filename[512] = {0};
  switch (packet_format) {
    case AM_EXPORT_PACKET_FORMAT_AVC: {
      snprintf(filename, 512, "%s_video_%d_%ld.h264",
               namebase.c_str(), stream_index, clock());
      if ((video_output_file[stream_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    case AM_EXPORT_PACKET_FORMAT_HEVC: {
      snprintf(filename, 512, "%s_video_%d_%ld.h265",
               namebase.c_str(), stream_index, clock());
      if ((video_output_file[stream_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    case AM_EXPORT_PACKET_FORMAT_MJPEG: {
      snprintf(filename, 512, "%s_video_%d_%ld.mjpeg",
               namebase.c_str(), stream_index, clock());
      if ((video_output_file[stream_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    default:
      ERROR("BAD video format %d", packet_format);
      break;
  }
}

static void open_audio_output_file(uint32_t stream_index,
                                   uint8_t packet_format,
                                   uint8_t sample_rate)
{
  char filename[512] = {0};
  uint8_t tem_index = (sample_rate / 16) > 1 ? 2:(sample_rate / 16);
  switch (packet_format) {
    case AM_EXPORT_PACKET_FORMAT_AAC_8KHZ:
    case AM_EXPORT_PACKET_FORMAT_AAC_16KHZ:
    case AM_EXPORT_PACKET_FORMAT_AAC_48KHZ: {
      snprintf(filename, 512, "%s_audio_%dkHz_%d_%ld.aac",
               namebase.c_str(), sample_rate,stream_index, clock());
      if ((audio_output_file[stream_index][tem_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    case AM_EXPORT_PACKET_FORMAT_G711MuLaw_8KHZ:
    case AM_EXPORT_PACKET_FORMAT_G711MuLaw_16KHZ:
    case AM_EXPORT_PACKET_FORMAT_G711MuLaw_48KHZ: {
      snprintf(filename, 512, "%s_audio_g711mu_%dkHz_%d_%ld.g711",
               namebase.c_str(), sample_rate, stream_index, clock());
      if ((audio_output_file[stream_index][tem_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    case AM_EXPORT_PACKET_FORMAT_G711ALaw_8KHZ:
    case AM_EXPORT_PACKET_FORMAT_G711ALaw_16KHZ:
    case AM_EXPORT_PACKET_FORMAT_G711ALaw_48KHZ: {
      snprintf(filename, 512, "%s_audio_g711a_%dkHz_%d_%ld.g711",
               namebase.c_str(), sample_rate, stream_index, clock());
      if ((audio_output_file[stream_index][tem_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    case AM_EXPORT_PACKET_FORMAT_G726_40KBPS: {
      snprintf(filename, 512, "%s_audio_g726_40_%dkHz_%d_%ld.g726",
               namebase.c_str(), sample_rate, stream_index, clock());
      if ((audio_output_file[stream_index][tem_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    case AM_EXPORT_PACKET_FORMAT_G726_32KBPS: {
      snprintf(filename, 512, "%s_audio_g726_32_%dkHz_%d_%ld.g726",
               namebase.c_str(), sample_rate, stream_index, clock());
      if ((audio_output_file[stream_index][tem_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    case AM_EXPORT_PACKET_FORMAT_G726_24KBPS: {
      snprintf(filename, 512, "%s_audio_g726_24_%dkHz_%d_%ld.g726",
               namebase.c_str(), sample_rate, stream_index, clock());
      if ((audio_output_file[stream_index][tem_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    case AM_EXPORT_PACKET_FORMAT_G726_16KBPS: {
      snprintf(filename, 512, "%s_audio_g726_16_%dkHz_%d_%ld.g726",
               namebase.c_str(), sample_rate, stream_index, clock());
      if ((audio_output_file[stream_index][tem_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    case AM_EXPORT_PACKET_FORMAT_PCM_8KHZ:
    case AM_EXPORT_PACKET_FORMAT_PCM_16KHZ:
    case AM_EXPORT_PACKET_FORMAT_PCM_48KHZ: {
      snprintf(filename, 512, "%s_audio_%dkHz_%d_%ld.pcm",
               namebase.c_str(), sample_rate, stream_index, clock());
      if ((audio_output_file[stream_index][tem_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    case AM_EXPORT_PACKET_FORMAT_OPUS_8KHZ:
    case AM_EXPORT_PACKET_FORMAT_OPUS_16KHZ:
    case AM_EXPORT_PACKET_FORMAT_OPUS_48KHZ: {
      snprintf(filename, 512, "%s_audio_%dkHz_%d_%ld.opus",
               namebase.c_str(), sample_rate, stream_index, clock());
      if ((audio_output_file[stream_index][tem_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    case AM_EXPORT_PACKET_FORMAT_BPCM_8KHZ:
    case AM_EXPORT_PACKET_FORMAT_BPCM_16KHZ:
    case AM_EXPORT_PACKET_FORMAT_BPCM_48KHZ: {
      snprintf(filename, 512, "%s_audio_%dkHz_%d_%ld.bpcm",
               namebase.c_str(), sample_rate, stream_index, clock());
      if ((audio_output_file[stream_index][tem_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    case AM_EXPORT_PACKET_FORMAT_SPEEX_8KHZ:
    case AM_EXPORT_PACKET_FORMAT_SPEEX_16KHZ:
    case AM_EXPORT_PACKET_FORMAT_SPEEX_48KHZ: {
      snprintf(filename, 512, "%s_audio_%dkHz_%d_%ld.speex",
               namebase.c_str(), sample_rate, stream_index, clock());
      if ((audio_output_file[stream_index][tem_index] =
          open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
        perror(filename);
      }
    } break;
    default:
      ERROR("BAD audio format %d", packet_format);
      break;
  }
}

static void write_packet(AMExportPacket* packet)
{
  uint8_t tem_index = (packet->audio_sample_rate)/16 > 1
      ? 2 : (packet->audio_sample_rate)/16;
  switch (packet->packet_type) {
    case AM_EXPORT_PACKET_TYPE_VIDEO_DATA:
      if (video_output_file.find(packet->stream_index) ==
          video_output_file.end()) {
        open_video_output_file(packet->stream_index, packet->packet_format);
      }
      if (write(video_output_file[packet->stream_index], packet->data_ptr,
                packet->data_size) != (ssize_t)packet->data_size) {
        switch (errno) {
          case EIO :
          case EINTR :
          case EAGAIN :
            for (auto &m : video_output_file) {
              close(m.second);
              video_output_file.erase(m.first);
            }
            break;
          default :
            PERROR("write failed");
        }
      }
      break;
    case AM_EXPORT_PACKET_TYPE_AUDIO_DATA:
      if (audio_output_file.find(packet->stream_index) ==
          audio_output_file.end() || audio_output_file
          [packet->stream_index].find(tem_index) ==
              audio_output_file[packet->stream_index].end()) {
        open_audio_output_file(packet->stream_index,
                               packet->packet_format,
                               packet->audio_sample_rate);
      }
      if (write(audio_output_file[packet->stream_index][tem_index],
                packet->data_ptr,
                packet->data_size) != (ssize_t)packet->data_size) {
        switch (errno) {
          case EIO :
          case EINTR :
          case EAGAIN:
            for (auto &m : audio_output_file) {
              for (auto &m_sub : m.second) {
                close(m_sub.second);
                audio_output_file[m.first].erase(m_sub.first);
              }
            }
            break;
          default :
            PERROR("write failed");
        }
      }
      break;
    default:
      NOTICE("discard non-video-audio packet here\n");
      break;
  }
}

static void data_process_fun(void *arg)
{
  AMExportPacket packet;
  int video_count_export = 0;
  int audio_count_export = 0;

  AMIExportClient* g_client = (AMIExportClient*) arg;
  do {


    if ((ret = g_client->connect_server(DEXPORT_PATH)) != AM_RESULT_OK) {
      ERROR("p_client->connect() failed, return code = %d\n",ret);
      break;
    }

    while (running_flag) {
      if (video_count_export == 30 || audio_count_export == 3) {
        video_count_export = 0;
        audio_count_export = 0;
      }
      if ((ret = g_client->receive(&packet)) == AM_RESULT_OK) {
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
                "samplerate: %d, "
                "channel: %d, sample size: %d\n",
                packet.stream_index,
                audio_info->samplerate,
                audio_info->channels,
                audio_info->sample_size);
          } break;
          case AM_EXPORT_PACKET_TYPE_VIDEO_DATA:
          case AM_EXPORT_PACKET_TYPE_AUDIO_DATA: {
            if (!write_flag) {
              write_packet(&packet);
            } else {
              printf("receive a packet, stream index %d, format %d, size %d\n",
                     packet.stream_index, packet.packet_type, packet.data_size);
            }
            if (packet.packet_type == AM_EXPORT_PACKET_TYPE_VIDEO_DATA) {
              video_count_export++;
            } else {
              audio_count_export++;
            }
          } break;
          default:
            break;
        }
        g_client->release(&packet);
      } else {
        running_flag = false;
        ERROR("receive_packet failed(return code = %d), server shut down",ret);
        break;
      }
    }
  } while (0);

  if (g_client) {
    g_client->disconnect_server();
    g_client->destroy();
  }

  for (auto &m : video_output_file) {
    close(m.second);
    m.second = -1;
  }

  for (auto &m : audio_output_file) {
    for (auto &m_sub : m.second) {
      close(m_sub.second);
      m_sub.second = -1;
    }
  }

}

int main(int argc, char *argv[])
{
  bool first_check = false;

  if (AM_UNLIKELY(2 > argc)) {
    show_usage();
    return (-10);
  }

  if ((ret = init_params(argc, argv)) != AM_RESULT_OK) {
    show_usage();
    return (-1);
  } else if (ret) {
    return ret;
  }
  m_config.need_sort = mode;
  m_config.video_map = video_choice;
  m_config.audio_map = audio_choice;

  signal(SIGINT, __sigstop);
  signal(SIGQUIT, __sigstop);
  signal(SIGTERM, __sigstop);
  do {

    if (!(monitor_wait = AMEvent::create())) {
      ERROR("Create Event failed");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }

    if (!(g_client = am_create_export_client(AM_EXPORT_TYPE_UNIX_DOMAIN_SOCKET,
                                             &m_config))) {
      ERROR("am_create_export_client() failed");
      ret = AM_RESULT_ERR_MEM;
      break;
    }

    if (!(data_process_thread = AMThread::create("data_process", data_process_fun,
                                           g_client))) {
      ERROR("Create Thread failed");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }

    NOTICE("read packet loop end");
  } while (0);

  if (1 == write_flag) {
    monitor_wait->wait();
  }

  while (running_flag ) {
    char input_char;
    AMExportConfig config_dyn;
    if (!first_check) {
      config_dyn.audio_map = audio_choice;
      config_dyn.video_map = video_choice;
      config_dyn.need_sort = 0;
      first_check = true;
    }
    show_interactive_usage();
    while((input_char = getchar()) != 'u' && running_flag)
    {
      switch (input_char) {
        case 'a':
          printf("please input audio map\n");
          scanf("%lld\n", &config_dyn.audio_map);
          break;
        case 'v':
          printf("please input video map\n");
          scanf("%d\n", &config_dyn.video_map);
          break;
        case 'r':
          if (g_client) {
            if (!(g_client->set_config(&config_dyn))) {
              printf("Update the map\n");
            };
          }
          break;
        case 'e':
          running_flag = false;
          break;
        default:
          break;
      }
    }
  }
  return ret;
}
