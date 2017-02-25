/*******************************************************************************
 * test_media_service_dyn_air_api.cpp
 *
 * History:
 *   2016-12-5 - [ccjing] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#include "am_log.h"
#include "am_define.h"

#include <getopt.h>
#include <signal.h>
#include <math.h>
#include <map>
#include <vector>
#include <string>
#include "am_api_helper.h"
#include "am_api_media.h"


using std::map;
using std::string;
using std::vector;

#define NO_ARG 0
#define HAS_ARG 1

#define VERIFY_PARA_1(x, min) \
    do { \
      if ((x) < min) { \
        printf("Parameter wrong: %d\n", (x)); \
        return -1; \
      } \
    } while (0)

#define VERIFY_PARA_2(x, min, max) \
    do { \
      if (((x) < min) || ((x) > max)) { \
        printf("Parameter wrong: %d\n", (x)); \
        return -1; \
      } \
    } while (0)

static AMAPIHelperPtr g_api_helper = nullptr;
static bool set_muxer_param_flag   = false;
static bool exit_flag = false;

enum short_opt {
  SET_FILE_DURATION  = 1,
  SET_RECORDING_FILE_NUM,
  SET_RECORDING_DURATION,
  ENABLE_DIGITAL_SIGNATURE,
  ENABLE_GSENSOR,
  ENABLE_RECONSTRUCT,
  SET_MAX_FILE_SIZE,
  ENABLE_WRITE_SYNC,
  ENABLE_HLS,
  ENABLE_SCRAMBLE,
  SET_VIDEO_FRAME_RATE,
  SAVE_TO_CONFIG_FILE,
};

static void sigstop(int arg)
{
  PRINTF("catch sigstop, exit.");
  exit_flag = true;
}

static struct option long_options[] =
{
 {"help",                     NO_ARG,  0, 'h'},
 /*set file muxer parameter*/
 {"muxer-id",                 HAS_ARG, 0, 'm'},
 {"set-file-duration",        HAS_ARG, 0, SET_FILE_DURATION},
 {"set-recording-file-num",   HAS_ARG, 0, SET_RECORDING_FILE_NUM},
 {"set-recording-duration",   HAS_ARG, 0, SET_RECORDING_DURATION},
 {"enable-digital-signature", HAS_ARG, 0, ENABLE_DIGITAL_SIGNATURE},
 {"enable-gsensor",           HAS_ARG, 0, ENABLE_GSENSOR},
 {"enable-reconstruct",       HAS_ARG, 0, ENABLE_RECONSTRUCT},
 {"set-max-file-size",        HAS_ARG, 0, SET_MAX_FILE_SIZE},
 {"set-video-frame-rate",     HAS_ARG, 0, SET_VIDEO_FRAME_RATE},
 {"enable-write-sync",        HAS_ARG, 0, ENABLE_WRITE_SYNC},
 {"enable-hls",               HAS_ARG, 0, ENABLE_HLS},
 {"enable-scramble",          HAS_ARG, 0, ENABLE_SCRAMBLE},
 {"save-to-config-file",      NO_ARG,  0, SAVE_TO_CONFIG_FILE},
 {0,0,0,0}
};

static const char *short_options = "hm:";

struct hint32_t_s {
    const char *arg;
    const char *str;
};

static const hint32_t_s hint32_t[] =
{
 {"",              "\t\t\t\t" "All parameters take effect from next file\n"},

 {"0-18",          "\t\t\t"   "Set muxer id"},
 {"0-0x7fffffff",  "\t"       "Set file duration, 0:not split file automatically"},
 {"1-0x7fffffff",  ""         "Set recording file num, should not set it when file is writing"},
 {"1-0x7fffffff",  ""         "Set recording duration, should not set it when file is writing"},
 {"0|1",           "\t"       "0:disable, 1:enable, for av3 only"},
 {"0|1",           "\t\t"     "0:disable, 1:enable"},
 {"0|1",           "\t\t"     "0:disable, 1:enable"},
 {"1-2048",        "\t"       "unit : MB"},
 {"1-90000",       "\t"       "Set video frame rate, only time-elapse-mp4 support this param currently"},
 {"0|1",           "\t\t"     "0:disable, 1:enable"},
 {"0|1",           "\t\t\t"   "0:disable, 1:enable"},
 {"0|1",           "\t\t"     "0:disable, 1:enable, for av3 only"},
 {"",              "\t\t"     "Save to config file"}
};

