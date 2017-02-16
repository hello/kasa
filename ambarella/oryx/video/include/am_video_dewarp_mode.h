/*******************************************************************************
 * am_video_dewarp_mode.h
 *
 * History:
 *   2014-8-6 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef AM_VIDEO_DEWARP_MODE_H_
#define AM_VIDEO_DEWARP_MODE_H_

#include "am_encode_device.h"

class AMVideoDewarpMode: public AMEncodeDevice
{
public:
  virtual ~AMVideoDewarpMode();
  static AMVideoDewarpMode *create();
protected:
  AMVideoDewarpMode();

};

#endif /* AM_VIDEO_DEWARP_MODE_H_ */
