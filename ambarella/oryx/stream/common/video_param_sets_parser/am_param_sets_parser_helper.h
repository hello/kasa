/*******************************************************************************
 * am_param_sets_parser_helper.h
 *
 * History:
 *   2015-11-11 - [ccjing] created file
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
#ifndef AM_PARAM_SETS_PARSER_HELPER_H_
#define AM_PARAM_SETS_PARSER_HELPER_H_
#include "am_base_include.h"
#include "h265.h"
struct GetBitContext
{
    const uint8_t *buffer;
    const uint8_t *buffer_end;
    uint32_t index;
    uint32_t size_in_bits;
    uint32_t size_in_bits_plus8;
};

struct AVRational
{
    int32_t num; ///< numerator
    int32_t den; ///< denominator
};

const uint8_t simple_log2_table[256] =
{ 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 };

const uint8_t simple_golomb_vlc_len[512] =
{ 19, 17, 15, 15, 13, 13, 13, 13, 11, 11, 11, 11, 11, 11, 11, 11, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

const uint8_t simple_ue_golomb_vlc_code[512] =
{ 32, 32, 32, 32, 32, 32, 32, 32, 31, 32, 32, 32, 32, 32, 32, 32, 15, 16, 17,
  18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 7, 7, 7, 7, 8, 8, 8, 8, 9,
  9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13, 14,
  14, 14, 14, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const int8_t simple_se_golomb_vlc_code[512] =
{ 17, 17, 17, 17, 17, 17, 17, 17, 16, 17, 17, 17, 17, 17, 17, 17, 8, -8, 9, -9,
  10, -10, 11, -11, 12, -12, 13, -13, 14, -14, 15, -15, 4, 4, 4, 4, -4, -4, -4,
  -4, 5, 5, 5, 5, -5, -5, -5, -5, 6, 6, 6, 6, -6, -6, -6, -6, 7, 7, 7, 7, -7,
  -7, -7, -7, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
  -3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const uint8_t default_scaling4[2][16] =
{
{ 6, 13, 20, 28, 13, 20, 28, 32, 20, 28, 32, 37, 28, 32, 37, 42 },
  { 10, 14, 20, 24, 14, 20, 24, 27, 20, 24, 27, 30, 24, 27, 30, 34 } };

const uint8_t default_scaling8[2][64] =
{
{ 6, 10, 13, 16, 18, 23, 25, 27, 10, 11, 16, 18, 23, 25, 27, 29, 13, 16, 18, 23,
  25, 27, 29, 31, 16, 18, 23, 25, 27, 29, 31, 33, 18, 23, 25, 27, 29, 31, 33,
  36, 23, 25, 27, 29, 31, 33, 36, 38, 25, 27, 29, 31, 33, 36, 38, 40, 27, 29,
  31, 33, 36, 38, 40, 42 },

  { 9, 13, 15, 17, 19, 21, 22, 24, 13, 13, 17, 19, 21, 22, 24, 25, 15, 17, 19,
    21, 22, 24, 25, 27, 17, 19, 21, 22, 24, 25, 27, 28, 19, 21, 22, 24, 25, 27,
    28, 30, 21, 22, 24, 25, 27, 28, 30, 32, 22, 24, 25, 27, 28, 30, 32, 33, 24,
    25, 27, 28, 30, 32, 33, 35 } };

const uint8_t simple_zigzag_scan[16] =
{ 0 + 0 * 4, 1 + 0 * 4, 0 + 1 * 4, 0 + 2 * 4, 1 + 1 * 4, 2 + 0 * 4, 3 + 0 * 4, 2
      + 1 * 4,
  1 + 2 * 4, 0 + 3 * 4, 1 + 3 * 4, 2 + 2 * 4, 3 + 1 * 4, 3 + 2 * 4, 2 + 3 * 4, 3
      + 3 * 4, };

uint8_t simple_zigzag_direct[64] =
{ 0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40,
  48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29,
  22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54,
  47, 55, 62, 63 };

const uint8_t default_scaling_list_intra[] =
{ 16, 16, 16, 16, 17, 18, 21, 24, 16, 16, 16, 16, 17, 19, 22, 25, 16, 16, 17,
  18, 20, 22, 25, 29, 16, 16, 18, 21, 24, 27, 31, 36, 17, 17, 20, 24, 30, 35,
  41, 47, 18, 19, 22, 27, 35, 44, 54, 65, 21, 22, 25, 31, 41, 54, 70, 88, 24,
  25, 29, 36, 47, 65, 88, 115 };

const uint8_t default_scaling_list_inter[] =
{ 16, 16, 16, 16, 17, 18, 20, 24, 16, 16, 16, 17, 18, 20, 24, 25, 16, 16, 17,
  18, 20, 24, 25, 28, 16, 17, 18, 20, 24, 25, 28, 33, 17, 18, 20, 24, 25, 28,
  33, 41, 18, 20, 24, 25, 28, 33, 41, 54, 20, 24, 25, 28, 33, 41, 54, 71, 24,
  25, 28, 33, 41, 54, 71, 91 };

const uint8_t ff_hevc_diag_scan4x4_x[16] =
{ 0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 1, 2, 3, 2, 3, 3, };

const uint8_t ff_hevc_diag_scan4x4_y[16] =
{ 0, 1, 0, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 3, 2, 3, };

const uint8_t ff_hevc_diag_scan8x8_x[64] =
{ 0, 0, 1, 0, 1, 2, 0, 1, 2, 3, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4,
  5, 6, 0, 1, 2, 3, 4, 5, 6, 7, 1, 2, 3, 4, 5, 6, 7, 2, 3, 4, 5, 6, 7, 3, 4, 5,
  6, 7, 4, 5, 6, 7, 5, 6, 7, 6, 7, 7, };

const uint8_t ff_hevc_diag_scan8x8_y[64] =
{ 0, 1, 0, 2, 1, 0, 3, 2, 1, 0, 4, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0, 6, 5, 4, 3, 2,
  1, 0, 7, 6, 5, 4, 3, 2, 1, 0, 7, 6, 5, 4, 3, 2, 1, 7, 6, 5, 4, 3, 2, 7, 6, 5,
  4, 3, 7, 6, 5, 4, 7, 6, 5, 7, 6, 7, };

const AVRational vui_sar[] =
{
  { 0, 1 },
  { 1, 1 },
  { 12, 11 },
  { 10, 11 },
  { 16, 11 },
  { 40, 33 },
  { 24, 11 },
  { 20, 11 },
  { 32, 11 },
  { 80, 33 },
  { 18, 11 },
  { 15, 11 },
  { 64, 33 },
  { 160, 99 },
  { 4, 3 },
  { 3, 2 },
  { 2, 1 }, };

const uint8_t zigzag_scan[16+1] = {
                           0 + 0 * 4, 1 + 0 * 4, 0 + 1 * 4, 0 + 2 * 4,
                           1 + 1 * 4, 2 + 0 * 4, 3 + 0 * 4, 2 + 1 * 4,
                           1 + 2 * 4, 0 + 3 * 4, 1 + 3 * 4, 2 + 2 * 4,
                           3 + 1 * 4, 3 + 2 * 4, 2 + 3 * 4, 3 + 3 * 4,
};

const uint8_t ff_zigzag_direct[64] = {
                                      0,   1,  8, 16,  9,  2,  3, 10,
                                      17, 24, 32, 25, 18, 11,  4,  5,
                                      12, 19, 26, 33, 40, 48, 41, 34,
                                      27, 20, 13,  6,  7, 14, 21, 28,
                                      35, 42, 49, 56, 57, 50, 43, 36,
                                      29, 22, 15, 23, 30, 37, 44, 51,
                                      58, 59, 52, 45, 38, 31, 39, 46,
                                      53, 60, 61, 54, 47, 55, 62, 63
};

const AVRational pixel_aspect[17] = {
                                     {   0,  1 },
                                     {   1,  1 },
                                     {  12, 11 },
                                     {  10, 11 },
                                     {  16, 11 },
                                     {  40, 33 },
                                     {  24, 11 },
                                     {  20, 11 },
                                     {  32, 11 },
                                     {  80, 33 },
                                     {  18, 11 },
                                     {  15, 11 },
                                     {  64, 33 },
                                     { 160, 99 },
                                     {   4,  3 },
                                     {   3,  2 },
                                     {   2,  1 },
};

#define QP(qP, depth) ((qP) + 6 * ((depth) - 8))

#define CHROMA_QP_TABLE_END(d)                                          \
        QP(0,  d), QP(1,  d), QP(2,  d), QP(3,  d), QP(4,  d), QP(5,  d),   \
        QP(6,  d), QP(7,  d), QP(8,  d), QP(9,  d), QP(10, d), QP(11, d),   \
        QP(12, d), QP(13, d), QP(14, d), QP(15, d), QP(16, d), QP(17, d),   \
        QP(18, d), QP(19, d), QP(20, d), QP(21, d), QP(22, d), QP(23, d),   \
        QP(24, d), QP(25, d), QP(26, d), QP(27, d), QP(28, d), QP(29, d),   \
        QP(29, d), QP(30, d), QP(31, d), QP(32, d), QP(32, d), QP(33, d),   \
        QP(34, d), QP(34, d), QP(35, d), QP(35, d), QP(36, d), QP(36, d),   \
        QP(37, d), QP(37, d), QP(37, d), QP(38, d), QP(38, d), QP(38, d),   \
        QP(39, d), QP(39, d), QP(39, d), QP(39, d)


#ifndef QP_MAX_NUM
#define QP_MAX_NUM   (51 + 6*6)
#endif
const uint8_t ff_h264_chroma_qp[7][QP_MAX_NUM + 1] =
{
{ CHROMA_QP_TABLE_END(8) },
  { 0, 1, 2, 3, 4, 5, CHROMA_QP_TABLE_END(9) },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, CHROMA_QP_TABLE_END(10) },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    CHROMA_QP_TABLE_END(11) },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, CHROMA_QP_TABLE_END(12) },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, CHROMA_QP_TABLE_END(13) },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    CHROMA_QP_TABLE_END(14) }, };

#define MIN_CACHE_BITS 25

#define READ_LE_32(x)                 \
  ((((const uint8_t*)(x))[3] << 24) | \
  (((const uint8_t*)(x))[2] << 16)  | \
  (((const uint8_t*)(x))[1] <<  8)  | \
  ((const uint8_t*)(x))[0])

#define READ_BE_32(x)                 \
  ((((const uint8_t*)(x))[0] << 24) | \
  (((const uint8_t*)(x))[1] << 16)  | \
  (((const uint8_t*)(x))[2] <<  8)  | \
  ((const uint8_t*)(x))[3])
/* load gb into local variables */
#define OPEN_READER(name, gb)      \
  uint32_t name##_index = (gb)->index; \
  uint32_t name##_cache  =   0
