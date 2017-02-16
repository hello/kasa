/**
 * am_rest_api_image.h
 *
 *  History:
 *		2015年8月19日 - [Huaiqing Wang] created file
 *
 * Copyright (C) 2007-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef ORYX_CGI_INCLUDE_AM_REST_API_IMAGE_H_
#define ORYX_CGI_INCLUDE_AM_REST_API_IMAGE_H_

#include "am_rest_api_handle.h"

class AMRestAPIImage: public AMRestAPIHandle
{
  public:
    virtual AM_REST_RESULT  rest_api_handle();

  private:
};

#endif /* ORYX_CGI_INCLUDE_AM_REST_API_IMAGE_H_ */