static string enum_muxer_id_to_string(AM_MEDIA_MUXER_ID id)
{
  string muxer_str;
  switch (id) {
    case AM_MEDIA_MUXER_MP4_NORMAL_0 : {
      muxer_str = "   mp4-normal-0";
    } break;
    case AM_MEDIA_MUXER_MP4_NORMAL_1 : {
      muxer_str = "   mp4-normal-1";
    } break;
    case AM_MEDIA_MUXER_MP4_EVENT_0 : {
      muxer_str = "    mp4-event-0";
    } break;
    case AM_MEDIA_MUXER_MP4_EVENT_1 : {
      muxer_str = "    mp4-event-1";
    } break;
    case AM_MEDIA_MUXER_TS_NORMAL_0 : {
      muxer_str = "    ts-normal-0";
    } break;
    case AM_MEDIA_MUXER_TS_NORMAL_1 : {
      muxer_str = "    ts-normal-1";
    } break;
    case AM_MEDIA_MUXER_TS_EVENT_0 : {
      muxer_str = "     ts-event-0";
    } break;
    case AM_MEDIA_MUXER_TS_EVENT_1 : {
      muxer_str = "     ts-event-1";
    } break;
    case AM_MEDIA_MUXER_RTP : {
      muxer_str = "            rtp";
    } break;
    case AM_MEDIA_MUXER_JPEG_NORMAL : {
      muxer_str = "    jpeg-normal";
    } break;
    case AM_MEDIA_MUXER_JPEG_EVENT_0 : {
      muxer_str = "   jpeg-event-0";
    } break;
    case AM_MEDIA_MUXER_JPEG_EVENT_1 : {
      muxer_str = "   jpeg-event-1";
    } break;
    case AM_MEDIA_MUXER_PERIODIC_JPEG : {
      muxer_str = "  periodic-jpeg";
    } break;
    case AM_MEDIA_MUXER_TIME_ELAPSE_MP4 : {
      muxer_str = "time-elapse-mp4";
    } break;
    case AM_MEDIA_MUXER_AV3_NORMAL_0 : {
      muxer_str = "   av3-normal-0";
    } break;
    case AM_MEDIA_MUXER_AV3_NORMAL_1 : {
      muxer_str = "   av3-normal-1";
    } break;
    case AM_MEDIA_MUXER_AV3_EVENT_0 : {
      muxer_str = "    av3-event-0";
    } break;
    case AM_MEDIA_MUXER_AV3_EVENT_1 : {
      muxer_str = "    av3-event-1";
    } break;
    case AM_MEDIA_MUXER_EXPORT : {
      muxer_str = "         export";
    } break;
    default : {
      muxer_str = "Unkonwn muxer";
    } break;
  }
  return muxer_str;
}

static void usage(int32_t argc, char **argv)
{
  printf("\n%s usage:\n", argv[0]);
  printf(" muxer id talbe : \n");
  for (int32_t i = 0; i < AM_MEDIA_MUXER_ID_MAX; ++ i) {
    printf("%s : %d\n", enum_muxer_id_to_string(AM_MEDIA_MUXER_ID(i)).c_str(), i);
  }
  for (uint32_t i = 0; i < sizeof(long_options)/sizeof(long_options[0])-1; ++i) {
    if (isalpha(long_options[i].val)) {
      printf("-%c, ", long_options[i].val);
    } else {
      printf("    ");
    }
    printf("--%s", long_options[i].name);
    if (hint32_t[i].arg[0] != 0) {
      printf(" [%s]", hint32_t[i].arg);
    }
    printf("\t%s\n", hint32_t[i].str);
  }
  printf("Examples: \n"
         "  %s -m 14 --set-file-duration 10 --enable-digital-signature 1\n"
         "  %s -m 13 -m 14 --set-file-duration 10 --enable-digital-signature 0\n",
         argv[0], argv[0]);
  printf("\n");
}

