/*******************************************************************************
 * am_platform_iav2.h
 *
 * History:
 *   Aug 12, 2015 - [ypchang] created file
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
#ifndef AM_PLATFORM_H_
#define AM_PLATFORM_H_

#include "am_platform_if.h"
#include "iav_ioctl.h"

#define OVERLAY_AREA_MAX_NUM      MAX_NUM_OVERLAY_AREA
using std::map;

struct AMDSPCapability
{
    bool basic_hdr;
    bool advanced_hdr;
    bool normal_iso;
    bool normal_iso_plus;
    bool advanced_iso;
    bool single_dewarp;
    bool multi_dewarp;
};

typedef map<iav_dsp_sub_buffer_id, AMMemMapInfo> AMDspSubMemMapInfoMap;
class AMPlatformIav2: public AMIPlatform
{
    friend AMIPlatform;
  public:
    AM_RESULT load_config()                                            override;

    AM_RESULT feature_config_set(const AMFeatureParam &param)          override;
    AM_RESULT feature_config_get(AMFeatureParam &param)                override;
    //For VIN
    AM_RESULT vin_params_get(AMVinInfo &info)                          override;
    AM_RESULT vin_number_get(uint32_t &number)                         override;
    AM_RESULT vin_mode_set(const AMPlatformVinMode &mode)              override;
    AM_RESULT vin_mode_get(AMPlatformVinMode &mode)                    override;
    AM_RESULT vin_fps_set(const AMPlatformVinFps &fps)                 override;
    AM_RESULT vin_fps_get(AMPlatformVinFps &fps)                       override;
    AM_RESULT vin_flip_set(const AMPlatformVinFlip &flip)              override;
    AM_RESULT vin_flip_get(AMPlatformVinFlip &flip)                    override;
    AM_RESULT vin_shutter_set(const AMPlatformVinShutter &shutter)     override;
    AM_RESULT vin_shutter_get(AMPlatformVinShutter &shutter)           override;
    AM_RESULT vin_agc_set(const AMPlatformVinAGC &agc)                 override;
    AM_RESULT vin_agc_get(AMPlatformVinAGC &agc)                       override;
    AM_RESULT vin_info_get(AMPlatformVinInfo &info)                    override;
    AM_RESULT vin_wait_next_frame()                                    override;
    AM_RESULT vin_info_list_get(AM_VIN_ID id,
                                AMPlatformVinInfoList &list)           override;
    AM_RESULT sensor_info_get(AMPlatformSensorInfo &info)              override;

    //For VOUT
    int32_t   vout_sink_id_get(AM_VOUT_ID id, AM_VOUT_TYPE type)       override;
    AM_RESULT vout_active_sink_set(int32_t sink_id)                    override;
    AM_RESULT vout_sink_config_set(const AMPlatformVoutSinkConfig &sink)override;
    AM_RESULT vout_halt(AM_VOUT_ID id)                                 override;

    //For BUFFER
    AM_RESULT buffer_state_get(AM_SOURCE_BUFFER_ID id,
                               AM_SRCBUF_STATE &state)                 override;
    AM_RESULT buffer_setup_set(const AMPlatformBufferFormatMap &format)override;
    AM_RESULT buffer_setup_get(AMPlatformBufferFormatMap &format)      override;
    AM_RESULT buffer_format_set(const AMPlatformBufferFormat &format)  override;
    AM_RESULT buffer_format_get(AMPlatformBufferFormat &format)        override;

    //For EFM Buffer
    AM_RESULT efm_frame_request(AMPlatformEFMRequestFrame &frame)      override;
    AM_RESULT efm_frame_handshake(AMPlatformEFMHandshakeFrame &frame)  override;

    //For STREAM
    AM_RESULT stream_format_set(AMPlatformStreamFormat &format)        override;
    AM_RESULT stream_format_get(AMPlatformStreamFormat &format)        override;
    AM_RESULT stream_config_set()                                      override;
    AM_RESULT stream_config_get()                                      override;
    AM_RESULT stream_h264_config_set(const AMPlatformH26xConfig &h264) override;
    AM_RESULT stream_h264_config_get(AMPlatformH26xConfig &h264)       override;
    AM_RESULT stream_h265_config_set(const AMPlatformH26xConfig &h265) override;
    AM_RESULT stream_h265_config_get(AMPlatformH26xConfig &h265)       override;
    AM_RESULT stream_mjpeg_config_set(
        const AMPlatformMJPEGConfig &mjpeg)                            override;
    AM_RESULT stream_mjpeg_config_get(AMPlatformMJPEGConfig &mjpeg)    override;

    AM_RESULT stream_bitrate_set(const AMPlatformBitrate &bitrate)     override;
    AM_RESULT stream_bitrate_get(AMPlatformBitrate &bitrate)           override;

    AM_RESULT stream_framerate_set(const AMPlatformFrameRate
                                   &framerate)                         override;
    AM_RESULT stream_framerate_get(AMPlatformFrameRate &framerate)     override;

    AM_RESULT stream_mjpeg_quality_get(AMPlatformMJPEGConfig &mjpeg)   override;
    AM_RESULT stream_mjpeg_quality_set(const AMPlatformMJPEGConfig
                                       &mjpeg)                         override;
    AM_RESULT stream_h26x_gop_get(AMPlatformH26xConfig &h264)          override;
    AM_RESULT stream_h26x_gop_set(const AMPlatformH26xConfig &h264)    override;
    AM_RESULT stream_offset_get(AM_STREAM_ID id, AMOffset &offset)     override;
    AM_RESULT stream_offset_set(AM_STREAM_ID id,
                                const AMOffset &offset)                override;

    AM_RESULT stream_state_get(AM_STREAM_ID id, AM_STREAM_STATE &state)override;

    //For RESOURCE LIMIT
    AM_RESULT system_resource_set(const AMBufferParamMap &buffer,
                                  const AMStreamParamMap &stream)      override;
    AM_RESULT system_resource_get(AMPlatformResourceLimit &res)        override;
    uint32_t  system_max_stream_num_get()                              override;
    uint32_t  system_max_buffer_num_get()                              override;

    //For ENCODE/DECODE
    AM_RESULT goto_idle()                                              override;
    AM_RESULT goto_preview()                                           override;
    AM_RESULT encode_start(uint32_t stream_bits)                       override;
    AM_RESULT encode_stop(uint32_t stream_bits)                        override;
    AM_RESULT decode_start()                                           override;
    AM_RESULT decode_stop()                                            override;
    AM_RESULT iav_state_get(AM_IAV_STATE &state)                       override;

    AM_RESULT get_power_mode(AM_POWER_MODE &mode)                      override;
    AM_RESULT set_power_mode(AM_POWER_MODE mode)                       override;

    //For Querying frames
    AM_RESULT query_frame(AMPlatformFrameInfo &frame)                  override;

    //For encode dynamic control
    AM_RESULT force_idr(AM_STREAM_ID id)                               override;

    //For overlay
    AM_RESULT overlay_set(const AMOverlayInsert &overlay)              override;
    AM_RESULT overlay_get(AMOverlayInsert &overlay)                    override;
    uint32_t  overlay_max_area_num()                                   override;
    AM_OVERLAY_TYPE get_overlay()                                      override;

    AM_RESULT map_overlay(AMMemMapInfo &mem)                           override;
    AM_RESULT unmap_overlay()                                          override;
    AM_RESULT map_dsp()                                                override;
    AM_RESULT unmap_dsp()                                              override;
    AM_RESULT get_dsp_mmap_info(AM_DSP_SUB_BUF_ID id,
                                AMMemMapInfo &mem)                     override;
    AM_RESULT map_bsb(AMMemMapInfo &mem)                               override;
    AM_RESULT unmap_bsb()                                              override;
    AM_RESULT map_warp(AMMemMapInfo &mem)                              override;
    AM_RESULT unmap_warp()                                             override;
    AM_RESULT map_usr(AMMemMapInfo &mem)                               override;
    AM_RESULT unmap_usr()                                              override;

    AM_RESULT gdmacpy(void *dst, const void *src,
                      size_t width, size_t height, size_t pitch)       override;

    //For Encode Parameters
    AM_RESULT encode_mode_get(AM_ENCODE_MODE &mode)                    override;
    AM_RESULT hdr_type_get(AM_HDR_TYPE &hdr)                           override;

    //For DPTZ
    AM_RESULT get_digital_zoom(AM_SOURCE_BUFFER_ID id, AMRect &rect)   override;
    AM_RESULT set_digital_zoom(AM_SOURCE_BUFFER_ID id, AMRect &rect)   override;
    AM_RESULT check_ldc_enable(bool &enable)                           override;
    AM_DPTZ_TYPE get_dptz()                                            override;

    //For WARP
    AM_RESULT get_aaa_info(AMVinAAAInfo &info)                         override;
    bool has_warp()                                                    override;
    AM_DEWARP_FUNC_TYPE get_dewarp_func()                              override;

    //For Bitrate Control
    AM_BITRATE_CTRL_METHOD get_bitrate_ctrl_method()                   override;

    //For EIS
    AM_RESULT map_eis_warp(AMMemMapInfo &mem)                          override;
    AM_RESULT unmap_eis_warp()                                         override;

    //FOR Video Motion Detect
    AM_VIDEO_MD_TYPE get_video_md()                                    override;

    // For cpu clock set
    AM_RESULT get_avail_cpu_clks(vector<int32_t> &cpu_clks)            override;
    AM_RESULT get_cur_cpu_clk(int32_t &cur_clk)                        override;
    AM_RESULT set_cpu_clk(const int32_t &index)                        override;

  private:
    AM_RESULT select_mode_by_features(AMFeatureParam &param);

  private:
    static AMPlatformIav2    *m_instance;
    static recursive_mutex    m_mtx;

    int                   m_iav = -1;
    atomic_int            m_ref_cnt = {0};
    bool                  m_is_overlay_mapped = false;
    bool                  m_is_dsp_mapped = false;
    bool                  m_is_dsp_sub_mapped = false;
    bool                  m_is_bsb_mapped = false;
    bool                  m_is_warp_mapped = false;
    bool                  m_is_eis_warp_mapped = false;
    bool                  m_is_usr_mapped = false;
    AMFeatureConfigPtr    m_feature_config;
    AMFeatureParam        m_feature_param;
    AMResourceConfigPtr   m_resource_config;
    AMResourceParam       m_resource_param;

    AMMemMapInfo          m_overlay_mem;
    AMMemMapInfo          m_dsp_mem;
    AMMemMapInfo          m_bsb_mem;
    AMMemMapInfo          m_warp_mem;
    AMMemMapInfo          m_eis_warp_mem;
    AMMemMapInfo          m_usr_mem;
    AMDspSubMemMapInfoMap m_dsp_sub_mem;

  private:
    static AMPlatformIav2* get_instance();
    static AMPlatformIav2* create();
    void inc_ref();
    void release();

    AM_RESULT init();
    AMPlatformIav2();
    virtual ~AMPlatformIav2();
};

#endif /* AM_PLATFORM_H_ */
