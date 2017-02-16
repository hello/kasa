/*
 * wifi_setup_config.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 20/01/2015 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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
