/**********************************************************************
 * DataStructure.h
 *
 * Histroy:
 *  2011年03月22日 - [Yupeng Chang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 **********************************************************************/
#ifndef APP_IPCAM_CONTROLSERVER_DATASTRUCTURE_H
#define APP_IPCAM_CONTROLSERVER_DATASTRUCTURE_H
#include "NetDataStructure.h"
#include "VideoDataStructure.h"

#define max(a,b) ((a) > (b) ? (a) : (b))
/* Struct ConfigData is common type of NetConfigData and VideoConfigData */
struct ConfigData {
  NetCmdType cmd;
  char data[];
};

struct NetworkData {
  int buffer_size;
  struct sockaddr src_addr;
  ConfigData * config_data;

  /* Constructor */
  NetworkData (int protocol) {
    buffer_size = max(sizeof(VideoConfigData), sizeof(NetConfigData));
    memset (&src_addr, 0, sizeof(struct sockaddr));
    config_data = (ConfigData *)malloc(buffer_size*sizeof(char));
    memset (config_data, 0, buffer_size);
    src_addr.sa_family = protocol;
  }
  ~NetworkData () {
    if (config_data) {
      delete config_data;
    }
  }
  /* Methods */
  void * buf_addr () {
    return (void *)config_data;
  }
  size_t buf_size () {
    return buffer_size;
  }
  struct sockaddr * get_src_addr () {
    return &src_addr;
  }
  socklen_t src_addr_size () {
    return (socklen_t)sizeof(struct sockaddr);
  }
  NetCmdType get_data_type () {
    return config_data->cmd;
  }
  NetConfigData * get_net_config_data() {
    return (NetConfigData *)config_data;
  }
  VideoConfigData * get_video_config_data () {
    return (VideoConfigData *)config_data;
  }
};
#endif //APP_IPCAM_CONTROLSERVER_DATASTRUCTURE_H

