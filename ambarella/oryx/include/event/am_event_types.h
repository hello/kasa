/*******************************************************************************
 * am_event_types.h
 *
 * Histroy:
 *  2014-11-19  [Dongge Wu] created file
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

/*! @file am_event_types.h
 *  @brief This file defines Event Service related data structure and enumeration
 */
#ifndef AM_EVENT_TYPES_H_
#define AM_EVENT_TYPES_H_

/*! @enum EVENT_MODULE_ID
 * Defines all event modules ID
 */
enum EVENT_MODULE_ID
{
  EV_AUDIO_ALERT_DECT = 0,//!< Plugin ID of audio alert
  EV_AUDIO_ANALYSIS_DECT, //!< Plugin ID of audio analysis
  EV_FACE_DECT,           //!< Plugin ID of face detection
  EV_KEY_INPUT_DECT,      //!< Plugin ID of key press detection
  EV_ALL_MODULE_NUM,      //!< Total number of Plugins
};

/*! @struct EVENT_MODULE_CONFIG
 *  Defines event module configure.
 */
struct EVENT_MODULE_CONFIG
{
    void *value; //!< specifying the value of configure item.
    uint32_t key;//!< specifying which configure item it is.
};

/*! Audio Alert max threshold  */
#define AM_AUDIO_ALERT_MAX_THRESHOLD   (100)

/*! Audio Alert max sensitivity */
#define AM_AUDIO_ALERT_MAX_SENSITIVITY (100)

/*! @enum AM_AUDIO_ALERT_KEY
 * Defines configure item of audio alert event module.
 */
enum AM_AUDIO_ALERT_KEY
{
  //! Channel Number. 1:mono, 2:stereo
  AM_CHANNEL_NUM = 0,

  //! Sample Rate. 8K, 16K, 32K, 48K
  AM_SAMPLE_RATE,

  //! Chunk Bytes. 1024, 2048
  AM_CHUNK_BYTES,

  //! Sample Format. PCM audio format
  AM_SAMPLE_FORMAT,

  //! Audio Alert Enable. 0: disable, 1: enable
  AM_ALERT_ENABLE,

  //! Audio Alert Callback function register.
  AM_ALERT_CALLBACK,

  //! Audio Alert Sensitivity. 0-100
  AM_ALERT_SENSITIVITY,

  //! Audio Alert Threshold. 0-100
  AM_ALERT_THRESHOLD,

  //! Audio Alert Direction. 0: rise edge, 1: down edge
  AM_ALERT_DIRECTION,

  //! Audio Alert sync up configure item to configure file.
  AM_ALERT_SYNC_CONFIG,

  //! Audio Alert Key number.
  AM_AUDIO_ALERT_KEY_NUM,
};

/*! @enum AM_KEY_STATE
 *  Defines key input states
 */
enum AM_KEY_STATE
{
  AM_KEY_UP = 0X01,   //!< key up.
  AM_KEY_DOWN,        //!< key down.
  AM_KEY_CLICKED,     //!< key clicked.
  AM_KEY_LONG_PRESSED,//!< key long time press.
  AM_KEY_STATE_NUM,   //!< total number of key states.
};

/*! @typedef AM_KEY_CODE
 *  @brief Defines key ID
 */
typedef int32_t AM_KEY_CODE;

/*! @struct AM_KEY_INPUT_EVENT
 */
struct AM_KEY_INPUT_EVENT
{
    AM_KEY_CODE key_value; //!< key ID
    AM_KEY_STATE key_state;//!< key state
};

/*! @enum AM_KEY_INPUT_KEY
 *  Defines key input event module configure item
 */
enum AM_KEY_INPUT_KEY
{
  AM_GET_KEY_STATE = 0, //!< Just for get, value : AM_KEY_INPUT_EVENT
  AM_WAIT_KEY_PRESSED,  //!< Just for get, value: AM_KEY_INPUT_EVENT
  AM_KEY_CALLBACK,      //!< Just for set, value: AM_KEY_INPUT_CALLBACK
  AM_LONG_PRESSED_TIME, //!< set/get, long press time(milliseconds)
  AM_KEY_INPUT_SYNC_CONFIG,//!< sync configure item and save to configure file
  AM_KEY_INPUT_KEY_NUM,    //!< total number of key input configure items
};

/*! @struct AM_EVENT_MESSAGE
 *  Defines event message for all event modules
 */
struct AM_EVENT_MESSAGE
{
    /*!
     * sequence number of events
     */
    uint64_t seq_num;

    /*!
     * PTS of this event
     */
    uint64_t pts;

    /*!
     * event type.
     * @sa EVENT_MODULE_ID
     */
    EVENT_MODULE_ID event_type;

    union
    {
        /*!
         * event message of key input event module.
         * @sa AM_KEY_INPUT_EVENT
         */
        AM_KEY_INPUT_EVENT key_event;
    };
};

/*! @typedef AM_EVENT_CALLBACK
 *  @brief Event callback function type
 *  @param event_msg @ref AM_EVENT_MESSAGE "event message data"
 *  @return int32_t
 */
typedef int32_t (*AM_EVENT_CALLBACK)(AM_EVENT_MESSAGE *event_msg);

/*! @struct AM_KEY_INPUT_CALLBACK
 *  Defines key input callback
 */
struct AM_KEY_INPUT_CALLBACK
{
    /*!
     * key ID.
     * @sa AM_KEY_CODE
     */
    AM_KEY_CODE key_value;

    /*!
     * callback.
     * @sa AM_EVENT_CALLBACK
     */
    AM_EVENT_CALLBACK callback;
};

#endif
