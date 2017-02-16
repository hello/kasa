/*******************************************************************************
 * am_service_frame_if.h
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
#ifndef ORYX_SERVICES_INCLUDE_AM_SERVICE_FRAME_IF_H_
#define ORYX_SERVICES_INCLUDE_AM_SERVICE_FRAME_IF_H_

typedef void (*AMServiceFrameCallback)(char cmd);

class AMIServiceFrame
{
  public:
    static AMIServiceFrame* create(const std::string& service_name);

  public:
    virtual void destroy() = 0;
    /*
     * This function blocks until interrupts are received or quit is called
     */
    virtual void run()     = 0;
    /*
     * This func5tion will make run() return
     */
    virtual bool quit()    = 0;
    virtual void set_user_input_callback(AMServiceFrameCallback cb) = 0;
    virtual uint32_t version() = 0;

  protected:
    virtual ~AMIServiceFrame(){}
};

#endif /* ORYX_SERVICES_INCLUDE_AM_SERVICE_FRAME_IF_H_ */
