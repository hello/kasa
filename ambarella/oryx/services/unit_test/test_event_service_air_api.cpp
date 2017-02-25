/**
 * test_event_service.cpp
 *
 *  History:
 *    Nov 11, 2014 - [Dongge Wu] created file
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

#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <signal.h>

#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"

#include "am_api_helper.h"
#include "am_api_event.h"

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
static bool show_audio_alert_flag = false;
static bool show_key_input_flag = false;
static bool show_all_info_flag = false;

static int stop_am_event_module_id = -1;
static int run_am_event_module_id = -1;
static int destroy_am_event_module_id = -1;
static int regsiter_am_event_module_id = -1;
static int show_regsiter_am_event_module_id = -1;

struct setting_option
{
    union
    {
        bool v_bool;
        float v_float;
        int32_t v_int;
    } value;
    bool is_set;
};

struct audio_alert_setting
{
    bool is_set;
    setting_option channel_num;
    setting_option sample_rate;
    setting_option chunk_bytes;
    setting_option sample_format;
    setting_option enable;
    setting_option sensitivity;
    setting_option threshold;
    setting_option direction;
};

struct key_input_setting
{
    bool is_set;
    setting_option key_press_time;
};

static audio_alert_setting g_audio_alert_cfg;
static key_input_setting g_key_input_cfg;
static char *g_module_path = nullptr;

enum numeric_short_options
{
  NONE = 0,
  REGISTER_MODULE,
  SHOW_REGISTER_STATE,

  AA_CH_NUM,
  AA_SAMPLE_RATE,
  AA_CHUNK_BYTES,
  AA_SAMPLE_FORMAT,
  AA_ENABLE,
  AA_SENSITIVITY,
  AA_THRESHOLD,
  AA_DIRECTION,

  KI_KP_TIME,

  SHOW_AA_INFO,
  SHOW_KI_INFO,
  SHOW_ALL_INFO
};

static struct option long_options[] =
{
{"help", NO_ARG, 0, 'h'},
{"run", HAS_ARG, 0, 'r'},
{"stop", HAS_ARG, 0, 's'},
{"destory", HAS_ARG, 0, 'd'},
{"register", HAS_ARG, 0, REGISTER_MODULE},
{"file", HAS_ARG, 0, 'f'},
{"show-register-state", HAS_ARG, 0, SHOW_REGISTER_STATE},

{"aa-ch-num", HAS_ARG, 0, AA_CH_NUM},
{"aa-sample-rate", HAS_ARG, 0, AA_SAMPLE_RATE},
{"aa-chunk-bytes", HAS_ARG, 0, AA_CHUNK_BYTES},
{"aa-sample-format", HAS_ARG, 0, AA_SAMPLE_FORMAT},
{"aa-enable", HAS_ARG, 0, AA_ENABLE},
{"aa-sensitivity", HAS_ARG, 0, AA_SENSITIVITY},
{"aa-threshold", HAS_ARG, 0, AA_THRESHOLD},
{"aa-direction", HAS_ARG, 0, AA_DIRECTION},

{"ki-lp-time", HAS_ARG, 0, KI_KP_TIME},

{"show-aa-info", NO_ARG, 0, SHOW_AA_INFO},
{"show-ki-info", NO_ARG, 0, SHOW_KI_INFO},
{"show-all-info", NO_ARG, 0, SHOW_ALL_INFO},

{0, 0, 0, 0}};

const char *event_type_str[ALL_MODULE_NUM] =
{"Audio Alert", "Audio Analysis", "Face Detect", "Key Input", };

static const char *short_options = "hr:s:d:f:";

struct hint32_t_s
{
    const char *arg;
    const char *str;
};

static const hint32_t_s hint32_t[] =
{
{"", "\t\t\t" "Show usage\n"},
{"0~5", "\t\t\t" "run event module, 0:Audio Alert,1:Audio Analysis,2:Face Detect,3:Key Input,4:All event modules\n"},
{"0~5", "\t\t" "stop event module, 0:Audio Alert,1:Audio Analysis,2:Face Detect,3:Key Input,4:All event modules\n"},
{"0~5", "\t\t" "destroy event module, 0:Audio Alert,1:Audio Analysis,2:Face Detect,3:Key Input,4:All event modules\n"},
{"0~4", "\t\t" "register event module, 0:Audio Alert,1:Audio Analysis,2:Face Detect,3:Key Input\n"},
{"", "\t\t\t" "register event module file path\n"},
{"0~5", "\t" "show register event module state, 0:Audio Alert,1:Audio Analysis,2:Face Detect,3:Key Input,4:All event modules\n"},

{"1|2", "\t" "audio channel number\n"},
{"8000|16000|24000|36000|40000|48000", "\t" "audio sample rate\n"},
{"", "\t\t" "chunk bytes\n"},
{"refer to snd_pcm_format_t pcm.h", "\t" "sample format\n"},
{"0|1", "\t\t" "disable or enable audio alert\n"},
{"", "\t\t" "audio alert sensitivity\n"},
{"", "\t\t" "audio alert threshold\n"},
{"0|1", "\t" "audio alert direction\n"},

{"", "\t\t" "key input pressed time\n"},

{"", "\t\t" "show audio alert config\n"},
{"", "\t\t" "show key input config\n"},
{"", "\t\t" "show all plugin config\n"}, };

static void usage(int32_t argc, char **argv)
{
  printf("\n%s usage:\n\n", argv[0]);
  for (uint32_t i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1;
      ++ i) {
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

  while ((ch = getopt_long(argc,
                           argv,
                           short_options,
                           long_options,
                           &option_index)) != -1) {
    switch (ch) {
      case 'h':
        usage(argc, argv);
        return -1;
      case 'r':
        run_am_event_module_id = atoi(optarg);
        break;
      case 's':
        stop_am_event_module_id = atoi(optarg);
        break;
      case 'd':
        destroy_am_event_module_id = atoi(optarg);
        break;
      case REGISTER_MODULE:
        regsiter_am_event_module_id = atoi(optarg);
        break;
      case 'f':
        g_module_path = optarg;
        break;
      case SHOW_REGISTER_STATE:
        show_regsiter_am_event_module_id = atoi(optarg);
        break;

      case AA_CH_NUM:
        VERIFY_PARA_2(atoi(optarg), 1, 2);
        g_audio_alert_cfg.channel_num.value.v_int = atoi(optarg);
        g_audio_alert_cfg.channel_num.is_set = true;
        break;
      case AA_SAMPLE_RATE:
        g_audio_alert_cfg.sample_rate.value.v_int = atoi(optarg);
        g_audio_alert_cfg.sample_rate.is_set = true;
        break;
      case AA_CHUNK_BYTES:
        g_audio_alert_cfg.chunk_bytes.value.v_int = atoi(optarg);
        g_audio_alert_cfg.chunk_bytes.is_set = true;
        break;
      case AA_SAMPLE_FORMAT:
        g_audio_alert_cfg.sample_format.value.v_int = atoi(optarg);
        g_audio_alert_cfg.sample_format.is_set = true;
        break;
      case AA_ENABLE:
        VERIFY_PARA_2(atoi(optarg), 0, 1);
        g_audio_alert_cfg.enable.value.v_int = atoi(optarg);
        g_audio_alert_cfg.enable.is_set = true;
        break;
      case AA_SENSITIVITY:
        g_audio_alert_cfg.sensitivity.value.v_int = atoi(optarg);
        g_audio_alert_cfg.sensitivity.is_set = true;
        break;
      case AA_THRESHOLD:
        g_audio_alert_cfg.threshold.value.v_int = atoi(optarg);
        g_audio_alert_cfg.threshold.is_set = true;
        break;
      case AA_DIRECTION:
        g_audio_alert_cfg.direction.value.v_int = atoi(optarg);
        g_audio_alert_cfg.direction.is_set = true;
        break;

      case KI_KP_TIME:
        g_key_input_cfg.key_press_time.value.v_int = atoi(optarg);
        g_key_input_cfg.key_press_time.is_set = true;
        break;

      case SHOW_AA_INFO:
        show_audio_alert_flag = true;
        break;
      case SHOW_KI_INFO:
        show_key_input_flag = true;
        break;
      case SHOW_ALL_INFO:
        show_all_info_flag = true;
        break;

      default:
        printf("unknown option found: %d\n", ch);
        break;
    }
  }

  return 0;
}

static void sigstop(int arg)
{
  INFO("test event service got signal!\n");
}

static int32_t set_audio_alert_config()
{
  int32_t ret = 0;
  am_service_result_t service_result =
  {0};
  am_event_audio_alert_config_s aa_cfg =
  {0};
  bool has_setting = false;

  if (g_audio_alert_cfg.channel_num.is_set) {
    SET_BIT(aa_cfg.enable_bits, AM_EVENT_AUDIO_ALERT_CONFIG_CHANNEL_NUM);
    aa_cfg.channel_num = g_audio_alert_cfg.channel_num.value.v_int;
    has_setting = true;
  }

  if (g_audio_alert_cfg.sample_rate.is_set) {
    SET_BIT(aa_cfg.enable_bits, AM_EVENT_AUDIO_ALERT_CONFIG_SAMPLE_RATE);
    aa_cfg.sample_rate = g_audio_alert_cfg.sample_rate.value.v_int;
    has_setting = true;
  }

  if (g_audio_alert_cfg.chunk_bytes.is_set) {
    SET_BIT(aa_cfg.enable_bits, AM_EVENT_AUDIO_ALERT_CONFIG_CHUNK_BYTES);
    aa_cfg.chunk_bytes = g_audio_alert_cfg.chunk_bytes.value.v_int;
    has_setting = true;
  }

  if (g_audio_alert_cfg.sample_format.is_set) {
    SET_BIT(aa_cfg.enable_bits, AM_EVENT_AUDIO_ALERT_CONFIG_SAMPLE_FORMAT);
    aa_cfg.sample_format = g_audio_alert_cfg.sample_format.value.v_int;
    has_setting = true;
  }

  if (g_audio_alert_cfg.enable.is_set) {
    SET_BIT(aa_cfg.enable_bits, AM_EVENT_AUDIO_ALERT_CONFIG_ENABLE);
    aa_cfg.enable = g_audio_alert_cfg.enable.value.v_bool;
    has_setting = true;
  }

  if (g_audio_alert_cfg.sensitivity.is_set) {
    SET_BIT(aa_cfg.enable_bits, AM_EVENT_AUDIO_ALERT_CONFIG_SENSITIVITY);
    aa_cfg.sensitivity = g_audio_alert_cfg.sensitivity.value.v_int;
    has_setting = true;
  }

  if (g_audio_alert_cfg.threshold.is_set) {
    SET_BIT(aa_cfg.enable_bits, AM_EVENT_AUDIO_ALERT_CONFIG_THRESHOLD);
    aa_cfg.threshold = g_audio_alert_cfg.threshold.value.v_int;
    has_setting = true;
  }

  if (g_audio_alert_cfg.direction.is_set) {
    SET_BIT(aa_cfg.enable_bits, AM_EVENT_AUDIO_ALERT_CONFIG_DIRECTION);
    aa_cfg.direction = g_audio_alert_cfg.direction.value.v_int;
    has_setting = true;
  }

  if (has_setting) {
    g_api_helper->method_call(AM_IPC_MW_CMD_EVENT_AUDIO_ALERT_CONFIG_SET,
                              &aa_cfg,
                              sizeof(aa_cfg),
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret != 0)) {
      ERROR("failed to set audio alert config!\n");
    }
  }

  return ret;
}

static int32_t set_key_input_config()
{
  int32_t ret = 0;
  am_service_result_t service_result =
  {0};
  am_event_key_input_config_s ki_cfg =
  {0};
  bool has_setting = false;

  if (g_key_input_cfg.key_press_time.is_set) {
    SET_BIT(ki_cfg.enable_bits, AM_EVENT_KEY_INPUT_CONFIG_LPT);
    ki_cfg.long_press_time = g_key_input_cfg.key_press_time.value.v_int;
    has_setting = true;
  }

  if (has_setting) {
    g_api_helper->method_call(AM_IPC_MW_CMD_EVENT_KEY_INPUT_CONFIG_SET,
                              &ki_cfg,
                              sizeof(ki_cfg),
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret != 0)) {
      ERROR("failed to set key input config!\n");
    }
  }

  return ret;
}

static int32_t show_audio_alert_config()
{
  int32_t ret = 0;
  am_service_result_t service_result =
  {0};
  am_event_audio_alert_config_s *aa_cfg = nullptr;
  uint32_t mod_id = 1;
  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_EVENT_AUDIO_ALERT_CONFIG_GET,
                              &mod_id,
                              sizeof(mod_id),
                              &service_result,
                              sizeof(service_result));

    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to get event config!");
      break;
    }
    aa_cfg = (am_event_audio_alert_config_s*) service_result.data;

    PRINTF("\naudio alert plugin:\nchannel_num=%d, sample_rate=%d, chunk_bytes=%d, "
           "sample_format=%d, enable=%d, sensitivity=%d, threshold=%d, direction=%d\n",
           aa_cfg->channel_num,
           aa_cfg->sample_rate,
           aa_cfg->chunk_bytes,
           aa_cfg->sample_format,
           aa_cfg->enable,
           aa_cfg->sensitivity,
           aa_cfg->threshold,
           aa_cfg->direction);
  } while (0);

  return ret;
}

static int32_t show_key_input_config()
{
  int32_t ret = 0;
  am_service_result_t service_result =
  {0};
  am_event_key_input_config_s *ki_cfg = nullptr;
  uint32_t mod_id = 4;
  do {
    g_api_helper->method_call(AM_IPC_MW_CMD_EVENT_KEY_INPUT_CONFIG_GET,
                              &mod_id,
                              sizeof(mod_id),
                              &service_result,
                              sizeof(service_result));

    if ((ret = service_result.ret) != 0) {
      ERROR("Failed to get event config!");
      break;
    }
    ki_cfg = (am_event_key_input_config_s*) service_result.data;

    PRINTF("\nkey input plugin:\nlong_press_time=%d\n",
           ki_cfg->long_press_time);
  } while (0);

  return ret;
}

static int32_t show_all_info()
{
  int32_t ret = 0;
  do {
    if ((ret = show_audio_alert_config()) < 0) {
      break;
    }
    if ((ret = show_key_input_config()) < 0) {
      break;
    }
  } while (0);

  return ret;
}

static int start_all_event_module()
{
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_EVENT_START_ALL_MODULE,
                            nullptr,
                            0,
                            &service_result,
                            sizeof(service_result));
  if (service_result.ret != 0) {
    ERROR("Failed to start all event modules!\n");
  }

  return service_result.ret;
}

static int stop_all_event_module()
{
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_EVENT_STOP_ALL_MODULE,
                            nullptr,
                            0,
                            &service_result,
                            sizeof(service_result));
  if (service_result.ret != 0) {
    ERROR("Failed to stop all event modules!\n");
  }

  return service_result.ret;
}

static int destroy_all_event_module()
{
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_EVENT_DESTROY_ALL_MODULE,
                            nullptr,
                            0,
                            &service_result,
                            sizeof(service_result));
  if (service_result.ret != 0) {
    ERROR("Failed to destroy all event modules!\n");
  }

  return service_result.ret;
}

static int start_event_module(am_event_module_id id)
{
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_EVENT_START_MODULE,
                            &id,
                            sizeof(id),
                            &service_result,
                            sizeof(service_result));
  if (service_result.ret != 0) {
    ERROR("Failed to start %s event modules!\n", event_type_str[id]);
  }

  return service_result.ret;
}

static int stop_event_module(am_event_module_id id)
{
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_EVENT_STOP_MODULE,
                            &id,
                            sizeof(id),
                            &service_result,
                            sizeof(service_result));
  if (service_result.ret != 0) {
    ERROR("Failed to stop %s event modules!\n", event_type_str[id]);
  }

  return service_result.ret;
}

static int destroy_event_module(am_event_module_id id)
{
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_EVENT_DESTROY_MODULE,
                            &id,
                            sizeof(id),
                            &service_result,
                            sizeof(service_result));
  if (service_result.ret != 0) {
    ERROR("Failed to destroy %s event modules!\n", event_type_str[id]);
  }

  return service_result.ret;
}

static int check_register_module_status(am_event_module_register_state *module_register_state)
{
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_EVENT_CHECK_MODULE_REGISTER_GET,
                            &module_register_state->id,
                            sizeof(am_event_module_register_state),
                            &service_result,
                            sizeof(service_result));
  if (service_result.ret == 0) {
    memcpy(module_register_state,
           service_result.data,
           sizeof(am_event_module_register_state));
  } else {
    ERROR("Failed to check register module state %s !\n",
          event_type_str[module_register_state->id]);
  }

  return service_result.ret;
}

#if 0
static int register_event_module(am_event_module_path *module_path)
{
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_EVENT_REGISTER_MODULE_SET,
      module_path,
      sizeof(am_event_module_path),
      &service_result,
      sizeof(service_result));
  if (service_result.ret != 0) {
    ERROR("Failed to register %s event modules, path : %s!\n",
        event_type_str[module_path->id],
        module_path->path_name);
  }

  return service_result.ret;
}
#endif

static int do_show_module_register_state()
{
  int ret = 0;
  am_event_module_register_state module_register_state;

  memset(&module_register_state, 0, sizeof(module_register_state));
  do {
    if (show_regsiter_am_event_module_id != -1) {
      if (show_regsiter_am_event_module_id >= AUDIO_ALERT_DECT
          && show_regsiter_am_event_module_id < ALL_MODULE_NUM) {
        module_register_state.id =
            (am_event_module_id) show_regsiter_am_event_module_id;

        if (check_register_module_status(&module_register_state) < 0) {
          ERROR("failed to check register module state");
          ret = -1;
          break;
        }
        PRINTF("Event module %s is %s! \n",
               event_type_str[module_register_state.id],
               module_register_state.state ? "registered" : "NOT registered");
        break;
      } else if (show_regsiter_am_event_module_id == ALL_MODULE_NUM) {
        for (int i = 0; i < ALL_MODULE_NUM; i ++) {
          memset(&module_register_state, 0, sizeof(module_register_state));
          module_register_state.id = (am_event_module_id) i;
          if (check_register_module_status(&module_register_state) < 0) {
            ERROR("failed to check register module state");
            ret = -1;
            break;
          }
          PRINTF("Event module %s is %s! \n",
                 event_type_str[i],
                 module_register_state.state ? "registered" : "NOT registered");
        }
        break;
      } else {
        ERROR("Invalid module id!\n");
        ret = -1;
      }
    }
  } while (0);

  return ret;
}

static int do_stop_event_module()
{
  if (stop_am_event_module_id == ALL_MODULE_NUM) {
    if (stop_all_event_module() < 0) {
      return -1;
    }
  } else if (stop_am_event_module_id >= 0
      && stop_am_event_module_id < ALL_MODULE_NUM) {
    if (stop_event_module((am_event_module_id) stop_am_event_module_id) < 0) {
      return -1;
    }
  } else {
    ERROR("Invalid module id!\n");
    return -1;
  }

  return 0;
}

static int do_destroy_event_module()
{
  if (destroy_am_event_module_id == ALL_MODULE_NUM) {
    if (destroy_all_event_module() < 0) {
      return -1;
    }
  } else if (destroy_am_event_module_id >= 0
      && destroy_am_event_module_id < ALL_MODULE_NUM) {
    if (destroy_event_module((am_event_module_id) destroy_am_event_module_id)
        < 0) {
      return -1;
    }
  } else {
    ERROR("Invalid module id!\n");
    return -1;
  }

  return 0;
}

static int do_run_evnet_module()
{
  am_service_result_t service_result;

  if (run_am_event_module_id >= 0 && run_am_event_module_id < ALL_MODULE_NUM) {
    if (start_event_module((am_event_module_id) run_am_event_module_id) == 0) {
      switch (run_am_event_module_id) {
        case AUDIO_ALERT_DECT: {
          /*am_event_audio_alert_config_s audio_alert_config;
           memset(&audio_alert_config, 0, sizeof(am_event_audio_alert_config_s));*/

          break;
        }
        case KEY_INPUT_DECT:

          break;
        case AUDIO_ANALYSIS_DECT:
        case FACE_DECT:
          ERROR("Not implemented event module!\n");
          break;
        default:
          ERROR("Unknown module ID\n");
          break;
      }
    } else {
      return -1;
    }
  } else if (run_am_event_module_id == ALL_MODULE_NUM) {
    if (start_all_event_module() == 0) {
      for (int i = 0; i < ALL_MODULE_NUM; i ++) {
        switch ((am_event_module_id) i) {
          case AUDIO_ALERT_DECT: {
            am_event_audio_alert_config_s audio_alert_config;
            memset(&audio_alert_config,
                   0,
                   sizeof(am_event_audio_alert_config_s));

            SET_BIT(audio_alert_config.enable_bits,
                    AM_EVENT_AUDIO_ALERT_CONFIG_ENABLE);

            audio_alert_config.enable = 1;

            g_api_helper->method_call(AM_IPC_MW_CMD_EVENT_AUDIO_ALERT_CONFIG_SET,
                                      &audio_alert_config,
                                      sizeof(am_event_audio_alert_config_s),
                                      &service_result,
                                      sizeof(service_result));
            if (service_result.ret != 0) {
              ERROR("Failed to set %s event modules!\n",
                    event_type_str[(am_event_module_id )i]);
            }

            break;
          }
          case KEY_INPUT_DECT:

            break;
          case AUDIO_ANALYSIS_DECT:
          case FACE_DECT:
            NOTICE("Not implemented event module!\n");
            break;
          default:
            ERROR("Unknown module ID\n");
            return -1;
        }
      }
    }
  } else {
    ERROR("Invalid module id\n");
    return -1;
  }

  return 0;
}

