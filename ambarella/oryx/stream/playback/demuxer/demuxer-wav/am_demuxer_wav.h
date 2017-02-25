/*******************************************************************************
 * am_demuxer_wav.h
 *
 * History:
 *   2014-11-10 - [ypchang] created file
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
#ifndef AM_DEMUXER_WAV_H_
#define AM_DEMUXER_WAV_H_
struct AM_AUDIO_INFO;
class AMDemuxerWav: public AMDemuxer
{
    typedef AMDemuxer inherited;

    enum WAV_AUDIO_FORMAT {
      WAV_FORMAT_LPCM       = 0x0001,
      WAV_FORMAT_G726       = 0x0045,
      WAV_FORMAT_MS_ALAW    = 0x0006,
      WAV_FORMAT_MS_ULAW    = 0x0007,
    };

  public:
    static AMIDemuxerCodec* create(uint32_t streamid);

  public:
    virtual bool is_drained();
    virtual AM_DEMUXER_STATE get_packet(AMPacket *&packet);
    virtual void destroy();

  private:
    AMDemuxerWav(uint32_t streamid);
    virtual ~AMDemuxerWav();
    AM_STATE init();

  private:
    bool wav_file_parser(AM_AUDIO_INFO& audioInfo,
                         AMFile& wav,
                         int64_t &data_size);

  private:
    int64_t             m_data_chunk_size  = 0ULL;
    uint32_t            m_read_data_size   = 0;
    AM_AUDIO_CODEC_TYPE m_audio_codec_type = AM_AUDIO_CODEC_NONE;
    bool                m_is_new_file      = true;
};



#endif /* AM_DEMUXER_WAV_H_ */
