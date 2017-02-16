/*******************************************************************************
 * am_encode_device_if.h
 *
 * History:
 *   2014-12-15 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_INCLUDE_VIDEO_AM_ENCODE_DEVICE_IF_H_
#define ORYX_INCLUDE_VIDEO_AM_ENCODE_DEVICE_IF_H_

#include "am_video_types.h"
#include "am_video_dsp.h"

class AMIDPTZWarp;
class AMLBRControl;
class AMIVinConfig;
class AMEncodeStream;
class AMEncodeSourceBuffer;
class AMIEncodeDeviceConfig;
class AMIOSDOverlay;
class AMIEncodeDevice
{
  public:
    // Encode device related APIs
    virtual AM_RESULT start_encode_stream(AM_STREAM_ID id) = 0;
    virtual AM_RESULT stop_encode_stream(AM_STREAM_ID id) = 0;
    virtual AM_RESULT show_stream_info(AM_STREAM_ID id) = 0;
    virtual AM_RESULT set_stream_bitrate(AM_STREAM_ID id, uint32_t bitrate) = 0;
    virtual AM_RESULT set_stream_fps(AM_STREAM_ID id, uint32_t fps) = 0;
    virtual AM_RESULT get_stream_fps(AM_STREAM_ID id, uint32_t *fps) = 0;
    virtual AM_RESULT show_source_buffer_info(AM_ENCODE_SOURCE_BUFFER_ID id)= 0;
    virtual AM_RESULT set_force_idr(AM_STREAM_ID id) = 0;
    virtual AMIDPTZWarp* get_dptz_warp() = 0;
    virtual AMIVinConfig* get_vin_config() = 0;
    virtual AMIOSDOverlay* get_osd_overlay() = 0;
    virtual AMIEncodeDeviceConfig* get_encode_config() = 0;
    virtual AMEncodeSourceBuffer* get_source_buffer_list() = 0;
    virtual AMEncodeStream* get_encode_stream_list() = 0;
    virtual AMLBRControl* get_lbr_ctrl_list() = 0;
    virtual bool is_motion_enable() = 0;
    virtual void update_motion_state(void *msg) = 0;
    virtual AM_RESULT set_lbr_config(AMEncodeLBRConfig *lbr_config) = 0;
    virtual AM_RESULT get_lbr_config(AMEncodeLBRConfig *lbr_config) = 0;
    virtual AM_RESULT set_dptz_warp_config(AMDPTZWarpConfig *dptz_warp_config) = 0;
    virtual AM_RESULT get_dptz_warp_config(AMDPTZWarpConfig *dptz_warp_config) = 0;
    virtual ~AMIEncodeDevice(){}
};

#endif /* ORYX_INCLUDE_VIDEO_AM_ENCODE_DEVICE_IF_H_ */
