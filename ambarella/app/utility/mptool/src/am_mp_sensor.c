/*******************************************************************************
 * am_mp_sensor.cpp
 *
 * History:
 *   May 8, 2015 - [longli] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "am_mp_server.h"
#include "am_mp_sensor.h"


#define SENSOR_TEST_TIME (3) //time unit second
#define VIN_ARRAY_SIZE (8)
#define VIN0_IDSP_LAST_INT "/proc/ambarella/vin0_idsp"

am_mp_err_t mptool_sensor_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg)
{
  char vin_int_array[VIN_ARRAY_SIZE] = { 0 };
  int32_t vin_idsp_fd = -1;
  int32_t count_idsp_last_int1 = 0, count_idsp_last_int2 = 0;
  to_msg->result.ret = MP_OK;

  switch (from_msg->stage) {
    case 0:
      break;
    case 1:
      system("/usr/local/bin/test_encode -i0 -X --bmaxsize 1080p --bsize 1080p "
          "-J --btype off -A -h1080p -e");

      if ((vin_idsp_fd = open(VIN0_IDSP_LAST_INT, O_RDONLY)) < 0) {
        printf("CANNOT open [%s].\n", VIN0_IDSP_LAST_INT);
        to_msg->result.ret = MP_ERROR;
        break;
      }

      read(vin_idsp_fd, vin_int_array, VIN_ARRAY_SIZE);
      sscanf(vin_int_array, "%x", &count_idsp_last_int1);

      sleep(SENSOR_TEST_TIME);
      system("/usr/local/bin/test_encode -A -h720p -s");

      memset(vin_int_array, 0, sizeof(vin_int_array));
      lseek(vin_idsp_fd, 0, SEEK_SET);
      read(vin_idsp_fd, vin_int_array, VIN_ARRAY_SIZE);
      sscanf(vin_int_array, "%x", &count_idsp_last_int2);

      printf("Diff IRQ count num : %d \n",
             count_idsp_last_int2 - count_idsp_last_int1);
      if ((count_idsp_last_int2 - count_idsp_last_int1) / SENSOR_TEST_TIME
          < 25) {
        to_msg->result.ret = MP_ERROR;
      }

      mptool_save_data(from_msg->result.type, to_msg->result.ret, SYNC_FILE);

      close(vin_idsp_fd);
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}
