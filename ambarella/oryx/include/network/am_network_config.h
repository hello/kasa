/*******************************************************************************
 * am_network_config.h
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

