/*******************************************************************************
 * test_am_pid_lock.cpp
 *
 * History:
 *   2014-9-4 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "am_log.h"

#include "am_pid_lock.h"

int main()
{
  int c;
  PRINTF("trying to get PID file, my pid is %d\n", getpid());
  AMPIDLock lock;
  if (lock.try_lock() < 0) {
    ERROR("unable to lock PID, same name process should be running already\n");
    return -1;
  } else {
    PRINTF("successfully got PID lock, press 'x' to quit\n");
  }

  do {
    c = getchar();
  } while (c != 'x');

  PRINTF("Quit process and delete PID lock\n");
  return 0;
}
