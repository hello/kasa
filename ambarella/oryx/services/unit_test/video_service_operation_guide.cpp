/*******************************************************************************
 * video_service_operation_guide.cpp
 *
 * History:
 *   Dec 21, 2015 - [Huaiqing Wang] created file
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

/*
 * This test file's demo is to tell custom how to combine more than one air api
 * to implement a specify function. Some apis may operate configure file, and
 * the others may direct dynamic operate device
*/

#include <getopt.h>
#include <signal.h>
#include <math.h>
#include <map>
#include <string>

#include "am_log.h"
#include "am_define.h"
#include "am_api_helper.h"
#include "am_video_types.h"
#include "am_api_video.h"

using std::map;
using std::string;

#define NO_ARG 0
#define HAS_ARG 1

#define VERIFY_ID(x) \
    do { \
      if ((x) < 0) { \
        printf ("wrong id: %d \n", (x)); \
        return -1; \
      } \
    } while (0)

struct setting_option {
    bool is_set;

    union {
        bool     v_bool;
        float    v_float;
        int32_t  v_int;
        uint32_t v_uint;
    } num_val;
    string str_val;
};

struct buffer_cfg_setting {
    bool is_set;
    setting_option type;
    setting_option input_crop;
    setting_option input_width;
    setting_option input_height;
    setting_option input_offset_x;
    setting_option input_offset_y;
    setting_option width;
    setting_option height;
    setting_option prewarp;
};

struct vout_cfg_setting {
    bool is_set;
    setting_option type;
    setting_option mode;
    setting_option video_type;
    setting_option flip;
    setting_option rotate;
    setting_option fps;
};

struct stream_size_setting {
    bool is_set;
    setting_option keep_fov;
    setting_option resolution;
};

static AMAPIHelperPtr g_api_helper = nullptr;

typedef map<AM_VOUT_ID, vout_cfg_setting> vout_setting_map;
typedef map<AM_STREAM_ID, stream_size_setting> stream_size_setting_map;
static vout_setting_map g_vout_setting;
static stream_size_setting_map g_stream_size_setting;

enum numeric_short_options {
  NEMERIC_BEGIN = 'z',

  //vout
  VOUT_SWITCH,
  VOUT_MODE_SET,

  //stream
  STREAM_FOV_KEEP,
  STREAM_RESOLUTION,
};

static struct option long_options[] =
{
 {"help",               NO_ARG,   0,  'h'},

 {"vout",               HAS_ARG,  0,  'V'},
 {"vout-switch",        HAS_ARG,  0,  VOUT_SWITCH},
 {"vout-mode-set",      HAS_ARG,  0,  VOUT_MODE_SET},

 {"stream",             HAS_ARG,  0,  's'},
 {"stream-resolution",  HAS_ARG,  0,  STREAM_RESOLUTION},
 {"stream-fov-keep",    HAS_ARG,  0,  STREAM_FOV_KEEP},

 {0, 0, 0, 0}
};

static const char *short_options = "hVs:";

struct hint32_t_s {
    const char *arg;
    const char *str;
};

static const hint32_t_s hint32_t[] =
{
 {"",     "\t\t\t"  "Show usage\n"},

 {"0-1",  "\t\t"    "VOUT ID. 0: VOUTB; 1:VOUTA"},
 {"0-3",  "\t\t"    "Turn on/off VOUT.0 is off, and the other num_val represent "
                     "VOUT physically type: 1=LCD; 2=HDMI; 3=CVBS; 4=YPbPr"},
 {"",     "\t\t"    "Set VOUT mode to 480i/576i/480p/576p/720p/1080p...\n"},

 {"",     "\t\t\t"  "Stream ID"},
 {"",     "\t\t"    "Stream resolution with format: 'width'x'height'"},
 {"",     "\t\t"    "Whether keep full fov when change stream resolution. 0 is not keep;"
                    " else other value is keep, default value is to keep full fov\n"},
};

