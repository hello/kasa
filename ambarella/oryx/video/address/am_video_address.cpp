/**
 * am_video_address.cpp
 *
 *  History:
 *    Aug 11, 2015 - [Shupeng Ren] created file
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
 */

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_video_types.h"
#include "am_platform_if.h"
#include "am_video_address_if.h"
#include "am_video_address.h"

#define AUTO_LOCK_VADDR(mtx) std::lock_guard<std::recursive_mutex> lck(mtx)

AMVideoAddress *AMVideoAddress::m_instance = nullptr;
std::recursive_mutex AMVideoAddress::m_mtx;

AMIVideoAddressPtr AMIVideoAddress::get_instance()
{
  return AMVideoAddress::get_instance();
}

AMVideoAddress* AMVideoAddress::get_instance()
{
  AUTO_LOCK_VADDR(m_mtx);
  if (AM_LIKELY(!m_instance)) {
    m_instance = AMVideoAddress::create();
  }

  return m_instance;
}

void AMVideoAddress::inc_ref()
{
  ++ m_ref_cnt;
}

void AMVideoAddress::release()
{
  AUTO_LOCK_VADDR(m_mtx);
  if (AM_LIKELY((m_ref_cnt > 0) && (--m_ref_cnt <= 0))) {
    delete m_instance;
    m_instance = nullptr;
  }
}

AMVideoAddress* AMVideoAddress::create()
{
  AMVideoAddress *result = new AMVideoAddress();
  if (result && (AM_RESULT_OK != result->init())) {
    delete result;
    result = nullptr;
  }
  return result;
}

AM_RESULT AMVideoAddress::init()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!(m_platform = AMIPlatform::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMIPlatform!");
      break;
    }
    if ((result = map_dsp()) != AM_RESULT_OK) {
      break;
    }
    if ((result = map_bsb()) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AMVideoAddress::AMVideoAddress() :
    m_ref_cnt(0)
{

}

AMVideoAddress::~AMVideoAddress()
{
  unmap_dsp();
  unmap_bsb();
  unmap_usr();
  m_platform = nullptr;
}

AM_RESULT AMVideoAddress::addr_get(AM_DATA_FRAME_TYPE type,
                                   uint32_t offset,
                                   AMAddress &addr)
{
  AM_RESULT result = AM_RESULT_OK;
  AM_DSP_SUB_BUF_ID id = AM_DSP_SUB_BUF_INVALID;
  AMMemMapInfo mem;

  do {
    switch (type) {
      case AM_DATA_FRAME_TYPE_VIDEO:
        addr.data = m_bsb_mem.addr + offset;
        addr.max_size = m_bsb_mem.length;
        break;
      case AM_DATA_FRAME_TYPE_YUV:
      case AM_DATA_FRAME_TYPE_RAW:
      case AM_DATA_FRAME_TYPE_ME0:
      case AM_DATA_FRAME_TYPE_ME1:
      case AM_DATA_FRAME_TYPE_VCA:
        if ((result=m_platform->get_dsp_mmap_info(id, mem)) != AM_RESULT_OK) {
          ERROR("Failed to get dsp buffer address!");
          addr.data = nullptr;
          addr.max_size = 0;
          break;
        }
        addr.data = mem.addr + offset;
        addr.max_size = mem.length - offset;
        break;
      default:
        addr.data = nullptr;
        addr.max_size = 0;
        result = AM_RESULT_ERR_INVALID;
        ERROR("Invalid data frame type: %d!", type);
        break;
    }
  } while (0);

  return result;
}

uint8_t* AMVideoAddress::video_addr_get(uint32_t video_data_offset)
{
  return m_bsb_mem.addr + video_data_offset;
}

uint8_t* AMVideoAddress::pic_addr_get(uint32_t pic_data_offset)
{
  uint8_t *addr = nullptr;
  AM_DSP_SUB_BUF_ID id = AM_DSP_SUB_BUF_INVALID;
  AMMemMapInfo mem;
  do {
    if (m_platform->get_dsp_mmap_info(id, mem) != AM_RESULT_OK) {
      ERROR("Failed to get dsp buffer address!");
      break;
    }
    addr = mem.addr + pic_data_offset;
  } while (0);

  return addr;
}

AM_RESULT AMVideoAddress::video_addr_get(const AMQueryFrameDesc &desc,
                                         AMAddress &addr)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (desc.type != AM_DATA_FRAME_TYPE_VIDEO) {
      ERROR("Data frame: %d is wrong!", desc.type);
      result = AM_RESULT_ERR_INVALID;
    }
    addr.data = m_bsb_mem.addr + desc.video.data_offset;
    addr.max_size = desc.video.data_size;
  } while (0);
  return result;
}

