/*******************************************************************************
 * am_muxer_codec_obj.cpp
 *
 * History:
 *   2014-12-29 - [ypchang] created file
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

#include "am_activemuxer_codec_obj.h"
#include "am_muxer_codec_if.h"

#include "am_plugin.h"

AMActiveMuxerCodecObj::AMActiveMuxerCodecObj() :
  m_codec(NULL),
  m_plugin(NULL)
{
  m_name.clear();
}

AMActiveMuxerCodecObj::~AMActiveMuxerCodecObj()
{
  destroy();
}

bool AMActiveMuxerCodecObj::is_valid()
{
  return (NULL != m_codec);
}

bool AMActiveMuxerCodecObj::load_codec(std::string& name)
{
  bool ret = false;
  std::string codec = ORYX_MUXER_DIR;
  codec.append("/muxer-").append(name).append(".so");

  m_plugin = AMPlugin::create(codec.c_str());

  if (AM_LIKELY(m_plugin)) {
    MuxerCodecNew get_muxer_codec =
        (MuxerCodecNew)m_plugin->get_symbol(MUXER_CODEC_NEW);
    if (AM_LIKELY(get_muxer_codec)) {
      std::string codecConf = ORYX_MUXER_CONF_DIR;
      codecConf.append("/muxer-").append(name).append(".acs");

      m_codec = get_muxer_codec(codecConf.c_str());
      if (AM_UNLIKELY(!m_codec)) {
        ERROR("Failed to load muxer %s", name.c_str());
      } else {
        NOTICE("Muxer plugin %s is loaded!", name.c_str());
        m_name = name;
        ret = true;
      }
    } else {
      ERROR("Failed to get symbol (%s) from %s!",
            MUXER_CODEC_NEW, codec.c_str());
    }
  }

  if (AM_UNLIKELY(!ret)) {
    destroy();
  }

  return ret;
}

void AMActiveMuxerCodecObj::destroy()
{
  if (AM_LIKELY(m_codec)) {
    m_codec->stop();
  }
  m_name.clear();
  delete m_codec;
  m_codec = NULL;
  AM_DESTROY(m_plugin);
}
