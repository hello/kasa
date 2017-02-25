/*
 * am_key_input.cpp
 *
 * Histroy:
 *  2014-11-19 [Dongge Wu] Create file
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
#include "am_log.h"
#include "am_define.h"
#include "am_thread.h"
#include "am_mutex.h"

#include "am_event_types.h"
#include "am_base_event_plugin.h"
#include "am_key_input_config.h"
#include "am_key_input.h"

#include <time.h>
#include <linux/input.h>
#include <sys/timerfd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define HW_TIMER ((const char*)"/proc/ambarella/ambarella_hwtimer")

#define KEY_INPUT_CONFIG ((const char*)"event-key-input.acs")

DECLARE_EVENT_PLUGIN_INIT_FINIT(AMKeyInput, EV_KEY_INPUT_DECT)

AMIEventPlugin* AMKeyInput::create(EVENT_MODULE_ID mid)
{
  INFO("AMKeyInput::Create \n");
  AMKeyInput *result = new AMKeyInput(mid);
  if (result && result->construct() < 0) {
    ERROR("Failed to create an instance of AMKeyInput");
    delete result;
    result = nullptr;
  }

  return result;
}

AMKeyInput::AMKeyInput(EVENT_MODULE_ID mid) :
  m_plugin_id(mid)
{
}

AMKeyInput::~AMKeyInput()
{
  /*save config*/
  if (AM_UNLIKELY(!sync_config())) {
    ERROR("save config file failed!\n");
  }

  m_key_event_running = false;
  m_wait_key_pressed = false;
  m_key_event_timer = false;

  if (AM_LIKELY(m_key_pressed_cond)) {
    m_key_pressed_cond->signal();
  }

  if (AM_LIKELY(m_key_released_cond)) {
    m_key_released_cond->signal_all();
  }

  AM_DESTROY(m_event_capture_thread);
  AM_DESTROY(m_event_timer_thread);
  AM_DESTROY(m_key_map_mutex);
  AM_DESTROY(m_callback_map_mutex);
  AM_DESTROY(m_key_input_mutex);
  AM_DESTROY(m_key_pressed_cond);
  AM_DESTROY(m_key_released_cond);
  delete m_key_input_config;

  if (AM_LIKELY(m_hw_timer_fd >= 0)) {
    close(m_hw_timer_fd);
  }
  if (AM_LIKELY(EVT_CAP_CTRL_R >= 0)) {
    close(EVT_CAP_CTRL_R);
  }
  if (AM_LIKELY(EVT_CAP_CTRL_W >= 0)) {
    close(EVT_CAP_CTRL_W);
  }
}

int32_t AMKeyInput::construct()
{
  KeyInputConfig *key_config = nullptr;
  if (AM_LIKELY(m_hw_timer_fd < 0)) {
    if (AM_UNLIKELY((m_hw_timer_fd = open(HW_TIMER, O_RDONLY)) < 0)) {
      ERROR("Failed to open %s: %s", HW_TIMER, strerror(errno));
      return -1;
    }
  }
  if (AM_UNLIKELY(socketpair(AF_UNIX, SOCK_STREAM, IPPROTO_IP,
                             m_event_cap_ctrl_sock))) {
    PERROR("socketpair");
    return -1;
  }

  m_key_map_mutex = AMMutex::create(false);
  if (AM_UNLIKELY(!m_key_map_mutex)) {
    ERROR("Create mutex failed!\n");
    return -1;
  }

  m_callback_map_mutex = AMMutex::create(false);
  if (AM_UNLIKELY(!m_callback_map_mutex)) {
    ERROR("Create mutex failed!\n");
    return -1;
  }

  m_key_input_mutex = AMMutex::create(false);
  if (AM_UNLIKELY(!m_key_input_mutex)) {
    ERROR("Create mutex failed!\n");
    return -1;
  }

  m_key_pressed_cond = AMCondition::create();
  if (AM_UNLIKELY(!m_key_pressed_cond)) {
    ERROR("Create key pressed condition failed!\n");
    return -1;
  }

  m_key_released_cond = AMCondition::create();
  if (AM_UNLIKELY(!m_key_released_cond)) {
    ERROR("Create key released condition failed!\n");
    return -1;
  }

  m_key_input_config = new AMKeyInputConfig();
  if (AM_UNLIKELY(!m_key_input_config)) {
    ERROR("Failed to new AMKeyInputConfig object!\n");
    return -1;
  }

  /*Load default config file*/
  m_conf_path.append(ORYX_EVENT_CONF_SUB_DIR).append(KEY_INPUT_CONFIG);
  key_config = m_key_input_config->get_config(m_conf_path);
  if (!key_config) {
    ERROR("key input get config failed!\n");
    return -1;
  }
  m_long_pressed_time = key_config->long_press_time;

  return 0;

}

