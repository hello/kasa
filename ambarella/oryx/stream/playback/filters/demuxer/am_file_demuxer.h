/*******************************************************************************
 * am_file_demuxer.h
 *
 * History:
 *   2014-8-27 - [ypchang] created file
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
#ifndef AM_FILE_DEMUXER_H_
#define AM_FILE_DEMUXER_H_

#include "am_queue.h"
#include "am_mutex.h"

enum AM_DEMUXER_MODE
{
  AM_DEMUXER_MODE_UNKNOWN,
  AM_DEMUXER_MODE_FILE,
  AM_DEMUXER_MODE_RTP,
  AM_DEMUXER_MODE_UDS
};

struct FileDemuxerConfig;
class AMEvent;
class AMSimplePacketPool;
class AMFileDemuxerObject;
class AMFileDemuxerOutput;
class AMFileDemuxerConfig;
class AMFileDemuxer: public AMPacketActiveFilter, public AMIFileDemuxer
{
    typedef AMPacketActiveFilter inherited;
    typedef AMSafeQueue<AMFileDemuxerObject*> DemuxerList;
    typedef AMSafeQueue<AMPlaybackUri> PlaybackUriQ;

  public:
    static AMIFileDemuxer* create(AMIEngine *engine, const std::string& config,
                                  uint32_t input_num, uint32_t output_num);

  public:
    virtual void* get_interface(AM_REFIID ref_iid);
    virtual void destroy();
    virtual void get_info(INFO& info);
    virtual void purge();
    virtual AMIPacketPin* get_input_pin(uint32_t index);
    virtual AMIPacketPin* get_output_pin(uint32_t index);
    virtual AM_STATE add_media(const AMPlaybackUri& uri);
    virtual AM_STATE play(AMPlaybackUri* uri = NULL);
    virtual AM_STATE start();
    virtual AM_STATE stop();
    virtual uint32_t version();

  private:
    virtual void on_run();
    AM_DEMUXER_TYPE check_media_type(const std::string& uri);
    AM_DEMUXER_TYPE check_media_type(const AMPlaybackUri& uri);
    DemuxerList* load_codecs();
    inline AMIDemuxerCodec* get_demuxer_codec(AM_DEMUXER_TYPE type,
                                              uint32_t stream_id);

  private:
    AMFileDemuxer(AMIEngine *engine);
    virtual ~AMFileDemuxer();
    AM_STATE init(const std::string& config,
                  uint32_t input_num,
                  uint32_t output_num);
    void send_packet(AMPacket *packet);

  private:
    AMFileDemuxerConfig  *m_config         = nullptr;
    FileDemuxerConfig    *m_demuxer_config = nullptr; /* No need to delete */
    AMSimplePacketPool   *m_packet_pool    = nullptr;
    AMIDemuxerCodec      *m_demuxer        = nullptr;
    AMEvent              *m_demuxer_event  = nullptr;
    DemuxerList          *m_demuxer_list   = nullptr;
    AMFileDemuxerOutput **m_output_pins    = nullptr;
    uint32_t              m_input_num      = 0;
    uint32_t              m_output_num     = 0;
    AM_DEMUXER_MODE       m_demuxer_mode   = AM_DEMUXER_MODE_UNKNOWN;
    std::atomic<bool>     m_run            = {false};
    std::atomic<bool>     m_started        = {false};
    AMMemLock             m_demuxer_lock;
    PlaybackUriQ          m_file_uri_q;
    PlaybackUriQ          m_rtp_uri_q;
    PlaybackUriQ          m_uds_uri_q;
};

class AMFileDemuxerOutput: public AMPacketOutputPin
{
    typedef AMPacketOutputPin inherited;
    friend class AMFileDemuxer;

  public:
    static AMFileDemuxerOutput* create(AMPacketFilter *filter)
    {
      return (new AMFileDemuxerOutput(filter));
    }

  private:
    AMFileDemuxerOutput(AMPacketFilter *filter) :
      inherited(filter){}
    virtual ~AMFileDemuxerOutput(){}
};
#endif /* AM_FILE_DEMUXER_H_ */
