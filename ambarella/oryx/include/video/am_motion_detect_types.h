/*******************************************************************************
 * am_motion_detect_types.h
 *
 * History:
 *   May 4, 2016 - [binwang] created file
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
#ifndef ORYX_INCLUDE_VIDEO_AM_MOTION_DETECT_TYPES_H_
#define ORYX_INCLUDE_VIDEO_AM_MOTION_DETECT_TYPES_H_

#include "am_video_types.h"

#define MAX_ROI_NUM 4

enum AM_MD_KEY
{
  AM_MD_ENABLE = 0,
  AM_MD_BUFFER_ID,
  AM_MD_BUFFER_TYPE,
  AM_MD_ROI,
  AM_MD_THRESHOLD,
  AM_MD_LEVEL_CHANGE_DELAY,
  AM_MD_SYNC_CONFIG,
  AM_MD_KEY_NUM
};

enum AM_MOTION_LEVEL
{
  AM_MOTION_L0 = 0,
  AM_MOTION_L1,
  AM_MOTION_L2,
  AM_MOTION_L_NUM
};

enum AM_MOTION_TYPE
{
  AM_MD_MOTION_NONE = 0,
  AM_MD_MOTION_START,
  AM_MD_MOTION_LEVEL_CHANGED,
  AM_MD_MOTION_END,
  AM_MD_MOTION_TYPE_NUM,
};

struct AMMDBuf
{
    enum AM_SOURCE_BUFFER_ID buf_id;
    enum AM_DATA_FRAME_TYPE buf_type;
};

struct AMMDConfig
{
    uint32_t key;
    void *value;
};

struct AMMDRoi
{
    uint32_t roi_id;
    uint32_t left;
    uint32_t right;
    uint32_t top;
    uint32_t bottom;
    bool valid;
};

struct AMMDThreshold
{
    uint32_t threshold[AM_MOTION_L_NUM - 1];
    uint32_t roi_id;
};

struct AMMDLevelChangeDelay
{
    uint32_t mt_level_change_delay[AM_MOTION_L_NUM - 1];
    uint32_t roi_id;
};

struct AMMDMessage
{
    AMMDBuf buf;
    uint64_t seq_num;
    uint32_t pts;
    uint32_t roi_id;
    AM_MOTION_TYPE mt_type;
    AM_MOTION_LEVEL mt_level;
    uint32_t mt_value;
    int32_t *motion_matrix;
};

typedef int32_t (*AMRecvMD)(void* owner, AMMDMessage *event_msg);

#endif /* ORYX_INCLUDE_VIDEO_AM_MOTION_DETECT_TYPES_H_ */
