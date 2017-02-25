/*******************************************************************************
 * am_audio_player_if.h
 *
 * History:
 *   Jul 8, 2016 - [ypchang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
#ifndef AM_AUDIO_PLAYER_IF_H_
#define AM_AUDIO_PLAYER_IF_H_
#include "am_audio_define.h"

/*! @struct AudioData
 *  Defines audio data which will be played
 */
struct AudioData
{
    //! (Input) Audio data pointer where to write data
    uint8_t *data         = nullptr;
    //! (Input) Audio data size that player wants
    uint32_t need_size    = 0;
    //! (Output) Audio data actual written
    uint32_t written_size = 0;
    //! (Output) Is player should be drained
    bool     drain        = false;
};

struct AudioPlayer
{
    void      *owner = nullptr; //!< Owner of the AMIAudioPlayer class
     AudioData data;            //!< Audio data info see @ref AudioData
};

/*! @typedef AudioPlayerCallback
 *  Callback function of AMIAudioPlayer. This callback function is called by the
 *  player when the player wants audio data to play.
 * @param data data pointer where to write data
 * @param size data size in bytes to indicate how many bytes the player wants
 * @param need_drain if true means this is the last data
 * @return how many bytes is written
 */
typedef void (*AudioPlayerCallback)(AudioPlayer *data);

/*! @class AMIAudioPlayer
 *  Defines interface for playing raw PCM
 */
class AMIAudioPlayer
{
  public:
    /*!
     * Destructor. Destroy AMAudioPlayer object
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
     * @param ainfo audio information @sa AM_AUDIO_INFO
     * @param volume audio initial volume, -1 means device default value
     * @return true if success, otherwise false
     */
    virtual bool start(int32_t volume = -1)    = 0;

    /*!
     * Stop audio capture
     * @param wait wait for player to drain data in fifo
     * @return true if success, otherwise false
     */
    virtual bool stop(bool wait = true)                              = 0;

    /*!
     * Pause / Resume player
     * @param enable set true to pause, set false to resume
     * @return true if success, false on failure
     */
    virtual bool pause(bool enable)                                  = 0;

    /*!
     * Set audio information of audio which will be played
     * @param ainfo audio information @sa AM_AUDIO_INFO
     */
    virtual void set_audio_info(AM_AUDIO_INFO &ainfo)                = 0;

    /*!
     * set stream volume
     * @param volume playback stream volume,
     *        (0, 100], -1 to set to default volume
     * @return true if success, otherwise false
     */
    virtual bool set_volume(int32_t volume)                          = 0;

    /*!
     * Check if player is playing
     * @return true if player is playing, otherwise false
     */
    virtual bool is_player_running()                                 = 0;

    /*!
     * Set player's latency buffer in milliseconds
     * @param ms milliseconds
     */
    virtual void set_player_default_latency(uint32_t ms)             = 0;
    /*!
     * Set audio player's callback funcion
     * @param callback @ref AudioPlayerCallback
     */
    virtual void set_player_callback(AudioPlayerCallback callback)   = 0;

    /*!
     * Virtual destructor function
     */
    virtual ~AMIAudioPlayer(){}
};

/*! Creator
 * @param interface
 * - pulse: use PulseAudio interface
 * - alsa:  use alsa interface(Not implemented)
 * @param name name of this player
 * @param owner owner class of this audio player
 * @param callback audio player callback function
 * @return The pointer of AMIAudioPlayer
 */
AMIAudioPlayer* create_audio_player(const std::string& interface,
                                    const std::string& name,
                                    void *owner,
                                    AudioPlayerCallback callback);
#endif /* AM_AUDIO_PLAYER_IF_H_ */
