/*******************************************************************************
 * am_file_demuxer_object.h
 *
 * History:
 *   2014-9-5 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_FILE_DEMUXER_OBJECT_H_
#define AM_FILE_DEMUXER_OBJECT_H_

class AMPlugin;
class AMFileDemuxerObject
{
    friend class AMIDemuxerCodec;
    friend class AMFileDemuxer;
  public:
    AMFileDemuxerObject(const std::string& demuxer);
    virtual ~AMFileDemuxerObject();

  public:
    bool open();
    void close();

  private:
    AMPlugin       *m_plugin;
    std::string     m_demuxer;
    AM_DEMUXER_TYPE m_type;
    DemuxerCodecNew get_demuxer_codec;
};

#endif /* AM_FILE_DEMUXER_OBJECT_H_ */
