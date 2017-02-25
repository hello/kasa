/*******************************************************************************
 * Mp4FileWriter.h
 *
 * History:
 *   2016-01-05 - [ccjing] created file
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
#ifndef MP4_FILE_WRITER_H_
#define MP4_FILE_WRITER_H_

class AMIFileWriter;

class Mp4FileWriter
{
  public :
    static Mp4FileWriter* create();
  private :
    Mp4FileWriter();
    virtual ~Mp4FileWriter();
    bool init();
  public :
    void destroy();
    bool create_file(const char* file_name);
    bool set_buf(uint32_t size);
    bool write_data(uint8_t *data, uint32_t data_len);
    bool seek_file(uint32_t offset, uint32_t whence);//SEEK_SET
    bool tell_file(uint64_t& offset);
    bool write_be_u8(uint8_t value);
    bool write_be_s8(int8_t value);
    bool write_be_u16(uint16_t value);
    bool write_be_s16(int16_t value);
    bool write_be_u24(uint32_t value);
    bool write_be_s24(int32_t value);
    bool write_be_u32(uint32_t value);
    bool write_be_s32(int32_t value);
    bool write_be_u64(uint64_t value);
    bool write_be_s64(int64_t value);
    bool flush_file();
    bool is_file_open();
    void close_file(bool need_sync);
    uint64_t get_file_size();
  private :
    AMIFileWriter *m_file_writer;
};

#endif /* MP4_FILE_WRITER_H_ */
