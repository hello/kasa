/*******************************************************************************
 * am_encode_buffer.h
 *
 * History:
 *  July 10, 2014 - [Louis] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_ENCODE_BUFFER_H_
#define AM_ENCODE_BUFFER_H_

#include <mutex>  //using C++11 mutex

#include "am_encode_device_if.h"
#include "am_video_device.h"
#include "am_video_dsp.h"
#include "am_lbr_control.h"

class AMVideoDevice;
class AMEncodeDevice;

class AMEncodeSourceBuffer
{
  public:
    AMEncodeSourceBuffer();
    virtual ~AMEncodeSourceBuffer();

    virtual AM_RESULT init(AMEncodeDevice *encode_device,
                           AM_ENCODE_SOURCE_BUFFER_ID id,
                           int iav_hd);
    virtual AM_RESULT setup_source_buffer_all(
        AMEncodeSourceBufferFormatAll *source_buffer_format_all);
    virtual AM_RESULT set_source_buffer_format(
        AMEncodeSourceBufferFormat *buffer_format);
    virtual AM_RESULT get_source_buffer_format(
        AMEncodeSourceBufferFormat *buffer_format);

    //ref counter is used to identify whether the buffer is being used
    //(is it useful?)
    virtual AM_RESULT IncRef();
    virtual AM_RESULT DecRef();

  public:
    AMEncodeSourceBufferFormat m_format;
    AM_ENCODE_SOURCE_BUFFER_STATE m_state;

  protected:
    int32_t m_max_source_buffer_num;
    int32_t m_ref_counter; //init zero, once used,  will increase
    AMEncodeDevice *m_encode_device;
    AM_ENCODE_SOURCE_BUFFER_ID m_id;
    int m_iav;
};

#endif /* AM_SOURCE_BUFFER_H_ */
