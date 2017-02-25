/*
 * am_key_input_config.cpp
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
#include "am_define.h"
#include "am_log.h"

#include "am_key_input_config.h"

AMKeyInputConfig::AMKeyInputConfig() :
    m_config(nullptr),
    m_key_input_config(nullptr)
{

}

AMKeyInputConfig::~AMKeyInputConfig()
{
  delete m_config;
  delete m_key_input_config;

}

KeyInputConfig* AMKeyInputConfig::get_config(const std::string& config)
{
  do {
    delete m_config;
    m_config = nullptr;
    m_config = AMConfig::create(config.c_str());
    if (!m_config) {
      ERROR("Create AMConfig failed!\n");
      break;
    }

    if (!m_key_input_config) {
      m_key_input_config = new KeyInputConfig();
      if (!m_key_input_config) {
        ERROR("new key input config failed!\n");
        break;
      }
    }

    AMConfig &key_input = *m_config;
    if (key_input["long_press_time"].exists()) {
      m_key_input_config->long_press_time =
          key_input["long_press_time"].get<int>(2000);
    } else {
      WARN("long_press_time is not specified, use default!\n");
      m_key_input_config->long_press_time = 2000;
    }
  } while (0);

  return m_key_input_config;
}

bool AMKeyInputConfig::set_config(KeyInputConfig *key_input_config,
                                  const std::string& config)
{
  bool ret = true;
  do {
    delete m_config;
    m_config = nullptr;
    m_config = AMConfig::create(config.c_str());
    if (!m_config) {
      ERROR("Create AMConfig failed!\n");
      ret = false;
      break;
    }

    if (!m_key_input_config) {
      m_key_input_config = new KeyInputConfig();
      if (!m_key_input_config) {
        ERROR("new key input config failed!\n");
        ret = false;
        break;
      }
    }

    memcpy(m_key_input_config, key_input_config, sizeof(KeyInputConfig));
    AMConfig &key_input = *m_config;
    if (key_input["long_press_time"].exists()) {
      key_input["long_press_time"] =
          m_key_input_config->long_press_time;
    }

    key_input.save();
  } while (0);

  return ret;
}
