/*******************************************************************************
 * am_platform.h
 *
 * History:
 *   May 7, 2015 - [ypchang] created file
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
 *
 ******************************************************************************/
#ifndef ORYX_VIDEO_INCLUDE_AM_PLATFORM_H_
#define ORYX_VIDEO_INCLUDE_AM_PLATFORM_H_

#include <map>
#include <mutex>
#include <vector>
#include <atomic>

#include "am_pointer.h"
#include "am_video_types.h"
#include "am_encode_types.h"
#include "am_encode_config_param.h"

using std::vector;
using std::map;
using std::string;
using std::atomic_int;
using std::recursive_mutex;

class AMIPlatform;

struct AMPlatformVinMode
{
    AM_VIN_ID   id = AM_VIN_ID_INVALID;
    AM_VIN_MODE mode  = AM_VIN_MODE_AUTO;
    AM_HDR_TYPE hdr_type = AM_HDR_SINGLE_EXPOSURE;
};

struct AMPlatformVinFps
{
    AM_VIN_ID id = AM_VIN_ID_INVALID;
    AM_VIDEO_FPS fps = AM_VIDEO_FPS_AUTO;
};

struct AMPlatformVinFlip
{
    AM_VIN_ID id = AM_VIN_ID_INVALID;
    AM_VIDEO_FLIP flip = AM_VIDEO_FLIP_AUTO;
};

struct AMPlatformVinInfo
{
    AM_VIN_ID         id = AM_VIN_ID_INVALID;
    AM_VIN_MODE       mode = AM_VIN_MODE_AUTO;
    AMResolution      size;
    AM_VIN_BITS       bits = AM_VIN_BITS_AUTO;
    AM_VIDEO_FPS      fps = AM_VIDEO_FPS_AUTO;
    AM_SENSOR_TYPE    type = AM_SENSOR_TYPE_NONE;
    AM_HDR_TYPE       hdr_type = AM_HDR_SINGLE_EXPOSURE;
};
typedef std::vector<AMPlatformVinInfo> AMPlatformVinInfoList;

struct AMPlatformSensorInfo
{
    AM_VIN_ID     id = AM_VIN_ID_INVALID;
    uint32_t      sensor_id = 0;
    AM_VIN_TYPE   dev_type = AM_VIN_TYPE_INVALID;
    string        name;
};

struct AMPlatformVinShutter
{
    AM_VIN_ID       id = AM_VIN_ID_INVALID;
    AMScaleFactor   shutter_time;
};

struct AMPlatformVinAGC
{
    AM_VIN_ID id = AM_VIN_ID_INVALID;
    AMVinAGC  agc_info;
};

struct AMPlatformVoutSinkConfig
{
    int32_t             sink_id = -1;
    AM_VOUT_TYPE        sink_type = AM_VOUT_TYPE_NONE;
    AM_VOUT_VIDEO_TYPE  video_type = AM_VOUT_VIDEO_TYPE_NONE;
    AM_VOUT_MODE        mode = AM_VOUT_MODE_AUTO;
    AM_VIDEO_FLIP       flip = AM_VIDEO_FLIP_NONE;
    AM_VIDEO_ROTATE     rotate = AM_VIDEO_ROTATE_NONE;
    AM_VIDEO_FPS        fps = AM_VIDEO_FPS_AUTO;
};

struct AMPlatformBufferFormat
{
    AM_SOURCE_BUFFER_ID id = AM_SOURCE_BUFFER_INVALID;
    AM_SOURCE_BUFFER_TYPE type = AM_SOURCE_BUFFER_TYPE_OFF;
    AMRect  input = {0};
    AMResolution size;
};
typedef map<AM_SOURCE_BUFFER_ID, AMPlatformBufferFormat> \
    AMPlatformBufferFormatMap;

struct AMPlatformEFMRequestFrame
{
    uint32_t frame_index = 0;
    uint32_t yuv_luma_offset = 0;
    uint32_t yuv_chroma_offset = 0;
    uint32_t me0_offset = 0;
    uint32_t me1_offset = 0;
};

struct AMPlatformEFMHandshakeFrame
{
    uint32_t stream_id = 0;
    uint32_t frame_index = 0;
    uint32_t frame_pts = 0;
};