bool AMKeyInput::get_key_state(AM_KEY_CODE key_code, AM_KEY_STATE *state)
{
  AUTO_MTX_LOCK(m_key_input_mutex);
  if (AM_UNLIKELY(!m_key_event_running)) {
    ERROR("Key input plugin is not running!");
    return false;
  }

  m_key_map_mutex->lock();
  std::map<AM_KEY_CODE, AM_KEY_NODE>::iterator it =
      m_pressed_key_map.find(key_code);
  if (it != m_pressed_key_map.end()) {
    *state = it->second.key_state;
    m_key_map_mutex->unlock();
    return true;
  }

  /*If not found key code in list, just return key up state*/
  *state = AM_KEY_UP;
  m_key_map_mutex->unlock();
  return true;
}

bool AMKeyInput::wait_key_pressed(AM_KEY_CODE key_code, AM_KEY_STATE *pResult)
{
  struct timeval pressed_time;

  AUTO_MTX_LOCK(m_key_input_mutex);
  if (AM_UNLIKELY(!m_key_event_running)) {
    ERROR("Key input plugin is not running!");
    return false;
  }

  memset(&pressed_time, 0, sizeof(pressed_time));
  m_key_map_mutex->lock();
  std::map<AM_KEY_CODE, AM_KEY_NODE>::iterator it =
      m_pressed_key_map.find(key_code);
  if (it != m_pressed_key_map.end() && it->second.key_state == AM_KEY_DOWN) {
    pressed_time = it->second.time_stamp;
  }

  while (m_wait_key_pressed) {
    m_key_released_cond->wait(m_key_map_mutex);
    it = m_pressed_key_map.find(key_code);

    if (it != m_pressed_key_map.end() && it->second.key_state == AM_KEY_UP) {
      *pResult =
          time_diff(&pressed_time, &it->second.time_stamp)
              < m_long_pressed_time ? AM_KEY_CLICKED : AM_KEY_LONG_PRESSED;
      m_key_map_mutex->unlock();
      break;
    }
  }

  return true;
}

bool AMKeyInput::set_key_callback(AM_KEY_CODE key_code,
                                  AM_EVENT_CALLBACK callback)
{
  AUTO_MTX_LOCK(m_key_input_mutex);
  m_key_map_mutex->lock();
  if (key_code == -1) {
    m_default_callback = callback;
  }
  m_key_callback_map[key_code] = callback;
  m_key_map_mutex->unlock();
  return true;
}

bool AMKeyInput::set_long_pressed_time(uint32_t ms)
{
  AUTO_MTX_LOCK(m_key_input_mutex);
  if (ms > LONG_PRESS_MAX_TIME || ms < LONG_PRESS_MIN_TIME) {
    ERROR("press time is out of valid range");
    return false;
  }

  m_long_pressed_time = ms;
  return true;
}

uint32_t AMKeyInput::get_long_pressed_time()
{
  AUTO_MTX_LOCK(m_key_input_mutex);
  return m_long_pressed_time;
}

bool AMKeyInput::sync_config()
{
  AUTO_MTX_LOCK(m_key_input_mutex);
  /*save config file*/
  KeyInputConfig *key_config;
  key_config = m_key_input_config->get_config(m_conf_path);
  if (!key_config) {
    ERROR("key input get config failed !\n");
    return false;
  }

  key_config->long_press_time = (int32_t) m_long_pressed_time;
  return m_key_input_config->set_config(key_config, m_conf_path);
}

