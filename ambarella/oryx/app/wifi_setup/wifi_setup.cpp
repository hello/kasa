/*
 * wifi_setup.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 15/01/2015 [Created]
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
#include "am_video_camera_if.h"
#include "am_qrcode_if.h"
#include "am_event_types.h"
#include "am_event_monitor_if.h"
#include "wifi_setup_config.h"
#include "am_playback_if.h"
#include "am_playback_msg.h"
#include "wifi_setup.h"

#ifndef MAX_CMD_LEN
#define MAX_CMD_LEN 128
#endif

#ifndef VOICE_PATH
#define VOICE_PATH "/usr/share/"
#endif

const char *resource_path[RESOURCE_NUM] = {
  VOICE_PATH"soundfx/voice/SYS_HELLO.aac",
  VOICE_PATH"soundfx/voice/SYS_MODE_NORMAL.aac",
  VOICE_PATH"soundfx/voice/SYS_MODE_QRCODE_WIFI_CONFIG.aac",
  VOICE_PATH"soundfx/voice/WIFI_SETUP_FINISH.aac",
  VOICE_PATH"soundfx/voice/WIFI_SETUP_MODE_AP.aac",
  VOICE_PATH"soundfx/voice/WIFI_SETUP_MODE_STATION.aac",
  VOICE_PATH"soundfx/voice/WIFI_SETUP_NOT_DONE.aac",
  VOICE_PATH"soundfx/voice/WIFI_SETUP_WRONG.aac",
  VOICE_PATH"soundfx/wav/error1.wav",
  VOICE_PATH"soundfx/wav/error2.wav",
  VOICE_PATH"soundfx/wav/error3.wav",
  VOICE_PATH"soundfx/wav/error4.wav",
  VOICE_PATH"soundfx/wav/failure.wav",
  VOICE_PATH"soundfx/wav/shutter1.wav",
  VOICE_PATH"soundfx/wav/shutter2.wav",
  VOICE_PATH"soundfx/wav/success1.wav",
  VOICE_PATH"soundfx/wav/success2.wav",
  VOICE_PATH"soundfx/wav/success3.wav",
  VOICE_PATH"soundfx/wav/success4.wav",
};

struct play_info_t {
  AMIPlaybackPtr  player; /* An instance to play media file */
  pthread_mutex_t lock;   /* lock and cond are used for sync. */
  pthread_cond_t  cond;
};

/*
 * playinfo[0] for voice;
 * playinfo[1] for effect.
 */
static play_info_t voice_playinfo;
static play_info_t effect_playinfo;

#define ORYX_APP_WIFI_SETUP_CONF_DIR "/etc/oryx/app/wifi_setup/"

static WIFI_SETUP_GUIDE_VOICE guide_voice = VOICE_WIFI_SETUP_NOT_DONE;
static WIFI_SETUP_STATUS status = WIFI_CONFIG_FETCH_MODE;
static AMIEventMonitorPtr event_instance = NULL;
static AMIVideoCameraPtr  video_camera = NULL;
static AMIQrcode *qrcode = NULL;
static AMWifiSetupConfig *setup_config = NULL;
static WifiSetupConfig *config = NULL;
static bool run = true;

static void callback_for_voice (AMPlaybackMsg &msg)
{
  switch (msg.msg) {
  case AM_PLAYBACK_MSG_START_OK:
    NOTICE("Voice playback engine started successfully!\n");
    break;
  case AM_PLAYBACK_MSG_PAUSE_OK:
    NOTICE("Voice playback engine paused successfully!\n");
    break;
  case AM_PLAYBACK_MSG_STOP_OK:
    NOTICE("Voice playback engine stopped successfully!\n");
    break;
  case AM_PLAYBACK_MSG_EOS:
    NOTICE("Voice playback finished successfully!\n");
    /* Notify upple level that audio has been played. */
    if (pthread_cond_broadcast (&voice_playinfo.cond)) {
      ERROR ("Error occurs on broadcasting condition variable.");
    }
    break;
  case AM_PLAYBACK_MSG_ABORT:
    NOTICE("Voice playback engine abort!\n");
    break;
  case AM_PLAYBACK_MSG_ERROR:
    NOTICE("Voice playback engine abort due to fatal error occurred!\n");
    break;
  case AM_PLAYBACK_MSG_TIMEOUT:
    NOTICE("Voice playback engine is timeout!\n");
    break;
  case AM_PLAYBACK_MSG_NULL:
  default:
    break;
  }
}

