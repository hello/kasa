/*******************************************************************************
 * am_network_manager_priv.cpp
 *
 * History:
 *   2015-1-12 - [Tao Wu] created file
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
#include <net/if.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if_arp.h>
#include <dbus/dbus-glib.h>
#include <nm-remote-settings.h>
#include <nm-utils.h>
#include <nm-client.h>
#include <nm-device-ethernet.h>
#include <nm-device-wifi.h>
#include <nm-device-wimax.h>
#include <nm-device.h>
#include <nm-device-adsl.h>
#include <nm-device-modem.h>
#include <nm-device-bt.h>
#include <nm-device-olpc-mesh.h>
#include <nm-device-infiniband.h>
#include <nm-device-bond.h>
#include <nm-device-vlan.h>

#include <nm-setting-ip4-config.h>
#include <nm-setting-ip6-config.h>
#include <nm-vpn-connection.h>
#include <nm-setting-connection.h>
#include <nm-setting-wired.h>
#include <nm-setting-adsl.h>
#include <nm-setting-pppoe.h>
#include <nm-setting-wireless.h>
#include <nm-setting-gsm.h>
#include <nm-setting-cdma.h>
#include <nm-setting-bluetooth.h>
#include <nm-setting-olpc-mesh.h>

#include <nm-setting-infiniband.h>
#include <nm-access-point.h>

#include <glib.h>
#include <glib/gi18n.h>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_network.h"
#include "am_network_manager_priv.h"

APInfo::APInfo() :
    active_bssid(nullptr),
    device(nullptr),
    bssid(nullptr),
    ap_name(nullptr),
    ssid_str(nullptr),
    mode(nullptr),
    freq_str(nullptr),
    bitrate_str(nullptr),
    strength_str(nullptr),
    security_str(nullptr),
    wpa_flags_str(nullptr),
    rsn_flags_str(nullptr),
    active(nullptr),
    path(nullptr),
    info_next(nullptr),
    index(0)
{
}

APInfo::~APInfo()
{
  g_free((gpointer)active_bssid);
  g_free((gpointer)device);
  g_free((gpointer)bssid);
  g_free((gpointer)ap_name);
  g_free((gpointer)ssid_str);
  g_free((gpointer)mode);
  g_free((gpointer)freq_str);
  g_free((gpointer)bitrate_str);
  g_free((gpointer)strength_str);
  g_free((gpointer)security_str);
  g_free((gpointer)wpa_flags_str);
  g_free((gpointer)rsn_flags_str);
  g_free((gpointer)active);
  g_free((gpointer)path);
  delete info_next;
}

void APInfo::print_ap_info()
{
  PRINTF("===================================================================");
  PRINTF("      ap_name = %s", ap_name);
  PRINTF("     ssid_str = %s", ssid_str);
  PRINTF("        bssid = %s", bssid);
  PRINTF("         mode = %s", mode);
  PRINTF("     freq_str = %s", freq_str);
  PRINTF("  bitrate_str = %s", bitrate_str);
  PRINTF(" strength_str = %s", strength_str);
  PRINTF(" security_str = %s", security_str);
  PRINTF("wpa_flags_str = %s", wpa_flags_str);
  PRINTF("rsn_flags_str = %s", rsn_flags_str);
  PRINTF(" info->device = %s", device);
  PRINTF("       active = %s", active);
  PRINTF("         path = %s", path);
}

AMNetworkManager_priv::AMNetworkManager_priv() :
mNmClient(nullptr),
mSysConnections(nullptr),
mSysSettings(nullptr),
mLoop(nullptr),
mIsInited(false)
{
}

AMNetworkManager_priv::~AMNetworkManager_priv()
{
  fini();
}

AMNetworkManager::NetworkState AMNetworkManager_priv::get_network_state()
{
  AMNetworkManager::NetworkState state = AMNetworkManager::AM_NM_STATE_UNKNOWN;

  if (init()) {
    switch (nm_client_get_state(mNmClient)) {
      case NM_STATE_ASLEEP:
        state = AMNetworkManager::AM_NM_STATE_ASLEEP;
        INFO("NetworkState: asleep");
        break;
      case NM_STATE_CONNECTING:
        state = AMNetworkManager::AM_NM_STATE_CONNECTING;
        INFO("NetworkState: connecting");
        break;
      case NM_STATE_CONNECTED_LOCAL:
        state = AMNetworkManager::AM_NM_STATE_CONNECTED_LOCAL;
        INFO("NetworkState: connected (local only)");
        break;
      case NM_STATE_CONNECTED_SITE:
        state = AMNetworkManager::AM_NM_STATE_CONNECTED_SITE;
        INFO("NetworkState: connected (site only)");
        break;
      case NM_STATE_CONNECTED_GLOBAL:
        state = AMNetworkManager::AM_NM_STATE_CONNECTED_GLOBAL;
        INFO("NetworkState: connected (global)");
        break;
      case NM_STATE_DISCONNECTED:
        state = AMNetworkManager::AM_NM_STATE_DISCONNECTED;
        INFO("NetworkState: disconnected");
        break;
      case NM_STATE_UNKNOWN:
      default:
        state = AMNetworkManager::AM_NM_STATE_UNKNOWN;
        INFO("NetworkState: unknown");
        break;
    }
  }
  return state;
}

AMNetworkManager::DeviceState AMNetworkManager_priv::get_device_state(
    const char *iface)
{
  AMNetworkManager::DeviceState state =
      AMNetworkManager::AM_DEV_STATE_UNAVAILABLE;
  NMDevice *nmDevice = find_device_by_name(iface);
  if (AM_LIKELY(nmDevice)) {
    switch (nm_device_get_state(nmDevice)) {
      case NM_DEVICE_STATE_UNMANAGED: {
        state = AMNetworkManager::AM_DEV_STATE_UNMANAGED;
        INFO("%s state: unmanaged", iface);
      }
      break;
      case NM_DEVICE_STATE_UNAVAILABLE: {
        state = AMNetworkManager::AM_DEV_STATE_UNAVAILABLE;
        INFO("%s state: unavailable", iface);
      }
      break;
      case NM_DEVICE_STATE_DISCONNECTED: {
        state = AMNetworkManager::AM_DEV_STATE_DISCONNECTED;
        INFO("%s state: disconnected", iface);
      }
      break;
      case NM_DEVICE_STATE_PREPARE: {
        state = AMNetworkManager::AM_DEV_STATE_PREPARE;
        INFO("%s state: connecting (prepare)", iface);
      }
      break;
      case NM_DEVICE_STATE_CONFIG: {
        state = AMNetworkManager::AM_DEV_STATE_CONFIG;
        INFO("%s state: connecting (configuring)", iface);
      }
      break;
      case NM_DEVICE_STATE_NEED_AUTH: {
        state = AMNetworkManager::AM_DEV_STATE_NEED_AUTH;
        INFO("%s state: connecting (need authentication)", iface);
      }
      break;
      case NM_DEVICE_STATE_IP_CONFIG: {
        state = AMNetworkManager::AM_DEV_STATE_IP_CONFIG;
        INFO("%s state: connecting (getting IP configuration)", iface);
      }
      break;
      case NM_DEVICE_STATE_IP_CHECK: {
        state = AMNetworkManager::AM_DEV_STATE_IP_CHECK;
        INFO("%s state: connecting (checking IP connectivity)", iface);
      }
      break;
      case NM_DEVICE_STATE_SECONDARIES: {
        state = AMNetworkManager::AM_DEV_STATE_SECONDARIES;
        INFO("%s state: connecting (starting dependent connections)", iface);
      }
      break;
      case NM_DEVICE_STATE_ACTIVATED: {
        state = AMNetworkManager::AM_DEV_STATE_ACTIVATED;
        INFO("%s state: connected", iface);
      }
      break;
      case NM_DEVICE_STATE_DEACTIVATING: {
        state = AMNetworkManager::AM_DEV_STATE_DEACTIVATING;
        INFO("%s state: disconnecting", iface);
      }
      break;
      case NM_DEVICE_STATE_FAILED: {
        state = AMNetworkManager::AM_DEV_STATE_FAILED;
        INFO("%s state: connection failed", iface);
      }
      break;
      case NM_DEVICE_STATE_UNKNOWN:
      default: {
        state = AMNetworkManager::AM_DEV_STATE_UNKNOWN;
        INFO("%s state: unknown", iface);
      }
      break;
    }
  }

  return state;
}

uint32_t AMNetworkManager_priv::get_current_device_speed(const char *iface)
{
  uint32_t speed = 0;
  NMDevice *nmDevice = find_device_by_name(iface);
  if (AM_LIKELY(nmDevice)) {
    if (NM_IS_DEVICE_ETHERNET(nmDevice)) {
      speed = nm_device_ethernet_get_speed(NM_DEVICE_ETHERNET(nmDevice) );
    } else if (NM_IS_DEVICE_WIFI(nmDevice)) {
      speed = nm_device_wifi_get_bitrate(NM_DEVICE_WIFI(nmDevice) ) / 1000;
    }
  }

  return speed;
}

bool AMNetworkManager_priv::init()
{
  if (AM_LIKELY(false == mIsInited)) {
#if GLIB_VERSION_MIN_REQUIRED < GLIB_VERSION_2_36
    g_type_init();
#endif
    if (AM_LIKELY(nullptr != (mNmClient = nm_client_new()))) {
      mLoop = g_main_loop_new(nullptr, FALSE);
      mIsInited = get_all_connections();
    } else {
      ERROR("Failed to create NetworkManager client!");
    }
  }

  return mIsInited;
}

void AMNetworkManager_priv::fini()
{
  if (AM_LIKELY(mIsInited)) {
    g_object_unref(mNmClient);
    g_slist_free(mSysConnections);
    g_object_unref(mSysSettings);
    g_main_loop_unref(mLoop);
    mNmClient = nullptr;
    mIsInited = false;
  }
}

static void get_connections_cb(NMRemoteSettings *settings, gpointer data)
{
  NmCbData *info = (NmCbData*)data;
  info->connections = nm_remote_settings_list_connections(settings);
  info->isdone = true;
}

bool AMNetworkManager_priv::get_all_connections()
{
  GError *error = nullptr;
  bool ret = false;
  DBusGConnection *bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
  if (AM_UNLIKELY(error || !bus)) {
    ERROR("Could not connect to dbus: ", error->message);
    g_clear_error(&error);
  } else {
    mSysSettings = nm_remote_settings_new(bus);
    if (AM_UNLIKELY(!mSysSettings)) {
      ERROR("Cannot get system settings!");
    } else {
      gboolean systemSettingRunning = false;
      g_object_get(mSysSettings, NM_REMOTE_SETTINGS_SERVICE_RUNNING,
                   &systemSettingRunning, nullptr);
      if (AM_UNLIKELY(!systemSettingRunning)) {
        ERROR("Settings service is not running!");
      } else {
        NmCbData info;
        info.isdone = false;
        g_signal_connect(mSysSettings, NM_REMOTE_SETTINGS_CONNECTIONS_READ,
                         G_CALLBACK(get_connections_cb), &info);
        while (!info.isdone) {
          g_main_context_iteration(g_main_loop_get_context(mLoop), FALSE);
          usleep(50000);
        }
        mSysConnections = info.connections;
        ret = true;
      }
    }
    dbus_g_connection_unref(bus);
  }

  return ret;
}

NMDevice* AMNetworkManager_priv::find_device_by_name(const char *iface)
{
  NMDevice *nmDevice = nullptr;
  if (AM_LIKELY(init() && iface)) {
    const GPtrArray *devices = nm_client_get_devices(mNmClient);
    if (AM_LIKELY(devices)) {
      bool isMatch = false;
      for (uint32_t i = 0; i < devices->len; ++ i) {
        NMDevice *dev = NM_DEVICE(g_ptr_array_index(devices, i));
        const char *name = nm_device_get_iface(dev);

        if (AM_LIKELY(true == (isMatch = is_str_same(iface, name)))) {
          nmDevice = dev;
          break;
        }
      }
      if (AM_UNLIKELY(false == isMatch)) {
        ERROR("No such device: %s", iface);
      }
    } else {
      ERROR("Failed to get devices!");
    }
  } else if (false == mIsInited) {
    ERROR("Not initialized!");
  } else {
    ERROR("Invalid iface: %s", iface);
  }

  return nmDevice;
}

int AMNetworkManager_priv::show_access_point_info(NMDevice *device,
                                                 APInfo **infos)
{
  NMAccessPoint *active_ap = nullptr;
  const char *active_bssid = nullptr;
  const GPtrArray *aps;
  APInfo **info = infos;
  unsigned j;

  if (nm_device_get_state(device) == NM_DEVICE_STATE_ACTIVATED) {
    active_ap =
        nm_device_wifi_get_active_access_point(NM_DEVICE_WIFI (device));
    active_bssid = active_ap ? nm_access_point_get_bssid(active_ap) : nullptr;
  }
  aps = nm_device_wifi_get_access_points(NM_DEVICE_WIFI (device) );
  for (j = 0; aps && (j < aps->len); j ++) {
    NMAccessPoint *ap = NM_ACCESS_POINT(g_ptr_array_index (aps, j));
    if (*info != nullptr) {
      info = &((*info)->info_next);
    }

    *info = new APInfo();
    (*info)->index = j;
    (*info)->active_bssid = active_bssid ? g_strdup(active_bssid) : nullptr;
    (*info)->device = g_strdup(nm_device_get_iface(device));
    detail_access_point(ap, *info);
  }
  return j;
}

void AMNetworkManager_priv::detail_access_point(NMAccessPoint *ap,
                                                APInfo        *info)
{
  NM80211ApFlags flags = NM_802_11_AP_FLAGS_NONE;
  NM80211ApSecurityFlags wpa_flags = NM_802_11_AP_SEC_NONE;
  NM80211ApSecurityFlags rsn_flags = NM_802_11_AP_SEC_NONE;
  const GByteArray *ssid = nullptr;
  NM80211Mode mode = NM_802_11_MODE_UNKNOWN;
  bool active = false;
  char *freq_str = nullptr;
  char *ssid_str = nullptr;
  char *bitrate_str = nullptr;
  char *strength_str = nullptr;
  char *wpa_flags_str = nullptr;
  char *rsn_flags_str = nullptr;
  GString *security_str = nullptr;

  if (info->active_bssid) {
    const char *current_bssid = nm_access_point_get_bssid(ap);
    active = (current_bssid && is_str_same (current_bssid, info->active_bssid));
  }
  /* Get AP properties */
  flags     = nm_access_point_get_flags(ap);
  ssid      = nm_access_point_get_ssid(ap);
  mode      = nm_access_point_get_mode(ap);
  wpa_flags = nm_access_point_get_wpa_flags(ap);
  rsn_flags = nm_access_point_get_rsn_flags(ap);
  /* Convert to strings */
  ssid_str    = ssid_to_printable((const char *) ssid->data, ssid->len);
  freq_str    = g_strdup_printf(_("%u MHz"), nm_access_point_get_frequency(ap));
  bitrate_str = g_strdup_printf(_("%u MB/s"),
                                nm_access_point_get_max_bitrate(ap) / 1000);
  strength_str  = g_strdup_printf("%u", nm_access_point_get_strength(ap));
  wpa_flags_str = ap_wpa_rsn_flags_to_string(wpa_flags);
  rsn_flags_str = ap_wpa_rsn_flags_to_string(rsn_flags);

  security_str = g_string_new(nullptr);

  if (!(flags & NM_802_11_AP_FLAGS_PRIVACY)
      && (wpa_flags != NM_802_11_AP_SEC_NONE)
      && (rsn_flags != NM_802_11_AP_SEC_NONE)) {
    g_string_append(security_str, _("Encrypted: "));
  }

  if ((flags & NM_802_11_AP_FLAGS_PRIVACY) &&
      (wpa_flags == NM_802_11_AP_SEC_NONE) &&
      (rsn_flags == NM_802_11_AP_SEC_NONE)) {
    g_string_append(security_str, _("WEP "));
  }

  if (wpa_flags != NM_802_11_AP_SEC_NONE) {
    g_string_append(security_str, _("WPA "));
  }

  if (rsn_flags != NM_802_11_AP_SEC_NONE) {
    g_string_append(security_str, _("WPA2 "));
  }

  if ((wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X) ||
      (rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)) {
    g_string_append(security_str, _("Enterprise "));
  }

  if (security_str->len > 0) {
    /* Chop off last space */
    g_string_truncate(security_str, security_str->len - 1);
  }

  info->ap_name  = g_strdup_printf("AP%d", info->index ++); /* AP */
  info->ssid_str = ssid_str;
  info->bssid    = g_strdup(nm_access_point_get_bssid(ap));
  info->mode     = g_strdup((mode == NM_802_11_MODE_ADHOC) ? _("Ad-Hoc") :
      ((mode == NM_802_11_MODE_INFRA) ? _("Infrastructure") : _("Unknown")));
  info->freq_str      = freq_str;
  info->bitrate_str   = bitrate_str;
  info->strength_str  = strength_str;
  info->security_str  = g_string_free(security_str, FALSE);
  info->wpa_flags_str = wpa_flags_str;
  info->rsn_flags_str = rsn_flags_str;
  info->active        = g_strdup(active ? _("yes") : _("no"));
  info->path          = g_strdup(nm_object_get_path(NM_OBJECT (ap)));
}