bool AMKeyInput::start_plugin()
{
  AUTO_MTX_LOCK(m_key_input_mutex);
  if (!m_key_event_running) {
    m_event_capture_thread = AMThread::create("KeyInput.cap",
                                              static_key_event_capture,
                                              (void*) this);
    if (AM_UNLIKELY(!m_event_capture_thread)) {
      ERROR("Create key input event capture thread failed!\n");
      return false;
    }
  } else {
    NOTICE("key event plugin is already running!\n");
    return true;
  }

  if (!m_key_event_timer) {
    m_event_timer_thread = AMThread::create("KeyInput.timer",
                                            static_key_event_timer,
                                            (void*) this);
    if (AM_UNLIKELY(!m_event_timer_thread)) {
      ERROR("Create key input event process thread failed!\n");
      return false;
    } else {
      m_wait_key_pressed = true;
      NOTICE("key event timer thread created OK!\n");
      return true;
    }
  }

  return true;
}

bool AMKeyInput::stop_plugin()
{
  AUTO_MTX_LOCK(m_key_input_mutex);
  if (!m_key_event_running && !m_wait_key_pressed && !m_key_event_timer) {
    NOTICE("key input plugin has been stopped already!");
  } else {
    int ret = 0;
    int count = 0;
    char cmd[1] = {'q'};

    INFO("stop key input plugin!");

    m_key_event_running = false;
    m_key_event_timer = false;
    m_wait_key_pressed = false;

    do {
      ret = write(EVT_CAP_CTRL_W, cmd, sizeof(cmd));
      if (AM_UNLIKELY(ret <= 0)) {
        if (AM_LIKELY((errno != EAGAIN) &&
                      (errno != EWOULDBLOCK) &&
                      (errno != EINTR))) {
          ERROR("Failed to send stop command(%c) to Key Event Capture Thread!",
                cmd);
          break;
        }
      }
    }while((++ count < 5) && ((ret > 0) && (ret < (int)sizeof(cmd))));

    if (AM_LIKELY(m_key_pressed_cond)) {
      m_key_pressed_cond->signal();
    }

    if (AM_LIKELY(m_key_released_cond)) {
      m_key_released_cond->signal_all();
    }

    AM_DESTROY(m_event_capture_thread);
    AM_DESTROY(m_event_timer_thread);
  }

  return true;
}

bool AMKeyInput::set_plugin_config(EVENT_MODULE_CONFIG *pConfig)
{
  bool ret = true;

  if (pConfig == nullptr || pConfig->value == nullptr) {
    ERROR("Invalid argument!\n");
    return false;
  }

  switch (pConfig->key) {
    case AM_KEY_CALLBACK:
      ret =
          set_key_callback(((AM_KEY_INPUT_CALLBACK*) pConfig->value)->key_value,
                           ((AM_KEY_INPUT_CALLBACK*) pConfig->value)->callback);
      break;
    case AM_LONG_PRESSED_TIME:
      ret = set_long_pressed_time(*(uint32_t *) pConfig->value);
      break;
    case AM_KEY_INPUT_SYNC_CONFIG:
      ret = sync_config();
      break;
    default:
      ERROR("Unknown key, please check 'enum AM_KEY_INPUT_KEY' define. \n");
      ret = false;
      break;
  }

  return ret;
}

bool AMKeyInput::get_plugin_config(EVENT_MODULE_CONFIG *pConfig)
{
  bool ret = true;

  if (pConfig == nullptr || pConfig->value == nullptr) {
    ERROR("Invalid argument!\n");
    return false;
  }

  switch (pConfig->key) {
    case AM_GET_KEY_STATE:
      ret = get_key_state(((AM_KEY_INPUT_EVENT*) pConfig->value)->key_value,
                          &((AM_KEY_INPUT_EVENT*) pConfig->value)->key_state);
      break;
    case AM_WAIT_KEY_PRESSED:
      ret = wait_key_pressed(((AM_KEY_INPUT_EVENT*) pConfig->value)->key_value,
                            &((AM_KEY_INPUT_EVENT*) pConfig->value)->key_state);
      break;
    case AM_LONG_PRESSED_TIME:
      *(uint32_t *) pConfig->value = get_long_pressed_time();
      break;
    default:
      ERROR("Unknown key, please check 'enum AM_KEY_INPUT_KEY' define. \n");
      ret = false;
      break;
  }

  return ret;
}