static void callback_for_effect (AMPlaybackMsg &msg)
{
  switch (msg.msg) {
  case AM_PLAYBACK_MSG_START_OK:
    NOTICE("Effect playback engine started successfully!\n");
    break;
  case AM_PLAYBACK_MSG_PAUSE_OK:
    NOTICE("Effect playback engine paused successfully!\n");
    break;
  case AM_PLAYBACK_MSG_STOP_OK:
    NOTICE("Effect playback engine stopped successfully!\n");
    break;
  case AM_PLAYBACK_MSG_EOS:
    NOTICE("Effect playback finished successfully!\n");
    /* Notify upple level that audio has been played. */
    if (pthread_cond_broadcast (&effect_playinfo.cond)) {
      ERROR ("Error occurs on broadcasting condition variable.");
    }
    break;
  case AM_PLAYBACK_MSG_ABORT:
    NOTICE("Effect playback engine abort!\n");
    break;
  case AM_PLAYBACK_MSG_ERROR:
    NOTICE("Effect playback engine abort due to fatal error occurred!\n");
    break;
  case AM_PLAYBACK_MSG_TIMEOUT:
    NOTICE("Effect playback engine is timeout!\n");
    break;
  case AM_PLAYBACK_MSG_NULL:
  default:
    break;
  }
}

static int32_t event_callback (AM_EVENT_MESSAGE *msg)
{
  if (msg->event_type == EV_KEY_INPUT_DECT) {
    NOTICE ("key value: %d, event type: %s", msg->key_event.key_value,
                                             (msg->key_event.key_state == AM_KEY_CLICKED) ? \
                                             "Short pressed" : "Long pressed");
  } else {
    DEBUG ("In wifi setup, other detect events are ignored.");
  }

  if (msg->key_event.key_state == AM_KEY_LONG_PRESSED) {
    NOTICE ("Long pressed event is detected.");
    run = false;
  }

  return 0;
}

static bool play_audio (WIFI_SETUP_AUDIO_TYPE type,
                       WIFI_SETUP_RESOURCE_ID source_id)
{
  int len = 0;
  bool ret = true;
  play_info_t *play_info = NULL;
  AMPlaybackUri play_uri;

  if (type == MUSIC_VOICE) {
    play_info = &voice_playinfo;
  } else {
    play_info = &effect_playinfo;
  }

  len = strlen (resource_path[source_id]);
  memcpy (play_uri.media.file, resource_path[source_id], len);

  play_uri.type = AM_PLAYBACK_URI_FILE;
  if (!play_info->player->play (&play_uri)) {
    ERROR ("Failed to play: %s", resource_path[source_id]);
    ret = false;
  } else {
    pthread_cond_wait (&play_info->cond, &play_info->lock);
  }

  return ret;
}

static bool delete_wifi_connection ()
{
  bool ret = true;
  int stat = 0;

  stat = system ("rm -rf /etc/NetworkManager/system-connections/*");
  if (WEXITSTATUS (stat)) {
    ERROR ("Failed to delete wifi connections.");
    ret = false;
  }

  return ret;
}

static bool connect_wifi (const char *ssid, const char *key)
{
  bool ret = true;
  char cmd[MAX_CMD_LEN];
  int stat = 0;

  do {
    if (!ssid || !key) {
      ERROR ("Invalid argument: null pointer is deteccted.");
      ret = false; break;
    }

    /* delete all connections, including the effective and not. */
    if (!delete_wifi_connection ()) {
      ERROR ("Failed to delete wifi connections.");
      ret = false; break;
    }

    snprintf (cmd, MAX_CMD_LEN,
              "nmcli dev wifi connect %s password %s",
              ssid, key);

    stat = system (cmd);
    if (WEXITSTATUS (stat)) {
      ERROR ("Failed to connect %s", ssid);
      ret = false;
    }
  } while (0);

  return ret;
}

