/*******************************************************************************
 * am_io.cpp
 *
 * History:
 *   2016年9月26日 - [ypchang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#include "am_io.h"
#include <limits.h>

ssize_t am_read(int fd,
                void *buf,
                size_t read_len,
                uint32_t retry_count)
{
  size_t read_size = 0;
  size_t size = (read_len > SSIZE_MAX) ? SSIZE_MAX : read_len;
  uint32_t count = 0;
  bool need_break = false;

  if (AM_UNLIKELY(size < read_len)) {
    WARN("read length is greater than SSIZE_MAX, will read SSIZE_MAX only!");
  }
  do {
    ssize_t read_ret = ::read(fd,
                              (((uint8_t*)buf) + read_size),
                              size - read_size);
    if (AM_LIKELY(read_ret >= 0)) {
      read_size += read_ret;
    } else {
      switch(errno) {
        case EAGAIN:
        case EINTR: break;
        default: {
          need_break = true;
        }break;
      }
    }
    ++ count;
  } while((read_size < size) && !need_break && (count < retry_count));

  if (AM_UNLIKELY(read_size != size)) {
    WARN("Failed to read data, tried %u %s, "
         "%u bytes are read, %u bytes left!",
         count, ((count > 1) ? "times" : "time"),
         read_size, (size - read_size));
  }

  return need_break ? -1 : read_size;
}


ssize_t am_write(int fd,
                 const void *data,
                 size_t data_len,
                 uint32_t retry_count)
{
  size_t write_size = 0;
  uint32_t count = 0;
  bool need_break = false;
  do {
    ssize_t write_ret = ::write(fd,
                                (((uint8_t*)data) + write_size),
                                data_len - write_size);
    if (AM_LIKELY(write_ret >= 0)) {
      write_size += write_ret;
    } else {
      switch(errno) {
        case EAGAIN:
        case EINTR: break;
        default: {
          need_break = true;
        }break;
      }
    }
    ++ count;
  } while((write_size < data_len) && !need_break && (count < retry_count));

  if (AM_UNLIKELY(write_size != data_len)) {
    WARN("Failed to write data, tried %u %s, "
         "%u bytes are written, %u bytes left!",
         count, ((count > 1) ? "times" : "time"),
         write_size, (data_len - write_size));
  }

  return need_break ? -1 : write_size;
}