struct AMPlatformStreamFormat
{
    AM_STREAM_ID            id = AM_STREAM_ID_INVALID;
    AM_STREAM_TYPE          type = AM_STREAM_TYPE_NONE;
    AM_SOURCE_BUFFER_ID     source = AM_SOURCE_BUFFER_INVALID;
    AM_VIDEO_FLIP           flip = AM_VIDEO_FLIP_NONE;
    int32_t                 duration = 0;
    bool                    rotate = false;
    AMRect                  enc_win;
    AMFps                   fps;
};

struct AMPlatformH26xConfig
{
    AM_STREAM_ID            id = AM_STREAM_ID_INVALID;
    AM_PROFILE              profile = AM_PROFILE_BASELINE;
    AM_AU_TYPE              au_type = AM_AU_TYPE_NO_AUD_NO_SEI;
    AM_GOP_MODEL            gop_model = AM_GOP_MODEL_SIMPLE;
    AM_CHROMA_FORMAT        chroma_format = AM_CHROMA_FORMAT_YUV420;
    AM_RATE_CONTROL         bitrate_control = AM_RC_CBR;
    uint32_t                M = 0;
    uint32_t                N = 0;
    uint32_t                rate = 0;  // Read only
    uint32_t                scale = 0; // Read only
    uint32_t                target_bitrate = 0;
    uint32_t                idr_interval = 0;
    uint32_t                mv_threshold = 0;
    uint32_t                fast_seek_intvl = 0;
    uint32_t                sar_width = 0;
    uint32_t                sar_height = 0;
    uint32_t                i_frame_max_size = 0;
    bool                    flat_area_improve = false;
    bool                    multi_ref_p = false;
};

struct AMPlatformMJPEGConfig
{
    AM_STREAM_ID id = AM_STREAM_ID_INVALID;
    AM_CHROMA_FORMAT chroma_format = AM_CHROMA_FORMAT_YUV420;
    uint32_t quality = 0;
};

struct AMPlatformBitrate
{
    AM_STREAM_ID id = AM_STREAM_ID_INVALID;
    AM_RATE_CONTROL rate_control_mode = AM_RC_NONE;
    int32_t target_bitrate = -1;
    int32_t vbr_min_bitrate = -1;
    int32_t vbr_max_bitrate = -1;
    int32_t i_frame_max_size = -1;
};

struct AMPlatformFrameRate
{
    AM_STREAM_ID id = AM_STREAM_ID_INVALID;
    int32_t keep_fps = -1;
    int32_t mul = 0;
    int32_t div = 0;
    int32_t fps = -1;
};


struct AMPlatformResourceLimitExIAV1
{
    bool sharpen_b_possible = false;
};

struct AMPlatformResourceLimitExIAV2
{
    uint32_t debug_iso_type = -1;
    bool lens_warp_possible = false;
    bool vout_swap_possible = false;
};

struct AMPlatformResourceLimit
{
    void *extra_area = nullptr;

    AM_ENCODE_MODE mode = AM_ENCODE_INVALID_MODE;
    AM_HDR_TYPE hdr_type = AM_HDR_SINGLE_EXPOSURE;

    uint32_t max_num_encode = 0;
    uint32_t max_num_cap_sources = 0;
    uint32_t max_warp_output_width = 0;
    uint32_t max_padding_width = 0;
    uint32_t enc_dummy_latency = 0;
    uint32_t idsp_upsample_type = 0;

    bool rotate_possible = false;
    bool raw_capture_possible = false;
    bool enc_from_raw_possible = false;
    bool mixer_a_possible = false;
    bool mixer_b_possible = false;

    AMResolution raw_max_size;
    AMResolution v_warped_main_max_size;
    AMResolution max_warp_input_size;

    vector<AMResolution> buf_max_size;
    vector<AMResolution> stream_max_size;
    vector<uint32_t> stream_max_M;
    vector<uint32_t> stream_max_N;
    vector<uint32_t> stream_max_advanced_quality_model;
    vector<bool> stream_long_ref_possible;

};

struct AMPlatformFrameInfo
{
    AM_DATA_FRAME_TYPE    type = AM_DATA_FRAME_TYPE_VIDEO;
    AM_SOURCE_BUFFER_ID   buf_id = AM_SOURCE_BUFFER_INVALID;
    uint32_t              timeout = -1;
    bool                  latest = false;
    AMQueryFrameDesc      desc;
};

