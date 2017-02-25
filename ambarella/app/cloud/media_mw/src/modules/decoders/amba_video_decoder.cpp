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

#include "amba_video_decoder.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//#define DLOCAL_DEBUG_VERBOSE

IVideoDecoder *gfCreateAmbaVideoDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CAmbaVideoDecoder::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

//-----------------------------------------------------------------------
//
// CAmbaVideoDecoder
//
//-----------------------------------------------------------------------
CAmbaVideoDecoder::CAmbaVideoDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mpDSPDecoder(NULL)
    , mDSPMode(EDSPOperationMode_Invalid)
    , mDSPPlatform(EDSPPlatform_Invalid)
    , mCodecFormat(StreamFormat_Invalid)
    , mCapMaxCodedWidth(0)
    , mCapMaxCodedHeight(0)
    , mbAddUdecWarpper(1)
    , mbFeedPts(1)
    , mbSpecifiedFPS(0)
    , mbUDECInstanceSetup(0)
    , mbUDECStopped(0)
    , mbUDECHasFatalError(0)
    , mbUDECPaused(0)
    , mbUDECStepMode(0)
    , mbHasWakeVout(0)
    , mH264DataFmt(H264_FMT_INVALID)
    , mH264AVCCNaluLen(0)
    , mbMP4S(0)
    , mbAddDelimiter(0)
    , mbBWplayback(0)
    , mbFeedingMode(0)
    , mIdentifyerCount(0)
    , mSpecifiedTimeScale(DDefaultVideoFramerateNum)
    , mSpecifiedFrameTick(DDefaultVideoFramerateDen)
    , mpBitSreamBufferStart(NULL)
    , mpBitSreamBufferEnd(NULL)
    , mpBitStreamBufferCurPtr(NULL)
    , mpBitStreamBufferCurAccumulatedPtr(NULL)
    , mAccumulatedFrameNumber(0)
    , mpVideoExtraData(NULL)
    , mVideoExtraDataSize(0)
    , mpH264spspps(NULL)
    , mpDumpFile(NULL)
    , mpDumpFileSeparate(NULL)
    , mDumpIndex(0)
    , mDumpStartFrame(0)
    , mDumpEndFrame(0xffffffff)
{
}

IVideoDecoder *CAmbaVideoDecoder::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CAmbaVideoDecoder *result = new CAmbaVideoDecoder(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CAmbaVideoDecoder::GetObject0() const
{
    return (CObject *) this;
}

EECode CAmbaVideoDecoder::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAmbaDecoder);

    DASSERT(mpPersistMediaConfig);
    if (EDSPPlatform_AmbaS2 == mpPersistMediaConfig->dsp_config.dsp_type) {
        mbAddDelimiter = 1;
    }

    if (!mpPersistMediaConfig->dsp_config.not_add_udec_wrapper) {
        mbAddUdecWarpper = 1;
    } else {
        mbAddUdecWarpper = 0;
    }

    if (!mpPersistMediaConfig->dsp_config.not_feed_pts) {
        mbFeedPts = 1;
    } else {
        mbFeedPts = 0;
    }

    if (!mpPersistMediaConfig->dsp_config.specified_fps) {
        mFrameRateNum = DDefaultVideoFramerateNum;
        mFrameRateDen = DDefaultVideoFramerateDen;
        mFrameRate = 30;
    } else {
        mFrameRate = mpPersistMediaConfig->dsp_config.specified_fps;
        if (30 == mpPersistMediaConfig->dsp_config.specified_fps) {
            mFrameRateNum = DDefaultVideoFramerateNum;
            mFrameRateDen = DDefaultVideoFramerateDen;
        } else if (15 == mpPersistMediaConfig->dsp_config.specified_fps) {
            mFrameRateNum = DDefaultVideoFramerateNum;
            mFrameRateDen = 6006;
        } else if (60 == mpPersistMediaConfig->dsp_config.specified_fps) {
            mFrameRateNum = DDefaultVideoFramerateNum;
            mFrameRateDen = 1501;
        } else {
            mFrameRateNum = DDefaultVideoFramerateNum;
            mFrameRateDen = DDefaultVideoFramerateNum / mpPersistMediaConfig->dsp_config.specified_fps;
        }
    }
    LOGM_INFO("mFrameRateNum %d, mFrameRateDen %d, mFrameRate %d, mbFeedPts %d, mbAddUdecWarpper %d\n", mFrameRateNum, mFrameRateDen, mFrameRate, mbFeedPts, mbAddUdecWarpper);

    return EECode_OK;
}

CAmbaVideoDecoder::~CAmbaVideoDecoder()
{
    //LOGM_RESOURCE("~CAmbaVideoDecoder.\n");
    if (mpVideoExtraData) {
        free(mpVideoExtraData);
        mpVideoExtraData = NULL;
    }

    if (mpH264spspps) {
        free(mpH264spspps);
        mpH264spspps = NULL;
    }
    //LOGM_RESOURCE("~CAmbaVideoDecoder done.\n");
}

