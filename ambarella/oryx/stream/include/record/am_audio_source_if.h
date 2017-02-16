/*******************************************************************************
 * am_audio_source_if.h
 *
 * History:
 *   2014-12-2 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_SOURCE_IF_H_
#define AM_AUDIO_SOURCE_IF_H_

extern const AM_IID IID_AMIAudioSource;

class AMIAudioSource: public AMIInterface
{
  public:
    DECLARE_INTERFACE(AMIAudioSource, IID_AMIAudioSource);
    virtual AM_STATE start()   = 0;
    virtual AM_STATE stop()    = 0;
    virtual uint32_t version() = 0;
};

#ifdef __cplusplus
extern "C" {
#endif
AMIInterface* create_filter(AMIEngine *engine, const char *config,
                            uint32_t input_num, uint32_t output_num);
#ifdef __cplusplus
}
#endif

#endif /* AM_AUDIO_SOURCE_IF_H_ */
