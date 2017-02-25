/*******************************************************************************
 * am_network_dev.cpp
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

#include <arpa/inet.h>
#include <net/if.h>
#include <net/route.h>
#include <netdb.h>
#include <ifaddrs.h>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"
#include "am_network.h"
/*
 * DnsIPv4
 */

bool operator==(DnsIPv4 &dns1, DnsIPv4 &dns2)
{
  return (&dns1 == &dns2) ? true : (dns1.get_dns() == dns2.get_dns());
}

bool operator!=(DnsIPv4 &dns1, DnsIPv4 &dns2)
{
  return !(dns1 == dns2);
}

DnsIPv4::DnsIPv4() :
  dns_next(nullptr),
  dns(0)
{
  memset(dns_string, 0, sizeof(dns_string));
}

DnsIPv4::~DnsIPv4()
{
  delete dns_next;
}

void DnsIPv4::set_dns(const char* d)
{
  dns = 0;
  memset(dns_string, 0, sizeof(dns_string));
  if (d) {
    if (AM_LIKELY(inet_pton(AF_INET, d, &dns) > 0)) {
      strncpy(dns_string, d, strlen(d));
    } else {
      PERROR("inet_pton");
    }
  }
}

void DnsIPv4::set_dns(uint32_t d)
{
  dns = d;
  memset(dns_string, 0, sizeof(dns_string));
  if (AM_UNLIKELY(nullptr == inet_ntop(AF_INET, (void*)&dns,
                                    dns_string, INET_ADDRSTRLEN))) {
    PERROR("inet_ntop");
  }
}

uint32_t DnsIPv4::get_dns()
{
  return dns;
}

char *DnsIPv4::get_dns_string()
{
  return dns_string;
}
/*
 * DnsIPv6
 */
bool operator==(struct in6_addr &addr1, struct in6_addr &addr2)
{
  return (0 == memcmp(&addr1, &addr2, sizeof(struct in6_addr)));
}

bool operator!=(struct in6_addr &addr1, struct in6_addr &addr2)
{
  return !(addr1 == addr2);
}

bool operator==(DnsIPv6 &dns1, DnsIPv6 &dns2)
{
  return (&dns1 == &dns2) ? true : ((*dns1.get_dns()) == (*dns2.get_dns()));
}

bool operator!=(DnsIPv6 &addr1, DnsIPv6 &addr2)
{
  return !(addr1 == addr2);
}

DnsIPv6::DnsIPv6() :
    dns_next(nullptr)
{
  memset(&dns, 0, sizeof(dns));
  memset(dns_string, 0, sizeof(dns_string));
}

DnsIPv6::~DnsIPv6()
{
  delete dns_next;
}

void DnsIPv6::set_dns(const struct in6_addr& d)
{
  memcpy(&dns, &d, sizeof(d));
}

void DnsIPv6::set_dns(struct in6_addr* d)
{
  if (AM_LIKELY(d)) {
    memcpy(&dns, d, sizeof(dns));
  }
}

void DnsIPv6::set_dns(const char* d)
{
  memset(&dns, 0, sizeof(dns));
  memset(dns_string, 0, sizeof(dns_string));
  if (AM_LIKELY(d)) {
    if (AM_LIKELY(inet_pton(AF_INET6, d, &dns) > 0)) {
      strncpy(dns_string, d, strlen(d));
    } else {
      PERROR("inet_pton");
    }
  }
}

struct in6_addr* DnsIPv6::get_dns()
{
  return &dns;
}

char* DnsIPv6::get_dns_string()
{
  memset(&dns_string, 0, sizeof(struct in6_addr));
  if (AM_UNLIKELY(nullptr == inet_ntop(AF_INET6, (void*)&dns,
                                    dns_string, INET6_ADDRSTRLEN))) {
    PERROR("inet_ntop");
  }

  return dns_string;
}

/*
 * NetInfoIPv4
 */
NetInfoIPv4::NetInfoIPv4() :
    dns(nullptr),
    info_next(nullptr),
    netdev_address(0),
    netdev_netmask(0),
    netdev_gateway(0),
    netdev_prefix(0)
{
}

