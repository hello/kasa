/*******************************************************************************
 * am_amf_packet_pool.h
 *
 * History:
 *   2014-7-24 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
    std::atomic_int m_ref_count;
    AMQueue        *m_packet_q;
    const char     *m_name;
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