AM_RESULT AMVideoAddress::yuv_y_addr_get(const AMQueryFrameDesc &desc,
                                         AMAddress &addr)
{
  AM_RESULT result = AM_RESULT_OK;
  AM_DSP_SUB_BUF_ID id = AM_DSP_SUB_BUF_INVALID;
  AMMemMapInfo mem;
  do {
    if (desc.type != AM_DATA_FRAME_TYPE_YUV) {
      ERROR("Data frame: %d is wrong!", desc.type);
      result = AM_RESULT_ERR_INVALID;
    }

    switch (desc.yuv.buffer_id) {
      case AM_SOURCE_BUFFER_MAIN:
        id = AM_DSP_SUB_BUF_MAIN_YUV;
        break;
      case AM_SOURCE_BUFFER_2ND:
        id = AM_DSP_SUB_BUF_2ND_YUV;
        break;
      case AM_SOURCE_BUFFER_3RD:
        id = AM_DSP_SUB_BUF_3RD_YUV;
        break;
      case AM_SOURCE_BUFFER_4TH:
        id = AM_DSP_SUB_BUF_4TH_YUV;
        break;
      default:
        WARN("Don't support get buffer%d yuv addr currently!",desc.yuv.buffer_id);
        result = AM_RESULT_ERR_INVALID;
        break;
    }
    if ((result = m_platform->get_dsp_mmap_info(id, mem)) != AM_RESULT_OK) {
      ERROR("Failed to get dsp buffer address!");
      break;
    }

    addr.data = mem.addr + desc.yuv.y_offset;
    addr.max_size = mem.length;
  } while (0);

  return result;
}

AM_RESULT AMVideoAddress::yuv_uv_addr_get(const AMQueryFrameDesc &desc,
                                          AMAddress &addr)
{
  AM_RESULT result = AM_RESULT_OK;
  AM_DSP_SUB_BUF_ID id = AM_DSP_SUB_BUF_INVALID;
  AMMemMapInfo mem;
  do {
    if (desc.type != AM_DATA_FRAME_TYPE_YUV) {
      ERROR("Data frame: %d is wrong!", desc.type);
      result = AM_RESULT_ERR_INVALID;
    }

    switch (desc.yuv.buffer_id) {
      case AM_SOURCE_BUFFER_MAIN:
        id = AM_DSP_SUB_BUF_MAIN_YUV;
        break;
      case AM_SOURCE_BUFFER_2ND:
        id = AM_DSP_SUB_BUF_2ND_YUV;
        break;
      case AM_SOURCE_BUFFER_3RD:
        id = AM_DSP_SUB_BUF_3RD_YUV;
        break;
      case AM_SOURCE_BUFFER_4TH:
        id = AM_DSP_SUB_BUF_4TH_YUV;
        break;
      default:
        WARN("Don't support get buffer%d yuv addr currently!",desc.yuv.buffer_id);
        result = AM_RESULT_ERR_INVALID;
        break;
    }
    if ((result = m_platform->get_dsp_mmap_info(id, mem)) != AM_RESULT_OK) {
      ERROR("Failed to get dsp buffer address!");
      break;
    }

    addr.data = mem.addr + desc.yuv.uv_offset;
    addr.max_size = mem.length;
  } while (0);

  return result;
}

AM_RESULT AMVideoAddress::raw_addr_get(const AMQueryFrameDesc &desc,
                                       AMAddress &addr)
{
  AM_RESULT result = AM_RESULT_OK;
  AM_DSP_SUB_BUF_ID id = AM_DSP_SUB_BUF_INVALID;
  AMMemMapInfo mem;
  do {
    if (desc.type != AM_DATA_FRAME_TYPE_RAW) {
      ERROR("Data frame: %d is wrong!", desc.type);
      result = AM_RESULT_ERR_INVALID;
    }

    id = AM_DSP_SUB_BUF_RAW;
    if ((result = m_platform->get_dsp_mmap_info(id, mem)) != AM_RESULT_OK) {
      ERROR("Failed to get dsp buffer address!");
      break;
    }

    addr.data = mem.addr + desc.raw.data_offset;
    addr.max_size = mem.length;
  } while (0);

  return result;
}

