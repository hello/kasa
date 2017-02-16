/*******************************************************************************
 * am_dptz_warp.h
 *
 * Histroy:
 *  2014-7-10         - [Louis ] created
 *  2014-11-11          [Louis]  modified it to am_dptz_warp.h
 *                      to that digital PTZ and Lens distortion correction
 *                      are done together
 * Copyright (C) 2008-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_DPTZ_WARP_H_
#define AM_DPTZ_WARP_H_

#include "am_dptz_warp_if.h"
#include "am_video_dsp.h"

class AMEncodeDevice;
class AMDPTZWarp: public AMIDPTZWarp
{
  public:
    AMDPTZWarp();
    virtual ~AMDPTZWarp();

  public:
    AM_RESULT set_dptz_ratio(AM_ENCODE_SOURCE_BUFFER_ID buf_id,
                             AMDPTZRatio *dptz_ratio);
    AM_RESULT set_dptz_input_window(AM_ENCODE_SOURCE_BUFFER_ID buf_id,
                                    AMRect *input_window);
    /* both ratio and input_window will be filled when calling get_xxx */
    AM_RESULT get_dptz_param(AM_ENCODE_SOURCE_BUFFER_ID buf_id,
                             AMDPTZParam *dptz_param);
    /* just set, no get, because easy set only has two choices */
    AM_RESULT set_dptz_easy(AM_ENCODE_SOURCE_BUFFER_ID  buf_id,
                            AM_EASY_ZOOM_MODE mode);
    //dptz and warp must apply together
    /* apply DPTZ to DSP.
     * just set_dptz_xxx will only save data but do not auto apply */
    AM_RESULT apply_dptz(AM_ENCODE_SOURCE_BUFFER_ID buf_id, AMDPTZParam *dptz_param);
    AM_RESULT apply_warp();
    AM_RESULT apply_dptz_warp();
    AM_RESULT apply_dptz_warp_all();
    AM_RESULT set_warp_param(AMWarpParam *warp_param);
    AM_RESULT get_warp_param(AMWarpParam *warp_param);
    AM_RESULT set_config(AMDPTZWarpConfig *dptz_warp_config);
    AM_RESULT set_config_all(AMDPTZWarpConfig *dptz_warp_config);
    AM_RESULT get_config(AMDPTZWarpConfig *dptz_warp_config);
    AMEncodeSourceBufferFormat *get_source_buffer_format(AM_ENCODE_SOURCE_BUFFER_ID id);

  public:
    AM_RESULT init(int fd_iav, AMEncodeDevice *m_encode_device);

  protected:
    AM_RESULT get_input_buffer_size(AM_ENCODE_SOURCE_BUFFER_ID buf_id,
                                    AMResolution *input_res);
    AM_RESULT set_dptz_param(AM_ENCODE_SOURCE_BUFFER_ID buf_id,
                             AMDPTZParam *dptz_param);
    AM_RESULT dz_ratio_to_input_window(AM_ENCODE_SOURCE_BUFFER_ID buf_id,
                                       AMDPTZRatio *dz_ratio,
                                       AMRect *input_window);
    AM_RESULT update_warp_control(struct iav_warp_main* p_control);
    int get_grid_spacing(const int spacing);
    bool is_ldc_enable();

  protected:
    AM_ENCODE_SOURCE_BUFFER_ID m_dptz_warp_buf_id;
    AMDPTZParam m_dptz_param[AM_ENCODE_SOURCE_BUFFER_MAX_NUM];
    AMWarpParam m_warp_param;
    /* so that DPTZ can retrieve VIn and buffer info from encode device */
    AMEncodeDevice *m_encode_device;
    int  m_iav;
    bool m_init_done;
    bool m_is_warp_mapped;
    double m_pixel_width_um;
    AM_WARP_ZOOM_SOURCE m_zoom_source;
    bool m_is_ldc_enable;
    bool m_is_ldc_checked;
    AMMemMapInfo m_warp_mem;
    dewarp_init_t m_dewarp_init_param;
    warp_vector_t m_lens_warp_vector;
    warp_region_t m_lens_warp_region;
    /* distortion lookup table */
    int m_distortion_lut[MAX_DISTORTION_LOOKUP_TABLE_ENTRY_NUM];
};

#endif  /*AM_DPTZ_WARP_H_ */
