/*******************************************************************************
 * am_record_engine_if.h
 *
 * History:
 *   2014-12-30 - [ypchang] created file
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
#ifndef ORYX_STREAM_INCLUDE_RECORD_AM_RECORD_ENGINE_IF_H_
#define ORYX_STREAM_INCLUDE_RECORD_AM_RECORD_ENGINE_IF_H_

#include "am_record_msg.h"
#include "am_record_event_sender.h"
#include "am_audio_define.h"

extern const AM_IID IID_AMIRecordEngine;

struct AMImage3AInfo;
struct AMRecordMsg;
class AMIRecordEngine: public AMIInterface
{
  public:
    enum AM_RECORD_ENGINE_STATUS
    {
      AM_RECORD_ENGINE_ERROR,
      AM_RECORD_ENGINE_TIMEOUT,
      AM_RECORD_ENGINE_STARTING,
      AM_RECORD_ENGINE_RECORDING,
      AM_RECORD_ENGINE_STOPPING,
      AM_RECORD_ENGINE_STOPPED,
      AM_RECORD_ENGINE_ABORT,
    };

    static AMIRecordEngine* create();

  public:
    DECLARE_INTERFACE(AMIRecordEngine, IID_AMIRecordEngine);
    virtual AM_RECORD_ENGINE_STATUS get_engine_status()                     = 0;
    virtual bool create_graph()                                             = 0;
    virtual bool record()                                                   = 0;
    virtual bool stop()                                                     = 0;
    virtual bool start_file_recording(uint32_t muxer_id_bit_map)            = 0;
    virtual bool stop_file_recording(uint32_t muxer_id_bit_map)             = 0;
    virtual bool start_file_muxer()                                         = 0;
    virtual bool set_recording_file_num(uint32_t muxer_id_bit_map,
                                        uint32_t file_num)                  = 0;
    virtual bool set_recording_duration(uint32_t muxer_id_bit_map,
                                        int32_t duration)                   = 0;
    virtual bool set_file_duration(uint32_t muxer_id_bit_map,
                                   int32_t file_duration,
                                   bool apply_conf_file)                    = 0;
    virtual bool set_muxer_param(AMMuxerParam &param)                       = 0;
    virtual void set_app_msg_callback(AMRecordCallback callback,
                                      void *data)                           = 0;
    virtual void set_aec_enabled(bool enabled)                              = 0;
    virtual bool is_ready_for_event(AMEventStruct& event)                   = 0;
    virtual bool send_event(AMEventStruct& event)                           = 0;
    virtual bool enable_audio_codec(AM_AUDIO_TYPE type,
                               uint32_t sample_rate, bool enable = true)    = 0;
    virtual bool set_file_operation_callback(uint32_t muxer_id_bit_map,
                                             AM_FILE_OPERATION_CB_TYPE type,
                                             AMFileOperationCB callback)    = 0;
    virtual bool update_image_3A_info(AMImage3AInfo *image_3A)              = 0;
};

#endif /* ORYX_STREAM_INCLUDE_RECORD_AM_RECORD_ENGINE_IF_H_ */
