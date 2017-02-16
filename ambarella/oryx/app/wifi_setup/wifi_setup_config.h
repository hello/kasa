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
struct WifiSetupConfig
{
  uint8_t source_buffer_id;
  uint8_t qrcode_timeout;
  uint8_t ssid_max_len;
  uint8_t password_max_len;
  uint8_t wifi_setup_max_times;
  uint8_t qrcode_scan_max_times;
};

class AMConfig;
class AMWifiSetupConfig
{
  public:
    AMWifiSetupConfig ();
    virtual ~AMWifiSetupConfig ();
    WifiSetupConfig *get_config (const char *config_file);

  private:
    AMConfig        *m_config;
    WifiSetupConfig *m_wifi_setup_config;
};
