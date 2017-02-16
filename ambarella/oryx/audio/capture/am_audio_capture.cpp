/*******************************************************************************
 * am_audio_capture.cpp
 *
 * History:
 *   2014-12-1 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_audio_capture_if.h"
#ifdef BUILD_AMBARELLA_ORYX_AUDIO_CAPTURE_PULSE
#include "am_audio_capture_pulse.h"
#endif

AMIAudioCapture* create_audio_capture(const std::string& interface,
                                      const std::string& name,
                                      void *owner,
                                      AudioCaptureCallback callback)
{
  AMIAudioCapture *capture = NULL;
#ifdef BUILD_AMBARELLA_ORYX_AUDIO_CAPTURE_PULSE
  if (is_str_equal(interface.c_str(), "pulse")) {
    capture = AMAudioCapturePulse::create(owner, name, callback);
  } else
#endif
  if (is_str_equal(interface.c_str(), "alsa")) {
    ERROR("Audio capture using ALSA is not implemented yet!");
  } else {
    ERROR("Unknown audio interface type: %s", interface.c_str());
  }

  return capture;
}
