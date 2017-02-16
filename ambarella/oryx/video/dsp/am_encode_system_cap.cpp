/*******************************************************************************
 * am_encode_system_resource.cpp
 *
 * History:
 *   2014-8-6日 - [lysun] created file
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
#include "am_log.h"
#include "am_video_dsp.h"
#include "am_encode_system_cap.h"
#include "iav_ioctl.h"

static AMEncodeDSPCapability  g_dsp_cap_by_modes[AM_VIDEO_ENCODE_MODE_NUM] = {
 /*
    bool basic_hdr;
    bool advanced_hdr;
    bool normal_iso;
    bool normal_plus_iso;

    bool advanced_iso;
    bool single_dewarp;
    bool multi_dewarp;

  */
 { 1, 0, 1, 0,   0, 1, 0}, //mode 0
 { 0, 0, 0, 0,   0, 0, 0}, //mode 1 (future)
 { 0, 0, 0, 0,   0, 0, 0}, //mode 2 (reserved)
 { 0, 0, 0, 0,   0, 0, 0}, //mode 3 (not used)
 { 1, 0, 0, 1,   1, 1, 0}, //mode 4
 { 0, 1, 0, 0,   1, 0, 0}, //mode 5
};


AMEncodeSystemCapability::AMEncodeSystemCapability()
{
}

AMEncodeSystemCapability::~AMEncodeSystemCapability()
{
}

AM_RESULT AMEncodeSystemCapability::get_encode_mode_capability(AM_ENCODE_MODE mode,
                                                               AMEncodeDSPCapability *dsp_cap)
{
  AM_RESULT result = AM_RESULT_OK;
  int mode_id;
  do {
    mode_id = (int) mode;
    if ((mode_id < 0) || (mode_id > (int) AM_VIDEO_ENCODE_MODE_NUM - 1)) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (!dsp_cap) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    *dsp_cap = g_dsp_cap_by_modes[mode_id];
  } while (0);

  return result;
}

AM_RESULT AMEncodeSystemCapability::get_encode_performance(AMEncodePerformance *perf)
{
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeSystemCapability::get_encode_performance_max(AM_ENCODE_MODE mode,
                                                               AMEncodePerformance *perf)
{
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeSystemCapability::find_mode_for_dsp_capability(AMEncodeDSPCapability *req,
                                                                 AM_ENCODE_MODE *next_mode)
{
  AM_RESULT result = AM_RESULT_OK;
  int mode_id;
  AMEncodeDSPCapability dsp_cap;
  bool found = false;
  do {
    if (!req || !next_mode) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    for (mode_id = (int) AM_VIDEO_ENCODE_MODE_FIRST;
        mode_id <= (int) AM_VIDEO_ENCODE_MODE_LAST; mode_id ++) {

      result = get_encode_mode_capability(AM_ENCODE_MODE(mode_id), &dsp_cap);
      if (result != AM_RESULT_OK) {
        break;
      }

      //compare dsp_cap and cap
      //if this mode does not support some advanced feature, however requested

      if (((!dsp_cap.basic_hdr) && (req->basic_hdr))
          || ((!dsp_cap.advanced_hdr) && (req->advanced_hdr))
          || ((!dsp_cap.normal_plus_iso) && (req->normal_plus_iso))
          || ((!dsp_cap.advanced_iso) && (req->advanced_iso))
          || ((!dsp_cap.single_dewarp) && (req->single_dewarp))
          || ((!dsp_cap.multi_dewarp) && (req->multi_dewarp))) {
        //this mode cannot be used, look for the next one
        continue;
      } else {
        INFO("AMEncodeSysteResource: find encode mode %d to use\n", mode_id);
        *next_mode = AM_ENCODE_MODE(mode_id);
        found = true;
        break;
      }
    }

    if (!found) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("AMEncodeSystemCapability: request features cannot be supported\n");
    }
  } while (0);

  return result;
}