static bool wifi_setup_init ()
{
  int status = 0;
  bool ret = true;

  do {
    if (!(event_instance = AMIEventMonitor::get_instance ())) {
      ERROR ("Failed to get an instance of event monitor.");
      ret = false; break;
    } else {
      EVENT_MODULE_CONFIG module_config;
      AM_KEY_INPUT_CALLBACK key_callback;
      memset (&module_config, 0, sizeof (EVENT_MODULE_CONFIG));

      key_callback.callback = event_callback;
      module_config.key = AM_KEY_CALLBACK;
      module_config.value = (void *)&key_callback;

      key_callback.key_value = 116;
      event_instance->set_monitor_config (EV_KEY_INPUT_DECT, &module_config);

      key_callback.key_value = 238;
      event_instance->set_monitor_config (EV_KEY_INPUT_DECT, &module_config);

      if (!event_instance->start_event_monitor (EV_KEY_INPUT_DECT)) {
        ERROR ("Failed to start event monitor.");
        ret = false; break;
      }
    }

    if (!(qrcode = AMIQrcode::create ())) {
      ERROR ("Failed to create an instance of qr code.");
      ret = false; break;
    } else {
      std::string file_name(ORYX_APP_WIFI_SETUP_CONF_DIR);
      file_name.append ("wifi.conf");
      if (!qrcode->set_config_path (file_name)) {
        ERROR ("Failed to set config path for qrcode.");
        ret = false; break;
      }
    }

    /* start 3A before creating video camera. */
    status = system ("test_image -i0 &");
    if (WEXITSTATUS (status)) {
      ERROR ("Failed to start 3A deamon process.");
      ret = false; break;
    }

    if (!(video_camera = AMIVideoCamera::get_instance ())) {
      ERROR ("Failed to create an instance of video camera.");
      ret = false; break;
    }

    if (!(setup_config = new AMWifiSetupConfig ())) {
      ERROR ("Failed to create an instance of AMWifiSetupConfig");
      ret = false; break;
    }

    std::string conf(ORYX_APP_WIFI_SETUP_CONF_DIR);
    conf.append ("wifi_setup.acs");

    if (!(config = setup_config->get_config (conf.c_str ()))) {
      ERROR ("Failed to get configuration of wifi setup app.");
      ret = false; break;
    }

    if (!(voice_playinfo.player = AMIPlayback::create ())) {
      ERROR ("Failed to create an instnace of AMIPlayback.");
      ret = false; break;
    }

    pthread_mutex_init(&voice_playinfo.lock, NULL);
    pthread_cond_init(&voice_playinfo.cond, NULL);

    if (voice_playinfo.player->init ()) {
      voice_playinfo.player->set_msg_callback(callback_for_voice, NULL);
    } else {
      ERROR ("Failed to initialize callback for voice player");
      ret = false; break;
    }

    if (!(effect_playinfo.player = AMIPlayback::create ())) {
      ERROR ("Failed to create an instnace of AMIPlayback.");
      ret = false; break;
    }

    pthread_mutex_init(&effect_playinfo.lock, NULL);
    pthread_cond_init(&effect_playinfo.cond, NULL);

    if (effect_playinfo.player->init ()) {
      effect_playinfo.player->set_msg_callback(callback_for_effect, NULL);
    } else {
      ERROR ("Failed to initialize callback for effect player");
      ret = false; break;
    }

  } while (0);

  return ret;
}

void wifi_setup_destroy ()
{
  video_camera = nullptr;

  if (qrcode) {
    qrcode->destroy ();
  }

  if (event_instance) {
    event_instance->destroy_event_monitor (EV_KEY_INPUT_DECT);
  }

  delete setup_config;
  system ("killall test_image");
}

bool scan_wifi_config_from_qrcode ()
{
  bool ret = true;

  do {
    if (video_camera->start () != AM_RESULT_OK) {
      ERROR ("Failed to start video camera");
      ret = false; break;
    }

    play_audio (MUSIC_VOICE, SYS_MODE_QRCODE_WIFI_CONFIG_VOICE);
    if (!qrcode->qrcode_read (config->source_buffer_id,
                              config->qrcode_timeout)) {
      ERROR ("Failed to read qrcode.");
      ret = false; break;
    }

    play_audio (SOUND_EFFECTS, SHUTTER2_EFFECT);
    video_camera->stop ();
  } while (0);

  return ret;
}

bool parse_wifi_config (char **ssid, char **password)
{
  bool ret = true;
  std::string key;

  do {
    if (!qrcode->qrcode_parse ()) {
      ERROR ("Failed to parse qrcode.");
      ret = false; break;
    } else {
      key = qrcode->get_key_value (std::string ("wifi"),
                                   std::string ("S"));
      if (strlen (key.c_str ()) >= config->ssid_max_len) {
        NOTICE ("wifi's ssid is too large, reallocate.");
        delete *ssid;
        if (!(*ssid = new char[strlen (key.c_str ()) + 1])) {
          ERROR ("Failed to reallocate memory for ssid");
          ret = false; break;
        }
      }

      strcpy (*ssid, key.c_str ());
      key = qrcode->get_key_value (std::string ("wifi"),
                                   std::string ("P"));
      if (strlen (key.c_str ()) >= config->password_max_len) {
        NOTICE ("wifi's password is too large, reallocate.");
        delete *password;
        if (!(*password = new char[strlen (key.c_str ()) + 1])) {
          ERROR ("Failed to reallocate memory for password");
          ret = false; break;
        }
      }

      strcpy (*password, key.c_str ());
    }
  } while (0);

  return ret;
}