/* store local vars in gb */
#define CLOSE_READER(name, gb) \
  (gb)->index = name##_index; \
  name##_cache = name##_cache - 1;
/* UPDATE_CACHE(name, gb)
 * Refill the internal cache from the bitstream.
 * After this call at least MIN_CACHE_BITS will be available
 * always big-endian for ambarella
 */
#define UPDATE_CACHE(name, gb) \
  name##_cache = READ_BE_32(((const uint8_t *)(gb)->buffer)+(name##_index>>3)) \
  << (name##_index&0x07)
/* Will remove the next num bits from the cache (note SKIP_COUNTER
 * MUST be called before UPDATE_CACHE / CLOSE_READER).
 * alway big endian for ambarella
 */
#define SKIP_CACHE(name, gb, num) name##_cache <<= (num)
/* Will increment the internal bit counter (see SKIP_CACHE & SKIP_BITS). */
#define SKIP_COUNTER(name, gb, num) name##_index += (num)
/* Will skip over the next num bits.
 * Note, this is equivalent to SKIP_CACHE; SKIP_COUNTER
 */
#define SKIP_BITS(name, gb, num) do { \
    SKIP_CACHE(name, gb, num);        \
    SKIP_COUNTER(name, gb, num);      \
} while (0)
/* Like SKIP_BITS, to be used if next call is UPDATE_CACHE or CLOSE_READER. */
#define LAST_SKIP_BITS(name, gb, num) SKIP_COUNTER(name, gb, num)
#define NEG_USR32(a, s) (((uint32_t)(a))>>(32-(s)))
#define NEG_SSR32(a, s) (((int32_t)(a))>>(32-(s)))
/* Will return the next num bits*/
#define SHOW_UBITS(name, gb, num) NEG_USR32(name##_cache, num)
#define SHOW_SBITS(name, gb, num) NEG_SSR32(name##_cache, num)
/* Will output the contents of the internal cache,
 * next bit is MSB of 32 or 64 bit (FIXME 64bit).
 */
