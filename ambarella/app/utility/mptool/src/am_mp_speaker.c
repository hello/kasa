/*******************************************************************************
 * am_mp_speaker.cpp
 *
 * History:
 *   Mar 18, 2015 - [longli] created file
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
#include <stdlib.h>
#include "am_mp_server.h"
#include "am_mp_speaker.h"

enum {
  MP_SPEAKER_HAND_SHAKE = 0,
  MP_SPEAKER_PLAY,
  MP_SAVE_RESULT
};

am_mp_err_t mptool_speaker_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg)
{
  int32_t status;
  to_msg->result.ret = MP_OK;

  switch (from_msg->stage) {
    case MP_SPEAKER_HAND_SHAKE:
      break;
    case MP_SPEAKER_PLAY:
      // playback audio data
      status = system("/usr/bin/aplay -f dat /usr/local/bin/test_speaker.wav &");
      if (WEXITSTATUS(status)) {
        printf("result verify: aplay failed!\n");
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;
    case MP_SAVE_RESULT:
      system("kill -9 `pidof aplay`");
      if (mptool_save_data(from_msg->result.type,
                           from_msg->result.ret,
                           SYNC_FILE) != MP_OK) {
        to_msg->result.ret = MP_NOT_SUPPORT;
        printf("save failed!\n");
      }
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}
