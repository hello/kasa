/*
 * am_audio_device_pulse.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 08/12/2014 [Created]
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
 */
#include "am_base_include.h"
#include "am_log.h"
#include "am_audio_device_pulse.h"
#include <utility>

#define CONTEXT_NAME ((const char *)"AMPulseAudioDevice")

AMPulseAudioDevice *AMPulseAudioDevice::m_instance = nullptr;
std::mutex AMPulseAudioDevice::m_mutex;

struct UserData
{
    AMPulseAudioDevice *adev;
    void *data;
    UserData(AMPulseAudioDevice *dev, void *userData) :
        adev(dev),
        data(userData)
    {
    }
};

struct AMAudioVolumeInfo
{
    uint8_t volume;
    uint8_t mute;
};

static const char *device_str[] =
{ "mic", "speaker", "playback stream", "recording stream" };

AMPulseAudioDevice::AMPulseAudioDevice() :
    m_sink_volume(nullptr),
    m_sink_input_volume(nullptr),
    m_source_volume(nullptr),
    m_source_output_volume(nullptr),
    m_context(nullptr),
    m_threaded_mainloop(nullptr),
    m_sink_index(-1),
    m_sink_input_index(-1),
    m_source_index(-1),
    m_source_output_index(-1),
    m_context_state(PA_CONTEXT_UNCONNECTED),
    m_ctx_connected(false),
    m_sink_muted(false),
    m_sink_input_muted(false),
    m_source_muted(false),
    m_source_output_muted(false),
    m_list_flag(false),
    m_set_operation_success(false),
    m_reference_count(0)
{
  memset(&m_sample_spec, 0, sizeof(m_sample_spec));
}

AMPulseAudioDevice::~AMPulseAudioDevice()
{
  if (m_ctx_connected) {
    pa_context_set_state_callback(m_context, nullptr, nullptr);
    pa_context_disconnect(m_context);
    m_ctx_connected = false;
    DEBUG("pa_context_disconnect\n");
  }

  if (m_context) {
    pa_context_unref(m_context);
    m_context = nullptr;
    DEBUG("pa_context_unref (m_context)\n");
  }

  if (m_threaded_mainloop) {
    pa_threaded_mainloop_free(m_threaded_mainloop);
    m_threaded_mainloop = nullptr;
    DEBUG("pa_threaded_mainloop_free (m_threaded_mainloop)\n");
  }

  delete m_sink_volume;
  delete m_sink_input_volume;
  delete m_source_volume;
  delete m_source_output_volume;
}

AMIAudioDevice *AMPulseAudioDevice::get_instance()
{
  m_mutex.lock();

  if (m_instance == nullptr) {
    if ((m_instance = new AMPulseAudioDevice()) == nullptr) {
      ERROR("Failed to create an instance of AMPulseAudioDevice!");
    } else {
      if (!m_instance->audio_device_init()) {
        ERROR("Failed to initialize audio device!\n");
        delete m_instance;
        m_instance = nullptr;
      }
    }
  }

  m_mutex.unlock();
  return m_instance;
}

void AMPulseAudioDevice::release()
{
  if ((m_reference_count >= 0) && (-- m_reference_count <= 0)) {
    DEBUG("Last reference of AMPulseAudioDevice's object: %p", m_instance);
    delete m_instance;
    m_instance = nullptr;
  }
}

void AMPulseAudioDevice::inc_ref()
{
  ++ m_reference_count;
}

bool AMPulseAudioDevice::audio_device_init()
{
  bool ret = true;


  do {
    UserData userData(this, m_threaded_mainloop);
    pa_mainloop_api *mainloop_api = nullptr;

    if (m_threaded_mainloop != nullptr) {
      INFO("m_threaded_mainloop already created!\n");
      break;
    }
    setenv("PULSE_RUNTIME_PATH", "/var/run/pulse/", 1);
    m_threaded_mainloop = pa_threaded_mainloop_new();
    if (m_threaded_mainloop == nullptr) {
      ret = false;
      ERROR("Failed to new threaded mainloop!\n");
      break;
    }
    userData.data = m_threaded_mainloop;
    mainloop_api = pa_threaded_mainloop_get_api(m_threaded_mainloop);
    m_context = pa_context_new(mainloop_api, CONTEXT_NAME);

    if (m_context == nullptr) {
      ret = false;
      ERROR("Failed to new context!\n");
      break;
    }

    pa_context_set_state_callback(m_context, static_pa_state, &userData);
    pa_context_connect(m_context, nullptr, PA_CONTEXT_NOFLAGS, NULL);

    if (pa_threaded_mainloop_start(m_threaded_mainloop) != 0) {
      ret = false;
      ERROR("Failed to start threaded mainloop!\n");
      break;
    }

    pa_threaded_mainloop_lock(m_threaded_mainloop);
    while ((m_context_state = pa_context_get_state(m_context)) !=
           PA_CONTEXT_READY) {
      if ((m_context_state == PA_CONTEXT_FAILED) ||
          (m_context_state == PA_CONTEXT_TERMINATED)) {
        break;
      }
      /* Will be signaled in pa_state()*/
      pa_threaded_mainloop_wait(m_threaded_mainloop);
    }
    pa_threaded_mainloop_unlock(m_threaded_mainloop);
    pa_context_set_state_callback(m_context, nullptr, nullptr);

    m_ctx_connected = (m_context_state == PA_CONTEXT_READY);
    if (!m_ctx_connected) {
      if ((m_context_state == PA_CONTEXT_TERMINATED) ||
          (m_context_state == PA_CONTEXT_FAILED)) {
        pa_threaded_mainloop_lock(m_threaded_mainloop);
        pa_context_disconnect(m_context);
        pa_threaded_mainloop_unlock(m_threaded_mainloop);
      }

      m_ctx_connected = false;

      ret = false;
      INFO("pa_conext_connect failed: %u!\n", m_context_state);
    }

    m_sink_volume = new pa_cvolume();
    if (nullptr == m_sink_volume) {
      ERROR("Failed to allocate memory for sink volume!");
      ret = false;
      break;
    }

    m_source_volume = new pa_cvolume();
    if (nullptr == m_source_volume) {
      ERROR("Failed to allocate memory for source volume!");
      ret = false;
      break;
    }

    m_sink_input_volume = new pa_cvolume();
    if (nullptr == m_sink_input_volume) {
      ERROR("Failed to allocate memory for sink input volume!");
      ret = false;
      break;
    }

    m_source_output_volume = new pa_cvolume();
    if (nullptr == m_source_output_volume) {
      ERROR("Failed to allocate memory for source output volume!");
      ret = false;
      break;
    }
  } while (0);

  if (!ret && m_threaded_mainloop) {
    pa_threaded_mainloop_stop(m_threaded_mainloop);
  }

  return ret;
}

