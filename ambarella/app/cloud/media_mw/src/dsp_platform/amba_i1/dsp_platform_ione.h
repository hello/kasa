/**
 * dsp_platform_interface.h
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __DSP_PLATFORM_IONE_H__
#define __DSP_PLATFORM_IONE_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

class CIOneDuplexDecAPI: public CObject, public IDSPDecAPI
{
    typedef CObject inherited;

public:
    virtual EECode InitDecoder(TUint &dec_id, SDecoderParam *param, TUint vout_start_index, TUint number_vout, const volatile SDSPVoutConfig *vout_config);
    virtual EECode ReleaseDecoder(TUint dec_id);

    virtual EECode RequestBitStreamBuffer(TUint dec_id, TU8 *pstart, TUint room);
    virtual EECode Decode(TUint dec_id, TU8 *pstart, TU8 *pend, TU32 number_of_frames = 1);
    virtual EECode Stop(TUint dec_id, TUint flag);

    virtual EECode PbSpeed(TUint dec_id, TU8 direction, TU8 feeding_rule, TU16 speed);
    virtual EECode QueryStatus(TUint dec_id, SDecStatus *status);

    virtual EECode TuningPB(TUint dec_id, TU8 fw, TU16 frame_tick);

public:
    CIOneDuplexDecAPI(TInt fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config);
    virtual ~CIOneDuplexDecAPI();

private:
    TInt mIavFd;
    EDSPOperationMode mDSPMode;
    StreamFormat mDecType;
    TU16 mDSPDecType;
    TU16 mTotalStreamNumber;

    const volatile SPersistMediaConfig *mpConfig;
};

class CIOneUdecDecAPI: public CObject, public IDSPDecAPI
{
    typedef CObject inherited;

public:
    virtual EECode InitDecoder(TUint &dec_id, SDecoderParam *param, TUint vout_start_index, TUint number_vout, const volatile SDSPVoutConfig *vout_config);
    virtual EECode ReleaseDecoder(TUint dec_id);

    virtual EECode RequestBitStreamBuffer(TUint dec_id, TU8 *pstart, TUint room);
    virtual EECode Decode(TUint dec_id, TU8 *pstart, TU8 *pend, TU32 number_of_frames = 1);
    virtual EECode Stop(TUint dec_id, TUint stop_flag);

    virtual EECode PbSpeed(TUint dec_id, TU8 direction, TU8 feeding_rule, TU16 speed);
    virtual EECode QueryStatus(TUint dec_id, SDecStatus *status);

    virtual EECode TuningPB(TUint dec_id, TU8 fw, TU16 frame_tick);


public:
    CIOneUdecDecAPI(TInt fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config);
    virtual ~CIOneUdecDecAPI();

private:
    TInt mIavFd;
    EDSPOperationMode mDSPMode;
    StreamFormat mDecType;
    TU16 mDSPDecType;
    TU16 mTotalStreamNumber;

private:
    const volatile SPersistMediaConfig *mpConfig;

};

class CIOneMUdecEncAPI: public CObject, public IDSPEncAPI
{
    typedef CObject inherited;

public:
    virtual EECode InitEncoder(TUint &enc_id, SEncoderParam *param);
    virtual EECode ReleaseEncoder(TUint enc_id);

    virtual EECode GetBitStreamBuffer(TUint enc_id, SBitDescs *p_desc);
    virtual EECode Start(TUint enc_id);
    virtual EECode Stop(TUint enc_id, TUint stop_flag);

public:
    CIOneMUdecEncAPI(TInt fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config);
    virtual ~CIOneMUdecEncAPI();

private:
    TInt mIavFd;
    EDSPOperationMode mDSPMode;

    TU16 mDSPEncType;//driver use

    TU8 mMainStreamEnabled;
    TU8 mSecondStreamEnabled;

    const volatile SPersistMediaConfig *mpConfig;
};

class CIOneDuplexEncAPI: public CObject, public IDSPEncAPI
{
    typedef CObject inherited;

public:
    virtual EECode InitEncoder(TUint &enc_id, SEncoderParam *param);
    virtual EECode ReleaseEncoder(TUint enc_id);

    virtual EECode GetBitStreamBuffer(TUint enc_id, SBitDescs *p_desc);
    virtual EECode Start(TUint enc_id);
    virtual EECode Stop(TUint enc_id, TUint stop_flag);

public:
    CIOneDuplexEncAPI(TInt fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config);
    virtual ~CIOneDuplexEncAPI();

private:
    TInt mIavFd;
    EDSPOperationMode mDSPMode;

    TU16 mDSPEncType;//driver use

    TU8 mMainStreamEnabled;
    TU8 mSecondStreamEnabled;

    const volatile SPersistMediaConfig *mpConfig;
};

class CIOneRecordEncAPI: public CObject, public IDSPEncAPI
{
    typedef CObject inherited;

public:
    virtual EECode InitEncoder(TUint &enc_id, SEncoderParam *param);
    virtual EECode ReleaseEncoder(TUint enc_id);

    virtual EECode GetBitStreamBuffer(TUint enc_id, SBitDescs *p_desc);
    virtual EECode Start(TUint enc_id);
    virtual EECode Stop(TUint enc_id, TUint stop_flag);

public:
    CIOneRecordEncAPI(TInt fd, EDSPOperationMode dsp_mode, const volatile SPersistMediaConfig *p_config);
    virtual ~CIOneRecordEncAPI();

private:
    TInt mIavFd;

    EDSPOperationMode mDSPMode;
    TU16 mDSPEncType;//driver use
    TU16 mTotalStreamNumber;
    TU16 reserved0[2];

    TUint mStreamMask;
    TUint mEosFlag;

    const volatile SPersistMediaConfig *mpConfig;
};

class CIOneDSPAPI: public CObject, public IDSPAPI
{
    typedef CObject inherited;
public:
    CIOneDSPAPI(const volatile SPersistMediaConfig *p_config);
    virtual ~CIOneDSPAPI();

public:
    virtual EECode OpenDevice(TInt &fd);
    virtual EECode CloseDevice();

    virtual EECode QueryVoutSettings(volatile SDSPConfig *config) const;
    virtual EECode DSPModeConfig(const volatile SPersistMediaConfig *config);
    virtual EECode EnterDSPMode(TUint mode);

    //some direct api to iav/dsp
    virtual EECode DSPControl(EDSPControlType type, void *p_params);

    virtual EDSPOperationMode QueryCurrentDSPMode() const;
    virtual EDSPPlatform QueryDSPPlatform() const;

private:
    EECode enterUdecMode();
    EECode enterMudecMode();
    void updateCurrentDSPMode();

private:
    TInt mIavFd;
    EDSPOperationMode mDSPMode;
    //TUint mRequestVoutMask;

private:
    const volatile SPersistMediaConfig *mpConfig;
    const volatile SDSPConfig *mpDSPConfig;
};

//extern IDSPDecAPI* gfCreateIOneEncAPI(TInt iav_fd, TUint dsp_mode, TUint codec_type, const volatile SPersistMediaConfig* p_config);
//extern IDSPDecAPI* gfCreateIOneDecAPI(TInt iav_fd, TUint dsp_mode, TUint codec_type, const volatile SPersistMediaConfig* p_config);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

