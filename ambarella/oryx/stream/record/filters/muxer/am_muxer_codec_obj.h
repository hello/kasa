/*******************************************************************************
 * am_muxer_codec_obj.h
 *
 * History:
 *   2014-12-29 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_CODEC_OBJ_H_
#define ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_CODEC_OBJ_H_

class AMMuxer;
class AMPlugin;
class AMIMuxerCodec;

class AMMuxerCodecObj
{
    friend class AMMuxer;

  public:
    AMMuxerCodecObj();
    virtual ~AMMuxerCodecObj();

  public:
    bool is_valid();
    bool load_codec(std::string& name);

  private:
    void destroy();

  private:
    AMIMuxerCodec *m_codec;
    AMPlugin      *m_plugin;
    std::string    m_name;
};

#ifdef BUILD_AMBARELLA_ORYX_CODEC_DIR
#define ORYX_MUXER_DIR ((const char*)BUILD_AMBARELLA_ORYX_MUXER_DIR)
#else
#define ORYX_MUXER_DIR ((const char*)"/usr/lib/oryx/muxer")
#endif

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define ORYX_MUXER_CONF_DIR \
  ((const char*)BUILD_AMBARELLA_ORYX_CONF_DIR"/stream/muxer")
#else
#define ORYX_MUXER_CONF_DIR ((const char*)"/etc/oryx/stream/muxer")
#endif

#endif /* ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_CODEC_OBJ_H_ */