bool AMPulseAudioDevice::get_volume_by_index(AM_AUDIO_DEVICE_TYPE device,
                                             unsigned char *volume,
                                             int index)
{
  bool ret = true;
  AMAudioVolumeInfo volume_info;

  if (!unified_entrance_for_getting_volume_info(device,
                                                &volume_info,
                                                (void *) &index,
                                                false)) {
    ERROR("Failed to get volume of %s!", device_str[device]);
    ret = false;
  } else {
    *volume = volume_info.volume;
  }

  return ret;
}

bool AMPulseAudioDevice::get_volume_by_name(AM_AUDIO_DEVICE_TYPE device,
                                            unsigned char *volume,
                                            const char *device_name)
{
  bool ret = true;
  AMAudioVolumeInfo volume_info;

  if (!unified_entrance_for_getting_volume_info(device,
                                                &volume_info,
                                                (void *) device_name,
                                                true)) {
    ERROR("Failed to get volume of %s!", device_str[device]);
    ret = false;
  } else {
    *volume = volume_info.volume;
  }

  return ret;
}

bool AMPulseAudioDevice::set_volume_by_index(AM_AUDIO_DEVICE_TYPE device,
                                             unsigned char volume,
                                             int index)
{
  AMAudioVolumeInfo volume_info;
  AM_AUDIO_DEVICE_OPERATION op;

  if (volume > 100) {
    ERROR("Invalid argument: volume [0 - 100]: %u", volume);
    return false;
  }

  volume_info.volume = volume;
  op = AM_AUDIO_DEVICE_OPERATION_VOLUME_SET;
  return unified_entrance_for_setting_volume_info(device,
                                                  &volume_info,
                                                  op,
                                                  (void *) &index,
                                                  false);
}

bool AMPulseAudioDevice::set_volume_by_name(AM_AUDIO_DEVICE_TYPE device,
                                            unsigned char volume,
                                            const char *device_name)
{
  AMAudioVolumeInfo volume_info;
  AM_AUDIO_DEVICE_OPERATION op;

  if (volume > 100) {
    ERROR("Invalid argument: volume [0 - 100]: %u", volume);
    return false;
  }

  volume_info.volume = volume;
  op = AM_AUDIO_DEVICE_OPERATION_VOLUME_SET;
  return unified_entrance_for_setting_volume_info(device,
                                                  &volume_info,
                                                  op,
                                                  (void *) device_name,
                                                  true);
}

bool AMPulseAudioDevice::mute_by_index(AM_AUDIO_DEVICE_TYPE device, int index)
{
  AMAudioVolumeInfo volume_info;
  AM_AUDIO_DEVICE_OPERATION op;

  op = AM_AUDIO_DEVICE_OPERATION_VOLUME_MUTE;
  return unified_entrance_for_setting_volume_info(device,
                                                  &volume_info,
                                                  op,
                                                  (void *) &index,
                                                  false);
}

bool AMPulseAudioDevice::mute_by_name(AM_AUDIO_DEVICE_TYPE device,
                                      const char *device_name)
{
  AMAudioVolumeInfo volume_info;
  AM_AUDIO_DEVICE_OPERATION op;

  op = AM_AUDIO_DEVICE_OPERATION_VOLUME_MUTE;
  return unified_entrance_for_setting_volume_info(device,
                                                  &volume_info,
                                                  op,
                                                  (void *) device_name,
                                                  true);
}

