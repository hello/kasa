/*******************************************************************************
 * EncodeServerConfig.cpp
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
#include "EncodeServerConfig.h"
#include "VideoDataStructure.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

EncodeServerConfig::~EncodeServerConfig() {
  if (mIavFd > 0) {
    close (mIavFd);
    mIavFd = -1;
  }
}

inline int EncodeServerConfig::open_iav () {
  if ((mIavFd = open("/dev/iav", O_RDWR, 0)) < 0) {
    perror ("/dev/iav");
  }
  return mIavFd;
}

bool EncodeServerConfig::get_encode_setting (EncodeSetting * encode_setting)
{
  if (mIavFd < 0) {
    if (open_iav () < 0) {
      return false;
    }
  }
  if (0 == encode_setting) {
    return false;
  }

  return encode_setting->get_stream_info (mIavFd);
}
