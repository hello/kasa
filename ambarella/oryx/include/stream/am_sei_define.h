/*******************************************************************************
 * am_sei_define.h
 *
 * History:
 *   2016-11-22 - [ccjing] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
/*! @file am_sei_define.h
 *  @brief sei define information
 */
#ifndef ORYX_INCLUDE_STREAM_AM_SEI_DEFINE_H_
#define ORYX_INCLUDE_STREAM_AM_SEI_DEFINE_H_
/*! @enum AM_SEI_TYPE
 *  @brief SEI type.
 */
enum AM_SEI_TYPE
{
  AM_SEI_NULL    = -1,
  AM_SEI_GSENSOR = 0,
  AM_SEI_GPS     = 1,
};
/*! @struct AM_GSENSOR_INFO
 *  @brief g-sensor info define
 */
struct AM_GSENSOR_INFO
{
    uint32_t    stream_id         = 0;
    AM_SEI_TYPE type              = AM_SEI_NULL;
    uint32_t    sample_rate       = 0;
    uint32_t    sample_size       = 0;
    uint32_t    pkt_pts_increment = 0;
};

#endif /* ORYX_INCLUDE_STREAM_AM_SEI_DEFINE_H_ */