EECode CAmbaVideoDecoder::SetupContext(SVideoDecoderInputParam *param)
{
    TInt iav_fd;

    DASSERT(!mbUDECInstanceSetup);
    DASSERT(mpPersistMediaConfig);

    if (param) {

        DASSERT(!mpDSPDecoder);
        if (mpDSPDecoder) {
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
            mCapMaxCodedWidth = 1920;
            mCapMaxCodedHeight = 1080;
            LOGM_WARN("max coded size not specified, use default %u x %u\n", mCapMaxCodedWidth, mCapMaxCodedHeight);
        }

        iav_fd = mpPersistMediaConfig->dsp_config.device_fd;
        LOGM_INFO("before gfDSPDecAPIFactory, mIndex %d, iav_fd %d, mCodecFormat %d, mDSPMode %d, mDSPPlatform %d, mCapMaxCodedWidth %d, mCapMaxCodedHeight %d\n", mIndex, iav_fd, (TInt)mCodecFormat, (TInt)mDSPMode, (TInt)mDSPPlatform, mCapMaxCodedWidth, mCapMaxCodedHeight);

        mpDSPDecoder = gfDSPDecAPIFactory(iav_fd, mDSPMode, mCodecFormat, mpPersistMediaConfig);

        LOGM_INFO("after gfDSPDecAPIFactory, mpDSPDecoder %p\n", mpDSPDecoder);

        if (mpDSPDecoder) {
            SDecoderParam dsp_param;
            dsp_param.codec_type = mCodecFormat;
            dsp_param.dsp_mode = mDSPMode;
            dsp_param.max_pic_width = mCapMaxCodedWidth;
            dsp_param.max_pic_height = mCapMaxCodedHeight;

            LOGM_INFO("before InitDecoder\n");
            TUint vout_start_index = 0;
            TUint num_vout = 0;

            if (0 == mpPersistMediaConfig->dsp_config.modeConfig.vout_mask) {
                vout_start_index = 0;
                num_vout = 0;
            } else if (1 == mpPersistMediaConfig->dsp_config.modeConfig.vout_mask) {
                vout_start_index = 0;
                num_vout = 1;
            } else if (2 == mpPersistMediaConfig->dsp_config.modeConfig.vout_mask) {
                vout_start_index = 1;
                num_vout = 1;
            } else if (3 == mpPersistMediaConfig->dsp_config.modeConfig.vout_mask) {
                vout_start_index = 0;
                num_vout = 2;
            } else {
                LOGM_ERROR("BAD vout mask 0x%08x\n", mpPersistMediaConfig->dsp_config.modeConfig.vout_mask);
            }

            mpDSPDecoder->InitDecoder(mIndex, &dsp_param, vout_start_index, num_vout, mpPersistMediaConfig->dsp_config.voutConfigs.voutConfig);

            mpBitStreamBufferCurPtr = mpBitSreamBufferStart = dsp_param.bits_fifo_start;
            mpBitSreamBufferEnd = dsp_param.bits_fifo_start + dsp_param.bits_fifo_size;

            LOGM_INFO("after InitDecoder, mpBitSreamBufferStart %p, mpBitSreamBufferEnd %p\n", mpBitSreamBufferStart, mpBitSreamBufferEnd);

            mbUDECInstanceSetup = 1;
        } else {
            LOGM_FATAL("NULL mpDSPDecoder, mDSPPlatform %d, mDSPMode %d\n", mDSPPlatform, mDSPMode);
            return EECode_InternalLogicalBug;
        }

        gfFillUSEQHeader(mUSEQHeader, mCodecFormat, mFrameRateNum, mFrameRateDen, 0, 0, 0);
        gfInitUPESHeader(mUPESHeader, mCodecFormat);

        LOGM_INFO("[DEBUG video info]: mFrameRateNum %d, mFrameRateDen %d\n", mFrameRateNum, mFrameRateDen);

    } else {
        LOGM_FATAL("NULL input in CAmbaVideoDecoder::SetupContext\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CAmbaVideoDecoder::DestroyContext()
{
    if (mpDSPDecoder) {
        mpDSPDecoder->ReleaseDecoder(mIndex);
        mbUDECInstanceSetup = 0;
        mpDSPDecoder->Delete();
        mpDSPDecoder = NULL;
    }

    return EECode_OK;
}

EECode CAmbaVideoDecoder::SetBufferPool(IBufferPool *buffer_pool)
{
    return EECode_OK;
}

EECode CAmbaVideoDecoder::Start()
{
    //debug assert
    DASSERT(mbUDECInstanceSetup);
    DASSERT(mpDSPDecoder);
    mIdentifyerCount = mpPersistMediaConfig->identifyer_count;
    if (mpDSPDecoder && mbUDECInstanceSetup) {
        if (mbUDECStopped) {
            mpDSPDecoder->Stop(mIndex, 0xff);
        }
        return EECode_OK;
    }

    return EECode_BadState;
}

EECode CAmbaVideoDecoder::Stop()
{
    DASSERT(mbUDECInstanceSetup);
    DASSERT(mpDSPDecoder);
    mIdentifyerCount = mpPersistMediaConfig->identifyer_count;
    if (mpDSPDecoder && mbUDECInstanceSetup) {
        if (!mbUDECStopped) {
            mpDSPDecoder->Stop(mIndex, mpPersistMediaConfig->dsp_config.prefer_udec_stop_flags);
            mbUDECStopped = 1;
            mbHasWakeVout = 0;
            mIdentifyerCount = mpPersistMediaConfig->identifyer_count;
            mpBitStreamBufferCurPtr = mpBitSreamBufferStart;
        } else {
            LOGM_WARN("Already stopped, udec %d\n", mIndex);
            return EECode_BadState;
        }
    } else {
        LOGM_WARN("NOT setup/started yet\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CAmbaVideoDecoder::fillEOSToBSB(TU8 *p_bsb_start)
{
    EECode err;
    TU8 *p_eos_start = NULL;
    TInt eos_size = 0;
    switch (mCodecFormat) {
        case StreamFormat_H264: {
                static TU8 eos[] = {0x00, 0x00, 0x00, 0x01, 0x0A};
                p_eos_start = eos;
                eos_size = sizeof(eos);
            }
            break;

        case StreamFormat_VC1: {
                // WMV3
                /*if( mpCodec->codec_id == CODEC_ID_WMV3 ){
                                static TU8 eos_wmv3[4] = {0xFF, 0xFF, 0xFF, 0x0A};// special eos for wmv3
                                p_eos_start = eos_wmv3;
                                eos_size = sizeof(eos_wmv3);
                }
                // VC-1
                else*/
                {
                    static TU8 eos[4] = {0, 0, 0x01, 0xA};
                    p_eos_start = eos;
                    eos_size = sizeof(eos);
                }
            }
            break;

        case StreamFormat_MPEG12: {
                static TU8 eos[4] = {0, 0, 0x01, 0xB7};
                p_eos_start = eos;
                eos_size = sizeof(eos);
            }
            break;

        case StreamFormat_MPEG4: {
                static TU8 eos[4] = {0, 0, 0x01, 0xB1};
                p_eos_start = eos;
                eos_size = sizeof(eos);
            }
            break;

        case StreamFormat_JPEG:
        default:
            return EECode_NoImplement;
    }

    err = mpDSPDecoder->RequestBitStreamBuffer(mIndex, p_bsb_start, eos_size);
    DASSERT_OK(err);
    //copy data here
    DASSERT(p_eos_start && eos_size);
    mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, p_eos_start, eos_size);
    return EECode_OK;
}

EECode CAmbaVideoDecoder::feedH264(CIBuffer *in_buffer)
{
    TU8 *p_data;
    TUint size;
    TU8 startcode[4] = {0, 0, 0, 0x01};
    TUint send_size = 0;
    TTime pts = in_buffer->GetBufferLinearPTS();

    DASSERT(mbUDECInstanceSetup);
    DASSERT(mpDSPDecoder);
    DASSERT(in_buffer);

    if (mpBitStreamBufferCurPtr == mpBitSreamBufferEnd) {
        mpBitStreamBufferCurPtr = mpBitSreamBufferStart;
    }

    if (!mpBitStreamBufferCurAccumulatedPtr) {
        DASSERT(!mAccumulatedFrameNumber);
        mpBitStreamBufferCurAccumulatedPtr = mpBitStreamBufferCurPtr;
    }

    size = in_buffer->GetDataSize();
    p_data = in_buffer->GetDataPtr();

    if (in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {

        if (mbAddUdecWarpper) {
            //LOGM_CONFIGURATION("send useq\n");
            mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mUSEQHeader, DUDEC_SEQ_HEADER_LENGTH);
        }

        if (H264_FMT_ANNEXB == mH264DataFmt) {
            mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mpVideoExtraData, mVideoExtraDataSize);
        } else if (H264_FMT_AVCC == mH264DataFmt) {
            mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mpH264spspps, mH264spsppsSize);
        } else {
            LOGM_FATAL("BAD h264 format %d\n", mH264DataFmt);
            return EECode_InternalLogicalBug;
        }
    }

    if (mbAddUdecWarpper) {
        //to do, the playload size is not always correct
        //pts = in_buffer->GetBufferPTS();
        //pts = mFakePTS;
        //mFakePTS += DDefaultVideoFramerateDen;

        //LOGM_PTS("pts %lld, in_buffer->GetBufferPTS() %lld\n", pts, in_buffer->GetBufferPTS());

        //fill gop header
        if (mbBWplayback && (in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME)) {
            gfFillPrivateGopNalHeader(mPrivateGOPHeader, mFrameRateNum, mFrameRateDen, 1, mFrameRate, pts >> 32,  pts & 0xffffffff);
            mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mPrivateGOPHeader, DPRIVATE_GOP_NAL_HEADER_LENGTH);
        }

        if (pts >= 0) {
            send_size = gfFillPESHeader(mUPESHeader, (pts & 0xffffffff), (pts >> 32), in_buffer->GetDataSize(), 1, 0);
        } else {
            send_size = gfFillPESHeader(mUPESHeader, 0, 0, in_buffer->GetDataSize(), 0, 0);
        }
        DASSERT(send_size <= DUDEC_PES_HEADER_LENGTH);
        mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mUPESHeader, send_size);
    }

    if (H264_FMT_ANNEXB == mH264DataFmt) {
        //remove addtional start code prefix
        //p_data = __validStartPoint(p_data, size);
        mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, p_data, size);
    } else if (H264_FMT_AVCC == mH264DataFmt) {

        while (size) {
            mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, startcode, sizeof(startcode));

            send_size = 0;
            DASSERT(mH264AVCCNaluLen <= 4);

            for (TUint index = 0; index < mH264AVCCNaluLen; index++, p_data++) {
                send_size = (send_size << 8) | (*p_data);
            }

            size -= mH264AVCCNaluLen;
            DASSERT(send_size <= size);
            size -= send_size;

            p_data = __validStartPoint(p_data, send_size);
            mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, p_data, send_size);
            p_data += send_size;

        }
    } else {
        LOGM_FATAL("BAD h264 format %d\n", mH264DataFmt);
        return EECode_InternalLogicalBug;
    }

    if (mbAddDelimiter) {
        TU8 _h264_delimiter[6] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x30};
        mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, _h264_delimiter, sizeof(_h264_delimiter));
    }

    if (mbBWplayback && (in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME)) {
        TU8 eos[] = {0x00, 0x00, 0x00, 0x01, 0x0A};
        mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, eos, sizeof(eos));
    }

    mAccumulatedFrameNumber ++;

    return EECode_OK;
}

