/*******************************************************************************
 * amba_video_decoder.cpp
 *
 * History:
 *    2012/08/03 - [Zhi He] create file
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

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"
#include "codec_misc.h"

#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "amba_video_encoder.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//#define DLOCAL_DEBUG_VERBOSE

IVideoEncoder *gfCreateAmbaVideoEncoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CAmbaVideoEncoder::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

//-----------------------------------------------------------------------
//
// CAmbaVideoEncoder
//
//-----------------------------------------------------------------------
CAmbaVideoEncoder::CAmbaVideoEncoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mpDSPEncoder(NULL)
    , mEncoderIndex(0)
    , mDSPMode(EDSPOperationMode_Invalid)
    , mDSPPlatform(EDSPPlatform_Invalid)
    , mCodecFormat(StreamFormat_Invalid)
    , mFrameRate(29.97)
    , mBitrate(1024 * 1024)
    , mFrameRateReduceFactor(1)
    , mpBitstreamBufferStart(NULL)
    , mBitstreamBufferSize(0)
    , mCapMaxCodedWidth(0)
    , mCapMaxCodedHeight(0)
    , mCurrentFramerateNum(DDefaultVideoFramerateNum)
    , mCurrentFramerateDen(DDefaultVideoFramerateDen)
    , mbEncoderStarted(0)
    , mRemainingDescNumber(0)
    , mCurrentDescIndex(0)
    , mbCopyOut(0)
    , mbSendSyncBuffer(0)
    , mpVideoExtraData(NULL)
    , mVideoExtraDataSize(0)
    , mpH264spspps(NULL)
    , mpDumpFile(NULL)
    , mpDumpFileSeparate(NULL)
    , mDumpIndex(0)
    , mDumpStartFrame(0)
    , mDumpEndFrame(0xffffffff)
{
    memset(&mCurrentDesc, 0x0, sizeof(mCurrentDesc));
}

IVideoEncoder *CAmbaVideoEncoder::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CAmbaVideoEncoder *result = new CAmbaVideoEncoder(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        //        LOGM_ERROR("CAmbaVideoEncoder->Construct() fail\n");
        delete result;
        result = NULL;
    }
    return result;
}

void CAmbaVideoEncoder::Destroy()
{
    Delete();
}

EECode CAmbaVideoEncoder::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAmbaEncoder);

    return EECode_OK;
}

CAmbaVideoEncoder::~CAmbaVideoEncoder()
{
    //LOGM_RESOURCE("~CAmbaVideoEncoder.\n");
    if (mpVideoExtraData) {
        free(mpVideoExtraData);
        mpVideoExtraData = NULL;
    }

    if (mpH264spspps) {
        free(mpH264spspps);
        mpH264spspps = NULL;
    }
    //LOGM_RESOURCE("~CAmbaVideoEncoder done.\n");
}

EECode CAmbaVideoEncoder::SetupContext(SVideoEncoderInputParam *param)
{
    TInt iav_fd;
    EECode err = EECode_OK;

    DASSERT(mpPersistMediaConfig);

    if (param) {

        DASSERT(!mpDSPEncoder);
        if (mpDSPEncoder) {
            LOGM_ERROR(" already setup, clear previous, before setup new.\n");
            DestroyContext();
        }

        mIndex = param->index;
        mCodecFormat = param->format;
        mDSPMode = (EDSPOperationMode)param->prefer_dsp_mode;
        mDSPPlatform = param->platform;
        if (param->cap_max_width && param->cap_max_height) {
            mCapMaxCodedWidth = param->cap_max_width;
            mCapMaxCodedHeight = param->cap_max_height;
        } else {
            mCapMaxCodedWidth = 1280;
            mCapMaxCodedHeight = 720;
            LOGM_WARN("max coded size not specified, use default %u x %u\n", mCapMaxCodedWidth, mCapMaxCodedHeight);
        }

        mFrameRate = param->framerate;
        mCurrentFramerateDen = mpPersistMediaConfig->dsp_config.modeConfigMudec.frame_rate_in_tick;
        mFrameRateReduceFactor = 1;//hardcode here
        mBitrate = param->bitrate;

        iav_fd = mpPersistMediaConfig->dsp_config.device_fd;
        LOGM_INFO("before gfDSPEncAPIFactory, mIndex %d, iav_fd %d, mCodecFormat %d, mDSPMode %d, mDSPPlatform %d, mCapMaxCodedWidth %d, mCapMaxCodedHeight %d\n", mIndex, iav_fd, (TInt)mCodecFormat, (TInt)mDSPMode, (TInt)mDSPPlatform, mCapMaxCodedWidth, mCapMaxCodedHeight);

        mpDSPEncoder = gfDSPEncAPIFactory(iav_fd, mDSPMode, mCodecFormat, mpPersistMediaConfig);

        LOGM_INFO("after gfDSPEncAPIFactory, mpDSPEncoder %p\n", mpDSPEncoder);

        if (mpDSPEncoder) {
            LOGM_INFO("before InitEncoder\n");
            SEncoderParam params;

            memset(&params, 0x0, sizeof(params));
            params.profile = 0;
            params.level = 0;
            params.main_width = mCapMaxCodedWidth;
            params.main_height = mCapMaxCodedHeight;
            params.enc_width = params.main_width;
            params.enc_height = params.main_height;
            params.enc_offset_x = 0;
            params.enc_offset_y = 0;
            params.M = 1;
            params.N = 30;
            params.idr_interval = 4;
            params.gop_structure = 0;
            params.numRef_P = 1;
            params.numRef_B = 2;
            params.use_cabac = 1;
            params.bitrate = mBitrate;
            params.quality_level = 0x83;
            params.vbr_setting = 1;// 1 - CBR, 2 - Pseudo CBR, 3 - VBR

            err = mpDSPEncoder->InitEncoder(mEncoderIndex, &params);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_FATAL("InitEncoder fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
            }

            mpBitstreamBufferStart = params.bits_fifo_start;
            mBitstreamBufferSize = params.bits_fifo_size / 2;
            LOGM_INFO("after InitEncoder, mEncoderIndex %d, mpBitstreamBufferStart %p, mBitstreamBufferSize %d\n", mEncoderIndex, mpBitstreamBufferStart, mBitstreamBufferSize);

        } else {
            LOGM_FATAL("NULL mpDSPEncoder, mDSPPlatform %d, mDSPMode %d\n", mDSPPlatform, mDSPMode);
            return EECode_InternalLogicalBug;
        }

        LOGM_INFO("[DEBUG video info]: mCurrentFramerateNum %d, mCurrentFramerateDen %d\n", mCurrentFramerateNum, mCurrentFramerateDen);
        err = Start();
        if (DUnlikely(EECode_OK != err)) {
            LOGM_FATAL("Encoder Start fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
            return EECode_BadState;
        }
    } else {
        LOGM_FATAL("NULL input in CAmbaVideoEncoder::SetupContext\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

void CAmbaVideoEncoder::DestroyContext()
{
    if (mpDSPEncoder) {
        mpDSPEncoder->ReleaseEncoder(mIndex);
        mpDSPEncoder->Delete();
        mpDSPEncoder = NULL;
    }
}

EECode CAmbaVideoEncoder::SetBufferPool(IBufferPool *p_buffer_pool)
{
    return EECode_OK;
}

EECode CAmbaVideoEncoder::Start()
{
    //debug assert
    DASSERT(mpDSPEncoder);
    if (DLikely(mpDSPEncoder)) {
        if (DLikely(!mbEncoderStarted)) {
            mpDSPEncoder->Start(mEncoderIndex);
            mbEncoderStarted = 1;
        } else {
            LOGM_WARN("already started\n");
        }
        return EECode_OK;
    }

    return EECode_BadState;
}

EECode CAmbaVideoEncoder::Stop()
{
    //debug assert
    DASSERT(mpDSPEncoder);
    if (DLikely(mpDSPEncoder)) {
        if (DLikely(mbEncoderStarted)) {
            mpDSPEncoder->Stop(mEncoderIndex, 0);
            mbEncoderStarted = 0;
        } else {
            LOGM_WARN("already stopped, or not started yet\n");
        }
        return EECode_OK;
    }

    return EECode_BadState;
}

EECode CAmbaVideoEncoder::Encode(CIBuffer *in_buffer, CIBuffer *out_buffer, TU32 &current_remaining, TU32 &all_cached_frames)
{
    DASSERT(mpDSPEncoder);
    DASSERT(!in_buffer);
    DASSERT(out_buffer);

    mDebugHeartBeat ++;
    current_remaining = 0;
    all_cached_frames = 0;

    if (DLikely(mpDSPEncoder && out_buffer)) {

        if (DLikely(mRemainingDescNumber)) {
            TU8 *p_data_start = NULL;
            TMemSize data_size = 0;
            TMemSize skip_size = 0;
            if (DLikely(!mbCopyOut)) {
                p_data_start = mCurrentDesc.desc[mCurrentDescIndex].pstart;
                data_size = mCurrentDesc.desc[mCurrentDescIndex].size;
#if 0
                LOGM_PRINTF("size %ld, %02x %02x %02x %02x, %02x %02x %02x %2x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n", \
                            data_size, p_data_start[0], p_data_start[1], p_data_start[2], p_data_start[3], \
                            p_data_start[4], p_data_start[5], p_data_start[6], p_data_start[7], \
                            p_data_start[8], p_data_start[9], p_data_start[10], p_data_start[11], \
                            p_data_start[12], p_data_start[13], p_data_start[14], p_data_start[15], \
                            p_data_start[16], p_data_start[17], p_data_start[18], p_data_start[19], \
                            p_data_start[20], p_data_start[21], p_data_start[22], p_data_start[23], \
                            p_data_start[24], p_data_start[25], p_data_start[26], p_data_start[27], \
                            p_data_start[28], p_data_start[29], p_data_start[30], p_data_start[31]);
#endif
                skip_size = gfSkipDelimter(p_data_start);
                p_data_start += skip_size;
                data_size -= skip_size;
                out_buffer->SetDataPtr(p_data_start);
                out_buffer->SetDataSize(data_size);

                out_buffer->SetBufferPTS((TTime)mCurrentDesc.desc[mCurrentDescIndex].pts);
                out_buffer->SetBufferNativePTS((TTime)mCurrentDesc.desc[mCurrentDescIndex].pts);
            } else {
                DASSERT(out_buffer->GetDataPtrBase());
                DASSERT(out_buffer->GetDataMemorySize() > mCurrentDesc.desc[mCurrentDescIndex].size);
                p_data_start = mCurrentDesc.desc[mCurrentDescIndex].pstart;
                data_size = mCurrentDesc.desc[mCurrentDescIndex].size;
                skip_size = gfSkipDelimter(p_data_start);
                p_data_start += skip_size;
                data_size -= skip_size;
                memcpy(out_buffer->GetDataPtrBase(), p_data_start, data_size);
                out_buffer->SetDataPtr(out_buffer->GetDataPtrBase());
                out_buffer->SetDataSize(data_size);
                out_buffer->SetBufferPTS((TTime)mCurrentDesc.desc[mCurrentDescIndex].pts);
                out_buffer->SetBufferNativePTS((TTime)mCurrentDesc.desc[mCurrentDescIndex].pts);
            }
            out_buffer->SetBufferType(EBufferType_VideoES);
            if (EPredefinedPictureType_IDR == mCurrentDesc.desc[mCurrentDescIndex].pic_type) {
                out_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
                if ((0x01 == p_data_start[2]) && (ENalType_SPS == (0x1f & p_data_start[3]))) {
                    out_buffer->SetBufferFlags(CIBuffer::WITH_EXTRA_DATA);
                }
                if (DUnlikely(!mbSendSyncBuffer)) {
                    mbSendSyncBuffer = 1;
                    out_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
                    out_buffer->mVideoWidth = mCapMaxCodedWidth;
                    out_buffer->mVideoHeight = mCapMaxCodedHeight;
                    out_buffer->mVideoBitrate = mBitrate;
                    out_buffer->mVideoFrameRateNum = mCurrentFramerateNum;
                    out_buffer->mVideoFrameRateDen = mCurrentFramerateDen;
                    out_buffer->mVideoRoughFrameRate = mFrameRate;
                }
            } else {
                out_buffer->SetBufferFlags(0);
            }
            mCurrentDescIndex ++;
            mRemainingDescNumber --;
            return EECode_OK;
        } else {
            TU8 *p_data_start = NULL;
            TMemSize data_size = 0;
            TMemSize skip_size = 0;
            EECode err = mpDSPEncoder->GetBitStreamBuffer(mEncoderIndex, &mCurrentDesc);
            if (DUnlikely(EECode_OK != err)) {
                if (mpPersistMediaConfig->app_start_exit) {
                    LOGM_NOTICE("[program start exit]: GetBitStreamBuffer return %s\n", gfGetErrorCodeString(err));
                } else {
                    LOGM_ERROR("GetBitStreamBuffer return %s\n", gfGetErrorCodeString(err));
                }
                return err;
            }
            mCurrentDescIndex = 1;
            mRemainingDescNumber = mCurrentDesc.tot_desc_number - mCurrentDescIndex;
            if (DLikely(!mbCopyOut)) {
                p_data_start = mCurrentDesc.desc[0].pstart;
                data_size = mCurrentDesc.desc[0].size;
#if 0
                LOGM_PRINTF("size %ld, %02x %02x %02x %02x, %02x %02x %02x %2x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n", \
                            data_size, p_data_start[0], p_data_start[1], p_data_start[2], p_data_start[3], \
                            p_data_start[4], p_data_start[5], p_data_start[6], p_data_start[7], \
                            p_data_start[8], p_data_start[9], p_data_start[10], p_data_start[11], \
                            p_data_start[12], p_data_start[13], p_data_start[14], p_data_start[15], \
                            p_data_start[16], p_data_start[17], p_data_start[18], p_data_start[19], \
                            p_data_start[20], p_data_start[21], p_data_start[22], p_data_start[23], \
                            p_data_start[24], p_data_start[25], p_data_start[26], p_data_start[27], \
                            p_data_start[28], p_data_start[29], p_data_start[30], p_data_start[31]);
#endif
                skip_size = gfSkipDelimter(p_data_start);
                p_data_start += skip_size;
                data_size -= skip_size;
                out_buffer->SetDataPtr(p_data_start);
                out_buffer->SetDataSize(data_size);
                out_buffer->SetBufferPTS((TTime)mCurrentDesc.desc[0].pts);
                out_buffer->SetBufferNativePTS((TTime)mCurrentDesc.desc[0].pts);
            } else {
                DASSERT(out_buffer->GetDataPtrBase());
                DASSERT(out_buffer->GetDataMemorySize() > mCurrentDesc.desc[0].size);
                p_data_start = mCurrentDesc.desc[0].pstart;
                data_size = mCurrentDesc.desc[0].size;
#if 0
                LOGM_PRINTF("%02x %02x %02x %02x, %02x %02x %02x %2x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n", \
                            p_data_start[0], p_data_start[1], p_data_start[2], p_data_start[3], \
                            p_data_start[4], p_data_start[5], p_data_start[6], p_data_start[7], \
                            p_data_start[8], p_data_start[9], p_data_start[10], p_data_start[11], \
                            p_data_start[12], p_data_start[13], p_data_start[14], p_data_start[15], \
                            p_data_start[16], p_data_start[17], p_data_start[18], p_data_start[19], \
                            p_data_start[20], p_data_start[21], p_data_start[22], p_data_start[23], \
                            p_data_start[24], p_data_start[25], p_data_start[26], p_data_start[27], \
                            p_data_start[28], p_data_start[29], p_data_start[30], p_data_start[31]);
#endif
                skip_size = gfSkipDelimter(p_data_start);
                p_data_start += skip_size;
                data_size -= skip_size;
                memcpy(out_buffer->GetDataPtrBase(), p_data_start, data_size);
                out_buffer->SetDataPtr(out_buffer->GetDataPtrBase());
                out_buffer->SetDataSize(data_size);
                out_buffer->SetBufferPTS((TTime)mCurrentDesc.desc[0].pts);
                out_buffer->SetBufferNativePTS((TTime)mCurrentDesc.desc[0].pts);
            }
            out_buffer->SetBufferType(EBufferType_VideoES);
            if (EPredefinedPictureType_IDR == mCurrentDesc.desc[0].pic_type) {
                out_buffer->SetBufferFlags(CIBuffer::WITH_EXTRA_DATA | CIBuffer::KEY_FRAME);
                if (DUnlikely(!mbSendSyncBuffer)) {
                    mbSendSyncBuffer = 1;
                    out_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
                    out_buffer->mVideoWidth = mCapMaxCodedWidth;
                    out_buffer->mVideoHeight = mCapMaxCodedHeight;
                    out_buffer->mVideoBitrate = mBitrate;
                    out_buffer->mVideoFrameRateNum = mCurrentFramerateNum;
                    out_buffer->mVideoFrameRateDen = mCurrentFramerateDen;
                    out_buffer->mVideoRoughFrameRate = mFrameRate;
                }
            } else {
                out_buffer->SetBufferFlags(0);
            }
        }
        current_remaining = mRemainingDescNumber;
    } else {
        LOGM_ERROR("NULL mpDSPEncoder %p, or NULL out_buffer %p\n", mpDSPEncoder, out_buffer);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CAmbaVideoEncoder::VideoEncoderProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    EECode err = EECode_Error;

    if (property_type < EFilterPropertyType_video_parameters
            || property_type > EFilterPropertyType_video_gop
            || !set_or_get//only support set API in driver now
            || !p_param) {
        LOGM_FATAL("invalid input in CAmbaVideoEncoder::VideoEncoderProperty, property_type=%u, set_or_get=%u, p_param=%p\n", property_type, set_or_get, p_param);
        return EECode_BadParam;
    }

    DASSERT(mpPersistMediaConfig);
    DASSERT(mpPersistMediaConfig->dsp_config.p_dsp_handler);
    if (DLikely(mpPersistMediaConfig && mpPersistMediaConfig->dsp_config.p_dsp_handler)) {
        IDSPAPI *pDSPAPI = (IDSPAPI *)mpPersistMediaConfig->dsp_config.p_dsp_handler;
        EDSPControlType type;
        switch (property_type) {
            case EFilterPropertyType_video_parameters: {
                    type = EDSPControlType_UDEC_TRANSCODER_update_params;
                }
                break;
            case EFilterPropertyType_video_framerate: {
                    type = EDSPControlType_UDEC_TRANSCODER_update_framerate;
                    SVideoEncoderParam *p_encoder_params = (SVideoEncoderParam *)p_param;
                    mbSendSyncBuffer = 1;
                    if (p_encoder_params->framerate > 1) {
                        mFrameRate = p_encoder_params->framerate;
                        mCurrentFramerateNum = DDefaultTimeScale;
                        mCurrentFramerateDen = DDefaultTimeScale / mFrameRate;
                    }
                    LOGM_PRINTF("update framerate, %f, %d, %d\n", mFrameRate, mCurrentFramerateNum, mCurrentFramerateDen);
                }
                break;
            case EFilterPropertyType_video_bitrate: {
                    type = EDSPControlType_UDEC_TRANSCODER_update_bitrate;
                }
                break;
            case EFilterPropertyType_video_demandIDR: {
                    type = EDSPControlType_UDEC_TRANSCODER_demand_IDR;
                }
                break;
            case EFilterPropertyType_video_gop: {
                    type = EDSPControlType_UDEC_TRANSCODER_update_gop;
                }
                break;
            default:
                LOGM_ERROR("unsupported property_type type %d\n", property_type);
                return EECode_NotSupported;
        }
        err = pDSPAPI->DSPControl(type, p_param);
    } else {
        LOGM_ERROR("NULL mpPersistMediaConfig %p, or NULL p_dsp_handler %p\n", mpPersistMediaConfig, mpPersistMediaConfig->dsp_config.p_dsp_handler);
    }

    return err;
}

EDecoderMode CAmbaVideoEncoder::GetDecoderMode() const
{
    return EDecoderMode_Normal;
}

EECode CAmbaVideoEncoder::DecodeDirect(CIBuffer *in_buffer)
{
    LOGM_FATAL("not supported\n");
    return EECode_NotSupported;
}

EECode CAmbaVideoEncoder::SetBufferPoolDirect(IOutputPin *p_output_pin, IBufferPool *p_bufferpool)
{
    LOGM_FATAL("not supported\n");
    return EECode_NotSupported;
}

void CAmbaVideoEncoder::Delete()
{
    LOGM_INFO("CAmbaVideoEncoder::Delete().\n");

    inherited::Delete();
}

void CAmbaVideoEncoder::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    mDebugHeartBeat = 0;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