char *AMNetworkManager_priv::ap_wpa_rsn_flags_to_string(
    NM80211ApSecurityFlags flags)
{
  char *flags_str[16]; /* Enough space for flags and terminating NULL */
  char *ret_str;
  int i = 0;

  if (flags & NM_802_11_AP_SEC_PAIR_WEP40) {
    flags_str[i ++] = g_strdup("pair_wpe40");
  }
  if (flags & NM_802_11_AP_SEC_PAIR_WEP104) {
    flags_str[i ++] = g_strdup("pair_wpe104");
  }
  if (flags & NM_802_11_AP_SEC_PAIR_TKIP) {
    flags_str[i ++] = g_strdup("pair_tkip");
  }
  if (flags & NM_802_11_AP_SEC_PAIR_CCMP) {
    flags_str[i ++] = g_strdup("pair_ccmp");
  }
  if (flags & NM_802_11_AP_SEC_GROUP_WEP40) {
    flags_str[i ++] = g_strdup("group_wpe40");
  }
  if (flags & NM_802_11_AP_SEC_GROUP_WEP104) {
    flags_str[i ++] = g_strdup("group_wpe104");
  }
  if (flags & NM_802_11_AP_SEC_GROUP_TKIP) {
    flags_str[i ++] = g_strdup("group_tkip");
  }
  if (flags & NM_802_11_AP_SEC_GROUP_CCMP) {
    flags_str[i ++] = g_strdup("group_ccmp");
  }
  if (flags & NM_802_11_AP_SEC_KEY_MGMT_PSK) {
    flags_str[i ++] = g_strdup("psk");
  }
  if (flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X) {
    flags_str[i ++] = g_strdup("802.1X");
  }

  if (i == 0) {
    flags_str[i ++] = g_strdup(_("(none)"));
  }

  flags_str[i] = nullptr;

  ret_str = g_strjoinv(" ", flags_str);

  i = 0;
  while (flags_str[i]) {
    g_free(flags_str[i ++]);
  }

  return ret_str;
}

