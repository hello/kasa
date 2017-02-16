/*******************************************************************************
 * am_export_client_uds.h
 *
 * History:
 *   2015-01-04 - [Zhi He] created file
 *
 * Copyright (C) 2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#ifndef __AM_EXPORT_CLIENT_UDS_H__
#define __AM_EXPORT_CLIENT_UDS_H__

using std::list;
class AMExportClientUDS: public AMIExportClient
{
  protected:
    AMExportClientUDS();
    AMExportClientUDS(AMExportConfig *config);
    virtual ~AMExportClientUDS();

  public:
    static AMExportClientUDS* create(AMExportConfig *config);
    virtual void destroy()                        override;

  public:
    virtual bool connect_server(const char *url)  override;
    virtual void disconnect_server()              override;
    virtual bool receive(AMExportPacket *packet)  override;
    virtual void release(AMExportPacket *packet)  override;

  public:
    bool init();
  private:
    static void thread_entry(void *arg);
    bool get_data();
    bool map_bsb();
    bool unmap_bsb();

  private:
    sockaddr_un     m_addr;
    int             m_iav_fd;
    int             m_socket_fd;
    int             m_max_fd;
    int             m_control_fd[2];
    bool            m_run;
    bool            m_block;
    bool            m_is_connected;

    fd_set          m_all_set;
    fd_set          m_read_set;

    uint32_t        m_bsb_size;
    uint8_t        *m_bsb_addr;

    AMEvent        *m_event;
    AMMutex        *m_mutex;
    AMCondition    *m_cond;

    AMThread       *m_recv_thread;
    list<AMExportPacket> m_packet_queue;

    AMExportConfig m_config;
};

#endif