static void usage(int32_t argc, char **argv)
{
  printf("\n%s usage:\n\n", argv[0]);
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
  printf("\n");
}

static int32_t init_param(int32_t argc, char **argv)
{
  int32_t ch;
  int32_t option_index = 0;
  int32_t current_vout_id = -1;
  AM_STREAM_ID current_stream_id = AM_STREAM_ID_INVALID;
  int32_t ret = 0;
  opterr = 0;

  while ((ch = getopt_long(argc, argv,
                           short_options,
                           long_options,
                           &option_index)) != -1) {
    switch (ch) {
      case 'h':
        usage(argc, argv);
        return -1;

      case 'V':
        VERIFY_ID(atoi(optarg));
        current_vout_id = atoi(optarg);
        g_vout_setting[AM_VOUT_ID(current_vout_id)].is_set = true;
        break;

      case VOUT_SWITCH:
        VERIFY_ID(current_vout_id);
        g_vout_setting[AM_VOUT_ID(current_vout_id)].type.is_set = true;
        g_vout_setting[AM_VOUT_ID(current_vout_id)].type.num_val.v_int = atoi(optarg);
        PRINTF("id = %d", AM_VOUT_ID(current_vout_id));
        break;

      case VOUT_MODE_SET:
        VERIFY_ID(current_vout_id);
          g_vout_setting[AM_VOUT_ID(current_vout_id)].mode.is_set = true;
          g_vout_setting[AM_VOUT_ID(current_vout_id)].mode.str_val = optarg;
          PRINTF("set mode to %s\n",
                 g_vout_setting[AM_VOUT_ID(current_vout_id)].mode.str_val.c_str());
        break;

      case 's':
        VERIFY_ID(atoi(optarg));
        current_stream_id = AM_STREAM_ID(atoi(optarg));
        g_stream_size_setting[current_stream_id].is_set = true;
        break;

      case STREAM_FOV_KEEP:
        g_stream_size_setting[current_stream_id].keep_fov.is_set = true;
        g_stream_size_setting[current_stream_id].keep_fov.num_val.v_int = atoi(optarg);
        break;

      case STREAM_RESOLUTION:
        g_stream_size_setting[current_stream_id].resolution.is_set = true;
        g_stream_size_setting[current_stream_id].resolution.str_val = optarg;
        break;

      default:
        ret = -1;
        printf("unknown option found: %d\n", ch);
        break;
    }
  }

  return ret;
}

static bool float_equal(float a, float b)
{
  if (fabsf(a - b) < 0.00001) {
    return true;
  } else {
    return false;
  }
}

static int32_t set_buffer_config(int32_t buf_id, const buffer_cfg_setting &conf)
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_buffer_fmt_t buffer_fmt = {0};
  bool has_setting = false;
  do {
    if (!conf.is_set) {
      break;
    }
    buffer_fmt.buffer_id = buf_id;

    if (conf.type.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_TYPE_EN_BIT);
      buffer_fmt.type = conf.type.num_val.v_int;
      has_setting = true;
    }
    if (conf.input_crop.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_CROP_EN_BIT);
      buffer_fmt.input_crop = conf.input_crop.num_val.v_bool;
      has_setting = true;
    }
    if (conf.input_width.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_WIDTH_EN_BIT);
      buffer_fmt.input_width = conf.input_width.num_val.v_int;
      has_setting = true;
    }
    if (conf.input_height.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_HEIGHT_EN_BIT);
      buffer_fmt.input_height = conf.input_height.num_val.v_int;
      has_setting = true;
    }
    if (conf.input_offset_x.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_X_EN_BIT);
      buffer_fmt.input_offset_x = conf.input_offset_x.num_val.v_int;
      has_setting = true;
    }
    if (conf.input_offset_y.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_Y_EN_BIT);
      buffer_fmt.input_offset_y = conf.input_offset_y.num_val.v_int;
      has_setting = true;
    }
    if (conf.width.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_WIDTH_EN_BIT);
      buffer_fmt.width = conf.width.num_val.v_int;
      has_setting = true;
    }
    if (conf.height.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_HEIGHT_EN_BIT);
      buffer_fmt.height = conf.height.num_val.v_int;
      has_setting = true;
    }
    if (conf.prewarp.is_set) {
      SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_PREWARP_EN_BIT);
      buffer_fmt.prewarp = conf.prewarp.num_val.v_bool;
      has_setting = true;
    }

    if (has_setting) {
      INFO("Buffer[%d]", buf_id);
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_SET,
                                &buffer_fmt, sizeof(buffer_fmt),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("failed to set Stream buffer format!\n");
        break;
      }
    }
  } while(0);

  return ret;
}

