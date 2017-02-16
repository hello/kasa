/*******************************************************************************
 * am_jpeg_encoder_if.h
 *
 * History:
 *   2015-09-15 - [zfgong] created file
 *
 * Copyright (C) 2015-2016, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_INCLUDE_AM_JPEG_ENCODER_IF_H_
#define ORYX_INCLUDE_AM_JPEG_ENCODER_IF_H_

#include "am_pointer.h"

typedef struct AMYUVData {
  int format;
  int width;
  int height;
  int pitch;
  struct iovec y;
  struct iovec uv;
} AMYUVData;

typedef struct AMJpegData {
  struct iovec data;
  int color_componenets;
} AMJpegData;

class AMIJpegEncoder;
typedef AMPointer<AMIJpegEncoder> AMIJpegEncoderPtr;

class AMIJpegEncoder
{
    friend class AMPointer<AMIJpegEncoder>;

  public:
    static AMIJpegEncoderPtr get_instance();
    virtual int create() = 0;
    virtual void destroy() = 0;

    virtual AMJpegData *new_jpeg_data(int width, int height) = 0;
    virtual void free_jpeg_data(AMJpegData *jpeg) = 0;
    virtual int encode_yuv(AMYUVData *input, AMJpegData *output) = 0;
    virtual int set_quality(int quality) = 0;
    virtual int get_quality() = 0;

  protected:
    virtual void release() = 0;
    virtual void inc_ref() = 0;
    virtual ~AMIJpegEncoder(){}
};


#endif /* ORYX_INCLUDE_AM_JPEG_ENCODER_IF_H_ */
