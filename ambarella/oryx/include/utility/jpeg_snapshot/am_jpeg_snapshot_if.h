/*******************************************************************************
 * am_jpeg_snapshot_if.h
 *
 * History:
 *   2015-09-18 - [zfgong] created file
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
#ifndef ORYX_INCLUDE_AM_JPEG_SNAPSHOT_IF_H_
#define ORYX_INCLUDE_AM_JPEG_SNAPSHOT_IF_H_

#include <iostream>
#include "am_pointer.h"
#include "video/am_video_types.h"

using std::string;

class AMIJpegSnapshot;
typedef AMPointer<AMIJpegSnapshot> AMIJpegSnapshotPtr;

typedef int (*AMJpegSnapshotCb)(void *data, int len);

class AMIJpegSnapshot
{
    friend class AMPointer<AMIJpegSnapshot>;

  public:
    static AMIJpegSnapshotPtr get_instance();
    virtual AM_RESULT run() = 0;

    /*
     * below methods can only be called before run
     * if save file disabled, there is no need to set file path and max num
     */
    virtual AM_RESULT set_file_path(string path) = 0;
    virtual AM_RESULT set_fps(float fps) = 0;
    virtual AM_RESULT set_file_max_num(int num) = 0;
    virtual AM_RESULT set_data_cb(AMJpegSnapshotCb cb) = 0;
    virtual AM_RESULT save_file_disable() = 0;
    virtual AM_RESULT save_file_enable() = 0;

    /*
     * below methods can be called before or after run
     */
    virtual AM_RESULT set_source_buffer(AM_SOURCE_BUFFER_ID id) = 0;
    virtual AM_RESULT capture_start() = 0;
    virtual AM_RESULT capture_stop() = 0;

  protected:
    virtual void release() = 0;
    virtual void inc_ref() = 0;
    virtual ~AMIJpegSnapshot(){}
};




#endif /* ORYX_INCLUDE_AM_JPEG_SNAPSHOT_IF_H_ */
