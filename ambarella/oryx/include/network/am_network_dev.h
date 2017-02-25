/*******************************************************************************
 * am_network_dev.h
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

#ifndef AM_NETWORK_DEV_H_
#define AM_NETWORK_DEV_H_
#include <arpa/inet.h>
struct DnsIPv4
{
    DnsIPv4 *dns_next;
    uint32_t dns;
    char dns_string[INET_ADDRSTRLEN + 1];
    DnsIPv4();
    ~DnsIPv4();

    void set_dns(const char* d);
    void set_dns(uint32_t d);
    uint32_t get_dns();
    char *get_dns_string();
};

struct DnsIPv6
{
    DnsIPv6 *dns_next;
    char dns_string[INET6_ADDRSTRLEN + 1];
    struct in6_addr dns;
    DnsIPv6();
    ~DnsIPv6();

    void set_dns(const struct in6_addr& d);
    void set_dns(struct in6_addr* d);
    void set_dns(const char* d);
    struct in6_addr* get_dns();
    char *get_dns_string();
};

struct NetInfoIPv4
{
    DnsIPv4     *dns;
    NetInfoIPv4 *info_next;
    uint32_t netdev_address;
    uint32_t netdev_netmask;
    uint32_t netdev_gateway;
    uint32_t netdev_prefix;
    char address_string[INET_ADDRSTRLEN + 1];
    char netmask_string[INET_ADDRSTRLEN + 1];
    char gateway_string[INET_ADDRSTRLEN + 1];
    NetInfoIPv4();
    ~NetInfoIPv4();

    void print_address_info();

    void add_dns(DnsIPv4 *dns);
    DnsIPv4* get_dns();

    void set_address(uint32_t addr);
    void set_address(const char* addr);
    uint32_t get_address();
    char *get_address_string();

    void set_netmask(uint32_t mask);
    void set_netmask(const char* mask);
    uint32_t get_netmask();
    char *get_netmask_string();

    void set_gateway(uint32_t gw);
    void set_gateway(const char* gw);
    uint32_t get_gateway();
    char *get_gateway_string();

    void set_prefix(uint32_t prefix);
    uint32_t get_prefix();
};

struct NetInfoIPv6
{
    NetInfoIPv6 *info_next;
    DnsIPv6     *dns;
    uint32_t netdev_prefix;
    struct in6_addr netdev_address;
    struct in6_addr netdev_netmask;
    struct in6_addr netdev_gateway;
    char address_string[INET6_ADDRSTRLEN + 1];
    char netmask_string[INET6_ADDRSTRLEN + 1];
    char gateway_string[INET6_ADDRSTRLEN + 1];
    NetInfoIPv6();
    ~NetInfoIPv6();

    void print_address_info();

    void add_dns(DnsIPv6 *dns);
    DnsIPv6* get_dns();

    void set_address(const struct in6_addr& addr);
    void set_address(struct in6_addr* addr);
    void set_address(const char* addr);
    struct in6_addr* get_address();
    char *get_address_string();

    void set_netmask(const struct in6_addr& mask);
    void set_netmask(struct in6_addr* mask);
    void set_netmask(const char * mask);
    struct in6_addr* get_netmask();
    char *get_netmask_string();

    void set_gateway(const struct in6_addr& gw);
    void set_gateway(struct in6_addr* gw);
    void set_gateway(const char *gw);
    struct in6_addr* get_gateway();
    char *get_gateway_string();

    void set_prefix(uint32_t prefix);
    uint32_t get_prefix();
};

struct NetDeviceInfo
{
    char *netdev_name;
    char *netdev_mac;
    NetInfoIPv4 *ipv4;
    NetInfoIPv6 *ipv6;
    NetDeviceInfo *info_next;
    uint32_t netdev_index;
    int32_t netdev_mtu;
    bool is_default;
    NetDeviceInfo();
    ~NetDeviceInfo();
    void print_netdev_info();

    NetDeviceInfo *insert_netdev_info(NetDeviceInfo *info);
    NetDeviceInfo *next();
    NetDeviceInfo *find_netdev_by_name(const char *name);
    NetDeviceInfo *find_netdev_by_mac(const char *mac);

    void set_netdev_mtu(int32_t mtuSize);
    int32_t get_netdev_mtu();

    void set_netdev_name(const char *name);
    char *get_netdev_name();

    void set_netdev_mac(const char *mac);
    char *get_netdev_mac();

    void add_netdev_ipv4(NetInfoIPv4 *ip4);
    NetInfoIPv4* get_netdev_ipv4();

    void add_netdev_ipv6(NetInfoIPv6 *ip6);
    NetInfoIPv6* get_netdev_ipv6();

    bool check_mac(const char *mac);
};

struct APInfo
{
    const char *active_bssid;
    const char *device;
    const char *bssid;

    const char *ap_name;
    const char *ssid_str;
    const char *mode;
    const char *freq_str;
    const char *bitrate_str;
    const char *strength_str;
    const char *security_str;
    const char *wpa_flags_str;
    const char *rsn_flags_str;
    const char *active;
    const char *path;

    APInfo *info_next;

    int index;

    APInfo();
    ~APInfo();
    void print_ap_info();
};

#endif /* AM_NETWORK_DEV_H_ */