EVENT_MODULE_ID AMKeyInput::get_plugin_ID()
{
  return m_plugin_id;
}

void AMKeyInput::key_event_timer()
{
  AUTO_MTX_LOCK(m_key_map_mutex);
  m_key_event_timer = true;

  while (m_key_event_timer) {
    /* wait key pressed event*/
    m_key_pressed_cond->wait(m_key_map_mutex);
    /* wait key released event if time out*/
    if (m_key_event_timer &&
        !m_key_released_cond->wait(m_key_map_mutex, m_long_pressed_time)) {
      std::map<AM_KEY_CODE, AM_KEY_NODE>::iterator it =
          m_pressed_key_map.begin();
      for (; it != m_pressed_key_map.end(); it ++) {
        if (it->second.key_state == AM_KEY_DOWN) {
          DEBUG("key event timer \n");
          std::map<AM_KEY_CODE, AM_EVENT_CALLBACK>::iterator it_callback =
              m_key_callback_map.find(it->first);
          if (it_callback != m_key_callback_map.end()) {
            AM_EVENT_MESSAGE msg;
            memset(&msg, 0, sizeof(AM_EVENT_MESSAGE));
            msg.event_type = m_plugin_id;
            msg.seq_num = m_seq_num ++;
            msg.pts = get_current_pts();
            msg.key_event.key_value = it->first;
            msg.key_event.key_state = AM_KEY_LONG_PRESSED;
            it_callback->second(&msg);
          }
          it->second.key_state = AM_KEY_UP;
          m_key_released_cond->signal();
        }
      }
    }
  }
}

inline void AMKeyInput::event_process(const struct input_event &event)
{
  AM_KEY_NODE key_node;

  /* key pressed state*/
  if (event.value) {
    DEBUG("key pressed state event.value == %d \n", event.value);
    key_node.time_stamp = event.time;
    key_node.key_state = AM_KEY_DOWN;
    m_key_map_mutex->lock();
    std::map<AM_KEY_CODE, AM_KEY_NODE>::iterator it =
        m_pressed_key_map.find(event.code);
    if (it != m_pressed_key_map.end()) {
      it->second = key_node;
    } else {
      m_pressed_key_map[event.code] = key_node;
      m_key_callback_map[event.code] = m_default_callback;
    }

    m_key_pressed_cond->signal();
    m_key_map_mutex->unlock();
  } else {
    /* key released state*/
    DEBUG("key released state event.value == %d \n", event.value);
    key_node.time_stamp = event.time;
    key_node.key_state = AM_KEY_UP;
    m_key_map_mutex->lock();

    std::map<AM_KEY_CODE, AM_KEY_NODE>::iterator it =
        m_pressed_key_map.find(event.code);
    if (it != m_pressed_key_map.end()) {
      if (it->second.key_state == AM_KEY_DOWN) {
        std::map<AM_KEY_CODE, AM_EVENT_CALLBACK>::iterator it_callback =
            m_key_callback_map.find(it->first);
        if (it_callback != m_key_callback_map.end()) {
          AM_EVENT_MESSAGE msg;
          memset(&msg, 0, sizeof(AM_EVENT_MESSAGE));
          msg.event_type = m_plugin_id;
          msg.seq_num = m_seq_num ++;
          msg.pts = get_current_pts();
          msg.key_event.key_value = event.code;
          msg.key_event.key_state = AM_KEY_CLICKED;
          it_callback->second(&msg);
        }

        it->second = key_node;
        m_key_released_cond->signal_all();
      } else {
        /*Time out has happened, donot need to issue signal*/
        it->second = key_node;
      }
    }

    m_key_map_mutex->unlock();
  }
}