NetInfoIPv4::~NetInfoIPv4()
{
  delete dns;
  delete info_next;
}

bool operator==(NetInfoIPv4 &addr1, NetInfoIPv4 &addr2)
{
  return (&addr1 == &addr2) ? true :
      ((addr1.get_address() == addr2.get_address()) &&
       (addr1.get_netmask() == addr2.get_netmask()) &&
       (addr1.get_gateway() == addr2.get_gateway()));
}

bool operator!=(NetInfoIPv4 &addr1, NetInfoIPv4 &addr2)
{
  return !(addr1 == addr2);
}

void NetInfoIPv4::print_address_info()
{
  PRINTF("   IP Address: %s\n", get_address_string());
  PRINTF("     Net Mask: %s\n", get_netmask_string());
  PRINTF("     Gate Way: %s\n", get_gateway_string());
  if (AM_LIKELY(dns)) {
    uint32_t count = 1;
    for (DnsIPv4 *d = dns; d; d = d->dns_next) {
      PRINTF("         DNS%u: %s\n", count ++, d->get_dns_string());
    }
  }
}

void NetInfoIPv4::add_dns(DnsIPv4 *dnsInfo)
{
  DnsIPv4 **info = &dns;
  for (; *info; info = &((*info)->dns_next)) {
    if (AM_UNLIKELY((*(*info)) == (*dnsInfo))) {
      if (AM_LIKELY(*info != dnsInfo)) {
        delete dnsInfo;
      }
      return;
    }
  }
  *info = dnsInfo;
}

DnsIPv4* NetInfoIPv4::get_dns()
{
  return dns;
}

void NetInfoIPv4::set_address(uint32_t addr)
{
  netdev_address = addr;
  memset(address_string, 0, sizeof(address_string));
  if (AM_UNLIKELY(nullptr == inet_ntop(AF_INET, (void*)&netdev_address,
          address_string, INET_ADDRSTRLEN))) {
    PERROR("inet_ntop");
  }
}

void NetInfoIPv4::set_address(const char* addr)
{
  memset(address_string, 0, sizeof(address_string));
  netdev_address = 0;
  if (AM_LIKELY(addr)) {
    if (AM_LIKELY(inet_pton(AF_INET, addr, &netdev_address) > 0)) {
      strncpy(address_string, addr, strlen(addr));
    } else {
      PERROR("inet_pton");
    }
  }
}

uint32_t NetInfoIPv4::get_address()
{
  return netdev_address;
}

char* NetInfoIPv4::get_address_string()
{
  return address_string;
}

void NetInfoIPv4::set_netmask(uint32_t mask)
{
  netdev_netmask = mask;
  memset(netmask_string, 0, sizeof(netmask_string));
  if (AM_UNLIKELY(nullptr == inet_ntop(AF_INET, (void*)&netdev_netmask,
          netmask_string, INET_ADDRSTRLEN))) {
    PERROR("inet_ntop");
  }
}

void NetInfoIPv4::set_netmask(const char* mask)
{
  memset(netmask_string, 0, sizeof(netmask_string));
  netdev_netmask = 0;
  if (AM_LIKELY(mask)) {
    if (AM_LIKELY(inet_pton(AF_INET, mask, &netdev_netmask) > 0)) {
      uint32_t prefix = 0;
      for (uint32_t i = 0; i < sizeof(netdev_netmask) * 8; ++ i) {
        prefix += ((1 << i) & netdev_netmask) ? 1 : 0;
      }
      set_prefix(prefix);
      strncpy(netmask_string, mask, strlen(mask));
    } else {
      PERROR("inet_pton");
    }
  }
}

uint32_t NetInfoIPv4::get_netmask()
{
  return netdev_netmask;
}

char* NetInfoIPv4::get_netmask_string()
{
  return netmask_string;
}

void NetInfoIPv4::set_gateway(uint32_t gw)
{
  netdev_gateway = gw;
  memset(gateway_string, 0, sizeof(gateway_string));
  if (AM_UNLIKELY(nullptr == inet_ntop(AF_INET, (void*)&netdev_gateway,
          gateway_string, INET_ADDRSTRLEN))) {
    PERROR("inet_ntop");
  }
}

