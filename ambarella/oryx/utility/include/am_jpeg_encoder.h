/*******************************************************************************
 * am_jpeg_encoder.h
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

#ifndef AM_JPEG_ENCODER_H_
#define AM_JPEG_ENCODER_H_

#include <atomic>
#include <jpeglib.h>
#include "am_thread.h"
#include "am_mutex.h"
#include "am_event.h"
#include "am_jpeg_encoder_if.h"

class AMJpegEncoder: public AMIJpegEncoder
{
    friend class AMIJpegEncoder;

  public:
    virtual int create();
    virtual void destroy();
    virtual AMJpegData *new_jpeg_data(int width, int height);
    virtual void free_jpeg_data(AMJpegData *jpeg);
    virtual int encode_yuv(AMYUVData *input, AMJpegData *output);
    virtual int set_quality(int quality);
    virtual int get_quality();

  protected:
    std::atomic_int m_ref_counter;
    static AMJpegEncoder *get_instance();
    virtual void inc_ref();
    virtual void release();

  private:
    AMJpegEncoder();
    virtual ~AMJpegEncoder();
    AMJpegEncoder(AMJpegEncoder const &copy) = delete;
    AMJpegEncoder& operator=(AMJpegEncoder const &copy) = delete;

  private:
    int m_quality;
    struct jpeg_compress_struct m_info;
    struct jpeg_error_mgr m_err;
    static AMJpegEncoder *m_instance;
};

#endif  /*AM_JPEG_ENCODER_H_ */
