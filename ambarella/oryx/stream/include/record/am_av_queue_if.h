/**
 * am_av_queue_if.h
 *
 *  History:
 *		Dec 29, 2014 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _AM_AV_QUEUE_IF_H_
#define _AM_AV_QUEUE_IF_H_

extern const AM_IID IID_AMIAVQueue;

class AMIAVQueue: public AMIInterface
{
  public:
    DECLARE_INTERFACE(AMIAVQueue, IID_AMIAVQueue);
    virtual bool is_ready_for_event() = 0;
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
#endif /* _AM_AV_QUEUE_IF_H_ */
