/*******************************************************************************
 * am_amf_msgsys.h
 *
 * History:
 *   2014-7-22 - [ypchang] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
