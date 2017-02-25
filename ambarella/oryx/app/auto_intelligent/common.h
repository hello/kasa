/*******************************************************************************
 * common.h
 *
 * History:
 *   Nov 17, 2015 - [Huaiqing Wang] created file
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
#ifndef ORYX_APP_AUTO_INTELLIGENT_COMMON_H_
#define ORYX_APP_AUTO_INTELLIGENT_COMMON_H_

#include "am_base_include.h"

#define QUERY_SOURCE_BUFFER_ID (2) //third buffer
#define OVERLAY_STREAM_ID (0) //main stream
#define FILE_NAME_NUM_MAX (128)

struct point_desc {
    uint32_t x;
    uint32_t y;
};

struct size_desc {
    uint32_t       width;
    uint32_t       height;
};

struct buffer_crop_desc {
    point_desc offset;
    size_desc size;
};

struct gray_frame_desc {
    uint64_t mono_pts;
    uint8_t *addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t seq_num;
};

struct vout_osd_desc {
    //when type is line or curve,k and b is the equation's
    //weighted factor and offset value
    double  k;
    //ponit position offset
    point_desc *offset;
    //point number
    uint32_t n;
    //osd area max size
    size_desc size;
    //0:draw dot,(x=k, y=b);1:draw line,x = k * y + b;
    //2:draw curve,x = k * y2 + b
    int32_t osd_type;
    int32_t b;
    uint32_t color;
};

struct overlay_layout_desc {
    //0:left top;1:right top;2:left bottom;3:right bottom;
    //4:custom position with offset
    int32_t layout_id;
    point_desc offset;
};

struct overlay_manipulate_desc {
    //0:nothing;1,add;2:delete;3:destory
    uint32_t     manipulate_flag;
    //area id information used for delete manipulate
    uint32_t    area_id;
    //overlay position used for add manipulate
    overlay_layout_desc layout;
    //bmp file information used for add manipulate
    char bmp_file[FILE_NAME_NUM_MAX];
};

//used for add bmp or update bmp for the specify area
struct bmp_overlay_manipulate_desc {
    //the specify area id which return by init api
    uint32_t    area_id;

    //offset position's start point is the area's start point,
    //not the stream's start point
    point_desc offset;

    //used to transparent
    // v:bit[24-31],u:bit[16-23],y:bit[8-15],a:bit[0-7],
    uint32_t color_key;

    //used to transparent with color_key
    uint32_t color_range;

    //bmp full path name
    char bmp_file[FILE_NAME_NUM_MAX];
};

#endif /* ORYX_APP_AUTO_INTELLIGENT_AUTO_INTELLIGENT_H_ */