static int32_t set_vout_config(AM_VOUT_ID id, const vout_cfg_setting &conf)
{
  int32_t ret  = 0;

  do {
    if (!conf.is_set) {
      break;
    }

    bool has_setting = false;
    am_vout_config_t config;
    memset(&config, 0, sizeof(config));
    am_service_result_t service_result;

    if (conf.type.is_set) {
      config.type = conf.type.num_val.v_int;
      SET_BIT(config.enable_bits, AM_VOUT_CONFIG_TYPE_EN_BIT);
      has_setting = true;
    }

    if (conf.video_type.is_set) {
      config.video_type = conf.video_type.num_val.v_int;
      SET_BIT(config.enable_bits, AM_VOUT_CONFIG_VIDEO_TYPE_EN_BIT);
      has_setting = true;
    }

    if (conf.mode.is_set) {
      snprintf(config.mode, VOUT_MAX_CHAR_NUM, "%s", conf.mode.str_val.c_str());
      SET_BIT(config.enable_bits, AM_VOUT_CONFIG_MODE_EN_BIT);
      has_setting = true;
    }

    if (conf.flip.is_set) {
      config.flip = conf.flip.num_val.v_int;
      SET_BIT(config.enable_bits, AM_VOUT_CONFIG_FLIP_EN_BIT);
      has_setting = true;
    }

    if (conf.rotate.is_set) {
      config.rotate = conf.rotate.num_val.v_int;
      SET_BIT(config.enable_bits, AM_VOUT_CONFIG_ROTATE_EN_BIT);
      has_setting = true;
    }

    if (conf.fps.is_set) {
      float fps = conf.fps.num_val.v_float;
      if (float_equal(fps, 29.97)) {
        config.fps = 1000;
      } else if (float_equal(fps, 59.94)) {
        config.fps = 1001;
      } else if (float_equal(fps, 23.976)) {
        config.fps = 1002;
      } else if (float_equal(fps, 12.5)) {
        config.fps = 1003;
      } else if (float_equal(fps, 6.25)) {
        config.fps = 1004;
      } else if (float_equal(fps, 3.125)) {
        config.fps = 1005;
      } else if (float_equal(fps, 7.5)) {
        config.fps = 1006;
      } else if (float_equal(fps, 3.75)) {
        config.fps = 1007;
      } else {
        config.fps = static_cast<int32_t>(fps);
      }
      SET_BIT(config.enable_bits, AM_VOUT_CONFIG_FPS_EN_BIT);
      has_setting = true;
    }
    if (has_setting) {
      config.vout_id = id;
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_VOUT_SET,
                                &config, sizeof(config),
                                &service_result, sizeof(service_result));
      if ((ret = service_result.ret) != 0) {
        ERROR("failed to set VOUT config!\n");
        break;
      }
    }
  } while(0);

  return ret;
}

static int32_t load_all_cfg()
{
  int32_t ret = 0;
  INFO("Load all configures!!!\n");
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_ALL_LOAD, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to load all configs!\n");
  }

  return ret;
}

static int32_t start_encoding()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_START, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to start encoding!\n");
  }
  return ret;
}

static int32_t stop_encoding()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_STOP, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to stop encoding!\n");
  }
  return ret;
}

