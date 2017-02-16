/*******************************************************************************
 * IPCamControlServer.cpp
 *
 * History:
 *  2011年03月17日 - [Yupeng Chang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <net/if.h>
#include <net/route.h>

#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "config.h"
#include "basetypes.h"
#include "iav_drv.h"
#include "amba_debug.h"
#include "IPCamControlServer.h"
#include "NandWrite.h"

IPCamControlServer::IPCamControlServer ()
: mNetDevInfo (0),
  mMulticastSockV4 (-1),
  mUnicastSockV4 (-1),
  mMulticastSockV6 (-1),
  mUnicastSockV6 (-1),
  mLocalSock (-1),
  mRunning (false),
  mInitialized (false),
  mUcodeLoaded (false),
  mMediaServerRunning (false),
  mDriverState (UNINSTALLED)
{
  memset (&mMultiGroupAddrV4, 0, sizeof (struct sockaddr_in));
  memset (&mMultiGroupAddrV6, 0, sizeof (struct sockaddr_in6));
  memset (&mUnicastAddrV4   , 0, sizeof (struct sockaddr_in));
  memset (&mUnicastAddrV6   , 0, sizeof (struct sockaddr_in6));
  memset (&mLocalAddr       , 0, sizeof (struct sockaddr_in));

  if (get_netdev_info ()) {
    mEncodeServerConfig = new EncodeServerConfig ();
    mInitialized = true;
  }
}

IPCamControlServer::~IPCamControlServer ()
{
  if (mEncodeServerConfig) {
    delete mEncodeServerConfig;
  }
  if (mNetDevInfo) {
    for (NetDevInfo * devInfo = mNetDevInfo;
         devInfo;
         devInfo = devInfo->next ()) {
      if (edit_route (devInfo, false, AF_INET) ||
        edit_route (devInfo, false, AF_INET6)) {
        /* Add route for every network device */
        fprintf (stderr, "Remove route from %s\n", devInfo->get_netdev_name());
      } else {
        fprintf (stderr,
                 "Failed modifying the route table of %s\n",
                 devInfo->get_netdev_name ());
      }
    }
    delete mNetDevInfo;
  }
  if (mUnicastSockV4 > 0) {
    fprintf (stderr, "Close mUnicastSockV4\n");
    close (mUnicastSockV4);
  }
  if (mMulticastSockV4 > 0) {
    fprintf (stderr, "Close mMulticastSockV4\n");
    close (mMulticastSockV4);
  }
  if (mUnicastSockV6 > 0) {
    fprintf (stderr, "Close mUnicastSockV6\n");
    close (mUnicastSockV6);
  }
  if (mMulticastSockV6 > 0) {
    fprintf (stderr, "Close mMulticastSockV6\n");
    close (mMulticastSockV6);
  }
  if (mLocalSock > 0) {
    fprintf (stderr, "Close mLocalSock\n");
    close (mLocalSock);
  }
  fprintf (stderr, "~IPCamControlServer\n");
}

inline void IPCamControlServer::reboot ()
{
  system ("reboot");
}

