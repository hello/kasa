/**
 * am_osd_overlay_if.h
 *
 *  History:
 *    2015-6-24 - [Huaiqing Wang] created file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef ORYX_INCLUDE_VIDEO_AM_OSD_OVERLAY_IF_H_
#define ORYX_INCLUDE_VIDEO_AM_OSD_OVERLAY_IF_H_

#include <string>
#include "iav_ioctl.h"
#include "am_video_dsp.h"

#define OSD_AREA_MAX_NUM       MAX_NUM_OVERLAY_AREA

class AMEncodeDevice;
class AMIOSDOverlay
{
  public:
    static AMIOSDOverlay *create(); //create AMIOSDOverlay instance

    virtual AM_RESULT init(int32_t fd_iav, AMEncodeDevice *encode_device) = 0;

    virtual bool destroy() = 0; //destroy all streams osd

    //add one or more area osd for a stream
    virtual bool add(AM_STREAM_ID stream_id, AMOSDInfo *overlay_info) = 0;

    //if area_id use default value, it will destroy all area osd for the stream.
    //otherwise, just destroy a area osd for the stream
    virtual bool remove(AM_STREAM_ID stream_id, int32_t area_id = OSD_AREA_MAX_NUM) = 0;

    //enable osd display, when it was disable before
    virtual bool enable(AM_STREAM_ID stream_id, int32_t area_id = OSD_AREA_MAX_NUM) = 0;

    //just disable osd display, no destroy action
    virtual bool disable(AM_STREAM_ID stream_id, int32_t area_id = OSD_AREA_MAX_NUM) = 0;

    //default vale print all stream osd information
    virtual void print_osd_infomation(AM_STREAM_ID stream_id = AM_STREAM_MAX_NUM) = 0;

  protected:
    virtual ~AMIOSDOverlay(){};
};

#endif /* ORYX_INCLUDE_VIDEO_AM_OSD_OVERLAY_IF_H_ */
