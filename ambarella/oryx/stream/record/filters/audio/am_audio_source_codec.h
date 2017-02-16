/*******************************************************************************
 * am_audio_source_codec.h
 *
 * History:
 *   2014-12-8 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_SOURCE_CODEC_H_
#define AM_AUDIO_SOURCE_CODEC_H_

#include <queue>
#include "am_audio_define.h"

class AMPlugin;
class AMthread;
class AMIAudioCodec;
class AMAudioSource;
class AMFixedPacketPool;

class AMAudioCodecObj
{
    typedef std::queue<AMPacket*> PacketQ;

    friend class AMAudioSource;

  public:
    AMAudioCodecObj();
    virtual ~AMAudioCodecObj();

  public:
    bool is_valid();
    bool is_running();
    bool load_codec(std::string& name);
    bool initialize(AM_AUDIO_INFO *src_audio_info,
                    uint32_t id,
                    uint32_t packet_pool_size);
    void finalize();
    void push_queue(AMPacket *&packet);
    void clear();
    bool start(bool RTPrio, uint32_t priority);
    void stop();

  private:
    static void static_encode(void *data);
    void encode();

  private:
    AMIAudioCodec     *m_codec;
    AMPlugin          *m_plugin;
    AMFixedPacketPool *m_pool;
    AM_AUDIO_INFO     *m_codec_info;
    AMThread          *m_thread;
    AMAudioSource     *m_audio_source;
    AM_AUDIO_INFO      m_codec_required_info;
    int64_t            m_last_sent_pts;
    uint32_t           m_id;
    uint16_t           m_codec_type;
    bool               m_run;
    std::string        m_name;
    PacketQ            m_packet_q;
};

#endif /* AM_AUDIO_SOURCE_CODEC_H_ */
