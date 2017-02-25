/*******************************************************************************
 * am_demuxer_codec.h
 *
 * History:
 *   2014-9-1 - [ypchang] created file
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
#ifndef AM_DEMUXER_CODEC_H_
#define AM_DEMUXER_CODEC_H_
#include "am_queue.h"
#include "am_mutex.h"

class AMFile;
typedef AMSafeQueue<AMFile*> FileQueue;

class AMDemuxer: public AMIDemuxerCodec
{
  public:
    virtual AM_DEMUXER_TYPE get_demuxer_type();
    virtual void destroy();
    virtual void enable(bool enabled);
    virtual bool is_play_list_empty();
    virtual bool is_drained();
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
    uint64_t        m_file_size;
    AMFile         *m_media;
    FileQueue      *m_play_list;
    AMIPacketPool  *m_packet_pool;
    AM_DEMUXER_TYPE m_demuxer_type;
    uint32_t        m_contents;
    uint32_t        m_stream_id;
    bool            m_is_enabled;
    AMMemLock       m_lock;
};

#endif /* AM_DEMUXER_CODEC_H_ */