char *AMNetworkManager_priv::ssid_to_printable(const char *str, gsize len)
{
  GString *printable;
  char *printable_str;

  if ((str == nullptr) || (len == 0)) {
    return nullptr;
  }

  if (g_utf8_validate(str, len, nullptr)) {
    return g_strdup_printf("%.*s", (int) len, str);
  }

  printable = g_string_new(nullptr);
  for (uint32_t i = 0; i < len; i ++) {
    g_string_append_printf(printable, "%02X", (unsigned char) str[i]);
  }
  printable_str = g_string_free(printable, FALSE);
  return printable_str;
}

NMDevice *
AMNetworkManager_priv::find_wifi_device_by_iface(const GPtrArray *devices,
                                                 const char *iface,
                                                 int *idx)
{
  NMDevice *device = nullptr;
  unsigned int i;

  for (i = *idx; devices && (i < devices->len); i ++) {
    device = (NMDevice *) g_ptr_array_index (devices, i);
    if (AM_LIKELY(NM_IS_DEVICE_WIFI (device))) {
      if (iface) {
        /* If a iface was specified then use it. */
        if (is_str_same (nm_device_get_iface (device), iface) ) {
          break;
        }
      } else {
        /* Else return the first Wi-Fi device. */
        break;
      }
    } else {
      device = nullptr;
    }
  }

  *idx = i + 1;
  return device;
}

NMAccessPoint * AMNetworkManager_priv::find_ap_on_device(NMDevice   *device,
                                                         const char *ssid)
{
  NMAccessPoint   *ap = nullptr;
  if (AM_LIKELY(NM_IS_DEVICE_WIFI(device) && ssid)) {
    const GPtrArray *aps =
        nm_device_wifi_get_access_points(NM_DEVICE_WIFI(device));
    for (guint i = 0; aps && (i < aps->len); ++ i) {
      GByteArray *bssid = nm_utils_hwaddr_atoba(ssid, ARPHRD_ETHER);
      ap = (NMAccessPoint *)g_ptr_array_index (aps, i);
      /* Compare BSSIDs */
      if (bssid) {
        if (is_str_same (nm_utils_hwaddr_ntoa(bssid->data, ARPHRD_ETHER),
                         nm_access_point_get_bssid(ap))) {
          break;
        } else {
          ap = nullptr;
        }
      } else {
        const GByteArray *id = nm_access_point_get_ssid(ap);
        if (id) {
          char *ssidstr = nm_utils_ssid_to_utf8(id);
          if (ssidstr && !is_str_same(ssidstr, ssid)) {
            ap = nullptr;
          }
          g_free(ssidstr);
          if (ap) {
            break;
          }
        }
      }
    }
  } else {
    if (!NM_IS_DEVICE_WIFI(device)) {
      ERROR("%s doesn't support WiFi!", nm_device_get_iface(device));
    }
    if (!ssid) {
      ERROR("Invalid BSSID or SSID!");
    }
  }

  return ap;
}

static gboolean timeout_cb(gpointer data)
{
  NOTICE("Timeout!");
  ((NmCbData*)data)->isdone = true;
  return FALSE;
}

static void monitor_device_state_cb(NMDevice   *dev,
                                    GParamSpec *pspec,
                                    gpointer    data)
{
  NmCbData *info = (NmCbData*)data;
  NMDeviceState state;
  NMDeviceStateReason reason;
  state = nm_device_get_state_reason(dev, &reason);
  if (state == NM_DEVICE_STATE_ACTIVATED) {
    info->retval = 0;
    info->isdone = true;
  } else if (state == NM_DEVICE_STATE_FAILED) {
    info->isdone = true;
  }
}

