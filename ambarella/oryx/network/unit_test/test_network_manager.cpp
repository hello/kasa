/*******************************************************************************
 * test_network_manager.cpp
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_network.h"

void usage(int argc, char**argv)
{
  PRINTF("usage:\n");
  PRINTF("%s help \t\t Get help\n", argv[0]);
  PRINTF("%s list \t\t Get AP list \n", argv[0]);
  PRINTF("%s up <id> \t\t Up Connection \n", argv[0]);
  PRINTF("%s delete <id> \t Delete Connection \n", argv[0]);
  PRINTF("%s wifi <on|off> \t Enable/Disable Wifi \n", argv[0]);

  PRINTF("%s <AP_Name> \t\t Connect open AP\n", argv[0]);
  PRINTF("%s <AP_Name> <Password> \t Connect security AP\n", argv[0]);

  PRINTF("%s <AP_Name> <Password> static <ipaddress> "
         "<netmask> <gateway> <dns1> <dns2>\n", argv[0]);
  PRINTF("\t\t Conncet AP with Static IP, Note: "
         "Password must be set and be anything when AP is open\n");

  PRINTF("%s <AP_Name> <Password> hidden <open | wep40 |wep104 | wpa> "
         "<wep_index>\n", argv[0]);
  PRINTF("\t\t Connect Hidden AP\n");

  PRINTF("%s <Adhoc_Name> <Password> adhoc <open | wep40 |wep104 | wpa>\n",
         argv[0]);
  PRINTF("\t\t Setup Wifi as Adhoc mode, Note: "
         "Password must be set and be anything when Adhoc is open\n");

  PRINTF("%s <AP_Name> <Password> ap <open | wep40 |wep104 | wpa> <channel>\n",
         argv[0]);
  PRINTF("\t\t Setup Wifi as AP mode, Note: "
         "Password must be set and be anything when AP is open\n");
}

int main(int argc, char **argv)
{
  AMNetworkConfig networkConfig;
  AMNetworkManager networkManager;
  int ret = -1;
  int isHidden = 0;
  int isStatic = 0;
  int isAdhoc = 0;
  int isAP = 0;
  int wep_index = 0;
  int channel = 0;
  const char*apName = nullptr;
  const char*password = nullptr;
  const char*ipaddress = nullptr;
  const char*netmask = nullptr;
  const char*getway = nullptr;
  const char*dns1 = nullptr;
  const char*dns2 = nullptr;
  AMNetworkManager::APSecurityFlags security =
      AMNetworkManager::AM_802_11_AP_SEC_NONE;
  if (argc <= 1) {
        usage(argc, argv);
        return 0;
  }
  if (argc > 1) {
    apName = argv[1];
  }
  if (argc > 2) {
    password = argv[2];
  }
  if (argc > 3) {
    isHidden = is_str_same("hidden", argv[3]);
    isStatic = is_str_same("static", argv[3]);
    isAdhoc = is_str_same("adhoc", argv[3]);
    isAP = is_str_same("ap", argv[3]);
  }
  if (argc > 4) {
      if (is_str_same("open", argv[4] )) {
        security = AMNetworkManager::AM_802_11_AP_SEC_NONE;
      }else if (is_str_same("wep40", argv[4] )) {
        security = AMNetworkManager::AM_802_11_AP_SEC_PAIR_WEP40;
      }else if (is_str_same("wep104", argv[4] )) {
        security = AMNetworkManager::AM_802_11_AP_SEC_PAIR_WEP104;
      }else if(is_str_same("wpa", argv[4] )) {
        security = AMNetworkManager::AM_802_11_AP_SEC_PAIR_CCMP;
      }
      ipaddress = argv[4];
  }
  if (argc > 5) {
    wep_index = atol( argv[5]);
    if (isAP){
      channel = wep_index;
    }
    netmask = argv[5];
  }
  if (argc > 6) {
    getway = argv[6];
  }
  if (argc > 7) {
    dns1 = argv[7];
  }
  if (argc > 8) {
    dns2 = argv[8];
  }
  if (apName) {
    if (is_str_equal(apName, "help")) {
      usage(argc, argv);
    }
    else if (is_str_equal(apName, "list")) {
      APInfo *apLists = nullptr;
      int num = networkManager.wifi_list_ap(&apLists);
      for (APInfo *info = apLists; info; info = info->info_next) {
        info->print_ap_info();
      }
      INFO("Total AP Number=%d\n", num);
      delete apLists;
    } else if (is_str_same(apName, "dis")) {
      if (networkManager.disconnect_device("wlan0")) {
        INFO("Wifi disabled OK");
      } else {
        ERROR("Wifi disabled Error");
      }
    } else if (is_str_same(apName, "up")) {
      if (password) {
        INFO("Up '%s' ", password);
        if (networkManager.wifi_connection_up(password) < 0) {
          ERROR("Wifi up '%s' Error", password);
        } else {
          INFO("Wifi up '%s' OK", password);
        }
      }
    }else if (is_str_same(apName, "delete")) {
      if (password) {
        INFO("Delete '%s' ", password);
        if (networkManager.wifi_connection_delete(password) < 0) {
          ERROR("Wifi down '%s' Error", password);
        } else {
          INFO("Wifi down '%s' OK", password);
        }
      }
    }else if (is_str_same(apName, "wifi")) {
      bool wifi_enable = true;
      if (is_str_same (password, "off")) {
        wifi_enable = false;
      }
      if (!networkManager.wifi_set_enabled(wifi_enable)) {
        ERROR("Wifi '%s' Error", wifi_enable ? "on":"off");
      } else {
        INFO("Wifi '%s' OK", wifi_enable ? "on":"off");
      }
    }else {
      if (isHidden) {
        NOTICE("Hidden AP=%s, Password=%s, Security=%s, wep_index=%d",
                apName, password, argv[4] ? argv[4] : "null",
                    wep_index ? wep_index : 0);
        ret = networkManager.wifi_connect_hidden_ap(apName,  password,
                                                    security, wep_index);
      }else if (isStatic) {
        NetInfoIPv4 *ipv4 = new NetInfoIPv4();
        ipv4->set_address( ipaddress ? ipaddress : nullptr );
        ipv4->set_netmask( netmask ? netmask : nullptr );
        ipv4->set_gateway( getway ? getway : nullptr );
        NOTICE("AP=%s, Password=%s, Static, IP=%s, NetMask=%s(%u), "
               "Gateway=%s Dns1=%s Dns2=%s",
                apName, password,
                ipaddress ? ipaddress : nullptr,
                netmask ? netmask : nullptr,
                netmask ? ipv4->get_prefix() : 0,
                getway ? getway : nullptr,
                dns1 ? dns1 : nullptr,
                dns2 ? dns2 : nullptr);
        if (AM_LIKELY(dns1)) {
          DnsIPv4 *ipv4_dns = new DnsIPv4();
          ipv4_dns->dns_next = nullptr;
          ipv4_dns->set_dns(dns1);
          if (AM_LIKELY(dns2)) {
            DnsIPv4 *ipv4_dns_tmp = ipv4_dns;
            ipv4_dns_tmp = ipv4_dns_tmp->dns_next;
            ipv4_dns_tmp = new DnsIPv4();
            ipv4_dns_tmp->dns_next = nullptr;
            ipv4_dns_tmp->set_dns(dns2);
          }
          ipv4->add_dns(ipv4_dns);
        }
        ret = networkManager.wifi_connect_ap_static(apName, password, ipv4);
        delete ipv4;
      }else if ( isAdhoc ) {
        NOTICE("Adhoc=%s, Password=%s, Security=%s",
               apName, password, argv[4] ? argv[4] : "null");
        ret = networkManager.wifi_setup_adhoc(apName, password, security);
        if (ret != 0) {
          ERROR("Setup Adhoc '%s' Failed", apName);
        }else {
          INFO("Setup Adhoc '%s' OK", apName);
        }
      }else if ( isAP ) {
        NOTICE("AP=%s, Password=%s, Security=%s",
               apName, password, argv[4] ? argv[4] : "null");
        ret = networkManager.wifi_setup_ap(apName, password, security, channel);
        if (ret != 0) {
          ERROR("Setup AP '%s' Failed", apName);
        }else {
          INFO("Setup AP '%s' OK", apName);
        }
      }else {
        NOTICE("AP=%s, Password=%s", apName, password);
        ret = networkManager.wifi_connect_ap(apName, password);
      }
      if (ret != 0) {
        ERROR("Wifi connect Error");
      } else {
        NetDeviceInfo *networkDevice = nullptr;
        INFO("Wifi connected OK");
        APInfo *ap_active = nullptr;
        networkManager.wifi_get_active_ap(&ap_active);
        for (APInfo *info = ap_active; info; info = info->info_next) {
          info->print_ap_info();
        }
        networkConfig.show_info();
        delete ap_active;
        if (AM_LIKELY(networkConfig.get_default_connection(&networkDevice))) {
          NetDeviceInfo *dev = networkDevice;
          networkDevice->print_netdev_info();
          while (dev) {
            NOTICE("device=%s, isdefault=%s",
            dev->netdev_name, (dev->is_default ? "Yes" : "No"));
            dev = dev->info_next;
          }
        } else {
          ERROR("Failed to get active network connection!");
        }
        delete networkDevice;
      }
    }
  }

  return 0;
}