EECode CAmbaVideoDecoder::sendRemainFrames()
{
    EECode err;

    DASSERT(mAccumulatedFrameNumber);
    DASSERT(mpBitStreamBufferCurAccumulatedPtr);
    if (!mpBitStreamBufferCurAccumulatedPtr || !mAccumulatedFrameNumber) {
        LOGM_ERROR("NULL mpBitStreamBufferCurAccumulatedPtr %p, or zero mAccumulatedFrameNumber %d\n", mpBitStreamBufferCurAccumulatedPtr, mAccumulatedFrameNumber);
        return EECode_BadState;
    }

    mDebugHeartBeat ++;

    err = mpDSPDecoder->Decode(mIndex, mpBitStreamBufferCurAccumulatedPtr, mpBitStreamBufferCurPtr, mAccumulatedFrameNumber);
    mAccumulatedFrameNumber = 0;
    mpBitStreamBufferCurAccumulatedPtr = 0;
    if (DUnlikely(EECode_OK != err)) {
        if (mpPersistMediaConfig->app_start_exit) {
            LOGM_NOTICE("[program start exit]: Decode return err=%d, %s\n", err, gfGetErrorCodeString(err));
        } else {
            LOGM_ERROR("Decode fail, return %d, %s\n", err, gfGetErrorCodeString(err));
        }
        return err;
    }

    if (!mbHasWakeVout) {
        DASSERT(mpMsgSink);
        LOGM_INFO("post EMSGType_NotifyUDECStateChanges, mpMsgSink %p\n", mpMsgSink);
        if (mpMsgSink) {
            SMSG msg;
            msg.code = EMSGType_NotifyUDECStateChanges;
            msg.p_owner = 0;
            msg.owner_id = 0;
            msg.identifyer_count = mIdentifyerCount;
            msg.p3 = mIndex;
            //LOGM_DEBUG("DEBUG, msg.identifyer_count %d\n", msg.identifyer_count);
            mpMsgSink->MsgProc(msg);
        }

        mbHasWakeVout = 1;
    }

    return EECode_OK;
}

