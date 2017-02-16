/**********************************************************************
 * EncodeServerConfig.h
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
#ifndef APP_IPCAM_CONTROLSERVER_ENCODESERVERCONFIG_H
#define APP_IPCAM_CONTROLSERVER_ENCODESERVERCONFIG_H

#include "mw_struct.h"
#include "mw_api.h"

struct EncodeSetting;

class EncodeServerConfig
{
  public:
    EncodeServerConfig():mIavFd(-1) {}
    ~EncodeServerConfig();
    int open_iav ();
    bool get_encode_setting (EncodeSetting * encode_setting);
  private:
    int mIavFd;
};

#endif //APP_IPCAM_CONTROLSERVER_ENCODESERVERCONFIG_H