struct AMPlatformDPTZRatio
{
    /* usually, zoom_factor_x should be equal to zoom_factor_y,
     * otherwise, after digital zoom, the aspect ratio will change
     */
    float zoom_factor_x = 0.0; /* like 1.0X,  4.0X,  8.0X */
    float zoom_factor_y = 0.0; /* like 1.0X,  4.0X,  8.0X */

    /* if zoom_center_pos_x and zoom_center_pos_y are both 0,
     * then it's center zoomed */
    float zoom_center_pos_x = 0.0; /* -1.0~ 1.0. -1.0: left most, 1.0: right most */
    float zoom_center_pos_y = 0.0; /* -1.0~ 1.0. -1.0: top most, 1.0: bottom most */
};

struct AMPlatformDPTZParam
{
    AMRect window;
    AMPlatformDPTZRatio ratio;
};
typedef map<AM_SOURCE_BUFFER_ID, AMPlatformDPTZParam>  AMPlatformDPTZParamMap;

struct AMWarpVector
{
    AMRectInMain input;
    AMRectInMain inter_output;
    AMRectInMain output;
    AMVectorMap ver_map;
    AMVectorMap hor_map;
    AMVectorMap me1_ver_map;
    int rotate_flip = 0;
};

struct AMPlatformWarpRegion
{
    AMRectInMain output;
    AMRectInMain inter_output;
    AMFrac zoom;
    int pitch = 0;
    int yaw = 0;
};

typedef float AMDegree;
struct AMDewarpInitParam
{
    int *lut_radius = nullptr;
    AM_WARP_PROJECTION_MODE projection_mode = AM_WARP_PROJRECTION_EQUIDISTANT;
    AMDegree max_fov = 0.0;
    int max_radius = 0;
    int max_input_width = 0;
    int max_input_height = 0;
    AMPoint lens_center_in_max_input;
    int lut_focal_length = 0;
    int main_buffer_width = 0;
};


typedef AMPointer<AMIPlatform> AMIPlatformPtr;
class AMIPlatform
{
    friend AMIPlatformPtr;

  public:
    static AMIPlatformPtr get_instance();

  public:
    /*
     * These APIs are for platform config files
     */
    virtual AM_RESULT load_config()                                         = 0;
    virtual AM_RESULT feature_config_set(const AMFeatureParam &param)       = 0;
    virtual AM_RESULT feature_config_get(AMFeatureParam &param)             = 0;

  public:
    /*
     * Below APIs are for IAV control.
     */
    //For VIN
    virtual AM_RESULT vin_params_get(AMVinInfo &info)                       = 0;
    virtual AM_RESULT vin_number_get(uint32_t &number)                      = 0;
    virtual AM_RESULT vin_mode_set(const AMPlatformVinMode &mode)           = 0;
    virtual AM_RESULT vin_mode_get(AMPlatformVinMode &mode)                 = 0;
    virtual AM_RESULT vin_fps_set(const AMPlatformVinFps &fps)              = 0;
    virtual AM_RESULT vin_fps_get(AMPlatformVinFps &fps)                    = 0;
    virtual AM_RESULT vin_flip_set(const AMPlatformVinFlip &flip)           = 0;
    virtual AM_RESULT vin_flip_get(AMPlatformVinFlip &flip)                 = 0;
    virtual AM_RESULT vin_shutter_set(const AMPlatformVinShutter &shutter)  = 0;
    virtual AM_RESULT vin_shutter_get(AMPlatformVinShutter &shutter)        = 0;
    virtual AM_RESULT vin_agc_set(const AMPlatformVinAGC &agc)              = 0;
    virtual AM_RESULT vin_agc_get(AMPlatformVinAGC &agc)                    = 0;
    virtual AM_RESULT vin_info_get(AMPlatformVinInfo &info)                 = 0;
    virtual AM_RESULT vin_wait_next_frame()                                 = 0;
    virtual AM_RESULT vin_info_list_get(AM_VIN_ID id,
                                        AMPlatformVinInfoList &list)        = 0;
    virtual AM_RESULT sensor_info_get(AMPlatformSensorInfo &info)           = 0;

