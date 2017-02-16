/*******************************************************************************
 * am_amf_msgsys.h
 *
 * History:
 *   2014-7-22 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AMF_MSGSYS_H_
#define AM_AMF_MSGSYS_H_

class AMQueue;
class AMMsgPort;
class AMMsgSys;

/*
 * AMMsgPort
 */
class AMMsgPort: public AMIMsgPort
{
    friend class AMMsgSys;

  public:
    static AMMsgPort* create(AMIMsgSink *msgSink, AMMsgSys *msgSys);

  public:
    virtual void* get_interface(AM_REFIID refiid);
    virtual void destroy();
    virtual AM_STATE post_msg(AmMsg& msg);

  private:
    AMMsgPort(AMIMsgSink *msgSink);
    virtual ~AMMsgPort();
    AM_STATE init(AMMsgSys *msgSys);

  private:
    AMIMsgSink *m_msg_sink;
    AMQueue    *m_queue;
};

/*
 * AMMsgSys
 */
class AMMsgSys
{
    friend class AMMsgPort;
  public:
    static AMMsgSys* create();
    void destroy();

  private:
    AMMsgSys();
    virtual ~AMMsgSys();
    AM_STATE init();

  private:
    static void static_thread_entry(void *data);
    void main_loop();

  private:
    AMQueue  *m_main_q;
    AMThread *m_thread;
};

#endif /* AM_AMF_MSGSYS_H_ */
