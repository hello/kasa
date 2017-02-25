/*******************************************************************************
 * am_network_manager_priv.h
 *
 * History:
 *   2015-1-12 - [Tao Wu] created file
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

#ifndef AM_NETWORK_MANAGER_PRIV_H_
#define AM_NETWORK_MANAGER_PRIV_H_

struct NmCbData
{
    NMDevice *device;
    GSList   *connections;
    int32_t   retval;
    uint32_t  timeout;
    bool      isdone;
    NmCbData() :
      device(nullptr),
      connections(nullptr),
      retval(-1),
      timeout(90),
      isdone(false){}
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
                        NetInfoIPv4 *ipv4 = nullptr,
                        NetInfoIPv6 *ipv6 = nullptr);
    int wifi_connect_ap_static(const char *ap_name,
                        const char *password,
                        NetInfoIPv4 *ipv4 = nullptr,
                        NetInfoIPv6 *ipv6 = nullptr);

    int wifi_connect_hidden_ap(const char  *ap_name,
                        const char  *password,
                        const AMNetworkManager::APSecurityFlags security,
                        uint32_t wep_index,
                        uint32_t     timeout = 90,
                        NetInfoIPv4 *ipv4 = nullptr,
                        NetInfoIPv6 *ipv6 = nullptr);
    int wifi_connect_hidden_ap_static(const char  *ap_name,
                        const char  *password,
                        const AMNetworkManager::APSecurityFlags security,
                        uint32_t wep_index,
                        NetInfoIPv4 *ipv4 = nullptr,
                        NetInfoIPv6 *ipv6 = nullptr);

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
    NMClient         *mNmClient;
    GSList           *mSysConnections;
    NMRemoteSettings *mSysSettings;
    GMainLoop        *mLoop;
    bool              mIsInited;
};

#endif /* AM_NETWORK_MANAGER_PRIV_H_ */
