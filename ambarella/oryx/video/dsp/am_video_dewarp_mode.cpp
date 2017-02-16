/*******************************************************************************
 * am_video_dewarp_mode.cpp
 *
 * History:
 *   2014-8-7 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <sys/ioctl.h>
#include "am_base_include.h"
#include "am_video_dsp.h"
#include "am_video_dewarp_mode.h"
#include "am_log.h"
#include "iav_ioctl.h"

//extern AMResolution g_source_buffer_max_size_default[AM_ENCODE_SOURCE_BUFFER_MAX_NUM];
AMVideoDewarpMode::AMVideoDewarpMode()
{
  m_encode_mode = AM_VIDEO_ENCODE_DEWARP_MODE;
}

AMVideoDewarpMode::~AMVideoDewarpMode()
{
}
AMVideoDewarpMode *AMVideoDewarpMode::create()
{
  AMVideoDewarpMode *result = new AMVideoDewarpMode;
  if (!result || result->construct() != AM_RESULT_OK) {
    ERROR("AMVideoDewarpMode::get_instance error\n");
    delete result;
    result = NULL;
  }

  return result;
}
