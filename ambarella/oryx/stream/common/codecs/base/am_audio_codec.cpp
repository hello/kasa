/*******************************************************************************
 * am_audio_codec.cpp
 *
 * History:
 *   2014-9-24 - [ypchang] created file
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

#include "am_amf_types.h"
#include "am_audio_define.h"
#include "am_audio_codec_if.h"
#include "am_audio_codec.h"

void AMAudioCodec::destroy()
{
  delete this;
}

const std::string& AMAudioCodec::get_codec_name()
{
  return m_name;
}

AM_AUDIO_CODEC_TYPE AMAudioCodec::get_codec_type()
{
  return m_codec_type;
}

bool AMAudioCodec::is_initialized()
{
  return m_is_initialized;
}

bool AMAudioCodec::initialize(AM_AUDIO_INFO *srcAudioInfo,
                              AM_AUDIO_CODEC_MODE mode)
{
  ERROR("Dummy audio codec, should be implemented in sub-class!");
  return false;
}

bool AMAudioCodec::finalize()
{
  ERROR("Dummy audio codec, should be implemented in sub-class!");
  return false;
}

AM_AUDIO_INFO* AMAudioCodec::get_codec_audio_info()
{
  ERROR("Dummy audio codec, should be implemented in sub-class!");
  return NULL;
}

uint32_t AMAudioCodec::get_codec_output_size()
{
  ERROR("Dummy audio codec, should be implemented in sub-class!");
  return 0;
}

bool AMAudioCodec::get_encode_required_src_parameter(AM_AUDIO_INFO &info)
{
  ERROR("Dummy audio codec, should be implemented in sub-class!");
  return false;
}

uint32_t AMAudioCodec::encode(uint8_t *input,  uint32_t in_data_size,
                              uint8_t *output, uint32_t *out_data_size)
{
  ERROR("Dummy audio codec, should be implemented in sub-class!");
  return (uint32_t)-1;
}

uint32_t AMAudioCodec::decode(uint8_t *input,  uint32_t in_data_size,
                              uint8_t *output, uint32_t *out_data_size)
{
  ERROR("Dummy audio codec, should be implemented in sub-class!");
  return (uint32_t)-1;
}

AMAudioCodec::AMAudioCodec(AM_AUDIO_CODEC_TYPE type, const std::string& name) :
    m_src_audio_info(nullptr),
    m_audio_info(nullptr),
    m_codec_type(type),
    m_mode(AM_AUDIO_CODEC_MODE_NONE),
    m_is_initialized(false),
    m_name(name)
{}

AMAudioCodec::~AMAudioCodec()
{
  delete[] m_audio_info;
  delete   m_src_audio_info;
}

bool AMAudioCodec::init()
{
  bool ret = true;
  do {
    m_src_audio_info = new AM_AUDIO_INFO;
    if (AM_UNLIKELY(!m_src_audio_info)) {
      ERROR("Failed to create AM_AUDIO_INFO object!");
      ret = false;
      break;
    }
    memset(m_src_audio_info, 0, sizeof(*m_src_audio_info));

    m_audio_info = new AM_AUDIO_INFO[AUDIO_INFO_NUM];
    if (AM_UNLIKELY(!m_audio_info)) {
      ERROR("Failed to allocate memory for AM_AUDIO_INFO!");
      ret = false;
      break;
    }

  } while(0);

  return ret;
}

std::string AMAudioCodec::codec_mode_string()
{
  std::string mode = "Unknown";
  switch(m_mode)
  {
    case AM_AUDIO_CODEC_MODE_NONE:   mode = "None";   break;
    case AM_AUDIO_CODEC_MODE_ENCODE: mode = "Encode"; break;
    case AM_AUDIO_CODEC_MODE_DECODE: mode = "Decode"; break;
    default: break;
  }
  return mode;
}