#define GET_CACHE(name, gb) ((uint32_t)name##_cache)



inline int32_t get_bit_count(const GetBitContext *s)
{
  return s->index;
}

inline void skip_bits_long(GetBitContext *s, int n)
{
  s->index += n;
}

inline int32_t get_sbits(GetBitContext *s, int n)
{
  register int tmp;
  OPEN_READER(re, s);
  UPDATE_CACHE(re, s);
  tmp = SHOW_SBITS(re, s, n);
  LAST_SKIP_BITS(re, s, n);
  CLOSE_READER(re, s);
  return tmp;
}
/*
 * Read 1-25 bits.
 */
inline uint32_t get_bits(GetBitContext *s, int n)
{
  register uint32_t tmp;
  OPEN_READER(re, s);
  UPDATE_CACHE(re, s);
  tmp = SHOW_UBITS(re, s, n);
  LAST_SKIP_BITS(re, s, n);
  CLOSE_READER(re, s);
  return tmp;
}
/*
 * Show 1-25 bits.
 */
inline uint32_t show_bits(GetBitContext *s, int n)
{
  register int tmp;
  OPEN_READER(re, s);
  UPDATE_CACHE(re, s);
  tmp = SHOW_UBITS(re, s, n);
  return tmp;
}

inline void skip_bits(GetBitContext *s, int n)
{
  OPEN_READER(re, s);
  LAST_SKIP_BITS(re, s, n);
  CLOSE_READER(re, s);
}

