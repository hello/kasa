/*
 * am_audio_device.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 12/12/2014 [Created]
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
#ifndef __AM_AUDIO_DEVICE_IF_H__
#define __AM_AUDIO_DEVICE_IF_H__

/*! @file am_audio_device_if.h
 *  @brief This file defines audio device interface to control audio volume
 */

#include "am_pointer.h"

/*! @enum AM_AUDIO_DEVICE_TYPE
 *  @brief defines audio device type
 */
enum AM_AUDIO_DEVICE_TYPE
{
   AM_AUDIO_DEVICE_MIC = 0,      //!< Audio input device MIC
   AM_AUDIO_DEVICE_SPEAKER,      //!< Audio output device speaker
   AM_AUDIO_DEVICE_SINK_INPUT,   //!< Audio output source
   AM_AUDIO_DEVICE_SOURCE_OUTPUT,//!< Audio input source
};

class AMIAudioDevice;
/*! @typedef AMIAudioDevicePtr
 *  @brief Smart pointer type to handle AMIAudioDevice pointer.
 */
typedef AMPointer<AMIAudioDevice> AMIAudioDevicePtr;

/*! @class AMIAudioDevice
 *  @brief AMIAudioDevice is the interface class that used to control audio
 *         device parameters.
 *         This class supports to get audio volume information of the raw device
 *         and every audio stream.
 */
class AMIAudioDevice
{
    friend AMIAudioDevicePtr;

  public:
    /*! Constructor function
     * @param interface: a string (alsa or pulse) to indicate the underlying
     *                  audio interface
     * @return AMIAudioDevicePtr
     * @sa AMIAudioDevicePtr
     */
    static AMIAudioDevicePtr create(const std::string& interface);

  public:
    /*! This api will try to fetch index list of audio device or
     *  stream list from pulseaudio library and Result will be
     *  stored into list. If the amount of device or stream is
     *  greater than len, false will be returned.
     *
     *  As the real amount of audio device or stream are normally
     *  not equal to what passed from upper level, this API needs
     *  to update it as the real amount.
     *
     * @param type specify audio device or stream's type.
     * @param list address of index array.
     * @param len  address of index array's length.
     *
     * @return true on success, false on failure.
     */
    virtual bool get_index_list(AM_AUDIO_DEVICE_TYPE device,
                                int *list,
                                int *len) = 0;

    /*! This api will try to get the name of audio device or
     *  stream. If the actual name's length is greater than
     *  the expected one, false will be returned.
     *
     * @param type  specify audio device or stream's type.
     * @param index specify audio device or stream's index.
     * @param name  used to store deive or stream's name.
     * @param len   name's length.
     *
     * @return true on success, false on failure.
     */
    virtual bool get_name_by_index(AM_AUDIO_DEVICE_TYPE device,
                                   int index,
                                   char *name,
                                   int len) = 0;

    /*! Each audio device or stream has its own index and name to
     *  identify itself. This api will try to get volume info from
     *  audio device or audio just by device's index.
     *
     * @param device used to identify device's type.
     * @param volume used to store result if device's volume can be get.
     * @param index  device's index, default value is -1.
     *
     * @return true on success, false on failure.
     */
    virtual bool get_volume_by_index(AM_AUDIO_DEVICE_TYPE device,
                                     unsigned char *volume,
                                     int index = -1) = 0;
    /*! Like audio_device_volume_get_by_index, this api will try to
     *  get volume from audio device or audio just by device's name.
     *
     * @param device used to identify device's type.
     * @param volume used to store result if device's volume can be get.
     * @param name   device's name, default value is NULL.
     *
     * @return true on success, false on failure.
     */
    virtual bool get_volume_by_name(AM_AUDIO_DEVICE_TYPE device,
                                    unsigned char *volume,
                                    const char *name = NULL) = 0;
    /*! Each audio device or stream has its own index and name to
     *  identify itself. This api will try to set volume expected
     *  by user for certain audio device or stream by index.
     *
     * @param device used to identify device's type.
     * @param volume expected volume passed from upper level.
     * @param index  device's index, default value is -1.
     *
     * @return true on success, false on failure.
     */
    virtual bool set_volume_by_index(AM_AUDIO_DEVICE_TYPE device,
                                     unsigned char volume,
                                     int index = -1) = 0;
    /*! Like audio_device_volume_set_by_index, this api will try
     *  to set volume expected by user for certain audio device
     *  or stream by name.
     *
     * @param device used to identify device's type.
     * @param volume expected volume passed from upper level.
     * @param name  device's name, default value is NULL.
     *
     * @return true on success, false on failure.
     */
    virtual bool set_volume_by_name(AM_AUDIO_DEVICE_TYPE device,
                                    unsigned char volume,
                                    const char *name = NULL) = 0;
    /*! Each audio device or stream has its own index and name to
     *  identify itself. This api will try to mute certain audio
     *  device or stream by index.
     *
     * @param device used to identify device's type.
     * @param index  device's index, default value is -1.
     *
     * @return true on success, false on failure.
     */
    virtual bool mute_by_index(AM_AUDIO_DEVICE_TYPE device, int index = -1) = 0;

