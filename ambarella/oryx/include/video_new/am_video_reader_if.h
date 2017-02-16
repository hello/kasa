/**
 * am_video_reader_if.h
 *
 *  History:
 *    Aug 10, 2015 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef ORYX_INCLUDE_VIDEO_NEW_AM_VIDEO_READER_IF_H_
#define ORYX_INCLUDE_VIDEO_NEW_AM_VIDEO_READER_IF_H_

#include "am_video_types.h"
#include "am_pointer.h"

class AMIVideoReader;

typedef AMPointer<AMIVideoReader> AMIVideoReaderPtr;
class AMIVideoReader
{
    friend AMIVideoReaderPtr;

  public:
    static AMIVideoReaderPtr get_instance();

  public:
    virtual AM_RESULT query_video_frame(AMQueryFrameDesc &desc,
                                        uint32_t timeout = -1) = 0;
    virtual AM_RESULT query_yuv_frame(AMQueryFrameDesc &desc,
                                      AM_SOURCE_BUFFER_ID id,
                                      bool latest_snapshot) = 0;
    virtual AM_RESULT query_me1_frame(AMQueryFrameDesc &desc,
                                      AM_SOURCE_BUFFER_ID id,
                                      bool latest_snapshot) = 0;
    virtual AM_RESULT query_raw_frame(AMQueryFrameDesc &desc) = 0;

    virtual AM_RESULT query_stream_info(AMStreamInfo &info) = 0;

  protected:
    virtual void inc_ref() = 0;
    virtual void release() = 0;
    virtual ~AMIVideoReader(){};
};

#endif /* ORYX_INCLUDE_VIDEO_NEW_AM_VIDEO_READER_IF_H_ */