bool AMPulseAudioDevice::unmute_by_index(AM_AUDIO_DEVICE_TYPE device, int index)
{
  AMAudioVolumeInfo volume_info;
  AM_AUDIO_DEVICE_OPERATION op;

  op = AM_AUDIO_DEVICE_OPERATION_VOLUME_UNMUTE;
  return unified_entrance_for_setting_volume_info(device,
                                                  &volume_info,
                                                  op,
                                                  (void *) &index,
                                                  false);
}

bool AMPulseAudioDevice::unmute_by_name(AM_AUDIO_DEVICE_TYPE device,
                                        const char *device_name)
{
  AMAudioVolumeInfo volume_info;
  AM_AUDIO_DEVICE_OPERATION op;

  op = AM_AUDIO_DEVICE_OPERATION_VOLUME_UNMUTE;
  return unified_entrance_for_setting_volume_info(device,
                                                  &volume_info,
                                                  op,
                                                  (void *) device_name,
                                                  true);
}

bool AMPulseAudioDevice::is_muted_by_index(AM_AUDIO_DEVICE_TYPE device,
                                           bool *mute,
                                           int index)
{
  bool ret = true;
  AMAudioVolumeInfo volume_info;

  if (!unified_entrance_for_getting_volume_info(device,
                                                &volume_info,
                                                (void *) &index,
                                                false)) {
    ERROR("Failed to get volume info of %s!", device_str[device]);
    ret = false;
  } else {
    *mute = (volume_info.mute == 1);
  }

  return ret;
}

bool AMPulseAudioDevice::is_muted_by_name(AM_AUDIO_DEVICE_TYPE device,
                                          bool *mute,
                                          const char *device_name)
{
  bool ret = true;
  AMAudioVolumeInfo volume_info;

  if (!unified_entrance_for_getting_volume_info(device,
                                                &volume_info,
                                                (void *) device_name,
                                                true)) {
    ERROR("Failed to get volume info of %s!", device_str[device]);
    ret = false;
  } else {
    *mute = (volume_info.mute == 1);
  }

  return ret;
}

bool AMPulseAudioDevice::get_sample_rate(unsigned int *sample)
{
  bool ret = true;

  if (!get_sample_spec_info()) {
    ERROR("Failed to get spec info from audio device!");
    ret = -1;
  } else {
    *sample = m_sample_spec.rate;
  }

  return ret;
}

bool AMPulseAudioDevice::get_channels(unsigned char *channels)
{
  bool ret = true;

  if (!get_sample_spec_info()) {
    ERROR("Failed to get spec info from audio device!");
    ret = -1;
  } else {
    *channels = m_sample_spec.channels;
  }

  return ret;
}

bool AMPulseAudioDevice::get_index_list(AM_AUDIO_DEVICE_TYPE device,
                                        int *list,
                                        int *len)
{
  bool ret = true;
  std::map<int, std::string>::iterator iter;
  int i = 0;

  do {
    if (!audio_device_info_list_get(device)) {
      ERROR("Failed to get audio device's info list.");
      ret = false;
      break;
    } else {
      if (!m_device_map.empty()) {
        if (m_device_map.size() > (unsigned int) *len) {
          ERROR("Length of array is not enough to store index list.");
          ret = false;
          break;
        } else {
          *len = m_device_map.size();
          iter = m_device_map.begin();
          for (i = 0; iter != m_device_map.end(); iter ++, i ++) {
            list[i] = iter->first;
            DEBUG("index = %d, name = %s\n", iter->first, iter->second.c_str());
          }
        }
      }
    }
  } while (0);

  return ret;
}

void AMPulseAudioDevice::static_pa_state(pa_context *context, void *data)
{
  ((UserData *) data)->adev->pa_state(context, ((UserData*)data)->data);
}

void AMPulseAudioDevice::pa_state(pa_context *context, void *data)
{
  DEBUG("pa_state is called!");
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)data, 0);
}

void AMPulseAudioDevice::static_get_sink_info(pa_context *c,
                                              const pa_sink_info *i,
                                              int eol,
                                              void *user_data)
{
  ((UserData*)user_data)->adev->get_sink_info(i);
  pa_threaded_mainloop_signal(((pa_threaded_mainloop*)
                              (((UserData*)user_data)->data)), 0);
  DEBUG("static_get_sink_info is called!");
}

void AMPulseAudioDevice::get_sink_info(const pa_sink_info *sink)
{
  int i;

  if (sink != nullptr) {
    m_sink_index = sink->index;
    m_sink_muted = (sink->mute > 0);

    m_sink_volume->channels = sink->volume.channels;
    for (i = 0; i < sink->volume.channels; i ++) {
      m_sink_volume->values[i] = sink->volume.values[i];
    }

    /*
     * As name, description, monitor_source_name and driver are declared as const
     * in prototype of pa_sink_info, coping operation is not allowed. So, don't
     * use these members in other place because parameter i will be destoried in
     * pulse audio library and contents of those varibles is uncertain.
     */
    DEBUG("sink->name: %s", sink->name);
    DEBUG("sink->description: %s", sink->description);
    DEBUG("sink->monitor_source_name: %s", sink->monitor_source_name);
    DEBUG("sink->driver: %s", sink->driver);
    DEBUG("sink->volume.channels: %d", sink->volume.channels);

    /* add index and name of sink into map. */
    if (m_list_flag) {
      m_device_map.insert(std::pair<int, std::string>(m_sink_index,
                                                      std::string(sink->name)));
    }
  }
}