static int32_t halt_vout(int32_t vout_id)
{
  int32_t ret = 0;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_VOUT_HALT,
                            &vout_id, sizeof(vout_id),
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to stop VOUT device!\n");
  }

  return ret;
}

/* we can use a vout dynamic api to trun off vout, but
 * after finished it, we must call config api to store
 * the vout and source buffer state in configure file
 * */
static int32_t vout_stop(AM_VOUT_ID vout_id, const vout_cfg_setting &conf)
{
  int32_t ret = 0;
  do {
    buffer_cfg_setting  buffer_conf = {0};

    //call dynamic api to halt vout
    if ((ret=halt_vout(vout_id)) != 0) {
      break;
    }

    //set vout type to "off" to configure file
    if ((ret=set_vout_config(vout_id, conf)) != 0) {
      break;
    }

    /* because relate source buffer also affect vout, so must
     * set relate source buffer type from "preview" to "off".
     * VOUT B use source third buffer, id 0 represent VOUT B;
     * VOUT A use fourth buffer, id 1 represent VOUT A.
     * */
    buffer_conf.is_set = true;
    buffer_conf.type.is_set = true;
    buffer_conf.type.num_val.v_int = 0;
    if ((ret=set_buffer_config(vout_id + 2, buffer_conf)) != 0) {
      break;
    }
  } while(0);

  return ret;
}

/* There is no vout dynamic api to trun on vout, because it must
 * goto idle first, then enter preview state. So we can use encode
 * stop and start dynamic api and another config apis to finish it.
 * note: restart vout will impact encode stream
 * */
static int32_t vout_start(AM_VOUT_ID vout_id, const vout_cfg_setting &conf)
{
  int32_t ret = 0;
  do {
    buffer_cfg_setting  buffer_conf = {0};

    //set vout type to a real type to configure file
    if ((ret=set_vout_config(vout_id, conf)) != 0) {
      break;
    }

    //set buffer type to "preview" to configure file
    buffer_conf.is_set = true;
    buffer_conf.type.is_set = true;
    buffer_conf.type.num_val.v_int = 2;
    if ((ret=set_buffer_config(vout_id + 2, buffer_conf)) != 0) {
      break;
    }

    //stop encode enter idle state
    if ((ret = stop_encoding()) < 0) {
      break;
    }

    //reload all config
    if ((ret = load_all_cfg()) < 0) {
      break;
    }

    //start encode, vout will enter preview state too
    if ((ret = start_encoding()) < 0) {
      break;
    }
  } while(0);

  return ret;
}

/* There is no vout dynamic api to change vout mode, because it must
 * goto idle first, then enter preview state. So we can use encode
 * stop and start dynamic api and another config apis to finish it.
 * note: set vout mode will impact encode stream
 * */
static int32_t vout_set_mode(AM_VOUT_ID vout_id, const vout_cfg_setting &conf)
{
  int32_t ret = 0;
  do {
    buffer_cfg_setting  buffer_conf = {0};

    //set vout mode to configure file
    if ((ret=set_vout_config(vout_id, conf)) != 0) {
      break;
    }

    //stop encode enter idle state
    if ((ret = stop_encoding()) < 0) {
      break;
    }

    //reload all config
    if ((ret = load_all_cfg()) < 0) {
      break;
    }

    //start encode, vout will enter preview state too
    if ((ret = start_encoding()) < 0) {
      break;
    }
  } while(0);

  return ret;
}

static int32_t vout_operation()
{
  int32_t ret = 0;

  for (auto &m : g_vout_setting) {
    if (m.second.is_set != true) {
      continue;
    }

    //turn on/off vout
    if (m.second.type.is_set) {
      if (m.second.type.num_val.v_int == 0) {
        if ((ret=vout_stop(m.first, m.second)) != 0) {
          break;
        }
        PRINTF("stop vout%d success\n",m.first);
      } else {
        if ((ret=vout_start(m.first, m.second)) != 0) {
          break;
        }
        PRINTF("start vout%d success\n",m.first);
      }
    }

    //change vout mode
    if (m.second.mode.is_set) {
      if ((ret=vout_set_mode(m.first, m.second)) != 0) {
        break;
      }
      PRINTF("set vout%d mode success\n",m.first);

    }
  }
  g_vout_setting.clear();

  return ret;
}