int main(int32_t argc, char **argv)
{
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);
  int ret = 0;

  if (argc < 2) {
    usage(argc, argv);
    return -1;
  }

  if (init_param(argc, argv) < 0) {
    return -1;
  }

  g_api_helper = AMAPIHelper::get_instance();
  if (!g_api_helper) {
    ERROR("unable to get AMAPIHelper instance\n");
    return -1;
  }

  /*show event register state*/
  if (show_regsiter_am_event_module_id != -1) {
    if (do_show_module_register_state() < 0) {
      return -1;
    }
  }

  /*stop event module*/
  if (stop_am_event_module_id != -1) {
    if (do_stop_event_module() < 0) {
      return -1;
    }
  }

  /*destroy event module*/
  if (destroy_am_event_module_id != -1) {
    if (do_destroy_event_module() < 0) {
      return -1;
    }
  }

  /*start event module*/
  if (run_am_event_module_id != -1) {
    if (do_run_evnet_module() < 0) {
      return -1;
    }
  }

  do {
    if (show_all_info_flag) {
      show_all_info();
      break;
    } else {
      if (show_audio_alert_flag) {
        show_audio_alert_config();
      }
      if (show_key_input_flag) {
        show_key_input_config();
      }
      if (show_audio_alert_flag | show_key_input_flag) {
        break;
      }
    }

    if ((ret = set_audio_alert_config()) < 0) {
      break;
    }
    if ((ret = set_key_input_config()) < 0) {
      break;
    }
  } while (0);

  return ret;
}
