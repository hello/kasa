/*******************************************************************************
 * test_sip_service_air_api.cpp
 *
 * History:
 *   2015-4-15 - [Shiming Dong] created file
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
#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"
#include "getopt.h"

#include "am_api_helper.h"
#include "am_api_sip.h"

static struct option long_options[] =
{
  {"help",                 no_argument,       0,  'h'},

  {"username",             required_argument, 0,  'n' },
  {"password",             required_argument, 0,  'w' },
  {"server-url",           required_argument, 0,  'u' },
  {"server-port",          required_argument, 0,  'p' },
  {"sip-port",             required_argument, 0,  's' },
  {"expires",              required_argument, 0,  'e' },
  {"is-register",          required_argument, 0,  'i' },

  {"rtp-stream-port-base", required_argument, 0,  'r' },
  {"max-conn-num",         required_argument, 0,  'm' },
  {"enable-video",         required_argument, 0,  'v' },

  {"audio-priority-list",  required_argument, 0,  'a' },

  {"call",                 required_argument, 0,  'c' },
  {"hang-up",              optional_argument, 0,  'g' },

  {"apply",                no_argument,       0,  'A' },
  {0,                      0,                 0,   0  }

};

static const char *short_options = "hn:w:u:p:s:e:i:r:m:v:a:c:g::A";

struct hint32_t_s {
  const char *arg;
  const char *str;
};

static const hint32_t_s hint32_t[] =
{
  {"",     "\t\t\t\t"  "Show usage\n"},

  {"", "\t\t\t\t"  "username used to register to SIP server."},
  {"", "\t\t\t\t"  "password used to register to SIP server."},
  {"", "\t\t\t"    "SIP server URL where register to."},
  {"", "\t\t\t"    "SIP server's port, default 5060."},
  {"", "\t\t\t\t"  "local SIP port, default 5060."},
  {"seconds", "\t\t"  "How long the registration is valid."},
  {"true|false", "\t\t"  "If register to SIP server.\n"},

  {"even number", "base RTP port to use for sending and receiving data."},
  {"", "\t\t\t"  "The maximum number of simultaneous connections."},
  {"true|false", "\t"  "Enable video in SIP communication.\n"},

  {"audio types",  "audio priority list determine which audio type "
                   "to choose first.\n"
                   "\t\t\t\t\t\tsupported types: PCMU PCMA G726-40 G726-32"
                   " G726-24 G726-16 opus mpeg4-generic\n"},

  {"SIP address", "\t\t" "Initiate a SIP call."},
  {"username",    "\t\t"  "Hang up the specific call or disconnect all "
                          "connections without argument.\n"},

  {"", "\t\t\t\t" "Apply config files' changes and Re-register to SIP server "
       "immediately.\n"},

};

static void usage(int32_t argc, char **argv)
{
  printf("\n%s usage:\n\n", argv[0]);
  for (uint32_t i = 0; i < sizeof(long_options)/sizeof(long_options[0])-1; ++i) {
    if (isalpha(long_options[i].val)) {
      printf("-%c,  ", long_options[i].val);
    } else {
      printf("    ");
    }
    printf("--%s", long_options[i].name);
    if (hint32_t[i].arg[0] != 0) {
      printf(" [%s]", hint32_t[i].arg);
    }
    printf("\t%s\n", hint32_t[i].str);
  }
  printf("Examples:\n  test_sip_service_air_api -c "
                   "sip:username@sip.linphone.org\n"
         "  test_sip_service_air_api -g            (Disconnect all connections"
                                                   " without argument)\n"
         "  test_sip_service_air_api -gusername    (Hangup the specific call. "
                                "NOTICE: There is no space between the option "
                                         "'g' and the argument \"username\")\n"
         "  test_sip_service_air_api -n name -w password -u sip.linphone.org"
         " -p 5060 -s 5060 -e 3600 -i true -r 5004 -m 2 -v true"
         " -a PCMA PCMU G726-32 opus -A\n\n");
}


int main(int argc, char *argv[]) {
  int ret = 0;
  int32_t option_index = 0;
  bool reg_modify = false;
  bool media_modify = false;
  bool con_modify = false;
  AMAPIHelperPtr api_helper = nullptr;
  am_service_result_t result; /* method call result */

  AMIApiSipRegisterParameter *reg_par = AMIApiSipRegisterParameter::create();
  AMIApiMediaPriorityList *media_list = AMIApiMediaPriorityList::create();
  AMIApiSipConfigParameter *con_par   = AMIApiSipConfigParameter::create();

  do {
    if (argc < 2) {
      usage(argc, argv);
      ret = -1;
      break;
    }

    if ((api_helper = AMAPIHelper::get_instance()) == nullptr) {
      ERROR("AMAPIHelper::get_instance failed.");
      ret = -1;
      break;
    }

    if (!reg_par || !media_list || !con_par) {
      ERROR("Create error!");
      break;
    }

    int opt;
    while ((opt = getopt_long(argc, argv, short_options, long_options,
                              &option_index)) != -1) {
      switch (opt) {
        case 'h':
          usage(argc, argv);
          return -1;
        case 'n':
          reg_par->set_username(std::string(optarg));
          reg_modify = true;
          break;
        case 'w':
          reg_par->set_password(std::string(optarg));
          reg_modify = true;
          break;
        case 'u':
          reg_par->set_server_url(std::string(optarg));
          reg_modify = true;
          break;
        case 'p':
          reg_par->set_server_port(atoi(optarg));
          reg_modify = true;
          break;
        case 's':
          reg_par->set_sip_port(atoi(optarg));
          reg_modify = true;
          break;
        case 'e':
          reg_par->set_expires(atoi(optarg));
          reg_modify = true;
          break;
        case 'i':
          reg_par->set_is_register((is_str_equal(argv[optind - 1], "true") ?
                                   true : false));
          reg_modify = true;
          break;
        case 'r':
          con_par->set_rtp_stream_port_base(atoi(optarg));
          con_modify = true;
          break;
        case 'm':
          con_par->set_max_conn_num(atoi(optarg));
          con_modify = true;
          break;
        case 'v':
          con_par->set_enable_video(is_str_equal(argv[optind - 1], "true") ?
          true : false);
          con_modify = true;
          break;
        case 'a':
          do {
            if ((optind <= argc) && !is_str_start_with(argv[optind - 1], "-")) {
              media_list->add_media_to_list(optarg);
            } else {
              ERROR("Invalid parameters!");
              break;
            }
            while ((optind < argc) && !is_str_start_with(argv[optind], "-")) {
              media_list->add_media_to_list(argv[optind++]);
            }
          } while (0);
          media_modify = true;
          break;
        case 'c': {
          AMIApiSipCalleeAddress  *address = AMIApiSipCalleeAddress::create();
          if (address) {
            address->set_callee_address(std::string(optarg));

            api_helper->method_call(AM_IPC_MW_CMD_SET_SIP_CALLEE_ADDRESS,
                        address->get_sip_callee_parameter(),
                        address->get_sip_callee_parameter_size(), &result,
                        sizeof(result));
            if (result.ret != 0) {
              ERROR("Failed to set sip callee address!");
              ret = -1;
            } else {
              NOTICE("set sip callee address successfully!");
            }
            address->destroy();
            address = nullptr;
          } else {
            ERROR("Create AMIApiSipCalleeAddress error!");
          }
        }break;
        case 'g': {
          AMIApiSipCalleeAddress *username = AMIApiSipCalleeAddress::create();
          if (username) {
            if (optarg) {
              username->set_hangup_username(std::string(optarg));
            } else {
              username->set_hangup_username(std::string("disconnect_all"));
            }
            api_helper->method_call(AM_IPC_MW_CMD_SET_SIP_HANGUP_USERNAME,
                        username->get_hangup_username(),
                        username->get_hangup_username_size(), &result,
                        sizeof(result));
            if (result.ret != 0) {
              ERROR("Failed to set sip hangup username!");
              ret = -1;
            } else {
              NOTICE("set sip hangup username successfully!");
            }
            username->destroy();
            username = nullptr;
          } else {
            ERROR("Create AMIApiSipCalleeAddress error!");
          }
        }break;
        case 'A':
          reg_par->set_is_apply(true);
          reg_modify = true;
          break;
        default:
          WARN("Invalid option!");
          usage(argc, argv);
          break;
      }
    }

    if (reg_modify) {
      api_helper->method_call(AM_IPC_MW_CMD_SET_SIP_REGISTRATION,
          reg_par->get_sip_register_parameter(),
          reg_par->get_sip_register_parameter_size(), &result,
          sizeof(result));
      if (result.ret != 0) {
        ERROR("Failed to set sip registration!");
        ret = -1;
      } else {
        reg_modify = false;
        NOTICE("set sip registration parameters successfully!");
      }
    }

    if (con_modify) {
      api_helper->method_call(AM_IPC_MW_CMD_SET_SIP_CONFIGURATION,
          con_par->get_sip_config_parameter(),
          con_par->get_sip_config_parameter_size(), &result, sizeof(result));
      if (result.ret != 0) {
        ERROR("Failed to set sip configuration!");
        ret = -1;
      } else {
        con_modify = false;
        NOTICE("set sip configuration successfully!");
      }
    }

    if (media_modify) {
      api_helper->method_call(AM_IPC_MW_CMD_SET_SIP_MEDIA_PRIORITY,
          media_list->get_media_priority_list(),
          media_list->get_media_priority_list_size(), &result, sizeof(result));
      if (result.ret != 0) {
        ERROR("Failed to set sip media priority!");
        ret = -1;
      } else {
        media_modify = false;
        NOTICE("set media priority list successfully!");
      }
    }
  } while (0);

  reg_par->destroy();
  reg_par = nullptr;

  con_par->destroy();
  con_par = nullptr;

  media_list->destroy();
  media_list = nullptr;

  return ret;
}