static void device_state_cb(NMDevice   *dev,
                            GParamSpec *pspec,
                            gpointer    data)
{
  if (NM_DEVICE_STATE_DISCONNECTED == nm_device_get_state(dev)) {
    ((NmCbData*)data)->retval = 0;
    ((NmCbData*)data)->isdone = true;
  }
}

static void added_cb (NMRemoteSettings *settings,
          NMRemoteConnection *remote,
          GError *error,
          gpointer user_data)
{
  NmCbData *info = (NmCbData*) user_data;
  if (error) {
    info->retval = error->code;
    ERROR ("Error adding connection: (%d) %s", error->code, error->message);
  }else {
    info->retval = 0;
    INFO ("Added: %s\n", nm_connection_get_path (NM_CONNECTION (remote)));
  }

  info->isdone = true;
}

static void add_and_activate_cb(NMClient *client,
                                NMActiveConnection *active,
                                const char *connection_path,
                                GError *error,
                                gpointer user_data)
{
  NmCbData *info = (NmCbData*) user_data;
  NMDevice *device = info->device;

  if (!error) {
    switch(nm_active_connection_get_state(active)) {
      case NM_ACTIVE_CONNECTION_STATE_UNKNOWN: {
        info->isdone = true;
      }break;
      case NM_ACTIVE_CONNECTION_STATE_ACTIVATED: {
        info->retval = 0;
        info->isdone = true;
      }break;
      default: {
        g_signal_connect(device, "notify::state",
                         G_CALLBACK(monitor_device_state_cb), info);
        g_timeout_add_seconds(info->timeout, timeout_cb, info);
      }break;
    }
  } else {
    ERROR("Error: (%d) %s", error->code, error->message);
    info->retval = error->code;
    info->isdone = true;
  }
}

static void activate_cb(NMClient *client,
                        NMActiveConnection *active,
                        GError *error,
                        gpointer user_data)
{
  add_and_activate_cb(client, active, nullptr, error, user_data);
}

static void disconnect_device_cb(NMDevice *device,
                                 GError   *error,
                                 gpointer user_data)
{
  NmCbData *info = (NmCbData*)user_data;

  if (!error) {
    switch(nm_device_get_state(device)) {
      case NM_DEVICE_STATE_DISCONNECTED: {
        info->isdone = true;
        info->retval = 0;
        INFO("%s is disconnected sucessfully!", nm_device_get_iface(device));
      }break;
      default: {
        g_signal_connect(device, "notify::state",
                         G_CALLBACK(device_state_cb), info);
        g_timeout_add_seconds(info->timeout, timeout_cb, info);
      }break;
    }
  } else {
    ERROR("Error: (%d) %s", error->code, error->message);
    info->retval = error->code;
    info->isdone = true;
  }
}

static void connection_commit_cb(NMRemoteConnection *connection,
                                 GError             *error,
                                 gpointer           user_data)
{
  if (AM_UNLIKELY(error)) {
    ERROR("(%u) %s", error->code, error->message);
  }
}

static void connection_removed_cb (NMRemoteConnection *con, gpointer user_data)
{
  INFO("connection remove OK");
}

static void delete_cb (NMRemoteConnection *con,
                       GError *error,
                       gpointer user_data)
{
  NmCbData *info = (NmCbData*)user_data;

  if (error) {
    info->retval = error->code;
    ERROR("delete Error");
  }else {
    info->retval = 0;
    INFO("delete OK");
  }
  info->isdone = true;
}

int AMNetworkManager_priv::wifi_list_ap(APInfo **infos)
{
  const GPtrArray *devices;
  unsigned int i;
  int num = 0;
  if (AM_LIKELY(init())) {
    devices = nm_client_get_devices(mNmClient);
    for (i = 0; devices && (i < devices->len); i ++) {
      NMDevice *dev = NM_DEVICE(g_ptr_array_index (devices, i));
      if ( dev && NM_IS_DEVICE_WIFI (dev)) {
        num += show_access_point_info(dev, infos);
      }
    }
  }
  return num;
}

int AMNetworkManager_priv::wifi_connect_ap(const char  *ap_name,
                                           const char  *password,
                                           NMWepKeyType wepKeyType,
                                           uint32_t     timeout,
                                           NetInfoIPv4 *ipv4,
                                           NetInfoIPv6 *ipv6)
{
  NmCbData info;

  info.device = nullptr;
  info.retval = -1;
  info.isdone = false;
  info.timeout = timeout;

  if (AM_LIKELY(init())) {
    if (AM_LIKELY(ap_name)) {
      const GPtrArray *devList = nm_client_get_devices(mNmClient);
      NMDevice        *wifiDev = nullptr;
      NMAccessPoint        *ap = nullptr;

      DEBUG("AP=%s, Password=%s\n", ap_name, password);

      for (guint i = 0; devList && (i < devList->len); ++ i) {
        wifiDev = (NMDevice*)g_ptr_array_index(devList, i);
        if (wifiDev && NM_IS_DEVICE_WIFI(wifiDev)){
          if ( nullptr != (ap = find_ap_on_device(wifiDev, ap_name))) {
            break;
          } else {
            wifiDev = nullptr;
          }
        }
      }

      if (wifiDev && ap ) {
        NMConnection *connection = nullptr;
        bool needCreateNew = true;
        for (guint i = 0; i < g_slist_length(mSysConnections); ++ i) {
          connection = (NMConnection*)g_slist_nth_data(mSysConnections, i);
          if (AM_LIKELY(connection)) {
            if (is_str_same(ap_name, nm_connection_get_id(connection))) {
              needCreateNew = false;
              break;
            }
          }
        }
        if (needCreateNew) {
          connection = nm_connection_new();
        }

        if (password) {
          NM80211ApFlags flags = nm_access_point_get_flags(ap);
          NM80211ApSecurityFlags wpa_flags = nm_access_point_get_wpa_flags(ap);
          NM80211ApSecurityFlags rsn_flags = nm_access_point_get_rsn_flags(ap);
          NMSettingWirelessSecurity *s_wsec =
              (NMSettingWirelessSecurity *)nm_setting_wireless_security_new();

          nm_connection_add_setting(connection, NM_SETTING(s_wsec));

          /* Set password for WEP or WPA-PSK. */
          if (flags & NM_802_11_AP_FLAGS_PRIVACY) {
            if (wpa_flags == NM_802_11_AP_SEC_NONE
                && rsn_flags == NM_802_11_AP_SEC_NONE) {
              /* WEP */
              nm_setting_wireless_security_set_wep_key(s_wsec, 0, password);
              g_object_set(G_OBJECT (s_wsec),
                           NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE,
                           wepKeyType,
                           nullptr);
            } else if (!(wpa_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)
                && !(rsn_flags & NM_802_11_AP_SEC_KEY_MGMT_802_1X)) {
              /* WPA PSK */
              g_object_set(s_wsec, NM_SETTING_WIRELESS_SECURITY_PSK,
                           password, nullptr);
            }
          }
        } else {
          nm_connection_remove_setting(
              connection, nm_setting_wireless_security_get_type());
        }

        if (ipv4 && ipv4->get_address()) {
          NMIP4Address *addr = nm_ip4_address_new();
          NMSettingIP4Config *ip4 =
              NM_SETTING_IP4_CONFIG(nm_setting_ip4_config_new());
          g_object_set (G_OBJECT (ip4),
                    NM_SETTING_IP4_CONFIG_METHOD,
                    NM_SETTING_IP4_CONFIG_METHOD_MANUAL,
                    nullptr);
          nm_ip4_address_set_address(addr, ipv4->get_address());
          if (ipv4->get_prefix()) {
            nm_ip4_address_set_prefix(addr, ipv4->get_prefix());
          }
          if (ipv4->get_gateway()) {
            nm_ip4_address_set_gateway(addr, ipv4->get_gateway());
          }
          if (!nm_setting_ip4_config_add_address(ip4, addr)) {
            ERROR("Failed to add IPv4 setting!");
            g_object_unref(ip4);
          } else {
            for (DnsIPv4 *dns = ipv4->get_dns(); dns; dns = dns->dns_next) {
              if (!nm_setting_ip4_config_add_dns(ip4, dns->get_dns())) {
                ERROR("Failed to add DNS setting!");
                g_object_unref(ip4);
                ip4 = nullptr;
                break;
              }
            }
            if (AM_LIKELY(ip4)) {
              nm_connection_add_setting(connection, NM_SETTING(ip4));
            }
          }
          nm_ip4_address_unref(addr);
        } else {
          nm_connection_remove_setting(connection,
                                       nm_setting_ip4_config_get_type());
        }

        if (ipv6 && ipv6->get_address()) {
          NMIP6Address *addr = nm_ip6_address_new();
          NMSettingIP6Config *ip6 =
              NM_SETTING_IP6_CONFIG(nm_setting_ip6_config_new());
          g_object_set (G_OBJECT (ip6),
                    NM_SETTING_IP6_CONFIG_METHOD,
                    NM_SETTING_IP6_CONFIG_METHOD_MANUAL,
                    nullptr);
          nm_ip6_address_set_address(addr, ipv6->get_address());
          if (ipv6->get_prefix()) {
            nm_ip6_address_set_prefix(addr, ipv6->get_prefix());
          }
          if (ipv6->get_gateway()) {
            nm_ip6_address_set_gateway(addr, ipv6->get_gateway());
          }
          if (!nm_setting_ip6_config_add_address(ip6, addr)) {
            ERROR("Failed to add IPv6 setting!");
            g_object_unref(ip6);
          } else {
            for (DnsIPv6 *dns = ipv6->get_dns(); dns; dns = dns->dns_next) {
              if (!nm_setting_ip6_config_add_dns(ip6, dns->get_dns())) {
                ERROR("Failed to add DNS setting!");
                g_object_unref(ip6);
                ip6 = nullptr;
                break;
              }
            }
            if (AM_LIKELY(ip6)) {
              nm_connection_add_setting(connection, NM_SETTING(ip6));
            }
          }
          nm_ip6_address_unref(addr);
        } else {
          nm_connection_remove_setting(connection,
                                       nm_setting_ip6_config_get_type());
        }

        info.device = wifiDev;
        if (needCreateNew) {
          nm_client_add_and_activate_connection(
              mNmClient,
              connection,
              wifiDev,
              nm_object_get_path(NM_OBJECT(ap)),
              add_and_activate_cb,
              &info);
        } else {
          nm_remote_connection_commit_changes(NM_REMOTE_CONNECTION(connection),
                                              connection_commit_cb, nullptr);
          nm_client_activate_connection(mNmClient,
                                        connection,
                                        wifiDev,
                                        nm_object_get_path(NM_OBJECT(ap)),
                                        activate_cb,
                                        &info);
        }
        while (!info.isdone) {
          g_main_context_iteration(g_main_loop_get_context(mLoop), FALSE);
          usleep(50000);
        }

        if (needCreateNew) {
          g_object_unref(connection);
        }
      } else {
        ERROR("Cannot find %s on any WiFi devices!", ap_name);
      }
    } else {
      ERROR("Please input valid AP!");
    }
  }

  return info.retval;
}

