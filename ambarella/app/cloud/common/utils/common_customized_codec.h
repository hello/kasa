/**
 * common_customized_codec.h
 *
 * History:
 *  2014/02/19 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

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