inline uint32_t get_bits1(GetBitContext *s)
{
  uint32_t index = s->index;
  uint8_t result = s->buffer[index >> 3];

  result <<= index & 7;
  result >>= 8 - 1;

  index ++;
  s->index = index;

  return result;
}
/*
 * Read 0-32 bits.
 */
inline uint32_t get_bits_long(GetBitContext *s, int n)
{
  if (!n) {
    return 0;
  } else if (n <= MIN_CACHE_BITS) {
    return get_bits(s, n);
  } else {
    unsigned ret = get_bits(s, 16) << (n - 16);
    return ret | get_bits(s, n - 16);
  }
}
/*
 * Read 0-64 bits.
 */
inline uint64_t get_bits64(GetBitContext *s, int n)
{
  if (n <= 32) {
    return get_bits_long(s, n);
  } else {
    uint64_t ret = (uint64_t) get_bits_long(s, n - 32) << 32;
    return ret | get_bits_long(s, 32);
  }
}

inline int32_t _sign_extend(int val, unsigned bits)
{
  unsigned shift = 8 * sizeof(int) - bits;
  union { unsigned u; int s; } v = { (unsigned) val << shift };
  return v.s >> shift;
}
/*
 * Read 0-32 bits as a signed integer.
 */
inline int32_t get_sbits_long(GetBitContext *s, int n)
{
  return _sign_extend(get_bits_long(s, n), n);
}

