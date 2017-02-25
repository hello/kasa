/*******************************************************************************
 * am_amf_packet_pool.h
 *
 * History:
 *   2014-7-24 - [ypchang] created file
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
#ifndef AM_AMF_PACKET_POOL_H_
#define AM_AMF_PACKET_POOL_H_

#include <atomic>

/*
 * AMPacketPool
 */
class AMPacketPool: public AMObject, public AMIPacketPool
{
    typedef AMObject inherited;
  public:
    static AMPacketPool* create(const char *name, uint32_t count);

  public:
    virtual void* get_interface(AM_REFIID refiid);
    virtual void destroy();

    virtual const char* get_name();
    virtual void enable(bool enabled = true);
    virtual bool is_enable();
    virtual bool alloc_packet(AMPacket*& packet, uint32_t size);
    virtual uint32_t get_avail_packet_num();
    virtual void add_ref(AMPacket *packet);
    virtual void release(AMPacket *packet);
    virtual void add_ref();
    virtual void release();

  protected:
    AMPacketPool();
    virtual ~AMPacketPool();
    AM_STATE init(const char *name, uint32_t count);

  protected:
    virtual void on_release_packet(AMPacket *packet)
    {/* do nothing here, can be over loaded in sub-class */}

  protected:
    AMQueue        *m_packet_q;
    const char     *m_name;
    std::atomic_int m_ref_count;
};

/*
 * AMSimplePacketPool
 */
class AMSimplePacketPool: public AMPacketPool
{
    typedef AMPacketPool inherited;
  public:
    static AMSimplePacketPool* create(const char *name,
                                      uint32_t count,
                                      uint32_t objSize = sizeof(AMPacket));
  protected:
    AMSimplePacketPool();
    virtual ~AMSimplePacketPool();
    AM_STATE init(const char *name,
                  uint32_t count,
                  uint32_t objSize = sizeof(AMPacket));

  protected:
    AMPacket *m_packet_mem;
};

/*
 * AMFixedPacketPool
 */
class AMFixedPacketPool: public AMSimplePacketPool
{
    typedef AMSimplePacketPool inherited;
  public:
    static AMFixedPacketPool* create(const char *name,
                                     uint32_t count,
                                     uint32_t dataSize);

  protected:
    AMFixedPacketPool();
    virtual ~AMFixedPacketPool();
    AM_STATE init(const char *name, uint32_t count, uint32_t dataSize);

  private:
    uint8_t *m_payload_mem;
    uint32_t m_max_data_size;
};

#endif /* AM_AMF_PACKET_POOL_H_ */
