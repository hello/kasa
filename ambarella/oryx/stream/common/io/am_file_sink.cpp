/*******************************************************************************
 * am_file_sink.cpp
 *
 * History:
 *   2014-12-8 - [ccjing] created file
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_file_sink.h"

#include "am_file.h"
#include "am_mutex.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#define WRITE_STATISTICS_THRESHOLD 100
/*
 * AMFileReader
 */
AMIFileReader* AMIFileReader::create()
{
  return AMFileReader::create();
}

AMFileReader* AMFileReader::create()
{
  AMFileReader* result = new AMFileReader();
  if (result && result->init() != true) {
    delete result;
    result = NULL;
  }
  return result;
}

bool AMFileReader::init()
{
  m_file = new AMFile();
  return (m_file != nullptr);
}

AMFileReader::~AMFileReader()
{
  close_file();
  delete m_file;
}

void AMFileReader::destroy()
{
  delete this;
}

bool AMFileReader::open_file(const char* file_name)
{
  bool ret = true;
  AUTO_MEM_LOCK(m_lock);
  do{
    m_file->close();
    m_file->set_file_name(file_name);
    if (AM_UNLIKELY(!(m_file->open(AMFile::AM_FILE_READONLY)))) {
      ERROR("Failed to open file: %s", file_name);
      ret = true;
      break;
    }
  } while(0);

  return ret;
}

void AMFileReader::close_file()
{
  AUTO_MEM_LOCK(m_lock);
  m_file->close();
}

uint64_t AMFileReader::get_file_size()
{
  AUTO_MEM_LOCK(m_lock);
  return m_file->size();
}

bool AMFileReader::tell_file(uint64_t& offset)
{
  off_t cur_pos = 0;
  bool ret = true;
  do {
    AUTO_MEM_LOCK(m_lock);
    cur_pos = ::lseek(m_file->handle(), 0, SEEK_CUR);
    if(cur_pos < 0) {
      PERROR("tell file :");
      ret = false;
      break;
    }
    offset = (uint64_t)cur_pos;
  }while(0);
  return ret;
}

bool AMFileReader::seek_file(int32_t offset)
{
  bool ret = true;
  AUTO_MEM_LOCK(m_lock);
  do{
    if (AM_UNLIKELY(!(m_file->seek(offset, AMFile::AM_FILE_SEEK_SET)))) {
      ERROR("Failed to seek the file: %s", m_file->name());
      ret = false;
      break;
    }
  } while(0);

  return ret;
}

bool AMFileReader::advance_file(int32_t offset)
{
  bool ret = true;
  AUTO_MEM_LOCK(m_lock);
  do{
    if (AM_UNLIKELY(!(m_file->seek(offset, AMFile::AM_FILE_SEEK_CUR)))) {
      ERROR("Failed to advance the file: %s", m_file->name());
      ret = false;
      break;
    }
  } while(0);

  return ret;
}

int AMFileReader::read_file(void * buffer, uint32_t size)
{
  int read_size = 0;
  AUTO_MEM_LOCK(m_lock);
  read_size = m_file->read((char*) buffer, size);
  return read_size;
}

/*
 * AMFileWriter
 */
AMIFileWriter* AMIFileWriter::create()
{
  return AMFileWriter::create();
}

AMFileWriter* AMFileWriter::create()
{
  AMFileWriter* result = new AMFileWriter();
  if (result && result->init() != true) {
    delete result;
    result = NULL;
  }
  return result;
}

bool AMFileWriter::init()
{
  return true;
}

AMFileWriter::~AMFileWriter()
{
  if (m_file_fd >= 0) {
    close_file();
  }
  delete[] m_custom_buf;
}

void AMFileWriter::destroy()
{
  delete this;
}

void AMFileWriter::set_enable_direct_IO(bool enable)
{
  m_enable_direct_IO = enable;
}

