/*******************************************************************************
 * am_network_config.h
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
#ifndef AM_NETWORK_CONFIG_H_
#define AM_NETWORK_CONFIG_H_
#include <net/if.h>
#define MAXIFACE 10
class AMNetworkConfig
{
  public:
    AMNetworkConfig();
    ~AMNetworkConfig();

  public:
    bool get_default_connection(NetDeviceInfo **);
    void show_info(void);

  private:
    bool init();
    void finish();
    int iw_sockets_open(void);
    char *iw_get_ifname(char *, int, char *);
    void iw_enum_devices(int);

    inline bool check_socket(void);
    bool is_default_connection(char *);

    uint32_t get_mtu_size(char *iface);
    bool get_mac_address(char *iface, char *mtu, uint32_t size);
    bool get_address(char *iface, uint32_t *addr);
    bool get_netmask(char *iface, uint32_t *mask);
    bool get_gateway(char *iface, uint32_t *gw);
    bool get_dns(DnsIPv4 **info);
    bool get_ipv4_details(char *, NetDeviceInfo &);

  private:
    int mSock;
    uint32_t mCount;
    bool     mIsInited;
    char     mInterface[MAXIFACE][IFNAMSIZ + 1];
};

#endif

