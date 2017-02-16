/*******************************************************************************
 * am_encode_device.h
 *
 * History:
 *  July 10, 2014 - [Louis] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_ENCODE_DEVICE_H_
#define AM_ENCODE_DEVICE_H_

#include <mutex>  //using C++11 mutex

#include "am_encode_device_if.h"
#include "am_video_device.h"
#include "am_video_dsp.h"
#include "am_lbr_control.h"
#include "am_event_types.h"
#include "am_jpeg_snapshot_if.h"

class AMDPTZWarp;
class AMPrivacyMask;
class AMEncodeDevice;
class AMEncodeDeviceConfig;

class AMEncodeStream
{
  public:
    AMEncodeStream();
    virtual ~AMEncodeStream();

    virtual AM_RESULT init(AMEncodeDevice *encode_device, AM_STREAM_ID id,
                           int iav_hd);
    virtual AM_RESULT set_encode_format(AMEncodeStreamFormat *encode_format);
    virtual AM_RESULT get_encode_format(AMEncodeStreamFormat *encode_format);



    virtual AM_RESULT set_encode_config(AMEncodeStreamConfig *stream_config);
    virtual AM_RESULT get_encode_config(AMEncodeStreamConfig *stream_config);

    //change bitrate, frame factor and other params on the fly
    virtual AM_RESULT set_encode_dynamic_config(
        AMEncodeStreamDynamicConfig* dynamic_config);

    //start encoding is format is H264 or MJPEG, or ignore and return OK
    //if AM_ENCODE_STREAM_TYPE is NONE
    virtual AM_RESULT start_encode();

    //stop encoding is format is H264 or MJPEG, or ignore and return OK
    //if AM_ENCODE_STREAM_TYPE is NONE
    virtual AM_RESULT stop_encode();
    virtual bool is_encoding();

  protected:
    virtual AM_RESULT update_stream_state();

    //get the source buffer which is associated with this stream
    virtual AMEncodeSourceBuffer* get_source_buffer();
    virtual AM_RESULT set_h264_encode_config(AMEncodeH264Config *h264_conf);
    virtual AM_RESULT set_mjpeg_encode_config(AMEncodeMJPEGConfig *mjpeg_conf);
    virtual AM_RESULT get_h264_encode_config(AMEncodeH264Config *h264_conf);
    virtual AM_RESULT get_mjpeg_encode_config(AMEncodeMJPEGConfig *mjpeg_conf);

  public:
    AMEncodeStreamFormat m_stream_format;
    AMEncodeStreamConfig m_stream_config;
    AM_ENCODE_STREAM_STATE m_state;
    AMOSDOverlay *m_osd_overlay;

  protected:
    AMEncodeDevice *m_encode_device;
    AM_STREAM_ID m_id;
    int m_iav;
};

class AMEncodeDevice: public AMVideoDevice, public AMIEncodeDevice
{
  public:
    static AMEncodeDevice *create();
    virtual AM_RESULT construct();
    virtual ~AMEncodeDevice();

  public:
    /*****  standard operation APIs           *****/
    virtual AM_RESULT init();  //init itself and base class
    virtual AM_RESULT destroy();

    virtual AM_RESULT load_config();
    virtual AM_RESULT save_config();


    /*try to start with configuration.   */
    virtual AM_RESULT start();
    /* stop all encoding behavior and stay in preview     */
    virtual AM_RESULT stop();
    /* if there is encoding, stops it first, and quit to idle */
    virtual AM_RESULT quit();

    /* update is on the fly update in preview/encoding,
     * but should not go to other states.
     * if there is chance that requires dsp in update() to go to other state
     * to do, update() will report false with error message,
     * which means "on-the-fly" update is not possible,
     * then user should call stop/start combination to do it.
     */
    virtual AM_RESULT update();
    virtual AM_RESULT get_encode_mode(AM_ENCODE_MODE *mode);

  public:
    //Direct Access APIs
    virtual AM_RESULT start_encode_stream(AM_STREAM_ID stream_id);
    virtual AM_RESULT stop_encode_stream(AM_STREAM_ID stream_id);
    virtual AM_RESULT show_stream_info(AM_STREAM_ID stream_id);
    virtual AM_RESULT set_stream_bitrate(AM_STREAM_ID stream_id,
                                         uint32_t bitrate);
    virtual AM_RESULT set_stream_fps(AM_STREAM_ID stream_id, uint32_t fps);
    virtual AM_RESULT get_stream_fps(AM_STREAM_ID stream_id, uint32_t *fps);
    virtual AM_RESULT show_source_buffer_info(AM_ENCODE_SOURCE_BUFFER_ID bufid);
    virtual AM_RESULT set_force_idr(AM_STREAM_ID stream_id);
    virtual AM_RESULT set_lbr_config(AMEncodeLBRConfig *lbr_config);
    virtual AM_RESULT set_dptz_warp_config(AMDPTZWarpConfig *dptz_warp_config);
    virtual AMIOSDOverlay* get_osd_overlay();
    virtual AMIDPTZWarp* get_dptz_warp();
    virtual AMIVinConfig* get_vin_config();
    virtual AMIEncodeDeviceConfig* get_encode_config();

    AMEncodeSourceBuffer* get_source_buffer_list()
    {
      return m_source_buffer;
    }

    AMEncodeStream* get_encode_stream_list()
    {
      return m_encode_stream;
    }

    virtual AM_RESULT get_encode_stream_format_all(
        AMEncodeStreamFormatAll *stream_format_all);
    virtual AM_RESULT get_encode_stream_config_all(
        AMEncodeStreamConfigAll *stream_config_all);
    virtual AM_RESULT get_lbr_config(AMEncodeLBRConfig *lbr_config);
    virtual AM_RESULT get_dptz_warp_config(AMDPTZWarpConfig *dptz_warp_config);
    virtual bool is_motion_enable();
    virtual void update_motion_state(void *msg);

    AMLBRControl* get_lbr_ctrl_list()
    {
      return m_lbr_ctrl;
    }


    AMEncodeSourceBuffer * get_source_buffer(AM_ENCODE_SOURCE_BUFFER_ID buf_id);

  protected:
    AMEncodeDevice();
    /*****  Stage 1 ,  prepare for Preview *****/
    virtual AM_RESULT set_encode_stream_format_all(
        AMEncodeStreamFormatAll *stream_format_all);
    virtual AM_RESULT set_encode_stream_config_all(
        AMEncodeStreamConfigAll *stream_config_all);

    /***** Stage 2 , Start/stop encoding and on the fly change during Encoding*/
    virtual AM_RESULT set_encode_stream_dynamic_config(
        AM_STREAM_ID stream_id,
        AMEncodeStreamDynamicConfig *dynamic_config);
    virtual bool is_goto_idle_needed();
    virtual bool is_stream_format_changed();
    virtual bool is_stream_config_changed();
    virtual bool is_stream_encode_control_changed();
    virtual bool is_stream_encode_on_the_fly_changed();
    virtual bool is_lbr_config_changed();
    virtual bool is_stream_enabled(int i);

    virtual AM_RESULT start_encode(); //start/stop some streams by config
    virtual AM_RESULT stop_encode(); //start/stop some streams
    virtual AM_RESULT start_stop_encode(); //start/stop sreams by config

    //state change APIs need more than bool type return value
    virtual AM_RESULT preview_to_encode(); //start from idle
    virtual AM_RESULT idle_to_preview();
    virtual AM_RESULT idle_to_encode();
    virtual AM_RESULT set_system_resource_limit();

  private:
    AM_RESULT auto_calc_encode_param();
    AM_RESULT assign_stream_to_buffer(AM_STREAM_ID stream,
                                      AM_ENCODE_SOURCE_BUFFER_ID buffer);
    bool                    m_init_flag;
    bool                    m_goto_idle_needed;
  protected:
    /* Encode Source buffer, m_source_buffer is the 1st member */
    AMEncodeSourceBuffer   *m_source_buffer;
    /*
     * max number may vary in different modes,
     * but always no bigger than AM_MAX_SOURCE_BUFFER_NUM
     */
    const int32_t           m_max_source_buffer_num;
    /* Encode Stream, m_encode_stream is the 1st member  */
    AMEncodeStream         *m_encode_stream;
    /*
     * max number may vary in different modes,
     * but always no bigger than AM_MAX_ENCODE_STREAM_NUM
     */
    const int32_t           m_max_encode_stream_num;

    AMDPTZWarp             *m_dptzwarp; /*DPTZ and Lens Warp are done together*/
    AMPrivacyMask          *m_pm;
    AMLBRControl           *m_lbr_ctrl;
    AMIJpegSnapshotPtr      m_jpeg_snapshot_handler;
    AMIOSDOverlay          *m_osd_overlay;

    //back up the config info that used to init AMEncodeDevice
    AMEncodeDeviceConfig   *m_config;
    AM_ENCODE_MODE          m_encode_mode;

    //C++ 11 mutex (using recursive type)
    std::recursive_mutex    m_mtx;

    //this will also be used in derived class to check auto buffer config
    AMEncodeParamAll        m_auto_encode_param;
    AMResourceLimitParam    m_rsrc_lmt;
    //New this class in son class AMVideoxxxMode to read config file of this mode
    AMResourceLimitConfig   *m_rsrc_lmt_cfg;
};

#endif /* AM_ENCODE_DEVICE_H_ */
