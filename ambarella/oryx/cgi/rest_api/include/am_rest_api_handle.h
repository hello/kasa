/**
 * am_rest_api_handle.h
 *
 *  History:
 *		2015年8月24日 - [Huaiqing Wang] created file
 *
 * Copyright (C) 2007-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef ORYX_CGI_INCLUDE_AM_REST_API_HANDLE_H_
#define ORYX_CGI_INCLUDE_AM_REST_API_HANDLE_H_

#include "am_api_helper.h"
#include "am_rest_api_utils.h"

extern AMAPIHelperPtr gCGI_api_helper;
//oryx services handle base class
class AMRestAPIHandle
{
  public:
    static AMRestAPIHandle *create(const std::string &service);

    void  destroy();
    virtual AM_REST_RESULT rest_api_handle() = 0;

  protected:
    AMRestAPIHandle();
    virtual ~AMRestAPIHandle();
    AMRestAPIUtilsPtr  m_utils;

  private:
    AM_REST_RESULT init();
    static AMRestAPIHandle *m_instance;
};


#endif /* ORYX_CGI_INCLUDE_AM_REST_API_HANDLE_H_ */