bool AMFileWriter::create_file(const char* file_name)
{
  bool ret = true;
  AUTO_MEM_LOCK(m_lock);
  do {
    if (m_file_fd >= 0) {
      _close_file();
    }
    if (AM_UNLIKELY(file_name == NULL)) {
      ERROR("File name is empty!");
      ret = false;
      break;
    }
    m_file_fd = ::open(file_name, O_WRONLY | O_CREAT | O_TRUNC,
    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (AM_UNLIKELY(m_file_fd < 0)) {
      ERROR("Can not open file %s: %s", file_name, strerror(errno));
      ret = false;
      break;
    }
    DEBUG("File %s is created successfully!", file_name);
    m_data_size = 0;
    m_data_write_addr = m_custom_buf;
    m_data_send_addr = m_custom_buf;
  } while (0);

  return ret;
}

void AMFileWriter::close_file(bool need_sync)
{
  AUTO_MEM_LOCK(m_lock);
  _close_file(need_sync);
}
void AMFileWriter::_close_file(bool need_sync)
{
  do {
    if (AM_UNLIKELY(m_file_fd < 0)) {
      INFO("File has been closed!");
      break;
    }
    _flush_file();
    if (AM_UNLIKELY(need_sync && (::fsync(m_file_fd) < 0))) {
      PERROR("fsync");
    }
    if (AM_UNLIKELY(::close(m_file_fd) != 0)) {
      ERROR("Failed to close file: %s!", strerror(errno));
    }
    m_file_fd = -1;
    m_IO_direct_set = false;
  } while (0);
}

bool AMFileWriter::tell_file(uint64_t& offset)
{
  off_t cur_pos = 0;
  bool ret = true;
  do {
    AUTO_MEM_LOCK(m_lock);
    _flush_file();
    cur_pos = ::lseek(m_file_fd, 0, SEEK_CUR);
    if(cur_pos < 0) {
      PERROR("tell file :");
      ret = false;
      break;
    }
    offset = (uint64_t)cur_pos;
  }while(0);
  return ret;
}

bool AMFileWriter::write_file(const void* buf, uint32_t len)
{
  uint32_t remain_data = len;
  uint8_t* data_addr = (uint8_t*) buf;
  bool ret = true;
  AUTO_MEM_LOCK(m_lock);
  do {
    if (AM_UNLIKELY(m_file_fd < 0)) {
      ERROR("File is not open!");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(len == 0)) {
      NOTICE("Buffer len is zero!");
      break;
    }
    if (AM_UNLIKELY(NULL == m_custom_buf)) { // No IO buffer
      int result = -1;
      while (remain_data > 0) {
        if ((result = write(data_addr, remain_data)) > 0) {
          remain_data -= result;
          data_addr += result;
        } else {
          ERROR("Write file error!");
          ret = false;
          break;
        }
      }
    } else { //Use IO buffer
      uint32_t buffer_size = 0;
      uint32_t data_size = 0;
      while (remain_data > 0) {
        buffer_size = m_buf_size + m_custom_buf - m_data_write_addr;
        data_size = (remain_data <= buffer_size ? remain_data : buffer_size);
        memcpy(m_data_write_addr, data_addr, data_size);
        data_addr += data_size;
        m_data_size += data_size;
        remain_data -= data_size;
        m_data_write_addr += data_size;
        if ((uint32_t) (m_data_write_addr - m_custom_buf) >= m_buf_size) {
          m_data_write_addr = m_custom_buf;
        }
        while (m_data_size >= IO_TRANSFER_BLOCK_SIZE) {
          int32_t write_ret = write(m_data_send_addr, IO_TRANSFER_BLOCK_SIZE);
          if (AM_LIKELY(write_ret == IO_TRANSFER_BLOCK_SIZE)) {
            m_data_size -= IO_TRANSFER_BLOCK_SIZE;
            m_data_send_addr += IO_TRANSFER_BLOCK_SIZE;
            if ((uint32_t) (m_data_send_addr - m_custom_buf) >= m_buf_size) {
              m_data_send_addr = m_custom_buf;
            }
          } else {
            ERROR("Write file error: %d!", write_ret);
            ret = false;
            return ret;
          }
        }
      }
    }
    if (AM_UNLIKELY(remain_data != 0)) {
      ERROR("Write data number error!");
      ret = false;
      break;
    }
  } while (0);

  return ret;
}

bool AMFileWriter::flush_file()
{
  AUTO_MEM_LOCK(m_lock);
  return _flush_file();
}
bool AMFileWriter::_flush_file()
{
  bool ret = true;
  if ((m_data_size > 0) && m_data_send_addr && m_custom_buf) {
    if (((m_data_size % 512) != 0) && m_IO_direct_set && m_enable_direct_IO) {
      m_IO_direct_set = !(true == set_file_flag((int) O_DIRECT, false));
    }
    while (m_data_size > 0) {
      uint32_t data_remain = m_data_size;
      int32_t write_size = -1;
      if (m_data_write_addr > m_data_send_addr) {
        data_remain = (m_data_write_addr - m_data_send_addr); //mDataSize
      } else if (m_data_write_addr < m_data_send_addr) {
        data_remain = (m_custom_buf + m_buf_size - m_data_send_addr);
      } else {
        INFO("No data to flush!");
        break;
      }
      if (AM_UNLIKELY((write_size = write(m_data_send_addr, data_remain))
          != (int32_t )m_data_size)) {
        ret = false;
        PERROR("Write file error when flush file!");
        break;
      }
      m_data_size -= data_remain;
      m_data_send_addr += data_remain;
      if ((uint32_t) (m_data_send_addr - m_custom_buf) >= m_buf_size) {
        m_data_send_addr = m_custom_buf;
      }
    }
    if (m_enable_direct_IO && m_custom_buf && ((m_buf_size % 512) == 0)
        && !m_IO_direct_set) {
      m_IO_direct_set = (true == set_file_flag((int) O_DIRECT, true));
    }
  }
  m_data_size = 0;
  m_data_write_addr = m_custom_buf;
  m_data_send_addr = m_custom_buf;

  return ret;
}

bool AMFileWriter::set_buf(uint32_t size)
{
  bool ret = true;
  AUTO_MEM_LOCK(m_lock);
  do {
    if (AM_UNLIKELY(m_file_fd < 0)) {
      ERROR("Bad file descriptor!");
      ret = false;
      break;
    }
    if (m_custom_buf && ((0 == size) || (size != m_buf_size))) {
      delete[] m_custom_buf;
      m_custom_buf = NULL;
      m_data_write_addr = NULL;
      m_data_send_addr = NULL;
      m_data_size = 0;
      m_buf_size = 0;
    }
    if (size == 0) {
      ret = true;
      break;
    }
    if (AM_LIKELY(!m_custom_buf && (size > 0) &&
                  ((size % IO_TRANSFER_BLOCK_SIZE) == 0))) {
      INFO("File write buffer is set to %d KB\n", size / 1024);
      m_custom_buf = new uint8_t[size];
      if(AM_UNLIKELY(!m_custom_buf)) {
        ERROR("Alloc memory error!");
        ret = false;
        break;
      }
      m_data_write_addr = m_custom_buf;
      m_data_send_addr = m_custom_buf;
      m_data_size = 0;
      m_buf_size = (m_custom_buf ? size : 0);
    } else if ((size % IO_TRANSFER_BLOCK_SIZE) != 0) {
      ERROR("Buffer size should be multiples of %uKB!",
            (IO_TRANSFER_BLOCK_SIZE / IO_SIZE_KB));
      ret = false;
      break;
    }

    if (m_enable_direct_IO && m_custom_buf && ((size % 512) == 0)
        && !m_IO_direct_set) {
      m_IO_direct_set = (true == set_file_flag(((int) O_DIRECT), true));
      INFO("Direct IO set %s!", m_IO_direct_set? "enable" : "disable");
    }
  } while (0);

  return ret;
}

bool AMFileWriter::set_file_flag(int flag, bool enable)
{
  int fileFlags = 0;
  bool ret = true;
  do {
    if (m_file_fd < 0) {
      ERROR("Bad file descriptor! File had been closed!");
      ret = false;
      break;
    }
    if (flag & ~(O_APPEND | O_ASYNC | O_NONBLOCK | O_DIRECT | O_NOATIME)) {
      ERROR("Unsupported file flag!");
      ret = false;
      break;
    }
    fileFlags = fcntl(m_file_fd, F_GETFL);
    if (enable) {
      fileFlags |= flag;
    } else {
      fileFlags &= ~flag;
    }
    if (fcntl(m_file_fd, F_SETFL, fileFlags) < 0) {
      ERROR("Failed to %s file flags!\n ", (enable ? "enable" : "disable"));
      ret = false;
      break;
    }
  } while (0);

  return ret;
}

ssize_t AMFileWriter::write(const void *buf, size_t nbyte)
{
  size_t remain_size = nbyte;
  ssize_t retval = 0;
  uint32_t write_cnt = 0;
  struct timeval start = { 0, 0 };
  struct timeval end   = { 0, 0 };

  if (AM_UNLIKELY(gettimeofday(&start, NULL) < 0)) {
    PERROR("gettimeofday");
  }

  do {
    retval = ::write(m_file_fd, buf, remain_size);
    if (AM_LIKELY(retval > 0)) {
      remain_size -= retval;
    } else if (AM_UNLIKELY((errno != EAGAIN) && (errno != EINTR))) {
      PERROR("write");
      break;
    }
  } while ((++ write_cnt < 5) && (remain_size > 0));

  ++ m_write_cnt;
  m_write_data_size += (nbyte - remain_size);

  if (AM_LIKELY(remain_size < nbyte)) {
    if (AM_UNLIKELY(gettimeofday(&end, NULL) < 0)) {
      PERROR("gettimeofday");
    } else {
      uint64_t timediff = ((end.tv_sec * 1000000 + end.tv_usec)
                          - (start.tv_sec * 1000000 + start.tv_usec));
      m_write_time += timediff;

      if (AM_UNLIKELY(timediff >= 200000)) {
        STAT("Write %u bytes data, takes %llu ms",
             (nbyte - remain_size),
             (timediff / 1000));
      }

      if (AM_UNLIKELY(m_write_cnt >= WRITE_STATISTICS_THRESHOLD)) {
        m_write_avg_speed = m_write_data_size * 1000000 / m_write_time / 1024;
        m_hist_min_speed = (m_write_avg_speed < m_hist_min_speed) ?
                            m_write_avg_speed : m_hist_min_speed;
        m_hist_max_speed = (m_write_avg_speed > m_hist_max_speed) ?
                            m_write_avg_speed : m_hist_max_speed;
        STAT("\nWrite %u times, total data %llu bytes, takes %llu ms"
             "\n       Average speed %llu KB/sec"
             "\nHistorical min speed %llu KB/sec"
             "\nHistorical max speed %llu KB/sec",
             WRITE_STATISTICS_THRESHOLD, m_write_data_size, m_write_time / 1000,
             m_write_avg_speed, m_hist_min_speed, m_hist_max_speed);
        m_write_data_size = 0;
        m_write_cnt = 0;
        m_write_time = 0;
      }
    }
  }

  return (retval < 0) ? retval : (nbyte - remain_size);
}

bool AMFileWriter::seek_file(uint32_t offset, uint32_t whence)
{
  bool ret = true;
  AUTO_MEM_LOCK(m_lock);
  do {
    if (AM_UNLIKELY(m_file_fd < 0)) {
      ERROR("Bad file descriptor!");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(_flush_file() != true)) {
      ERROR("Failed to sync file!");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(::lseek(m_file_fd, offset, whence) < 0)) {
      PERROR("Failed to seek");
      ret = false;
      break;
    }
  } while (0);

  return ret;
}

bool AMFileWriter::is_file_open()
{
  return (m_file_fd < 0 ? false : true);
}

uint64_t AMFileWriter::get_file_size()
{
  AUTO_MEM_LOCK(m_lock);
  struct stat fileStat;
  uint64_t size = 0;
  if (AM_UNLIKELY(fstat(m_file_fd, &fileStat) < 0)) {
    ERROR("stat error: %s", strerror(errno));
  } else {
    size = fileStat.st_size;
  }

  return size;
}
