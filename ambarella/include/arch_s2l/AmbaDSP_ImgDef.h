/*
 * include/arch_s2l/AmbaDSP_ImgDef.h
 *
 * History:
 *	12/12/2012  - [Steve Chen] created file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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
 */

#ifndef _AMBA_DSP_IMG_DEF_H_
#define _AMBA_DSP_IMG_DEF_H_

#include "AmbaDataType.h"

#define AMBA_DSP_IMG_RVAL_SUCCESS   (0)
#define AMBA_DSP_IMG_RVAL_ERROR     (-1)           // Common Error
#define AMBA_DSP_IMG_RVAL_CLAMP     (0x10000000)   // Auto Clamp

typedef enum _AMBA_DSP_IMG_INITIAL_MODE_e_ {
    AMBA_DSP_IMG_HARDRESET_MODE = 0x00,
    AMBA_DSP_IMG_SOFTRESET_MODE,
    AMBA_DSP_IMG_CLONE_MODE,
} AMBA_DSP_IMG_INITIAL_MODE_e;

typedef enum _AMBA_DSP_IMG_PIPE_e_ {
    AMBA_DSP_IMG_PIPE_VIDEO = 0x00,
    AMBA_DSP_IMG_PIPE_STILL,
    AMBA_DSP_IMG_PIPE_DEC,
} AMBA_DSP_IMG_PIPE_e;

typedef enum _AMBA_DSP_IMG_ALGO_MODE_e_{
    AMBA_DSP_IMG_ALGO_MODE_FAST = 0x00,
    AMBA_DSP_IMG_ALGO_MODE_LISO,
    AMBA_DSP_IMG_ALGO_MODE_HISO,
} AMBA_DSP_IMG_ALGO_MODE_e;

typedef enum _AMBA_DSP_IMG_FUNC_MODE_e_{
    AMBA_DSP_IMG_FUNC_MODE_FV = 0x00,
    AMBA_DSP_IMG_FUNC_MODE_QV,
    AMBA_DSP_IMG_FUNC_MODE_PIV,
    AMBA_DSP_IMG_FUNC_MODE_HDR,
} AMBA_DSP_IMG_FUNC_MODE_e;

typedef struct  amba_img_dsp_mode_cfg_s{
    AMBA_DSP_IMG_PIPE_e         Pipe;
    AMBA_DSP_IMG_ALGO_MODE_e    AlgoMode;
    AMBA_DSP_IMG_FUNC_MODE_e    FuncMode;
    UINT8                       ContextId;   /* Context ID */
    UINT32                      BatchId;    /* Batch cmd buf ID */
    UINT8                       ConfigId;   /* Configuration reg buf ID */
    UINT8                       Reserved;
    UINT8                       Reserved1;
    UINT8                       Reserved2;
} amba_img_dsp_mode_cfg_t;

typedef struct amba_img_dsp_sensor_subsampling_s {
    UINT8   FactorNum;              /* subsamping factor (numerator) */
    UINT8   FactorDen;              /* subsamping factor (denominator) */
} amba_img_dsp_sensor_subsampling_t;

typedef struct amba_img_dsp_vin_sensor_geometry_s{
    UINT32  StartX;     // Unit in pixel. Before downsample.
    UINT32  StartY;     // Unit in pixel. Before downsample.
    UINT32  Width;      // Unit in pixel. After downsample.
    UINT32  Height;     // Unit in pixel. After downsample.
    amba_img_dsp_sensor_subsampling_t  HSubSample;
    amba_img_dsp_sensor_subsampling_t  VSubSample;
} amba_img_dsp_vin_sensor_geometry_t;

typedef struct amba_img_dsp_win_coordinates_s{
    UINT32  LeftTopX;   // 16.16 format
    UINT32  LeftTopY;   // 16.16 format
    UINT32  RightBotX;  // 16.16 format
    UINT32  RightBotY;  // 16.16 format
} amba_img_dsp_win_coordinates_t;

typedef struct amba_img_dsp_win_geometry_s{
    UINT32  StartX;     // Unit in pixel
    UINT32  StartY;     // Unit in pixel
    UINT32  Width;      // Unit in pixel
    UINT32  Height;     // Unit in pixel
} amba_img_dsp_win_geometry_t;

typedef struct amba_img_dsp_win_dimension_s{
    UINT32  Width;      // Unit in pixel
    UINT32  Height;     // Unit in pixel
} amba_img_dsp_win_dimension_t;

typedef struct amba_img_dsp_win_scale_info_s{
    UINT8   ScaleFlag;
    UINT8   Reserved;
    UINT16  WidthIn;
    UINT16  HeightIn;
    UINT16  WidthOut;
    UINT16  HeightOut;
} amba_img_dsp_win_scale_info_t;

typedef struct amba_img_dsp_warp_flip_info_s {
    UINT16   HorizontalEnable;
    UINT16   VerticalEnable;
} amba_img_dsp_warp_flip_info_t;

#endif  /* _AMBA_DSP_IMG_DEF_H_ */
