/*******************************************************************************
 * am_video_adv_hdr_mode.h
 *
 * History:
 *   2014-8-6 - [lysun] created file
 *   2015-4-20 - [binwang] modified file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef AM_VIDEO_ADV_HDR_MODE_H_
#define AM_VIDEO_ADV_HDR_MODE_H_

#include "am_encode_device.h"

class AMVideoAdvHDRMode: public AMEncodeDevice
{
public:
  virtual ~AMVideoAdvHDRMode();
  static AMVideoAdvHDRMode *create();
  virtual AM_RESULT construct();
  //each mode may use its own config file, however, if not override the
  //load_config method in AMEncodeDevice, default config file will be used.
  virtual AM_RESULT load_config();
  virtual AM_RESULT save_config();
  virtual AM_RESULT update();
protected:
  AMVideoAdvHDRMode();
  virtual AM_RESULT set_system_resource_limit();
public:
};

#endif /* AM_VIDEO_ADV_HDR_MODE_H_ */
