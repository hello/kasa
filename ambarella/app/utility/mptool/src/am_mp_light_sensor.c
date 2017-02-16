/*******************************************************************************
 * am_lmp_light_sensor.cpp
 *
 * History:
 *   May 7, 2015 - [longli] created file
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
#include <stdint.h>
#include <string.h>
#include "am_mp_server.h"
#include "am_mp_light_sensor.h"

#include "basetypes.h"
#include "img_dev.h"

typedef struct {
    int32_t channel;
    uint32_t value;
}mp_light_sensor_msg;

am_mp_err_t mptool_light_sensor_handler(am_mp_msg_t *from_msg,
                                        am_mp_msg_t *to_msg)
{
  mp_light_sensor_msg sensor_msg;
  to_msg->result.ret = MP_OK;

  switch (from_msg->stage) {
    case 0:
      break;
    case 1:
      memcpy(&sensor_msg, from_msg->msg, sizeof(sensor_msg));
      if (ir_led_get_adc_value(&(sensor_msg.value)) != 0) {
        to_msg->result.ret = MP_ERROR;
      }
      break;
    case 2:
      if (mptool_save_data(from_msg->result.type,
                           from_msg->result.ret, SYNC_FILE) != MP_OK) {
        to_msg->result.ret = MP_ERROR;
      }
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }
  memcpy(to_msg->msg, &sensor_msg, sizeof(sensor_msg));

  return MP_OK;
}
