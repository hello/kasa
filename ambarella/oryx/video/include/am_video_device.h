/*******************************************************************************
 * am_video_device.h
 *
 * Histroy:
 *  2012-3-6 2012 - [ypchang] created file
 *  2014-6-24 2014 - [Louis] modify this for Oryx MW
 *
 * Copyright (C) 2008-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_VIDEO_DEVICE_H_
#define AM_VIDEO_DEVICE_H_

#include "am_vin.h"
#include "am_vout.h"
#include "am_video_param.h"
#include "am_osd_overlay.h"

/*******************************************************************************
 * AmVideoDevice is the base class of all the video devices,
 * DO NOT use this class directly, use the inherited classes instead
 ******************************************************************************/

class AMVideoDevice
{
  public:
    AMVideoDevice();
    virtual ~AMVideoDevice();
    AM_RESULT init();
    virtual AM_RESULT destroy();

    AM_RESULT goto_idle();
    AM_RESULT start(); //start dsp and required VIN/VOUT
    AM_RESULT stop(); //stop dsp  and required VIN/VOUT
    AM_RESULT update(); //on the fly update current mode

    AM_RESULT get_version(AMVideoVersion *version);
    bool is_vout_enabled();
    AMResolution get_vout_resolution();
    AMResolution get_vin_resolution(AM_VIN_ID id);
    AM_ENCODE_SOURCE_BUFFER_ID get_vout_related_buffer_id();

    AM_RESULT set_vin_hdr_type(AM_HDR_EXPOSURE_TYPE hdr_type);
    AM_RESULT get_vin_hdr_type(AM_HDR_EXPOSURE_TYPE *hdr_type);

    AMVinConfig* get_vin_config();
    AMVoutConfig* get_vout_config();

    AMVin* get_primary_vin();

  protected:
    int get_iav();
    AM_DSP_STATE get_dsp_state();
    AM_RESULT check_init();
    AM_RESULT load_config();
    AM_RESULT save_config();

    AM_RESULT update_dsp_state();
    AM_RESULT vin_create(AMVinParamAll *vin_param);
    AM_RESULT vin_destroy();
    AM_RESULT vin_change(AMVinParamAll *vin_param); //on the fly change without reset
    AM_RESULT vin_setup();

    AM_RESULT vout_create(AMVoutParamAll *vout_param);
    AM_RESULT vout_destroy();
    AM_RESULT vout_change(AMVoutParamAll *vout_param); //on the fly change without reset
    AM_RESULT vout_setup();

  protected:
    bool m_init_ready;
    AM_HDR_EXPOSURE_TYPE m_hdr_type;

  private:
    AM_DSP_STATE m_dsp_state;

    AMVin *m_vin[AM_VIN_MAX_NUM];
    const int32_t m_max_vin_num;

    AMVout *m_vout[AM_VOUT_MAX_NUM];
    const int32_t m_max_vout_num;

    AMVinConfig *m_vin_config;
    AMVoutConfig *m_vout_config;

    int m_iav; //iav file handle
    int32_t m_iav_state;

    bool m_first_create_vin;
    bool m_first_create_vout;
};

#endif /* AM_VIDEO_DEVICE_H_ */
