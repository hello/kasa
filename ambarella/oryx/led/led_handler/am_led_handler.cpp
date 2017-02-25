/*******************************************************************************
 * am_led_handler.cpp
 *
 * History:
 *   2015-1-27 - [longli] created file
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
#include <sys/time.h>
#include <mutex>
#include <iostream>
#include <thread>
#include "am_led_handler.h"

using namespace std;
static mutex m_mtx;
static mutex m_mtx_i;
#define  DECLARE_MUTEX  lock_guard<mutex> lck(m_mtx);
#define  DECLARE_MUTEX_I  lock_guard<mutex> lck_i(m_mtx_i);

#define POLL_INTERVAL 1
#define USEC_INTERVAL 100000
#define BUF_LEN 128
#define VBUF_LEN  4

AMLEDHandler *AMLEDHandler::m_instance = nullptr;

AMLEDHandler::AMLEDHandler():
  m_blk_count(0),
  m_thread_run(false),
  m_ref_counter(0)
{
}

AMLEDHandler::~AMLEDHandler()
{
  del_all_led();
  m_instance = nullptr;
}

void AMLEDHandler::print_led_list()
{
  DECLARE_MUTEX;

  INFO("Enter print_led_list()");
  int32_t count = 0;
  printf("---------------------------------------------\n");
  printf("gpio_id\tled_on\tblink\ton_time\toff_time\n");
  for (auto &it : m_led_config) {
    printf(" %d\t %s\t %s\t %dms\t %dms\n", it.gpio_id,
                                            it.led_on ? "on" : "off",
                                            it.blink_flag ? "on" : "off",
                                            it.on_time * 100,
                                            it.off_time * 100);
    ++count;
  }

  if (count) {
    printf("---------------------------------------------\n");
  } else {
    printf("\nNo gpio led has been set yet!\n");
  }
}

bool AMLEDHandler::gpio_export(const int32_t gpio_id, const bool gpio_export)
{
  bool ret = true;
  int32_t fd = -1;
  int32_t access_ret = -1;
  char buf[BUF_LEN] = {0};
  char vbuf[VBUF_LEN] = {0};

  INFO("%s gpio %d", gpio_export ? "export" : "unexport", gpio_id);
  if (gpio_id > -1) {
    sprintf(buf, "/sys/class/gpio/gpio%d", gpio_id);
    access_ret = access(buf, F_OK);
    if ((access_ret && gpio_export) || (!access_ret && !gpio_export)) {
      sprintf(buf, "/sys/class/gpio/%s", gpio_export ? "export" : "unexport");
      if ((fd = open(buf, O_WRONLY)) < 0) {
        ret = false;
        ERROR("open %s fail", buf);
      } else {
        sprintf(vbuf, "%d", gpio_id);
        int32_t len = (int32_t)strlen(vbuf);
        if (len != write(fd, vbuf, len)) {
          ret = false;
          ERROR("write %s to %s fail.", vbuf, buf);
        }

        close(fd);
      }
    }
  } else {
    ERROR("Invalid gpio id: %d", gpio_id);
    ret = false;
  }

  return ret;
}

bool AMLEDHandler::set_gpio_direction_out(const int32_t gpio_id,
                                          const bool gpio_out)
{
  bool ret = true;
  int32_t fd = -1;
  char buf[BUF_LEN] = {0};
  char vbuf[VBUF_LEN] = {0};

  INFO("set_gpio_direction gpio%d %s", gpio_id, gpio_out ? "out":"in");
  sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
  if ((fd = open(buf, O_WRONLY)) < 0) {
    ret = false;
    ERROR("Fail to open %s", buf);
  } else {
    int32_t len = 0;
    if (gpio_out) {
      sprintf(vbuf, "%s","out");
      len = 3;
    } else {
      sprintf(vbuf, "%s","in");
      len = 2;
    }

    if (len != write(fd, vbuf, len)) {
      ret = false;
      ERROR("Fail to write %s to %s", vbuf, buf);
    }
    close(fd);
  }

  return ret;
}

bool AMLEDHandler::init_gpio(const AMLEDConfig &led_ins)
{
  bool ret = true;

  INFO("init gpio %d", led_ins.gpio_id);
  if (gpio_export(led_ins.gpio_id, true)) {
    if (set_gpio_direction_out(led_ins.gpio_id, true)) {
      if (led_ins.blink_flag) {
        AMLEDConfigList::iterator it = m_led_config.begin();
        m_led_config.insert(it, led_ins);
      } else {
        m_led_config.push_back(led_ins);
      }
    } else {
      ret = false;
      gpio_export(led_ins.gpio_id, false);
      ERROR("set gpio%d direction out error.", led_ins.gpio_id);
    }
  } else {
    ret = false;
    ERROR("do gpio%d export error", led_ins.gpio_id);
  }

  return ret;
}

bool AMLEDHandler::light_gpio_led(const int32_t gpio_id, const bool led_on)
{
  bool ret = true;
  int32_t fd = -1;
  char buf[BUF_LEN] = {0};
  char vbuf[VBUF_LEN] = {0};

  sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio_id);
  fd = open(buf, O_RDWR);
  if (fd < 0) {
    ERROR("Open %s value failed\n", buf, gpio_id);
    ret = false;
  } else {
    sprintf(vbuf, "%s",(led_on ? "0":"1"));
    if (write(fd, vbuf, 1) != 1) {
      ERROR("Write gpio%d value failed\n", gpio_id);
      ret = false;
    }
    close(fd);
  }

  return ret;
}

bool AMLEDHandler::deinit_led(const int32_t gpio_id)
{
  DECLARE_MUTEX;
  bool ret = true;
  bool found = false;

  INFO("Enter deinit_led gpio index %d", gpio_id);
  do {
    if (m_led_config.empty()) {
      ret = false;
      PRINTF("No gpio led has been inited.");
      break;
    }

    AMLEDConfigList::iterator it = m_led_config.begin();
    for ( ;it != m_led_config.end(); ++it) {
      if(it->gpio_id == gpio_id) {
        gpio_export(gpio_id, false);
        if (it->blink_flag) {
          --m_blk_count;
          if (m_blk_count < 1)
            m_thread_run = false;
        }
        m_led_config.erase(it);
        found = true;
        INFO("find: gpio_id exists in led list.");
        break;
      }
    }

    if (!found) {
      ret = false;
      PRINTF("gpio led index %d hasn't been inited before.", gpio_id);
      break;
    }
  } while (0);

  return ret;
}

void AMLEDHandler::deinit_all_led()
{
  DECLARE_MUTEX;

  INFO("Enter deinit_all_led");
  del_all_led();
}

void AMLEDHandler::del_all_led()
{
  INFO("Enter del_all_led");
  if (!m_led_config.empty()) {
    m_thread_run = false;
    AMLEDConfigList::iterator it = m_led_config.begin();
    for ( ;it != m_led_config.end(); ++it) {
      INFO("deinit gpio%d...");
      gpio_export(it->gpio_id, false);
    }

    m_led_config.clear();
    m_blk_count = 0;
  }
}

bool AMLEDHandler::get_led_state(AMLEDConfig &led_ins)
{
  bool ret = false;

  INFO("Enter get_led_state gpio index %d", led_ins.gpio_id);
  do {
    if (!m_led_config.empty()) {
      for (auto &it : m_led_config) {
        if(it.gpio_id == led_ins.gpio_id) {
          led_ins = it;
          ret = true;
          INFO("find: gpio_id exists in led list.");
          break;
        }
      }
    } else {
      PRINTF("No gpio led has been inited.");
    }
  } while (0);

  return ret;
}

bool AMLEDHandler::set_led_state(const AMLEDConfig &led_set)
{
  DECLARE_MUTEX;
  bool ret = true;
  bool found = false;
  bool blk_flag = false;
  AMLEDConfig led_ins = led_set;

  INFO("set_led_state gpio%d", led_ins.gpio_id);
  do {
    if (led_ins.blink_flag) {
      led_ins.led_on = false;
    }

    if (!m_led_config.empty()) {
      AMLEDConfigList::iterator it = m_led_config.begin();
      for ( ;it != m_led_config.end(); ++it) {
        if(it->gpio_id == led_ins.gpio_id) {
          blk_flag = it->blink_flag;
          if (!blk_flag && led_ins.blink_flag) {
            /* put the blink led at the head of the list */
            m_led_config.erase(it);
            it = m_led_config.begin();
            m_led_config.insert(it, led_ins);
          } else if (blk_flag && !led_ins.blink_flag) {
            m_led_config.erase(it);
            m_led_config.push_back(led_ins);
          } else {
            *it = led_ins;
          }
          found = true;
          INFO("find: gpio%d exists in led list.", led_ins.gpio_id);
          break;
        }
      }
    }

    if (!found) {
      if (!init_gpio(led_ins)) {
        ret = false;
        ERROR("init gpio%d fail", led_ins.gpio_id);
        break;
      }
    }

    if (blk_flag) {
      --m_blk_count;
    }

    if (led_ins.blink_flag) {
      ++m_blk_count;
    } else {
      if (!light_gpio_led(led_ins.gpio_id, led_ins.led_on)) {
        ret = false;
        ERROR("set gpio led %s error", led_ins.led_on ? "on":"off");
        break;
      }
    }
  } while (0);

  if (m_blk_count < 1) {
    INFO("blink thread m_thread_run = false.");
    m_thread_run = false;
  } else if (!m_thread_run){
    INFO("blink thread m_thread_run = false.");
    m_thread_run = true;
    //run blink thread
    thread th(blink_thread);
    th.detach();
  }

  return ret;
}

