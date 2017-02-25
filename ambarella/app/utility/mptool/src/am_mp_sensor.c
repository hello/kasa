/*******************************************************************************
 * am_mp_sensor.cpp
 *
 * History:
 *   May 8, 2015 - [longli] created file
 *
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


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
