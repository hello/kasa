/**
 * am_video_camera_if.h
 *
 *  History:
 *    Aug 6, 2015 - [Shupeng Ren] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef ORYX_INCLUDE_VIDEO_AM_VIDEO_CAMERA_IF_H_
#define ORYX_INCLUDE_VIDEO_AM_VIDEO_CAMERA_IF_H_
#include "am_pointer.h"
#include "am_encode_config_param.h"
#include <string>
#include <map>

class AMIVideoCamera;

typedef AMPointer<AMIVideoCamera> AMIVideoCameraPtr;
class AMIVideoCamera
{
    friend AMIVideoCameraPtr;

  public:
    static AMIVideoCameraPtr get_instance();

  public:
    virtual AM_RESULT start()                                               = 0;
    virtual AM_RESULT start_stream(AM_STREAM_ID id)                         = 0;
    virtual AM_RESULT stop()                                                = 0;
    virtual AM_RESULT stop_stream(AM_STREAM_ID id)                          = 0;

    virtual AM_RESULT idle()                                                = 0;
    virtual AM_RESULT preview()                                             = 0;
    virtual AM_RESULT encode()                                              = 0;
    virtual AM_RESULT decode()                                              = 0;

    virtual AM_RESULT get_vin_status(AMVinInfo &vin)                        = 0;
    virtual AM_RESULT get_stream_status(AM_STREAM_ID id,
                                        AM_STREAM_STATE &state)             = 0;

    virtual AM_RESULT get_buffer_state(AM_SOURCE_BUFFER_ID id,
                                       AM_SRCBUF_STATE &state)              = 0;

    virtual AM_RESULT get_buffer_format(AMBufferConfigParam &param)         = 0;
    virtual AM_RESULT set_buffer_format(const AMBufferConfigParam &param)   = 0;
    virtual AM_RESULT save_buffer_config()                                  = 0;

    virtual uint32_t  get_encode_stream_max_num()                           = 0;
    virtual uint32_t  get_source_buffer_max_num()                           = 0;
    virtual AM_RESULT get_bitrate(AMBitrate &bitrate)                       = 0;
    virtual AM_RESULT set_bitrate(const AMBitrate &bitrate)                 = 0;
    virtual AM_RESULT get_framerate(AMFramerate &rate)                      = 0;
    virtual AM_RESULT set_framerate(const AMFramerate &rate)                = 0;
    virtual AM_RESULT get_mjpeg_info(AMMJpegInfo &mjpeg)                    = 0;
    virtual AM_RESULT set_mjpeg_info(const AMMJpegInfo &mjpeg)              = 0;
    virtual AM_RESULT get_h26x_gop(AMGOP &h264)                             = 0;
    virtual AM_RESULT set_h26x_gop(const AMGOP &h264)                       = 0;
    virtual AM_RESULT get_stream_type(AM_STREAM_ID id, AM_STREAM_TYPE &type)= 0;
    virtual AM_RESULT set_stream_type(AM_STREAM_ID id, AM_STREAM_TYPE &type)= 0;
    virtual AM_RESULT get_stream_size(AM_STREAM_ID id, AMResolution &res)   = 0;
    virtual AM_RESULT set_stream_size(AM_STREAM_ID id, AMResolution &res)   = 0;
    virtual AM_RESULT get_stream_offset(AM_STREAM_ID id, AMOffset &offset)  = 0;
    virtual AM_RESULT set_stream_offset(AM_STREAM_ID id,
                                        const AMOffset &offset)             = 0;
    virtual AM_RESULT get_stream_source(AM_STREAM_ID id,
                                        AM_SOURCE_BUFFER_ID &source)        = 0;
    virtual AM_RESULT set_stream_source(AM_STREAM_ID id,
                                        AM_SOURCE_BUFFER_ID &source)        = 0;
    virtual AM_RESULT get_stream_flip(AM_STREAM_ID id, AM_VIDEO_FLIP &flip) = 0;
    virtual AM_RESULT set_stream_flip(AM_STREAM_ID id, AM_VIDEO_FLIP &flip) = 0;
    virtual AM_RESULT get_stream_rotate(AM_STREAM_ID id,
                                        AM_VIDEO_ROTATE &rot)               = 0;
    virtual AM_RESULT set_stream_rotate(AM_STREAM_ID id,
                                        AM_VIDEO_ROTATE &rot)               = 0;
    virtual AM_RESULT get_stream_profile(AM_STREAM_ID id,
                                         AM_PROFILE &profile)               = 0;
    virtual AM_RESULT set_stream_profile(AM_STREAM_ID id,
                                         AM_PROFILE &profile)               = 0;
    virtual AM_RESULT save_stream_config()                                  = 0;
    virtual AM_RESULT get_power_mode(AM_POWER_MODE &mode)                   = 0;
    virtual AM_RESULT set_power_mode(AM_POWER_MODE mode)                    = 0;

    virtual AM_RESULT get_avail_cpu_clks(std::map<int32_t, int32_t> &clks)  = 0;
    virtual AM_RESULT get_cur_cpu_clk(int32_t &clk)                         = 0;
    virtual AM_RESULT set_cpu_clk(int32_t index)                            = 0;

    virtual AM_RESULT stop_vin()                                            = 0;

    virtual AM_RESULT halt_vout(AM_VOUT_ID id)                              = 0;

    virtual AM_RESULT force_idr(AM_STREAM_ID stream_id)                     = 0;

    virtual AM_RESULT get_ldc_state(bool &state)                            = 0;
    //For Video Plugin
    virtual void* get_video_plugin(const std::string& plugin_name)          = 0;

    virtual AM_RESULT load_config_all()                                     = 0;

  public:
    //For configure files operation
    virtual AM_RESULT get_feature_config(AMFeatureParam &config)            = 0;
    virtual AM_RESULT set_feature_config(const AMFeatureParam &config)      = 0;
    virtual AM_RESULT get_vin_config(AMVinParamMap &config)                 = 0;
    virtual AM_RESULT set_vin_config(const AMVinParamMap &config)           = 0;
    virtual AM_RESULT get_vout_config(AMVoutParamMap &config)               = 0;
    virtual AM_RESULT set_vout_config(const AMVoutParamMap &config)         = 0;
    virtual AM_RESULT get_buffer_config(AMBufferParamMap &config)           = 0;
    virtual AM_RESULT set_buffer_config(const AMBufferParamMap &config)     = 0;
    virtual AM_RESULT get_stream_config(AMStreamParamMap &config)           = 0;
    virtual AM_RESULT set_stream_config(const AMStreamParamMap &config)     = 0;

  protected:
    virtual void inc_ref() = 0;
    virtual void release() = 0;
    virtual ~AMIVideoCamera(){};
};

#endif /* ORYX_INCLUDE_VIDEO_AM_VIDEO_CAMERA_IF_H_ */