void NetInfoIPv4::set_gateway(const char* gw)
{
  memset(gateway_string, 0, sizeof(gateway_string));
  netdev_gateway = 0;
  if (AM_LIKELY(gw)) {
    if (AM_LIKELY(inet_pton(AF_INET, gw, &netdev_gateway) > 0)) {
      strncpy(gateway_string, gw, strlen(gw));
    } else {
      PERROR("inet_pton");
    }
  }
}

uint32_t NetInfoIPv4::get_gateway()
{
  return netdev_gateway;
}

char* NetInfoIPv4::get_gateway_string()
{
  return gateway_string;
}

void NetInfoIPv4::set_prefix(uint32_t prefix)
{
  netdev_prefix = prefix;
}
uint32_t NetInfoIPv4::get_prefix()
{
  return netdev_prefix;
}

/*
 * NetInfoIPv6
 */
NetInfoIPv6::NetInfoIPv6() :
    info_next(nullptr),
    dns(nullptr),
    netdev_prefix(0)
{
  netdev_address = in6addr_any;
  netdev_netmask = in6addr_any;
  netdev_gateway = in6addr_any;
}

NetInfoIPv6::~NetInfoIPv6()
{
  delete info_next;
}

bool operator==(NetInfoIPv6 &addr1, NetInfoIPv6 &addr2)
{
  return (&addr1 == &addr2) ? true :
      (((*addr1.get_address()) == (*addr2.get_address())) &&
       ((*addr1.get_gateway()) == (*addr2.get_gateway())) &&
       ((*addr1.get_netmask()) == (*addr2.get_netmask())) &&
       (addr1.get_prefix() == addr2.get_prefix()));
}

bool operator!=(NetInfoIPv6 &addr1, NetInfoIPv6 &addr2)
{
  return !(addr1 == addr2);
}

void NetInfoIPv6::print_address_info()
{
  PRINTF("   IP Address: %s\n", get_address_string());
  PRINTF("     Net Mask: %s\n", get_netmask_string());
  PRINTF("     Gate Way: %s\n", get_gateway_string());
  if (AM_LIKELY(dns)) {
    uint32_t count = 1;
    for (DnsIPv6 *d = dns; d; d = d->dns_next) {
      PRINTF("         DNS%u: %s\n", count ++, d->get_dns_string());
    }
  }
}

void NetInfoIPv6::add_dns(DnsIPv6 *dnsInfo)
{
  DnsIPv6 **info = &dns;
  for (; *info; info = &((*info)->dns_next)) {
    if (AM_UNLIKELY((*(*info)) == (*dnsInfo))) {
      if (AM_LIKELY(*info != dnsInfo)) {
        delete dnsInfo;
      }
      return;
    }
  }
  *info = dnsInfo;
}

DnsIPv6* NetInfoIPv6::get_dns()
{
  return dns;
}

void NetInfoIPv6::set_address(const struct in6_addr& addr)
{
  memcpy(&netdev_address, &addr, sizeof(struct in6_addr));
}

void NetInfoIPv6::set_address(struct in6_addr* addr)
{
  if (AM_LIKELY(addr)) {
    memcpy(&netdev_address, addr, sizeof(struct in6_addr));
  }
}

void NetInfoIPv6::set_address(const char* addr)
{
  memset(&netdev_address, 0, sizeof(netdev_address));
  memset(address_string, 0, sizeof(address_string));
  if (AM_LIKELY(addr)) {
    if (AM_LIKELY(inet_pton(AF_INET6, addr, &netdev_address) > 0)) {
      strncpy(address_string, addr, strlen(addr));
    } else {
      PERROR("inet_ntop");
    }
  }
}

struct in6_addr* NetInfoIPv6::get_address()
{
  return &netdev_address;
}
char* NetInfoIPv6::get_address_string()
{
  memset(&address_string, 0, sizeof(struct in6_addr));
  if (AM_UNLIKELY(nullptr == inet_ntop(AF_INET6, (void*)&netdev_address,
          address_string, INET6_ADDRSTRLEN))) {
    PERROR("inet_ntop");
  }

  return address_string;
}

