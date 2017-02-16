/**
 * am_event_sender_if.h
 *
 *  History:
 *    Mar 6, 2015 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _AM_EVENT_SENDER_IF_H_
#define _AM_EVENT_SENDER_IF_H_

extern const AM_IID IID_AMIEventSender;

class AMIEventSender: public AMIInterface
{
  public:
    DECLARE_INTERFACE(AMIEventSender, IID_AMIEventSender);
    virtual AM_STATE start() = 0;
    virtual AM_STATE stop() = 0;
    virtual bool send_event() = 0;
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
#endif /* _AM_EVENT_SENDER_IF_H_ */
