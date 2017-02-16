/**********************************************************************
 * VideoDataStructure.h
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
#ifndef APP_IPCAM_CONTROLSERVER_VIDEODATASTRUCTURE_H
#define APP_IPCAM_CONTROLSERVER_VIDEODATASTRUCTURE_H

#include "mw_struct.h"
#include "NetCommand.h"
#include <string.h>

struct EncodeInfo {
  mw_encode_format encode_format;               /* Encode format info */
  union {
    mw_h264_config _h264;
    mw_jpeg_config _jpeg;
  } encode_type;                                /* Encode type H.264 or Mjpeg */
  void clean () {
    memset (&encode_type, 0, sizeof(encode_type));
    memset (&encode_format, 0, sizeof(mw_encode_format));
  }
  bool get_encode_info (int iav_fd, unsigned int stream_id);
};

struct EncodeSetting {
  bool stream_state[MW_MAX_STREAM_NUM];         /* STREAM STATUS  */
  EncodeInfo stream_info[MW_MAX_STREAM_NUM];

  void clean () {
    memset (stream_state, 0, sizeof(bool) * MW_MAX_STREAM_NUM);
  }

  bool get_stream_info (int iav_fd);
};

struct VideoConfigData {
  NetCmdType cmd;
  union {
    EncodeSetting _encode;
  }configData;
#define encodeSetting (configData._encode)

  VideoConfigData (NetCmdType command) {
    cmd = command;
    memset (&configData, 0, sizeof(configData));
  }
};
#endif //APP_IPCAM_CONTROLSERVER_VIDEODATASTRUCTURE_H