void AMPulseAudioDevice::static_get_sink_input_info(pa_context *c,
                                                    const pa_sink_input_info *i,
                                                    int eol,
                                                    void *user_data)
{
  ((UserData *) user_data)->adev->get_sink_input_info(i);
  pa_threaded_mainloop_signal(((pa_threaded_mainloop*)
                               (((UserData*) user_data)->data)),
                              0);
  DEBUG("static_get_sink_input_info is called!");
}

void AMPulseAudioDevice::get_sink_input_info(
    const pa_sink_input_info *sink_input)
{
  int i;

  if (sink_input != nullptr) {
    m_sink_input_index = sink_input->index;
    m_sink_input_muted = (sink_input->mute > 0);

    m_sink_input_volume->channels = sink_input->volume.channels;
    for (i = 0; i < sink_input->volume.channels; i ++) {
      m_sink_input_volume->values[i] = sink_input->volume.values[i];
    }

    /*
     * As name, description, monitor_source_name and driver are declared as
     * const in prototype of pa_sink_input_info, coping operation is not allowed.
     * So, don't use these members in other place because parameter i will be
     * destroyed in pulse audio library and contents of those variables
     * are uncertain.
     */
    DEBUG("sink_input->name: %s", sink_input->name);
    DEBUG("sink_input->driver: %s", sink_input->driver);

    /* add index and name of sink input into map. */
    if (m_list_flag) {
      m_device_map.insert(std::pair<int, std::string>(
          m_sink_input_index, std::string(sink_input->name)));
    }
  }
}

void AMPulseAudioDevice::static_get_source_info(pa_context *c,
                                                const pa_source_info *i,
                                                int eol,
                                                void *user_data)
{
  ((UserData *) user_data)->adev->get_source_info(i);
  pa_threaded_mainloop_signal(((pa_threaded_mainloop*)
                               (((UserData *) user_data)->data)),
                              0);
  DEBUG("static_get_source_info is called!");
}

void AMPulseAudioDevice::get_source_info(const pa_source_info *source)
{
  int i;

  if (source != nullptr) {
    m_source_index = source->index;
    m_source_muted = (source->mute > 0);

    m_source_volume->channels = source->volume.channels;
    for (i = 0; i < source->volume.channels; i ++) {
      m_source_volume->values[i] = source->volume.values[i];
    }

    /*
     * As name, description, monitor_of_sink_name and driver are declared as const
     * in prototype of pa_sink_info, coping operation is not allowed. So, don't
     * use these members in other place because parameter i will be destoried in
     * pulse audio library and contents of those varibles is uncertain.
     */
    DEBUG("source->name: %s", source->name);
    DEBUG("source->description: %s", source->description);
    DEBUG("source->monitor_of_source_name: %s", source->monitor_of_sink_name);
    DEBUG("source->driver: %s", source->driver);

    /* add index and name of source into map. */
    if (m_list_flag) {
      m_device_map.insert(std::pair<int, std::string>(m_source_index,
                                                      std::string(source->name)));
    }
  }
}

void AMPulseAudioDevice::static_get_source_output_info(
    pa_context *c,
    const pa_source_output_info *i,
    int eol,
    void *user_data)
{
  ((UserData *) user_data)->adev->get_source_output_info(i);
  pa_threaded_mainloop_signal(((pa_threaded_mainloop*)
                               (((UserData*)user_data)->data)),
                              0);
  DEBUG("static_get_source_output_info is called!");
}

void AMPulseAudioDevice::get_source_output_info(
    const pa_source_output_info *source_output)
{
  if (source_output != nullptr) {
    m_source_output_index = source_output->index;
    m_source_output_muted = (source_output->mute > 0);

    m_source_output_volume->channels = source_output->volume.channels;
    for (int i = 0; i < source_output->volume.channels; i ++) {
      m_source_output_volume->values[i] = source_output->volume.values[i];
    }

    /*
     * As name, description, monitor_of_sink_name and driver are declared as const
     * in prototype of pa_sink_info, coping operation is not allowed. So, don't
     * use these members in other place because parameter i will be destroyed in
     * pulse audio library and contents of those variables is uncertain.
     */
    DEBUG("source_output->name: %s", source_output->name);
    DEBUG("source_output->driver: %s", source_output->driver);

    /* add index and name of source into map. */
    if (m_list_flag) {
      m_device_map.insert(std::pair<int, std::string>(
          m_source_output_index, std::string(source_output->name)));
    }
  }
}

void AMPulseAudioDevice::static_get_server_info(pa_context *c,
                                                const pa_server_info *s,
                                                void *user_data)
{
  ((UserData *) user_data)->adev->get_server_info(s);
  pa_threaded_mainloop_signal(
      (pa_threaded_mainloop*)(((UserData*)user_data)->data), 0);
  DEBUG("static_get_server_info is called!");
}