int AMNetworkManager_priv::wifi_connect_ap_static(const char  *ap_name,
                                                  const char  *password,
                                                  NetInfoIPv4 *ipv4,
                                                  NetInfoIPv6 *ipv6)
{
  return wifi_connect_ap(ap_name, password,NM_WEP_KEY_TYPE_KEY, 90, ipv4, ipv6);
}

int AMNetworkManager_priv::wifi_connect_hidden_ap(
    const char  *ap_name,
    const char  *password,
    const AMNetworkManager::APSecurityFlags security,
    uint32_t wep_index,
    uint32_t     timeout,
    NetInfoIPv4 *ipv4,
    NetInfoIPv6 *ipv6)
{
  NmCbData info;
  if (AM_LIKELY(init())) {
    if (AM_LIKELY(ap_name)) {
      NMConnection *connection = nullptr;
      NMSettingConnection *s_con = nullptr;
      NMSettingWireless *s_wifi = nullptr;

      DEBUG("Hidden AP=%s, Password=%s\n", ap_name, password);

      bool needCreateNew = true;
      for (guint i = 0; i < g_slist_length(mSysConnections); ++ i) {
        connection = (NMConnection*)g_slist_nth_data(mSysConnections, i);
        if (AM_LIKELY(connection)) {
          if (is_str_same(ap_name, nm_connection_get_id(connection))) {
            needCreateNew = false;
            break;
          }
        }
      }
      if (needCreateNew) {
        connection = nm_connection_new();
        s_con = (NMSettingConnection *) nm_setting_connection_new ();
        char *uuid  = nm_utils_uuid_generate ();
        g_object_set (G_OBJECT (s_con),
                NM_SETTING_CONNECTION_UUID, uuid,
                NM_SETTING_CONNECTION_ID, ap_name,
                NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                nullptr);
        g_free (uuid);
        nm_connection_add_setting (connection, NM_SETTING (s_con));

        s_wifi = (NMSettingWireless *) nm_setting_wireless_new ();

        guint32 ssid_len = strlen (ap_name);
        GByteArray *ssid_ba = g_byte_array_sized_new (ssid_len);
        g_byte_array_append (ssid_ba, (unsigned char *) ap_name, ssid_len);
        g_object_set(s_wifi,
                NM_SETTING_WIRELESS_SSID, ssid_ba,
                NM_SETTING_WIRELESS_SEC,
                NM_SETTING_WIRELESS_SECURITY_SETTING_NAME,
                nullptr);
        g_byte_array_free(ssid_ba, TRUE);
        nm_connection_add_setting (connection, NM_SETTING (s_wifi));

        switch (security) {
        case NM_802_11_AP_SEC_NONE : {
          g_object_set (s_wifi, NM_SETTING_WIRELESS_SEC, nullptr, nullptr);
        }break;
        case NM_802_11_AP_SEC_PAIR_WEP40 : {
          if ( password ) {
            NMSettingWirelessSecurity *s_wsec =
            (NMSettingWirelessSecurity *)nm_setting_wireless_security_new();
            nm_connection_add_setting(connection, NM_SETTING(s_wsec));
            if (wep_index == 1) {
              g_object_set(s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY1, password,
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 1,
                            nullptr);
            }else if (wep_index == 2) {
              g_object_set(s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY2, password,
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 1,
                            nullptr);
            }else if (wep_index == 3) {
              g_object_set(s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY3, password,
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 1,
                            nullptr);
            }else {
              g_object_set(s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY0, password,
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 1,
                            nullptr);
            }
          }
        }break;
        case NM_802_11_AP_SEC_PAIR_WEP104:{
          if ( password ) {
            NMSettingWirelessSecurity *s_wsec =
            (NMSettingWirelessSecurity *)nm_setting_wireless_security_new();
            nm_connection_add_setting(connection, NM_SETTING(s_wsec));
            if (wep_index == 1) {
              g_object_set(s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY1, password,
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 2,
                            nullptr);
            }else if (wep_index == 2) {
              g_object_set(s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY2, password,
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 2,
                            nullptr);
            }else if (wep_index == 3) {
              g_object_set(s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY3, password,
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 2,
                            nullptr);
            }else{
              g_object_set(s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY0, password,
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 2,
                            nullptr);
            }
          }
        }break;
        case NM_802_11_AP_SEC_PAIR_TKIP :
        case NM_802_11_AP_SEC_PAIR_CCMP:{
          if ( password ) {
            NMSettingWirelessSecurity *s_wsec =
            (NMSettingWirelessSecurity *)nm_setting_wireless_security_new();
            nm_connection_add_setting(connection, NM_SETTING(s_wsec));
            g_object_set(s_wsec,
                        NM_SETTING_WIRELESS_SECURITY_KEY_MGMT,"wpa-psk",
                        NM_SETTING_WIRELESS_SECURITY_PSK, password,
                        nullptr);
          }
        }break;
        default:
          ERROR("Not support this security yet");
        break;
        }

        if (ipv4 && ipv4->get_address()) {
          NMIP4Address *addr = nm_ip4_address_new();
          NMSettingIP4Config *ip4 =
          NM_SETTING_IP4_CONFIG(nm_setting_ip4_config_new());
          g_object_set (G_OBJECT (ip4),
                    NM_SETTING_IP4_CONFIG_METHOD,
                    NM_SETTING_IP4_CONFIG_METHOD_MANUAL,
                    nullptr);
          nm_ip4_address_set_address(addr, ipv4->get_address());
          if (ipv4->get_gateway()) {
            nm_ip4_address_set_gateway(addr, ipv4->get_gateway());
          }
          if (!nm_setting_ip4_config_add_address(ip4, addr)) {
            ERROR("Failed to add IPv4 setting!");
            g_object_unref(ip4);
          } else {
            for (DnsIPv4 *dns = ipv4->get_dns(); dns; dns = dns->dns_next) {
              if (!nm_setting_ip4_config_add_dns(ip4, dns->get_dns())) {
                ERROR("Failed to add DNS setting!");
                g_object_unref(ip4);
                ip4 = nullptr;
                break;
              }
            }
            if (AM_LIKELY(ip4)) {
              nm_connection_add_setting(connection, NM_SETTING(ip4));
            }
          }
          nm_ip4_address_unref(addr);
        } else {
          NMSettingIP4Config *s_ip4 = (NMSettingIP4Config *)
              nm_setting_ip4_config_new ();
          g_object_set (G_OBJECT (s_ip4),
                    NM_SETTING_IP4_CONFIG_METHOD,
                    NM_SETTING_IP4_CONFIG_METHOD_AUTO,
                    nullptr);
          nm_connection_add_setting (connection, NM_SETTING (s_ip4));
        }

        if (ipv6 && ipv6->get_address()) {
          NMIP6Address *addr = nm_ip6_address_new();
          NMSettingIP6Config *ip6 =
          NM_SETTING_IP6_CONFIG(nm_setting_ip6_config_new());
          g_object_set (G_OBJECT (ip6),
                    NM_SETTING_IP6_CONFIG_METHOD,
                    NM_SETTING_IP6_CONFIG_METHOD_MANUAL,
                    nullptr);
          nm_ip6_address_set_address(addr, ipv6->get_address());
          if (ipv6->get_prefix()) {
            nm_ip6_address_set_prefix(addr, ipv6->get_prefix());
          }
          if (ipv6->get_gateway()) {
            nm_ip6_address_set_gateway(addr, ipv6->get_gateway());
          }
          if (!nm_setting_ip6_config_add_address(ip6, addr)) {
            ERROR("Failed to add IPv6 setting!");
            g_object_unref(ip6);
          } else {
          for (DnsIPv6 *dns = ipv6->get_dns(); dns; dns = dns->dns_next) {
          if (!nm_setting_ip6_config_add_dns(ip6, dns->get_dns())) {
            ERROR("Failed to add DNS setting!");
                  g_object_unref(ip6);
            ip6 = nullptr;
            break;
            }
          }
          if (AM_LIKELY(ip6)) {
            nm_connection_add_setting(connection, NM_SETTING(ip6));
          }
        }
        nm_ip6_address_unref(addr);
        } else {
          NMSettingIP6Config *s_ip6 = (NMSettingIP6Config *)
              nm_setting_ip6_config_new ();
          g_object_set (G_OBJECT (s_ip6),
                    NM_SETTING_IP6_CONFIG_METHOD,
                    NM_SETTING_IP6_CONFIG_METHOD_AUTO,
                    nullptr);
          nm_connection_add_setting (connection, NM_SETTING (s_ip6));
        }
      }
      NMDevice        *wifiDev = nullptr;
      NMAccessPoint *ap = nullptr;

      const GPtrArray *devList = nm_client_get_devices(mNmClient);
      for (guint i = 0; devList && (i < devList->len); ++ i) {
        NMDevice *dev = (NMDevice*)g_ptr_array_index(devList, i);
        if ( dev && NM_IS_DEVICE_WIFI(dev) ) {
          wifiDev = dev;
          break;
        }
      }

      info.device = wifiDev;
      info.retval = -1;
      info.isdone = false;
      info.timeout = timeout;

      if ( needCreateNew ) {
        if (connection && wifiDev ) {
           DEBUG("Using Device [%s].", nm_device_get_iface(wifiDev));
           nm_client_add_and_activate_connection (mNmClient,
                      connection,
                      wifiDev,
                      nullptr,
                      add_and_activate_cb,
                      &info);
           while (!info.isdone) {
             g_main_context_iteration(g_main_loop_get_context(mLoop), FALSE);
             usleep(50000);
           }
           g_object_unref(connection);
         }
       }else {
         if ( connection && wifiDev ) {
           DEBUG("Using Device [%s].", nm_device_get_iface(wifiDev));
           nm_client_activate_connection (mNmClient,
                      connection,
                      wifiDev,
                      ap ? nm_object_get_path (NM_OBJECT (ap)) : nullptr,
                      activate_cb,
                      &info);
             while (!info.isdone) {
               g_main_context_iteration(g_main_loop_get_context(mLoop), FALSE);
               usleep(50000);
             }
           }
         }
     }
   }
  return info.retval;
}