void AMLEDHandler::poll_to_light_led()
{
  DECLARE_MUTEX;
  int32_t count = 0;

  do {
    if (m_led_config.empty()) {
      break;
    }

    for(auto &it : m_led_config) {
      if (it.blink_flag) {
        it.accum_time += POLL_INTERVAL;
        if (it.led_on) {
          if (it.accum_time >= it.on_time) {
            light_gpio_led(it.gpio_id, false);
            it.accum_time = 0;
            it.led_on = false;
          }
        } else {
          if (it.accum_time >= it.off_time) {
            light_gpio_led(it.gpio_id, true);
            it.accum_time = 0;
            it.led_on = true;
          }
        }

        if (++count >= m_blk_count) {
          break;
        }

      } else {
        continue;
      }
    }//for
  } while (0);
}

void AMLEDHandler::blink_thread()
{
  AMLEDHandler *ins = nullptr;

  INFO("Enter blink thread...");
  do {
    ins = AMLEDHandler::get_instance();
    if (!ins) {
      ERROR("AMLEDHandler::get_instance() failed, exit!!!");
      break;
    }

    while (ins->m_thread_run) {
      ins->poll_to_light_led();
      usleep(POLL_INTERVAL * USEC_INTERVAL);
    }
  } while (0);
  INFO("Exit blink thread...");
}

AMILEDHandlerPtr AMILEDHandler::get_instance()
{
  return ((AMILEDHandler*)AMLEDHandler::get_instance());
}

AMLEDHandler *AMLEDHandler::get_instance()
{
  DECLARE_MUTEX_I;
  INFO("AMLEDHandler::get_instance() is called.");

  if (!m_instance) {
    m_instance = new AMLEDHandler();
    if (!m_instance) {
      ERROR("AMLEDHandler::get_instance() init failed.");
    }
  }

  return m_instance;
}

void AMLEDHandler::release()
{
  DECLARE_MUTEX_I;
  if ( m_thread_run && m_ref_counter == 1) {
    /* Last instance call */
    m_thread_run = false;
    /* wait led poll thread to exit */
    usleep(POLL_INTERVAL * USEC_INTERVAL * 2);
  }

  DECLARE_MUTEX;
  INFO("AMLEDHandler release is called.");
  if ((m_ref_counter >= 0) && (--m_ref_counter <= 0)) {
    NOTICE("This is the last reference of AMLEDHandler's object, "
        "delete object instance 0x%p", m_instance);
    delete m_instance;
    m_instance = nullptr;
  }
}

void AMLEDHandler::inc_ref()
{
  ++ m_ref_counter;
}
