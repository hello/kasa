/*******************************************************************************
 * am_record_if.h
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

/*! @mainpage Oryx Stream Record
 *  @section Introduction
 *  Oryx Stream Record module is used to record Video and Audio data into file
 *  container or into RTP payload for network transmission.
 *  Oryx record module supports MP4 and TS file container, and supports RTP
 *  packet.
 */

/*! @file am_record_if.h
 *  @brief AMIRecord interface
 *  This file contains AMRecord engine interface
 */

#ifndef AM_RECORD_IF_H_
#define AM_RECORD_IF_H_

#include "am_record_msg.h"
#include "am_pointer.h"
#include "version.h"
#include "am_record_event_sender.h"
#include "am_audio_define.h"
#include "am_image_quality_data.h"
class AMIRecord;
/*! @typedef AMIRecordPtr
 *  @brief Smart pointer type used to handle AMIRecord pointer.
 */
typedef AMPointer<AMIRecord> AMIRecordPtr;

/*! @class AMIRecord
 *  @brief AMIRecord is the interface class that used to record Video and Audio
 *         into file container or into RTP packets.
 */
class AMIRecord
{
    friend AMIRecordPtr;

  public:
    /*! Constructor function.
     * Constructor static function, which creates an object of the underlying
     * object of AMIRecord class and return an smart pointer class type.
     * This function doesn't take any parameters.
     * @return AMIRecordPtr
     * @sa init()
     */
    static AMIRecordPtr create();

    /*! Initialize the object return by create()
     * @return true if success, otherwise return false
     * @sa create()
     */
    virtual bool init()                                                       = 0;

    /*! Re-initialize the object
     * @return true if success, otherwise return false
     * @sa init()
     * @sa create()
     */
    virtual bool reload()                                                     = 0;

    /*! Start recording
     * @return true if success, otherwise return false
     * @sa stop();
     */
    virtual bool start()                                                      = 0;

    /*! Stop recording
     * @return true if success, otherwise return false
     * @sa start()
     */
    virtual bool stop()                                                       = 0;

    /*! Start file muxer
     * @return true if success, otherwise return false
     */
    virtual bool start_file_muxer()                                           = 0;

    /*! Start file recording
     * @param muxer_id_bit_map specifies the muxer which you want to start file
     * recording. For example, muxer_id_bit_map should be set as this
     * muxer_id_bit_map |= (0x01 << 1) if you want to start muxer 1 file recording.
     * @return true if success, otherwise return false
     * @sa stop_file_recording();
     */
    virtual bool start_file_recording(uint32_t muxer_id_bit_map = 0xffffffff) = 0;

    /*! Stop file recording
     * @param muxer_id_bit_map specifies the muxer which you want to stop file
     * recording. For example, muxer_id_bit_map should be set as this
     * muxer_id_bit_map |= (0x01 << 1) if you want to stop muxer 1 file recording.
     * @return true if success, otherwise return false
     * @sa start_file_recording()
     */
    virtual bool stop_file_recording(uint32_t muxer_id_bit_map = 0xffffffff)  = 0;

    /*! This function is used to set recording file num.
     * @param muxer_id_bit_map, file_num.
     * @return true if success, otherwise return false
     */
    virtual bool set_recording_file_num(uint32_t muxer_id_bit_map,
                                        uint32_t file_num)                    = 0;
    /*! This function is used to set recording duration.
     * @param muxer_id_bit_map, duration.
     * @return true if success, otherwise return false
     */
    virtual bool set_recording_duration(uint32_t muxer_id_bit_map,
                                        int32_t duration)                     = 0;
    /*! This function is used to set file duration.
     * @param muxer_id_bit_map, file duration, apply_conf_file.
     * @return true if success, otherwise return false
     */
    virtual bool set_file_duration(uint32_t muxer_id_bit_map,
                                   int32_t file_duration,
                                   bool apply_conf_file)                      = 0;
    /*! This function is used to set muxer param.
     * @param param, muxer parameter.
     * @return true if success, otherwise return false
     */
    virtual bool set_muxer_param(AMMuxerParam &param)                         = 0;
    /*! This function is used to set file finished callback.
     * @param muxer_id_bit_map, callback function, file operation type
     * @return true if success, otherwise return false
     */
    virtual bool set_file_operation_callback(uint32_t muxer_id_bit_map,
                                             AM_FILE_OPERATION_CB_TYPE type,
                                             AMFileOperationCB callback)      = 0;

    /*! This function is used to update image 3A info.
     * @param AMImage3AInfo
     * @return true if success, otherwise return false
     */
    virtual bool update_image_3A_info(AMImage3AInfo *image_3A)                = 0;


    /*! Test function to test if record engine is working
     * @return true if record engine is recording, otherwise return false
     * @sa is_ready_for_event()
     */
    virtual bool is_recording()                                               = 0;

    /*! Must be called before calling send_event(), this function tests if
     *  the record engine is currently ready for recording event file.
     * @param  EventStruct which describes event attr and other information.
     * @return true if record engine is ready for recording event file,
     *         otherwise return false.
     * @sa send_event()
     * @sa is_recording()
     */
    virtual bool is_ready_for_event(AMEventStruct& event)                     = 0;

    /*! Trigger an event operation inside record engine to make the engine start
     *  recording file clip to record the duration of this event.
     * @param EventStruct which describes event attr and other information.
     * @return true if success, otherwise return false
     */
    virtual bool send_event(AMEventStruct& event)                             = 0;

    /*! Enable or disable specified audio codec
     * @param audio type.
     * @param sample_rate
     * @param true means enable and false means disable.
     * @return true if success, otherwise return false
     */
    virtual bool enable_audio_codec(AM_AUDIO_TYPE type, uint32_t sample_rate,
                                    bool enable)                              = 0;
    /*! Set a callback function to receive record engine messages
     * @param callback callback function type
     * @param data user data
     */
    virtual void set_msg_callback(AMRecordCallback callback, void *data)      = 0;

    /*! Set Audio Echo Cancellation enabled / disabled
     *
     * @param enabled true/false
     */
    virtual void set_aec_enabled(bool enabled)                                = 0;

  protected:
    /*! Decrease the reference of this object, when the reference count reaches
     *  0, destroy the object.
     * Must be implemented by the inherited class
     */
    virtual void release()                                                    = 0;

    /*! Increase the reference count of this object
     */
    virtual void inc_ref()                                                    = 0;

  protected:
    /*! Destructor
     */
    virtual ~AMIRecord(){}
};

/*! @example test_oryx_record.cpp
 *  Test program of AMIRecord.
 */
#endif /* AM_RECORD_IF_H_ */