int AMNetworkManager_priv::wifi_connect_hidden_ap_static(
    const char  *ap_name,
    const char  *password,
    const AMNetworkManager::APSecurityFlags security,
    uint32_t wep_index,
    NetInfoIPv4 *ipv4,
    NetInfoIPv6 *ipv6)
{
  return wifi_connect_hidden_ap(ap_name, password,
                                security, wep_index,
                                90, ipv4, ipv6);
}

int AMNetworkManager_priv::wifi_setup_adhoc(
    const char *name,
    const char*password,
    const AMNetworkManager::APSecurityFlags security)
{
  NmCbData info;
  if (AM_LIKELY(init())) {
    if (AM_LIKELY(name)) {
      NMConnection *connection = nullptr;

      DEBUG("Adhoc=%s, Password=%s\n", name, password);

      bool needCreateNew = true;
      for (guint i = 0; i < g_slist_length(mSysConnections); ++ i) {
        connection = (NMConnection*)g_slist_nth_data(mSysConnections, i);
        if (AM_LIKELY(connection)) {
          if (is_str_same(name, nm_connection_get_id(connection))) {
            needCreateNew = false;
            break;
          }
        }
      }
      if (needCreateNew) {
        NMSettingConnection *s_con = nullptr;
        NMSettingWireless *s_wifi = nullptr;
        NMSettingIP4Config *s_ip4 = nullptr;
        NMSettingIP6Config *s_ip6 = nullptr;

        connection = nm_connection_new();
        s_con = (NMSettingConnection *) nm_setting_connection_new ();
        char *uuid  = nm_utils_uuid_generate ();
        g_object_set (G_OBJECT (s_con),
                NM_SETTING_CONNECTION_UUID, uuid,
                NM_SETTING_CONNECTION_ID, name,
                NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                nullptr);
        g_free (uuid);
        nm_connection_add_setting (connection, NM_SETTING (s_con));

        s_wifi = (NMSettingWireless *) nm_setting_wireless_new ();
        guint32 ssid_len = strlen (name);
        GByteArray *ssid_ba = g_byte_array_sized_new (ssid_len);
        g_byte_array_append (ssid_ba, (unsigned char *) name, ssid_len);
        g_object_set(s_wifi,
                NM_SETTING_WIRELESS_SSID, ssid_ba,
                NM_SETTING_WIRELESS_SEC,
                NM_SETTING_WIRELESS_SECURITY_SETTING_NAME,
                NM_SETTING_WIRELESS_MODE, NM_SETTING_WIRELESS_MODE_ADHOC,
                nullptr);
        g_byte_array_free(ssid_ba, TRUE);
        nm_connection_add_setting (connection, NM_SETTING (s_wifi));
        switch (security) {
          case NM_802_11_AP_SEC_NONE : {
            g_object_set (s_wifi, NM_SETTING_WIRELESS_SEC, nullptr, nullptr);
          }break;
          case NM_802_11_AP_SEC_PAIR_WEP40 : {
            if (password) {
              NMSettingWirelessSecurity *s_wsec = (NMSettingWirelessSecurity*)
                  nm_setting_wireless_security_new();
              nm_connection_add_setting(connection, NM_SETTING(s_wsec));
              g_object_set (s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                            NM_SETTING_WIRELESS_SECURITY_AUTH_ALG, "open",
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 1,
                            nullptr);
              nm_setting_wireless_security_set_wep_key (s_wsec, 0, password);
            }
          }break;
          case NM_802_11_AP_SEC_PAIR_WEP104 : {
            if (password) {
              NMSettingWirelessSecurity *s_wsec = (NMSettingWirelessSecurity*)
                  nm_setting_wireless_security_new ();
              nm_connection_add_setting(connection, NM_SETTING(s_wsec));
              g_object_set (s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                            NM_SETTING_WIRELESS_SECURITY_AUTH_ALG, "open",
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 2,
                            nullptr);
              nm_setting_wireless_security_set_wep_key (s_wsec, 0, password);
            }
          }break;
          case NM_802_11_AP_SEC_PAIR_CCMP : {
            if (password) {
              NMSettingWirelessSecurity *s_wsec = (NMSettingWirelessSecurity*)
                  nm_setting_wireless_security_new ();
              nm_connection_add_setting(connection, NM_SETTING(s_wsec));
              g_object_set(s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT,"wpa-psk",
                            NM_SETTING_WIRELESS_SECURITY_PSK, password,
                            nullptr);
            }
          }break;
          default: {
            ERROR("Not support this security yet");
            break;
          }
        }

        s_ip4 = (NMSettingIP4Config *) nm_setting_ip4_config_new ();
        g_object_set (G_OBJECT (s_ip4),
                NM_SETTING_IP4_CONFIG_METHOD,
                NM_SETTING_IP4_CONFIG_METHOD_SHARED,
                nullptr);
        nm_connection_add_setting (connection, NM_SETTING (s_ip4));

        s_ip6 = (NMSettingIP6Config *) nm_setting_ip6_config_new ();
        g_object_set (s_ip6,
                NM_SETTING_IP6_CONFIG_METHOD,
                NM_SETTING_IP6_CONFIG_METHOD_IGNORE,
                NM_SETTING_IP6_CONFIG_MAY_FAIL, TRUE,
                nullptr);
        nm_connection_add_setting (connection, NM_SETTING (s_ip6));

        if (nm_remote_settings_add_connection (mSysSettings, connection,
                                               added_cb, &info)) {
          while (!info.isdone) {
            g_main_context_iteration(g_main_loop_get_context(mLoop), FALSE);
            usleep(50000);
          }
        }else {
          ERROR("Error adding Adhoc connection");
        }
        g_object_unref (connection);
      }else {
        INFO("Adhoc '%s' exist, use the previous one", name);
        info.retval = wifi_connection_up(name);
      }
    }
  }
  return info.retval;
}

