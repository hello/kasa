/*******************************************************************************
 * am_file_sink.h
 *
 * History:
 *   2014-12-11 - [ccjing] created file
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
#ifndef AM_FILE_SINK_H_
#define AM_FILE_SINK_H_

#include "am_file_sink_if.h"
#include "am_mutex.h"

class AMFile;
/*
 * AMFileReader
*/
class AMFileReader: public AMIFileReader
{
  public:
    static AMFileReader * create();

  private:
    AMFileReader(): m_file(nullptr){}
    bool init();
    virtual ~AMFileReader();

  public:
    virtual void destroy();
    //AMIFileReader
    virtual bool open_file(const char* file_name);
    virtual void close_file();
    virtual uint64_t get_file_size();
    virtual bool tell_file(uint64_t& offset);
    virtual bool seek_file(int32_t offset);
    virtual bool advance_file(int32_t offset);
    virtual int read_file(void* buffer, uint32_t size);

  private:
    AMFile   *m_file;
    AMMemLock m_lock;
};

/*
 * AMFileWriter
*/
class AMFileWriter: public AMIFileWriter
{
  public:
    static AMFileWriter* create();
  private:
    AMFileWriter() :
      m_write_data_size(0),
      m_write_avg_speed(0),
      m_hist_min_speed(0xFFFFFFFF),
      m_hist_max_speed(0),
      m_write_time(0),
      m_custom_buf(nullptr),
      m_data_write_addr(nullptr),
      m_data_send_addr(nullptr),
      m_buf_size(0),
      m_data_size(0),
      m_write_cnt(0),
      m_file_fd(-1),
      m_IO_direct_set(false),
      m_enable_direct_IO(false)
    {}
    bool init();
    virtual ~AMFileWriter();

  public:
    virtual void destroy();

    //AMIFileWriter
    virtual bool create_file(const char* file_name);
    virtual void close_file(bool need_sync = false);
    virtual bool flush_file();
    virtual bool set_buf(uint32_t size);
    virtual bool tell_file(uint64_t& offset);
    virtual bool seek_file(uint32_t offset, uint32_t whence = SEEK_SET);
    virtual bool write_file(const void* buf, uint32_t len);
    virtual void set_enable_direct_IO(bool enable);
    virtual bool is_file_open();
    virtual uint64_t get_file_size();

  private:
    bool set_file_flag(int flag, bool enable);
    ssize_t write(const void* buf, size_t nbyte);
    void _close_file(bool need_sync = false);
    bool _flush_file();

  private:
    uint64_t  m_write_data_size;
    uint64_t  m_write_avg_speed;
    uint64_t  m_hist_min_speed;
    uint64_t  m_hist_max_speed;
    uint64_t  m_write_time;
    uint8_t  *m_custom_buf;
    uint8_t  *m_data_write_addr;
    uint8_t  *m_data_send_addr;
    uint32_t  m_buf_size;
    uint32_t  m_data_size;
    uint32_t  m_write_cnt;
    int       m_file_fd;
    bool      m_IO_direct_set;
    bool      m_enable_direct_IO;
    AMMemLock m_lock;
};

#endif /* AM_FILE_SINK_H_ */
