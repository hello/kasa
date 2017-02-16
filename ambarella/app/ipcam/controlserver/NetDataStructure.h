/**********************************************************************
 * NetDataStructure.h
 *
 * Histroy:
 *  2011年03月17日 - [Yupeng Chang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 **********************************************************************/
#ifndef NETWORKDATASTRUCTURE_H
#define NETWORKDATASTRUCTURE_H
#include <string.h>
#include "NetCommand.h"
struct NetConfigData {
  NetCmdType command;
  char mac     [16];
  char ip      [40];
  char netmask [40];
  char gateway [40];
  char dns1    [40];
  char dns2    [40];
  char dns3    [40];
  char newmac  [16];

  /* Constructor */
  NetConfigData ()
  {
    command = NOACTION;
    memset (mac    , 0, 16);
    memset (ip     , 0, 40);
    memset (netmask, 0, 40);
    memset (gateway, 0, 40);
    memset (dns1   , 0, 40);
    memset (dns2   , 0, 40);
    memset (dns3   , 0, 40);
    memset (newmac , 0, 16);
  }

  /* Methods */
  void setIp (const char * ipAddr)
  {
    memset (ip, 0, 40);
    strcpy (ip, ipAddr);
  }

  void setMac (const char * macAddr)
  {
    memset (mac, 0, 16);
    strcpy (mac, macAddr);
  }

  void setGateway (const char * gw)
  {
    memset (gateway, 0, 40);
    strcpy (gateway, gw);
  }

  void setNetmask (const char * nm)
  {
    memset (netmask, 0, 40);
    strcpy (netmask, nm);
  }

  void setdns1 (const char * dnsOne)
  {
    memset (dns1, 0, 40);
    strcpy (dns1, dnsOne);
  }

  void setdns2 (const char * dnsTwo)
  {
    memset (dns2   , 0, 40);
    strcpy (dns2, dnsTwo);
  }

  void setdns3 (const char * dnsThree)
  {
    memset (dns3, 0, 40);
    strcpy (dns3, dnsThree);
  }

  void setnewmac (const char * mac)
  {
    memset (newmac, 0, 16);
    strcpy (newmac, mac);
  }

  const char * getIp () {return ip;}
  const char * getMac () {return mac;}
  const char * getGateway () {return gateway;}
  const char * getNetmask () {return netmask;}
  const char * getdns1 () {return dns1;}
  const char * getdns2 () {return dns2;}
  const char * getdns3 () {return dns3;}
  const char * getNewmac () {return newmac;}
};

#endif // NETWORKDATASTRUCTURE_H
