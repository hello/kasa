/*******************************************************************************
 * mp4_file_reader.h
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
#ifndef MP4_FILE_READER_H_
#define MP4_FILE_READER_H_
class AMIFileReader;

class Mp4FileReader
{
  public :
    static Mp4FileReader* create(const char* file_name);
  private :
    Mp4FileReader();
    virtual ~Mp4FileReader();
    bool init(const char* file_name);
  public :
    void destroy();
  public :
    bool seek_file(uint32_t offset);//from the begin of the file
    bool advance_file(uint32_t offset);//from the current position
    int  read_data(uint8_t *data, uint32_t size);
    bool read_le_u8(uint8_t& value);
    bool read_le_s8(int8_t& value);
    bool read_le_u16(uint16_t& value);
    bool read_le_s16(int16_t& value);
    bool read_le_u24(uint32_t& value);
    bool read_le_s24(int32_t& value);
    bool read_le_u32(uint32_t& value);
    bool read_le_s32(int32_t& value);
    bool read_le_u64(uint64_t& value);
    bool read_le_s64(int64_t& value);
    bool read_box_tag(uint32_t& value);
    bool tell_file(uint64_t& offset);
    uint32_t get_box_type();
    uint64_t get_file_size();
    void close_file();
  private :
    AMIFileReader *m_file_reader;
};

#endif /* MP4_FILE_READER_H_ */
