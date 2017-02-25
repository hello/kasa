/*******************************************************************************
 * am_encode_warp_s2l.h
 *
 * History:
 *   Nov 6, 2015 - [zfgong] created file
 *   Sep 22, 2016 - [Huaiqing Wang] modified file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#ifndef ORYX_VIDEO_INCLUDE_AM_ENCODE_WARP_S2L_H_
#define ORYX_VIDEO_INCLUDE_AM_ENCODE_WARP_S2L_H_

#include "lib_dewarp_header.h"
#include "lib_dewarp.h"
#include "iav_ioctl.h"
#include "am_encode_warp.h"
#include "am_encode_warp_config_s2l.h"

class AMEncodeWarpS2L: public AMEncodeWarp
{
    typedef AMEncodeWarp inherited;

  public:
    static AMEncodeWarpS2L* create(AMVin *vin, AMEncodeStream *stream);
    virtual void destroy();
    virtual void* get_interface();
    virtual AM_RESULT set_ldc_mode(int region_id, AM_WARP_MODE mode);
    virtual AM_RESULT get_ldc_mode(int region_id, AM_WARP_MODE &mode);
    virtual AM_RESULT set_ldc_strength(int region_id, float strength);
    virtual AM_RESULT get_ldc_strength(int region_id, float &strength);
    virtual AM_RESULT set_max_radius(int region_id, int radius);
    virtual AM_RESULT get_max_radius(int region_id, int &radius);
    virtual AM_RESULT set_pano_hfov_degree(int region_id, float degree);
    virtual AM_RESULT get_pano_hfov_degree(int region_id, float &degree);
    virtual AM_RESULT set_warp_region_yaw_pitch(int region_id,
                                                int yaw, int pitch);
    virtual AM_RESULT get_warp_region_yaw_pitch(int region_id,
                                                int &yaw, int &pitch);
    virtual AM_RESULT set_hor_ver_zoom(int region_id, AMFrac hor, AMFrac ver);
    virtual AM_RESULT get_hor_ver_zoom(int region_id, AMFrac &hor, AMFrac &ver);
    virtual AM_RESULT apply();
    //for config files
    virtual AM_RESULT load_config();
    virtual AM_RESULT save_config();

  private:
    AMEncodeWarpS2L();
    virtual ~AMEncodeWarpS2L();
    AM_RESULT init_lens_warp_param(dewarp_init_t &param, warp_region_t &region);
    AM_RESULT create_lens_warp_vector(warp_region_t &region,
                                      warp_vector_t &vector);
    AM_RESULT update_lens_warp_area(warp_vector_t &vector, iav_warp_area &area);
    AM_RESULT check_warp_control(iav_warp_area &area, dewarp_init_t &param);
    int get_grid_spacing(const int spacing);

  private:
    AMWarpConfigS2LPtr       m_config;
    AMWarpConfigS2LParam     m_config_param;


};

#endif /* ORYX_VIDEO_INCLUDE_AM_ENCODE_WARP_S2L_H_ */