void AMPulseAudioDevice::set_operation_success_or_not(int success)
{
  if (success) {
    m_set_operation_success = true;
    DEBUG("Setting operation is executed successfully!");
  } else {
    m_set_operation_success = false;
    NOTICE("Setting operation is executed unsuccessfully!");
  }

}

void AMPulseAudioDevice::static_set_operation_success_or_not(pa_context *c,
                                                             int success,
                                                             void *user_data)
{
  ((UserData *) user_data)->adev->set_operation_success_or_not(success);
  pa_threaded_mainloop_signal(
      (pa_threaded_mainloop*) (((UserData*)user_data)->data), 0);
  DEBUG("static_set_operation_success_or_not is called");
}

void AMPulseAudioDevice::get_server_info(const pa_server_info *s)
{
  m_sample_spec.rate = s->sample_spec.rate;
  m_sample_spec.channels = s->sample_spec.channels;

  DEBUG("Sample Rate: %u", s->sample_spec.rate);
  DEBUG("   Channels: %u", s->sample_spec.channels);

}

bool AMPulseAudioDevice::unified_entrance_for_getting_volume_info(
    AM_AUDIO_DEVICE_TYPE device,
    AMAudioVolumeInfo *volume_info,
    void *index_or_name,
    bool flag)
{
  bool ret = true;
  bool device_is_specified = false;
  std::map<int, std::string>::iterator iter;
  unsigned int i = 0;
  char *name = nullptr;
  int index = 0;

  do {
    if (!volume_info) {
      ERROR("Invalid argument: volume_info is a null pointer! ");
      ret = false;
      break;
    }

    if (device < AM_AUDIO_DEVICE_MIC
        || device > AM_AUDIO_DEVICE_SOURCE_OUTPUT) {
      ERROR("No such audio device or stream!");
      ret = false;
      break;
    }

    if (!audio_device_info_list_get(device)) {
      ERROR("Failed to get info list of audio device or stream!");
      ret = false;
      break;
    }

    if (flag) {
      /* User want to fetch volume info by name. */
      name = (char *) index_or_name;
      device_is_specified = (name != nullptr);
    } else {
      /* User want to fetch volume info by index. */
      index = *((int *) index_or_name);
      device_is_specified = (index != -1);
    }

    /*
     * There are four situations need to be considered:
     *
     * 1. User doesn't pass index or name and there is just one
     *    audio device or stream with the same type. Then we
     *    fetch the volume of the default audio device or stream.
     *
     * 2. User doesn't pass index or name and there are two or more
     *    audio devices or streams with the same type. We can't
     *    handle this situation, so we notice user in the form of
     *    printing message on screen and return false.
     *
     * 3. User passes index or name and there is audio device or
     *    stream whose index or name is the same to what user passed.
     *    Then we fetch the volume of this audio device or stream
     *    and return to user.
     *
     * 4. User passes index or name and there is no audio device or
     *    stream whose index or name is the same one. We handle this
     *    situation by printing message on screen to notice user and
     *    return false.
     */
    if (!device_is_specified) {
      if (m_device_map.size() == 1) {
        iter = m_device_map.begin();
        if (!get_volume_info_by_index(device, volume_info, iter->first)) {
          ERROR("Error occurs on getting volume info of %s!",
                device_str[device]);
          ret = false;
          break;
        }
      } else {
        NOTICE("Index is not specified and there are %d %ss!",
               m_device_map.size(),
               device_str[device]);
        ret = false;
        break;
      }
    } else {
      iter = m_device_map.begin();
      for (i = 0; iter != m_device_map.end(); iter ++, i ++) {
        if (flag) {
          /* User want to fetch volume info by name. */
          if (!strcmp(name, iter->second.c_str()))
            break;
        } else {
          /* User want to fetch volume info by index. */
          if (iter->first == index)
            break;
        }
      }

      if (i == m_device_map.size()) {
        ERROR("There is no %s with index equal to %d",
              device_str[device],
              index);
        ret = false;
        break;
      } else {
        if (!get_volume_info_by_index(device, volume_info, iter->first)) {
          ERROR("Error occurs on getting volume info of %s!",
                device_str[device]);
          ret = false;
          break;
        }
      }
    }
  } while (0);

  return ret;
}