static int32_t init_param(int32_t argc, char **argv, AMMediaMuxerParam& param)
{
  int32_t ch;
  int32_t option_index = 0;
  int32_t ret = 0;
  memset(&param, 0, sizeof(AMMediaMuxerParam));
  while((ch = getopt_long(argc, argv,
                          short_options,
                          long_options,
                          &option_index)) != -1) {
    switch(ch) {
      case 'h' : {
        usage(argc, argv);
      } break;
      case 'm' : {
        int32_t muxer_id = atoi(optarg);
        VERIFY_PARA_2(muxer_id, AM_MEDIA_MUXER_MP4_NORMAL_0,
                      AM_MEDIA_MUXER_ID_MAX - 1);
        param.muxer_id_bit_map |= 0x01 << muxer_id;
        set_muxer_param_flag = true;
      } break;
      case SET_FILE_DURATION : {
          int32_t file_duration = atoi(optarg);
          VERIFY_PARA_2(file_duration, 0, 0x7fffffff);
          param.file_duration_int32.is_set = true;
          param.file_duration_int32.value.v_int32 = file_duration;
          set_muxer_param_flag = true;
      } break;
      case SET_RECORDING_FILE_NUM : {
        int32_t file_num = atoi(optarg);
        VERIFY_PARA_2(file_num, 0, 0x7fffffff);
        param.recording_file_num_u32.is_set = true;
        param.recording_file_num_u32.value.v_u32 = (uint32_t)file_num;
        set_muxer_param_flag = true;
      } break;
      case SET_RECORDING_DURATION : {
        int32_t duration = atoi(optarg);
        VERIFY_PARA_2(duration, 0, 0x7fffffff);
        param.recording_duration_u32.is_set = true;
        param.recording_duration_u32.value.v_u32 = (uint32_t)duration;
        set_muxer_param_flag = true;
      } break;
      case ENABLE_DIGITAL_SIGNATURE : {
        int32_t digital_sig = atoi(optarg);
        VERIFY_PARA_2(digital_sig, 0, 1);
        param.digital_sig_enable_bool.is_set = true;
        param.digital_sig_enable_bool.value.v_bool =
            (digital_sig == 0) ? false : true;
        set_muxer_param_flag = true;
      } break;
      case ENABLE_GSENSOR : {
        int32_t enable_gsensor = atoi(optarg);
        VERIFY_PARA_2(enable_gsensor, 0, 1);
        param.gsensor_enable_bool.is_set = true;
        param.gsensor_enable_bool.value.v_bool =
            (enable_gsensor == 0) ? false : true;
        set_muxer_param_flag = true;
      } break;
      case ENABLE_RECONSTRUCT : {
        int32_t enable_reconstruct = atoi(optarg);
        VERIFY_PARA_2(enable_reconstruct, 0, 1);
        param.reconstruct_enable_bool.is_set = true;
        param.reconstruct_enable_bool.value.v_bool =
            (enable_reconstruct == 0) ? false : true;
        set_muxer_param_flag = true;
      } break;
      case SET_MAX_FILE_SIZE : {
        int32_t max_file_size = atoi(optarg);
        VERIFY_PARA_2(max_file_size, 0, 0x7fffffff);
        param.max_file_size_u32.is_set = true;
        param.max_file_size_u32.value.v_u32 = (uint32_t)max_file_size;
        set_muxer_param_flag = true;
      } break;
      case SET_VIDEO_FRAME_RATE : {
        uint32_t video_frame_rate = atoi(optarg);
        VERIFY_PARA_2(video_frame_rate, 1, 90000);
        param.video_frame_rate_u32.is_set = true;
        param.video_frame_rate_u32.value.v_u32 = video_frame_rate;
        set_muxer_param_flag = true;
      }  break;
      case ENABLE_WRITE_SYNC : {
        int32_t enable_sync = atoi(optarg);
        VERIFY_PARA_2(enable_sync, 0, 1);
        param.write_sync_enable_bool.is_set = true;
        param.write_sync_enable_bool.value.v_bool =
            (enable_sync == 0) ? false : true;
        set_muxer_param_flag = true;
      } break;
      case ENABLE_HLS : {
        int32_t hls = atoi(optarg);
        VERIFY_PARA_2(hls, 0, 1);
        param.hls_enable_bool.is_set = true;
        param.hls_enable_bool.value.v_bool =
            (hls == 0) ? false : true;
        set_muxer_param_flag = true;
      } break;
      case ENABLE_SCRAMBLE : {
        int32_t scramble = atoi(optarg);
        VERIFY_PARA_2(scramble, 0, 1);
        param.scramble_enable_bool.is_set = true;
        param.scramble_enable_bool.value.v_bool =
            (scramble == 0) ? false : true;
        set_muxer_param_flag = true;
      } break;
      case SAVE_TO_CONFIG_FILE : {
        param.save_to_config_file = true;
      } break;
      default : {
        ERROR("Invalid short option");
        ret = -1;
        break;
      }
    }
  }
  return ret;
}

static int32_t set_muxer_param(AMMediaMuxerParam& param)
{
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_SET_MUXER_PARAM,
                            &param, sizeof(AMMediaMuxerParam),
                            &service_result, sizeof(service_result));
  if (service_result.ret != AM_RESULT_OK) {
    ERROR("failed to get platform max stream number!\n");
    return -1;
  }
  return service_result.ret;
}

int32_t main(int32_t argc, char **argv)
{
  int32_t ret = 0;
  AMMediaMuxerParam param;
  do {
    if (argc < 2) {
      usage(argc, argv);
      return -1;
    }
    signal(SIGINT, sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);
    if (init_param(argc, argv, param) < 0) {
      ERROR("Failed to init param.");
      ret = -1;
      break;
    }
    g_api_helper = AMAPIHelper::get_instance();
    if (!g_api_helper) {
      ERROR("unable to get AMAPIHelper instance\n");
      ret = -1;
      break;
    }
    if (set_muxer_param_flag) {
      if (set_muxer_param(param) < 0) {
        ERROR("Failed to set muxer param");
        ret = -1;
        break;
      }
    }
  } while(0);
  return ret;
}
