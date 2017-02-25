/*******************************************************************************
 * test_media_service_air_api.cpp
 *
 * History:
 *   2015-2-27 - [ccjing] created file
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
#include <limits.h>
#include <iostream>
#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"
#include "getopt.h"

#include "am_api_helper.h"
#include "am_api_media.h"

bool exit_flag = false;
enum short_opt {
  ADD_PLAYBACK_UNIX_DOMAIN  = 1,
  GET_PLAYBACK_INSTANCE_ID,
  RELEASE_PLAYBACK_INSTANCE_ID,
  START_FILE_MUXER,
  SET_FILE_FINISH_NOTIFY,
  CANCEL_FILE_FINISH_NOTIFY,
  SET_FILE_CREATE_NOTIFY,
  CANCEL_FILE_CREATE_NOTIFY,
  SET_FILE_DURATION,
  RELOAD_RECORD_ENGINE,
};

static void sigstop(int arg)
{
  PRINTF("catch sigstop, exit.");
  exit_flag = true;
}

static int on_notify(const void *msg_data, int msg_data_size)
{
  am_service_notify_payload *payload = (am_service_notify_payload *)msg_data;
  if (payload->type == AM_MEDIA_SERVICE_NOTIFY) {
    AMApiMediaFileInfo *file_info = (AMApiMediaFileInfo*)payload->data;
    if (file_info->type == AM_API_MEDIA_NOTIFY_TYPE_FILE_FINISH) {
      PRINTF("File finish callback func :\n"
             "         file name : %s\n"
             "create time string : %s\n"
             "finish time string : %s\n"
             "         stream id : %d\n"
             "          muxer id : %d\n",
          file_info->file_name, file_info->create_time_str,
          file_info->finish_time_str,file_info->stream_id, file_info->muxer_id);
    } else if (file_info->type == AM_API_MEDIA_NOTIFY_TYPE_FILE_CREATE) {
      PRINTF("File create callback func :\n"
             "         file name : %s\n"
             "create time string : %s\n"
             "finish time string : %s\n"
             "         stream id : %d\n"
             "          muxer id : %d\n",
          file_info->file_name, file_info->create_time_str,
          file_info->finish_time_str, file_info->stream_id, file_info->muxer_id);
    } else {
      PRINTF("Unknown media notify type.");
    }
  } else {
    PRINTF("receive unknown nofity type.");
  }
  return 0;
}

static struct option long_options[] =
{
  {"help",                        no_argument,       0,  'h'},

  {"start_file_muxer",            no_argument,       0,  START_FILE_MUXER },
  {"start_record_engine",         no_argument,       0,  'r' },
  {"stop_record_engine",          no_argument,       0,  's' },
  {"reload_record_engine",        no_argument,       0,  RELOAD_RECORD_ENGINE},
  {"set_file_finish_notify",      required_argument, 0,  SET_FILE_FINISH_NOTIFY},
  {"cancel_file_finish_notify",   required_argument, 0,  CANCEL_FILE_FINISH_NOTIFY},
  {"set_file_create_notify",      required_argument, 0,  SET_FILE_CREATE_NOTIFY},
  {"cancel_file_create_notify",   required_argument, 0,  CANCEL_FILE_CREATE_NOTIFY},
  {"add_audio_file",              required_argument, 0,  'a' },
  {"add_playback_unix_domain",    required_argument, 0,  ADD_PLAYBACK_UNIX_DOMAIN},
  {"get_playback_instance_id",    no_argument,       0,  GET_PLAYBACK_INSTANCE_ID},
  {"release_playback_instance_id",required_argument, 0,  RELEASE_PLAYBACK_INSTANCE_ID},
  {"start_audio_playback",        required_argument, 0,  'y' },
  {"stop_audio_playback",         required_argument, 0,  'n' },
  {"pause_audio_playback",        required_argument, 0,  'p' },
  {"start_file_writing",          required_argument, 0,  'd' },
  {"stop_file_writing",           required_argument, 0,  'b' },
  {"set_record_file_num",         required_argument, 0,  'f' },
  {"set_record_duration",         required_argument, 0,  'u' },
  {"set_file_duration",           required_argument, 0,  SET_FILE_DURATION},
  {"send_event",                  required_argument, 0,  'e' },
  {"set_periodic_jpeg_recording", required_argument, 0,  'j' },
  {"enable_audio_codec",          required_argument, 0,  'l' },
  {0,                        0,                 0,   0  }
};

static const char *short_options = "hrsa:y:n:p:d:b:f:u:e:j:l:";

struct hint32_t_s {
    const char *arg;
    const char *str;
};

static const hint32_t_s hint32_t[] =
{
  { "",            "\t\t\t\t\t\t\t"   "Show usage" },
  { "",            "\t\t\t\t\t\t" "Start file muxer." },
  { "",            "\t\t\t\t\t"   "Start record engine." },
  { "",            "\t\t\t\t\t"   "Stop record engine." },
  { "",            "\t\t\t\t\t"   "reload record engine." },
  { "muxer_id_bit_map", "\t\t"    "set file finish notify"},
  { "muxer_id_bit_map", "\t\t"    "cancel file finish notify"},
  { "muxer_id_bit_map", "\t\t"    "set file create notify"},
  { "muxer_id_bit_map", "\t\t"    "cancel file create notify"},
  { "id*file_name",   "\t\t\t\t"  "Add audio file to playback" },
  { "playback_id*unix_domain_name*audio_type*sample_rate*channel",
    "\n\t\t\t\t\t\t\t\t\tAdd unix domain uri to playback" },
  { "",            "\t\t\t\t\t"   "Get playback instance id." },
  { "playback id", "\t\t"         "Release playback instance id" },
  { "playback id", "\t\t\t"       "Start audio file playback." },
  { "playback id", "\t\t\t"       "Stop audio file playback." },
  { "playback id", "\t\t\t"       "Pause audio file playback." },
  { "muxer_id_bit_map", "\t\t\t"  "Start file writing." },
  { "muxer_id_bit_map", "\t\t\t"  "Stop file writing." },
  { "muxer_id_bit_map*recording_file_num", "set_recording_file_num. \n"
      "\t\t\t\t\t\t\t\t\t'recording_file_num' means that the file recording \n"
      "\t\t\t\t\t\t\t\t\twould be stopped automatically when file number reach\n"
      "\t\t\t\t\t\t\t\t\tthis value. 0 means file recording would not be stopped\n"
      "\t\t\t\t\t\t\t\t\tuntil stop cmd was received." },
  { "muxer_id_bit_map*recording_duration", "Set recording duration. \n"
      "\t\t\t\t\t\t\t\t\t'recording_duration' means that the file recording\n"
      "\t\t\t\t\t\t\t\t\twould be stopped automatically when recording\n"
    "\t\t\t\t\t\t\t\t\tduration reach this value. 0 means file recording\n"
    "\t\t\t\t\t\t\t\t\twould not be stopped until stop cmd was received." },
  { "muxer_id_bit_map*file_duration*apply_conf_file",
    "\n\t\t\t\t\t\t\t\t\tSet file duration."},
  { "(h26x: h*stream_id*history_duration*future_duration)"
    "(mjpeg: m*stream_id*pre_num*after_num*closest_num)"
    "(event_stop : s*stream_id)",
    "\n\t\t\t\t\t\t\t\t\tSend event.\n"
    "\t\t\t\t\t\t\t\t\t'h' menas h_26xevent,\n"
    "\t\t\t\t\t\t\t\t\t'm' means mjpeg event. If you want to set closest_num,\n"
    "\t\t\t\t\t\t\t\t\tpre_num and after_num must be set to 0.\n"
    "\t\t\t\t\t\t\t\t\t's' means stop event cmd." },
  { "stream_id*start_time*interval_second*once_jpeg_num*end_time",
     "\n\t\t\t\t\t\t\t\t\tset periodic jpeg recording. start_time: hh-mm-ss,\n"
     "\t\t\t\t\t\t\t\t\tinterval second: ss, once_jpeg_num: nn, end_time: hh-mm-ss"},
  { "audio_type*sample_rate*enable", "\taudio codec enalbe."
     "\n\t\t\t\t\t\t\t\t\taudio_type : aac, opus, g711A, g711U, g726_40, g726_32, g726_24, g726_16 speex"
     "\n\t\t\t\t\t\t\t\t\tsample_rate : 8000, 16000, 48000. enable : 1(enable), 0(disable)"},
};

static void usage(int32_t argc, char **argv)
{
  printf("\n%s usage:\n", argv[0]);
  for (uint32_t i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1;
      ++ i) {
    if (isalpha(long_options[i].val)) {
      printf("-%c,  ", long_options[i].val);
    } else {
      printf("     ");
    }
    printf("--%s", long_options[i].name);
    if (hint32_t[i].arg[0] != 0) {
      printf(" [%s]", hint32_t[i].arg);
    }
    printf("\t%s\n", hint32_t[i].str);
  }
  printf("Examples:\n  %s -r\n"
         "  %s -s\n"
         "  %s --reload_record_engine\n"
         "  %s -a 0*audio.aac -a 0*audio2.aac\n"
         "  %s --get_playback_instance_id\n"
         "  %s --release_playback_instance_id 0\n"
         "  %s --add_playback_unix_domain 1*demuxer.unix_domain*aac*48000*2\n"
         "  %s --start_file_muxer\n"
         "  %s -y 0 \t\t\t\tNOTICE: 0 means playback instance id\n"
         "  %s -n 0\n"
         "  %s -p 0\n"
         "  %s -d 7 \t\t\t\tNOTICE: 7 means start muxer0 muxer1 and muxer2 "
         "((1 << 0) | (1 << 1) | (1 << 2)),\n"
         "  %s -b 6 \t\t\t\tNOTICE: 6 means stop muxer1 and muxer2 "
         "((1 << 1) | (1 << 2))\n"
         "  %s -f 4*3 \t\t\t\tNOTICE: set muxer2(0x01 << 2) recording "
         "file number to be 3.\n"
         "  %s --set_file_duration 1*20*0  \tNOTICE: 0 means do not apply to config file"
         ", 1 means apply to config file\n"
         "  %s -u 1*20 \t\t\t\tNOTICE: set muxer0(0x01 << 0) recording duration"
         " to be 20 seconds\n"
         "  %s -e h*0*6*10 \t\t\tNOTICE: 'h' means h26x event, 0 means stream0\n"
         "  %s -e m*1*2*3*0  \t\t\tNOTICE: 'm' means mjpeg event, 1 means stream1,\n"
         "\t\t\t\t\t\t\t\t\t2 means before current pts number is 2,\n"
         "\t\t\t\t\t\t\t\t\t3 means after current pts number is 3,\n"
         "\t\t\t\t\t\t\t\t\t0 means closest current pts number is 0.\n"
         "  %s -e m*1*0*0*4  \t\t\tNOTICE: 4 means closest current pts number is 4.\n"
         "  %s -j 0*8-20-30*100*2*12-2-2 \t\tNOTICE: 0 means stream_id\n"
         "\t\t\t\t\t\t\t\t\t8-20-30 means start time is 8:20:30\n"
         "\t\t\t\t\t\t\t\t\t100 means interval second is 100 seconds\n"
         "\t\t\t\t\t\t\t\t\t2 means 2 jpeg files will be recorded per interval second\n"
         "\t\t\t\t\t\t\t\t\t12-2-2 means end time is 12:2:2\n"
         "  %s -l aac*8000*1     \t\t\tNOTICE: enable aac-8K codec.\n"
         "  %s -l opus*16000*0   \t\t\tNOTICE: disable opus-16k codec\n"
         "  %s -l g711A*48000*1  \t\t\tNOTICE: enable g711A-48k codec\n"
         "  %s -l g726_40*8000*0 \t\t\tNOTICE: disable g726_40-8K codec\n"
         "  %s --set_file_finish_notify 5 \tNOTICE: muxer_id_bit_map = 5 "
         "means set muxer0 and muxer2. ((0x01 << 0) | (0x01 << 2))\n"
         "  %s --cancel_file_finish_notify 5\n"
         "  %s --set_file_create_notify 5\n"
         "  %s --cancel_file_create_notify 5\n",
         argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0],
         argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0],
         argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0],
         argv[0], argv[0], argv[0], argv[0]);
}

bool parse_event_arg(std::string event_arg, AMIApiMediaEvent& event);
bool parse_periodic_jpeg_arg(std::string periodic_jpeg_arg,
                             AMIApiMediaEvent& periodic_jpeg);
bool parse_recording_duration(std::string arg, AMIApiRecordingParam& data);
bool parse_recording_file_num(std::string arg, AMIApiRecordingParam& data);
bool parse_file_duration(std::string arg, AMIApiRecordingParam& data);
bool parse_audio_codec_arg(std::string arg, AMIApiAudioCodecParam& data);
bool parse_playback_unix_domain_arg(std::string arg,
                                    AMIApiPlaybackUnixDomainUri& data);
bool parse_add_file_arg(std::string arg, AMIApiPlaybackAudioFileList& data);
int main(int argc, char **argv)
{
  int ret = 0;
  am_service_result_t result;//method call result
  AMAPIHelperPtr g_api_helper = nullptr;
  AMIApiPlaybackAudioFileList *audio_file = nullptr;
  AMIApiMediaEvent *event = nullptr;
  AMIApiMediaEvent *periodic_jpeg = nullptr;
  AMIApiRecordingParam *recording_param = nullptr;
  AMIApiAudioCodecParam *audio_codec_param = nullptr;
  AMIApiPlaybackUnixDomainUri *playback_unix_domain = nullptr;
  AMIAPiFileOperationParam* file_operation_param = nullptr;
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  do {
    audio_file = AMIApiPlaybackAudioFileList::create();
    if(!audio_file) {
      ERROR("Failed to create AMIApiPlaybackAudioFileList");
      ret = -1;
      break;
    }
    if (argc < 2) {
      usage(argc, argv);
      ret = -1;
      break;
    }
    AMAPIHelperPtr g_api_helper = nullptr;
    if ((g_api_helper = AMAPIHelper::get_instance()) == nullptr) {
      ERROR("CCWAPIConnection::get_instance failed.\n");
      ret = -1;
      break;
    }
    int opt = 0;
    while((opt = getopt_long(argc, argv, short_options, long_options,
                             nullptr)) != -1) {
      switch (opt) {
        case 'h' : {
          usage(argc, argv);
        } break;
        case 'r' : {
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_START_RECORDING,
                                    nullptr, 0, &result,
                                    sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to start recording engine!");
            ret = -1;
            break;
          }
        } break;
        case 's' : {
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_STOP_RECORDING,
                                    nullptr, 0, &result,
                                    sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to stop recording!");
            ret = -1;
            break;
          }
        } break;
        case RELOAD_RECORD_ENGINE : {
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_RELOAD_RECORD_ENGINE,
                                    nullptr, 0, &result,
                                    sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to reload record engine!");
            ret = -1;
            break;
          }
        } break;
        case GET_PLAYBACK_INSTANCE_ID : {
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_GET_PLAYBACK_INSTANCE_ID,
                                    nullptr, 0, &result,
                                    sizeof(am_service_result_t));
          int32_t playback_id = ((result.data[0] & 0x000000ff) << 24) |
                                ((result.data[1] & 0x000000ff) << 16) |
                                ((result.data[2] & 0x000000ff) << 8)  |
                                ((result.data[3] & 0x000000ff));
          printf("playback id is %d\n", playback_id);
          if (playback_id < 0) {
            ERROR("Get playback id error.");
            ret = -1;
          }
        } break;
        case RELEASE_PLAYBACK_INSTANCE_ID : {
          int32_t playback_id = atoi(optarg);
            g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_RELEASE_PLAYBACK_INSTANCE_ID,
                                      &playback_id, sizeof(int32_t), &result,
                                      sizeof(am_service_result_t));
            if (result.ret < 0) {
              ERROR("Failed to release playback instance id, id is %d", playback_id);
              ret = -1;
              break;
            }
        } break;
        case 'a' : {
          if (!parse_add_file_arg(optarg, *audio_file)) {
            ERROR("Failed to parse file information.");
            ret = -1;
            break;
          }
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE,
                                    audio_file->get_file_list(),
                                    audio_file->get_file_list_size(),
                                    &result,
                                    sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to add audio file.");
            ret = -1;
          }
          audio_file->clear_file();
        } break;
        case ADD_PLAYBACK_UNIX_DOMAIN : {
          std::string data = optarg;
          playback_unix_domain = AMIApiPlaybackUnixDomainUri::create();
          if (!parse_playback_unix_domain_arg(data, *playback_unix_domain)) {
            ERROR("Failed to parse playback unix domain arg.");
            ret = -1;
            break;
          }
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_ADD_PLAYBACK_UNIX_DOMAIN,
                                    playback_unix_domain->get_data(),
                                    playback_unix_domain->get_data_size(),
                                    &result, sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to set playback unix domain uri!");
            ret = -1;
            break;
          }
        } break;
        case SET_FILE_FINISH_NOTIFY : {
          uint32_t muxer_id_bit_map = atoi(optarg);
          file_operation_param = AMIAPiFileOperationParam::create();
          if (!file_operation_param) {
            ERROR("Failed to create file_finish_param");
            ret = -1;
            break;
          }
          file_operation_param->set_muxer_id_bit_map(muxer_id_bit_map);
          file_operation_param->enable_callback_notify();
          file_operation_param->set_file_finish_notify();
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK,
                                    file_operation_param->get_data(),
                                    file_operation_param->get_data_size(),
                                    &result, sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to set file finish callback!");
            ret = -1;
            break;
          }
          g_api_helper->register_notify_cb(on_notify);
          while(!exit_flag) {
            sleep(1);
          }
        } break;
        case CANCEL_FILE_FINISH_NOTIFY : {
          uint32_t muxer_id_bit_map = atoi(optarg);
          file_operation_param = AMIAPiFileOperationParam::create();
          if (!file_operation_param) {
            ERROR("Failed to create file_finish_param");
            ret = -1;
            break;
          }
          file_operation_param->set_muxer_id_bit_map(muxer_id_bit_map);
          file_operation_param->disable_callback_notify();
          file_operation_param->set_file_finish_notify();
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK,
                                    file_operation_param->get_data(),
                                    file_operation_param->get_data_size(),
                                    &result, sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to cancel file finish callback!");
            ret = -1;
            break;
          }
        } break;
        case SET_FILE_CREATE_NOTIFY : {
          uint32_t muxer_id_bit_map = atoi(optarg);
          file_operation_param = AMIAPiFileOperationParam::create();
          if (!file_operation_param) {
            ERROR("Failed to create file_operation_param");
            ret = -1;
            break;
          }
          file_operation_param->set_muxer_id_bit_map(muxer_id_bit_map);
          file_operation_param->enable_callback_notify();
          file_operation_param->set_file_create_notify();
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK,
                                    file_operation_param->get_data(),
                                    file_operation_param->get_data_size(),
                                    &result, sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to set file create callback!");
            ret = -1;
            break;
          }
          g_api_helper->register_notify_cb(on_notify);
          while(!exit_flag) {
            sleep(1);
          }
        } break;
        case CANCEL_FILE_CREATE_NOTIFY : {
          uint32_t muxer_id_bit_map = atoi(optarg);
          file_operation_param = AMIAPiFileOperationParam::create();
          if (!file_operation_param) {
            ERROR("Failed to create file_operation_param");
            ret = -1;
            break;
          }
          file_operation_param->set_muxer_id_bit_map(muxer_id_bit_map);
          file_operation_param->disable_callback_notify();
          file_operation_param->set_file_create_notify();
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK,
                                    file_operation_param->get_data(),
                                    file_operation_param->get_data_size(),
                                    &result,
                                    sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to cancel file create callback!");
            ret = -1;
            break;
          }
        } break;
        case START_FILE_MUXER : {
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_START_FILE_MUXER,
                                    nullptr, 0, &result,
                                    sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to start file muxer!");
            ret = -1;
            break;
          }
        } break;
        case 'y' : {
          int32_t playback_id = atoi(optarg);
          if (playback_id < 0) {
            ERROR("There is no valid playback id.");
            ret = -1;
            break;
          }
          g_api_helper->method_call(
          AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE,
          &playback_id, sizeof(int32_t), &result, sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to start playback audio file.");
            ret = -1;
            break;
          }
        } break;
        case 'n' : {
          int32_t playback_id = atoi(optarg);
          if (playback_id < 0) {
            ERROR("There is no valid playback id.");
            ret = -1;
            break;
          }
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE,
                        &playback_id, sizeof(int32_t), &result,
                        sizeof(am_service_result_t));
          if(result.ret < 0) {
            ERROR("Failed to stop playback audio file.");
            ret = -1;
            break;;
          }
        } break;
        case 'p' : {
          int32_t playback_id = atoi(optarg);
          if (playback_id < 0) {
            ERROR("There is no valid playback id.");
            ret = -1;
            break;
          }
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE,
                        &playback_id, sizeof(int32_t), &result,
                        sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to pause playback audio file.");
            ret = -1;
            break;;
          }
        } break;
        case 'd' : {
          uint32_t muxer_id_bit_map = atoi(optarg);
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING,
                             &muxer_id_bit_map, sizeof(muxer_id_bit_map),
                             &result, sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to start file recording!");
            ret = -1;
            break;
          }
        } break;
        case 'b' : {
          uint32_t muxer_id_bit_map = atoi(optarg);
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING,
                             &muxer_id_bit_map, sizeof(muxer_id_bit_map),
                             &result, sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to stop file recording!");
            ret = -1;
            break;
          }
        } break;
        case 'f' : {
          std::string data = optarg;
          recording_param = AMIApiRecordingParam::create();
          if (!parse_recording_file_num(data, *recording_param)) {
            ERROR("Failed to parse recording param.");
            ret = -1;
            break;
          }
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_RECORDING_FILE_NUM,
                                    recording_param->get_data(),
                                    recording_param->get_data_size(),
                                    &result, sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to set recording file num!");
            ret = -1;
            break;
          }
        } break;
        case 'u' : {
          std::string data = optarg;
          recording_param = AMIApiRecordingParam::create();
          if (!parse_recording_duration(data, *recording_param)) {
            ERROR("Failed to parse recording duration.");
            ret = -1;
            break;
          }
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_RECORDING_DURATION,
                                    recording_param->get_data(),
                                    recording_param->get_data_size(),
                                    &result, sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to set recording duration!");
            ret = -1;
            break;
          }
        } break;
        case SET_FILE_DURATION : {
          std::string data = optarg;
          recording_param = AMIApiRecordingParam::create();
          if (!parse_file_duration(data, *recording_param)) {
            ERROR("Failed to parse file duration.");
            ret = -1;
            break;
          }
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_FILE_DURATION,
                                    recording_param->get_data(),
                                    recording_param->get_data_size(),
                                    &result, sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to set file duration!");
            ret = -1;
            break;
          }
        } break;
        case 'e' : {
          std::string event_arg = optarg;
          event = AMIApiMediaEvent::create();
          if (!event) {
            ERROR("Failed to create AMIApiMediaEventStruct");
            ret = -1;
            break;
          }
          if (!parse_event_arg(event_arg, *event)) {
            ERROR("Failed to parse event arg.");
            ret = -1;
            break;
          }
          if (event->is_attr_event_stop_cmd()) {
            g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_STOP,
                                      event->get_data(), event->get_data_size(),
                                      &result, sizeof(am_service_result_t));
            if (result.ret < 0) {
              ERROR("Event stop error!");
              ret = -1;
              break;
            }
          } else {
            g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START,
                                      event->get_data(), event->get_data_size(),
                                      &result, sizeof(am_service_result_t));
            if (result.ret < 0) {
              ERROR("Event start error!");
              ret = -1;
              break;
            }
          }
        } break;
        case 'j' : {
          std::string periodic_arg = optarg;
          periodic_jpeg = AMIApiMediaEvent::create();
          if (!periodic_jpeg) {
            ERROR("Failed to create AMIApiMediaPeriodicMjpeg");
            ret = -1;
            break;
          }
          if (!parse_periodic_jpeg_arg(periodic_arg, *periodic_jpeg)) {
            ERROR("Failed to parse event arg.");
            ret = -1;
            break;
          }
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_PERIODIC_JPEG_RECORDING,
                    periodic_jpeg->get_data(), periodic_jpeg->get_data_size(),
                     &result, sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to set periodic jpeg recording!");
            ret = -1;
            break;
          }
        } break;
        case 'l' : {
          std::string arg = optarg;
          audio_codec_param = AMIApiAudioCodecParam::create();
          if (!audio_codec_param) {
            ERROR("Failed to create audio codec param");
            ret = false;
            break;
          }
          if (!parse_audio_codec_arg(arg, *audio_codec_param)) {
            ERROR("Failed to parse audio codec arg.");
            ret = -1;
            break;
          }
          g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_ENABLE_AUDIO_CODEC,
                                    audio_codec_param->get_data(),
                                    audio_codec_param->get_data_size(),
                                    &result, sizeof(am_service_result_t));
          if (result.ret < 0) {
            ERROR("Failed to enable audio codec!");
            ret = -1;
            break;
          }
        } break;
        default : {
          ERROR("Invalid short_options");
          ret = -1;
        } break;
      }
    }
  } while (0);
  delete audio_file;
  delete event;
  delete periodic_jpeg;
  delete recording_param;
  delete audio_codec_param;
  delete playback_unix_domain;
  delete file_operation_param;
  return ret;
}

bool parse_event_arg(std::string event_arg, AMIApiMediaEvent& event)
{
  bool ret = true;
  do {
    std::string find_flag = "*";
    std::string::size_type pos = 0;
    switch (event_arg[0]) {
      case 'h' : {
        event.set_attr_h26X();
        if (!event.set_stream_id(atoi(&(event_arg[2])))) {
          ERROR("Failed to set stream id.");
          ret = false;
        }
        pos = 2;
        if ((pos = event_arg.find(find_flag, pos)) != std::string::npos) {
          pos += 1;
          if (!event.set_history_duration(atoi(&(event_arg[pos])))) {
            ERROR("Failed to set history duration");
            ret = false;
            break;
          }
        } else {
          //ERROR("h26x event param is invalid.");
          //ret = false;
          break;
        }
        if ((pos = event_arg.find(find_flag, pos)) != std::string::npos) {
          pos += 1;
          if (!event.set_future_duration(atoi(&(event_arg[pos])))) {
            ERROR("Failed to set future duration");
            ret = false;
            break;
          }
        } else {
          ERROR("h26x event param is invalid.");
          ret = false;
          break;
        }
        INFO("Receive h26x event, stream id is %u, history duration is %u,"
            "future duration is %u", event.get_stream_id(),
            event.get_history_duration(), event.get_future_duration());
      } break;
      case 'm' : {
        event.set_attr_mjpeg();
        if (!event.set_stream_id(atoi(&(event_arg[2])))) {
          ERROR("Failed to set stream id.");
          ret = false;
          break;
        }
        pos = 2;
        if ((pos = event_arg.find(find_flag, pos)) != std::string::npos) {
          pos += 1;
          if (!event.set_pre_cur_pts_num(atoi(&(event_arg[pos])))) {
            ERROR("Failed to set pre cur pts num");
            ret = false;
            break;
          }
        } else {
          ERROR("Mjpeg event param is invalid.");
          ret = false;
          break;
        }
        if ((pos = event_arg.find(find_flag, pos)) != std::string::npos) {
          pos += 1;
          if (!event.set_after_cur_pts_num(atoi(&(event_arg[pos])))) {
            ERROR("Failed to set after cur pts num");
            ret = false;
            break;
          }
        } else {
          ERROR("Mjpeg event param is invalid.");
          ret = false;
          break;
        }
        if ((pos = event_arg.find(find_flag, pos)) != std::string::npos) {
          pos += 1;
          if (!event.set_closest_cur_pts_num(atoi(&(event_arg[pos])))) {
            ERROR("Failed to set closest cur pts num");
            ret = false;
            break;
          }
        } else {
          ERROR("Mjpeg event param is invalid.");
          ret = false;
          break;
        }
        INFO("Receive mjpeg event, stream id is %u, pre num is %u,"
            "after num is %u, closest num is %u", event.get_stream_id(),
            event.get_pre_cur_pts_num(), event.get_after_cur_pts_num(),
            event.get_closest_cur_pts_num());
      } break;
      case 's' : {
        event.set_attr_event_stop_cmd();
        if (!event.set_stream_id(atoi(&(event_arg[2])))) {
          ERROR("Failed to set stream id.");
          ret = false;
        }
      } break;
      default : {
        ERROR("Event attr error : %c", event_arg[0]);
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool parse_recording_file_num(std::string arg, AMIApiRecordingParam& data)
{
  bool ret = true;
  do {
    std::string flag = "*";
    if (!data.set_muxer_id_bit_map(atoi(&(arg[0])))) {
      ERROR("Failed to set muxer id");
      ret = false;
      break;
    }
    std::string::size_type pos = 0;
    if ((pos = arg.find(flag, pos)) != std::string::npos) {
      pos += 1;
      if (!data.set_recording_file_num(atoi(&(arg[pos])))) {
        ERROR("Failed to set recording file number");
        ret = false;
        break;
      }
    } else {
      data.set_recording_file_num(0);
      NOTICE("Do not find recording file num, set it 0 as default.");
    }
  } while(0);
  return ret;
}

bool parse_recording_duration(std::string arg, AMIApiRecordingParam& data)
{
  bool ret = true;
  do {
    std::string flag = "*";
    if (!data.set_muxer_id_bit_map(atoi(&(arg[0])))) {
      ERROR("Failed to set muxer id");
      ret = false;
      break;
    }
    std::string::size_type pos = 0;
    if ((pos = arg.find(flag, pos)) != std::string::npos) {
      pos += 1;
      if (!data.set_recording_duration(atoi(&(arg[pos])))) {
        ERROR("Failed to set recording duration");
        ret = false;
        break;
      }
    } else {
      data.set_recording_duration(0);
      NOTICE("Do not find recording duration, set it 0 as default.");
    }
  } while(0);
  return ret;
}

bool parse_file_duration(std::string arg, AMIApiRecordingParam& data)
{
  bool ret = true;
  do {
    std::string flag = "*";
    if (!data.set_muxer_id_bit_map(atoi(&(arg[0])))) {
      ERROR("Failed to set muxer id");
      ret = false;
      break;
    }
    std::string::size_type pos = 0;
    if ((pos = arg.find(flag, pos)) != std::string::npos) {
      pos += 1;
      if (!data.set_file_duration(atoi(&(arg[pos])))) {
        ERROR("Failed to set file duration");
        ret = false;
        break;
      }
    } else {
      ERROR("Parse file duration error.");
      ret = false;
      break;
    }
    if ((pos = arg.find(flag, pos)) != std::string::npos) {
      pos += 1;
      int32_t flag = atoi(&(arg[pos]));
      if (flag > 0) {
        if (!data.apply_conf_file(true)) {
          ERROR("Failed to set file duration");
          ret = false;
          break;
        }
      } else {
        if (!data.apply_conf_file(false)) {
          ERROR("Failed to set file duration");
          ret = false;
          break;
        }
      }
    }
  } while(0);
  return ret;
}

bool parse_periodic_jpeg_arg(std::string periodic_jpeg_arg,
                             AMIApiMediaEvent& periodic_jpeg)
{
  bool ret = true;
  do {
    std::string external_flag = "*";
    std::string internal_flag = "-";
    std::string::size_type pos = 0;
    periodic_jpeg.set_attr_periodic_mjpeg();
    // stream_id
    if (!periodic_jpeg.set_stream_id(atoi(&(periodic_jpeg_arg[0])))) {
      ERROR("Failed to set event id.");
      ret = false;
      break;
    }
    pos = 0;
    //start time
    if ((pos = periodic_jpeg_arg.find(external_flag, pos)) != std::string::npos) {
      pos += 1;
      if (!periodic_jpeg.set_start_time_hour(atoi(&(periodic_jpeg_arg[pos])))) {
        ERROR("Failed to set start time hour.");
        ret = false;
        break;
      }
      if ((pos = periodic_jpeg_arg.find(internal_flag, pos)) != std::string::npos) {
        pos += 1;
        if (!periodic_jpeg.set_start_time_minute(atoi(&(periodic_jpeg_arg[pos])))) {
          ERROR("Failed to set start time minute.");
          ret = false;
          break;
        }
        if ((pos = periodic_jpeg_arg.find(internal_flag, pos)) != std::string::npos) {
          pos += 1;
          if (!periodic_jpeg.set_start_time_second(atoi(&(periodic_jpeg_arg[pos])))) {
            ERROR("Failed to set start time second");
            ret = false;
            break;
          }
        } else {
          ERROR("Periodic jepg recording param is invalid");
          ret = false;
          break;
        }
      } else {
        ERROR("Periodic jpeg recording param is invalid.");
        ret = false;
        break;
      }
    } else {
      ERROR("Periodic jepg recording param is invalid.");
      ret = false;
      break;
    }
    //interval second
    if ((pos = periodic_jpeg_arg.find(external_flag, pos)) != std::string::npos) {
      pos += 1;
      if (!periodic_jpeg.set_interval_second(atoi(&(periodic_jpeg_arg[pos])))) {
        ERROR("Failed to set interval second");
        ret = false;
        break;
      }
    } else {
      ERROR("Periodic jepg recording param is invalid.");
      ret = false;
      break;
    }
    //once jpeg number
    if ((pos = periodic_jpeg_arg.find(external_flag, pos)) != std::string::npos) {
      pos += 1;
      if (!periodic_jpeg.set_once_jpeg_num(atoi(&(periodic_jpeg_arg[pos])))) {
        ERROR("Failed to set once jpeg number");
        ret = false;
        break;
      }
    } else {
      ERROR("Periodic jepg recording param is invalid.");
      ret = false;
      break;
    }
    // end time
    if ((pos = periodic_jpeg_arg.find(external_flag, pos)) != std::string::npos) {
      pos += 1;
      if (!periodic_jpeg.set_end_time_hour(atoi(&(periodic_jpeg_arg[pos])))) {
        ERROR("Failed to set end time hour.");
        ret = false;
        break;
      }
      if ((pos = periodic_jpeg_arg.find(internal_flag, pos)) != std::string::npos) {
        pos += 1;
        if (!periodic_jpeg.set_end_time_minute(atoi(&(periodic_jpeg_arg[pos])))) {
          ERROR("Failed to set end time minute.");
          ret = false;
          break;
        }
        if ((pos = periodic_jpeg_arg.find(internal_flag, pos)) != std::string::npos) {
          pos += 1;
          if (!periodic_jpeg.set_end_time_second(atoi(&(periodic_jpeg_arg[pos])))) {
            ERROR("Failed to set end time second");
            ret = false;
            break;
          }
        } else {
          ERROR("Periodic jepg recording param is invalid");
          ret = false;
          break;
        }
      } else {
        ERROR("Periodic jpeg recording param is invalid.");
        ret = false;
        break;
      }
    } else {
      ERROR("Periodic jpeg recording param is invalid.");
      ret = false;
      break;
    }
    INFO("Receive periodic jpeg recording, stream id is %u, start time is "
        "%u-%u-%u, interval second is %u, end time is %u-%u-%u",
        periodic_jpeg.get_stream_id(), periodic_jpeg.get_start_time_hour(),
        periodic_jpeg.get_start_time_minute(), periodic_jpeg.get_start_time_second(),
        periodic_jpeg.get_interval_second(), periodic_jpeg.get_end_time_hour(),
        periodic_jpeg.get_end_time_minute(), periodic_jpeg.get_end_time_second());
  } while(0);
  return ret;
}

bool parse_audio_codec_arg(std::string arg, AMIApiAudioCodecParam& data)
{
  bool ret = true;
  do {
    std::string flag = "*";
    std::string::size_type pos = 0;
    std::string audio_type;
    if ((pos = arg.find(flag, pos)) != std::string::npos) {
      audio_type = arg.substr(0, (pos - 0));
      if (audio_type == std::string("aac")) {
        data.set_audio_type_aac();
      } else if (audio_type == std::string("opus")) {
        data.set_audio_type_opus();
      } else if (audio_type == std::string("g711A")) {
        data.set_audio_type_g711A();
      } else if (audio_type == std::string("g711U")) {
        data.set_audio_type_g711U();
      } else if (audio_type == std::string("g726_40")) {
        data.set_audio_type_g726_40();
      } else if (audio_type == std::string("g726_32")) {
        data.set_audio_type_g726_32();
      } else if (audio_type == std::string("g726_24")) {
        data.set_audio_type_g726_24();
      } else if (audio_type == std::string("g726_16")) {
        data.set_audio_type_g726_16();
      } else if (audio_type == std::string("speex")) {
        data.set_audio_type_speex();
      } else {
        ERROR("Audio type invalid");
        ret = false;
        break;
      }
    } else {
      ERROR("Enable audio codec param is invalid.");
      ret = false;
      break;
    }
    pos += 1;
    uint32_t sample_rate = atoi(&(arg[pos]));
    if ((sample_rate != 8000) && (sample_rate != 16000) &&
        (sample_rate != 48000)) {
      ERROR("The sample rate is invalid, only support 8K 16K 48K currently");
      ret = false;
      break;
    }
    data.set_sample_rate(sample_rate);
    if ((pos = arg.find(flag, pos)) != std::string::npos) {
      pos += 1;
      bool enable = (atoi(&(arg[pos])) == 0) ? false : true;
      data.enable(enable);
    } else {
      ERROR("Enable audio codec param is invalid.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool parse_playback_unix_domain_arg(std::string arg,
                                    AMIApiPlaybackUnixDomainUri& data)
{
  bool ret = true;
  do {
    std::string flag = "*";
    std::string::size_type pos = 0;
    std::string audio_type;
    std::string unix_domain_name;
    data.set_playback_id(atoi(&(arg[0])));
    if ((pos = arg.find(flag, pos)) != std::string::npos) {
      pos += 1;
      std::string::size_type old_pos = pos;
      if ((pos = arg.find(flag, pos)) != std::string::npos) {
        unix_domain_name = arg.substr(old_pos, (pos - old_pos));
        if (!data.set_unix_domain_name(unix_domain_name.c_str(),
                                       unix_domain_name.size())) {
          ERROR("Failed to set unix domain name.");
          ret = false;
          break;
        }
      }
    }
    pos += 1;
    std::string::size_type old_pos = pos;
    if ((pos = arg.find(flag, pos)) != std::string::npos) {
      audio_type = arg.substr(old_pos, (pos - old_pos));
      if (audio_type == std::string("aac")) {
        data.set_audio_type_aac();
      } else if (audio_type == std::string("opus")) {
        data.set_audio_type_opus();
      } else if (audio_type == std::string("g711A")) {
        data.set_audio_type_g711A();
      } else if (audio_type == std::string("g711U")) {
        data.set_audio_type_g711U();
      } else if (audio_type == std::string("g726_40")) {
        data.set_audio_type_g726_40();
      } else if (audio_type == std::string("g726_32")) {
        data.set_audio_type_g726_32();
      } else if (audio_type == std::string("g726_24")) {
        data.set_audio_type_g726_24();
      } else if (audio_type == std::string("g726_16")) {
        data.set_audio_type_g726_16();
      } else if (audio_type == std::string("speex")) {
        data.set_audio_type_speex();
      } else {
        ERROR("Audio type invalid");
        ret = false;
        break;
      }
    } else {
      ERROR("Enable audio codec param is invalid.");
      ret = false;
      break;
    }
    pos += 1;
    uint32_t sample_rate = atoi(&(arg[pos]));
    if ((sample_rate != 8000) && (sample_rate != 16000) &&
        (sample_rate != 48000)) {
      ERROR("The sample rate is invalid, only support 8K 16K 48K currently");
      ret = false;
      break;
    }
    data.set_sample_rate(sample_rate);
    if ((pos = arg.find(flag, pos)) != std::string::npos) {
      pos += 1;
      uint32_t channel = atoi(&(arg[pos]));
      data.set_audio_channel(channel);
    } else {
      ERROR("Enable audio codec param is invalid.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool parse_add_file_arg(std::string arg, AMIApiPlaybackAudioFileList& data)
{
  bool ret = true;
  do {
    std::string flag = "*";
    std::string::size_type pos = 0;
    if (!data.set_playback_id(atoi(&(arg[0])))) {
      ERROR("Failed to set playback id");
      ret = false;
      break;
    }
    if ((pos = arg.find(flag, pos)) != std::string::npos) {
      pos += 1;
      std::string file_name = arg.substr(pos, std::string::npos);
      char abs_path[PATH_MAX] = { 0 };
      if ((nullptr != realpath(file_name.c_str(), abs_path))
          && !data.add_file(std::string(abs_path))) {
        ERROR("Failed to add file %s to file list, "
            "file name maybe too long, drop it.");
        ret = false;
      }
    } else {
      ERROR("Input arg is invalid");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