static int32_t get_stream_number_max()
{
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_MAX_NUM_GET,
                            nullptr, 0,
                            &service_result, sizeof(service_result));
  if (service_result.ret != 0) {
    ERROR("failed to get platform max stream number!\n");
    return -1;
  }
  uint32_t *val = (uint32_t*)service_result.data;

  return *val;
}

static int32_t start_streaming(int32_t id)
{
  int32_t ret = 0;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_START,
                            &id, sizeof(int32_t),
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to resume stream%d!\n",id);
  }
  return ret;
}

static int32_t stop_streaming(int32_t id)
{
  int32_t ret = 0;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STOP,
                            &id, sizeof(int32_t),
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to pause stream%d!\n",id);
  }
  return ret;
}

static int32_t get_source_buffer_format(int32_t id, am_buffer_fmt_t &buffer_fmt)
{
  int32_t ret = 0;
  am_service_result_t service_result = {0};
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_GET,
                            &id, sizeof(id),
                            &service_result, sizeof(service_result));
  if ((ret=service_result.ret) == 0) {
    memcpy(&buffer_fmt, service_result.data, sizeof(buffer_fmt));
  } else {
    ERROR("Failed to get buffer[%d] format!", id);
  }
  return ret;
}

static int32_t set_source_buffer_format(am_buffer_fmt_t &buffer_fmt)
{
  int32_t ret = 0;
  am_service_result_t service_result = {0};

  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_SET,
                            &buffer_fmt, sizeof(buffer_fmt),
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("Failed to set buffer format!");
  }

  return ret;
}

