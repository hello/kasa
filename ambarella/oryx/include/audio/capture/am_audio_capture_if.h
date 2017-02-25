/*******************************************************************************
 * am_audio_capture_if.h
 *
 * History:
 *   2014-11-28 - [ypchang] created file
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
 ******************************************************************************/
/*! @file am_audio_capture_if.h
 *  @brief This file defines AMIAudioCapture.
 */
#ifndef AM_AUDIO_CAPTURE_IF_H_
#define AM_AUDIO_CAPTURE_IF_H_

#include "am_audio_define.h"

/*! @struct AudioPacket
 *  Defines data packet
 */
struct AudioPacket
{
    int64_t  pts;    //!< Audio data PTS
    uint8_t *data;   //!< Audio data address
    size_t   length; //!< Audio data length
};

/*! @struct AudioCapture
 *  Used in AudioCaptureCallback
 */
struct AudioCapture
{
    void        *owner;  //!< Owner of the AMIAudioCapture class
    AudioPacket  packet; //!< Audio packet info see @ref AudioPacket
};

/*! @typedef AudioCaptureCallback
 *  Callback function of AMIAudioCapture. This callback function is called
 *  every time the audio data is available for reading.
 *  @param data AudioCapture pointer
 */
typedef void (*AudioCaptureCallback)(AudioCapture *data);

/*! @class AMIAudioCapture
 *  Defines interface for reading data
 */
class AMIAudioCapture
{
  public:
    /*!
     * Destructor. Destroy AMAudioCapture object.
     */
    virtual void destroy()                                           = 0;

    /*! Enable or disable echo cancellation function
     * @param enabled
     *  - true: enable audio cancellation
     *  - false: disable audio cancellation
     */
    virtual void set_echo_cancel_enabled(bool enabled)               = 0;

    /*!
     *  Start audio capture
     * @param volume audio initial volume, -1 means device default value
     * @return true if success, otherwise false
     */
    virtual bool start(int32_t volume = -1)                          = 0;

    /*!
     * Stop audio capture
     * @return true if success, otherwise false
     */
    virtual bool stop()                                              = 0;

    /*!
     * Set audio capture's channel number, must be called before start()
     * @param channel
     * - 1: Mono
     * - 2: Stereo
     * @return true if success, otherwise false
     */
    virtual bool set_channel(uint32_t channel)                       = 0;

    /*!
     * Set audio capture's sample rate, must be called before start()
     * @param sample_rate audio sample rate
     * @return true if success, otherwise false
     */
    virtual bool set_sample_rate(uint32_t sample_rate)               = 0;

    /*!
     * Set audio capture's chunk bytes, must be called before start().
     * @param chunk_bytes chunk size in bytes
     * @return true if success, otherwise false
     */
    virtual bool set_chunk_bytes(uint32_t chunk_bytes)               = 0;

    /*!
     * Set audio sample format
     * @param format see @ref AM_AUDIO_SAMPLE_FORMAT
     * @return true if success, otherwise false
     */
    virtual bool set_sample_format(AM_AUDIO_SAMPLE_FORMAT format)    = 0;


    /*!
     * set stream volume
     * @param volume recording stream volume
     * @return true if success, otherwise false
     */
    virtual bool set_volume(uint32_t volume)                         = 0;

    /*!
     * Get current audio capture's channel number
     * @return channel number
     */
    virtual uint32_t get_channel()                                   = 0;

    /*!
     * Get current audio capture's sample rate
     * @return audio sample rate
     */
    virtual uint32_t get_sample_rate()                               = 0;

    /*!
     * Get current audio capture's chunk bytes
     * @return chunk bytes
     */
    virtual uint32_t get_chunk_bytes()                               = 0;

    /*!
     * Get current audio capture's sample size
     * @return audio sample size
     */
    virtual uint32_t get_sample_size()                               = 0;

    /*!
     * Get audio data chunk's PTS duration in 90k
     * @return audio data chunk's PTS duration in 90k
     */
    virtual int64_t  get_chunk_pts()                                 = 0;

    /*!
     * Get current audio capture's sample format
     * @return @ref AM_AUDIO_SAMPLE_FORMAT
     */
    virtual AM_AUDIO_SAMPLE_FORMAT get_sample_format()               = 0;

    /*!
     * Set audio capture's callback function
     * @param callback @ref AudioCaptureCallback
     * @return true if success, otherwise false
     */
    virtual bool set_capture_callback(AudioCaptureCallback callback) = 0;

    /*!
     * Virtual destructor function
     */
    virtual ~AMIAudioCapture(){}
};

/*! Creator
 * @param interface
 * - pulse: use pulseaudio interface
 * - alsa:  use alsa interface(Not implemented)
 * @param name name of this capture
 * @param owner  owner class of this audio capture
 * @param callback audio capture callback function
 */
AMIAudioCapture* create_audio_capture(const std::string& interface,
                                      const std::string& name,
                                      void *owner,
                                      AudioCaptureCallback callback);
#endif /* AM_AUDIO_CAPTURE_IF_H_ */