EECode CAmbaVideoDecoder::decodeH264(CIBuffer *in_buffer, CIBuffer *out_buffer)
{
    TU8 *p_frame_start;
    TU8 *p_data;
    TUint size;
    EECode err;
    TU8 startcode[4] = {0, 0, 0, 0x01};
    TUint send_size = 0;
    TTime pts = 0;

    DASSERT(mbUDECInstanceSetup);
    DASSERT(mpDSPDecoder);
    DASSERT(in_buffer);

    if (mbBWplayback && !(in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME)) {
        LOGM_WARN("bw playback support IOnly now, non-key frames won't be decoded.\n");
        return EECode_OK;
    }

    if (mpBitStreamBufferCurPtr == mpBitSreamBufferEnd) {
        mpBitStreamBufferCurPtr = mpBitSreamBufferStart;
    }

    p_frame_start = mpBitStreamBufferCurPtr;

    size = in_buffer->GetDataSize();
    p_data = in_buffer->GetDataPtr();
    err = mpDSPDecoder->RequestBitStreamBuffer(mIndex, p_frame_start, size + 1024);
    if (DUnlikely(EECode_OK != err)) {
        if (mpPersistMediaConfig->app_start_exit) {
            LOGM_NOTICE("[program start exit]: RequestBitStreamBuffer return err=%d, %s\n", err, gfGetErrorCodeString(err));
        } else {
            LOGM_ERROR("RequestBitStreamBuffer fail, return %d, %s\n", err, gfGetErrorCodeString(err));
            if (EECode_OSError == err) {
                SDecStatus status;
                if (EECode_OK == mpDSPDecoder->QueryStatus(mIndex, &status)) {
                    LOGM_INFO("mpDSPDecoder->QueryStatus: dec_state=%hu, vout_state=%hu, error_code=%u, error_level=%hu, error_type=%hu\n", status.dec_state, status.vout_state, status.error_code, status.error_level, status.error_type);
                }
            }
        }
        return err;
    }

    if (in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME) {

        if (mbAddUdecWarpper) {
            //LOGM_CONFIGURATION("send useq\n");
            mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mUSEQHeader, DUDEC_SEQ_HEADER_LENGTH);
        }

        if (H264_FMT_ANNEXB == mH264DataFmt) {
            mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mpVideoExtraData, mVideoExtraDataSize);
        } else if (H264_FMT_AVCC == mH264DataFmt) {
            mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mpH264spspps, mH264spsppsSize);
        } else {
            LOGM_FATAL("BAD h264 format %d\n", mH264DataFmt);
            return EECode_InternalLogicalBug;
        }
    }

    if (mbAddUdecWarpper) {
        //to do, the playload size is not always correct
        pts = in_buffer->GetBufferLinearPTS();
        //pts = mFakePTS;
        //mFakePTS += DDefaultVideoFramerateDen;

        //LOGM_PTS("pts %lld, in_buffer->GetBufferPTS() %lld\n", pts, in_buffer->GetBufferPTS());

        //fill gop header
        if (mbBWplayback && (in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME)) {
            gfFillPrivateGopNalHeader(mPrivateGOPHeader, mFrameRateNum, mFrameRateDen, 1, mFrameRate, pts >> 32,  pts & 0xffffffff);
            mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mPrivateGOPHeader, DPRIVATE_GOP_NAL_HEADER_LENGTH);
        }

        if (mbFeedPts && pts >= 0) {
            send_size = gfFillPESHeader(mUPESHeader, (pts & 0xffffffff), (pts >> 32), in_buffer->GetDataSize(), 1, 0);
        } else {
            send_size = gfFillPESHeader(mUPESHeader, 0, 0, in_buffer->GetDataSize(), 0, 0);
        }
        DASSERT(send_size <= DUDEC_PES_HEADER_LENGTH);
        mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, mUPESHeader, send_size);
    }

    if (H264_FMT_ANNEXB == mH264DataFmt) {
        //remove addtional start code prefix
        //p_data = __validStartPoint(p_data, size);
        mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, p_data, size);
    } else if (H264_FMT_AVCC == mH264DataFmt) {

        while (size) {
            mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, startcode, sizeof(startcode));

            send_size = 0;
            DASSERT(mH264AVCCNaluLen <= 4);

            for (TUint index = 0; index < mH264AVCCNaluLen; index++, p_data++) {
                send_size = (send_size << 8) | (*p_data);
            }

            size -= mH264AVCCNaluLen;
            //DASSERT(send_size <= size);
            if (send_size > size) {
                LOGM_ERROR("data corruption in AVCC format? send_size %d, remaining size %d\n", send_size, size);
                return EECode_DataCorruption;
            }
            size -= send_size;

            p_data = __validStartPoint(p_data, send_size);
            mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, p_data, send_size);
            p_data += send_size;

        }
    } else {
        LOGM_FATAL("BAD h264 format %d\n", mH264DataFmt);
        return EECode_InternalLogicalBug;
    }

    if (mbAddDelimiter) {
        TU8 _h264_delimiter[6] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x30};
        mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, _h264_delimiter, sizeof(_h264_delimiter));
    }

    if (mbBWplayback && (in_buffer->GetBufferFlags() & CIBuffer::KEY_FRAME)) {
        TU8 eos[] = {0x00, 0x00, 0x00, 0x01, 0x0A};
        mpBitStreamBufferCurPtr = copyDataToBSB(mpBitStreamBufferCurPtr, eos, sizeof(eos));
    }