void NetInfoIPv6::set_netmask(const struct in6_addr& mask)
{
  memcpy(&netdev_netmask, &mask, sizeof(struct in6_addr));
}

void NetInfoIPv6::set_netmask(struct in6_addr* mask)
{
  if (AM_LIKELY(mask)) {
    memcpy(&netdev_netmask, mask, sizeof(struct in6_addr));
  }
}

void NetInfoIPv6::set_netmask(const char* mask)
{
  memset(&netdev_netmask, 0, sizeof(netdev_netmask));
  memset(netmask_string, 0, sizeof(netmask_string));
  if (AM_LIKELY(mask)) {
    if (AM_LIKELY(inet_pton(AF_INET6, mask, &netdev_netmask) > 0)) {
      strncpy(netmask_string, mask, strlen(mask));
    } else {
      PERROR("inet_ntop");
    }
  }
}

struct in6_addr* NetInfoIPv6::get_netmask()
{
  return &netdev_netmask;
}
char* NetInfoIPv6::get_netmask_string()
{
  memset(&netmask_string, 0, sizeof(struct in6_addr));
  if (AM_UNLIKELY(nullptr == inet_ntop(AF_INET6, (void*)&netdev_netmask,
          netmask_string, INET6_ADDRSTRLEN))) {
    PERROR("inet_ntop");
  }

  return netmask_string;
}

void NetInfoIPv6::set_gateway(const struct in6_addr& gw)
{
  memcpy(&netdev_gateway, &gw, sizeof(struct in6_addr));
}

void NetInfoIPv6::set_gateway(struct in6_addr* gw)
{
  if (AM_LIKELY(gw)) {
    memcpy(&netdev_gateway, gw, sizeof(struct in6_addr));
  }
}

void NetInfoIPv6::set_gateway(const char* gw)
{
  memset(&netdev_gateway, 0, sizeof(netdev_gateway));
  memset(gateway_string, 0, sizeof(gateway_string));
  if (AM_LIKELY(gw)) {
    if (AM_LIKELY(inet_pton(AF_INET6, gw, &netdev_gateway) > 0)) {
      strncpy(gateway_string, gw, strlen(gw));
    } else {
      PERROR("inet_ntop");
    }
  }
}

struct in6_addr* NetInfoIPv6::get_gateway()
{
  return &netdev_gateway;
}

char* NetInfoIPv6::get_gateway_string()
{
  memset(&gateway_string, 0, sizeof(struct in6_addr));
  if (AM_UNLIKELY(nullptr == inet_ntop(AF_INET6, (void*)&netdev_gateway,
          gateway_string, INET6_ADDRSTRLEN))) {
    PERROR("inet_ntop");
  }

  return gateway_string;
}

void NetInfoIPv6::set_prefix(uint32_t prefix)
{
  uint32_t bytes = prefix / sizeof(uint8_t);
  uint32_t remainbits = sizeof(struct in6_addr) * sizeof(uint8_t)
      - bytes * sizeof(uint8_t);
  uint32_t count;
  netdev_prefix = prefix;
  memset(&netdev_netmask, 0, sizeof(struct in6_addr));
  for (count = sizeof(struct in6_addr) - 1;
      count >= (sizeof(struct in6_addr) - bytes); ++ count) {
    netdev_netmask.s6_addr[count] = 0xff;
  }
  netdev_netmask.s6_addr[count] = (0xff << (8 - remainbits));
}
uint32_t NetInfoIPv6::get_prefix()
{
  return netdev_prefix;
}

/*
 * NetDeviceInfo
 */
NetDeviceInfo::NetDeviceInfo() :
    netdev_name(nullptr),
    netdev_mac(nullptr),
    ipv4(nullptr),
    ipv6(nullptr),
    info_next(nullptr),
    netdev_index(0),
    netdev_mtu(0),
    is_default(false)
{
}
NetDeviceInfo::~NetDeviceInfo()
{
  delete[] netdev_name;
  delete[] netdev_mac;
  delete ipv4;
  delete ipv6;
  delete info_next;
}