bool AMPulseAudioDevice::get_volume_info_by_index(AM_AUDIO_DEVICE_TYPE device,
                                                  AMAudioVolumeInfo *volumeInfo,
                                                  int index)
{
  bool ret = true;
  bool has_get_volume_info = true;
  pa_operation *pa_op = nullptr;
  pa_operation_state_t op_state;
  UserData userData(this, m_threaded_mainloop);

  do {
    pa_threaded_mainloop_lock(m_threaded_mainloop);
    if (device == AM_AUDIO_DEVICE_MIC) {
      pa_op = pa_context_get_source_info_by_index(m_context,
                                                  index,
                                                  static_get_source_info,
                                                  &userData);
    } else if (device == AM_AUDIO_DEVICE_SPEAKER) {
      pa_op = pa_context_get_sink_info_by_index(m_context,
                                                index,
                                                static_get_sink_info,
                                                &userData);
    } else if (device == AM_AUDIO_DEVICE_SINK_INPUT) {
      pa_op = pa_context_get_sink_input_info(m_context,
                                             index,
                                             static_get_sink_input_info,
                                             &userData);
    } else if (device == AM_AUDIO_DEVICE_SOURCE_OUTPUT) {
      pa_op = pa_context_get_source_output_info(m_context,
                                                index,
                                                static_get_source_output_info,
                                                &userData);
    } else {
      INFO("No such audio device: device = %d", (int )device);
      ret = false;
      break;
    }

    if (pa_op) {
      while ((op_state = pa_operation_get_state(pa_op)) != PA_OPERATION_DONE) {
        if (op_state == PA_OPERATION_CANCELLED) {
          INFO("Get source info operation cancelled!\n");
          has_get_volume_info = false;
          break;
        }

        pa_threaded_mainloop_wait(m_threaded_mainloop);
      }

      pa_operation_unref(pa_op);
    }
    pa_threaded_mainloop_unlock(m_threaded_mainloop);

    if (has_get_volume_info) {
      if (device == AM_AUDIO_DEVICE_MIC) {
        volumeInfo->volume = pa_cvolume_avg(m_source_volume)
            * 100/ PA_VOLUME_NORM;
        volumeInfo->mute = (m_source_muted) ? 1 : 0;
      } else if (device == AM_AUDIO_DEVICE_SPEAKER) {
        volumeInfo->volume = pa_cvolume_avg(m_sink_volume)
            * 100/ PA_VOLUME_NORM;
        volumeInfo->mute = (m_sink_muted) ? 1 : 0;
      } else if (device == AM_AUDIO_DEVICE_SINK_INPUT) {
        volumeInfo->volume = pa_cvolume_avg(m_sink_input_volume)
            * 100/ PA_VOLUME_NORM;
        volumeInfo->mute = (m_sink_input_muted) ? 1 : 0;
      } else if (device == AM_AUDIO_DEVICE_SOURCE_OUTPUT) {
        volumeInfo->volume = pa_cvolume_avg(m_source_output_volume)
            * 100/ PA_VOLUME_NORM;
        volumeInfo->mute = (m_source_output_muted) ? 1 : 0;
      } else {
        INFO("No such audio device: device = %d", (int )device);
        ret = false;
        break;
      }
    } else {
      INFO("Failed to get volume info!\n");
      ret = false;
    }
  } while (0);

  return ret;
}

bool AMPulseAudioDevice::unified_entrance_for_setting_volume_info(
    AM_AUDIO_DEVICE_TYPE device,
    AMAudioVolumeInfo *volume_info,
    AM_AUDIO_DEVICE_OPERATION op,
    void *index_or_name,
    bool flag)
{
  bool ret = true;
  bool device_is_specified = false;
  std::map<int, std::string>::iterator iter;
  unsigned int i = 0;
  char *name = nullptr;
  int index = 0;

  do {
    if (!volume_info) {
      ERROR("Invalid argument: volume_info is a null pointer! ");
      ret = false;
      break;
    }

    if (device < AM_AUDIO_DEVICE_MIC
        || device > AM_AUDIO_DEVICE_SOURCE_OUTPUT) {
      ERROR("No such audio device or stream!");
      ret = false;
      break;
    }

    if (!audio_device_info_list_get(device)) {
      ERROR("Failed to get info list of audio device or stream!");
      ret = false;
      break;
    }

    if (flag) {
      /* User want to fetch volume info by name. */
      name = (char *) index_or_name;
      device_is_specified = (name != nullptr);
    } else {
      /* User want to fetch volume info by index. */
      index = *((int *) index_or_name);
      device_is_specified = (index != -1);
    }

    /*
     * There are four situations need to be considered:
     *
     * 1. User doesn't pass index and there is just one audio device
     *    or stream with the same type. Then we set the volume passed
     *    from the upper level to the default audio device or stream.
     *
     * 2. User doesn't pass index and there are two or more audio
     *    devices or streams with the same type. We can't handle
     *    this situation, so we notice user in the form of printing
     *    message on screen and return false.
     *
     * 3. User pass index and audio device or stream whose index is
     *    the same one.  Then we set the volume passed from the upper
     *    level to the default audio device or stream.
     *
     * 4. User pass index and there is no audio device or stream whose
     *    index is not the same one. We handle this situation by
     *    printing message on screen to notice user and return false.
     */
    if (!device_is_specified) {
      if (m_device_map.size() == 1) {
        iter = m_device_map.begin();
        if (!set_volume_info_by_index(device, volume_info, op, iter->first)) {
          ERROR("Error occurs on getting volume info of %s!",
                device_str[device]);
          ret = false;
          break;
        }
      } else {
        NOTICE("Index is not specified and there are %d %ss!",
               m_device_map.size(),
               device_str[device]);
        ret = false;
        break;
      }
    } else {
      iter = m_device_map.begin();
      for (i = 0; iter != m_device_map.end(); iter ++, i ++) {
        if (flag) {
          /* User want to fetch volume info by name. */
          if (!strcmp(name, iter->second.c_str()))
            break;
        } else {
          /* User want to fetch volume info by index. */
          if (iter->first == index)
            break;
        }
      }

      if (i == m_device_map.size()) {
        ERROR("There is no %s with index equal to %d",
              device_str[device],
              index);
        ret = false;
        break;
      } else {
        if (!set_volume_info_by_index(device, volume_info, op, iter->first)) {
          ERROR("Error occurs on getting volume info of %s!",
                device_str[device]);
          ret = false;
          break;
        }
      }
    }
  } while (0);

  return ret;
}