#ifdef DLOCAL_DEBUG_VERBOSE
    TU8 *pttt = in_buffer->GetDataPtr();
    LOGM_VERBOSE("video decoder, pBuffer %p, data %p, size %d, p_mem_start %p\n", in_buffer, in_buffer->GetDataPtr(), in_buffer->GetDataSize(), in_buffer->GetDataPtrBase());
    LOGM_VERBOSE("data: %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x.\n",
                 pttt[0], pttt[1], pttt[2], pttt[3],
                 pttt[4], pttt[5], pttt[6], pttt[7],
                 pttt[8], pttt[9], pttt[10], pttt[11],
                 pttt[12], pttt[13], pttt[14], pttt[15]);

    pttt = p_frame_start;
    if (p_frame_start + 32 < mpBitSreamBufferEnd) {
        LOGM_VERBOSE("data in bsb: %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x.\n",
                     pttt[0], pttt[1], pttt[2], pttt[3],
                     pttt[4], pttt[5], pttt[6], pttt[7],
                     pttt[8], pttt[9], pttt[10], pttt[11],
                     pttt[12], pttt[13], pttt[14], pttt[15]);
    }
#endif

    dumpEsData(p_frame_start, mpBitStreamBufferCurPtr);
    mDebugHeartBeat ++;
    err = mpDSPDecoder->Decode(mIndex, p_frame_start, mpBitStreamBufferCurPtr);
    if (DUnlikely(EECode_OK != err)) {
        if (mpPersistMediaConfig->app_start_exit) {
            LOGM_NOTICE("[program start exit]: Decode return err=%d, %s\n", err, gfGetErrorCodeString(err));
        } else {
            LOGM_ERROR("Decode fail, return %d, %s\n", err, gfGetErrorCodeString(err));
        }
        return err;
    }

    if (!mbHasWakeVout) {
        DASSERT(mpMsgSink);
        LOGM_INFO("post EMSGType_NotifyUDECStateChanges, mpMsgSink %p\n", mpMsgSink);
        if (mpMsgSink) {
            SMSG msg;
            msg.code = EMSGType_NotifyUDECStateChanges;
            msg.p_owner = 0;
            msg.owner_id = 0;
            msg.identifyer_count = mIdentifyerCount;
            //LOGM_DEBUG("DEBUG, msg.identifyer_count %d\n", msg.identifyer_count);
            msg.p3 = mIndex;
            mpMsgSink->MsgProc(msg);
        }

        mbHasWakeVout = 1;
    }

    return EECode_OK;
}

