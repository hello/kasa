/*******************************************************************************
 * am_record_event_sender.h
 *
 * History:
 *   2016-2-29 - [ccjing] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#ifndef AM_EVENT_SENDER_STRUCTURE_H_
#define AM_EVENT_SENDER_STRUCTURE_H_

enum AM_EVENT_ATTR
{
  AM_EVENT_NONE           = 0,
  AM_EVENT_H26X           = 1,
  AM_EVENT_MJPEG          = 2,
  AM_EVENT_PERIODIC_MJPEG = 3,
  AM_EVENT_STOP_CMD       = 4,
};

struct AMEventPeriodicMjpeg
{
    uint32_t      stream_id         = 0;
    uint32_t      interval_second   = 0;
    uint32_t      once_jpeg_num     = 0;
    uint8_t       start_time_hour   = 0;
    uint8_t       start_time_minute = 0;
    uint8_t       start_time_second = 0;
    uint8_t       end_time_hour     = 0;
    uint8_t       end_time_minute   = 0;
    uint8_t       end_time_second   = 0;
};

struct AMEventMjpeg
{
    uint32_t stream_id           = 0;
    uint8_t  pre_cur_pts_num     = 0;
    uint8_t  after_cur_pts_num   = 0;
    uint8_t  closest_cur_pts_num = 0;
};

struct AMEventH26x
{
    uint32_t stream_id        = 0;
    uint32_t history_duration = 0;
    uint32_t future_duration  = 0;
};

struct AMEventStopCMD
{
    uint32_t stream_id = 0;
};

struct AMEventStruct
{
    AM_EVENT_ATTR attr = AM_EVENT_NONE;
    union {
        AMEventH26x          h26x;
        AMEventMjpeg         mjpeg;
        AMEventPeriodicMjpeg periodic_mjpeg;
        AMEventStopCMD       stop_cmd;
    };
    AMEventStruct() {}
};
#endif /* AM_EVENT_SENDER_STRUCTURE_H_ */
