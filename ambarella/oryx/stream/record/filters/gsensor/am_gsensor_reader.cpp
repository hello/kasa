/*******************************************************************************
 * am_gsensor_reader.cpp
 *
 * History:
 *   2016-11-28 - [ccjing] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#include "am_gsensor_reader.h"


AMGsensorReader* AMGsensorReader::create(std::string dev_path)
{
  AMGsensorReader *result = nullptr;
  result = new AMGsensorReader();
  if (result && (!result->init(dev_path))) {
    ERROR("Failed to init AMGsensorReader");
    delete result;
    result = nullptr;
  }
  return result;
}

bool AMGsensorReader::query_gsensor_frame(uint8_t *data, uint32_t& size)
{
  bool ret = true;
  uint32_t read_size= 0;
  do {
    if (m_fd < 0) {
      ERROR("The gsensor device is not opened.");
      ret = false;
      break;
    }
    read_size = read(m_fd, data, size);
    while ((read_size < 0) && ((errno == EAGAIN) || (errno == EINTR))) {
      read_size = read(m_fd, data, size);
    }
    if (read_size < 0) {
      ERROR("Read gsensor data error : %s", strerror(errno));
      ret = false;
      break;
    }
    size = read_size;
  } while(0);
  return ret;
}

int32_t AMGsensorReader::get_gsensor_fd()
{
  return m_fd;
}

void AMGsensorReader::destroy()
{
  delete this;
}

AMGsensorReader::AMGsensorReader()
{}

AMGsensorReader::~AMGsensorReader()
{
  if (m_fd >= 0) {
    close_gsensor();
  }
}

void AMGsensorReader::close_gsensor()
{
  do {
    if (m_fd < 0) {
      NOTICE("gsensor device have been closed.");
      break;
    }
    if (close(m_fd) < 0) {
      ERROR("Failed to close gsensor device:  %s", strerror(errno));
    }
    m_fd = -1;
  } while(0);
  return;
}

bool AMGsensorReader::init(std::string dev_path)
{
  bool ret = true;
  do {
    m_fd = open(dev_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (m_fd < 0) {
      ERROR("Failed to open %s : %s", dev_path.c_str(), strerror(errno));
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

