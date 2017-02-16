/*******************************************************************************
 * am_file_demuxer.h
 *
 * History:
 *   2014-8-27 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_FILE_DEMUXER_H_
#define AM_FILE_DEMUXER_H_

#include <queue>

struct FileDemuxerConfig;
class AMEvent;
class AMSpinLock;
class AMSimplePacketPool;
class AMFileDemuxerObject;
class AMFileDemuxerOutput;
class AMFileDemuxerConfig;
class AMFileDemuxer: public AMPacketActiveFilter, public AMIFileDemuxer
{
    typedef AMPacketActiveFilter inherited;
    typedef std::queue<AMFileDemuxerObject*> DemuxerList;

  public:
    static AMIFileDemuxer* create(AMIEngine *engine, const std::string& config,
                                  uint32_t input_num, uint32_t output_num);

  public:
    virtual void* get_interface(AM_REFIID ref_iid);
    virtual void destroy();
    virtual void get_info(INFO& info);
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
    AMFileDemuxerConfig  *m_config;
    FileDemuxerConfig    *m_demuxer_config; /* No need to delete */
    AMSimplePacketPool   *m_packet_pool;
    AMIDemuxerCodec      *m_demuxer;
    AMSpinLock           *m_demuxer_lock;
    AMEvent              *m_demuxer_event;
    DemuxerList          *m_demuxer_list;
    AMFileDemuxerOutput **m_output_pins;
    uint32_t              m_input_num;
    uint32_t              m_output_num;
    bool                  m_run;
    bool                  m_paused;
    bool                  m_started;
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
