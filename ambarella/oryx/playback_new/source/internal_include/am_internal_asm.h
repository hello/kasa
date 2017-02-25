/**
 * am_internal_asm.h
 *
 * History:
 *    2015/09/15 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
 */

#ifndef __AM_INTERNAL_ASM_H__
#define __AM_INTERNAL_ASM_H__

typedef struct
{
  unsigned char *src_y;
  unsigned char *src_cb;
  unsigned char *src_cr;
  unsigned char *des;
  unsigned int src_stride_y;
  unsigned int src_stride_cbcr;
  unsigned int des_stride;
  unsigned int src_width;
  unsigned int src_height;
} SASMArguYUV420p2VYU;

typedef struct
{
    unsigned char *dst;
    unsigned char *src;
    unsigned int width;
    unsigned int height;
    unsigned int dst_pitch;
    unsigned int src_pitch;
} SASMArguGenerateMe1Pitch;

typedef struct
{
    unsigned char *y_dst;
    unsigned char *uv_dst;
    unsigned char *src;
    unsigned int width;
    unsigned int height;
    unsigned int dst_y_pitch;
    unsigned int dst_uv_pitch;
    unsigned int src_pitch;
} SASMArguCopyYUYVtoNV12;

//#ifdef __cplusplus
extern "C" {
  //#endif

  void asm_neon_yuv420p_to_vyu565(SASMArguYUV420p2VYU *params);
  void asm_neon_yuv420p_to_avyu8888(SASMArguYUV420p2VYU *params);
  void asm_neon_yuv420p_to_ayuv8888(SASMArguYUV420p2VYU *params);
  void asm_neon_yuv420p_to_ayuv8888_ori(SASMArguYUV420p2VYU *params);

void asm_neon_generate_me1_pitch(SASMArguGenerateMe1Pitch *params);

void asm_neon_copy_yuyv_to_nv12(SASMArguCopyYUYVtoNV12 *params);

  //#ifdef __cplusplus
}
//#endif

#endif

