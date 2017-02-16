/*******************************************************************************
 * am_video_address_if.h
 *
 * History:
 *   Sep 11, 2015 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_VIDEO_ADDRESS_IF_H_
#define AM_VIDEO_ADDRESS_IF_H_

#include "am_video_types.h"
#include "am_pointer.h"

class AMIVideoAddress;

typedef AMPointer<AMIVideoAddress> AMIVideoAddressPtr;
class AMIVideoAddress
{
    friend AMIVideoAddressPtr;

  public:
    static AMIVideoAddressPtr get_instance();

  public:
    virtual AM_RESULT addr_get(AM_DATA_FRAME_TYPE type,
                               uint32_t offset,
                               AMAddress &addr)                             = 0;
    virtual AM_RESULT video_addr_get(const AMQueryFrameDesc &desc,
                                     AMAddress &addr)                       = 0;
    virtual AM_RESULT yuv_y_addr_get(const AMQueryFrameDesc &desc,
                                     AMAddress &addr)                       = 0;
    virtual AM_RESULT yuv_uv_addr_get(const AMQueryFrameDesc &desc,
                                      AMAddress &addr)                      = 0;
    virtual AM_RESULT raw_addr_get(const AMQueryFrameDesc &desc,
                                   AMAddress &addr)                         = 0;
    virtual AM_RESULT me1_addr_get(const AMQueryFrameDesc &desc,
                                   AMAddress &addr)                         = 0;
    virtual bool is_new_video_session(uint32_t session_id,
                                      AM_STREAM_ID stream_id)               = 0;

  protected:
    virtual void inc_ref() = 0;
    virtual void release() = 0;
    virtual ~AMIVideoAddress(){};
};


#endif /* AM_VIDEO_ADDRESS_IF_H_ */
