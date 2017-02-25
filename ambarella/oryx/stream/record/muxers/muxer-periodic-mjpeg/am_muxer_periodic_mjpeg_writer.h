/*******************************************************************************
 * am_muxer_periodic_mjpeg_writer.h
 *
 * History:
 *   2016-04-20 - [ccjing] created file
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
#ifndef AM_MUXER_PERIODIC_MJPEG_WRITER_H_
#define AM_MUXER_PERIODIC_MJPEG_WRITER_H_
#include "am_mutex.h"
#include "am_record_msg.h"
class AMIFileWriter;
struct AMMuxerCodecPeriodicMjpegConfig;

class AMPeriodicMjpegWriter
{
  public:
    static AMPeriodicMjpegWriter* create(AMMuxerCodecPeriodicMjpegConfig *config);
  private:
    AMPeriodicMjpegWriter();
    AM_STATE init(AMMuxerCodecPeriodicMjpegConfig *config);
    virtual ~AMPeriodicMjpegWriter();

  public:
    void destroy();
    AM_STATE set_file_name(const char* file_name);
    AM_STATE write_jpeg_data(uint8_t* data, uint32_t data_len);
    AM_STATE write_text_data(uint8_t* data, uint32_t data_len);
    AM_STATE create_next_files();
    void close_files();
    bool is_file_open();
    bool set_file_operation_cb(AM_FILE_OPERATION_CB_TYPE type,
                               AMFileOperationCB callback);
  private :
    bool get_current_time_string(char *time_str, int32_t len);
    bool file_finish_cb();
    bool file_create_cb();
  private:
    char                            *m_file_name            = nullptr;
    char                            *m_tmp_jpeg_name        = nullptr;
    char                            *m_tmp_text_name        = nullptr;
    char                            *m_path_name            = nullptr;
    char                            *m_base_name            = nullptr;
    AMIFileWriter                   *m_jpeg_writer          = nullptr;
    AMIFileWriter                   *m_text_writer          = nullptr;
    AMMuxerCodecPeriodicMjpegConfig *m_config               = nullptr;
    AMFileOperationCB                m_file_finish_cb       = nullptr;
    AMFileOperationCB                m_file_create_cb       = nullptr;
    uint32_t                         m_file_counter         = 0;
    char                             m_time_string[32]      = { 0 };
    std::string                      m_name;
    AMMemLock                        m_callback_lock;
};
#endif /* AM_MUXER_PERIODIC_MJPEG_WRITER_H_ */