bool IPCamControlServer::edit_route (NetDevInfo * devInfo,
  bool action,
  int protocol)
{
  struct rtentry  rt;
  int             fd = -1;
  if (!devInfo || (devInfo && !devInfo->get_netdev_name ())) {
    fprintf (stderr, "No such device\n");
    return false;
  }
  memset (&rt, 0, sizeof(struct rtentry));
  rt.rt_dev = devInfo->get_netdev_name ();
  rt.rt_flags = RTF_UP;
  switch (protocol) {
    case AF_INET  : {
      struct sockaddr_in addr;
      memset (&addr, 0, sizeof(struct sockaddr_in));
      if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror ("edit_route socket");
        return false;
      }
      addr.sin_family = AF_INET;
      if (inet_pton (AF_INET, "238.0.0.0", (void *)&(addr.sin_addr)) <= 0) {
        perror ("IPCamControlServer::edit_route:inet_pton");
        return false;
      }
      memcpy ((void *)&(rt.rt_dst), (void *)&addr, sizeof(struct sockaddr_in));
      if (inet_pton (AF_INET, "255.0.0.0", (void *)&(addr.sin_addr)) <= 0) {
        perror ("IPCamControlServer::edit_route:inet_pton");
        return false;
      }
      memcpy ((void *)&(rt.rt_genmask),
              (void *)&addr,
              sizeof(struct sockaddr_in));
    }break;
    case AF_INET6 : {
      struct sockaddr_in6 addr;
      memset (&addr, 0, sizeof(struct sockaddr_in6));
      if ((fd = socket (AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror ("edit_route socket");
        return false;
      }
      addr.sin6_family = AF_INET6;
      if (inet_pton (AF_INET6, "FF12::", (void *)&(addr.sin6_addr)) <= 0) {
        perror ("IPCamControlServer::edit_route:inet_pton");
        return false;
      }
      memcpy ((void *)&(rt.rt_dst), (void *)&addr, sizeof(struct sockaddr_in6));
      if (inet_pton (AF_INET6, "FFFF::", (void *)&(addr.sin6_addr)) <= 0) {
        perror ("IPCamControlServer::edit_route:inet_pton");
        return false;
      }
      memcpy ((void *)&(rt.rt_genmask),
              (void *)&addr,
               sizeof(struct sockaddr_in6));
    }
    /* no break */
    default: break;
  }
  if (((protocol == AF_INET) && devInfo->get_netdev_ipv4 ()) ||
    /* If this device has IPv4 address */
    ((protocol == AF_INET6) && devInfo->get_netdev_ipv6 ())) {
    /* If this device has IPv6 address */
    if (action) { /* if action is TRUE, add route*/
      if (ioctl (fd, SIOCADDRT, &rt) < 0) {
        fprintf (stderr,
                 "%s ioctl SIOCADDRT %s\n",
                 devInfo->get_netdev_name (),
                 strerror(errno));
        close (fd);
        if (errno == 17) { /* This means the route has already been added */
          return true;
        }
        return false;
      }
    } else { /* if action is FALSE, delete route*/
      if (ioctl (fd, SIOCDELRT, &rt) < 0) {
        fprintf (stderr,
                 "%s ioctl SIOCDELRT %s\n",
                 devInfo->get_netdev_name (),
                 strerror(errno));
        close (fd);
        if (errno == 3) { /* This means the route has already been deleted */
          return true;
        }
        return false;
      }
    }
  } else {
    close (fd);
    return false;
  }
  close (fd);
  return true;
}

bool IPCamControlServer::init_socket (int protocol)
{
  /* Run Addroute first */
  bool addRouteState = false;
  for (NetDevInfo * devInfo = mNetDevInfo;
       devInfo;
       devInfo = devInfo->next ()) {
    if (edit_route (devInfo, true, protocol)) {
      /* Add route for every network device */
      addRouteState = true;
    } else {
      fprintf (stderr,
        "Failed modifying the route table of %s for %s network.\n",
        devInfo->get_netdev_name (),
        (protocol == AF_INET ? "IPv4": "IPv6"));
    }
  }
  if (false == addRouteState) {
#ifdef DEBUG
    if (protocol == AF_INET) {
      fprintf (stderr, "Failed to add route 238.0.0.0/8 in protocol IPv4\n");
    } else {
      fprintf (stderr, "Failed to add route FF12::0/16 in protocol IPv6\n");
    }
#endif
    return false;
  }
  if (mLocalSock < 0) {
    /* Initialize local socket for encode server controling */
    mLocalAddr.sin_family = AF_INET;
    mLocalAddr.sin_port   = htons (LOCAL_PORT);
    if (inet_pton(AF_INET,
                  LOCAL_ADDR,
                  (void *)(&(mLocalAddr.sin_addr))) <= 0) {
      perror ("IPCamControlServer::init_socket inet_pton error");
      return false;
    }
    /* Init local socket*/
    if ((mLocalSock = socket (AF_INET , SOCK_STREAM, IPPROTO_TCP)) < 0) {
      perror ("IPCamControlServer::init_socket socket error");
      return false;
    }
  }
  /* The local socket will be connected to the encode
   * server when the media server is enabled */
  switch (protocol) {
    case AF_INET  : {
      int optval = 1;
      struct ip_mreqn ipMreqn;

      /* Initialize IP address structure*/
      mMultiGroupAddrV4.sin_family = AF_INET;
      mMultiGroupAddrV4.sin_port   = htons (MULTICAST_PORT);
      if (inet_pton (AF_INET,
                     MULTICAST_ADDR_IPV4,
                     (void *)(&(mMultiGroupAddrV4.sin_addr))) <= 0) {
        perror ("IPCamControlServer::init_socket inet_pton error");
        return false;
      }
      mUnicastAddrV4.sin_family      = AF_INET;
      mUnicastAddrV4.sin_port        = htons (UNICAST_PORT);
      mUnicastAddrV4.sin_addr.s_addr = htonl(INADDR_ANY);

      /* Initialize socket */
      if (((mMulticastSockV4 = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)||
          ((mUnicastSockV4 = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)) {
        perror ("IPCamControlServer::init_socket ipv4 socket error");
        if (mMulticastSockV4 > 0) {
          close (mMulticastSockV4);
          mMulticastSockV4 = -1;
        }
        if (mUnicastSockV4 > 0) {
          close (mUnicastSockV4);
          mUnicastSockV4 = -1;
        }
        return false;
      }

      /* Set socket option */
      if ((setsockopt(mMulticastSockV4,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      &optval,
                      sizeof(optval)) < 0) ||
          (setsockopt(mUnicastSockV4,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      &optval,
                      sizeof(optval)) < 0)) {
        perror ("IPCamControlServer::init_socket setsockopt error");
        close (mMulticastSockV4);
        close (mUnicastSockV4);
        mMulticastSockV4 = -1;
        mUnicastSockV4   = -1;
        return false;
      }

      /* Set Multicast socket option */
      memset (&ipMreqn  , 0 , sizeof (struct ip_mreqn));
      memcpy (&(ipMreqn.imr_multiaddr),
              &(mMultiGroupAddrV4.sin_addr.s_addr),
              sizeof(struct in_addr));
      for (NetDevInfo * devInfo = mNetDevInfo;
           devInfo;
           devInfo = devInfo->next ()) {
        if (devInfo->get_netdev_ipv4()) {
          if (inet_pton (AF_INET,
                        devInfo->get_netdev_ipv4(),
                        &(ipMreqn.imr_address)) <= 0) {
            perror ("IPCamControlServer::init_socket inet_pton error");
            close (mMulticastSockV4);
            close (mUnicastSockV4);
            mMulticastSockV4 = -1;
            mUnicastSockV4   = -1;
            return false;
          }
          ipMreqn.imr_ifindex = devInfo->get_netdev_ifindex ();
          if (setsockopt (mMulticastSockV4, IPPROTO_IP, IP_ADD_MEMBERSHIP,
              &ipMreqn, sizeof(struct ip_mreqn)) < 0) {
            perror ("IPCamControlServer::init_socket setsockopt error");
            close (mMulticastSockV4);
            close (mUnicastSockV4);
            mMulticastSockV4 = -1;
            mUnicastSockV4   = -1;
            return false;
          }
        }
      }

      /* bind socket */
      if ((bind (mMulticastSockV4,
                 (struct sockaddr *)&mMultiGroupAddrV4,
                 sizeof(struct sockaddr)) < 0) ||
          (bind (mUnicastSockV4,
                 (struct sockaddr *)&mUnicastAddrV4,
                 sizeof(struct sockaddr)) < 0)) {
        perror ("IPCamControlServer::init_socket bind error");
        close (mMulticastSockV4);
        close (mUnicastSockV4);
        mMulticastSockV4 = -1;
        mUnicastSockV4   = -1;
        return false;
      }
    } break;
    case AF_INET6 : {
      int optval = 1;
      struct ipv6_mreq ipv6Mreq;
      /* Initialize IPv6 address structure */
      mMultiGroupAddrV6.sin6_family = AF_INET6;
      mMultiGroupAddrV6.sin6_port   = htons (MULTICAST_PORT);
      if (inet_pton(AF_INET6,
                    MULTICAST_ADDR_IPV6,
                    (void *)(&(mMultiGroupAddrV6.sin6_addr))) <= 0) {
        perror ("IPCamControlServer::init_socket inet_pton error");
        return false;
      }
      mUnicastAddrV6.sin6_family = AF_INET6;
      mUnicastAddrV6.sin6_port   = htons (UNICAST_PORT);
      mUnicastAddrV6.sin6_addr   = in6addr_any;

      /* Initialize socket */
      if (((mMulticastSockV6 = socket (AF_INET6,
                                       SOCK_DGRAM,
                                       IPPROTO_UDP)) < 0) ||
        ((mUnicastSockV6   = socket (AF_INET6,
                                     SOCK_DGRAM,
                                     IPPROTO_UDP)) < 0)) {
        perror ("IPCamControlServer::init_socket ipv6 socket error");
        if (mMulticastSockV6 > 0) {
          close (mMulticastSockV6);
          mMulticastSockV6 = -1;
        }
        if (mUnicastSockV6 > 0) {
          close (mUnicastSockV6);
          mUnicastSockV6 = -1;
        }
        return false;
      }

      /* Set socket option */
      if ((setsockopt(mMulticastSockV6,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      &optval,
                      sizeof(optval)) < 0) ||
          (setsockopt(mUnicastSockV6,
                      SOL_SOCKET,
                      SO_REUSEADDR,
                      &optval,
                      sizeof(optval)) < 0)) {
        perror ("IPCamControlServer::init_socket setsockopt error");
        close (mMulticastSockV6);
        close (mUnicastSockV6);
        mMulticastSockV6 = -1;
        mUnicastSockV6   = -1;
        return false;
      }

      /* bind socket */
      if ((bind (mMulticastSockV6,
                 (struct sockaddr *)&mMultiGroupAddrV6,
                 sizeof(struct sockaddr)) < 0) ||
          (bind (mUnicastSockV6,
                 (struct sockaddr *)&mUnicastAddrV6,
                 sizeof(struct sockaddr)) < 0)) {
        perror ("IPCamControlServer::init_socket bind error");
        close (mMulticastSockV6);
        close (mUnicastSockV6);
        mMulticastSockV6 = -1;
        mUnicastSockV6   = -1;
        return false;
      }

      /* Set Multicast socket option*/
      memset (&ipv6Mreq , 0 , sizeof (struct ipv6_mreq));
      memcpy (&(ipv6Mreq.ipv6mr_multiaddr),
              &(mMultiGroupAddrV6.sin6_addr),
              sizeof(struct in6_addr));
      for (NetDevInfo * devInfo = mNetDevInfo;
           devInfo;
           devInfo = devInfo->next ()) {
        if (devInfo->get_netdev_ipv6()) {
          ipv6Mreq.ipv6mr_interface = devInfo->get_netdev_ifindex ();
          if (setsockopt (mMulticastSockV6, IPPROTO_IPV6, IPV6_JOIN_GROUP,
              &ipv6Mreq, sizeof(struct ipv6_mreq)) < 0) {
            perror ("IPCamControlServer::init_socket setsockopt error");
            close (mMulticastSockV6);
            close (mUnicastSockV6);
            mMulticastSockV6 = -1;
            mUnicastSockV6   = -1;
            return false;
          }
        }
      }
    }
    /* no break */
    default: break;
  }
  return true;
}

bool IPCamControlServer::get_netdev_info ()
{
  struct ifaddrs * interfaces = 0;
  struct ifaddrs * temp_if    = 0;
  int fd                      = -1;
  struct ifreq if_req;

  if ((fd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    perror ("NetDevInfo: Get MAC address: can't create socket");
    return false;
  }
  if (getifaddrs (&interfaces) < 0) {
    perror ("Cannot get network interface info");
    if (mNetDevInfo) {
      delete mNetDevInfo;
      mNetDevInfo = 0;
    }
    return false;
  }
  /* Get Network Device and it's IP Address */
  for (temp_if = interfaces; temp_if; temp_if = temp_if->ifa_next) {
    if (!(temp_if->ifa_flags & IFF_LOOPBACK)) {
      NetDevInfo * devInfo = NULL;
      /* If the interface name is in the form like eth0:avahi
       * remove the ":avahi" substring
       */
      char * tail = strstr(temp_if->ifa_name, ":avahi");
      if (tail) {
        *tail = '\0';
      }
      if (mNetDevInfo) {
        devInfo = mNetDevInfo->find_netdev_by_name (temp_if->ifa_name);
        if (!devInfo) {
          devInfo     = new NetDevInfo ();
          mNetDevInfo = mNetDevInfo->insert_netdev_info (devInfo);
        }
        devInfo->set_netdev_name (temp_if->ifa_name);
        devInfo->set_netdev_ifindex (if_nametoindex(temp_if->ifa_name));
      } else {
        mNetDevInfo = new NetDevInfo ();
        mNetDevInfo->set_netdev_name (temp_if->ifa_name);
        mNetDevInfo->set_netdev_ifindex (if_nametoindex(temp_if->ifa_name));
        devInfo = mNetDevInfo;
      }
      /* Get IP Address and MAC address */
      if (temp_if->ifa_addr != NULL) {
        char ip_str[256] = {0};
        switch (temp_if->ifa_addr->sa_family) {
          case AF_INET  : {
            struct sockaddr_in * ipv4 = (struct sockaddr_in *)temp_if->ifa_addr;
            inet_ntop (AF_INET, (void *)&(ipv4->sin_addr), ip_str, 256);
            devInfo->set_netdev_ipv4 (ip_str);
          }break;
          case AF_INET6 : {
            struct sockaddr_in6 * ipv6 =
                (struct sockaddr_in6 *)temp_if->ifa_addr;
            inet_ntop (AF_INET6, (void *)&(ipv6->sin6_addr), ip_str, 256);
            devInfo->set_netdev_ipv6 (ip_str);
          }
          /* no break */
          default: break;
        }
        /* Get MAC Address for Every Active Network Device */
        memset (&if_req, 0, sizeof(struct ifreq));
        strcpy (if_req.ifr_name, devInfo->get_netdev_name ());
        if (ioctl(fd, SIOCGIFHWADDR, &if_req) < 0) {
          perror ("NetDevInfo: Get MAC address: ioctl error");
          return false;
        } else {
          char mac[36] = {0};
          for (int i = 0; i < 6; ++ i) {
            sprintf (&mac[i*2],
                     "%02x",
                     (unsigned char)if_req.ifr_hwaddr.sa_data[i]);
          }
          devInfo->set_netdev_mac (mac);
#ifdef DEBUG
          fprintf (stderr,
                   "Found device %s:%s\n",
                   devInfo->get_netdev_name(),
                   mac);
#endif
        }
      }
      /* Get network mask information */
      if (temp_if->ifa_netmask != NULL) {
        char mask_str[256] = {0};
        switch (temp_if->ifa_netmask->sa_family) {
          case AF_INET  : {
            struct sockaddr_in * maskv4 =
                (struct sockaddr_in *)temp_if->ifa_netmask;
            inet_ntop (AF_INET, (void *)&(maskv4->sin_addr), mask_str, 256);
            devInfo->set_netdev_maskv4 (mask_str);
          }break;
          case AF_INET6 : {
            struct sockaddr_in6 * maskv6 =
                (struct sockaddr_in6 *)temp_if->ifa_netmask;
            inet_ntop (AF_INET6, (void *)&(maskv6->sin6_addr), mask_str, 256);
            devInfo->set_netdev_maskv6 (mask_str);
          }
          /* no break */
          default: break;
        }
      }
    }
  }
  freeifaddrs (interfaces);
  return true;
}

inline bool IPCamControlServer::check_mac (const char *mac)
{
  if (mNetDevInfo && mac) {
    return mNetDevInfo->check_mac (mac);
  }
  return false;
}

bool IPCamControlServer::check_ip (const char *mac, const char *ip)
{
  /* true means this network device already has this IP address
   * false means this network device doesn't have this IP address
   */
#define min_len(a, b) ((strlen(a) <= strlen(b)) ? strlen(a) : strlen(b))
  if (mNetDevInfo && mac && ip) {
    NetDevInfo *devInfo = mNetDevInfo->find_netdev_by_mac(mac);
    if (devInfo) {
      char * ipv4 = devInfo->get_netdev_ipv4();
      char * ipv6 = devInfo->get_netdev_ipv6();
      if ((ipv4 && (0 != strncmp(ipv4, ip, min_len(ipv4, ip)))) ||
          (ipv6 && (0 != strncmp(ipv6, ip, min_len(ipv6, ip))))) {
        return false;
      }
    }
  }

  return true;
}

bool IPCamControlServer::check_ip_existence (int protocol)
{
  if (mNetDevInfo == 0) {
    return false;
  }
  for (NetDevInfo * devInfo = mNetDevInfo;
       devInfo;
       devInfo = devInfo->next()) {
    switch (protocol) {
      case AF_INET: {
        if (devInfo->get_netdev_ipv4 () != 0) {
          return true;
        }
      }break;
      case AF_INET6: {
        if (devInfo->get_netdev_ipv6 () != 0) {
          return true;
        }
      }break;
      default:
        return false;
    }
  }
  return false;
}

NetworkData * IPCamControlServer::get_data (int socket, int protocol)
{
  NetworkData * data_recv = new NetworkData (protocol);
  socklen_t addrlen = data_recv->src_addr_size ();
  if (recvfrom (socket,
                data_recv->buf_addr(),
                data_recv->buf_size(),
                0,
                data_recv->get_src_addr (),
                &addrlen) <= 0) {
    delete data_recv;
    return 0;
  }
  return data_recv;
}

bool IPCamControlServer::load_ucode ()
{
  ucode_load_info_t info;
  unsigned char *   ucode_mem;
  int               fd;
  if ((fd = open("/dev/ucode", O_RDWR, 0)) < 0) {
    perror ("/dev/ucode");
    return false;
  }
  if (ioctl(fd, IAV_IOC_GET_UCODE_INFO, &info) < 0) {
    perror ("IAV_IOC_GET_UCODE_INFO");
    return false;
  }
  ucode_mem = (unsigned char *)mmap(0,
                                    info.map_size,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,
                                    fd,
                                    0);
  if ((int)ucode_mem == -1) {
    perror ("mmap");
    return false;
  }
  for (int i = 0; i < info.nr_item; ++ i) {
    unsigned char * addr = ucode_mem + info.items[i].addr_offset;
    char filename[256]   = {0};
    FILE * fp            = 0;
    int file_length      = 0;
    sprintf (filename, "/lib/firmware/%s", info.items[i].filename);
    if ((fp = fopen(filename, "rb")) == 0) {
      perror ("filename");
      if (munmap (ucode_mem, info.map_size) < 0) {
        perror ("munmap");
      }
      close(fd);
      return false;
    }
    if (fseek(fp, 0, SEEK_END) < 0) {
      perror ("SEEK_END");
      if (munmap (ucode_mem, info.map_size) < 0) {
        perror ("munmap");
      }
      fclose (fp);
      close(fd);
      return false;
    }
    file_length = ftell (fp);
    if (fseek(fp, 0, SEEK_SET) < 0) {
      perror ("SEEK_SET");
      if (munmap (ucode_mem, info.map_size) < 0) {
        perror ("munmap");
      }
      fclose (fp);
      close(fd);
      return false;
    }
    if (fread(addr, file_length, 1, fp) != 1) {
      perror("fread");
      if (munmap (ucode_mem, info.map_size) < 0) {
        perror ("munmap");
      }
      fclose (fp);
      close(fd);
      return false;
    }
    fclose (fp);
  }
  if (ioctl(fd, IAV_IOC_UPDATE_UCODE, 0) < 0) {
    perror ("IAV_IOC_UPDATE_UCODE");
    if (munmap (ucode_mem, info.map_size) < 0) {
      perror ("munmap");
    }
    close(fd);
    return false;
  }
  if (munmap(ucode_mem, info.map_size) < 0) {
    perror("munmap");
  }
  close(fd);
  mUcodeLoaded = true;
  return true;
}

bool IPCamControlServer::check_kernel_module (const char * module)
{
  if (module) {
    char command[128] = {0};
    FILE * fd         = 0;
    sprintf (command, "lsmod | grep ^%s", module);
    fd = popen (command, "r");
    if (fd) {
      char result[64] = {0};
      fread (result, 64, 1, fd);
      if (strlen (result) != 0) {
        pclose (fd);
        return true;
      } else {
        memset (command, 0, sizeof(command));
        sprintf (command, "modprobe %s", module);
        if (0 == system (command)) {
          pclose (fd);
          return true;
        }
      }
    }
    if (fd) {
      pclose (fd);
    }
  }
  return false;
}

bool IPCamControlServer::install_kernel_modules ()
{
  if (access(CONFIG_DIR, F_OK) == 0) {
    FILE * module_config_fp = fopen (CONFIG_DIR"module.conf", "r");
    if (module_config_fp) {
      char buffer[16] = {0};
      /* Read config first */
      fscanf (module_config_fp, "AutoLoad:%s", buffer);
      if (strcasecmp(buffer, "False") == 0) {
        /* if Auto Load is set to false, just return false */
        return false;
      } else if (strcasecmp(buffer, "True") == 0) {
        /* if Auto Load is set to true, start to read module and load them */
        char module[32] = {0};
        while (EOF != fscanf (module_config_fp, "%s", module)) {
          if (!check_kernel_module(module)) {
            fprintf (stderr, "Failed to install kernel module %s\n", module);
            return false;
          }
          memset (module, 0, 32);
        }
        /* When kernel module is loaded successfully, load ucode */
        if (false == mUcodeLoaded) {
          mUcodeLoaded = load_ucode ();
        }
        return mUcodeLoaded;
      }
    } else {
      perror (CONFIG_DIR"module.conf");
    }
  }
  return false;
}

bool IPCamControlServer::check_sensor_driver (SensorModuleInfo * module)
{
  if (module) {
    char cmd [64] = {0};
    sprintf (cmd, "modprobe %s", module->get_module_name ());
    if (0 == system (cmd)) {
      int debug_fd = open ("/dev/ambad", O_RDWR, 0);
      unsigned int id = 0x0;
      sprintf (cmd, "rmmod %s", module->get_module_name ());
      if (debug_fd < 0) {
        perror ("/dev/ambad");
        fprintf (stderr, "Remove %s\n", module->get_module_name ());
        system (cmd);
        return false;
      }
      if (ioctl (debug_fd, AMBA_DEBUG_IOC_VIN_GET_DEV_ID, &id) < 0) {
        perror ("AMBA_DEBUG_IOC_VIN_GET_DEV_ID");
        fprintf (stderr, "Remove %s\n", module->get_module_name ());
        system (cmd);
        close (debug_fd);
        return false;
      }
      close (debug_fd);
      /* Hight 16bit is Sensor ID, Low 16bit is Sensor Version */
      id = (id >> 16) | 0x0;
      if (id == module->get_device_id ()) {
        fprintf (stdout,
                 "Device ID of %s is %x\n",
                 module->get_module_name(),
                 id);
        return true;
      } else {
        fprintf (stderr,
                 "Device ID 0x%x doesn't match module driver's ID 0x%x\n"
                 "Remove %s\n",
                 id,
                 module->get_device_id(),
                 module->get_module_name ());
        system (cmd);
        return false;
      }
    } else {
      perror (cmd);
    }
  }
  return false;
}

void IPCamControlServer::install_sensor_driver ()
{
  FILE * sensor_config_fp = 0;
  int sensor_num =
    (int)(sizeof(mSensorModuleInfoList)/sizeof(SensorModuleInfo));
  if (mDriverState != UNINSTALLED) {
    return ;
  }
  mDriverState = INSTALLING;
#ifdef DEBUG
  fprintf (stderr, "IPCamControlServer::install_sensor_driver!\n");
#endif
  if (access (CONFIG_DIR, F_OK) == 0) {
    if ((sensor_config_fp = fopen(CONFIG_DIR"sensor.conf", "r")) != 0) {
      char sensor_module[32] = {0};
      unsigned int sensor_id = 0x0;
      SensorModuleInfo * module_info = 0;
      fscanf (sensor_config_fp, "%x:%s", &sensor_id, sensor_module);
      if ((sensor_id == 0) && (strcmp(sensor_module, "disabled") == 0)) {
        /* When sensor_id is set to 0 and sensor_module is disabled,
         * just return, don't load any sensor driver*/
        return;
      }
      module_info = find_module_info (sensor_module, sensor_id);
      if (module_info && check_sensor_driver (module_info)) {
        mDriverState = INSTALLED;
        fclose (sensor_config_fp);
        return;
      }
    }
  } else {
    if (access (CONFIG_ROOT_DIR, F_OK) != 0) {
      if (mkdir (CONFIG_ROOT_DIR, 755) < 0) {
#ifdef DEBUG
        perror ("mkdir "CONFIG_ROOT_DIR);
#endif
        mDriverState = UNINSTALLED;
        return;
      }
    }
    if (mkdir (CONFIG_DIR, 755) < 0) {
#ifdef DEBUG
      perror ("mkdir "CONFIG_DIR);
#endif
      mDriverState = UNINSTALLED;
      return;
    }
  }
  /* When reaching here, it means no config file has been read
   * or different sensor has been installed,
   * so we need to detect sensor driver again
   */
#ifdef DEBUG
  fprintf (stderr, "Starting to detect sensor!\n");
#endif
  for (int count = 0; count < sensor_num; ++ count) {
    if (check_sensor_driver(&mSensorModuleInfoList[count]) == true) {
      mDriverState = INSTALLED;
      sensor_config_fp = fopen (CONFIG_DIR"sensor.conf", "w");
      if (sensor_config_fp) {
        fprintf (sensor_config_fp,
          "0x%x:%s\n", mSensorModuleInfoList[count].get_device_id(),
          mSensorModuleInfoList[count].get_module_name());
        fclose (sensor_config_fp);
      }
      fprintf (stderr,
        "Suitable sensor module found: 0x%x:%s\n",
        mSensorModuleInfoList[count].get_device_id(),
        mSensorModuleInfoList[count].get_module_name());
      return;
    }
  }
  /* No suitable sensor driver has been found,
   * just mark the sensor status to UNINSTALLED
   */
  fprintf (stderr, "Sensor detect failed!\n");
  mDriverState = UNINSTALLED;
}

SensorModuleInfo * IPCamControlServer::find_module_info (const char * name,
                                                         unsigned int id)
{
  int sensor_num = (int)(sizeof(mSensorModuleInfoList) /
                         sizeof(struct SensorModuleInfo));
  for (int count = 0; count < sensor_num; ++ count) {
    if ((id == mSensorModuleInfoList[count].get_device_id()) &&
      (0 == strncmp(name,
                    mSensorModuleInfoList[count].get_module_name(),
                    strlen(name)))) {
      return &mSensorModuleInfoList[count];
    }
  }
  return 0;
}

void * IPCamControlServer::start_server (void * data)
{
  IPCamControlServer * server = (IPCamControlServer *)data;
  if (server) {
    server->run_server();
  }
  return 0;
}

void IPCamControlServer::run_server ()
{
  bool isSocketOK = false;
  bool isIPv4OK   = false;
  bool isIPv6OK   = false;
  int count       = 0;
  if (!mInitialized) {
    /* server is not initialized,
     * kill itself
     */
    pid_t pid = getpid();
    kill (pid, SIGTERM);
  }
  /* We must make sure that at least one IP address has been assigned to
   * at least one network interface
   */
  while ((count ++) < 20) {
    isIPv4OK = check_ip_existence (AF_INET);
    isIPv6OK = check_ip_existence (AF_INET6);
    if (isIPv4OK || isIPv6OK) {
      break;
    } else {
      get_netdev_info ();
    }
    /* Sleep for 1 second to wait for avahi-autoip to assign ip address */
    sleep (1);
  }
  if (isIPv4OK && init_socket(AF_INET)) {
    isSocketOK = true;
  }
  if (isIPv6OK && init_socket(AF_INET6)) {
    isSocketOK = true;
  }
  if (isSocketOK) {
    fd_set fd_sets;
    sigset_t sigset_sigs;
    sigset_t sigset_empty;

    sigemptyset (&sigset_sigs);
    sigemptyset (&sigset_empty);
    sigaddset (&sigset_sigs, SIGTERM);
    sigaddset (&sigset_sigs, SIGINT);
    sigaddset (&sigset_sigs, SIGQUIT);
    mRunning = true;
    sigprocmask (SIG_BLOCK, &sigset_sigs, NULL);
    while (mRunning) {
      int max_fd = -1;
      FD_ZERO (&fd_sets);
      if (mMulticastSockV4 > 0) {
        max_fd = (max_fd > mMulticastSockV4 ? max_fd : mMulticastSockV4);
#ifdef DEBUG
        fprintf (stderr, "IPv4 Multicast socket fd is %d\n", mMulticastSockV4);
#endif
        FD_SET (mMulticastSockV4, &fd_sets);
      }
      if (mUnicastSockV4 > 0) {
#ifdef DEBUG
        fprintf (stderr, "IPv4 Unicast socket fd is %d\n", mUnicastSockV4);
#endif
        max_fd = (max_fd > mUnicastSockV4 ? max_fd : mUnicastSockV4);
        FD_SET (mUnicastSockV4, &fd_sets);
      }
      if (mMulticastSockV6 > 0) {
#ifdef DEBUG
        fprintf (stderr, "IPv6 Multicast socket fd is %d\n", mMulticastSockV6);
#endif
        max_fd = (max_fd > mMulticastSockV6 ? max_fd : mMulticastSockV6);
        FD_SET (mMulticastSockV6, &fd_sets);
      }
      if (mUnicastSockV6 > 0) {
#ifdef DEBUG
        fprintf (stderr, "IPv6 Unicast socket fd is %d\n", mUnicastSockV6);
#endif
        max_fd = (max_fd > mUnicastSockV6 ? max_fd : mUnicastSockV6);
        FD_SET (mUnicastSockV6, &fd_sets);
      }
#ifdef DEBUG
      fprintf (stderr, "max_fd is %d\n", max_fd);
#endif
      if (pselect (max_fd + 1, &fd_sets, 0, 0, 0, &sigset_empty) > 0) {
        NetworkData * data = 0;
        if ((mMulticastSockV4 > 0) && FD_ISSET (mMulticastSockV4, &fd_sets)) {
          data = get_data (mMulticastSockV4, AF_INET);
          if (data) {
            fprintf (stderr, "IPv4 get Multicast data from %s\n",
              data->get_net_config_data()->getMac());
          }
          process_network_data (mMulticastSockV4, data, AF_INET);
        }
        if ((mUnicastSockV4 > 0) && FD_ISSET (mUnicastSockV4, &fd_sets)) {
          data = get_data (mUnicastSockV4, AF_INET);
          if (data) {
            fprintf (stderr, "IPv4 get UDP data from %s\n",
              data->get_net_config_data()->getMac());
          }
          process_network_data (mUnicastSockV4, data, AF_INET);
        }
        if ((mMulticastSockV6 > 0) && FD_ISSET (mUnicastSockV6, &fd_sets)) {
          data = get_data (mMulticastSockV6, AF_INET6);
          if (data) {
            fprintf (stderr, "IPv6 get Multicast data from %s\n",
              data->get_net_config_data()->getMac());
          }
          process_network_data (mMulticastSockV6, data, AF_INET6);
        }
        if ((mUnicastSockV6 > 0) && FD_ISSET (mUnicastSockV6, &fd_sets)) {
          data = get_data (mUnicastSockV6, AF_INET6);
          if (data) {
            fprintf (stderr, "IPv6 get UDP data from %s\n",
              data->get_net_config_data()->getMac());
          }
          process_network_data (mUnicastSockV6, data, AF_INET6);
        }
      }
    }
  } else {
    /* Server is not running due to network failure, just kill itself */
    pid_t pid = getpid();
    kill (pid, SIGTERM);
  }
}

void IPCamControlServer::process_network_data (int socket,
                                               NetworkData * networkData,
                                               int protocol)
{
  if (networkData) {
    NetCmdType command = networkData->get_data_type ();
    switch (command) {
      case DHCP:
      case STATIC: {
        change_ip_address (networkData->get_net_config_data(), protocol);
        get_netdev_info ();
      }break;
      case QUERY: {
        NetConfigData * netConfigData = networkData->get_net_config_data();
        if (netConfigData) {
          fprintf (stderr,
            "The MAC address to be checked is %s\n",
            netConfigData->getMac());
          if (check_mac(netConfigData->getMac())) {
            if (sendto (socket,
                "TRUE",
                4,
                0,
                networkData->get_src_addr(),
                networkData->src_addr_size()) < 0) {
              perror ("TRUE Sendto error");
            }
          } else {
            if (sendto (socket,
                "FALSE",
                5,
                0,
                networkData->get_src_addr(),
                networkData->src_addr_size()) < 0) {
              perror ("FALSE Sendto error");
            }
          }
        }
      }break;
      case RTSP: {
        pthread_t thread;
        ThreadInfo * info = new ThreadInfo(this, 0);
        if (0 != pthread_create (&thread, 0, detect_sensor, info)) {
          perror ("Failed to start Sensor Detecting Thread");
          delete info;
        }
        /* Sleep 3 seconds to wait for driver loading */
        sleep (3);
        enable_media_server (socket, networkData);
      }break;
      case REBOOT: {
        if (sendto (socket,
                    "REBOOT",
                    6,
                    0,
                    networkData->get_src_addr(),
                    networkData->src_addr_size()) < 0) {
          perror ("REBOOT Sendto error");
        } else {
          reboot ();
        }
      }break;
      case GET_ENCODE_SETTING: {
        EncodeSetting * encode_setting = new EncodeSetting();
#ifdef DEBUG
        fprintf (stderr, "Processing GET_ENCODE_SETTING\n");
#endif
        if (mEncodeServerConfig->get_encode_setting(encode_setting)) {
          if (sendto(socket,
              encode_setting,
              sizeof(EncodeSetting),
              0,
              networkData->get_src_addr(),
              networkData->src_addr_size()) != sizeof(EncodeSetting)) {
            perror ("Sending EncodeSettings error");
          } else {
#ifdef DEBUG
            fprintf (stderr, "Sent %d bytes of EncodeSetting\n",
              sizeof(EncodeSetting));
#endif
          }
        }
        if (encode_setting) {
          delete encode_setting;
        }
      }break;
      case NETBOOT: {
        NetConfigData * netConfigData = networkData->get_net_config_data();
        if (check_mac(netConfigData->getMac())) {
#if defined(CONFIG_ARCH_A5S)
          uint32_t priAddr = 0xc0100000;
#elif defined(CONFIG_ARCH_I1)
          uint32_t priAddr = 0x00100000;
#endif
          NetDevInfo * devInfo =
              mNetDevInfo->find_netdev_by_mac(netConfigData->getMac());
          NandWrite nandWrite;
          uint32_t src = ((struct sockaddr_in *)networkData->get_src_addr())->\
              sin_addr.s_addr;
          /* We have to ensure that MAC address is set in AMBoot */
          if (nandWrite.setNetbootParameters(devInfo->get_netdev_ipv4(),
                                             devInfo->get_netdev_maskv4(),
                                             src,
                                             src,
                                             devInfo->get_netdev_mac(),
                                             "AutoNetboot",
                                             priAddr)) {
            if (sendto (socket,
                        "NETBOOT",
                        7,
                        0,
                        networkData->get_src_addr(),
                        networkData->src_addr_size()) < 0) {
              perror ("NETBOOT Sendto error");
            } else {
              sleep (1);
              reboot ();
            }
          } else {
            if (sendto (socket,
                        "NETBOOTERR",
                        10,
                        0,
                        networkData->get_src_addr(),
                        networkData->src_addr_size()) < 0) {
              perror ("NETBOOTERR, Sendto error");
            }
          }
        }
      }break;
      default:break;
    }
    delete networkData;
  }
}

void IPCamControlServer::change_ip_address (NetConfigData * netConfigData,
                                            int protocol)
{
  if (netConfigData && check_mac (netConfigData->getMac())) {
    /* Only if the netConfigData contains the MAC address
     * which belongs to this IP camera's network device
     * we can go on processing
     */
    bool changeMac = ((strlen(netConfigData->getNewmac()) != 0) &&
                      (strcasecmp(netConfigData->getNewmac(),
                                  netConfigData->getMac()) != 0)) ? true : false;
    NetDevInfo * devInfo =
        mNetDevInfo->find_netdev_by_mac (netConfigData->getMac());
    char command[512] = {0};
    switch (netConfigData->command) {
      case DHCP: {
        sprintf (command, "sed -e '/^iface\\ %s/s/static/dhcp/'"
                 " -e '/^auto\\ %s/,/^auto/{ /address/d;/netmask/d;/gateway/d }'"
                 " /etc/network/interfaces > /tmp/interfaces.tmp &&"
                 " mv /tmp/interfaces.tmp /etc/network/interfaces &&"
                 " cat /dev/null > /etc/resolv.conf",
                 devInfo->get_netdev_name(), devInfo->get_netdev_name());
#ifdef DEBUG
        fprintf (stderr, "DHCP command is %s\n", command);
#endif
      }break;
      case STATIC: {
        /* If the IP address in netConfigData != this camera's IP address
         * go on setting
         */
        if (false == check_ip(netConfigData->getMac(), netConfigData->getIp())) {
          char * dev_name = devInfo->get_netdev_name();
          sprintf (
              command, "sed -e '/^iface\\ %s/s/dhcp/static/'"
              " -e '/^auto\\ %s/,/^auto/{ /address/d;/netmask/d;/gateway/d }'"
              " -e '/iface\\ %s\\ inet\\ static/aaddress %s'"
              " -e '/iface\\ %s\\ inet\\ static/anetmask %s'"
              " -e '/iface\\ %s\\ inet\\ static/agateway %s'"
              " /etc/network/interfaces > /tmp/interfaces.tmp &&"
              " mv /tmp/interfaces.tmp /etc/network/interfaces &&"
              " echo nameserver %s %s %s > /tmp/resolv.conf.tmp &&"
              " mv /tmp/resolv.conf.tmp /etc/resolv.conf",
              dev_name,
              dev_name,
              dev_name,
              netConfigData->getIp(),
              dev_name,
              netConfigData->getNetmask(),
              dev_name,
              netConfigData->getGateway(),
              netConfigData->getdns1(),
              netConfigData->getdns2(),
              netConfigData->getdns3());
#ifdef DEBUG
          fprintf (stderr, "STATIC command is %s\n", command);
#endif
        } else {
          command[0] = '\0';
        }
      }
      /* no break */
      default:break;
    }
    if ((strlen(command) > 0) && (0 == system(command))) {
      char cmd[128] = {0};
      sprintf (cmd,
               "/etc/init.d/S98avahi-daemon stop;"
               "/etc/init.d/S40network restart;"
               "/etc/init.d/S98avahi-daemon start");
      if (0 == system (cmd)) {
        edit_route (devInfo, true, protocol);
      }
    }
    if (changeMac) {
      /*Change MAC address*/
      NandWrite nandWrite;
      if (nandWrite.changeMacAddr(netConfigData->getNewmac())) {
        reboot ();
      }
#ifdef DEBUG
      else {
        fprintf (stderr, "DEBUG: NandWrite::changeMacAddr Failed!\n");
      }
#endif
    }
  }
}

void IPCamControlServer::enable_media_server (int socket,
                                              NetworkData * networkData)
{
  if (networkData && mUcodeLoaded) {
    bool isMediaServerUp = false;
    switch (mDriverState) {
      case INSTALLED: {
        FILE * fp = popen ("ps | grep mediaserver | grep -v grep", "r");
        if (fp) {
          char buffer[128] = {0};
          fread (buffer, 128, 1, fp);
          if (strlen(buffer) != 0) {
            mMediaServerRunning = true;
          } else {
            mMediaServerRunning = false;
          }
          pclose(fp);
        }
        if (!mMediaServerRunning) {
          if (0 == system ("/usr/local/bin/mediaserver -e -r -i&")) {
            isMediaServerUp = true;
            mMediaServerRunning = true;
          }
        }
        if (mMediaServerRunning) {
          if (isMediaServerUp) {
            /* If media server is just enabled
             * sleep 2 seconds to wait for mediaserver */
            sleep (2);
          }
          /* Send RTSPOK to client */
          sendto(socket,
            "RTSPOK",
            6,
            0,
            networkData->get_src_addr(),
            networkData->src_addr_size());
        } else {
          sendto(socket,
            "RTSPFAILED",
            10,
            0,
            networkData->get_src_addr(),
            networkData->src_addr_size());
        }
      }break;
      case INSTALLING: {
        sendto(socket,
               "DRVINSTALLING",
               13,
               0,
               networkData->get_src_addr(),
               networkData->src_addr_size());
      }break;
      case UNINSTALLED: {
        sendto(socket,
          "DRVFAILED",
          9,
          0,
          networkData->get_src_addr(),
          networkData->src_addr_size());
      }
      /* no break */
      default:break;
    }
  } else if (!mUcodeLoaded) {
    sendto(socket,
      "UCODEERR",
      8,
      0,
      networkData->get_src_addr(),
      networkData->src_addr_size());
  }
}

void * IPCamControlServer::detect_sensor (void * data)
{
  if (data) {
    ThreadInfo * info = (ThreadInfo *)data;
    IPCamControlServer * server = info->get_server ();
    delete info;
    if (server && server->install_kernel_modules ()) {
      server->install_sensor_driver ();
    }
  }
  return 0;
}
