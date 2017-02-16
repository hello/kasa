/*******************************************************************************
 * am_video_camera_if.h
 *
 * History:
 *   2014-12-12 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_INCLUDE_VIDEO_AM_VIDEO_CAMERA_IF_H_
#define ORYX_INCLUDE_VIDEO_AM_VIDEO_CAMERA_IF_H_

#include "am_video_types.h"
#include "am_pointer.h"

class AMIVideoCamera;
class AMIEncodeDevice;

typedef AMPointer<AMIVideoCamera> AMIVideoCameraPtr;

class AMIVideoCamera
{
    friend class AMPointer<AMIVideoCamera>;

  public:
    static AMIVideoCameraPtr get_instance();
    virtual AM_RESULT init(AMEncodeDSPCapability *dsp_cap) = 0;
    virtual AM_RESULT init(AM_ENCODE_MODE mode) = 0;
    virtual AM_RESULT init() = 0; //init with config file to specify mode
    virtual AM_RESULT destroy() = 0;

    //high level camera APIs here
    //these high level APIs will create feature set, and then
    //let system resource class to test and then,
    //get input from m_system_resource, calculate detail config, then
    //update m_config->m_loaded

    virtual AM_RESULT set_hdr_enable(bool enable) = 0;
    virtual AM_RESULT set_advanced_iso_enable(bool enable) = 0;
    virtual AM_RESULT set_ldc_enable(bool enable) = 0;
    virtual AM_RESULT set_dewarp_enable(bool enable) = 0;

    //high level camera interface
    //it will try to load default config file, and find a mode, init that mode,
    //and call that device's update method.
    virtual AM_RESULT start() = 0;

    //try to call that device's stop method.
    virtual AM_RESULT stop() = 0;

    virtual AM_RESULT load_config() = 0; //explicitly load from config file

    virtual AM_RESULT update() = 0;      //update with current config,

    virtual AM_RESULT save_config() = 0; //explicitly save to config file
    virtual AMIEncodeDevice* get_encode_device() = 0;

  protected:
    virtual void release() = 0;
    virtual void inc_ref() = 0;
    virtual ~AMIVideoCamera(){}
};

#endif /* ORYX_INCLUDE_VIDEO_AM_VIDEO_CAMERA_IF_H_ */
