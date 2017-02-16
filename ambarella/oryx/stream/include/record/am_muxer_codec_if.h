/*******************************************************************************
 * am_muxer_if.h
 *
 * History:
 *   2014-12-23 - [Chengcai Jing] created file
 *
 * Copyright (C) 2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_MUXER_CODEC_IF_H_
#define AM_MUXER_CODEC_IF_H_
#include "am_muxer_codec_info.h"
class AMPacket;

struct AMMuxerCodecConfig;

enum AM_MUXER_CODEC_STATE {
  AM_MUXER_CODEC_ERROR = -1, //Error, can not be restarted, should be destroyed
  AM_MUXER_CODEC_INIT = 1,   //Finished initialing, not started
  AM_MUXER_CODEC_STOPPED,    //stopped, can be restarted
  AM_MUXER_CODEC_RUNNING,    //Have been started, is running.
};

class AMIMuxerCodec
{
  public:
    virtual AM_STATE start()                                = 0;
    virtual AM_STATE stop()                                 = 0;
    virtual bool is_running()                               = 0;
    virtual bool start_file_writing()                       = 0;
    virtual bool stop_file_writing()                        = 0;
    virtual AM_STATE set_config(AMMuxerCodecConfig *config) = 0;
    virtual AM_STATE get_config(AMMuxerCodecConfig *config) = 0;
    virtual AM_MUXER_CODEC_STATE get_state()                = 0;
    virtual AM_MUXER_ATTR get_muxer_attr()                  = 0;
    virtual void feed_data(AMPacket* packet)                = 0;
    virtual ~AMIMuxerCodec(){}
};

#ifdef __cplusplus
extern "C"
{
#endif
AMIMuxerCodec* get_muxer_codec(const char* config);
#ifdef __cplusplus
}
#endif

typedef AMIMuxerCodec* (*MuxerCodecNew)(const char* config);

#define MUXER_CODEC_NEW ((const char*)"get_muxer_codec")

#endif /* AM_MUXER_CODEC_IF_H_ */