EECode CAmbaVideoDecoder::Decode(CIBuffer *in_buffer, CIBuffer *&out_buffer)
{
    EECode err = EECode_OK;

    DASSERT(mbUDECInstanceSetup);
    DASSERT(mpDSPDecoder);
    DASSERT(in_buffer);

    if (!mbFeedingMode) {
        mbFeedingMode = 1;
    } else if (1 != mbFeedingMode) {
        if (mAccumulatedFrameNumber) {
            err = sendRemainFrames();
            DASSERT_OK(err);
        }
        mbFeedingMode = 1;
    }

    if (EBufferType_FlowControl_EOS == in_buffer->GetBufferType()) {
        LOGM_INFO("fill eos\n");
        err = fillEOSToBSB(mpBitStreamBufferCurPtr);
        if (EECode_OK != err) {
            LOGM_FATAL("fillEOSToBSB fail, return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        switch (mCodecFormat) {

            case StreamFormat_H264:
                err = decodeH264(in_buffer, out_buffer);
                if (mbUDECStopped && EECode_OK == err) {
                    mbUDECStopped = 0;
                }
                break;

            default:
                LOGM_FATAL("need add codec support %d\n", mCodecFormat);
                err = EECode_NotSupported;
                break;
        }
    }

    return err;
}

EECode CAmbaVideoDecoder::Flush()
{
    DASSERT(mbUDECInstanceSetup);
    DASSERT(mpDSPDecoder);
    mIdentifyerCount = mpPersistMediaConfig->identifyer_count;
    if (mpDSPDecoder && mbUDECInstanceSetup) {
        if (!mbUDECStopped) {
            mpDSPDecoder->Stop(mIndex, mpPersistMediaConfig->dsp_config.prefer_udec_stop_flags);
            mbUDECStopped = 1;
            mpBitStreamBufferCurPtr = mpBitSreamBufferStart;
            mpDSPDecoder->Stop(mIndex, 0xff);
        } else {
            LOGM_WARN("Already stopped, udec %d\n", mIndex);
            return EECode_BadState;
        }
    } else {
        LOGM_WARN("NOT setup/started yet\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CAmbaVideoDecoder::Suspend()
{
    DASSERT(mbUDECInstanceSetup);
    DASSERT(mpDSPDecoder);
    mIdentifyerCount = mpPersistMediaConfig->identifyer_count;
    if (mpDSPDecoder) {
        if (mbUDECInstanceSetup) {
            LOGM_INFO("\n");
            mpDSPDecoder->ReleaseDecoder(mIndex);
            mbUDECInstanceSetup = 0;
        }
    } else {
        LOGM_WARN("NOT setup/started yet\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

void CAmbaVideoDecoder::Delete()
{
    LOGM_INFO("CAmbaVideoDecoder::Delete().\n");

    inherited::Delete();
}

TU8 *CAmbaVideoDecoder::copyDataToBSB(TU8 *ptr, TU8 *buffer, TUint size)
{
    if (ptr + size <= mpBitSreamBufferEnd) {
        memcpy((void *)ptr, (const void *)buffer, size);
        return ptr + size;
    } else {
        TInt room = mpBitSreamBufferEnd - ptr;
        TU8 *ptr2;
        memcpy((void *)ptr, (const void *)buffer, room);
        ptr2 = buffer + room;
        size -= room;
        memcpy((void *)mpBitSreamBufferStart, (const void *)ptr2, size);
        return mpBitSreamBufferStart + size;
    }
}

void CAmbaVideoDecoder::dumpEsData(TU8 *pStart, TU8 *pEnd)
{
    if (mConfigLogOutput & DCAL_BITMASK(ELogOutput_BinaryTotalFile)) {
        DASSERT(!mpDumpFile);
        TChar total_dump_file_name[128] = {0};
        snprintf(total_dump_file_name, 127, "CAmbaVideoDecoder_%d.total.es", mIndex);
        mpDumpFile = fopen(total_dump_file_name, "ab");

        if (mpDumpFile) {
            //AM_INFO("write data.\n");
            DASSERT(pEnd != pStart);
            if (pEnd < pStart) {
                //wrap around
                fwrite(pStart, 1, (TU32)(mpBitSreamBufferEnd - pStart), mpDumpFile);
                fwrite(mpBitSreamBufferStart, 1, (TU32)(pEnd - mpBitSreamBufferStart), mpDumpFile);
            } else {
                fwrite(pStart, 1, (TU32)(pEnd - pStart), mpDumpFile);
            }
            fclose(mpDumpFile);
            mpDumpFile = NULL;
        } else {
            LOGM_WARN("open  mpDumpFile fail.\n");
        }
    }

    if (mConfigLogOutput & DCAL_BITMASK(ELogOutput_BinarySeperateFile)) {

        mDumpIndex++;
        if (mDumpIndex < mDumpStartFrame || mDumpIndex > mDumpEndFrame)
        { return; }

        TChar separate_dump_file_name[128] = {0};

        snprintf(separate_dump_file_name, 127, "CAmbaVideoDecoder_%d.%d.seperate.es", mIndex, mDumpIndex);
        mpDumpFileSeparate = fopen(separate_dump_file_name, "wb");
        if (mpDumpFileSeparate) {
            DASSERT(pEnd != pStart);
            if (pEnd < pStart) {
                //wrap around
                fwrite(pStart, 1, (size_t)(mpBitSreamBufferEnd - pStart), mpDumpFileSeparate);
                fwrite(mpBitSreamBufferStart, 1, (size_t)(pEnd - mpBitSreamBufferStart), mpDumpFileSeparate);
            } else {
                fwrite(pStart, 1, (size_t)(pEnd - pStart), mpDumpFileSeparate);
            }
            fclose(mpDumpFileSeparate);
            mpDumpFileSeparate = NULL;
        }
    }

}

EECode CAmbaVideoDecoder::SetExtraData(TU8 *p, TMemSize size)
{
    TU8 startcode[4] = {0, 0, 0, 0x01};
    TUint sps, pps;
    TU8 *pH264spspps_start = NULL;

    if (DUnlikely((!p) || (!size))) {
        LOGM_ERROR("NULL extradata %p, or zero size %ld\n", p, size);
        return EECode_BadParam;
    }

    if (mpVideoExtraData && (mVideoExtraDataSize == size)) {
        if (!memcmp(mpVideoExtraData, p, size)) {
            return EECode_OK;
        }
    }

    if (mpVideoExtraData) {
        free(mpVideoExtraData);
        mpVideoExtraData = NULL;
        mVideoExtraDataSize = 0;
    }

    if (mpH264spspps) {
        free(mpH264spspps);
        mpH264spspps = NULL;
    }

    DASSERT(!mpVideoExtraData);
    DASSERT(!mVideoExtraDataSize);

    mVideoExtraDataSize = size;
    LOGM_INFO("video extra data size %ld, first 8 bytes: 0x%02x 0x%02x 0x%02x 0x%02x, 0x%02x 0x%02x 0x%02x 0x%02x\n", mVideoExtraDataSize, \
              p[0], p[1], p[2], p[3], \
              p[4], p[5], p[6], p[7]);
    mpVideoExtraData = (TU8 *)DDBG_MALLOC(mVideoExtraDataSize, "VDAE");
    if (mpVideoExtraData) {
        memcpy(mpVideoExtraData, p, size);
    } else {
        LOGM_FATAL("NO memory\n");
        return EECode_NoMemory;
    }

    if (p[0] != 0x01) {
        LOGM_INFO("extradata is annex-b format.\n");
        mH264DataFmt = H264_FMT_ANNEXB;
    } else {

        mH264spsppsSize = 0;
        DASSERT(!mpH264spspps);

        mH264DataFmt = H264_FMT_AVCC;
        mH264AVCCNaluLen = 1 + (mpVideoExtraData[4] & 3);
        LOGM_INFO("extradata is AVCC format, mH264AVCCNaluLen %d.\n", mH264AVCCNaluLen);

        mpH264spspps = (TU8 *)DDBG_MALLOC(mVideoExtraDataSize + 16, "VDAE");
        if (mpH264spspps) {
            pH264spspps_start = mpH264spspps;
            sps = BE_16(mpVideoExtraData + 6);
            memcpy(mpH264spspps, startcode, sizeof(startcode));
            mpH264spspps += sizeof(startcode);
            mH264spsppsSize += sizeof(startcode);
            memcpy(mpH264spspps, mpVideoExtraData + 8, sps);
            mpH264spspps += sps;
            mH264spsppsSize += sps;

            pps = BE_16(mpVideoExtraData + 6 + 2 + sps + 1);
            memcpy(mpH264spspps, startcode, sizeof(startcode));
            mpH264spspps += sizeof(startcode);
            mH264spsppsSize += sizeof(startcode);
            memcpy(mpH264spspps, mpVideoExtraData + 6 + 2 + sps + 1 + 2, pps);
            mH264spsppsSize += pps;
            mpH264spspps = pH264spspps_start;
        } else {
            LOGM_FATAL("NO memory\n");
            return EECode_NoMemory;
        }

        LOGM_INFO("mH264spsppsSize %ld\n", mH264spsppsSize);
    }

    if ((mConfigLogOutput & DCAL_BITMASK(ELogOutput_BinaryTotalFile)) || (mConfigLogOutput & DCAL_BITMASK(ELogOutput_BinarySeperateFile))) {
        TChar extradata_file_name[128] = {0};
        FILE *pfile = NULL;

        snprintf(extradata_file_name, 127, "CAmbaVideoDecoder_%d.extradata", mIndex);
        pfile = fopen(extradata_file_name, "wb");
        if (pfile) {
            fwrite(mpVideoExtraData, 1, (size_t)mVideoExtraDataSize, pfile);
            fclose(pfile);
            pfile = NULL;
        }

        snprintf(extradata_file_name, 127, "CAmbaVideoDecoder_%d.modif.extradata", mIndex);
        pfile = fopen(extradata_file_name, "wb");
        if (pfile) {
            fwrite(mpH264spspps, 1, (size_t)mH264spsppsSize, pfile);
            fclose(pfile);
            pfile = NULL;
        }

    }

    return EECode_OK;
}

EECode CAmbaVideoDecoder::SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed)
{
    EECode err = EECode_OK;
    DASSERT(mbUDECInstanceSetup);
    DASSERT(mpDSPDecoder);

    if (mpDSPDecoder && mbUDECInstanceSetup) {
        err = mpDSPDecoder->PbSpeed(mIndex, direction, feeding_rule, speed);
        if (EECode_OK == err) {
            mbBWplayback = direction;
        }
    } else {
        LOGM_WARN("NOT setup/started yet\n");
        err = EECode_BadState;
    }

    return err;
}

EECode CAmbaVideoDecoder::SetFrameRate(TUint framerate_num, TUint frameate_den)
{
    if (mFrameRateNum != framerate_num && mFrameRateDen != frameate_den) {
        mFrameRateNum = framerate_num;
        mFrameRateDen = frameate_den;
        LOGM_INFO("CAmbaVideoDecoder::SetFrameRate(), frame rate updated to num=%u, den=%u.\n", mFrameRateNum, mFrameRateDen);
        if (mbAddUdecWarpper) {
            gfFillUSEQHeader(mUSEQHeader, mCodecFormat, mFrameRateNum, mFrameRateDen, 0, 0, 0);
        }
    } else {
        //LOGM_VERBOSE("CAmbaVideoDecoder::SetFrameRate(), frame rate not changed, do nothing.\n");
    }

    return EECode_OK;
}

void CAmbaVideoDecoder::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    mDebugHeartBeat = 0;
}

EECode CAmbaVideoDecoder::QueryFreeZoom(TU32 &free_zoom, TU32 &fullness)
{
    SDecStatus status;

    DASSERT(mpDSPDecoder);
    if (mpDSPDecoder) {
        mpDSPDecoder->QueryStatus(mIndex, &status);
        free_zoom = status.free_zoom;
        fullness = status.fullness;
    } else {
        LOGM_FATAL("NULL mpDSPDecoder\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CAmbaVideoDecoder::QueryLastDisplayedPTS(TTime &pts)
{
    SDecStatus status;

    DASSERT(mpDSPDecoder);
    if (mpDSPDecoder) {
        mpDSPDecoder->QueryStatus(mIndex, &status);
        pts = status.last_pts;
    } else {
        LOGM_FATAL("NULL mpDSPDecoder\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CAmbaVideoDecoder::PushData(CIBuffer *in_buffer, TInt flush)
{
    EECode err = EECode_NotRunning;

    //debug check
    if (!mbFeedingMode) {
        mbFeedingMode = 2;
    }

    if (in_buffer) {
        err = feedH264(in_buffer);
        DASSERT_OK(err);
    }

    if (flush) {
        err = sendRemainFrames();
        DASSERT_OK(err);
    }

    return err;
}

EECode CAmbaVideoDecoder::PullDecodedFrame(CIBuffer *out_buffer, TInt block_wait, TInt &remaining)
{
    //to do
    //debug check
    DASSERT(2 == mbFeedingMode);

    LOGM_FATAL("TO DO\n");
    return EECode_OK;
}

EECode CAmbaVideoDecoder::TuningPB(TU8 fw, TU16 frame_tick)
{
    if (DLikely(mpDSPDecoder)) {
        mpDSPDecoder->TuningPB(mIndex, fw, frame_tick);
    } else {
        LOGM_ERROR("NULL mpDSPDecoder\n");
    }

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

