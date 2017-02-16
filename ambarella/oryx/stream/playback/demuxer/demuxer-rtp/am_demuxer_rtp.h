/*******************************************************************************
 * am_demuxer_rtp.h
 *
 * History:
 *   2014-11-16 - [ccjing] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_DEMUXER_RTP_H_
#define AM_DEMUXER_RTP_H_

#include "am_audio_type.h"
typedef std::queue<AMPacket*> packet_queue;
struct AMPlaybackRtpUri;

class AMDemuxerRtp: public AMDemuxer
{
    typedef AMDemuxer inherited;
  public:
    static AMIDemuxerCodec* create(uint32_t streamid);

  public:
    virtual void destroy();
    virtual void enable(bool enabled);//rewrite enable
    virtual bool is_play_list_empty();//rewrite is_play_list_empty
    virtual bool add_uri(const AMPlaybackUri& uri);//rewrite add_uri
    virtual AM_DEMUXER_STATE get_packet(AMPacket *&packet);

    void stop();
    bool add_udp_port(const AMPlaybackRtpUri& rtp_uri); /*create socket and thread*/
    static void thread_entry(void *p);
    void main_loop();

  protected:
    AMDemuxerRtp(uint32_t streamid);
    virtual ~AMDemuxerRtp();
    AM_STATE init();
    std::string sample_format_to_string(AM_AUDIO_SAMPLE_FORMAT format);
    std::string audio_type_to_string(AM_AUDIO_TYPE type);

  private:
    AM_AUDIO_INFO      *m_audio_info;
    AMSpinLock         *m_sock_lock;
    AMSpinLock         *m_packet_queue_lock;
    char               *m_recv_buf;
    AMThread           *m_thread;//create in the add_udp_port
    packet_queue       *m_packet_queue;
    int                 m_sock_fd;
    uint32_t            m_packet_count;
    bool                m_run;/* running flag of the thread*/
    bool                m_is_new_sock;
    bool                m_is_info_ready;
    uint8_t             m_audio_rtp_type;/*parse rtp packet header*/
    AM_AUDIO_CODEC_TYPE m_audio_codec_type; /*used in packet->set_frame_type()*/
};



#endif /* AM_DEMUXER_RTP_H_ */