int AMNetworkManager_priv::wifi_setup_ap(const char *name, const char*password,
  const AMNetworkManager::APSecurityFlags security, int channel)
{
  NmCbData info;
  if (AM_LIKELY(init())) {
    if (AM_LIKELY(name)) {
      NMConnection *connection = nullptr;

      DEBUG("AP=%s, Password=%s\n", name, password);

      bool needCreateNew = true;
      for (guint i = 0; i < g_slist_length(mSysConnections); ++ i) {
        connection = (NMConnection*)g_slist_nth_data(mSysConnections, i);
        if (AM_LIKELY(connection)) {
          if (is_str_same(name, nm_connection_get_id(connection))) {
            needCreateNew = false;
            break;
          }
        }
      }
      if (needCreateNew) {
        NMSettingConnection *s_con = nullptr;
        NMSettingWireless *s_wifi = nullptr;
        NMSettingIP4Config *s_ip4 = nullptr;
        NMSettingIP6Config *s_ip6 = nullptr;

        connection = nm_connection_new();
        s_con = (NMSettingConnection *) nm_setting_connection_new ();
        char *uuid  = nm_utils_uuid_generate ();
        g_object_set (G_OBJECT (s_con),
                NM_SETTING_CONNECTION_UUID, uuid,
                NM_SETTING_CONNECTION_ID, name,
                NM_SETTING_CONNECTION_TYPE, NM_SETTING_WIRELESS_SETTING_NAME,
                nullptr);
        g_free (uuid);
        nm_connection_add_setting (connection, NM_SETTING (s_con));

        s_wifi = (NMSettingWireless *) nm_setting_wireless_new ();
        guint32 ssid_len = strlen (name);
        GByteArray *ssid_ba = g_byte_array_sized_new (ssid_len);
        g_byte_array_append (ssid_ba, (unsigned char *) name, ssid_len);
        g_object_set(s_wifi,
                NM_SETTING_WIRELESS_SSID, ssid_ba,
                NM_SETTING_WIRELESS_SEC,
                NM_SETTING_WIRELESS_SECURITY_SETTING_NAME,
                NM_SETTING_WIRELESS_MODE, NM_SETTING_WIRELESS_MODE_AP,
                nullptr);
        if ((channel > 0) && (channel < 15)) {
          g_object_set(s_wifi,
                NM_SETTING_WIRELESS_BAND, "bg",
                NM_SETTING_WIRELESS_CHANNEL, channel,
                nullptr);
        }
        g_byte_array_free(ssid_ba, TRUE);
        nm_connection_add_setting (connection, NM_SETTING (s_wifi));

        switch (security) {
          case NM_802_11_AP_SEC_NONE : {
            g_object_set (s_wifi, NM_SETTING_WIRELESS_SEC, nullptr, nullptr);
          }break;
          case NM_802_11_AP_SEC_PAIR_WEP40 : {
            if (password) {
              NMSettingWirelessSecurity *s_wsec = (NMSettingWirelessSecurity*)
                  nm_setting_wireless_security_new ();
              nm_connection_add_setting(connection, NM_SETTING(s_wsec));
              g_object_set (s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                            NM_SETTING_WIRELESS_SECURITY_AUTH_ALG, "open",
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 1,
                            nullptr);
              nm_setting_wireless_security_set_wep_key (s_wsec, 0, password);
            }
          }break;
          case NM_802_11_AP_SEC_PAIR_WEP104 : {
            if (password) {
              NMSettingWirelessSecurity *s_wsec = (NMSettingWirelessSecurity*)
                  nm_setting_wireless_security_new ();
              nm_connection_add_setting(connection, NM_SETTING(s_wsec));
              g_object_set (s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT, "none",
                            NM_SETTING_WIRELESS_SECURITY_AUTH_ALG, "open",
                            NM_SETTING_WIRELESS_SECURITY_WEP_KEY_TYPE, 2,
                            nullptr);
              nm_setting_wireless_security_set_wep_key (s_wsec, 0, password);
            }
          }break;
          case NM_802_11_AP_SEC_PAIR_CCMP : {
            if (password) {
              NMSettingWirelessSecurity *s_wsec = (NMSettingWirelessSecurity*)
                  nm_setting_wireless_security_new ();
              nm_connection_add_setting(connection, NM_SETTING(s_wsec));
              g_object_set(s_wsec,
                            NM_SETTING_WIRELESS_SECURITY_KEY_MGMT,"wpa-psk",
                            NM_SETTING_WIRELESS_SECURITY_PSK, password,
                            nullptr);
            }
          }break;
          default: {
            ERROR("Not support this security yet");
            break;
          }
        }

        s_ip4 = (NMSettingIP4Config *) nm_setting_ip4_config_new ();
        g_object_set (G_OBJECT (s_ip4),
                NM_SETTING_IP4_CONFIG_METHOD,
                NM_SETTING_IP4_CONFIG_METHOD_SHARED,
                nullptr);
        nm_connection_add_setting (connection, NM_SETTING (s_ip4));

        s_ip6 = (NMSettingIP6Config *) nm_setting_ip6_config_new ();
        g_object_set (s_ip6,
                NM_SETTING_IP6_CONFIG_METHOD,
                NM_SETTING_IP6_CONFIG_METHOD_IGNORE,
                NM_SETTING_IP6_CONFIG_MAY_FAIL, TRUE,
                nullptr);
        nm_connection_add_setting (connection, NM_SETTING (s_ip6));

        if (nm_remote_settings_add_connection (mSysSettings, connection,
                                               added_cb, &info)) {
          while (!info.isdone) {
            g_main_context_iteration(g_main_loop_get_context(mLoop), FALSE);
            usleep(50000);
          }
        }else {
          ERROR("Error adding AP connection");
        }
        g_object_unref (connection);
      }else {
        INFO("AP '%s' exist, use the previous one", name);
        info.retval = wifi_connection_up(name);
      }
    }
  }
  return info.retval;
}

