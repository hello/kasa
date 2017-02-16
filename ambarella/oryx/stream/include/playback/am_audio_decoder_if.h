/*******************************************************************************
 * am_audio_decoder_if.h
 *
 * History:
 *   2014-9-24 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_DECODER_IF_H_
#define AM_AUDIO_DECODER_IF_H_

#include "am_audio_codec_if.h"

extern const AM_IID IID_AMIAudioDecoder;

class AMIAudioDecoder: public AMIInterface
{
  public:
    DECLARE_INTERFACE(AMIAudioDecoder, IID_AMIAudioDecoder);
    virtual AM_STATE load_audio_codec(AM_AUDIO_CODEC_TYPE codec) = 0;
    virtual uint32_t version()                                   = 0;
};

#ifdef __cplusplus
extern "C" {
#endif
AMIInterface* create_filter(AMIEngine *engine, const char *config,
                            uint32_t input_num, uint32_t output_num);
#ifdef __cplusplus
}
#endif


#endif /* AM_AUDIO_DECODER_IF_H_ */