void NetDeviceInfo::print_netdev_info()
{
  PRINTF("        Index: %u\n", netdev_index);
  PRINTF("     MTU Size: %d\n", netdev_mtu);
  PRINTF("         Name: %s\n", netdev_name);
  PRINTF("  MAC Address: %s\n", netdev_mac);
  if (ipv4) {
    PRINTF("==============IPv4==============\n");
    ipv4->print_address_info();
  }
  if (ipv6) {
    PRINTF("==============IPv6==============\n");
    ipv6->print_address_info();
  }
  if (AM_LIKELY(info_next)) {
    info_next->print_netdev_info();
  }
}

NetDeviceInfo* NetDeviceInfo::insert_netdev_info(NetDeviceInfo *info)
{
  if (info && (info != this)) {
    info->info_next = this;
    return info;
  }
  return this;
}

NetDeviceInfo* NetDeviceInfo::next()
{
  return info_next;
}

NetDeviceInfo* NetDeviceInfo::find_netdev_by_name(const char *name)
{
  if (name) {
    for (NetDeviceInfo *devInfo = this; devInfo; devInfo = devInfo->next()) {
      if (devInfo->get_netdev_name()
          && (strcmp(devInfo->get_netdev_name(), name) == 0)) {
        return devInfo;
      }
    }
  }
  return 0;
}

NetDeviceInfo* NetDeviceInfo::find_netdev_by_mac(const char *mac)
{
  if (mac) {
    for (NetDeviceInfo *devInfo = this; devInfo; devInfo = devInfo->next()) {
      if (devInfo->get_netdev_mac()
          && (strcasecmp(devInfo->get_netdev_mac(), mac) == 0)) {
        return devInfo;
      }
    }
  }
  return 0;
}

void NetDeviceInfo::set_netdev_mtu(int32_t mtuSize)
{
  netdev_mtu = mtuSize;
}

int32_t NetDeviceInfo::get_netdev_mtu()
{
  return netdev_mtu;
}

void NetDeviceInfo::set_netdev_name(const char *name)
{
  delete[] netdev_name;
  netdev_name = nullptr;
  if (AM_LIKELY(name)) {
    netdev_name = amstrdup(name);
    netdev_index = if_nametoindex(netdev_name);
  }
}
char* NetDeviceInfo::get_netdev_name()
{
  return netdev_name;
}

void NetDeviceInfo::set_netdev_mac(const char *mac)
{
  delete[] netdev_mac;
  netdev_mac = nullptr;
  if (AM_LIKELY(mac)) {
    netdev_mac = amstrdup(mac);
  }
}
char* NetDeviceInfo::get_netdev_mac()
{
  return netdev_mac;
}

void NetDeviceInfo::add_netdev_ipv4(NetInfoIPv4 *ip4)
{
  NetInfoIPv4 **info = &ipv4;
  for (; *info; info = &((*info)->info_next)) {
    if (AM_UNLIKELY(*(*info) == *ip4)) {
      if (AM_LIKELY(*info != ip4)) {
        delete ip4;
      }
      return;
    }
  }
  *info = ip4;
}
NetInfoIPv4* NetDeviceInfo::get_netdev_ipv4()
{
  return ipv4;
}

void NetDeviceInfo::add_netdev_ipv6(NetInfoIPv6 *ip6)
{
  NetInfoIPv6 **info = &ipv6;
  for (; *info; info = &((*info)->info_next)) {
    if (AM_UNLIKELY(*(*info) == *ip6)) {
      if (AM_LIKELY(*info != ip6)) {
        delete ip6;
      }
      return;
    }
  }
  *info = ip6;
}
NetInfoIPv6* NetDeviceInfo::get_netdev_ipv6()
{
  return ipv6;
}

bool NetDeviceInfo::check_mac(const char *mac)
{
  if (mac) {
    for (NetDeviceInfo *devInfo = this; devInfo; devInfo = devInfo->next()) {
      if (AM_LIKELY(is_str_equal(devInfo->get_netdev_mac (), mac))) {
        return true;
      }
    }
  }
  return false;
}