bool AMPulseAudioDevice::set_volume_info_by_index(
    AM_AUDIO_DEVICE_TYPE device,
    AMAudioVolumeInfo *volume_info,
    AM_AUDIO_DEVICE_OPERATION op,
    int index)
{
  int i = 0;
  bool ret = true;
  AMAudioVolumeInfo temp;
  pa_operation *pa_op = nullptr;
  pa_operation_state_t op_state;
  UserData userData(this, m_threaded_mainloop);

  do {
    /*
     * Firstly, we get channels' info of corresponding
     * audio device or stream.
     */
    if (!get_volume_info_by_index(device, &temp, index)) {
      ERROR("Failed to fetch channels' info!\n");
      ret = false;
      break;
    }

    pa_threaded_mainloop_lock(m_threaded_mainloop);
    switch (op) {
      case AM_AUDIO_DEVICE_OPERATION_VOLUME_SET: {
        if (device == AM_AUDIO_DEVICE_MIC) {
          for (i = 0; i < m_source_volume->channels; i ++) {
            m_source_volume->values[i] = volume_info->volume * PA_VOLUME_NORM
                / 100;
          }

          pa_op = pa_context_set_source_volume_by_index(
              m_context,
              m_source_index,
              m_source_volume,
              static_set_operation_success_or_not,
              &userData);
        } else if (device == AM_AUDIO_DEVICE_SPEAKER) {
          for (i = 0; i < m_sink_volume->channels; i ++) {
            m_sink_volume->values[i] = volume_info->volume * PA_VOLUME_NORM
                / 100;
          }

          pa_op = pa_context_set_sink_volume_by_index(
              m_context,
              m_sink_index,
              m_sink_volume,
              static_set_operation_success_or_not,
              &userData);
        } else if (device == AM_AUDIO_DEVICE_SINK_INPUT) {
          for (i = 0; i < m_sink_input_volume->channels; i ++) {
            m_sink_input_volume->values[i] = volume_info->volume
                * PA_VOLUME_NORM / 100;
          }

          pa_op = pa_context_set_sink_input_volume(
              m_context,
              m_sink_input_index,
              m_sink_input_volume,
              static_set_operation_success_or_not,
              &userData);
        } else if (device == AM_AUDIO_DEVICE_SOURCE_OUTPUT) {
          for (i = 0; i < m_source_output_volume->channels; i ++) {
            m_source_output_volume->values[i] = volume_info->volume
                * PA_VOLUME_NORM / 100;
          }

          pa_op = pa_context_set_source_output_volume(
              m_context,
              m_source_output_index,
              m_source_output_volume,
              static_set_operation_success_or_not,
              &userData);
        } else {
          INFO("No such audio device: device = %d", (int )device);
          return -1;
        }
      }
        break;

      case AM_AUDIO_DEVICE_OPERATION_VOLUME_MUTE: {
        if (device == AM_AUDIO_DEVICE_MIC) {
          pa_op = pa_context_set_source_mute_by_index(
              m_context,
              m_source_index,
              1,
              static_set_operation_success_or_not,
              &userData);
        } else if (device == AM_AUDIO_DEVICE_SPEAKER) {
          pa_op = pa_context_set_sink_mute_by_index(
              m_context,
              m_sink_index,
              1,
              static_set_operation_success_or_not,
              &userData);
        } else if (device == AM_AUDIO_DEVICE_SINK_INPUT) {
          pa_op = pa_context_set_sink_input_mute(
              m_context,
              m_sink_input_index,
              1,
              static_set_operation_success_or_not,
              &userData);
        } else if (device == AM_AUDIO_DEVICE_SOURCE_OUTPUT) {
          pa_op = pa_context_set_source_output_mute(
              m_context,
              m_source_output_index,
              1,
              static_set_operation_success_or_not,
              &userData);
        } else {
          INFO("No such audio device: device = %d", (int )device);
          ret = false;
          break;
        }
      }
        break;

      case AM_AUDIO_DEVICE_OPERATION_VOLUME_UNMUTE: {
        if (device == AM_AUDIO_DEVICE_MIC) {
          pa_op = pa_context_set_source_mute_by_index(
              m_context,
              m_source_index,
              0,
              static_set_operation_success_or_not,
              &userData);
        } else if (device == AM_AUDIO_DEVICE_SPEAKER) {
          pa_op = pa_context_set_sink_mute_by_index(
              m_context,
              m_sink_index,
              0,
              static_set_operation_success_or_not,
              &userData);
        } else if (device == AM_AUDIO_DEVICE_SINK_INPUT) {
          pa_op = pa_context_set_sink_input_mute(
              m_context,
              m_sink_input_index,
              0,
              static_set_operation_success_or_not,
              &userData);
        } else if (device == AM_AUDIO_DEVICE_SOURCE_OUTPUT) {
          pa_op = pa_context_set_source_output_mute(
              m_context,
              m_source_output_index,
              0,
              static_set_operation_success_or_not,
              &userData);
        } else {
          INFO("No such audio device: device = %d", (int )device);
          ret = false;
          break;
        }
      }
        break;

      default: {
        ERROR("No such operation in set_audio_volume_info!");
        ret = false;
        break;
      }
        break;

    }

    if (pa_op) {
      while ((op_state = pa_operation_get_state(pa_op)) != PA_OPERATION_DONE) {
        if (op_state == PA_OPERATION_CANCELLED) {
          INFO("Get source info operation cancelled!\n");
          break;
        }
        pa_threaded_mainloop_wait(m_threaded_mainloop);
      }
      pa_operation_unref(pa_op);
    }
    pa_threaded_mainloop_unlock(m_threaded_mainloop);

    ret = m_set_operation_success;
  } while (0);

  return ret;
}

