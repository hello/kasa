/*******************************************************************************
 * am_jpeg_snapshot.h
 *
 * History:
 *   2015-07-01 - [Zhifeng Gong] created file
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
#ifndef AM_JPEG_SNAPSHOT_H_
#define AM_JPEG_SNAPSHOT_H_

#include <sys/uio.h>
#include "am_thread.h"
#include "am_mutex.h"
#include "am_event.h"
#include "am_video_reader_if.h"
#include "am_video_address_if.h"
#include "am_jpeg_encoder_if.h"
#include "am_jpeg_snapshot_if.h"

#define MAX_FILENAME_LEN       (256)
#define JPEG_PATH_DEFAULT      "/tmp/jpeg"

struct JpegSnapshotParam {
  AM_SOURCE_BUFFER_ID source_buffer;
  float fps;
  bool need_save_file;
  string jpeg_path;
  int max_files;
  JpegSnapshotParam() :
    source_buffer(AM_SOURCE_BUFFER_2ND),
    fps(3.0),
    need_save_file(true),
    jpeg_path(JPEG_PATH_DEFAULT),
    max_files(50)
  {
  }
};

typedef struct file_table {
  int id;
  int used;
  char name[MAX_FILENAME_LEN];
} file_table_t;

enum AM_SNAPSHOT_STATE {
  AM_SNAPSHOT_START,
  AM_SNAPSHOT_STOP,
};

class AMJpegSnapshot: public AMIJpegSnapshot
{
    friend class AMIJpegSnapshot;

  public:
    static AMJpegSnapshot *create();
    virtual AM_RESULT run();

    virtual AM_RESULT set_file_path(string path);
    virtual AM_RESULT set_fps(float fps);
    virtual AM_RESULT set_file_max_num(int num);
    virtual AM_RESULT set_data_cb(AMJpegSnapshotCb cb);
    virtual AM_RESULT save_file_disable();
    virtual AM_RESULT save_file_enable();

    virtual AM_RESULT set_source_buffer(AM_SOURCE_BUFFER_ID id);
    virtual AM_RESULT capture_start();
    virtual AM_RESULT capture_stop();

  public:
    static AMJpegSnapshot *m_instance;

  protected:
    std::atomic_int m_ref_counter;
    static AMJpegSnapshot *get_instance();
    virtual void inc_ref();
    virtual void release();

  private:
    AM_RESULT init();
    //copy yuv data
    AMYUVData *capture_yuv_buffer();
    //only query yuv addr, not copy
    AMYUVData *query_yuv_buffer();
    void free_yuv_buffer(AMYUVData *yuv_buf);
    static void static_jpeg_encode_thread(void *arg);
    int encode_yuv_to_jpeg();
    bool init_dir();
    void deinit_dir();
    int trim_jpeg_files();
    int save_jpeg_in_file(char *filename, void *data, size_t size);
    int save_yuv_in_file(char *filename, AMYUVData *data);
    int get_file_name(char *str, int len);
    int get_yuv_file_name(char *str, int len);

  private:
    AMJpegSnapshot();
    virtual ~AMJpegSnapshot();
    virtual void destroy();
    AMJpegSnapshot(AMJpegSnapshot const &copy) = delete;
    AMJpegSnapshot& operator=(AMJpegSnapshot const &copy) = delete;

  private:
    bool               m_gdma_support  = false;
    bool               m_run           = false;
    int                m_file_index    = 0;
    AM_SNAPSHOT_STATE  m_state         = AM_SNAPSHOT_STOP;
    struct file_table *m_file_table    = nullptr;
    JpegSnapshotParam *m_param         = nullptr;
    AMThread          *m_thread        = nullptr;
    AMMutex           *m_mutex         = nullptr;
    AMCondition       *m_cond          = nullptr;
    AMEvent           *m_sem           = nullptr;
    AMIJpegEncoderPtr  m_jpeg_encoder  = nullptr;
    AMIVideoReaderPtr  m_video_reader  = nullptr;
    AMIVideoAddressPtr m_video_address = nullptr;
    AMJpegSnapshotCb   m_data_cb       = nullptr;
    AMAddress          m_usr_addr      = {0};
};

#endif  /*AM_JPEG_SNAPSHOT_H_ */
