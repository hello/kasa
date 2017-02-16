/*******************************************************************************
 * am_jpeg_snapshot_if.h
 *
 * History:
 *   2015-09-18 - [zfgong] created file
 *
 * Copyright (C) 2015-2016, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_INCLUDE_AM_JPEG_SNAPSHOT_IF_H_
#define ORYX_INCLUDE_AM_JPEG_SNAPSHOT_IF_H_

#include "am_pointer.h"

class AMIJpegSnapshot;
typedef AMPointer<AMIJpegSnapshot> AMIJpegSnapshotPtr;

class AMIJpegSnapshot
{
    friend class AMPointer<AMIJpegSnapshot>;

  public:
    static AMIJpegSnapshotPtr get_instance();
    virtual AM_RESULT init() = 0;
    virtual void destroy() = 0;
    virtual AM_RESULT start() = 0;
    virtual AM_RESULT stop() = 0;

    virtual void update_motion_state(AM_EVENT_MESSAGE *msg) = 0;
    virtual bool is_enable() = 0;

  protected:
    virtual void release() = 0;
    virtual void inc_ref() = 0;
    virtual ~AMIJpegSnapshot(){}
};




#endif /* ORYX_INCLUDE_AM_JPEG_SNAPSHOT_IF_H_ */
