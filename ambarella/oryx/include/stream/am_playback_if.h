/*******************************************************************************
 * am_playback_if.h
 *
 * History:
 *   2014-12-9 - [ypchang] created file
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

/*! @mainpage Oryx Stream Playback
 *  @section Introduction
 *  Oryx Stream Playback module is used to play audio file and audio RTP stream,
 *  which supports ADTS AAC, OPUS, WAV(G.711, G.726, PCM) and RTP stream that
 *  contains AAC, OPUS, G.711 and G.726
 */

/*! @file am_playback_if.h
 *  @brief AMIPlayback interface
 *         This file contains AMPlayback engine interface
 */

#ifndef AM_PLAYBACK_IF_H_
#define AM_PLAYBACK_IF_H_

#include "am_playback_msg.h"
#include "am_playback_info.h"
#include "am_pointer.h"
#include "version.h"

class AMIPlayback;
/*! @typedef AMIPlaybackPtr
 *  @brief Smart pointer type used to handle AMIPlayback pointer.
 */
typedef AMPointer<AMIPlayback> AMIPlaybackPtr;

/*! @class AMIPlayback
 *  @brief AMIPlayback is the interface class that used to play audio file or
 *         RTP audio payloads.
 *  This class supports to play ADTS AAC, OPUS, WAV(G.711, G.726, PCM) and RTP
 *  payloads that contains AAC, OPUS, G.711, G.726
 */
class AMIPlayback
{
    friend AMIPlaybackPtr;

  public:
    /*! Constructor function.
     * Constructor static function, which creates an object of the underlying
     * object of AMIPlayback class and return an smart pointer class type.
     * This function doesn't take any parameters.
     * @return AMIPlaybackPtr
     */
    static AMIPlaybackPtr create();

    /*! Initialize the object returned by create()
     * @sa create()
     * @return true if initialization is successful, otherwise return false.
     */
    virtual bool init()                                                    = 0;

    /*! Re-initialize the playback object, make it work from beginning.
     * @return true if success, otherwise return false.
     * @sa init()
     */
    virtual bool reload()                                                  = 0;

    /*! Test function to get playback engine's status.
     * @return true if engine is playing, otherwise false.
     * @sa init()
     */
    virtual bool is_playing()                                              = 0;

    /*! Test function to get playback engine's status.
     * @return true if engine is paused, otherwise false.
     * @sa is_paused()
     */
    virtual bool is_paused()                                               = 0;

    /*! Test function to get playback engine's status.
     * @return true if engine is idle, otherwise false.
     * @sa is_idle()
     */
    virtual bool is_idle()                                                 = 0;

    /*! Add media that to be played into playback engine.
     * @param uri a reference to struct AMPlaybackUri object.
     * @return true if success, otherwise false.
     * @sa play()
     * @sa is_playing()
     * @sa AMPlaybackUri
     */
    virtual bool add_uri(const AMPlaybackUri& uri)                         = 0;

    /*! Play a media that described by uri.
     * The default value of uri is nullptr, in this case, play() will
     * start playing those media added by add_uri();
     * If the value of uri is NOT nullptr, play() will start playing the
     * media specified by uri;
     * @param uri a pointer type of AMPlaybackUri.
     * @return true if success, otherwise false.
     * @sa add_uri()
     */
    virtual bool play(AMPlaybackUri *uri = nullptr)                        = 0;

    /*! If enable is true, it pauses playing, otherwise resume playing
     * @param enable bool type, true/false indicate pause/resume
     * @return true if success, otherwise false.
     */
    virtual bool pause(bool enable)                                        = 0;

    /*! Stop playing, takes no parameters
     * @return true if success, otherwise false.
     */
    virtual bool stop()                                                    = 0;

    /*! Specify a callback function to playback engine to get engine status
     * @param callback AMPlaybackCallback function type
     * @param data user data
     * @sa AMPlaybackCallback
     */
    virtual void set_msg_callback(AMPlaybackCallback callback, void *data) = 0;

    /*! Set Audio Echo Cancellation enabled / disabled
     *
     * @param enabled true/false
     */
    virtual void set_aec_enabled(bool enabled)                             = 0;

  protected:
    /*! Decrease the reference of this object, when the reference count reaches
     *  0, destroy the object.
     * Must be implemented by the inherited class
     */
    virtual void release()                                                 = 0;

    /*! Increase the reference count of this object
     */
    virtual void inc_ref()                                                 = 0;

  protected:
    /*! Destructor
     */
    virtual ~AMIPlayback(){}
};

/*! @example test_oryx_playback.cpp
 *  Test program of AMIPlayback.
 */
#endif /* AM_PLAYBACK_IF_H_ */
