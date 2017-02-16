/*******************************************************************************
 * am_muxer_if.h
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
#ifndef ORYX_STREAM_INCLUDE_RECORD_AM_MUXER_IF_H_
#define ORYX_STREAM_INCLUDE_RECORD_AM_MUXER_IF_H_

extern const AM_IID IID_AMIMuxer;

enum AM_MUXER_TYPE
{
  AM_MUXER_TYPE_NONE,
  AM_MUXER_TYPE_DIRECT,
  AM_MUXER_TYPE_FILE,
};

class AMIMuxer: public AMIInterface
{
  public:
    DECLARE_INTERFACE(AMIMuxer, IID_AMIMuxer);
    virtual AM_STATE start()            = 0;
    virtual AM_STATE stop()             = 0;
    virtual bool start_file_recording() = 0;
    virtual bool stop_file_recording()  = 0;
    virtual uint32_t version()          = 0;
    virtual AM_MUXER_TYPE type()        = 0;
};

#ifdef __cplusplus
extern "C" {
#endif
AMIInterface *create_filter(AMIEngine *engine, const char *config,
                            uint32_t input_num, uint32_t output_num);
#ifdef __cplusplus
}
#endif

#endif /* ORYX_STREAM_INCLUDE_RECORD_AM_MUXER_IF_H_ */