bool AMPulseAudioDevice::get_sample_spec_info()
{
  bool ret = true;
  pa_operation *pa_op = nullptr;
  UserData userData(this, m_threaded_mainloop);
  pa_operation_state_t op_state;

  pa_threaded_mainloop_lock(m_threaded_mainloop);
  pa_op = pa_context_get_server_info(m_context,
                                     static_get_server_info,
                                     &userData);

  if (pa_op) {
    while ((op_state = pa_operation_get_state(pa_op)) != PA_OPERATION_DONE) {
      if (op_state == PA_OPERATION_CANCELLED) {
        INFO("Get server info operation cancelled!\n");
        ret = false;
        break;
      }

      pa_threaded_mainloop_wait(m_threaded_mainloop);
    }

    pa_operation_unref(pa_op);
  }

  pa_threaded_mainloop_unlock(m_threaded_mainloop);
  return ret;
}

bool AMPulseAudioDevice::get_name_by_index(AM_AUDIO_DEVICE_TYPE device,
                                           int index,
                                           char *name,
                                           int len)
{
  bool ret = true;
  std::map<int, std::string>::iterator iter;
  unsigned int i = 0;

  do {
    if (!audio_device_info_list_get(device)) {
      ERROR("Failed to get audio device's info list.");
      ret = false;
      break;
    } else {
      if (!m_device_map.empty()) {
        iter = m_device_map.begin();
        for (i = 0; iter != m_device_map.end(); iter ++, i ++) {
          if (iter->first == index)
            break;
        }

        if (i == m_device_map.size()) {
          NOTICE("Audio device or stream with index = %d is not found!", index);
          ret = false;
        } else {
          if (iter->second.length() > (unsigned int) (len - 1)) {
            NOTICE("Space of array used to store name of device or stream"
                   " is not enough!");
            ret = false;
            break;
          } else {
            strcpy(name, iter->second.c_str());
          }
        }
      }
    }
  } while (0);

  return ret;
}

bool AMPulseAudioDevice::audio_device_info_list_get(AM_AUDIO_DEVICE_TYPE device)
{
  bool ret = true;
  pa_operation *pa_op = nullptr;
  UserData userData(this, m_threaded_mainloop);
  pa_operation_state_t op_state;

  do {
    pa_threaded_mainloop_lock(m_threaded_mainloop);
    m_list_flag = true;
    m_device_map.clear();

    if (AM_AUDIO_DEVICE_MIC == device) {
      pa_op = pa_context_get_source_info_list(m_context,
                                              static_get_source_info,
                                              &userData);
    } else if (AM_AUDIO_DEVICE_SPEAKER == device) {
      pa_op = pa_context_get_sink_info_list(m_context,
                                            static_get_sink_info,
                                            &userData);
    } else if (AM_AUDIO_DEVICE_SINK_INPUT == device) {
      pa_op = pa_context_get_sink_input_info_list(m_context,
                                                  static_get_sink_input_info,
                                                  &userData);
    } else if (AM_AUDIO_DEVICE_SOURCE_OUTPUT == device) {
      pa_op =
          pa_context_get_source_output_info_list(m_context,
                                                 static_get_source_output_info,
                                                 &userData);
    } else {
      ERROR("No such audio device or stream: %d\n", (int )device);
      ret = false;
      break;
    }

    if (pa_op) {
      while ((op_state = pa_operation_get_state(pa_op)) != PA_OPERATION_DONE) {
        if (op_state == PA_OPERATION_CANCELLED) {
          INFO("Get server info operation cancelled!\n");
          ret = false;
          break;
        }

        pa_threaded_mainloop_wait(m_threaded_mainloop);
      }

      pa_operation_unref(pa_op);
    }
    pa_threaded_mainloop_unlock(m_threaded_mainloop);
  } while (0);

  if (m_device_map.size() == 0) {
    NOTICE("There is no %s on board", device_str[device]);
    ret = false;
  }

  m_list_flag = false;
  return ret;
}
