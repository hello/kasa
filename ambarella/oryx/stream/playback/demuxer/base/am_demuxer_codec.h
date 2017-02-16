/*******************************************************************************
 * am_demuxer_codec.h
 *
 * History:
 *   2014-9-1 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_DEMUXER_CODEC_H_
#define AM_DEMUXER_CODEC_H_
#include <queue>

class AMFile;
class AMSpinLock;
typedef std::queue<AMFile*> FileQueue;

class AMDemuxer: public AMIDemuxerCodec
{
  public:
    virtual AM_DEMUXER_TYPE get_demuxer_type();
    virtual void destroy();
    virtual void enable(bool enabled);
    virtual bool is_play_list_empty();
    virtual uint32_t stream_id();
    virtual uint32_t get_contents_type();
    virtual bool add_uri(const AMPlaybackUri& uri);
    virtual AM_DEMUXER_STATE get_packet(AMPacket *&packet);

  protected:
    AMDemuxer(AM_DEMUXER_TYPE type, uint32_t streamid);
    virtual ~AMDemuxer();
    virtual AM_STATE init();

  protected:
    uint32_t available_packet_num();
    bool allocate_packet(AMPacket*& packet);
    AMFile* get_new_file();

  protected:
    AM_DEMUXER_TYPE m_demuxer_type;
    uint32_t        m_contents;
    uint32_t        m_stream_id;
    uint64_t        m_file_size;
    bool            m_is_enabled;
    AMFile         *m_media;
    AMSpinLock     *m_lock;
    FileQueue      *m_play_list;
    AMIPacketPool  *m_packet_pool;
};

#endif /* AM_DEMUXER_CODEC_H_ */
