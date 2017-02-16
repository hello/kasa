/**********************************************************************
 * IPCamControlServer.h
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
#ifndef IPCAM_CTRL_SERVER_H
#define IPCAM_CTRL_SERVER_H

#include <netinet/in.h>

#include "NetDevInfoStruct.h"
#include "SensorModuleInfo.h"
#include "DataStructure.h"
#include "EncodeServerConfig.h"

static SensorModuleInfo mSensorModuleInfoList[] = {
  SensorModuleInfo("ov2710"     , 0x2710), /* Sensor OV2710 && OV2715 */
  SensorModuleInfo("ov9710"     , 0x9711), /* Sensor OV9710 && OV9715 */
  SensorModuleInfo("ov5653"     , 0x5611), /* Sensor OV5653           */
  SensorModuleInfo("mt9m033"    , 0x2400), /* Sensor MT9M033          */
  SensorModuleInfo("mt9t002"    , 0x2604), /* Sensor MT9T002          */
  SensorModuleInfo("ov14810"    , 0xe810), /* Sensor OV14810          */
  SensorModuleInfo("ov10630_yuv", 0xa630), /* Sensor OV16030 YUV Mode */
  SensorModuleInfo("ov10630_rgb", 0xa630), /* Sensor OV16030 RGB Mode */
  SensorModuleInfo("ov10620"    , 0xa620), /* Sensor OV10620          */
};

class IPCamControlServer {
public:
  IPCamControlServer         ( );
  ~IPCamControlServer        ( );
public:
  enum SensorDriverState {UNINSTALLED, INSTALLED, INSTALLING};
  bool is_server_running      () {return mRunning;}
  bool is_server_initialized  () {return mInitialized;}
  static void * start_server  (void * data);
  void stop_server            () {mRunning = false;}
private:
  void run_server             ();
  bool init_socket            (int protocol);
  bool get_netdev_info        ();
  bool check_mac              (const char * mac);
  bool check_ip               (const char * mac, const char * ip);
  bool check_ip_existence     (int protocol );
  NetworkData * get_data      (int socket, int protocol);
  void reboot                 ();
  bool install_kernel_modules ();
  void install_sensor_driver  ();
  void destroy                () {delete this;}
  bool edit_route             (NetDevInfo * devInfo,
                               bool action,
                               int protocol);
  bool load_ucode             ();
  bool check_kernel_module    (const char * module);
  bool check_sensor_driver    (SensorModuleInfo * module);
  void process_network_data   (int socket,
                               NetworkData * networkData,
                               int protocol);
  void change_ip_address      (NetConfigData * netConfigData,
                               int protocol);
  void enable_media_server    (int socket, NetworkData * networkData);
  SensorModuleInfo * find_module_info (const char * name, unsigned int id);
  static void * detect_sensor         (void * data);
private:
  NetDevInfo   * mNetDevInfo;
  int            mMulticastSockV4;
  int            mUnicastSockV4;
  int            mMulticastSockV6;
  int            mUnicastSockV6;
  int            mLocalSock;
  bool           mRunning;
  bool           mInitialized;
  bool           mUcodeLoaded;
  bool           mMediaServerRunning;
  struct sockaddr_in  mMultiGroupAddrV4;
  struct sockaddr_in6 mMultiGroupAddrV6;
  struct sockaddr_in  mUnicastAddrV4;
  struct sockaddr_in6 mUnicastAddrV6;
  struct sockaddr_in  mLocalAddr;
  SensorDriverState   mDriverState;
  EncodeServerConfig * mEncodeServerConfig;

#define CONFIG_ROOT_DIR "/etc/ambaipcam/"
#define CONFIG_DIR      "/etc/ambaipcam/controlserver/"
#define MULTICAST_ADDR_IPV4 "238.237.236.235"
#define MULTICAST_ADDR_IPV6 "ff12::238.237.236.235"
#define LOCAL_ADDR          "127.0.0.1"
#define MULTICAST_PORT 5555
#define UNICAST_PORT   5566
#define LOCAL_PORT     20000
};

struct ThreadInfo {
  IPCamControlServer * server;
  void * data;
  ThreadInfo () {
    server = 0;
    data = 0;
  }
  ThreadInfo (IPCamControlServer * svr, void * dt) {
    server = svr;
    data = dt;
  }
  void set_server (IPCamControlServer * svr) {
    server = svr;
  }
  IPCamControlServer * get_server () {
    return server;
  }
  void set_data (void * dt) {
    data = dt;
  }
  void * get_data () {
    return data;
  }
};

#endif //IPCAM_CTRL_SERVER_H
