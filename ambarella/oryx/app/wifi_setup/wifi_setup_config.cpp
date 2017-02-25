/*
 * wifi_setup_config.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 20/01/2015 [Created]
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

#include "wifi_setup_config.h"
#include "am_configure.h"

AMWifiSetupConfig::AMWifiSetupConfig () :
  m_config (NULL),
  m_wifi_setup_config (NULL)
{}

AMWifiSetupConfig::~AMWifiSetupConfig ()
{
  delete m_config;
  delete m_wifi_setup_config;
}

WifiSetupConfig *AMWifiSetupConfig::get_config (const char *config_file)
{
  do {
    if (!m_wifi_setup_config) {
      if (!(m_wifi_setup_config = new WifiSetupConfig ())) {
        ERROR ("Failed to create an instance of wifi setup config.");
        break;
      }
    }

    delete m_config;
    if (!(m_config = AMConfig::create (config_file))) {
      ERROR ("Failed to create an instance of AMConfig.");
      break;
    }

    AMConfig &wifi_setup = *m_config;
    if (wifi_setup["source_buffer_id"].exists ()) {
      m_wifi_setup_config->source_buffer_id =
        wifi_setup["source_buffer_id"].get<unsigned>(0);
    } else {
      NOTICE ("source buffer id is not specified, use default!");
      m_wifi_setup_config->source_buffer_id = 0;
    }

    if (wifi_setup["qrcode_timeout"].exists ()) {
      m_wifi_setup_config->qrcode_timeout =
        wifi_setup["qrcode_timeout"].get<unsigned>(5);
    } else {
      NOTICE ("qrcode time is not specified, use default!");
      m_wifi_setup_config->qrcode_timeout = 5;
    }

    if (wifi_setup["ssid_max_len"].exists ()) {
      m_wifi_setup_config->ssid_max_len =
        wifi_setup["ssid_max_len"].get<unsigned>(32);
    } else {
      NOTICE ("ssid_max_len is not specified, use default!");
      m_wifi_setup_config->ssid_max_len = 32;
    }

    if (wifi_setup["password_max_len"].exists ()) {
      m_wifi_setup_config->password_max_len =
        wifi_setup["password_max_len"].get<unsigned>(64);
    } else {
      NOTICE ("password is not specified, use default!");
      m_wifi_setup_config->password_max_len = 64;
    }

    if (wifi_setup["wifi_setup_max_times"].exists ()) {
      m_wifi_setup_config->wifi_setup_max_times =
        wifi_setup["wifi_setup_max_times"].get<unsigned>(5);
    } else {
      NOTICE ("maximum times for wifi setup is not specified, use default!");
      m_wifi_setup_config->wifi_setup_max_times = 5;
    }

    if (wifi_setup["qrcode_scan_max_times"].exists ()) {
      m_wifi_setup_config->qrcode_scan_max_times =
        wifi_setup["qrcode_scan_max_times"].get<unsigned>(5);
    } else {
      NOTICE ("maximum times for qrcode scanning is not specified, use default!");
      m_wifi_setup_config->qrcode_scan_max_times = 5;
    }

  } while (0);

  return m_wifi_setup_config;
}