int AMNetworkManager_priv::wifi_connection_up(const char* ap_name,
                                              uint32_t timeout)
{
  NmCbData info;

  if (AM_LIKELY(init())) {
    if (AM_LIKELY(ap_name)) {
      NMConnection *connection = nullptr;
      NMDevice        *wifiDev = nullptr;
      NMAccessPoint        *ap = nullptr;

      for (guint i = 0; i < g_slist_length(mSysConnections); ++ i) {
        NMConnection *conn =
            (NMConnection*)g_slist_nth_data(mSysConnections, i);
        if (AM_LIKELY(conn)) {
          if (is_str_same(ap_name, nm_connection_get_id(conn))) {
            connection = conn;
            break;
          }
        }
      }
      if (AM_LIKELY(connection)) {
        const GPtrArray *devList = nm_client_get_devices(mNmClient);
        for (guint i = 0; devList && (i < devList->len); ++ i) {
          NMDevice *dev = (NMDevice*)g_ptr_array_index(devList, i);
          if ( dev && NM_IS_DEVICE_WIFI(dev)) {
            wifiDev = dev;
            break;
          }
        }

        info.device = wifiDev;
        info.retval = -1;
        info.isdone = false;
        info.timeout = timeout;

        if (wifiDev) {
          DEBUG("Using Device [%s].", nm_device_get_iface(wifiDev));
          ap = find_ap_on_device(wifiDev, ap_name);
          nm_client_activate_connection(mNmClient,
                    connection,
                    wifiDev,
                    ap ? nm_object_get_path(NM_OBJECT(ap)) : nullptr,
                    activate_cb,
                    &info);

          while (!info.isdone) {
            g_main_context_iteration(g_main_loop_get_context(mLoop), FALSE);
            usleep(50000);
          }
        }
      }else {
        ERROR("Not found '%s'", ap_name);
      }
    }
  }
  return info.retval;
}

int AMNetworkManager_priv::wifi_connection_delete(const char* ap_name)
{
  NmCbData info;

  if (AM_LIKELY(init())) {
    if ( AM_LIKELY(ap_name)) {
      NMConnection *connection = nullptr;

      for (guint i = 0; i < g_slist_length(mSysConnections); ++ i) {
        NMConnection *conn =
            (NMConnection*)g_slist_nth_data(mSysConnections, i);
        if (AM_LIKELY(conn)) {
          if (is_str_same(ap_name, nm_connection_get_id(conn))) {
            connection = conn;
            break;
          }
        }
      }
      info.retval = -1;
      info.isdone = false;

      if (connection) {
        g_signal_connect (connection, NM_REMOTE_CONNECTION_REMOVED,
                          G_CALLBACK (connection_removed_cb), nullptr);
        nm_remote_connection_delete (NM_REMOTE_CONNECTION (connection),
                                     delete_cb, &info);
        while (!info.isdone) {
          g_main_context_iteration(g_main_loop_get_context(mLoop), FALSE);
          usleep(50000);
        }
      }
    }
  }
  return info.retval;
}

bool AMNetworkManager_priv::wifi_get_active_ap(APInfo **infos)
{
  if (AM_LIKELY(init())) {
    const GPtrArray *devices = nm_client_get_devices(mNmClient);
    APInfo **info = infos;
    int num = 0;
    for (unsigned int i = 0; devices && (i < devices->len); i ++) {
      NMDevice *dev = NM_DEVICE(g_ptr_array_index (devices, i));
      if (dev && NM_IS_DEVICE_WIFI (dev)) {
        if (nm_device_get_state(dev) == NM_DEVICE_STATE_ACTIVATED) {
          NMAccessPoint *active_ap =
          nm_device_wifi_get_active_access_point(NM_DEVICE_WIFI (dev));
          const char *active_bssid = nm_access_point_get_bssid(active_ap);
          if (AM_LIKELY(active_ap)) {
            *info = new APInfo();
            (*info)->info_next= nullptr;
            (*info)->index = num++;
            (*info)->active_bssid = active_bssid ?
                g_strdup(active_bssid) : nullptr;
            (*info)->device = g_strdup(nm_device_get_iface(dev));
            detail_access_point(active_ap, *info);
            info = &((*info)->info_next);
          }
        }
      }
    }
  }
  return (infos != nullptr);
}

int AMNetworkManager_priv::disconnect_device(const char *iface)
{
  NMDevice *device = nullptr;
  const GPtrArray *devices;
  NmCbData info;

  if (AM_LIKELY(init())) {
    if (!iface) {
      ERROR("Interface not be set");
      return -1;
    }

    devices = nm_client_get_devices(mNmClient);
    for (uint32_t i = 0; devices && (i < devices->len); i ++) {
      NMDevice *candidate = (NMDevice *) g_ptr_array_index (devices, i);
      const char *dev_iface = nm_device_get_iface (candidate);

      if (is_str_same (dev_iface, iface)) {
        device = candidate;
        break;
      }
    }
    info.device = device;
    info.retval = -1;
    info.isdone = false;
    info.timeout = 10;

    nm_device_disconnect(device, disconnect_device_cb, &info);
    while (!info.isdone) {
      g_main_context_iteration(g_main_loop_get_context(mLoop), FALSE);
      usleep(50000);
    }
  }

  return info.retval;
}

bool AMNetworkManager_priv::wifi_set_enabled(bool enable)
{
  if (AM_LIKELY(init())) {
    nm_client_wireless_set_enabled(mNmClient, enable);
  }
  return (enable == (nm_client_wireless_get_enabled(mNmClient) &&
                     nm_client_wireless_hardware_get_enabled(mNmClient)));
}

bool AMNetworkManager_priv::network_set_enabled(bool enable)
{
  if (AM_LIKELY(init())) {
    nm_client_networking_set_enabled(mNmClient, enable);
  }
  return (enable == nm_client_networking_get_enabled(mNmClient));
}