void AMKeyInput::key_event_capture()
{
  struct input_event event;
  fd_set rfds;
  fd_set allfds;
  int maxfd = -1;
  int ret = 0;
  char str[64] = { 0 };
  int fd[EVENT_MAX_CHANNEL] = {-1};
  bool run = true;

  FD_ZERO(&allfds);
  FD_ZERO(&rfds);

  FD_SET(EVT_CAP_CTRL_R, &allfds);
  maxfd = AM_MAX(maxfd, EVT_CAP_CTRL_R);

  for (int i = 0; i < EVENT_MAX_CHANNEL; i ++) {
    sprintf(str, "%s%d", EVENT_PATH, i);
    fd[i] = open(str, O_NONBLOCK);
    if (AM_UNLIKELY(fd[i] < 0)) {
      WARN("open %s error: %s", str, strerror(errno));
      continue;
    } else {
      FD_SET(fd[i], &allfds);
      maxfd = AM_MAX(maxfd, fd[i]);
      m_key_event_running = true;
    }
  }

  if (m_key_event_running == false) {
    WARN("open event device failed!");
    return;
  }

  while (run && m_key_event_running) {
    struct timeval timeout = { 0, 200000 };
    rfds = allfds;

    ret = select(maxfd + 1, &rfds, nullptr, nullptr, &timeout);
    if (ret < 0) {
      if (AM_UNLIKELY(errno != EINTR)) {
        PERROR("select");
        m_key_event_running = false;
      }
      continue;
    } else if (AM_LIKELY(ret == 0)) {
      continue;
    }

    if (AM_LIKELY(FD_ISSET(EVT_CAP_CTRL_R, &rfds))) {
      char cmd[1] = {0};
      if (AM_LIKELY(read(EVT_CAP_CTRL_R, cmd, sizeof(cmd)) < 0)) {
        ERROR("Failed to read Key Event Capture Control Command! Quit anyway!");
        cmd[0] = 'q';
      } else {
        NOTICE("Received command from Key Event Capture Control Socket: %c",
               cmd[0]);
      }
      switch(cmd[0]) {
        case 'q': run = false; break;
        default:break;
      }
    }
    for (int i = 0; i < EVENT_MAX_CHANNEL; i ++) {
      if (AM_LIKELY(FD_ISSET(fd[i], &rfds))) {
        ret = read(fd[i], &event, sizeof(event));
        if (AM_UNLIKELY(ret < 0)) {
          ERROR("read fd[%d] error: %s", i, strerror(errno));
          m_key_event_running= false;
          break;
        }

        if (event.type != EV_KEY) {
          continue;
        }

        event_process(event);
      }
    }
  }

  for (int i = 0; i < EVENT_MAX_CHANNEL; i ++) {
    if (fd[i] >= 0) {
      close(fd[i]);
    }
    fd[i] = -1;
  }
}

void AMKeyInput::static_key_event_capture(void* arg)
{
  return ((AMKeyInput*) arg)->key_event_capture();
}

void AMKeyInput::static_key_event_timer(void * arg)
{
  return ((AMKeyInput*) arg)->key_event_timer();
}

inline int64_t AMKeyInput::time_diff(const struct timeval *start_time,
                                     const struct timeval *end_time)
{
  int64_t msec;
  msec = (end_time->tv_sec - start_time->tv_sec) * 1000;
  msec += (end_time->tv_usec - start_time->tv_usec) / 1000;
  return msec;
}

inline uint64_t AMKeyInput::get_current_pts()
{
  uint8_t pts[32] =
  { 0 };
  uint64_t cur_pts = 0;
  if (AM_LIKELY(m_hw_timer_fd >= 0)) {
    if (AM_UNLIKELY(read(m_hw_timer_fd, pts, sizeof(pts)) < 0)) {
      PERROR("read");
    } else {
      cur_pts = strtoull((const char*) pts, (char**) nullptr, 10);
    }
  }

  return cur_pts;
}
