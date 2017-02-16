/**
 * am_video_camera_if.h
 *
 *  History:
 *    Aug 6, 2015 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef ORYX_INCLUDE_VIDEO_NEW_AM_VIDEO_CAMERA_IF_H_
#define ORYX_INCLUDE_VIDEO_NEW_AM_VIDEO_CAMERA_IF_H_
#include "am_pointer.h"
#include "am_encode_config_param.h"

class AMIVideoCamera;

typedef AMPointer<AMIVideoCamera> AMIVideoCameraPtr;
class AMIVideoCamera
{
    friend AMIVideoCameraPtr;

  public:
    static AMIVideoCameraPtr get_instance();

  public:
    virtual AM_RESULT start()                                               = 0;
    virtual AM_RESULT stop()                                                = 0;

    virtual AM_RESULT idle()                                                = 0;
    virtual AM_RESULT preview()                                             = 0;
    virtual AM_RESULT encode()                                              = 0;
    virtual AM_RESULT decode()                                              = 0;

    virtual AM_RESULT load_config_all()                                     = 0;

  public:
    //For configure files operation
    virtual AM_RESULT get_vin_config(AMVinParamMap &config)                 = 0;
    virtual AM_RESULT set_vin_config(const AMVinParamMap &config)           = 0;
    virtual AM_RESULT get_vout_config()                                     = 0;
    virtual AM_RESULT set_vout_config()                                     = 0;
    virtual AM_RESULT get_buffer_config(AMBufferParamMap &config)           = 0;
    virtual AM_RESULT set_buffer_config(const AMBufferParamMap &config)     = 0;
    virtual AM_RESULT get_stream_config(AMStreamParamMap &config)           = 0;
    virtual AM_RESULT set_stream_config(const AMStreamParamMap &config)     = 0;

  protected:
    virtual void inc_ref() = 0;
    virtual void release() = 0;
    virtual ~AMIVideoCamera(){};
};

#endif /* ORYX_INCLUDE_VIDEO_NEW_AM_VIDEO_CAMERA_IF_H_ */
