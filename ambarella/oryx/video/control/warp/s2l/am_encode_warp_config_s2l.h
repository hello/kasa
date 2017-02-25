/*******************************************************************************
 * am_encode_warp_config_s2l.h
 *
 * History:
 *   2016/1/6 - [smdong] created file
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
#ifndef ORYX_VIDEO_CONTROL_WARP_S2L_AM_ENCODE_WARP_CONFIG_S2L_H_
#define ORYX_VIDEO_CONTROL_WARP_S2L_AM_ENCODE_WARP_CONFIG_S2L_H_

#include <map>
#include <mutex>
#include <atomic>

#include "am_pointer.h"
#include "am_encode_warp_if.h"

class AMWarpConfigS2L;
typedef AMPointer<AMWarpConfigS2L> AMWarpConfigS2LPtr;

struct AMLensParam
{
    float max_hfov_degree;
    float pano_hfov_degree;         // For Panorama only
    float image_circle_mm;          //fisheye lens image circle in mm
    float efl_mm;                   //Effective focal length in mm
    int32_t lut_entry_num;          //lookup table entry num
    int pitch;
    int yaw;
    AMFrac hor_zoom;
    AMFrac ver_zoom;
    AMPoint lens_center_in_max_input;
    std::string lut_file;
    AMLensParam():
      max_hfov_degree(0.0),
      pano_hfov_degree(0.0),
      image_circle_mm(0.0),
      efl_mm(2.1),
      lut_entry_num(0),
      pitch(0),
      yaw(0)
    {
    }
    AMLensParam(const AMLensParam& lens) :
      max_hfov_degree(lens.max_hfov_degree),
      pano_hfov_degree(lens.pano_hfov_degree),
      image_circle_mm(lens.image_circle_mm),
      efl_mm(lens.efl_mm),
      lut_entry_num(lens.lut_entry_num),
      pitch(lens.pitch),
      yaw(lens.yaw),
      hor_zoom(lens.hor_zoom),
      ver_zoom(lens.ver_zoom),
      lut_file(lens.lut_file)
    {
      lens_center_in_max_input.x = lens.lens_center_in_max_input.x;
      lens_center_in_max_input.y = lens.lens_center_in_max_input.y;
    }
};

struct AMWarpConfigS2LParam
{
    std::pair<bool, AM_WARP_PROJECTION_MODE> proj_mode;
    std::pair<bool, AM_WARP_MODE> warp_mode;
    std::pair<bool, int>   max_radius;
    std::pair<bool, float> ldc_strength;
    std::pair<bool, float> pano_hfov_degree;
    std::pair<bool, AMLensParam> lens;
    AMWarpConfigS2LParam():
      proj_mode(false, AM_WARP_PROJRECTION_EQUIDISTANT),
      warp_mode(false, AM_WARP_MODE_RECTLINEAR),
      max_radius(false, 0),
      ldc_strength(false, 0.0),
      pano_hfov_degree(false, 0.0)
    {
    }
    AMWarpConfigS2LParam(const AMWarpConfigS2LParam &conf) :
      proj_mode(conf.proj_mode),
      warp_mode(conf.warp_mode),
      max_radius(conf.max_radius),
      ldc_strength(conf.ldc_strength),
      pano_hfov_degree(conf.pano_hfov_degree),
      lens(conf.lens)
    {
    }
};

class AMWarpConfigS2L
{
    friend AMWarpConfigS2LPtr;
  public:
    static AMWarpConfigS2LPtr get_instance();
    AM_RESULT get_config(AMWarpConfigS2LParam &config);
    AM_RESULT set_config(const AMWarpConfigS2LParam &config);

    AM_RESULT load_config();
    AM_RESULT save_config();

    AMWarpConfigS2L();
    virtual ~AMWarpConfigS2L();
    void inc_ref();
    void release();

  private:
    static AMWarpConfigS2L      *m_instance;
    static std::recursive_mutex  m_mtx;
    AMWarpConfigS2LParam         m_config;
    std::atomic_int              m_ref_cnt;
};

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define WARP_CONF_DIR ((const char*)(BUILD_AMBARELLA_ORYX_CONF_DIR"/video"))
#else
#define WARP_CONF_DIR ((const char*)"/etc/oryx/video")
#endif

#define WARP_CONFIG_FILE "warp.acs"



#endif /* ORYX_VIDEO_CONTROL_WARP_S2L_AM_ENCODE_WARP_CONFIG_S2L_H_ */
