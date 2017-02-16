/*******************************************************************************
 * am_vin_config_if.h
 *
 * History:
 *   2014-12-15 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_INCLUDE_VIDEO_AM_VIN_CONFIG_IF_H_
#define ORYX_INCLUDE_VIDEO_AM_VIN_CONFIG_IF_H_

#include "am_video_types.h"

class AMIVinConfig
{
  public:
    virtual AM_RESULT load_config() = 0;
    virtual bool need_restart() = 0;
    virtual AM_RESULT set_mode_config(AM_VIN_ID id,
                                      AM_VIN_MODE mode) = 0;
    virtual AM_RESULT set_flip_config(AM_VIN_ID id,
                                      AM_VIDEO_FLIP flip) = 0;
    virtual AM_RESULT set_fps_config(AM_VIN_ID id,
                                     AM_VIDEO_FPS fps) = 0;
    virtual AM_RESULT set_bayer_config(AM_VIN_ID id,
                                       AM_VIN_BAYER_PATTERN bayer) = 0;
    virtual AM_RESULT save_config() = 0;
    virtual ~AMIVinConfig(){}
};

#endif /* ORYX_INCLUDE_VIDEO_AM_VIN_CONFIG_IF_H_ */
