/*******************************************************************************
 * am_demuxer_wav.h
 *
 * History:
 *   2014-11-10 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
    virtual AM_DEMUXER_STATE get_packet(AMPacket *&packet);
    virtual void destroy();

  private:
    AMDemuxerWav(uint32_t streamid);
    virtual ~AMDemuxerWav();
    AM_STATE init();

  private:
    bool wav_file_parser(AM_AUDIO_INFO& audioInfo, AMFile& wav);

  private:
    bool                m_is_new_file;
    AM_AUDIO_CODEC_TYPE m_audio_codec_type;
    uint32_t            m_read_data_size;
};



#endif /* AM_DEMUXER_WAV_H_ */
