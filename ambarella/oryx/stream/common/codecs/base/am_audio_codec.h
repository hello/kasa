/*******************************************************************************
 * am_audio_codec.h
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
#ifndef AM_AUDIO_CODEC_H_
#define AM_AUDIO_CODEC_H_

enum AudioInfoType
{
  AUDIO_INFO_ENCODE = 0,
  AUDIO_INFO_DECODE = 1,
  AUDIO_INFO_NUM
};

struct AM_AUDIO_INFO;
class AMAudioCodec: public AMIAudioCodec
{
  public:
    virtual void destroy();
    virtual const std::string& get_codec_name();
    virtual AM_AUDIO_CODEC_TYPE get_codec_type();
    virtual bool is_initialized();
    virtual bool initialize(AM_AUDIO_INFO *srcAudioInfo,
                            AM_AUDIO_CODEC_MODE mode);
    virtual bool finalize();
    virtual AM_AUDIO_INFO* get_codec_audio_info();
    virtual uint32_t get_codec_output_size();
    virtual bool get_encode_required_src_parameter(AM_AUDIO_INFO &info);
    virtual uint32_t encode(uint8_t *input,  uint32_t in_data_size,
                            uint8_t *output, uint32_t *out_data_size);
    virtual uint32_t decode(uint8_t *input,  uint32_t in_data_size,
                            uint8_t *output, uint32_t *out_data_size);

  protected:
    AMAudioCodec(AM_AUDIO_CODEC_TYPE type, const std::string& name);
    virtual ~AMAudioCodec();
    bool init();
    std::string codec_mode_string();

  protected:
    AM_AUDIO_INFO      *m_src_audio_info;
    AM_AUDIO_INFO      *m_audio_info;
    AM_AUDIO_CODEC_TYPE m_codec_type;
    AM_AUDIO_CODEC_MODE m_mode;
    bool                m_is_initialized;
    std::string         m_name;
};

#endif /* AM_AUDIO_CODEC_H_ */
