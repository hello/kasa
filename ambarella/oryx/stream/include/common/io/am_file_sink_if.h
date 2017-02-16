/*******************************************************************************
 * am_file_sink_if.h
 *
 * History:
 *   2014-12-8 - [ccjing] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_FILE_SINK_IF_H_
#define AM_FILE_SINK_IF_H_

#include "version.h"

#ifndef IO_SIZE_KB
#define IO_SIZE_KB (1024)
#endif

#ifndef IO_TRANSFER_BLOCK_SIZE
#define IO_TRANSFER_BLOCK_SIZE (512 * IO_SIZE_KB)
#endif

#ifndef MAX_FILE_NAME_LEN
#define MAX_FILE_NAME_LEN (256)
#endif

/*
 * AMIFileReader
*/
class AMIFileReader
{
  public:
    static AMIFileReader* create();
    virtual bool open_file(const char* file_name) = 0;
    virtual void close_file() = 0;
    virtual uint64_t get_file_size() = 0;
    /*get the current read or write position*/
    virtual bool tell_file(uint64_t& offset) = 0;
    /*seek form begin of the file*/
    virtual bool seek_file(int32_t offset) = 0;
    /*seek form current position of the file*/
    virtual bool advance_file(int32_t offset) = 0;
    virtual int read_file(void* buffer, uint32_t size) = 0;
    virtual void destroy() = 0;
    virtual ~AMIFileReader() {}
};
/*
 * AMIFileWriter
*/
class AMIFileWriter
{
  public:
    static AMIFileWriter* create();
    virtual bool create_file(const char* file_name) = 0;
    virtual void close_file() = 0;
    /*Write the data in the cache buf into file*/
    virtual bool flush_file() = 0;
    /*Set the cache buf size which should be multiples of
     * IO_TRANSFER_BLOCK_SIZE( default setting 512KB)
    */
    virtual bool is_file_open() = 0;
    virtual bool tell_file(uint64_t& offset) = 0;
    virtual bool set_buf(uint32_t size) = 0;
    virtual bool seek_file(uint32_t offset, uint32_t whence) = 0;
    virtual bool write_file(const void* buf, uint32_t size) = 0;
    virtual void destroy() = 0;
    virtual ~AMIFileWriter() {}
};

#endif /* AM_FILE_SINK_IF_H_ */
