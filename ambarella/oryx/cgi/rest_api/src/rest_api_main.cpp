/*
 * rest_api_main.cpp
 *
 *  History:
 *		2015-8-10 - [Huaiqing Wang] created file
 *
 * Copyright (C) 2007-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include "am_rest_api.h"

int32_t main(int32_t argc, char **argv)
{
  AMRestAPI *rest_api = AMRestAPI::create();
  if (rest_api) {
    rest_api->run();
    rest_api->destory();
  }

  return 0;
}
