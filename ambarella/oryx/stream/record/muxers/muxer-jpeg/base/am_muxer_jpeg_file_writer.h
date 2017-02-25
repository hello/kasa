/*******************************************************************************
 * am_muxer_jpeg_file_writer.h
 *
 * History:
 *   2015-10-8 - [ccjing] created file
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
#ifndef AM_MUXER_JPEG_FILE_WRITER_H_
#define AM_MUXER_JPEG_FILE_WRITER_H_
#include "am_record_msg.h"
#include "am_mutex.h"
class AMIFileWriter;
struct AMMuxerCodecJpegConfig;

class AMJpegFileWriter
{
  public:
    static AMJpegFileWriter* create(AMMuxerCodecJpegConfig *muxer_jpeg_config);

  private:
    AMJpegFileWriter();
    AM_STATE init(AMMuxerCodecJpegConfig *muxer_jpeg_config);
    virtual ~AMJpegFileWriter();

  public:
    void destroy();
    AM_STATE set_media_sink(const char* file_name);
    AM_STATE write_data(uint8_t* data, uint32_t data_len);
    AM_STATE write_exif_data(uint8_t* data, uint32_t data_len);
    std::string get_current_file_name();
    bool set_file_operation_callback(AMFileOperationCB callback);
  private :
    AM_STATE create_next_file();
    AM_STATE delete_oldest_file();
    AM_STATE create_exif_next_file();
    AM_STATE delete_exif_oldest_file();
    bool file_operation_callback();
    bool get_current_time_string(char *time_str, int32_t len,
                                 const char *format);
  private:
    char                   *m_file_name              = nullptr;
    char                   *m_tmp_file_name          = nullptr;
    char                   *m_exif_tmp_file_name     = nullptr;
    char                   *m_path_name              = nullptr;
    char                   *m_base_name              = nullptr;
    AMIFileWriter          *m_file_writer            = nullptr;
    AMIFileWriter          *m_exif_file_writer       = nullptr;
    AMMuxerCodecJpegConfig *m_config                 = nullptr;
    AMFileOperationCB       m_file_operation_cb      = nullptr;
    uint32_t                m_file_counter           = 0;
    uint32_t                m_exif_file_counter      = 0;
    char                    m_time_string[32]        = { 0 };
    std::string             m_name;
    AMMemLock               m_callback_lock;
};
#endif /* AM_MUXER_JPEG_FILE_WRITER_H_ */
