/*
 * am_audio_device_pulse.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 25/06/2013 [Created]
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
#ifndef __AM_AUDIO_DEVICE_PULSE_H__
#define __AM_AUDIO_DEVICE_PULSE_H__

#include "am_audio_device_if.h"
#include <pulse/pulseaudio.h>
#include <pthread.h>
#include <mutex>
#include <atomic>
#include <map>

enum AM_AUDIO_DEVICE_OPERATION
{
  AM_AUDIO_DEVICE_OPERATION_NONE,
  AM_AUDIO_DEVICE_OPERATION_VOLUME_GET,
  AM_AUDIO_DEVICE_OPERATION_VOLUME_SET,
  AM_AUDIO_DEVICE_OPERATION_VOLUME_MUTE,
  AM_AUDIO_DEVICE_OPERATION_VOLUME_UNMUTE,
};

struct AMAudioVolumeInfo;

class AMPulseAudioDevice: public AMIAudioDevice
{
  public:
    /*
     * get_instance
     *
     * @Desc: This api is a static method, used to create an instance of
     *        AMPulseAudioDevice. As it uses singleton pattern, there is just
     *        only one instance of AMPulseAudioDevice in any time.
     *
     *        We use an atomic reference count to record how many times
     *        this api is called. User should call release to subtract
     *        reference count. If not, memory leak could be happen
     *        because audio_device_destroy only delete instance when
     *        reference count is equal to zero.
     *
     * @result: address of an instance of AMPulseAudioDevice, NULL on failure.
     */
    static AMIAudioDevice *get_instance();

    virtual bool get_index_list(AM_AUDIO_DEVICE_TYPE device,
                                int *list,
                                int *len);

    virtual bool get_name_by_index(AM_AUDIO_DEVICE_TYPE device,
                                   int index,
                                   char *name,
                                   int len);

    virtual bool get_volume_by_index(AM_AUDIO_DEVICE_TYPE device,
                                     unsigned char *volume,
                                     int index = -1);

    virtual bool get_volume_by_name(AM_AUDIO_DEVICE_TYPE device,
                                    unsigned char *volume,
                                    const char *name = nullptr);

    virtual bool set_volume_by_index(AM_AUDIO_DEVICE_TYPE device,
                                     unsigned char volume,
                                     int index = -1);

    virtual bool set_volume_by_name(AM_AUDIO_DEVICE_TYPE device,
                                    unsigned char volume,
                                    const char *name = nullptr);

    virtual bool mute_by_index(AM_AUDIO_DEVICE_TYPE device, int index = -1);
    virtual bool mute_by_name(AM_AUDIO_DEVICE_TYPE device, const char *name =
                                  nullptr);

    virtual bool unmute_by_index(AM_AUDIO_DEVICE_TYPE device, int index = -1);
    virtual bool unmute_by_name(AM_AUDIO_DEVICE_TYPE device, const char *name =
                                    nullptr);

    virtual bool is_muted_by_index(AM_AUDIO_DEVICE_TYPE device,
    bool *mute,
                                   int index = -1);
    virtual bool is_muted_by_name(AM_AUDIO_DEVICE_TYPE device,
    bool *mute,
                                  const char *name = nullptr);

    virtual bool get_sample_rate(unsigned int *sample);
    virtual bool get_channels(unsigned char *channels);

  protected:

    virtual void release();
    virtual void inc_ref();

  private:
    /*
     * AMPulseAudioDevice
     *
     * @Desc: Default Constructor, used in get_instance.
     */
    AMPulseAudioDevice();

    /*
     * ~AMPulseAudioDevice
     *
     * @Desc: Destructor
     */
    virtual ~AMPulseAudioDevice();

    /*
     * audio_device_init
     *
     * @Desc: This subroutine is used to initialize pulseaudio library.
     *
     * @Return: true on success, false on failure.
     */
    bool audio_device_init();

    /*
     * get_sample_spec_info
     *
     * @Desc: This subroutine will try to get pulseaudio server's sample
     *        rate and channels info. Actually, these information are
     *        contained in pa_server_info and it needs to call certain
     *        pulseaudio APIs to fetch pulseaudio server information.
     *
     * @Return: true on success, false on failure.
     */
    bool get_sample_spec_info();

    /*
     * unified_entrance_for_getting_volume_info
     *
     * @Desc: This subroutine provide real service for two APIs:
     *
     *        1. audio_device_volume_get_by_index
     *        2. audio_device_volume_get_by_name
     *
     * @Param device       : specify audio device or stream.
     * @Param volume       : used to store result device's volume.
     * @Param index_or_name: address of index or name.
     * @Param flag         : indicate index_or_name is index or name.
     *
     * @Return: true on success, false on failure.
     */
    bool unified_entrance_for_getting_volume_info(AM_AUDIO_DEVICE_TYPE device,
                                                  AMAudioVolumeInfo *volume,
                                                  void *index_or_name,
                                                  bool flag);
    /*
     * unified_entrance_for_setting_volume_info
     *
     * @Desc: This subroutine provide real service for two APIs:
     *
     *        1. audio_device_volume_set_by_index
     *        2. audio_device_volume_set_by_name
     *
     * @Param device       : specify audio device or stream.
     * @Param volumeInfo   : device's volume needs to be set.
     * @Param op           : audio device operation type.
     * @Param index_or_name: address of index or name.
     * @Param flag         : indicate index_or_name is index or name.
     *
     * @Return: true on success, false on failure.
     */
    bool unified_entrance_for_setting_volume_info(AM_AUDIO_DEVICE_TYPE device,
                                                  AMAudioVolumeInfo *volumeInfo,
                                                  AM_AUDIO_DEVICE_OPERATION op,
                                                  void *index_or_name,
                                                  bool flag);

    /*
     * get_volume_info_by_index
     *
     * @Desc: This subroutine provides real service of getting volume
     *        of certain audio device or stream.
     *
     * @Param device    : specify audio device or stream.
     * @Param volumeInfo: store result if volume info is fetched.
     * @Param index     : audio device or stream's index.
     *
     * @Return: true on success, false on failure.
     */
    bool get_volume_info_by_index(AM_AUDIO_DEVICE_TYPE device,
                                  AMAudioVolumeInfo *volumeInfo,
                                  int index);
    /*
     * audio_device_volume_info_set_by_index
     *
     * @Desc: This subroutine provides real service of setting volume
     *        of certain audio device or stream.
     *
     * @Param device    : specify audio device or stream.
     * @Param volumeInfo: expected volume info passed from upper level.
     * @Param op        : audio device operation type.
     * @Param index     : audio device or stream's index.
     *
     * @Return: true on success, false on failure.
     */
    bool set_volume_info_by_index(AM_AUDIO_DEVICE_TYPE device,
                                  AMAudioVolumeInfo *volumeInfo,
                                  AM_AUDIO_DEVICE_OPERATION op,
                                  int index);

    /*
     * audio_device_info_list_get
     *
     * @Desc: This subroutine provides real service of getting index
     *        and name of audio device or stream.
     *
     * @Param device: specify audio device or stream's type.
     *
     * @Return: true on success, false on failure.
     * */
    bool audio_device_info_list_get(AM_AUDIO_DEVICE_TYPE device);

    /*
     * Following methods will be called in corresponding callbacks which
     * registered into pulseaudio APIs. When pulseaudio APIs has been
     * called completely, callback will be called to let users fetch what
     * they are interested in.
     */
    void pa_state(pa_context *context, void *data);
    void get_server_info(const pa_server_info *info);
    void get_source_info(const pa_source_info *source);
    void get_source_output_info(const pa_source_output_info *source);
    void get_sink_info(const pa_sink_info *sink);
    void get_sink_input_info(const pa_sink_input_info *sink);
    void set_operation_success_or_not(int success);

  private:
    /*
     * Pulseaudio library provides callback way to use its APIs.
     *
     * It allows user to prepare callback and register it to certain one
     * pulseaudio api. When this api is called successfully, callback
     * will be called automatically. So, user can put some operations
     * in callback, such as fetching audio data.
     *
     * In audio device module, we define a simple structure which contains
     * the address of an instance of AMPulseAudioDevice and pass it's address
     * with the time of passing callback to pulseaudio APIs.
     *
     * When callback is called, the address of an instance of the simple
     * structure performs as parameter to be passed to callback.
     *
     * static_pa_state: used to fetch pulseaudio's status
     *
     * static_get_sink_info: used to get speaker's info
     *
     * static_get_sink_input_info: used to fetch recording stream's info
     *
     * static_get_source_output_info: used to fetch playback stream's info
     *
     * static_get_source_info: used to get mic's info
     *
     * static_get_server_info: used to fetch pulseaudio server's info
     */
    static void static_pa_state(pa_context *context, void *data);
    static void static_get_source_info(pa_context *c,
                                       const pa_source_info *i,
                                       int eol,
                                       void *user_data);
    static void static_get_sink_info(pa_context *c,
                                     const pa_sink_info *i,
                                     int eol,
                                     void *user_data);
    static void static_get_sink_input_info(pa_context *c,
                                           const pa_sink_input_info *i,
                                           int eol,
                                           void *user_data);
    static void static_get_source_output_info(pa_context *c,
                                              const pa_source_output_info *i,
                                              int eol,
                                              void *user_data);
    static void static_get_server_info(pa_context *c,
                                       const pa_server_info *info,
                                       void *user_data);
    static void static_set_operation_success_or_not(pa_context *c,
                                                    int success,
                                                    void *user_data);

  private:
    pa_cvolume                *m_sink_volume;
    pa_cvolume                *m_sink_input_volume;
    pa_cvolume                *m_source_volume;
    pa_cvolume                *m_source_output_volume;
    pa_context                *m_context;
    pa_threaded_mainloop      *m_threaded_mainloop;
    int                        m_sink_index;
    int                        m_sink_input_index;
    int                        m_source_index;
    int                        m_source_output_index;
    pa_context_state           m_context_state;
    bool                       m_ctx_connected;
    bool                       m_sink_muted;
    bool                       m_sink_input_muted;
    bool                       m_source_muted;
    bool                       m_source_output_muted;
    bool                       m_list_flag;
    bool                       m_set_operation_success;
    pa_sample_spec             m_sample_spec;
    std::map<int, std::string> m_device_map;
    std::atomic_int            m_reference_count;

  private:
    static AMPulseAudioDevice *m_instance;
    static std::mutex m_mutex;
};

#endif