/*
 * Show 0-32 bits.
 */
inline uint32_t show_bits_long(GetBitContext *s, int n)
{
  if (n <= MIN_CACHE_BITS) {
    return show_bits(s, n);
  } else {
    GetBitContext gb = *s;
    return get_bits_long(&gb, n);
  }
}

inline int32_t _simple_log2_c(uint32_t v)
{
  int n = 0;
  if (v & 0xffff0000) {
    v >>= 16;
    n += 16;
  }
  if (v & 0xff00) {
    v >>= 8;
    n += 8;
  }
  n += simple_log2_table[v];

  return n;
}
/*
 * read unsigned exp golomb code
 */
inline uint32_t get_ue_golomb(GetBitContext *gb)
{
  uint32_t buf;
  int log;

  OPEN_READER(re, gb);
  UPDATE_CACHE(re, gb);
  buf = GET_CACHE(re, gb);

  if (buf >= (1 << 27)) {
    buf >>= 32 - 9;
    LAST_SKIP_BITS(re, gb, simple_golomb_vlc_len[buf]);
    CLOSE_READER(re, gb);
    return simple_ue_golomb_vlc_code[buf];
  } else {
    log = 2 * _simple_log2_c(buf) - 31;
    buf >>= log;
    buf --;
    LAST_SKIP_BITS(re, gb, 32 - log);
    CLOSE_READER(re, gb);
    return buf;
  }
}
/*
 * Read an unsigned Exp-Golomb code in the range 0 to UINT32_MAX-1.
 */
inline uint32_t get_ue_golomb_long(GetBitContext *gb)
{
  uint32_t buf;
  int32_t log;

  buf = show_bits_long(gb, 32);
  log = 31 - _simple_log2_c(buf);
  skip_bits_long(gb, log);

  return get_bits_long(gb, log + 1) - 1;
}

/*
 * read unsigned exp golomb code, constraint to a max of 31.
 * the return value is undefined if the stored value exceeds 31.
 */
inline int32_t get_ue_golomb_31(GetBitContext *gb)
{
  uint32_t buf;
  OPEN_READER(re, gb);
  UPDATE_CACHE(re, gb);
  buf = GET_CACHE(re, gb);
  buf >>= 32 - 9;
  LAST_SKIP_BITS(re, gb, simple_golomb_vlc_len[buf]);
  CLOSE_READER(re, gb);

  return simple_ue_golomb_vlc_code[buf];
}
/*
 * read signed exp golomb code.
 */
inline int32_t get_se_golomb(GetBitContext *gb)
{
  uint32_t buf;
  int log;
  OPEN_READER(re, gb);
  UPDATE_CACHE(re, gb);
  buf = GET_CACHE(re, gb);
  if (buf >= (1 << 27)) {
    buf >>= 32 - 9;
    LAST_SKIP_BITS(re, gb, simple_golomb_vlc_len[buf]);
    CLOSE_READER(re, gb);

    return simple_se_golomb_vlc_code[buf];
  } else {
    log = _simple_log2_c(buf);
    LAST_SKIP_BITS(re, gb, 31 - log);
    UPDATE_CACHE(re, gb);
    buf = GET_CACHE(re, gb);
    buf >>= log;
    LAST_SKIP_BITS(re, gb, 32 - log);
    CLOSE_READER(re, gb);
    if (buf & 1) {
      buf = -(buf >> 1);
    } else {
      buf = (buf >> 1);
    }
    return buf;
  }
}

inline int32_t get_bits_left(GetBitContext *gb)
{
  return gb->size_in_bits - get_bit_count(gb);
}

#endif /* AM_PARAM_SETS_PARSER_HELPER_H_ */
