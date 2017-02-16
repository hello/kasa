/*******************************************************************************
 * am_jpeg_snapshot.h
 *
 * History:
 *   2015-07-01 - [Zhifeng Gong] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef AM_JPEG_SNAPSHOT_H_
#define AM_JPEG_SNAPSHOT_H_

#include <sys/uio.h>
#include "am_thread.h"
#include "am_mutex.h"
#include "am_event.h"
#include "am_event_types.h"
#include "am_video_reader_if.h"
#include "am_jpeg_encoder_if.h"
#include "am_jpeg_snapshot_if.h"

#define MAX_FILENAME_LEN       (256)

struct JpegSnapshotParam {
  bool md_enable;
  int source_buffer;
  float fps;
  char jpeg_path[MAX_FILENAME_LEN];
  int max_files;
};

typedef struct file_table {
  int id;
  int used;
  char name[MAX_FILENAME_LEN];
} file_table_t;


class AMJpegSnapshotConfig
{
  public:
    AMJpegSnapshotConfig();
    virtual ~AMJpegSnapshotConfig();
    JpegSnapshotParam *get_config(const std::string &cfg_file_path);
    bool set_config(JpegSnapshotParam *config, const std::string &cfg_file_path);
    char *get_jpeg_path();
    int get_max_files();

  private:
    JpegSnapshotParam *m_param;
};

class AMJpegSnapshot: public AMIJpegSnapshot
{
    friend class AMIJpegSnapshot;

  public:
    virtual AM_RESULT init();
    virtual void destroy();
    static AMJpegSnapshot *create();
    virtual AM_RESULT start();
    virtual AM_RESULT stop();

    virtual void update_motion_state(AM_EVENT_MESSAGE *msg);
    virtual bool is_enable();
    static AMJpegSnapshot *m_instance;

  protected:
    std::atomic_int m_ref_counter;
    static AMJpegSnapshot *get_instance();
    virtual void inc_ref();
    virtual void release();

  private:
    AMYUVData *capture_yuv_buffer();
    void free_yuv_buffer(AMYUVData *yuv_buf);
    static void static_jpeg_encode_thread(void *arg);
    int encode_yuv_to_jpeg();
    bool init_dir();
    void deinit_dir();
    int trim_jpeg_files();
    int save_jpeg_in_file(char *filename, void *data, size_t size);
    int get_file_name(char *str, int len);

  private:
    AMJpegSnapshot();
    virtual ~AMJpegSnapshot();
    AMJpegSnapshot(AMJpegSnapshot const &copy) = delete;
    AMJpegSnapshot& operator=(AMJpegSnapshot const &copy) = delete;

  private:
    bool m_run;

    struct file_table *m_file_table;
    int m_file_index;

    AM_MOTION_TYPE m_motion_state;
    JpegSnapshotParam *m_jpeg_encode_param;
    AMIJpegEncoderPtr m_jpeg_encoder;
    AMThread *m_thread;
    AMMutex *m_mutex;
    AMCondition *m_cond;
    AMEvent *m_sem;
    AMIVideoReaderPtr m_video_reader;

    AMJpegSnapshotConfig *m_config;
    std::string m_conf_path;

};

#endif  /*AM_JPEG_SNAPSHOT_H_ */
