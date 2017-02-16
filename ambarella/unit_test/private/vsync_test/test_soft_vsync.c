/*******************************************************************************
 * test_soft_vsync.c
 *
 * History:
 *   2014/09/05 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/


#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
static char irq_proc_name[256] = "/proc/ambarella/vin0_idsp";



int main()
{
  int irq_fd = open (irq_proc_name, O_RDONLY);
  char vin_int_array[32];
  struct timeval time1, time2;

  if (irq_fd < 0) {
    printf("unable to open vin irq proc file %s\n", irq_proc_name);
    return -1;
  }

  while(1)
  {
    gettimeofday(&time1, NULL);
    read(irq_fd, vin_int_array, 8);
    gettimeofday(&time2, NULL);
    lseek(irq_fd, 0 , SEEK_SET);
    printf("frame time: %3.4fms\n", (time2.tv_sec-time1.tv_sec)*1000.f +
           (time2.tv_usec-time1.tv_usec)/1000.f);
  }
  return 0;
}
