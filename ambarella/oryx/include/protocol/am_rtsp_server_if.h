/*******************************************************************************
 * am_rtsp_server_if.h
 *
 * History:
 *   2015-1-26 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_INCLUDE_PROTOCOL_AM_RTSP_SERVER_IF_H_
#define ORYX_INCLUDE_PROTOCOL_AM_RTSP_SERVER_IF_H_

#include "am_pointer.h"
#include "version.h"

class AMIRtspServer;
typedef AMPointer<AMIRtspServer> AMIRtspServerPtr;

class AMIRtspServer
{
    friend AMIRtspServerPtr;

  public:
    static AMIRtspServerPtr create();
    virtual bool start()       = 0;
    virtual void stop()        = 0;
    virtual uint32_t version() = 0;

  protected:
    virtual void inc_ref()     = 0;
    virtual void release()     = 0;
    ~AMIRtspServer(){}
};

#endif /* ORYX_INCLUDE_PROTOCOL_AM_RTSP_SERVER_IF_H_ */
