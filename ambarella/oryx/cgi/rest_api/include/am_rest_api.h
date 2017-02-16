/**
 * am_rest_api.h
 *
 *  History:
 *		2015年8月17日 - [Huaiqing Wang] created file
 *
 * Copyright (C) 2007-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef ORYX_CGI_INCLUDE_AM_REST_API_H_
#define ORYX_CGI_INCLUDE_AM_REST_API_H_

#include "am_rest_api_handle.h"
#include "am_rest_api_utils.h"

class AMRestAPI
{
  public:
    static AMRestAPI  *create();
    void  destory();
    void  run();

  private:
    AMRestAPI();
    virtual ~AMRestAPI();
    static AMRestAPI *m_instance;
    AMRestAPIHandle  *m_handle;
    AMRestAPIUtilsPtr m_utils;
};


#endif /* ORYX_CGI_INCLUDE_AM_REST_API_H_ */
