/*******************************************************************************
 * am_record_msg.h
 *
 * History:
 *   2014-12-2 - [ypchang] created file
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

/*! @file am_record_msg.h
 *  @brief defines record engine message types, data structures and callbacks
 */

#ifndef AM_RECORD_MSG_H_
#define AM_RECORD_MSG_H_

/*! @enum AM_RECORD_MSG
 *  @brief record engine message
 *         These message types indicate record engine status.
 */
enum AM_RECORD_MSG
{
  AM_RECORD_MSG_START_OK, //!< START is OK
  AM_RECORD_MSG_STOP_OK,  //!< STOP is OK
  AM_RECORD_MSG_ERROR,    //!< ERROR has occurred
  AM_RECORD_MSG_ABORT,    //!< Engine is aborted
  AM_RECORD_MSG_EOS,      //!< End of Stream
  AM_RECORD_MSG_OVER_FLOW,//!< IO Over flow
  AM_RECORD_MSG_TIMEOUT,  //!< Operation timeout
  AM_RECORD_MSG_NULL      //!< Invalid message
};

/*! @struct AMRecordMsg
 *  @brief This structure contains message that sent by record engine.
 *  It is usually used by applications to retrieve engine status.
 */
struct AMRecordMsg
{
    void         *data = nullptr; //!< user data
    AM_RECORD_MSG msg  = AM_RECORD_MSG_NULL;  //!< engine message
};

/*! @typedef AMRecordCallback
 *  @brief record callback function type
 *         Use this function to get message sent by record engine, usually
 *         this message contains engine status.
 * @param msg AMRecordMsg reference
 * @sa AMRecordMsg
 */
typedef void (*AMRecordCallback)(AMRecordMsg &msg);

/*! @enum AM_RECORD_FILE_INFO_TYPE
 *  @brief This enum defines record file info type.
 */
enum AM_RECORD_FILE_INFO_TYPE
{
  AM_RECORD_FILE_INFO_NULL   = -1,
  AM_RECORD_FILE_CREATE_INFO = 0,
  AM_RECORD_FILE_FINISH_INFO = 1,
  AM_RECORD_FILE_INFO_NUM,
};

/*! @struct AMRecordFileInfo
 *  @brief This structure contains information of media file which was
 *         recorded by oryx.
 */
struct AMRecordFileInfo
{
    AM_RECORD_FILE_INFO_TYPE type = AM_RECORD_FILE_INFO_NULL;
    uint32_t stream_id            = 0;
    uint32_t muxer_id             = 0;
    char     create_time_str[32]  = { 0 };
    char     finish_time_str[32]  = { 0 };
    char     file_name[128]       = { 0 };
};

/*! @typedef file_operation_callback
 *  @brief file operation callback function.
 */
typedef void (*AMFileOperationCB)(AMRecordFileInfo &file_info);

/*! @enum AM_FILE_OPERATION_CB_TYPE
 *  @brief File operation callback type.
 */
enum AM_FILE_OPERATION_CB_TYPE
{
  AM_OPERATION_CB_TYPE_NULL   = -1,
  AM_OPERATION_CB_FILE_FINISH = 0,
  AM_OPERATION_CB_FILE_CREATE = 1,
  AM_OPERATION_CB_TYPE_NUM
};

/*! @enum AM_MUXER_ID
 *  @brief muxer id.
 */
enum AM_MUXER_ID
{
  AM_MUXER_MP4_NORMAL_0    = 0,
  AM_MUXER_MP4_NORMAL_1    = 12,
  AM_MUXER_MP4_EVENT_0     = 1,
  AM_MUXER_MP4_EVENT_1     = 2,
  AM_MUXER_TS_NORMAL_0     = 3,
  AM_MUXER_TS_NORMAL_1     = 13,
  AM_MUXER_TS_EVENT_0      = 4,
  AM_MUXER_TS_EVENT_1      = 5,
  AM_MUXER_RTP             = 6,
  AM_MUXER_JPEG_NORMAL     = 7,
  AM_MUXER_JPEG_EVENT_0    = 8,
  AM_MUXER_JPEG_EVENT_1    = 9,
  AM_MUXER_PERIODIC_JPEG   = 10,
  AM_MUXER_TIME_ELAPSE_MP4 = 11,
  AM_MUXER_AV3_NORMAL_0    = 14,
  AM_MUXER_AV3_NORMAL_1    = 15,
  AM_MUXER_AV3_EVENT_0     = 16,
  AM_MUXER_AV3_EVENT_1     = 17,
  AM_MUXER_EXPORT          = 18,
  AM_MUXER_ID_MAX,
};

/*! @struct AMSettingOption
 *  @brief muxer parameter element.
 */
struct AMSettingOption
{
    union {
        uint32_t v_u32;
        int32_t  v_int32;
        bool     v_bool;
    } value;
    bool is_set = false;
};

/*! @struct AMMuxerParam
 *  @brief muxer parameters.
 */
struct AMMuxerParam
{
    uint32_t        muxer_id_bit_map  = 0;//0x00000001 << AM_MUXER_ID
    AMSettingOption file_duration_int32;
    AMSettingOption recording_file_num_u32;
    AMSettingOption recording_duration_u32;
    AMSettingOption digital_sig_enable_bool; //for av3 only
    AMSettingOption gsensor_enable_bool;
    AMSettingOption reconstruct_enable_bool;
    AMSettingOption max_file_size_u32;
    AMSettingOption write_sync_enable_bool;
    AMSettingOption hls_enable_bool;
    AMSettingOption scramble_enable_bool; //for av3 only
    AMSettingOption video_frame_rate_u32; // for time_elapse_mp4 only currently
    bool            save_to_config_file = false;
};

#endif /* AM_RECORD_MSG_H_ */
