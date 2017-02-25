/*******************************************************************************
 * am_dptz_types.h
 *
 * History:
 *   Mar 28, 2016 - [zfgong] created file
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
#ifndef AM_DPTZ_TYPES_H_
#define AM_DPTZ_TYPES_H_

#include "am_video_types.h"

#include <vector>
#include <map>


struct AMDPTZRatio
{
    std::pair<bool, float> pan  = {false, 0.0};
    std::pair<bool, float> tilt = {false, 0.0};
    std::pair<bool, float> zoom = {false, 0.0};
};


struct AMDPTZSize
{
    std::pair<bool, int> x = {false, 0};
    std::pair<bool, int> y = {false, 0};
    std::pair<bool, int> w = {false, 0};
    std::pair<bool, int> h = {false, 0};
};

typedef std::map<AM_SOURCE_BUFFER_ID, AMDPTZRatio>  AMDPTZRatioMap;
typedef std::map<AM_SOURCE_BUFFER_ID, AMDPTZSize>  AMDPTZSizeMap;

#endif /* AM_DPTZ_TYPES_H_ */
