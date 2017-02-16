/*******************************************************************************
 * am_vin.h
 *
 * Histroy:
 *  2012-3-6 2012 - [ypchang] created file
 *  2014-6-24 2014 - [Louis] modify this for Oryx MW
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMVIN_H_
#define AMVIN_H_

/*******************************************************************************
 * AmVin is the base class of all VIN devices, DO NOT use this class
 * directly, use the inherited classes instead
 ******************************************************************************/
#include "am_video_types.h"
#include "am_video_param.h"

#include <semaphore.h>
#include <atomic>   //for m_vin_sync_waiters_num
#include <thread>
class AMVin
{
  public:

    AMVin();
    virtual ~AMVin();
    virtual AM_RESULT init(int iav_hd, int vin_id, AMVinParam *vin_param);
    virtual AM_RESULT destroy();
    virtual AM_RESULT reset();

    virtual AM_RESULT set_mode();  //call this to set the required
    //mode with auto FPS
    virtual AM_RESULT get_mode(AM_VIN_MODE *mode);

    virtual AM_RESULT set_fps();   //set VIN FPS by this
    virtual AM_RESULT get_fps(AM_VIDEO_FPS *fps); //VIN FPS may change on the fly

    //VIN state, means not working, or working.
    virtual AM_RESULT get_state(AM_VIN_STATE *state);

    virtual AM_RESULT flip();

    virtual AM_RESULT detect_vin_capability(); //detect VIN capability from
    //loaded driver
    virtual AM_RESULT show_vin_info();

    virtual AM_RESULT change(AMVinParam *vin_param);
    virtual AM_RESULT update();  //refresh from config

    virtual AM_RESULT get_resolution(AMResolution *size);
    virtual AM_RESULT get_AGC(AMVinAGC *agc);

    virtual AM_RESULT wait_frame_sync();


    virtual AM_RESULT set_hdr_type(AM_HDR_EXPOSURE_TYPE type);
    virtual AM_RESULT get_hdr_type(AM_HDR_EXPOSURE_TYPE *type);

    AM_VIN_ID      m_id;

    //VIN config attributes
    AMVinParam     m_param;


    int            m_iav;
    //state machine
    AM_VIN_STATE m_state;

  protected:
    AM_RESULT print_vin_info(const void *video_info, int fps_q9);

    //FPS Q9 format is sometimes useful
    AMVideoFpsFormatQ9 m_video_fps_q9_format;
    AMResolution m_resolution;

    AM_HDR_EXPOSURE_TYPE m_hdr_type;
};

/* For RGB bayer pattern output CMOS sensor VIN
 * Such VIN can control resolution, fps, shutter, gain and etc.
 * as common property
 * need Ambarella ISP to do image processing,
 * Example,  SONY IMX136 RGB output
 */
class AMRGBSensorVin: public AMVin
{
  public:
    AMRGBSensorVin();
    virtual ~AMRGBSensorVin();
    virtual AM_RESULT init(int iav_hd, int vin_id, AMVinParam *vin_param);
    virtual AM_RESULT destroy();
    virtual AM_RESULT set_mode();
    virtual AM_RESULT set_fps();
    virtual AM_RESULT get_AGC(AMVinAGC *agc);

    AM_RESULT wait_frame_sync();
  private:
    AM_RESULT set_sensor_initial_agc_shutter();
    AM_RESULT post_frame_sync();
    static void vin_sync_thread_func(AMRGBSensorVin *pThis);
    sem_t   m_vin_sync_sem;
    std::atomic<int>  m_vin_sync_waiters_num;
    std::thread *m_vin_sync_thread;
    bool m_vin_sync_exit_flag;
};

/* For YUV format output CMOS sensor VIN, with built-in AE/AWB control
 * Such VIN can control resolution, fps, as common property,
 * No need ISP to do image processing, except MCTF,
 * example:  OV7740 YUV output mode
 */
class AMYUVSensorVin: public AMVin
{
  public:
    AMYUVSensorVin()
  {
      m_param.type = AM_VIN_TYPE_YUV_SENSOR;
  }
    virtual ~AMYUVSensorVin() {}
    virtual AM_RESULT set_fps(AM_VIDEO_FPS fps);
};

/* For RGB bayer pattern output CMOS sensor VIN
 * Such VIN can control resolution, fps, as common property,
 * No need ISP to do image processing, except MCTF,
 * example:     ADV7441A
 */
class AMYUVGenericVin: public AMVin
{
  public:
    AMYUVGenericVin(): AMVin(),
    m_vin_source(0)
  {
      m_param.type = AM_VIN_TYPE_YUV_GENERIC;
  }
    virtual ~AMYUVGenericVin() {}
    virtual AM_RESULT set_fps(AM_VIDEO_FPS fps) {return AM_RESULT_ERR_PERM;}

    uint32_t m_vin_source; //only YUV generic has concept of "source"
};

#endif /* AMVIN_H_ */
