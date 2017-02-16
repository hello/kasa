/*******************************************************************************
 * am_video_camera.h
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
#ifndef AM_VIDEO_CAMERA_H_
#define AM_VIDEO_CAMERA_H_

#include "am_video_param.h"
#include "am_video_camera_if.h"
#include <atomic>

/*
 *
 * If mode specified
 *    create encode device by mode.
 *    run that encode device, DONE.
 * else
 *    load config
 *    analyze config
 *    decide which mode to use.
 *    create encode device by mode.
 *    run that encode device. done.
 * end if
 *
 *
 */

enum AM_CAMERA_STATE{
  AM_CAMERA_STATE_NOT_INIT = 0,
  AM_CAMERA_STATE_INIT_DONE = 1,
  AM_CAMERA_STATE_RUNNING = 2,
  AM_CAMERA_STATE_STOPPED = 3,
  AM_CAMERA_STATE_ERROR = 4,
};

class AMIEncodeDevice;
class AMEncodeSystemCapability;

struct AMCameraParam{
    AM_ENCODE_MODE          mode;
    AM_HDR_EXPOSURE_TYPE    hdr;
    AM_IMAGE_ISO_TYPE       iso;
    AM_DEWARP_TYPE          dewarp;
    bool                    high_mp_encode;
};


class AMCameraConfig
{
  public:
    AMCameraConfig();
    virtual ~AMCameraConfig();
    //load config from file to m_loaded
    virtual AM_RESULT load_config(AMEncodeDSPCapability *dsp_cap);
    //save config m_using to file
    virtual AM_RESULT save_config();
    //compare "saved" copy to find difference
    //copy m_loaded to m_using
    virtual AM_RESULT sync();

    bool m_changed;
    AMCameraParam m_loaded;
  protected:
    AM_RESULT camera_param_to_dsp_cap(AMCameraParam * cam_param, AMEncodeDSPCapability *dsp_cap);
    AMCameraParam m_using;

};


class AMVideoCamera: public AMIVideoCamera
{
    friend class AMIVideoCamera;

  public:
    virtual AM_RESULT init(AMEncodeDSPCapability *dsp_cap);
    virtual AM_RESULT init(AM_ENCODE_MODE mode);
    virtual AM_RESULT init(); //init with config file to specify mode
    virtual AM_RESULT destroy();

    //high level camera APIs here
    //these high level APIs will create feature set, and then
    //let system resource class to test and then,
    //get input from m_system_resource, calculate detail config, then
    //update m_config->m_loaded

    virtual AM_RESULT set_hdr_enable(bool enable);
    virtual AM_RESULT set_advanced_iso_enable(bool enable);
    virtual AM_RESULT set_ldc_enable(bool enable);
    virtual AM_RESULT set_dewarp_enable(bool enable);

    //high level camera interface
    //it will try to load default config file, and find a mode, init that mode,
    //and call that device's update method.
    virtual AM_RESULT start();

    //try to call that device's stop method.
    virtual AM_RESULT stop();

    virtual AM_RESULT load_config(); //explicitly load from config file

    virtual AM_RESULT update();      //update with current config,

    virtual AM_RESULT save_config(); //explicitly save to config file

    virtual AMIEncodeDevice *get_encode_device();
    /*
     *
     *...
     *... More High level APIs added here.
     *... Which is like APIs that high level user would call
     *...
     *... For low level users, the method is to write to the config file,
     *... then let encode_device to update.     *...
     *...
     *...
     *...
     */

    bool is_mode_change_required();

  protected:
    static AMVideoCamera *get_instance();
    virtual void release();
    virtual void inc_ref();
    virtual AM_RESULT find_encode_mode_by_feature(AM_ENCODE_MODE *mode);
    virtual AM_RESULT create_encode_device();

  protected:
    AMVideoCamera();
    virtual ~AMVideoCamera();
    AMVideoCamera(AMVideoCamera const &copy) = delete;
    AMVideoCamera& operator=(AMVideoCamera const &copy) = delete;

  protected:
    static AMVideoCamera     *m_instance;
    //high level camera config class
    AMCameraConfig           *m_camera_config;
    //this class is used to help AMEncodeDeviceConfig to decide which mode
    //to work and create derived class of AMEncodeDevice for that mode
    AMEncodeSystemCapability *m_system_capability;
    //this will be created by type.
    AMEncodeDevice           *m_encode_device;
    std::atomic_int           m_ref_count;
    AM_ENCODE_MODE            m_encode_mode;
    AM_CAMERA_STATE           m_camera_state;
    AMEncodeDSPCapability     m_dsp_cap; //features of this encode mode
};

#endif /* AM_VIDEO_CAMERA_H_ */