    /*! Like audio_device_volume_mute_by_index, this api will try
     *  to mute certain audio device or stream by index.
     *
     * @param device used to identify device's type.
     * @param index  device's name, default value is NULL.
     *
     * @return true on success, false on failure.
     */
    virtual bool mute_by_name(AM_AUDIO_DEVICE_TYPE device, const char *name =
                                  NULL) = 0;

    /*! Each audio device or stream has its own index and name to
     *  identify itself. This api will try to recover volume of
     *  certain audio device or stream by index.
     *
     * @param device used to identify device's type.
     * @param index  device's index, default value is 0.
     *
     * @return true on success, false on failure.
     */
    virtual bool unmute_by_index(AM_AUDIO_DEVICE_TYPE device,
                                 int index = -1) = 0;
    /*! Like audio_device_volume_unmute_by_name, this api will try to
     *  recover volume of certain audio device or stream by name.
     *
     * @param device used to identify device's type.
     * @param index  device's name, default value is NULL.
     *
     * @return true on success, false on failure.
     */
    virtual bool unmute_by_name(AM_AUDIO_DEVICE_TYPE device,
                                const char *name = NULL) = 0;
    /*! Each audio device or stream has its own index and name to
     *  identify itself. This api try to know certain audio device
     *  or stream identified by index is mute or not.
     *
     * @param device used to identify device's type.
     * @param volume used to store result if mute's status can be get.
     * @param index  device's index, default value is -1.
     *
     * @return true on success, false on failure.
     */
    virtual bool is_muted_by_index(AM_AUDIO_DEVICE_TYPE device,
                                   bool *mute,
                                   int index = -1) = 0;
    /*! Like audio_device_is_mute_by_index, This api try to know
     *  certain audio device or stream identified by index is mute
     *  or not.
     *
     * @param device used to identify device's type.
     * @param volume used to store result if mute's status can be get.
     * @param name  device's name, default value is 0.
     *
     * @return true on success, false on failure.
     */
    virtual bool is_muted_by_name(AM_AUDIO_DEVICE_TYPE device,
                                  bool *mute,
                                  const char *name = NULL) = 0;

    /*! This api will try to fetch sample rate of pulseaudio server.
     *
     * @param sample used to store result if sample rate can be get.
     * @result true on success, false on failure.
     */
    virtual bool get_sample_rate(unsigned int *sample) = 0;

    /*! This api will try to fetch channels of pulseaudio server.
     *
     * @param sample used to store result if channels can be get.
     * @result true on success, false on failure.
     */
    virtual bool get_channels(unsigned char *channels) = 0;

  protected:
    virtual void release() = 0;
    virtual void inc_ref() = 0;
    virtual ~AMIAudioDevice()
    {
    }
};

#ifdef __cplusplus
extern "C" {
#endif

/*! @fn create_audio_device
 * This interface is used to create an instance of audio device.
 * User need to specify which library is adapted to provide service.
 *
 * @param interface_name: audio library's name.
 * @return AMIAudioDevicePtr.
 */
AMIAudioDevicePtr create_audio_device (const char *interface_name);

#ifdef __cplusplus
}
#endif

#endif
