/*******************************************************************************
 * codec_misc.h
 *
 * History:
 *    2014/08/26 - [Zhi He] create file
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

#ifndef __CODEC_MISC_H__
#define __CODEC_MISC_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

enum {
    H264_FMT_INVALID = 0,
    H264_FMT_AVCC,      // nal delimit: 4 byte, which represents data length
    H264_FMT_ANNEXB,     // nal delimiter: 00 00 00 01
};

enum {
    EH264SliceType_P = 0,
    EH264SliceType_B = 1,
    EH264SliceType_I = 2,
    EH264SliceType_SP = 3,
    EH264SliceType_SI = 4,

    EH264SliceType_FieldOffset = 5,
    EH264SliceType_MaxValue = 9,
};

typedef struct {
#ifdef DCONFIG_BIG_ENDIAN
    TU8 syncword_11to4 : 8;
    TU8 syncword_3to0 : 4;
    TU8 id : 1;
    TU8 layer : 2;
    TU8 protection_absent : 1;
    TU8 profile : 2;
    TU8 sampling_frequency_index : 4;
    TU8 private_bit : 1;
    TU8 channel_configuration_2 : 1;
    TU8 channel_configuration_1to0 : 2;
    TU8 orignal_copy : 1;
    TU8 home : 1;
    TU8 reserved : 4;
#else
    TU8 syncword_11to4 : 8;
    TU8 protection_absent : 1;
    TU8 layer : 2;
    TU8 id : 1;
    TU8 syncword_3to0 : 4;
    TU8 channel_configuration_2 : 1;
    TU8 private_bit : 1;
    TU8 sampling_frequency_index : 4;
    TU8 profile : 2;
    TU8 reserved : 4;
    TU8 home : 1;
    TU8 orignal_copy : 1;
    TU8 channel_configuration_1to0 : 2;
#endif
} SADTSFixedHeader;

TU8 *gfNALUFindNextStartCode(TU8 *p, TU32 len);
TU8 *gfNALUFindIDR(TU8 *p, TU32 len);
TU8 *gfNALUFindSPSHeader(TU8 *p, TU32 len);
TU8 *gfNALUFindPPSEnd(TU8 *p, TU32 len);
void gfFindH264SpsPpsIdr(TU8 *data_base, TInt data_size, TU8 &has_sps, TU8 &has_pps, TU8 &has_idr, TU8 *&p_sps, TU8 *&p_pps, TU8 *&p_pps_end, TU8 *&p_idr);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif
