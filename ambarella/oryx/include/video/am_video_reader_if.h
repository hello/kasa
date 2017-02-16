/*******************************************************************************
 * am_video_reader_if.h
 *
 * History:
 *   2014-12-12 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_INCLUDE_VIDEO_AM_VIDEO_READER_IF_H_
#define ORYX_INCLUDE_VIDEO_AM_VIDEO_READER_IF_H_

#include "am_video_types.h"
#include "am_pointer.h"

struct AMStreamInfo
{
    AM_STREAM_ID stream_id;
    uint32_t m;
    uint32_t n;
    uint32_t mul;
    uint32_t div;
    uint32_t rate;
    uint32_t scale;
};

struct AMVideoFrameDesc
{
    AM_VIDEO_FRAME_TYPE video_type;
    uint32_t width;
    uint32_t height;
    uint32_t stream_id;         //0 ~ N
    uint32_t frame_num;
    uint32_t data_addr_offset;
    uint32_t data_size;
    uint32_t session_id;
    uint8_t  jpeg_quality;      //1 ~ 99
    uint8_t  stream_end_flag;   //0 or 1
    uint8_t  reserved[2];
};

struct AMYUVFrameDesc
{
    AM_ENCODE_SOURCE_BUFFER_ID buffer_id;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t y_addr_offset;
    uint32_t uv_addr_offset;    //NV12 format, (UV interleaved)
    uint32_t seq_num;
    AM_ENCODE_CHROMA_FORMAT format;
    uint32_t non_block_flag;
    uint32_t reserved;
};

struct AMLumaFrameDesc
{
    AM_ENCODE_SOURCE_BUFFER_ID buffer_id;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t data_addr_offset;
    uint32_t seq_num;
    uint32_t non_block_flag;
    uint32_t reserved;
};

struct AMBayerPatternFrameDesc
{
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t data_addr_offset;
    uint32_t non_block_flag;
    uint32_t reserved;
};

struct AMGenericDataFrameDesc
{
    uint32_t generic_data[16];
};

struct AMQueryDataFrameDesc
{
    AM_DATA_FRAME_TYPE frame_type;  //know whether it's video or yuv or luma
    //or bayer or other generic data
    uint64_t mono_pts;
    bool cancel;                   //cancel the current or the next query once
    union
    {
        AMVideoFrameDesc video;
        AMYUVFrameDesc yuv;
        AMLumaFrameDesc luma;
        AMBayerPatternFrameDesc bayer;
        AMGenericDataFrameDesc data;
    };
};

class AMIVideoReader;
typedef AMPointer<AMIVideoReader> AMIVideoReaderPtr;

class AMIVideoReader
{
    friend class AMPointer<AMIVideoReader>;

  public:
    static AMIVideoReaderPtr get_instance();
    virtual AM_RESULT init() = 0;
    virtual AM_RESULT query_video_frame(AMQueryDataFrameDesc *frame_desc,
                                        uint32_t timeout) = 0;
    virtual AM_RESULT query_yuv_frame(AMQueryDataFrameDesc *frame_desc,
                                      AM_ENCODE_SOURCE_BUFFER_ID buffer_id,
                                      bool latest_snapshot) = 0;
    virtual AM_RESULT query_luma_frame(AMQueryDataFrameDesc      *frame_desc,
                                       AM_ENCODE_SOURCE_BUFFER_ID buffer_id,
                                       bool latest_snapshot) = 0;
    virtual AM_RESULT query_bayer_raw_frame(
        AMQueryDataFrameDesc *frame_desc) = 0;
    virtual AM_RESULT query_stream_info(AMStreamInfo *stream_info) = 0;

    virtual AM_RESULT get_dsp_mem(AMMemMapInfo *dsp_mem) = 0;
    virtual AM_RESULT get_bsb_mem(AMMemMapInfo *bsb_mem) = 0;
    virtual bool is_new_video_session(uint32_t video_stream_id) = 0;

  protected:
    virtual void release() = 0;
    virtual void inc_ref() = 0;
    virtual ~AMIVideoReader(){}
};

#endif /* ORYX_INCLUDE_VIDEO_AM_VIDEO_READER_IF_H_ */