AM_RESULT AMVideoAddress::me0_addr_get(const AMQueryFrameDesc &desc,
                                       AMAddress &addr)
{
  AM_RESULT result = AM_RESULT_OK;
  AM_DSP_SUB_BUF_ID id = AM_DSP_SUB_BUF_INVALID;
  AMMemMapInfo mem;
  do {
    if (desc.type != AM_DATA_FRAME_TYPE_ME0) {
      ERROR("Data frame: %d is wrong!", desc.type);
      result = AM_RESULT_ERR_INVALID;
    }

    switch (desc.me.buffer_id) {
      case AM_SOURCE_BUFFER_MAIN:
        id = AM_DSP_SUB_BUF_MAIN_ME;
        break;
      case AM_SOURCE_BUFFER_2ND:
        id = AM_DSP_SUB_BUF_2ND_ME;
        break;
      case AM_SOURCE_BUFFER_3RD:
        id = AM_DSP_SUB_BUF_3RD_ME;
        break;
      case AM_SOURCE_BUFFER_4TH:
        id = AM_DSP_SUB_BUF_4TH_ME;
        break;
      default:
        WARN("Don't support get buffer%d me addr currently!",desc.me.buffer_id);
        result = AM_RESULT_ERR_INVALID;
        break;
    }
    if ((result = m_platform->get_dsp_mmap_info(id, mem)) != AM_RESULT_OK) {
      ERROR("Failed to get dsp buffer address!");
      break;
    }

    addr.data = mem.addr + desc.me.data_offset;
    addr.max_size = mem.length;
  } while (0);

  return result;
}

AM_RESULT AMVideoAddress::me1_addr_get(const AMQueryFrameDesc &desc,
                                       AMAddress &addr)
{
  AM_RESULT result = AM_RESULT_OK;
  AM_DSP_SUB_BUF_ID id = AM_DSP_SUB_BUF_INVALID;
  AMMemMapInfo mem;
  do {
    if (desc.type != AM_DATA_FRAME_TYPE_ME1) {
      ERROR("Data frame: %d is wrong!", desc.type);
      result = AM_RESULT_ERR_INVALID;
    }

    switch (desc.me.buffer_id) {
      case AM_SOURCE_BUFFER_MAIN:
        id = AM_DSP_SUB_BUF_MAIN_ME;
        break;
      case AM_SOURCE_BUFFER_2ND:
        id = AM_DSP_SUB_BUF_2ND_ME;
        break;
      case AM_SOURCE_BUFFER_3RD:
        id = AM_DSP_SUB_BUF_3RD_ME;
        break;
      case AM_SOURCE_BUFFER_4TH:
        id = AM_DSP_SUB_BUF_4TH_ME;
        break;
      default:
        WARN("Don't support get buffer%d me addr currently!",desc.me.buffer_id);
        result = AM_RESULT_ERR_INVALID;
        break;
    }
    if ((result = m_platform->get_dsp_mmap_info(id, mem)) != AM_RESULT_OK) {
      ERROR("Failed to get dsp buffer address!");
      break;
    }

    addr.data = mem.addr + desc.me.data_offset;
    addr.max_size = mem.length;
  } while (0);
  return result;
}

bool AMVideoAddress::is_new_video_session(uint32_t session_id,
                                          AM_STREAM_ID stream_id)
{
  bool ret = false;
  if (m_stream_session_id.find(stream_id) != m_stream_session_id.end()) {
    if (session_id != m_stream_session_id[stream_id]) {
      ret = true;
      m_stream_session_id[stream_id] = session_id;
    }
  } else {
    ret = true;
    m_stream_session_id[stream_id] = session_id;
  }
  return ret;
}

AM_RESULT AMVideoAddress::map_bsb()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if ((result = m_platform->map_bsb(m_bsb_mem)) != AM_RESULT_OK) {
      ERROR("Failed to map bsb!");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoAddress::unmap_bsb()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if ((result = m_platform->unmap_bsb()) != AM_RESULT_OK) {
      ERROR("Failed to unmap bsb!");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoAddress::map_dsp()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if ((result = m_platform->map_dsp()) != AM_RESULT_OK) {
      ERROR("Failed to map dsp!");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoAddress::unmap_dsp()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if ((result = m_platform->unmap_dsp()) != AM_RESULT_OK) {
      ERROR("Failed to unmap dsp!");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoAddress::map_usr()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if ((result = m_platform->map_usr(m_usr_mem)) != AM_RESULT_OK) {
      ERROR("Failed to map usr!");
      break;
    }
  } while (0);

  return result;
}


AM_RESULT AMVideoAddress::unmap_usr()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if ((result = m_platform->unmap_usr()) != AM_RESULT_OK) {
      ERROR("Failed to unmap usr!");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVideoAddress::usr_addr_get(AMAddress &usr)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if ((result = map_usr()) != AM_RESULT_OK) {
      ERROR("Failed to map usr!");
      break;
    }
    usr.data = m_usr_mem.addr;
    usr.max_size = m_usr_mem.length;
  } while (0);

  return result;
}
