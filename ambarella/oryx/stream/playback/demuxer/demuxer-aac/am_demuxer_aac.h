/*******************************************************************************
 * am_demuxer_aac.h
 *
 * History:
 *   2014-10-27 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_DEMUXER_AAC_H_
#define AM_DEMUXER_AAC_H_


class AMDemuxerAac: public AMDemuxer
{
    typedef AMDemuxer inherited;
  public:
    static AMIDemuxerCodec* create(uint32_t streamid);

  public:
    virtual AM_DEMUXER_STATE get_packet(AMPacket *&packet);
    virtual void destroy();

  private:
    AMDemuxerAac(uint32_t streamid);
    virtual ~AMDemuxerAac();
    AM_STATE init();

  private:
    char    *m_buffer;
    uint32_t m_sent_size;
    uint32_t m_avail_size;
    uint64_t m_remain_size;
    bool     m_need_to_read;
    bool     m_is_new_file;
};

#endif /* AM_DEMUXER_AAC_H_ */
