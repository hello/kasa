/*******************************************************************************
 * am_network_dev.h
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

#ifndef AM_NETWORK_DEV_H_
#define AM_NETWORK_DEV_H_
#include <arpa/inet.h>
struct DnsIPv4
{
    uint32_t dns;
    char dns_string[INET_ADDRSTRLEN + 1];
    DnsIPv4 *dns_next;
    DnsIPv4();
    ~DnsIPv4();

    void set_dns(const char* d);
    void set_dns(uint32_t d);
    uint32_t get_dns();
    char *get_dns_string();
};

struct DnsIPv6
{
    struct in6_addr dns;
    char dns_string[INET6_ADDRSTRLEN + 1];
    DnsIPv6 *dns_next;
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
    uint32_t netdev_address;
    uint32_t netdev_netmask;
    uint32_t netdev_gateway;
    uint32_t netdev_prefix;
    char address_string[INET_ADDRSTRLEN + 1];
    char netmask_string[INET_ADDRSTRLEN + 1];
    char gateway_string[INET_ADDRSTRLEN + 1];
    DnsIPv4     *dns;
    NetInfoIPv4 *info_next;
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
    struct in6_addr netdev_address;
    struct in6_addr netdev_netmask;
    struct in6_addr netdev_gateway;
    uint32_t netdev_prefix;
    char address_string[INET6_ADDRSTRLEN + 1];
    char netmask_string[INET6_ADDRSTRLEN + 1];
    char gateway_string[INET6_ADDRSTRLEN + 1];
    NetInfoIPv6 *info_next;
    DnsIPv6     *dns;
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
    bool is_default;
    uint32_t netdev_index;
    int32_t netdev_mtu;
    char *netdev_name;
    char *netdev_mac;
    NetInfoIPv4 *ipv4;
    NetInfoIPv6 *ipv6;
    NetDeviceInfo *info_next;
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
    int index;

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
    APInfo();
    ~APInfo();
    void print_ap_info();
};

#endif /* AM_NETWORK_DEV_H_ */
