/*******************************************************************************
 * am_file_sink.h
 *
 * History:
 *   2014-12-11 - [ccjing] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_FILE_SINK_H_
#define AM_FILE_SINK_H_

#include "am_file_sink_if.h"
class AMSpinLock;
class AMFile;
/*
 * AMFileReader
*/
class AMFileReader: public AMIFileReader
{
  public:
    static AMFileReader * create();

  private:
    AMFileReader(): m_lock(NULL), m_file(NULL){}
    bool init();
    virtual ~AMFileReader();

  public:
    virtual void destroy();
    //AMIFileReader
    virtual bool open_file(const char* file_name);
    virtual void close_file();
    virtual uint64_t get_file_size();
    virtual bool tell_file(uint64_t& offset);
    virtual bool seek_file(int32_t offset);
    virtual bool advance_file(int32_t offset);
    virtual int read_file(void* buffer, uint32_t size);

  private:
    AMSpinLock     *m_lock;
    AMFile         *m_file;
};

/*
 * AMFileWriter
*/
class AMFileWriter: public AMIFileWriter
{
  public:
    static AMFileWriter* create();
  private:
    AMFileWriter():
      m_buf_size(0),
      m_data_size(0),
      m_write_cnt(0),
      m_write_data_size(0),
      m_write_avg_speed(0),
      m_hist_min_speed(0xFFFFFFFF),
      m_hist_max_speed(0),
      m_write_time(0),
      m_file_fd(-1),
      m_custom_buf(NULL),
      m_data_write_addr(NULL),
      m_data_send_addr(NULL),
      m_lock(NULL),
      m_IO_direct_set(false),
      m_enable_direct_IO(false)
    {}
    bool init();
    virtual ~AMFileWriter();

  public:
    virtual void destroy();

    //AMIFileWriter
    virtual bool create_file(const char* file_name);
    virtual void close_file();
    virtual bool flush_file();
    virtual bool set_buf(uint32_t size);
    virtual bool tell_file(uint64_t& offset);
    virtual bool seek_file(uint32_t offset, uint32_t whence = SEEK_SET);
    virtual bool write_file(const void* buf, uint32_t len);
    virtual void set_enable_direct_IO(bool enable);
    virtual bool is_file_open();

  private:
    bool set_file_flag(int flag, bool enable);
    ssize_t write(const void* buf, size_t nbyte);
    void _close_file();
    bool _flush_file();

  private:
    uint32_t       m_buf_size;
    uint32_t       m_data_size;
    uint32_t       m_write_cnt;
    uint64_t       m_write_data_size;
    uint64_t       m_write_avg_speed;
    uint64_t       m_hist_min_speed;
    uint64_t       m_hist_max_speed;
    uint64_t       m_write_time;
    int            m_file_fd;

    uint8_t       *m_custom_buf;
    uint8_t       *m_data_write_addr;
    uint8_t       *m_data_send_addr;
    AMSpinLock    *m_lock;

    bool           m_IO_direct_set;
    bool           m_enable_direct_IO;
};

#endif /* AM_FILE_SINK_H_ */