bool wifi_setup ()
{
  bool ret = true;
  char *ssid = NULL;
  char *password = NULL;
  int wifi_setup_retry_times = 0;
  int qrcode_retry_times = 0;

  do {
    if (!wifi_setup_init ()) {
      ERROR ("Failed to do initializing operation in wifi setup");
      ret = false; break;
    } else {
      play_audio (MUSIC_VOICE, SYS_HELLO_VOICE);
    }

    if (!(ssid = new char[config->ssid_max_len])) {
      ERROR ("Failed to allocate memory to store ssid.");
      ret = false; break;
    }

    if (!(password = new char[config->password_max_len])) {
      ERROR ("Failed to allocate memory to store password.");
      ret = false; break;
    }

    std::string file_name(ORYX_APP_WIFI_SETUP_CONF_DIR);
    file_name.append ("wifi.conf");
    if (!access (file_name.c_str (), F_OK)) {
      status = WIFI_CONFIG_EXISTS_MODE;
    }

    while (run) {
      switch (status) {
      case WIFI_CONFIG_FETCH_MODE: {
        NOTICE ("In WIFI_CONFIG_FETCH_MODE.");
        qrcode_retry_times = 0;
        guide_voice = VOICE_QRCODE_CONFIG;
        NOTICE ("Press Button B in short time.");
        NOTICE ("qrcode_scan_max_times = %d", config->qrcode_scan_max_times);

        while (++qrcode_retry_times <= config->qrcode_scan_max_times) {
          if (!scan_wifi_config_from_qrcode ()) {
            NOTICE ("Failed to scan wifi config, try again.");
          } else {
            status = WIFI_CONFIG_EXISTS_MODE;
            break;
          }
        }

        if (qrcode_retry_times > config->qrcode_scan_max_times) {
          NOTICE ("Retry %d times, quit.", config->qrcode_scan_max_times);
          guide_voice = VOICE_WIFI_SETUP_WRONG;
          play_audio (MUSIC_VOICE, WIFI_SETUP_WRONG_VOICE);
          run = false;
        }
      } break;

      case WIFI_CONFIG_EXISTS_MODE: {
        NOTICE ("In WIFI_CONFIG_EXISTS_MODE.");
        status = WIFI_CONFIG_FETCH_MODE;
        if (!parse_wifi_config (&ssid, &password)) {
          NOTICE ("Failed to parse wifi config: wrong format.");
        } else {
          NOTICE ("ssid: %s, password: %s", ssid, password);
          if (!connect_wifi (ssid, password)) {
            ERROR ("Failed to connect %s", ssid);
          } else {
            status = NORMAL_RUNNING_MODE;
            guide_voice = VOICE_WIFI_SETUP_FINISHED;
            play_audio (MUSIC_VOICE, WIFI_SETUP_FINISH_VOICE);
            NOTICE ("wifi setup OK.");
          }
        }

        if ((status == WIFI_CONFIG_FETCH_MODE) &&
            (++wifi_setup_retry_times > config->wifi_setup_max_times)) {
          run = false;
          guide_voice = VOICE_WIFI_SETUP_WRONG;
          play_audio (MUSIC_VOICE, WIFI_SETUP_WRONG_VOICE);
        }
      } break;

      case NORMAL_RUNNING_MODE: {
        /*
         * Wifi has been configured successfully and you can
         * start what you want to do here.
         *
         * Currently, we just endup this while loop.
         */
        NOTICE ("In NORMAL_RUNNING_MODE.");
        run = false;
      } break;

      default: {
        ERROR ("No such status: %s", (uint8_t)status);
      } break;
      }
    }


  } while (0);

  delete[] ssid;
  delete[] password;

  return ret;
}

int
main (int argc, char **argv)
{
  int ret = 0;

  wifi_setup ();
  wifi_setup_destroy ();

  return ret;
}