static int32_t stream_resolution_operation_with_fov()
{
  int32_t ret = 0;
  int32_t stream_num_max = 0;
  am_service_result_t service_result;
  am_stream_parameter_t params = {0};

  if (g_stream_size_setting.empty()) {
    return ret;
  }

  stream_num_max = get_stream_number_max();
  for (auto &m : g_stream_size_setting) {
    if (m.second.is_set != true) {
      continue;
    }
    if (m.first >= stream_num_max) {
      PRINTF("Invalid stream id: %d\n",m.first);
      break;
    }

    if (m.second.resolution.is_set) {
      int32_t stream_x = -1, stream_y = -1;
      params.stream_id = m.first;
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_SIZE_EN_BIT);
      sscanf(m.second.resolution.str_val.c_str(), "%dx%d",
             &(params.width), &(params.height));
      params.width = ROUND_UP(params.width, 16);
      params.height = ROUND_UP(params.height, 8);

      //get the relevant source buffer id for the specify stream id
      am_stream_fmt_t fmt = {0};
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_GET,
                                &(params.stream_id), sizeof(params.stream_id),
                                &service_result, sizeof(service_result));
      memcpy(&fmt, service_result.data, sizeof(fmt));

      //the buffer can't be main buffer, because it will affect all other sub buffer
      if (fmt.source == AM_SOURCE_BUFFER_MAIN) {
        PRINTF("Stream%d use main source buffer. "
            "Don't change main buffer's size\n", params.stream_id);
        continue;
      }

      //stop stream first
      if ((ret=stop_streaming(m.first)) != 0) {
        break;
      }

      //check the source buffer if busy, busy represent it also used by other streams now
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_STATE_GET,
                                &fmt.source, sizeof(fmt.source),
                                &service_result, sizeof(service_result));
      if (service_result.ret != 0) {
        ERROR("Failed to get buffer[%d] format!", fmt.source);
        break;
      }
      am_buffer_state_t buffer_state = {0};
      memcpy(&buffer_state, service_result.data, sizeof(buffer_state));
      if (buffer_state.state == 1) {
        ERROR("Stream%d use source buffer %d, but the source buffer also used by "
            "other streams now, please disable them first!",
            params.stream_id, fmt.source);
        //restart the stream stopped before
        if ((ret=start_streaming(m.first)) != 0) {
          break;
        }
        continue;
      }

      am_buffer_fmt_t buffer = {0};
      if ((ret=get_source_buffer_format(fmt.source, buffer)) != 0) {
        break;
      }

      uint32_t buffer_w, buffer_h;
      uint32_t buffer_input_w = buffer.input_width;
      uint32_t buffer_input_h = buffer.input_height;
      if (!m.second.keep_fov.is_set || m.second.keep_fov.num_val.v_int == 1) {
        buffer_w = params.width;
        buffer_h = params.height;
        stream_x = 0;
        stream_y = 0;
      } else {
        uint32_t source_buf_input_gcd = get_gcd(buffer_input_w, buffer_input_h);
        uint32_t stream_size_gcd = get_gcd(params.width, params.height);

        if (((buffer_input_w / source_buf_input_gcd) ==
            (params.width / stream_size_gcd)) &&
            ((buffer_input_h / source_buf_input_gcd) ==
                (params.height / stream_size_gcd))) {
          buffer_w = params.width;
          buffer_h = params.height;
          stream_x = 0;
          stream_y = 0;
        } else if ((float(buffer_input_w) / buffer_input_h) >
        (float(params.width) / params.height)) {
          //crop stream horizontal content
          buffer_h = params.height;
          buffer_w = (buffer_h * buffer_input_w) / buffer_input_h;
          buffer_w = ROUND_UP(buffer_w, 16);
          stream_x = (buffer_w - params.width) / 2;
        } else {
          //crop stream vertical content
          buffer_w = params.width;
          buffer_h = (buffer_w * buffer_input_h) / buffer_input_w;
          buffer_w = ROUND_UP(buffer_h, 8);
          stream_y = (buffer_h - params.height) / 2;
        }
      }

      buffer.buffer_id = fmt.source;
      SET_BIT(buffer.enable_bits, AM_BUFFER_FMT_WIDTH_EN_BIT);
      buffer.width = buffer_w;
      SET_BIT(buffer.enable_bits, AM_BUFFER_FMT_HEIGHT_EN_BIT);
      buffer.height = buffer_h;
      if ((ret=set_source_buffer_format(buffer)) != 0) {
        break;
      }

      if (stream_x >= 0) {
        SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_OFFSET_EN_BIT);
        params.offset_x = ROUND_DOWN(stream_x, 2);
      }
      if (stream_y >= 0) {
        SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_OFFSET_EN_BIT);
        params.offset_y = ROUND_DOWN(stream_y, 2);
      }
      printf("stream size: %d x %d, offset: %d x %d\n",
             params.width, params.height,params.offset_x, params.offset_y);
      //set stream resolution parameter to device
      g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_SET,
                                &params, sizeof(params),
                                &service_result, sizeof(service_result));
      if ((ret=service_result.ret) != 0) {
        ERROR("failed to set stream size!\n");
        break;
      }

      //start stream to apply stream resolution parameter
      if ((ret=start_streaming(m.first)) != 0) {
        break;
      }
    }

  }
  g_stream_size_setting.clear();

  return ret;
}

int32_t main(int32_t argc, char **argv)
{
  if (argc < 2) {
    usage(argc, argv);
    return -1;
  }

  if (init_param(argc, argv) < 0) {
    return -1;
  }

  int32_t ret = 0;
  g_api_helper = AMAPIHelper::get_instance();
  if (!g_api_helper) {
    ERROR("unable to get AMAPIHelper instance\n");
    return -1;
  }

  do {
    if ((ret=vout_operation()) <0) {
      break;
    }
    if ((ret=stream_resolution_operation_with_fov()) <0) {
      break;
    }

  } while (0);

  return ret;
}
