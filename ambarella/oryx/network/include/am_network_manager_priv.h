/*******************************************************************************
 * am_network_manager_priv.h
 *
 * History:
 *   2015-1-12 - [Tao Wu] created file
 *
 * Copyright (C) 2008-2015, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_NETWORK_MANAGER_PRIV_H_
#define AM_NETWORK_MANAGER_PRIV_H_

struct NmCbData
{
    NMDevice *device;
    GSList   *connections;
    bool      isdone;
    int32_t   retval;
    uint32_t  timeout;
    NmCbData() :
      device(NULL),
      connections(NULL),
      isdone(false),
      retval(-1),
      timeout(90){}
};

class AMNetworkManager_priv
{
  public:
    AMNetworkManager_priv();
    virtual ~AMNetworkManager_priv();

  public:
    AMNetworkManager::NetworkState get_network_state();
    AMNetworkManager::DeviceState get_device_state(const char *iface);
    uint32_t get_current_device_speed(const char *iface);
    bool wifi_get_active_ap(APInfo **infos);

    int wifi_list_ap(APInfo **infos);
    int wifi_connect_ap(const char *ap_name,
                        const char *password,
                        NMWepKeyType wepKeyType = NM_WEP_KEY_TYPE_KEY,
                        uint32_t timeout = 90,
                        NetInfoIPv4 *ipv4 = NULL,
                        NetInfoIPv6 *ipv6 = NULL);
    int wifi_connect_ap_static(const char *ap_name,
                        const char *password,
                        NetInfoIPv4 *ipv4 = NULL,
                        NetInfoIPv6 *ipv6 = NULL);

    int wifi_connect_hidden_ap(const char  *ap_name,
                        const char  *password,
                        const AMNetworkManager::APSecurityFlags security,
                        uint32_t wep_index,
                        uint32_t     timeout = 90,
                        NetInfoIPv4 *ipv4 = NULL,
                        NetInfoIPv6 *ipv6 = NULL);
    int wifi_connect_hidden_ap_static(const char  *ap_name,
                        const char  *password,
                        const AMNetworkManager::APSecurityFlags security,
                        uint32_t wep_index,
                        NetInfoIPv4 *ipv4 = NULL,
                        NetInfoIPv6 *ipv6 = NULL);

    int wifi_setup_adhoc(const char *name, const char*password, const AMNetworkManager::APSecurityFlags security);
    int wifi_setup_ap(const char *name, const char*password, const AMNetworkManager::APSecurityFlags security, int channel = 0);

    int wifi_connection_up(const char* ap_name, uint32_t timeout = 90);
    int wifi_connection_delete(const char* ap_name);

    int disconnect_device(const char *iface);
    bool wifi_set_enabled(bool enable);
    bool network_set_enabled(bool enable);

  private:
    bool init();
    void fini();

  private:
    bool get_all_connections();
    NMDevice* find_device_by_name(const char *iface);
    NMAccessPoint* find_ap_on_device(NMDevice *device,
                                     const char *ssid);
    NMDevice* find_wifi_device_by_iface(const GPtrArray *devices,
                                        const char      *iface,
                                        int             *idx);

    int show_access_point_info(NMDevice *device, APInfo **infos);
    void detail_access_point(NMAccessPoint *ap, APInfo *info);
    char* ap_wpa_rsn_flags_to_string(NM80211ApSecurityFlags flags);
    char* ssid_to_printable(const char *str, gsize len);

  private:
    bool              mIsInited;
    NMClient         *mNmClient;
    GSList           *mSysConnections;
    NMRemoteSettings *mSysSettings;
    GMainLoop        *mLoop;
};

#endif /* AM_NETWORK_MANAGER_PRIV_H_ */
