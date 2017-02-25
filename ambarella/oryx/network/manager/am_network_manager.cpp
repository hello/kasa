/*******************************************************************************
 * am_network_manager.cpp
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
#include <dbus/dbus-glib.h>
#include <nm-utils.h>
#include <nm-client.h>
#include <nm-access-point.h>
#include <nm-remote-settings.h>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_network.h"
#include "am_network_manager_priv.h"


static const char *netstate_to_string[] =
{
  "Unknown",
  "Asleep",
  "Connecting",
  "Connected (Local Only)",
  "Connected (Site Only)",
  "Connected (Global)",
  "Disconnected"
};

static const char *devstate_to_string[] =
{
  "Unknown",
  "Unmanaged",
  "Unavailable",
  "Disconnected",
  "Connecting (Prepare)",
  "Connecting (Configuring)",
  "Connecting (Need Authentication)",
  "Connecting (Getting IP Configuration)",
  "Connecting (Checking IP Connectivity)",
  "Connecting (Starting Dependent Connections)",
  "Connected",
  "Disconnecting",
  "Failed"
};

AMNetworkManager::AMNetworkManager() :
  mNmPriv(nullptr)
{
  mNmPriv = new AMNetworkManager_priv();
}

AMNetworkManager::~AMNetworkManager()
{
  delete mNmPriv;
}

const char* AMNetworkManager::net_state_to_string(
    AMNetworkManager::NetworkState state)
{
  return netstate_to_string[state];
}

const char* AMNetworkManager::device_state_to_string(
    AMNetworkManager::DeviceState state)
{
  return devstate_to_string[state];
}

AMNetworkManager::NetworkState AMNetworkManager::get_network_state()
{
  return (mNmPriv ? mNmPriv->get_network_state() :
                    AMNetworkManager::AM_NM_STATE_UNKNOWN);
}

AMNetworkManager::DeviceState AMNetworkManager::get_device_state(
    const char *iface)
{
  return (mNmPriv ? mNmPriv->get_device_state(iface) :
                    AMNetworkManager::AM_DEV_STATE_UNAVAILABLE);
}

uint32_t AMNetworkManager::get_current_device_speed(const char *iface)
{
  return mNmPriv ? mNmPriv->get_current_device_speed(iface) : 0;
}

int AMNetworkManager::wifi_list_ap(APInfo **infos)
{
  return mNmPriv ? mNmPriv->wifi_list_ap(infos) : 0;
}

int AMNetworkManager::wifi_connect_ap(const char *apName, const char* password)
{
  return mNmPriv ? mNmPriv->wifi_connect_ap(apName, password) :  (-1);
}

int AMNetworkManager::wifi_connect_ap_static(const char *apName, const char* password, NetInfoIPv4 *ipv4)
{
  return mNmPriv ? mNmPriv->wifi_connect_ap_static(apName, password, ipv4) :  (-1);
}

int AMNetworkManager::wifi_connect_hidden_ap(const char *apName, const char* password,
     const AMNetworkManager::APSecurityFlags security, uint32_t wep_index)
{
  return mNmPriv ? mNmPriv->wifi_connect_hidden_ap(apName, password, security, wep_index) : (-1);
}

int AMNetworkManager::wifi_connect_hidden_ap_static(const char *apName, const char* password,
     const AMNetworkManager::APSecurityFlags security, uint32_t wep_index, NetInfoIPv4 *ipv4)
{
  return mNmPriv ? mNmPriv->wifi_connect_hidden_ap_static(apName, password, security, wep_index, ipv4) : (-1);
}

int AMNetworkManager::wifi_setup_adhoc(const char *name, const char*password, const AMNetworkManager::APSecurityFlags security)
{
  return mNmPriv ? mNmPriv->wifi_setup_adhoc(name, password, security) : (-1);
}

int AMNetworkManager::wifi_setup_ap(const char *name, const char*password, const AMNetworkManager::APSecurityFlags security, int channel)
{
  return mNmPriv ? mNmPriv->wifi_setup_ap(name, password, security, channel) : (-1);
}

int AMNetworkManager::wifi_connection_up(const char* ap_name)
{
  return mNmPriv ? mNmPriv->wifi_connection_up(ap_name) : (-1);
}

int AMNetworkManager::wifi_connection_delete(const char* ap_name)
{
  return mNmPriv ? mNmPriv->wifi_connection_delete(ap_name) : false;
}

bool AMNetworkManager::wifi_get_active_ap(APInfo **infos)
{
  return mNmPriv ? mNmPriv->wifi_get_active_ap(infos) : false;
}

bool AMNetworkManager::disconnect_device(const char *iface)
{
  return mNmPriv ? (mNmPriv->disconnect_device(iface) == 0) : false;
}

bool AMNetworkManager::wifi_set_enabled(bool enable)
{
  return mNmPriv ? mNmPriv->wifi_set_enabled(enable) : false;
}

bool AMNetworkManager::network_set_enabled(bool enable)
{
  return mNmPriv ? mNmPriv->network_set_enabled(enable) : false;
}
