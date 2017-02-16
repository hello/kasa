/*******************************************************************************
 * am_demuxer_ogg.h
 *
 * History:
 *   2014-11-11 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_DEMUXER_OGG_H_
#define AM_DEMUXER_OGG_H_

class Ogg;
class AMDemuxerOgg: public AMDemuxer
{
    typedef AMDemuxer inherited;

  public:
    static AMIDemuxerCodec* create(uint32_t streamid);

  public:
    virtual AM_DEMUXER_STATE get_packet(AMPacket *&packet);
    virtual void destroy();
    virtual void enable(bool enable);

  private:
    AMDemuxerOgg(uint32_t streamid);
    virtual ~AMDemuxerOgg();
    AM_STATE init();

  private:
    bool                m_is_new_file;
    AM_AUDIO_CODEC_TYPE m_audio_codec_type;
    Ogg                *m_ogg;
};

#endif /* AM_DEMUXER_OGG_H_ */
