/*******************************************************************************
 * am_file_sink_if.h
 *
 * History:
 *   2014-12-8 - [ccjing] created file
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
    virtual void close_file(bool need_sync = false) = 0;
    /*Write the data in the cache buf into file*/
    virtual bool flush_file() = 0;
    /*Set the cache buf size which should be multiples of
     * IO_TRANSFER_BLOCK_SIZE( default setting 512KB)
    */
    virtual bool is_file_open() = 0;
    virtual uint64_t get_file_size() = 0;
    virtual bool tell_file(uint64_t& offset) = 0;
    virtual bool set_buf(uint32_t size) = 0;
    virtual bool seek_file(uint32_t offset, uint32_t whence) = 0;
    virtual bool write_file(const void* buf, uint32_t size) = 0;
    virtual void destroy() = 0;
    virtual ~AMIFileWriter() {}
};

#endif /* AM_FILE_SINK_IF_H_ */
