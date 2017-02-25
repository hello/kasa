/*******************************************************************************
 * am_network_manager.h
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

#ifndef AM_NETWORK_MANAGER_H_
#define AM_NETWORK_MANAGER_H_

class AMNetworkManager_priv;
class AMNetworkManager
{
  public:
    enum NetworkState {
      AM_NM_STATE_UNKNOWN          = 0,
      AM_NM_STATE_ASLEEP           = 1,
      AM_NM_STATE_CONNECTING       = 2,
      AM_NM_STATE_CONNECTED_LOCAL  = 3,
      AM_NM_STATE_CONNECTED_SITE   = 4,
      AM_NM_STATE_CONNECTED_GLOBAL = 5,
      AM_NM_STATE_DISCONNECTED     = 6
    };
    enum DeviceState {
      AM_DEV_STATE_UNKNOWN      = 0,
      AM_DEV_STATE_UNMANAGED    = 1,
      AM_DEV_STATE_UNAVAILABLE  = 2,
      AM_DEV_STATE_DISCONNECTED = 3,
      AM_DEV_STATE_PREPARE      = 4,
      AM_DEV_STATE_CONFIG       = 5,
      AM_DEV_STATE_NEED_AUTH    = 6,
      AM_DEV_STATE_IP_CONFIG    = 7,
      AM_DEV_STATE_IP_CHECK     = 8,
      AM_DEV_STATE_SECONDARIES  = 9,
      AM_DEV_STATE_ACTIVATED    = 10,
      AM_DEV_STATE_DEACTIVATING = 11,
      AM_DEV_STATE_FAILED       = 12
    };
    enum APSecurityFlags{
      AM_802_11_AP_SEC_NONE            = 0x00000000,
      AM_802_11_AP_SEC_PAIR_WEP40      = 0x00000001,
      AM_802_11_AP_SEC_PAIR_WEP104     = 0x00000002,
      AM_802_11_AP_SEC_PAIR_TKIP       = 0x00000004,
      AM_802_11_AP_SEC_PAIR_CCMP       = 0x00000008,
      AM_802_11_AP_SEC_GROUP_WEP40     = 0x00000010,
      AM_802_11_AP_SEC_GROUP_WEP104    = 0x00000020,
      AM_802_11_AP_SEC_GROUP_TKIP      = 0x00000040,
      AM_802_11_AP_SEC_GROUP_CCMP      = 0x00000080,
      AM_802_11_AP_SEC_KEY_MGMT_PSK    = 0x00000100,
      AM_802_11_AP_SEC_KEY_MGMT_802_1X = 0x00000200
    };

  public:
    AMNetworkManager();
    virtual ~AMNetworkManager();

  public:
    const char *net_state_to_string(AMNetworkManager::NetworkState state);
    const char *device_state_to_string(AMNetworkManager::DeviceState state);
    NetworkState get_network_state();
    DeviceState get_device_state(const char *iface);
    uint32_t get_current_device_speed(const char *iface);
    bool wifi_get_active_ap(APInfo **infos);

    int wifi_list_ap(APInfo **infos);
    int wifi_connect_ap(const char *apName, const char*password);
    int wifi_connect_ap_static(const char *apName, const char*password, NetInfoIPv4 *ipv4);
    int wifi_connect_hidden_ap(const char *apName, const char*password,
                    const AMNetworkManager::APSecurityFlags secuirty, uint32_t wep_index);
    int wifi_connect_hidden_ap_static(const char *apName, const char*password,
                    const AMNetworkManager::APSecurityFlags secuirty, uint32_t wep_index, NetInfoIPv4 *ipv4);

    int wifi_setup_adhoc(const char *name, const char*password, const AMNetworkManager::APSecurityFlags security);
    int wifi_setup_ap(const char *name, const char*password, const AMNetworkManager::APSecurityFlags security, int channel);

    int wifi_connection_up(const char* ap_name);
    int wifi_connection_delete(const char* ap_name);

    bool disconnect_device(const char *iface);
    bool wifi_set_enabled(bool enable);
    bool network_set_enabled(bool enable);

  private:
    AMNetworkManager_priv *mNmPriv;
};

#endif /* AM_NETWORK_MANAGER_H_ */
