/*
 * am_audio_device.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 15/12/2014 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_audio_device_if.h"
#ifdef BUILD_AMBARELLA_ORYX_AUDIO_DEVICE_PULSE
#include "am_audio_device_pulse.h"
#endif

AMIAudioDevicePtr AMIAudioDevice::create(const std::string& interface)
{
  AMIAudioDevice *dev = NULL;
#ifdef BUILD_AMBARELLA_ORYX_AUDIO_DEVICE_PULSE
  if (is_str_equal(interface.c_str(), "pulse")) {
    if ((dev = AMPulseAudioDevice::get_instance ()) == NULL) {
      ERROR ("Failed to create an instance of AMPulseAudioDevice");
    }
  } else
#endif
  if (is_str_equal(interface.c_str(), "alsa")) {
    ERROR("Audio device using ALSA interfaces is not implemented yet.");
  } else {
    ERROR("Unknown audio interface type: %s", interface.c_str());
  }

  return dev;
}

AMIAudioDevicePtr create_audio_device (const char *interface_name)
{
  return AMIAudioDevice::create(interface_name);
}
