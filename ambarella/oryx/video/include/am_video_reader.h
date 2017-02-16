/*******************************************************************************
 * am_video_reader.h
 *
 * History:
 *   2014-8-25 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef AM_VIDEO_READER_H_
#define AM_VIDEO_READER_H_

#include "am_video_reader_if.h"
#include "am_video_device.h"
#include <atomic>

//AMVideoReader does not inherit from any class,
//it's a simple read class
class AMVideoReader: public AMIVideoReader
{
    friend class AMIVideoReader;

  public:
    //simple init function will enable both bsb and dsp mapping
    virtual AM_RESULT init();

    virtual AM_RESULT query_video_frame(AMQueryDataFrameDesc *frame_desc,
                                        uint32_t timeout);
    virtual AM_RESULT query_yuv_frame(AMQueryDataFrameDesc *frame_desc,
                                      AM_ENCODE_SOURCE_BUFFER_ID buffer_id,
                                      bool latest_snapshot);

    virtual AM_RESULT query_luma_frame(AMQueryDataFrameDesc *frame_desc,
                                       AM_ENCODE_SOURCE_BUFFER_ID buffer_id,
                                       bool latest_snapshot);

    virtual AM_RESULT query_bayer_raw_frame(AMQueryDataFrameDesc *frame_desc);
    virtual AM_RESULT query_stream_info(AMStreamInfo *stream_info);

    virtual AM_RESULT get_dsp_mem(AMMemMapInfo *dsp_mem);
    virtual AM_RESULT get_bsb_mem(AMMemMapInfo *bsb_mem);

    //helper functions
    virtual bool is_new_video_session(uint32_t video_stream_id)
    {
      return m_stream_session_changed[video_stream_id];
    }

  protected:
    static AMVideoReader *get_instance();
    virtual void release();
    virtual void inc_ref();

    virtual AM_RESULT destroy();
    //dump_bsb:  enable it if you want to get "Encoded Video frame".
    //dump_dsp:  enable it if you want to get "YUV, LUMA, bayer pattern RAW"
    virtual AM_RESULT init(bool dump_bsb, bool dump_dsp);

    AM_RESULT map_dsp();
    AM_RESULT unmap_dsp();

    AM_RESULT map_bsb();
    AM_RESULT unmap_bsb();

  private:
    AMVideoReader();
    virtual ~AMVideoReader();
    //disable the possibility to use copy constructor
    AMVideoReader(AMVideoReader const &copy) = delete;
    AMVideoReader& operator=(AMVideoReader const &copy) = delete;

  protected:
    int             m_iav; //iav file handle
    std::atomic_int m_ref_counter;
    bool            m_init_ready;
    bool            m_is_dsp_mapped;
    bool            m_is_bsb_mapped;
    bool            m_stream_session_changed[AM_STREAM_MAX_NUM];
    //video session management, helps user to manage session change
    //for data driven readout
    uint32_t        m_stream_session_id[AM_STREAM_MAX_NUM];
    AMMemMapInfo    m_dsp_mem;
    AMMemMapInfo    m_bsb_mem;

  private:
    static AMVideoReader  *m_instance;
};

#endif /* AM_VIDEO_READER_H_ */
