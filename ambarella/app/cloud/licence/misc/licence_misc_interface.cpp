/*******************************************************************************
 * licence_lib_if.h
 *
 * History:
 *  2014/03/06 - [Zhi He] create file
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

#include "common_config.h"

#ifndef BUILD_OS_WINDOWS
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
//#include <string.h>
#endif

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "licence_lib_if.h"
#include "licence_encryption_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//to do

EECode gfGetNetworkDeviceInfo(SNetworkDevices *devices)
{
#ifdef BUILD_OS_WINDOWS
    LOG_FATAL("gfGetNetworkDeviceInfo, to do\n");
    return EECode_NotSupported;
#else
    struct ifreq ifr;
    struct ifconf ifc;
    TInt success = 0;
    TInt index = 0;
    TChar buf[1024];

    if (DUnlikely(!devices)) {
        LOG_ERROR("NULL devices\n");
        return EECode_BadParam;
    }

    devices->number = 0;

    TInt sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (DUnlikely(0 > sock)) {
        LOG_ERROR("socket error %d\n", sock);
        return EECode_OSError;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    success = ioctl(sock, SIOCGIFCONF, &ifc);
    if (DUnlikely(0 > success)) {
        LOG_ERROR("socket(SIOCGIFCONF) error %d\n", success);
        return EECode_OSError;
    }

    struct ifreq *it = ifc.ifc_req;
    devices->number = (ifc.ifc_len / sizeof(struct ifreq));
    const struct ifreq *const end = it + devices->number;

    DASSERT(devices->number < 5);

    for (index = 0; it != end; ++it) {
        strcpy(ifr.ifr_name, it->ifr_name);
        success = ioctl(sock, SIOCGIFFLAGS, &ifr);
        if (DLikely(0 <= success)) {
            if (!(ifr.ifr_flags & IFF_LOOPBACK)) {
                devices->device[index].flags = 0x0;
                success = ioctl(sock, SIOCGIFHWADDR, &ifr);
                memset(devices->device[index].ip_address, 0x0, sizeof(devices->device[index].ip_address));
                memset(devices->device[index].ipv6_address, 0x0, sizeof(devices->device[index].ipv6_address));
                memset(devices->device[index].mac_address, 0x0, sizeof(devices->device[index].mac_address));
                memset(devices->device[index].name, 0x0, sizeof(devices->device[index].name));
                if (DLikely(0 <= success)) {
                    TU8 *pprint = (TU8 *)ifr.ifr_hwaddr.sa_data;
                    snprintf(devices->device[index].mac_address, 31, "%02x:%02x:%02x:%02x:%02x:%02x", pprint[0], pprint[1], pprint[2], pprint[3], pprint[4], pprint[5]);
                    strcpy(devices->device[index].name, it->ifr_name);
                    //LOG_DEBUG("devices->device[index].mac_address [%s], devices->device[index].name [%s]\n", devices->device[index].mac_address, devices->device[index].name);
                }
                index ++;
            } else {
                //devices->device[index].flags = DFLAGS_LOOPBACK;
                //memset(devices->device[index].ip_address, 0x0, sizeof(devices->device[index].ip_address));
                //memset(devices->device[index].ipv6_address, 0x0, sizeof(devices->device[index].ipv6_address));
                //memset(devices->device[index].mac_address, 0x0, sizeof(devices->device[index].mac_address));
                //memset(devices->device[index].name, 0x0, sizeof(devices->device[index].name));
                //strcpy(devices->device[index].name, it->ifr_name);
            }
        } else {
            LOG_ERROR("socket(SIOCGIFFLAGS) error %d\n", success);
            return EECode_OSError;
        }
    }
    return EECode_OK;
#endif
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

