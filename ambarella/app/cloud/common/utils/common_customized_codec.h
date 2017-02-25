/*******************************************************************************
 * common_customized_codec.h
 *
 * History:
 *  2014/02/19 - [Zhi He] create file
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

#ifndef __COMMON_CUSTOMIZED_CODEC_H__
#define __COMMON_CUSTOMIZED_CODEC_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

typedef struct {
    TU8 ad_bit_depth : 4;
    TU8 skip_bits : 2;
    TU8 merge_factor : 2;

    TU8 quant_param : 4;
    TU8 quant_pattern : 4;
} SCustomizedADPCMHeader;

class CCustomizedADPCMCodec: public ICustomizedCodec
{
public:
    static ICustomizedCodec *Create(TUint index);

protected:
    CCustomizedADPCMCodec(TUint index);
    virtual ~CCustomizedADPCMCodec();
    EECode Construct();

public:
    virtual EECode ConfigCodec(TU32 param1, TU32 param2, TU32 param3, TU32 param4, TU32 param5);
    virtual EECode QueryInOutBufferSize(TMemSize &encoder_input_buffer_size, TMemSize &encoder_output_buffer_size) const;

public:
    virtual EECode Encoding(void *in_buffer, void *out_buffer, TMemSize in_data_size, TMemSize &out_data_size);
    virtual EECode Decoding(void *in_buffer, void *out_buffer, TMemSize in_data_size, TMemSize &out_data_size);

public:
    virtual void Destroy();

private:
    void encodeGroupSample(TU16 &ref_value, TU16 *p_in, TU8 *p_out);
    void encodeFirstGroupSample(TU16 &ref_value, TU16 *p_in, TU8 *p_out);
    void decodeGroupSample(TU16 &ref_value, TU8 *p_in, TU16 *p_out);
    void decodeFirstGroupSample(TU16 &ref_value, TU8 *p_in, TU16 *p_out);

private:
    TMemSize mEncoderInputBufferSize;
    TMemSize mEncoderOutputBufferSize;

private:
    TUint mAdBitDepth;
    TUint mSkipBits;
    TUint mMergeFactor;
    TUint mQuantParam;
    TUint mQuantPattern;

    TU16 mIndex;
    SCustomizedADPCMHeader mHeader;

private:
    TU8 mbModeVersion;//
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;
};


DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