    //For VOUT
    virtual int32_t   vout_sink_id_get(AM_VOUT_ID id, AM_VOUT_TYPE type)    = 0;
    virtual AM_RESULT vout_active_sink_set(int32_t sink_id)                 = 0;
    virtual AM_RESULT vout_sink_config_set(const AMPlatformVoutSinkConfig
                                           &sink)                           = 0;
    virtual AM_RESULT vout_halt(AM_VOUT_ID id)                              = 0;

    //For Buffer
    virtual AM_RESULT buffer_state_get(AM_SOURCE_BUFFER_ID id,
                                       AM_SRCBUF_STATE &state)              = 0;
    virtual AM_RESULT buffer_setup_set(const AMPlatformBufferFormatMap &format) = 0;
    virtual AM_RESULT buffer_setup_get(AMPlatformBufferFormatMap &format)   = 0;
    virtual AM_RESULT buffer_format_set(const AMPlatformBufferFormat &format) = 0;
    virtual AM_RESULT buffer_format_get(AMPlatformBufferFormat &format)     = 0;

    //For EFM Buffer
    virtual AM_RESULT efm_frame_request(AMPlatformEFMRequestFrame &frame)   = 0;
    virtual AM_RESULT efm_frame_handshake(AMPlatformEFMHandshakeFrame
                                          &frame)                           = 0;

    //For Stream
    virtual AM_RESULT stream_format_set(AMPlatformStreamFormat &format)     = 0;
    virtual AM_RESULT stream_format_get(AMPlatformStreamFormat &format)     = 0;
    virtual AM_RESULT stream_config_set()                                   = 0;
    virtual AM_RESULT stream_config_get()                                   = 0;
    virtual AM_RESULT stream_h264_config_set(const AMPlatformH26xConfig
                                             &h264)                         = 0;
    virtual AM_RESULT stream_h264_config_get(AMPlatformH26xConfig &h264)    = 0;
    virtual AM_RESULT stream_h265_config_set(const AMPlatformH26xConfig
                                             &h265)                         = 0;
    virtual AM_RESULT stream_h265_config_get(AMPlatformH26xConfig &h265)    = 0;
    virtual AM_RESULT stream_mjpeg_config_set(const AMPlatformMJPEGConfig
                                              &mjpeg)                       = 0;
    virtual AM_RESULT stream_mjpeg_config_get(AMPlatformMJPEGConfig &mjpeg) = 0;

    virtual AM_RESULT stream_bitrate_set(const AMPlatformBitrate &bitrate)  = 0;
    virtual AM_RESULT stream_bitrate_get(AMPlatformBitrate &bitrate)        = 0;

    virtual AM_RESULT stream_framerate_set(const AMPlatformFrameRate
                                             &framerate)                    = 0;
    virtual AM_RESULT stream_framerate_get(AMPlatformFrameRate &framerate)= 0;
    virtual AM_RESULT stream_mjpeg_quality_get(AMPlatformMJPEGConfig
                                               &mjpeg)                      = 0;
    virtual AM_RESULT stream_mjpeg_quality_set(const AMPlatformMJPEGConfig
                                               &mjpeg)                      = 0;
    virtual AM_RESULT stream_h26x_gop_get(AMPlatformH26xConfig &h264)       = 0;
    virtual AM_RESULT stream_h26x_gop_set(const AMPlatformH26xConfig &h264) = 0;
    virtual AM_RESULT stream_offset_get(AM_STREAM_ID id, AMOffset &offset)  = 0;
    virtual AM_RESULT stream_offset_set(AM_STREAM_ID id,
                                        const AMOffset &offset)             = 0;

    virtual AM_RESULT stream_state_get(AM_STREAM_ID id,
                                       AM_STREAM_STATE &state)              = 0;

    //For Resource Limit
    virtual AM_RESULT system_resource_set(const AMBufferParamMap &buffer,
                                          const AMStreamParamMap &stream)   = 0;
    virtual AM_RESULT system_resource_get(AMPlatformResourceLimit &res)     = 0;
    virtual uint32_t  system_max_stream_num_get()                           = 0;
    virtual uint32_t  system_max_buffer_num_get()                           = 0;

    //For Encode/Decode
    virtual AM_RESULT goto_idle()                                           = 0;
    virtual AM_RESULT goto_preview()                                        = 0;
    virtual AM_RESULT encode_start(uint32_t stream_bits)                    = 0;
    virtual AM_RESULT encode_stop(uint32_t stream_bits)                     = 0;
    virtual AM_RESULT decode_start()                                        = 0;
    virtual AM_RESULT decode_stop()                                         = 0;

    virtual AM_RESULT iav_state_get(AM_IAV_STATE &state)                    = 0;

    virtual AM_RESULT get_power_mode(AM_POWER_MODE &mode)                   = 0;
    virtual AM_RESULT set_power_mode(AM_POWER_MODE mode)                    = 0;

    //For Querying Frames
    virtual AM_RESULT query_frame(AMPlatformFrameInfo &frame)               = 0;

    //For Encode Dynamic Control
    virtual AM_RESULT force_idr(AM_STREAM_ID id)                            = 0;

    //For Overlay
    virtual AM_RESULT overlay_set(const AMOverlayInsert &overlay)           = 0;
    virtual AM_RESULT overlay_get(AMOverlayInsert &area)                    = 0;
    virtual uint32_t  overlay_max_area_num()                                = 0;
    virtual AM_OVERLAY_TYPE get_overlay()                                   = 0;

    virtual AM_RESULT map_overlay(AMMemMapInfo &mem)                        = 0;
    virtual AM_RESULT unmap_overlay()                                       = 0;
    virtual AM_RESULT map_dsp()                                             = 0;
    virtual AM_RESULT unmap_dsp()                                           = 0;
    virtual AM_RESULT get_dsp_mmap_info(AM_DSP_SUB_BUF_ID id,
                                        AMMemMapInfo &mem)                  = 0;
    virtual AM_RESULT map_bsb(AMMemMapInfo &mem)                            = 0;
    virtual AM_RESULT unmap_bsb()                                           = 0;
    virtual AM_RESULT map_warp(AMMemMapInfo &mem)                           = 0;
    virtual AM_RESULT unmap_warp()                                          = 0;
    virtual AM_RESULT map_usr(AMMemMapInfo &mem)                            = 0;
    virtual AM_RESULT unmap_usr()                                           = 0;

    virtual AM_RESULT gdmacpy(void *dst, const void *src,
                              size_t width, size_t height, size_t pitch)    = 0;

    //For Encode Parameters
    virtual AM_RESULT encode_mode_get(AM_ENCODE_MODE &mode)                 = 0;
    virtual AM_RESULT hdr_type_get(AM_HDR_TYPE &hdr)                        = 0;

    //For DPTZ
    virtual AM_RESULT get_digital_zoom(AM_SOURCE_BUFFER_ID id, AMRect &rect)= 0;
    virtual AM_RESULT set_digital_zoom(AM_SOURCE_BUFFER_ID id, AMRect &rect)= 0;
    virtual AM_RESULT check_ldc_enable(bool &enable)                        = 0;
    virtual AM_DPTZ_TYPE get_dptz()                                         = 0;

    //For WARP
    virtual AM_RESULT get_aaa_info(AMVinAAAInfo &info)                      = 0;
    virtual bool has_warp()                                                 = 0;
    virtual AM_DEWARP_FUNC_TYPE get_dewarp_func()                           = 0;

    //For Bitrate Control
    virtual AM_BITRATE_CTRL_METHOD get_bitrate_ctrl_method()                = 0;

    //For EIS
    virtual AM_RESULT map_eis_warp(AMMemMapInfo &mem)                       = 0;
    virtual AM_RESULT unmap_eis_warp()                                      = 0;

    //FOR Video Motion Detect
    virtual AM_VIDEO_MD_TYPE get_video_md()                                 = 0;

    //FOR Switching Cpu Clock
    virtual AM_RESULT get_avail_cpu_clks(vector<int32_t> &cpu_clks)         = 0;
    virtual AM_RESULT get_cur_cpu_clk(int32_t &cur_clk)                     = 0;
    virtual AM_RESULT set_cpu_clk(const int32_t &index)                     = 0;


  protected:
    virtual void inc_ref()                                                  = 0;
    virtual void release()                                                  = 0;
    virtual ~AMIPlatform() {};
};


#endif /* ORYX_VIDEO_INCLUDE_AM_PLATFORM_H_ */
