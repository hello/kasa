/*******************************************************************************
 * ts_muxer.cpp
 *
 * History:
 *    2013/09/09 - [Zhi He] create file
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

#ifdef BUILD_WITH_UNDER_DEVELOP_COMPONENT
#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"
#include "common_io.h"
#include "common_private_data.h"

#include "storage_lib_if.h"
#include "media_mw_if.h"
#include "media_mw_utils.h"
#include "framework_interface.h"
#include "codec_misc.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "mpeg_ts_defs.h"
#include "property_ts.h"

#include "ts_muxer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static inline void __crc32_byte(TInt *preg, TInt x)
{
    TInt i;
    for (i = 0, x <<= 24; i < 8; ++ i, x <<= 1) {
        (*preg) = ((*preg) << 1) ^ (((x ^ (*preg)) >> 31) & 0x04C11DB7);
    }
}

static inline TInt __crc32(TU8 *buf, TInt size)
{
    TInt crc = 0xffffffffL;

    for (TInt i = 0; i < size; ++ i) {
        __crc32_byte(&crc, (TInt)buf[i]);
    }
    return (crc);
}

IMuxer *gfCreateTSMuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CTSMuxer::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

IMuxer *CTSMuxer::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CTSMuxer *result = new CTSMuxer(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CTSMuxer::Destroy()
{
    Delete();
}

CTSMuxer::CTSMuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mpCacheBufferBase(NULL)
    , mpCacheBufferBaseAligned(NULL)
    , mpCurrentPosInCache(NULL)
    , mCacheBufferSize(0)
    , mPatPmtInterval(1024)
    , mnTsPacketSize(MPEG_TS_TP_PACKET_SIZE)
    , mnMaxTsPacketNumber(4096)
    , mCurrentTsPacketIndex(0)
    , mpIO(NULL)
    , mH264DataFmt(H264_FMT_INVALID)
    , mH264AVCCNaluLen(0)
    , mbConvertH264DataFormat(0)
    , mbIOOpend(0)
    , mpUrlName(NULL)
    , mnMaxStreamNumber(0)
    , mpConversionBuffer(NULL)
    , mConversionDataSize(0)
    , mConversionBufferSize(0)
    , mVideoIndex(0)
    , mAudioIndex(1)
    , mPrivDataIndex(2)
    , mSubtitleIndex(3)
    , mbVideoKeyFrameComes(0)
    , mbAudioKeyFrameComes(0)
    , mnTotalStreamNumber(0)
    , mbNeedFindSPSPPSInBitstream(0)
    , mnVideoFrameCount(0)
    , mnAudioFrameCount(0)
    , mnPrivdataFrameCount(0)
    , mnSubtitleFrameCount(0)
    , mEstimatedFileSize(0)
    , mFileDurationSeconds(0)
    , mEstimatedBitrate(0)
    , mFileBitrate(0)
    , mnPatContinuityCounter(0)
    , mnPmtContinuityCounter(0)
    , mnVideoContinuityCounter(0)
    , mnAudioContinuityCounter(0)
    , mnCustomizedPrivateDataContinuityCounter(0)
    , mPcrPatPmtFrameCount(0)
    , mbAddCustomizedPrivateData(1)
    , mChannelNameLength(0)
    , mbSuspend(0)
    , mMaxGopInFile(2048)
    , mCurrentGopIndex(0)
    , mpGopIndexBaseUnaligned(NULL)
    , mpGopIndexBase(NULL)
{
    TUint i = 0;

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        mpExtraData[i] = NULL;
        mExtraDataSize[i] = 0;
        mpExtraDataOri[i] = NULL;
        mExtraDataSizeOri[i] = 0;
    }

}

EECode CTSMuxer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleNativeTSMuxer);

    mCacheBufferSize = mnMaxTsPacketNumber * mnTsPacketSize;
    mpCacheBufferBase = (TU8 *)malloc(mCacheBufferSize + 31);
    mpCacheBufferBaseAligned = (TU8 *)((((TULong)mpCacheBufferBase) + 31) & (~0x1f));

    if (!mpCacheBufferBase) {
        LOGM_FATAL("No Memory! request size %u\n", mCacheBufferSize + 31);
        return EECode_NoMemory;
    }

    LOGM_NOTICE("mpCacheBufferBaseAligned %p, mpCacheBufferBase %p, mCacheBufferSize %u\n", mpCacheBufferBaseAligned, mpCacheBufferBase, mCacheBufferSize);
    mpCurrentPosInCache = mpCacheBufferBaseAligned;

    if (mbAddCustomizedPrivateData) {
        mpGopIndexBaseUnaligned = (TU8 *) malloc(mMaxGopInFile * sizeof(TU32) + 3);
        if (DUnlikely(!mpGopIndexBaseUnaligned)) {
            LOG_ERROR("no moemory, request size %ld\n", (TULong)(mMaxGopInFile * sizeof(TU32) + 3));
            return EECode_NoMemory;
        }
        mpGopIndexBase = (TU32 *)(((TULong)(mpGopIndexBaseUnaligned + 3)) & (~3));
        mCurrentGopIndex = 0;
    }

    return EECode_OK;
}

void CTSMuxer::Delete()
{
    TUint i = 0;

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpExtraDataOri[i]) {
            free(mpExtraDataOri[i]);
            mpExtraDataOri[i] = NULL;
        }
        if (mpExtraData[i]) {
            free(mpExtraData[i]);
            mpExtraData[i] = NULL;
        }
        mExtraDataSizeOri[i] = 0;
        mExtraDataSize[i] = 0;
    }
    if (mpConversionBuffer) {
        free(mpConversionBuffer);
        mpConversionBuffer = NULL;
    }
    mConversionBufferSize = 0;
    mConversionDataSize = 0;

    if (mpGopIndexBaseUnaligned) {
        free(mpGopIndexBaseUnaligned);
        mpGopIndexBase = NULL;
        mpGopIndexBaseUnaligned = NULL;
        mCurrentGopIndex = 0;
    }

    if (mpCacheBufferBase) {
        free(mpCacheBufferBase);
        mpCacheBufferBase = NULL;
    }
    mpCacheBufferBaseAligned = NULL;
    mCacheBufferSize = 0;

    if (mpIO) {
        mpIO->Delete();
        mpIO = NULL;
    }

    inherited::Delete();
}

CTSMuxer::~CTSMuxer()
{
    TUint i = 0;

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpExtraDataOri[i]) {
            free(mpExtraDataOri[i]);
            mpExtraDataOri[i] = NULL;
        }
        if (mpExtraData[i]) {
            free(mpExtraData[i]);
            mpExtraData[i] = NULL;
        }
        mExtraDataSizeOri[i] = 0;
        mExtraDataSize[i] = 0;
    }

    if (mpConversionBuffer) {
        free(mpConversionBuffer);
        mpConversionBuffer = NULL;
    }
    mConversionBufferSize = 0;
    mConversionDataSize = 0;

    if (mpGopIndexBaseUnaligned) {
        free(mpGopIndexBaseUnaligned);
        mpGopIndexBase = NULL;
        mpGopIndexBaseUnaligned = NULL;
        mCurrentGopIndex = 0;
    }

    if (mpIO) {
        mpIO->Delete();
        mpIO = NULL;
    }
}

EECode CTSMuxer::SetupContext()
{
    if (DLikely(!mpIO)) {
        mpIO = gfCreateIO(EIOType_File);
    } else {
        LOGM_WARN("mpIO is created yet?\n");
    }

    return EECode_OK;
}

EECode CTSMuxer::DestroyContext()
{
    if (DLikely(mpIO)) {
        mpIO->Delete();
        mpIO = NULL;
    } else {
        LOGM_WARN("mpIO is not created?\n");
    }

    return EECode_OK;
}

EECode CTSMuxer::SetSpecifiedInfo(SRecordSpecifiedInfo *info)
{
    return EECode_OK;
}

EECode CTSMuxer::GetSpecifiedInfo(SRecordSpecifiedInfo *info)
{
    return EECode_OK;
}

EECode CTSMuxer::SetExtraData(StreamType type, TUint stream_index, void *p_data, TUint data_size)
{
    DASSERT(stream_index < EConstMaxDemuxerMuxerStreamNumber);

    DASSERT(p_data);
    DASSERT(data_size);
    if (!p_data || !data_size) {
        LOGM_FATAL("[stream_index %d] p_data %p, data_size %d\n", stream_index, p_data, data_size);
        return EECode_BadParam;
    }

    LOGM_INFO("SetExtraData[%d], data %p, data size %d\n", stream_index, p_data, data_size);
    if (stream_index < EConstMaxDemuxerMuxerStreamNumber) {
        if (mpExtraData[stream_index]) {
            if (mExtraDataSize[stream_index] >= data_size) {
                LOGM_INFO("replace extra data\n");
                memcpy(mpExtraData[stream_index], p_data, data_size);
                mExtraDataSize[stream_index] = data_size;
            } else {
                LOGM_INFO("replace extra data, with larger size\n");
                free(mpExtraData[stream_index]);
                mpExtraData[stream_index] = (TU8 *)malloc(data_size);
                if (mpExtraData[stream_index]) {
                    memcpy(mpExtraData[stream_index], p_data, data_size);
                    mExtraDataSize[stream_index] = data_size;
                } else {
                    LOGM_FATAL("malloc(%d) fail\n", data_size);
                    return EECode_NoMemory;
                }
            }
        } else {
            mpExtraData[stream_index] = (TU8 *)malloc(data_size);
            if (mpExtraData[stream_index]) {
                memcpy(mpExtraData[stream_index], p_data, data_size);
                mExtraDataSize[stream_index] = data_size;
            } else {
                LOGM_FATAL("malloc(%d) fail\n", data_size);
                return EECode_NoMemory;
            }
        }
    } else {
        LOGM_FATAL("BAD stream_index %d\n", stream_index);
        return EECode_BadParam;
    }

    if (StreamType_Video == type) {
        //check the h264 extra data format
        TU8 *p = NULL;
        TU8 *p_extra = mpExtraData[stream_index];
        TU8 startcode[4] = {0, 0, 0, 0x01};
        TUint sps, pps;

        if (0x01 == p_extra[0]) {

            mH264DataFmt = H264_FMT_AVCC;
            LOGM_INFO("extradata is AVCC format, mH264AVCCNaluLen %d, need change to H264_FMT_AVCC to H264_FMT_ANNEXB.\n", mH264AVCCNaluLen);

            mbConvertH264DataFormat = 1;

            DASSERT(!mpExtraDataOri[stream_index]);
            DASSERT(!mExtraDataSizeOri[stream_index]);

            mpExtraDataOri[stream_index] = mpExtraData[stream_index];
            mExtraDataSizeOri[stream_index] = mExtraDataSize[stream_index];
            mpExtraData[stream_index] = NULL;
            mExtraDataSize[stream_index] = 0;

            mH264AVCCNaluLen = 1 + (p_extra[4] & 3);

            p = (TU8 *)malloc(mExtraDataSizeOri[stream_index] + 16);
            if (p) {
                mpExtraData[stream_index] = p;
                sps = BE_16(p_extra + 6);
                memcpy(p, startcode, sizeof(startcode));
                p += sizeof(startcode);
                mExtraDataSize[stream_index] += sizeof(startcode);
                memcpy(p, p_extra + 8, sps);
                p += sps;
                mExtraDataSize[stream_index] += sps;

                pps = BE_16(p_extra + 6 + 2 + sps + 1);
                memcpy(p, startcode, sizeof(startcode));
                p += sizeof(startcode);
                mExtraDataSize[stream_index] += sizeof(startcode);
                memcpy(p, p_extra + 6 + 2 + sps + 1 + 2, pps);
                mExtraDataSize[stream_index] += pps;
            } else {
                LOGM_FATAL("NO memory\n");
                return EECode_NoMemory;
            }

            LOGM_INFO("mExtraDataSize[stream_index] %d\n", mExtraDataSize[stream_index]);

        } else {
            mH264DataFmt = H264_FMT_ANNEXB;
            LOGM_INFO("extradata is H264_FMT_ANNEXB format.\n");
        }
    }

    return EECode_OK;
}

EECode CTSMuxer::SetPrivateDataDurationSeconds(void *p_data, TUint data_size)
{
    if (!p_data || sizeof(TU64) != data_size) {
        LOGM_ERROR("bad params, p_data %p, data_size %d\n", p_data, data_size);
        return EECode_BadParam;
    }
    mFileDurationSeconds = *(TU64 *)p_data;
    return EECode_OK;
}

EECode CTSMuxer::SetPrivateDataChannelName(void *p_data, TUint data_size)
{
    if ((!p_data) || (data_size > DMaxChannelNameLength - 1) || (!data_size)) {
        LOGM_ERROR("bad params, p_data %p, data_size %d\n", p_data, data_size);
        return EECode_BadParam;
    }
    TChar *p_tmp = (TChar *)p_data;
    strncpy(mChannelName, p_tmp, data_size);
    mChannelNameLength = data_size;
    return EECode_OK;
}

EECode CTSMuxer::InitializeFile(const SStreamCodecInfos *infos, TChar *url, ContainerType type, TChar *thumbnailname, TTime start_pts, TTime start_dts)
{
    EECode err = EECode_OK;

    if (mbSuspend) {
        return EECode_OK;
    }

    if (DUnlikely(!infos || !url)) {
        LOGM_ERROR("NULL input: url %p, infos %p\n", url, infos);
        return EECode_BadParam;
    }

    DASSERT(ContainerType_TS == type);

    mpUrlName = url;

    if (DUnlikely(mConfigLogLevel > ELogLevel_Notice)) {
        gfPrintCodecInfoes((SStreamCodecInfos *)infos);
    }

    LOGM_INFO("InitializeFile, start, mpUrlName %s\n", mpUrlName);

    DASSERT(infos->total_stream_number <= EConstMaxDemuxerMuxerStreamNumber);

    mEstimatedBitrate = 0;
    mnMaxStreamNumber = (mbAddCustomizedPrivateData && infos->total_stream_number < EConstMaxDemuxerMuxerStreamNumber) ? infos->total_stream_number + 1 : infos->total_stream_number;

    mPcrPatPmtFrameCount = 0;

    //hard code here
    mPrgInfo.pidPMT = DTS_PMT_PID;
    mPrgInfo.pidPCR = 0x0;
    mPrgInfo.prgNum = 1;
    mPrgInfo.totStream = mnMaxStreamNumber;

    mPatInfo.prgInfo = &mPrgInfo;
    mPmtInfo.prgInfo = &mPrgInfo;
    mPmtInfo.prgInfo->prgNum = mnMaxStreamNumber;
    mPatInfo.totPrg = 1;

    mPmtInfo.descriptor_tag = 5;
    mPmtInfo.descriptor_len = 4;
    mPmtInfo.pDescriptor[0] = 0x48;
    mPmtInfo.pDescriptor[1] = 0x44;
    mPmtInfo.pDescriptor[2] = 0x4d;
    mPmtInfo.pDescriptor[3] = 0x56;

    err = getMPEGPsiStreamInfo(infos);
    DASSERT_OK(err);

    DASSERT(!mbIOOpend);
    DASSERT(mpIO);
    if (mbIOOpend) {
        LOGM_WARN("mpIO is opened here? close it first\n");
        mpIO->Close();
        mbIOOpend = 0;
    }

    LOGM_INFO("before IIO->Open(%s).\n", mpUrlName);

    if (mpIO) {
        err = mpIO->Open(mpUrlName, EIOFlagBit_Write | EIOFlagBit_Binary);
        if (EECode_OK != err) {
            LOGM_INFO("IIO->Open(%s) fail, return %d, %s.\n", mpUrlName, err, gfGetErrorCodeString(err));
            return err;
        }
        mbIOOpend = 1;
    }
    LOGM_INFO("after IIO->Open(%s).\n", mpUrlName);

    createPATPacket(mPatPacket);
    createPMTPacket(mPmtPacket);

    insertPATPacket(mpCurrentPosInCache);
    mpCurrentPosInCache += MPEG_TS_TP_PACKET_SIZE;
    mCurrentTsPacketIndex++;
    insertPMTPacket(mpCurrentPosInCache);
    mpCurrentPosInCache += MPEG_TS_TP_PACKET_SIZE;
    mCurrentTsPacketIndex++;
    LOGM_DEBUG("InitializeFile, ts write PATPMT done, mCurrentTsPacketIndex=%u\n", mCurrentTsPacketIndex);
    if (mbAddCustomizedPrivateData) {
        err = insertCustomizedPrivateDataHeadPacket(mpCurrentPosInCache);
        if (EECode_OK != err) {
            LOGM_WARN("InitializeFile, insertCustomizedPrivateDataHeadPacket failed.\n");
        } else {
            mpCurrentPosInCache += MPEG_TS_TP_PACKET_SIZE;
            mCurrentTsPacketIndex++;
        }
    }

    return EECode_OK;
}

EECode CTSMuxer::FinalizeFile(SMuxerFileInfo *p_file_info)
{
    EECode err = EECode_OK;

    if (DLikely(mpIO)) {
        DASSERT(mbIOOpend);
        if (DLikely(mbIOOpend)) {
            if (mbAddCustomizedPrivateData) {
                err = insertCustomizedPrivateDataTailPacket();
                if (EECode_OK != err) {
                    LOGM_ERROR("FinalizeFile, insertCustomizedPrivateDataTailPacket failed.\n");
                }
            }

            err = writeToFile(mpCacheBufferBaseAligned, mpCurrentPosInCache - mpCacheBufferBaseAligned, 1);
            DASSERT_OK(err);
            LOGM_INFO("ts write mpTSPakIdx4GopStart to file size %lu.\n", (TULong)(mpCurrentPosInCache - mpCacheBufferBaseAligned));
            mpCurrentPosInCache = mpCacheBufferBaseAligned;
            mCurrentGopIndex = 0;

            mpIO->Sync();
            err = mpIO->Close();
            if (DUnlikely(EECode_OK != err)) {
                LOGM_WARN("IIO->Close(%s) fail, return %d, %s.\n", mpUrlName, err, gfGetErrorCodeString(err));
                return err;
            }
            mbIOOpend = 0;
        }
    }
    return EECode_OK;
}

void CTSMuxer::convertH264Data2annexb(TU8 *p_input, TUint size)
{
    DASSERT(p_input);
    DASSERT(size);

    if (p_input && size) {
        if (size > mConversionBufferSize) {
            if (mpConversionBuffer) {
                free(mpConversionBuffer);
                mpConversionBuffer = NULL;
            }

            mConversionBufferSize = size + 512;
            LOGM_ALWAYS("[memory]: malloc more memory for h264 conversion, request size %d\n", mConversionBufferSize);
            mpConversionBuffer = (TU8 *)malloc(mConversionBufferSize);
        }

        if (mpConversionBuffer) {

            TU8 startcode[4] = {0, 0, 0, 0x01};
            TU8 *p_data = p_input;
            TU8 *p_dest = mpConversionBuffer;
            TUint send_size = 0;

            mConversionDataSize = 0;

            while (size) {
                memcpy(p_dest, startcode, sizeof(startcode));
                mConversionDataSize += sizeof(startcode);
                p_dest += sizeof(startcode);

                DASSERT(mH264AVCCNaluLen <= 4);

                send_size = 0;
                for (TUint index = 0; index < mH264AVCCNaluLen; index++, p_data++) {
                    send_size = (send_size << 8) | (*p_data);
                }

                size -= mH264AVCCNaluLen;
                DASSERT(send_size <= size);
                size -= send_size;

                p_data = __validStartPoint(p_data, send_size);
                memcpy(p_dest, p_data, send_size);
                mConversionDataSize += send_size;
                p_data += send_size;
                p_dest += send_size;
            }
        } else {
            LOG_FATAL("No memory\n");
            mConversionDataSize = 0;
            mConversionBufferSize = 0;
        }

        DASSERT(mConversionDataSize <= mConversionBufferSize);
    }
}

EECode CTSMuxer::WriteVideoBuffer(CIBuffer *pBuffer, SMuxerDataTrunkInfo *info)
{
    if (!mbIOOpend) {
        return EECode_OK;
    }

    TMemSize packet_size = 0;
    TU8 *packet_data = NULL;
    TU8 index = info->stream_index;

    //LOGM_DEBUG("WriteVideoBuffer(index %d): mbConvertH264DataFormat %d, size %ld info->pts %lld, info->dts %lld, info->native_pts %lld, info->native_dts %lld, duration %lld, normalized duration %lld\n", index, mbConvertH264DataFormat, pBuffer->GetDataSize(), info->pts, info->dts, info->native_pts, info->native_dts, info->duration, info->av_normalized_duration);
    mDebugHeartBeat ++;

    if (!mbConvertH264DataFormat) {
        packet_size = pBuffer->GetDataSize();
        packet_data = pBuffer->GetDataPtr();
    } else {
        DASSERT(H264_FMT_AVCC == mH264DataFmt);
        convertH264Data2annexb(pBuffer->GetDataPtr(), pBuffer->GetDataSize());
        packet_size = mConversionDataSize;
        packet_data = mpConversionBuffer;
    }

    //ensure first frame are key frame
    if (DUnlikely(!mbVideoKeyFrameComes)) {
        DASSERT(pBuffer->mVideoFrameType == EPredefinedPictureType_IDR);
        if (pBuffer->mVideoFrameType == EPredefinedPictureType_IDR) {
            if (mbNeedFindSPSPPSInBitstream) {
                TU8 *pseq = packet_data;
                //extradata sps
                LOGM_INFO("Beginning to calculate sps-pps's length.\n");

                //get seq data for safe, not exceed boundary when data is corrupted by DSP
                TU8 *pIDR = NULL;
                TU8 *pstart = NULL;
                bool find_seq = gfGetSpsPps(pBuffer->GetDataPtr(), pBuffer->GetDataSize(), pseq, mExtraDataSize[index], pIDR, pstart);

                if (DLikely(true == find_seq)) {
                    if (!pstart) {
                        DASSERT(pBuffer->GetDataPtr() == pseq);
                        packet_data = pseq;
                        packet_size  -= (TMemSize)pseq - (TMemSize)pBuffer->GetDataPtr();
                        //pBuffer->SetDataSize(packet_size);
                        //pBuffer->SetDataPtr(packet_data);
                    } else {
                        packet_data = pstart;
                        packet_size  -= (TMemSize)pstart - (TMemSize)pBuffer->GetDataPtr();
                        //pBuffer->SetDataSize(packet_size);
                        //pBuffer->SetDataPtr(packet_data);
                    }
                } else {
                    //LOGM_FATAL("IDR data buffer corrupted!!, how to handle it?\n");
                    //msState = STATE_ERROR;
                    //return;
                }

                LOGM_INFO("Calculate sps-pps's length completely, len=%d.\n", mExtraDataSize[index]);
                DASSERT(mExtraDataSize[index] <= SPS_PPS_LEN);

                if (!mpExtraData[index]) {
                    mpExtraData[index] = (TU8 *)malloc(mExtraDataSize[index] + 8);
                    memset(mpExtraData[index], 0, mExtraDataSize[index]);
                }

                DASSERT(mpExtraData[index]);
                if (mpExtraData[index]) {
                    memcpy(mpExtraData[index], pseq, mExtraDataSize[index]);
                } else {
                    LOGM_FATAL("NULL mpExtraData[mVideoIndex(%d)], %p\n", index, mpExtraData[index]);
                }
            }
            //LOGM_INFO("Key frame comes, info->duration %lld, pBuffer->GetBufferPTS() %lld.\n", info->duration, pBuffer->GetBufferPTS());
            mbVideoKeyFrameComes = 1;
        } else {
            LOGM_WARN("non-IDR frame (%d), data comes here, seq num [%d, %d], when stream not started, discard them.\n", pBuffer->mVideoFrameType, pBuffer->mBufferSeqNumber0, pBuffer->mBufferSeqNumber1);
            return EECode_OK;
        }
    }

    if (DLikely(0 ==  mpPersistMediaConfig->muxer_skip_video)) {
        TULong videoPacketRemainedSize = packet_size;
        TULong videoPacketConsumedSize = 0;
        TU8 *psrc = packet_data;
        TU8 first_slice = 1;
        TU32 write_pcr_pat_pmt = !(mPcrPatPmtFrameCount & 0xff);

        if (DUnlikely(write_pcr_pat_pmt)) {
            if (DUnlikely((mpCurrentPosInCache + MPEG_TS_TP_PACKET_SIZE + MPEG_TS_TP_PACKET_SIZE) >= mpCacheBufferBaseAligned + mCacheBufferSize)) {
                EECode err = writeToFile(mpCacheBufferBaseAligned, mpCurrentPosInCache - mpCacheBufferBaseAligned, 1);
                DASSERT_OK(err);
                mpCurrentPosInCache = mpCacheBufferBaseAligned;
            }
            insertPATPacket(mpCurrentPosInCache);
            mpCurrentPosInCache += MPEG_TS_TP_PACKET_SIZE;
            mCurrentTsPacketIndex++;
            insertPMTPacket(mpCurrentPosInCache);
            mpCurrentPosInCache += MPEG_TS_TP_PACKET_SIZE;
            mCurrentTsPacketIndex++;
            LOGM_DEBUG("ts write PATPMT done, mCurrentTsPacketIndex=%u\n", mCurrentTsPacketIndex);
        }

        while (videoPacketRemainedSize > 0) {
            if (DUnlikely(mpCurrentPosInCache + MPEG_TS_TP_PACKET_SIZE >= mpCacheBufferBaseAligned + mCacheBufferSize)) {
                EECode err = writeToFile(mpCacheBufferBaseAligned, mpCurrentPosInCache - mpCacheBufferBaseAligned, 1);
                DASSERT_OK(err);
                mpCurrentPosInCache = mpCacheBufferBaseAligned;
            }

            if (mbAddCustomizedPrivateData && first_slice && pBuffer->mVideoFrameType == EPredefinedPictureType_IDR) {
                if (DLikely(mCurrentGopIndex < mMaxGopInFile)) {
                    mpGopIndexBase[mCurrentGopIndex] = mCurrentTsPacketIndex;
                    mCurrentGopIndex ++;
                } else {
                    mMaxGopInFile += 1024;
                    TU8 *ptmp = (TU8 *) malloc(mMaxGopInFile * sizeof(TU32) + 3);
                    if (DUnlikely(!ptmp)) {
                        LOGM_FATAL("no memory, request size %ld\n", (TULong)(mMaxGopInFile * sizeof(TU32) + 3));
                    }
                    TU32 *ptmp_aligned = (TU32 *)((TULong)(ptmp + 3) & (~0x3));
                    LOGM_NOTICE("increate index array, from %d to %d\n", mCurrentTsPacketIndex, mMaxGopInFile);
                    memcpy(ptmp_aligned, mpGopIndexBase, mCurrentGopIndex * sizeof(TU32));
                    free(mpGopIndexBaseUnaligned);
                    mpGopIndexBaseUnaligned = ptmp;
                    mpGopIndexBase = ptmp_aligned;
                    mpGopIndexBase[mCurrentGopIndex] = mCurrentTsPacketIndex;
                    mCurrentGopIndex ++;
                }
            }

            videoPacketConsumedSize = insertVideoPacket(mpCurrentPosInCache, psrc, videoPacketRemainedSize, write_pcr_pat_pmt, write_pcr_pat_pmt, info->is_key_frame,  info->stream_index, first_slice, info->pts);
            write_pcr_pat_pmt = 0;
            first_slice = 0;
            DASSERT(videoPacketRemainedSize >= videoPacketConsumedSize);
            videoPacketRemainedSize -= videoPacketConsumedSize;
            psrc += videoPacketConsumedSize;
            mpCurrentPosInCache += MPEG_TS_TP_PACKET_SIZE;
            mCurrentTsPacketIndex++;
        }

        mPcrPatPmtFrameCount ++;
    } else {
        LOGM_WARN("video is skiped\n");
    }

    return EECode_OK;
}

EECode CTSMuxer::WriteAudioBuffer(CIBuffer *pBuffer, SMuxerDataTrunkInfo *info)
{
    if (!mbIOOpend) {
        return EECode_OK;
    }

    TMemSize packet_size = 0;
    TU8 *packet_data = NULL;
    TU8 index = info->stream_index;

    mDebugHeartBeat ++;

    packet_size = pBuffer->GetDataSize();
    packet_data = pBuffer->GetDataPtr();

    if (DLikely(mpPersistMediaConfig->muxer_skip_audio == 0)) {
        TULong audioPacketRemainedSize = packet_size;
        TULong audioPacketConsumedSize = 0;
        TU8 *psrc = packet_data;
        TU8 first_slice = 1;
        LOGM_DEBUG("[A] mPcrBase=%llu, pts=%llu\n", mPcrBase, (TU64)info->pts);
        LOGM_DEBUG("before insert packet_size %lu to cache(audio)\n", packet_size);
        while (audioPacketRemainedSize > 0) {
            if (DUnlikely(mpCurrentPosInCache + MPEG_TS_TP_PACKET_SIZE >= mpCacheBufferBaseAligned + mCacheBufferSize)) {
                EECode err = writeToFile(mpCacheBufferBaseAligned, mpCurrentPosInCache - mpCacheBufferBaseAligned, 1);
                DASSERT_OK(err);
                LOGM_DEBUG("flush to file\n");
                mpCurrentPosInCache = mpCacheBufferBaseAligned;
            }
            if (0 == mCurrentTsPacketIndex % mPatPmtInterval) {
                insertPATPacket(mpCurrentPosInCache);
                mpCurrentPosInCache += MPEG_TS_TP_PACKET_SIZE;
                mCurrentTsPacketIndex++;
                insertPMTPacket(mpCurrentPosInCache);
                mpCurrentPosInCache += MPEG_TS_TP_PACKET_SIZE;
                mCurrentTsPacketIndex++;
                LOGM_DEBUG("ts write PATPMT done, mCurrentTsPacketIndex=%u\n", mCurrentTsPacketIndex);
            }
            audioPacketConsumedSize = insertAudioPacket(mpCurrentPosInCache, psrc, audioPacketRemainedSize, 1, 1, first_slice, info->pts, info->dts);
            first_slice = 0;
            DASSERT(audioPacketRemainedSize >= audioPacketConsumedSize);
            audioPacketRemainedSize -= audioPacketConsumedSize;
            psrc += audioPacketConsumedSize;
            mpCurrentPosInCache += MPEG_TS_TP_PACKET_SIZE;
            mCurrentTsPacketIndex++;
        }
    } else {
        LOGM_WARN("audio is skiped\n");
    }

    return EECode_OK;
}

EECode CTSMuxer::WritePridataBuffer(CIBuffer *pBuffer, SMuxerDataTrunkInfo *info)
{
    LOGM_FATAL("TO DO\n");
    return EECode_OK;
}

EECode CTSMuxer::SetState(TU8 b_suspend)
{
    mbSuspend = b_suspend;
    return EECode_OK;
}

void CTSMuxer::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    mDebugHeartBeat = 0;
}

EECode CTSMuxer::createPATPacket(TU8 *pcurrent)
{
    MPEG_TS_TP_HEADER *tsheader = (MPEG_TS_TP_HEADER *)pcurrent;
    PAT_HDR *patHeader  = (PAT_HDR *)(pcurrent + 5);
    TU8 *pPatContent = (TU8 *)(((TU8 *)patHeader) + PAT_HDR_SIZE);
    TInt crc32  = 0;

    tsheader->sync_byte = MPEG_TS_TP_SYNC_BYTE;
    tsheader->transport_error_indicator = MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
    tsheader->payload_unit_start_indicator = MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_START;
    tsheader->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_PRIORITY;
    tsheader->transport_scrambling_control = MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
    tsheader->adaptation_field_control = MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;
    tsheader->continuity_counter = mnPatContinuityCounter;
    MPEG_TS_TP_HEADER_PID_SET(tsheader, MPEG_TS_PAT_PID);

    pcurrent[4] = 0x00;

    patHeader->table_id = 0x00;
    patHeader->section_syntax_indicator = 1;
    patHeader->b0    = 0;
    patHeader->reserved0 = 0x3;
    patHeader->transport_stream_id_l = 0x00;
    patHeader->transport_stream_id_h = 0x00;
    patHeader->reserved1 = 0x3;
    patHeader->version_number    = mPSIVersion.pat;
    patHeader->current_next_indicator    = 1;
    patHeader->section_number    = 0x0;
    patHeader->last_section_number   = 0x0;
    patHeader->section_length0to7    = 0; //Update later
    patHeader->section_length8to11   = 0; //Update later

    // add informations for all programs    (only one for now)
    pPatContent[0] = (mPatInfo.prgInfo->prgNum >> 8) & 0xff;
    pPatContent[1] = mPatInfo.prgInfo->prgNum & 0xff;
    pPatContent[2] = 0xE0 | ((mPatInfo.prgInfo->pidPMT & 0x1fff) >> 8);
    pPatContent[3] = mPatInfo.prgInfo->pidPMT & 0xff;
    pPatContent += 4;

    // update patHdr.section_length
    TU16 section_len = pPatContent + 4 - (TU8 *)patHeader - 3 ;
    patHeader->section_length8to11 = (section_len & 0x0fff) >> 8;
    patHeader->section_length0to7  = (section_len & 0x00ff);

    // Calc CRC32
    crc32 = __crc32((TU8 *)patHeader, (int)(pPatContent - (TU8 *)patHeader));
    pPatContent[0] = (crc32 >> 24) & 0xff;
    pPatContent[1] = (crc32 >> 16) & 0xff;
    pPatContent[2] = (crc32 >> 8) & 0xff;
    pPatContent[3] = crc32 & 0xff;
    // Stuff rest of the packet
    memset(pPatContent + 4, 0xff, MPEG_TS_TP_PACKET_SIZE - (pPatContent + 4 - pcurrent));

    return EECode_OK;
}

EECode CTSMuxer::createPMTPacket(TU8 *pcurrent)
{
    MPEG_TS_TP_HEADER *tsheader = (MPEG_TS_TP_HEADER *)pcurrent;
    PMT_HDR *pmtHeader = (PMT_HDR *)(pcurrent + 5);
    TU8 *pPmtContent = (TU8 *)(((TU8 *)pmtHeader) + PMT_HDR_SIZE);
    TInt crc32;

    // TS HEADER
    tsheader->sync_byte = MPEG_TS_TP_SYNC_BYTE;
    tsheader->transport_error_indicator = MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
    tsheader->payload_unit_start_indicator = MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_START;
    tsheader->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_PRIORITY;

    MPEG_TS_TP_HEADER_PID_SET(tsheader, mPmtInfo.prgInfo->pidPMT);

    tsheader->transport_scrambling_control = MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
    tsheader->adaptation_field_control = MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;
    tsheader->continuity_counter = mnPmtContinuityCounter;

    pcurrent[4] = 0x00;

    // PMT HEADER
    pmtHeader->table_id = 0x02;
    pmtHeader->section_syntax_indicator = 1;
    pmtHeader->b0 = 0;
    pmtHeader->reserved0 = 0x3;
    pmtHeader->section_length0to7     = 0; //update later
    pmtHeader->section_length8to11    = 0; //update later
    pmtHeader->program_number_h  = (mPmtInfo.prgInfo->prgNum >> 8) & 0xff;
    pmtHeader->program_number_l  = mPmtInfo.prgInfo->prgNum & 0xff;
    pmtHeader->reserved1 = 0x3;
    pmtHeader->version_number = mPSIVersion.pmt;
    pmtHeader->current_next_indicator = 1;
    pmtHeader->section_number = 0x0;
    pmtHeader->last_section_number = 0x0;
    pmtHeader->reserved2 = 0x7;
    pmtHeader->PCR_PID8to12 = (mPmtInfo.prgInfo->pidPCR >> 8) & 0x1f;
    pmtHeader->PCR_PID0to7 = mPmtInfo.prgInfo->pidPCR & 0xff;
    pmtHeader->reserved3 = 0xf;
    if (mPmtInfo.descriptor_len == 0) {
        pmtHeader->program_info_length0to7 = 0;
        pmtHeader->program_info_length8to11 = 0;
    } else {
        pmtHeader->program_info_length8to11 = ((2 + mPmtInfo.descriptor_len) >> 8) & 0x0f;
        pmtHeader->program_info_length0to7 = ((2 + mPmtInfo.descriptor_len) & 0xff);
    }

    if (mPmtInfo.descriptor_len > 0) {
        pPmtContent[0] = mPmtInfo.descriptor_tag;
        pPmtContent[1] = mPmtInfo.descriptor_len;
        memcpy(&pPmtContent[2], mPmtInfo.pDescriptor, mPmtInfo.descriptor_len);
        pPmtContent += (2 + mPmtInfo.descriptor_len);
    }

    // Add all stream elements
    for (TU16 strNum = 0; strNum < mPmtInfo.prgInfo->totStream; ++ strNum) {
        STSMuxPsiStreamInfo *pStr = &mPmtInfo.stream[strNum];
        PMT_ELEMENT *pPmtElement = (PMT_ELEMENT *)pPmtContent;
        pPmtElement->stream_type = pStr->type;
        pPmtElement->reserved0   = 0x7; // 3 bits
        pPmtElement->elementary_PID8to12 = ((pStr->pid & 0x1fff) >> 8);
        pPmtElement->elementary_PID0to7  = (pStr->pid & 0xff);
        pPmtElement->reserved1   = 0xf; // 4 bits
        pPmtElement->ES_info_length_h = (pStr->descriptor_len > 0) ? (((pStr->descriptor_len + 2) >> 8) & 0x0f) : 0;
        pPmtElement->ES_info_length_l = (pStr->descriptor_len > 0) ? ((pStr->descriptor_len + 2) & 0xff) : 0;

        pPmtContent += PMT_ELEMENT_SIZE;

        // printf("pPmtContent2 %d, descriptor_len %d\n",
        //        pPmtContent - pBufPMT, pStr->descriptor_len);
        if (pStr->descriptor_len > 0) {
            pPmtContent[0] = pStr->descriptor_tag;  //descriptor_tag
            pPmtContent[1] = pStr->descriptor_len;  //descriptor_length
            memcpy(&pPmtContent[2], pStr->pDescriptor, pStr->descriptor_len);
            //printf("pPmtContent3 %d, 0x%x 0x%x, 0x%x\n",
            //       pPmtContent - pBufPMT, ((TU8*)pStr->pDescriptor)[0],
            //       ((TU8*)pStr->pDescriptor)[1],
            //       ((TU8*)pStr->pDescriptor)[2]);
            pPmtContent += (2 + pStr->descriptor_len);
        }
    }

    // update pmtHdr.section_length
    TU16 section_len = pPmtContent + 4 - ((TU8 *)pmtHeader + 3);
    pmtHeader->section_length8to11 = (section_len >> 8) & 0x0f;
    pmtHeader->section_length0to7  = (section_len & 0xff);

    crc32 = __crc32((TU8 *)pmtHeader, (TU32)(pPmtContent - (TU8 *)pmtHeader));
    pPmtContent[0] = (crc32 >> 24) & 0xff;
    pPmtContent[1] = (crc32 >> 16) & 0xff;
    pPmtContent[2] = (crc32 >> 8) & 0xff;
    pPmtContent[3] = crc32 & 0xff;
    memset((pPmtContent + 4), 0xff, (MPEG_TS_TP_PACKET_SIZE - (((TU8 *)pPmtContent + 4) - pcurrent)));

    return EECode_OK;
}

EECode CTSMuxer::insertCustomizedPrivateDataHeadPacket(TU8 *pcurrent)//file beginning, append starttime+duration+channelname
{
    MPEG_TS_TP_HEADER *tsheader = (MPEG_TS_TP_HEADER *)pcurrent;
    TU16 consumed_data_len = 0;
    TU16 init_pes_packet_len  = MPEG_TS_TP_PACKET_SIZE - MPEG_TS_TP_PACKET_HEADER_SIZE - DPESPakStartCodeSize - DPESPakStreamIDSize - DPESPakLengthSize;
    TU8 adaption_field_length = init_pes_packet_len \
                                - (sizeof(STSPrivateDataHeader) + sizeof(STSPrivateDataStartTime)) \
                                - (sizeof(STSPrivateDataHeader) + sizeof(STSPrivateDataDuration)) \
                                - (sizeof(STSPrivateDataHeader) + mChannelNameLength) \
                                - 1;//one byte is "adaption_field_length"
    TU16 pes_packet_len = init_pes_packet_len - (adaption_field_length + 1);
    TU8 *p_dst = NULL;
    STSPrivateDataPesPacketHeader *pridataPesHeader = (STSPrivateDataPesPacketHeader *)(pcurrent + MPEG_TS_TP_PACKET_HEADER_SIZE + (adaption_field_length ? adaption_field_length + 1 : 2));
    TU8 *p_data_dest = (TU8 *)pridataPesHeader + DPESPakStartCodeSize + DPESPakStreamIDSize + DPESPakLengthSize;
    SDateTime time = {0};
    tsheader->sync_byte = MPEG_TS_TP_SYNC_BYTE;
    tsheader->transport_error_indicator = MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
    tsheader->payload_unit_start_indicator = MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_START;
    tsheader->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_NO_PRIORITY;
    tsheader->transport_scrambling_control = MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
    tsheader->adaptation_field_control = MPEG_TS_ADAPTATION_FIELD_BOTH;
    tsheader->continuity_counter = mnCustomizedPrivateDataContinuityCounter++;
    MPEG_TS_TP_HEADER_PID_SET(tsheader, DTS_PRIDATA_PID_START);

    MPEG_TS_TP_ADAPTATION_FIELD_HEADER *adaptHdr = (MPEG_TS_TP_ADAPTATION_FIELD_HEADER *)(pcurrent + MPEG_TS_TP_PACKET_HEADER_SIZE);
    adaptHdr->adaptation_field_length         = adaption_field_length;
    if (0 == adaptHdr->adaptation_field_length) {
        p_dst = (TU8 *)adaptHdr + 1;
        *p_dst = 0xff;
    } else {
        adaptHdr->adaptation_field_extension_flag =               MPEG_TS_ADAPTATION_FIELD_EXTENSION_NOT_PRESENT;
        adaptHdr->transport_private_data_flag     =               MPEG_TS_TRANSPORT_PRIVATE_DATA_NOT_PRESENT;
        adaptHdr->splicing_point_flag             =               MPEG_TS_SPLICING_POINT_FLAG_NOT_PRESENT;
        adaptHdr->opcr_flag                       = MPEG_TS_OPCR_FLAG_NOT_PRESENT;
        adaptHdr->pcr_flag                        = MPEG_TS_PCR_FLAG_NOT_PRESENT;
        adaptHdr->elementary_stream_priority_indicator =               MPEG_TS_ELEMENTARY_STREAM_PRIORITY_NO_PRIORITY;
        adaptHdr->random_access_indicator              =               MPEG_TS_RANDOM_ACCESS_INDICATOR_NOT_PRESENT;
        adaptHdr->discontinuity_indicator = MPEG_TS_DISCONTINUITY_INDICATOR_NO_DISCONTINUTY;
        p_dst = (TU8 *)adaptHdr + 2;
        memset(p_dst, 0xff, adaption_field_length - 1);
    }

    pridataPesHeader->start_code_prefix_23to16 = 0x00;
    pridataPesHeader->start_code_prefix_15to8 = 0x00;
    pridataPesHeader->start_code_prefix_7to0 = 0x01;
    pridataPesHeader->stream_id = 0xBF;
    pridataPesHeader->packet_len_15to8 = (pes_packet_len >> 8) & 0xff;
    pridataPesHeader->packet_len_7to0 = pes_packet_len & 0xff;

    EECode err = gfGetCurrentDateTime(&time);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("gfGetCurrentDateTime return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }
    p_data_dest = gfAppendTSPriData(p_data_dest, &time, ETSPrivateDataType_StartTime, 0, sizeof(SDateTime), sizeof(SDateTime), consumed_data_len);
    if (DUnlikely(!p_data_dest)) {
        LOG_ERROR("gfAppendTSPriData ETSPrivateDataType_StartTime failed.\n");
        return EECode_InternalLogicalBug;
    }
    p_data_dest = gfAppendTSPriData(p_data_dest, &mFileDurationSeconds, ETSPrivateDataType_Duration, 0, sizeof(TU16), sizeof(TU16), consumed_data_len);
    if (DUnlikely(!p_data_dest)) {
        LOG_ERROR("gfAppendTSPriData ETSPrivateDataType_Duration failed.\n");
        return EECode_InternalLogicalBug;
    }
    p_data_dest = gfAppendTSPriData(p_data_dest, mChannelName, ETSPrivateDataType_ChannelName, 0, mChannelNameLength, DMaxChannelNameLength, consumed_data_len);
    if (DUnlikely(!p_data_dest)) {
        LOG_ERROR("gfAppendTSPriData ETSPrivateDataType_ChannelName failed.\n");
        return EECode_InternalLogicalBug;
    }

    DASSERT(MPEG_TS_TP_PACKET_SIZE == (p_data_dest - pcurrent));

    return EECode_OK;
}

EECode CTSMuxer::insertCustomizedPrivateDataTailPacket()
{
    if (DUnlikely(!mCurrentGopIndex)) {
        LOGM_WARN("no index\n");
        return EECode_NotExist;
    }

    TU32 payload_length = mCurrentGopIndex * sizeof(TU32);
    TU32 packet_number = (payload_length + sizeof(STSPrivateDataPesPacketHeader) + sizeof(STSPrivateDataHeader) + 2 + MPEG_TS_TP_PACKET_SIZE - MPEG_TS_TP_PACKET_HEADER_SIZE - 1) / (MPEG_TS_TP_PACKET_SIZE - MPEG_TS_TP_PACKET_HEADER_SIZE));

    TU8 *p_dst = NULL;
    TU32 *p_src = mpGopIndexBase;
    TU32 i = 0, max = 0, remain_index = 0;

    if (DUnlikely((mpCurrentPosInCache + (packet_number * MPEG_TS_TP_PACKET_SIZE)) > (mpCacheBufferBaseAligned + mCacheBufferSize))) {
        EECode err = writeToFile(mpCacheBufferBaseAligned, mpCurrentPosInCache - mpCacheBufferBaseAligned, 1);
        DASSERT_OK(err);
        LOGM_DEBUG("flush to file\n");
        mpCurrentPosInCache = mpCacheBufferBaseAligned;
    }

    MPEG_TS_TP_HEADER *tsheader = (MPEG_TS_TP_HEADER *) mpCurrentPosInCache;
    tsheader->sync_byte = MPEG_TS_TP_SYNC_BYTE;
    tsheader->transport_error_indicator = MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
    tsheader->payload_unit_start_indicator = MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_START;
    tsheader->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_NO_PRIORITY;
    tsheader->transport_scrambling_control = MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
    tsheader->adaptation_field_control = MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;
    tsheader->continuity_counter = mnCustomizedPrivateDataContinuityCounter++;
    MPEG_TS_TP_HEADER_PID_SET(tsheader, DTS_PRIDATA_PID_START);

    STSPrivateDataPesPacketHeader *pridata_pes_header = (STSPrivateDataPesPacketHeader *)(mpCurrentPosInCache + MPEG_TS_TP_PACKET_HEADER_SIZE);
    pridata_pes_header->start_code_prefix_23to16 = 0x00;
    pridata_pes_header->start_code_prefix_15to8 = 0x00;
    pridata_pes_header->start_code_prefix_7to0 = 0x01;
    pridata_pes_header->stream_id = 0xBF;
    remain_index = (MPEG_TS_TP_PACKET_SIZE - MPEG_TS_TP_PACKET_HEADER_SIZE) * packet_number - sizeof(STSPrivateDataPesPacketHeader);
    DASSERT(remain_index < (1 << 16));
    pridata_pes_header->packet_len_15to8 = (remain_index >> 8) & 0xff;
    pridata_pes_header->packet_len_7to0 = remain_index & 0xff;

    STSPrivateDataHeader *p_header = (STSPrivateDataHeader *)(((TU8 *)pridata_pes_header) + sizeof(STSPrivateDataPesPacketHeader));
    p_header->data_type_4cc_31to24 = 0x54;  //T
    p_header->data_type_4cc_23to16 = 0x53;   //S
    p_header->data_type_4cc_15to8 = 0x50;    //P
    p_header->data_type_4cc_7to0 = 0x49;     //I
    p_header->data_sub_type_15to8 = (ETSPrivateDataType_TSPakIdx4GopStart >> 8) & 0xff;
    p_header->data_sub_type_7to0 = ETSPrivateDataType_TSPakIdx4GopStart & 0xff;
    p_header->data_length_15to8 = (payload_length >> 8) & 0xff;
    p_header->data_length_7to0 = payload_length & 0xff;

    remain_index = packet_number;
    p_dst = (TU8 *) p_header + sizeof(STSPrivateDataHeader);
    max = (TU32)(MPEG_TS_TP_PACKET_SIZE - MPEG_TS_TP_PACKET_HEADER_SIZE - sizeof(STSPrivateDataPesPacketHeader) - sizeof(STSPrivateDataHeader)) >> 2;
    for (i = 0; i < max; i ++) {
    DBEW32(p_src[i], p_dst);
        p_dst += sizeof(TU32);
    }
    p_src += max;
    remain_index -= max;
    mpCurrentPosInCache += MPEG_TS_TP_PACKET_SIZE;

    max = (MPEG_TS_TP_PACKET_SIZE - MPEG_TS_TP_PACKET_HEADER_SIZE) >> 2;

    while (remain_index > 0) {

    MPEG_TS_TP_HEADER *tsheader = (MPEG_TS_TP_HEADER *)mpCurrentPosInCache;
        tsheader->sync_byte = MPEG_TS_TP_SYNC_BYTE;
        tsheader->transport_error_indicator = MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
        tsheader->payload_unit_start_indicator = MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_NORMAL;
        tsheader->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_NO_PRIORITY;
        tsheader->transport_scrambling_control = MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
        tsheader->adaptation_field_control = MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;
        tsheader->continuity_counter = mnCustomizedPrivateDataContinuityCounter ++;
        MPEG_TS_TP_HEADER_PID_SET(tsheader, DTS_PRIDATA_PID_START);

        p_dst = mpCurrentPosInCache + MPEG_TS_TP_PACKET_HEADER_SIZE;

        if (DUnlikely(remain_index < max)) {
            for (i = 0; i < remain_index; i ++) {
                DBEW32(p_src[i], p_dst);
                p_dst += sizeof(TU32);
            }
            mpCurrentPosInCache += MPEG_TS_TP_PACKET_SIZE;
            return EECode_OK;
        }

        for (i = 0; i < max; i ++) {
            DBEW32(p_src[i], p_dst);
            p_dst += sizeof(TU32);
        }
        p_src += max;
        mpCurrentPosInCache += MPEG_TS_TP_PACKET_SIZE;
    }

    return EECode_OK;
}

void CTSMuxer::insertPATPacket(TU8 *pcurrent)
{
    MPEG_TS_TP_HEADER *tsheader = (MPEG_TS_TP_HEADER *)mPatPacket;
    tsheader->continuity_counter = mnPatContinuityCounter++;

    DASSERT(pcurrent);
    memcpy(pcurrent, mPatPacket, MPEG_TS_TP_PACKET_SIZE);
}

void CTSMuxer::insertPMTPacket(TU8 *pcurrent)
{
    MPEG_TS_TP_HEADER *tsheader = (MPEG_TS_TP_HEADER *)mPmtPacket;
    tsheader->continuity_counter = mnPmtContinuityCounter++;

    DASSERT(pcurrent);
    memcpy(pcurrent, mPmtPacket, MPEG_TS_TP_PACKET_SIZE);
}

TFileSize CTSMuxer::insertVideoPacket(TU8 *p_dst, TU8 *psrc, TFileSize remaining_size, TU8 with_pcr, TU8 with_pts, TU8 is_key_frame, TU8 stream_index, TU8 firstslice, TTime pts)
{
    MPEG_TS_TP_HEADER *tsheader = (MPEG_TS_TP_HEADER *)p_dst;
    TU8 *pesPacket   = (TU8 *)(p_dst + MPEG_TS_TP_PACKET_HEADER_SIZE);
    TU8 *tmpWritePtr = NULL;

    TU32 pesPtsDtsFlag = MPEG_TS_PTS_DTS_NO_PTSDTS;
    TU32 pesHeaderDataLength = 0;
    if (firstslice) {
        pesPtsDtsFlag = MPEG_TS_PTS_DTS_PTS_ONLY;
        pesHeaderDataLength = PTS_FIELD_SIZE;
    }

    tsheader->sync_byte = MPEG_TS_TP_SYNC_BYTE;
    tsheader->transport_error_indicator = MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
    tsheader->payload_unit_start_indicator =
        (firstslice ? MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_START :
         MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_NORMAL);
    tsheader->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_NO_PRIORITY;
    tsheader->transport_scrambling_control = MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
    tsheader->continuity_counter = mnVideoContinuityCounter & 0x0f;
    mnVideoContinuityCounter++;

    TInt initialAdaptFieldSize = with_pcr ? (2 + PCR_FIELD_SIZE) : 0;
    // check if stuffing is required and calculate stuff size
    TU32 PESHdrSize = 0;
    TInt stuffSize = 0;
    if (firstslice) {
        PESHdrSize = PES_HEADER_LEN + pesHeaderDataLength;
    }
    TInt PESPayloadSpace = MPEG_TS_TP_PACKET_SIZE - MPEG_TS_TP_PACKET_HEADER_SIZE - initialAdaptFieldSize - PESHdrSize;
    if (remaining_size < PESPayloadSpace) {
        stuffSize = PESPayloadSpace - remaining_size;
    }
    // update adaptation_field_control of TS header
    tsheader->adaptation_field_control = (with_pcr || stuffSize > 0) ? MPEG_TS_ADAPTATION_FIELD_BOTH : MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;

    MPEG_TS_TP_HEADER_PID_SET(tsheader, mPmtInfo.stream[mVideoIndex].pid);

    MPEG_TS_TP_ADAPTATION_FIELD_HEADER *adaptHdr = (MPEG_TS_TP_ADAPTATION_FIELD_HEADER *)((TU8 *)tsheader + MPEG_TS_TP_PACKET_HEADER_SIZE);
    tmpWritePtr = (TU8 *)adaptHdr;

    if (MPEG_TS_ADAPTATION_FIELD_ADAPTATION_ONLY == tsheader->adaptation_field_control
            || MPEG_TS_ADAPTATION_FIELD_BOTH == tsheader->adaptation_field_control) {
        // fill the common fields
        adaptHdr->adaptation_field_length    = 0;
        adaptHdr->adaptation_field_extension_flag = MPEG_TS_ADAPTATION_FIELD_EXTENSION_NOT_PRESENT;
        adaptHdr->transport_private_data_flag    = MPEG_TS_TRANSPORT_PRIVATE_DATA_NOT_PRESENT;
        adaptHdr->splicing_point_flag    = MPEG_TS_SPLICING_POINT_FLAG_NOT_PRESENT;
        adaptHdr->opcr_flag  = MPEG_TS_OPCR_FLAG_NOT_PRESENT;
        adaptHdr->pcr_flag   = MPEG_TS_PCR_FLAG_NOT_PRESENT;
        adaptHdr->elementary_stream_priority_indicator = MPEG_TS_ELEMENTARY_STREAM_PRIORITY_NO_PRIORITY;
        adaptHdr->random_access_indicator    =   MPEG_TS_RANDOM_ACCESS_INDICATOR_NOT_PRESENT;
        adaptHdr->discontinuity_indicator = MPEG_TS_DISCONTINUITY_INDICATOR_NO_DISCONTINUTY;

        if (DUnlikely(with_pcr)) {
            //# of bytes following adaptation_field_length (1 + 6 pcr size)
            adaptHdr->adaptation_field_length = 1 + PCR_FIELD_SIZE;
            adaptHdr->pcr_flag                = MPEG_TS_PCR_FLAG_PRESENT;
            adaptHdr->random_access_indicator =
                MPEG_TS_RANDOM_ACCESS_INDICATOR_PRESENT;

            /*skip adapt hdr and point to optional field (PCR field)*/
            tmpWritePtr   += ADAPT_HEADER_LEN;
            tmpWritePtr[0] = ((TU64)pts) >> 25;
            tmpWritePtr[1] = (((TU64)pts) & 0x1fe0000) >> 17;
            tmpWritePtr[2] = (((TU64)pts) & 0x1fe00) >> 9;
            tmpWritePtr[3] = (((TU64)pts) & 0x1fe) >> 1;
            tmpWritePtr[4] = ((((TU64)pts) & 0x1) << 7) | (0x7e);
            tmpWritePtr[5] = 0x0; //((TU16)0) & 0xff;
            /*point to the start of potential stuffing area*/
            tmpWritePtr += PCR_FIELD_SIZE;
            //initialAdaptFieldSize = adaptHdr->adaptation_field_length + 1;
        }

        if (stuffSize > 0) { // need stuffing
            if (initialAdaptFieldSize == 0) { // adapt header is not written yet
                if (stuffSize == 1) {
                    adaptHdr->adaptation_field_length = 0;// write the adapt_field_length byte (=0)
                    tmpWritePtr += 1; //0x00 ->tmpWritePtr_now
                } else if (stuffSize == 2) {
                    adaptHdr->adaptation_field_length = 1;
                    tmpWritePtr += 2;
                } else {
                    adaptHdr->adaptation_field_length = stuffSize - 1;
                    memset(tmpWritePtr + 2, 0xff, stuffSize - 2);
                    tmpWritePtr += stuffSize;
                }
            } else {
                // tmpWritePtr should point to the start of stuffing area
                memset(tmpWritePtr, 0xff, stuffSize);
                tmpWritePtr += stuffSize;
                adaptHdr->adaptation_field_length += stuffSize;
            }
        }
    }

    pesPacket = tmpWritePtr;

    if (tsheader->payload_unit_start_indicator) {
        // one PES packet is started in this transport packet
        MPEG_TS_TP_PES_HEADER *pesHeader = (MPEG_TS_TP_PES_HEADER *)pesPacket;

        pesHeader->packet_start_code_23to16 = 0;
        pesHeader->packet_start_code_15to8  = 0;
        pesHeader->packet_start_code_7to0   = 0x01;
        pesHeader->marker_bits  = 2;
        pesHeader->pes_scrambling_control   = MPEG_TS_PES_SCRAMBLING_CTRL_NOT_SCRAMBLED;
        pesHeader->pes_priority = 0;
        pesHeader->data_alignment_indicator = MPEG_TS_PES_ALIGNMENT_CONTROL_STARTCODE;
        pesHeader->copyright    = MPEG_TS_PES_COPYRIGHT_UNDEFINED;
        pesHeader->original_or_copy     = MPEG_TS_PES_ORIGINAL_OR_COPY_COPY;
        pesHeader->escr_flag    = MPEG_TS_PES_ESCR_NOT_PRESENT;
        pesHeader->es_rate_flag = MPEG_TS_PES_ES_NOT_PRESENT;
        pesHeader->dsm_trick_mode_flag  = MPEG_TS_PES_DSM_TRICK_MODE_NOT_PRESENT;
        pesHeader->add_copy_info_flag   = MPEG_TS_PES_ADD_COPY_INFO_NOT_PRESENT;
        pesHeader->pes_crc_flag = MPEG_TS_PES_CRC_NOT_PRESENT;
        pesHeader->pes_ext_flag = MPEG_TS_PES_EXT_NOT_PRESENT;
        pesHeader->pts_dts_flags    = pesPtsDtsFlag;
        pesHeader->header_data_length   = pesHeaderDataLength;

        TU16 pesPacketSize;

        pesHeader->stream_id = MPEG_TS_STREAM_ID_VIDEO_00;
        if (remaining_size < (MPEG_TS_TP_PACKET_SIZE - (((TU8 *)pesPacket) - p_dst) - PES_HEADER_LEN - pesHeader->header_data_length)) {
            pesPacketSize = 3 + pesHeader->header_data_length + (remaining_size);
        } else {
            pesPacketSize = 0;
        }

        pesHeader->pes_packet_length_15to8 = ((pesPacketSize) >> 8) & 0xff;
        pesHeader->pes_packet_length_7to0  = (pesPacketSize) & 0xff;

        // point to PES header optional field
        tmpWritePtr += PES_HEADER_LEN;

        switch (pesHeader->pts_dts_flags) {

            case MPEG_TS_PTS_DTS_PTS_ONLY:
                tmpWritePtr[0] = 0x21 | (((((TU64)pts) >> 30) & 0x07) << 1);
                tmpWritePtr[1] = ((((TU64)pts) >> 22) & 0xff);
                tmpWritePtr[2] = (((((TU64)pts) >> 15) & 0x7f) << 1) | 0x1;
                tmpWritePtr[3] = ((((TU64)pts) >> 7) & 0xff);
                tmpWritePtr[4] = ((((TU64)pts) & 0x7f) << 1) | 0x1;
                break;

            case MPEG_TS_PTS_DTS_NO_PTSDTS:
                LOGM_ERROR("TODO, no pts/dts?\n");
                break;

            default:
                LOGM_FATAL("why comes here\n");
                break;
        }
        tmpWritePtr += pesHeader->header_data_length;
    }

    if (is_key_frame && firstslice) {
        memcpy(tmpWritePtr, mpExtraData[stream_index], mExtraDataSize[stream_index]);
        tmpWritePtr += mExtraDataSize[stream_index];
    }

    TFileSize payload_fillsize = MPEG_TS_TP_PACKET_SIZE - (tmpWritePtr - p_dst);

    DASSERT(payload_fillsize <= remaining_size);
    memcpy(tmpWritePtr, psrc, payload_fillsize);
    return payload_fillsize;
}

TFileSize CTSMuxer::insertAudioPacket(TU8 *p_dst, TU8 *psrc, TFileSize remaining_size, TU8 with_pts, TU8 firstslice, TTime pts)
{
    MPEG_TS_TP_HEADER *tsheader = (MPEG_TS_TP_HEADER *)p_dst;
    TU8 *pesPacket   = (TU8 *)(p_dst + MPEG_TS_TP_PACKET_HEADER_SIZE);
    TU8 *tmpWritePtr = NULL;

    TU32 pesPtsDtsFlag = MPEG_TS_PTS_DTS_NO_PTSDTS;
    TU32 pesHeaderDataLength = 0;
    if (DLikely(firstslice)) {
        pesPtsDtsFlag = MPEG_TS_PTS_DTS_PTS_ONLY;
        pesHeaderDataLength = PTS_FIELD_SIZE;
    }

    tsheader->sync_byte = MPEG_TS_TP_SYNC_BYTE;
    tsheader->transport_error_indicator = MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
    tsheader->payload_unit_start_indicator =
        (firstslice ? MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_START :
         MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_NORMAL);
    tsheader->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_NO_PRIORITY;
    tsheader->transport_scrambling_control = MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
    tsheader->continuity_counter = mnAudioContinuityCounter & 0x0f;
    mnAudioContinuityCounter++;

    TU32 PESHdrSize = 0;
    TInt stuffSize = 0;
    if (firstslice) {
        PESHdrSize = PES_HEADER_LEN + pesHeaderDataLength;
    }

    TInt PESPayloadSpace = MPEG_TS_TP_PACKET_SIZE - MPEG_TS_TP_PACKET_HEADER_SIZE - PESHdrSize;
    if (remaining_size < PESPayloadSpace) {
        stuffSize = PESPayloadSpace - remaining_size;
    }

    tsheader->adaptation_field_control = (stuffSize > 0) ? MPEG_TS_ADAPTATION_FIELD_BOTH : MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;

    MPEG_TS_TP_HEADER_PID_SET(tsheader, mPmtInfo.stream[mAudioIndex].pid);

    MPEG_TS_TP_ADAPTATION_FIELD_HEADER *adaptHdr = (MPEG_TS_TP_ADAPTATION_FIELD_HEADER *)((TU8 *)tsheader + MPEG_TS_TP_PACKET_HEADER_SIZE);
    tmpWritePtr = (TU8 *)adaptHdr;

    if (MPEG_TS_ADAPTATION_FIELD_ADAPTATION_ONLY == tsheader->adaptation_field_control
            || MPEG_TS_ADAPTATION_FIELD_BOTH == tsheader->adaptation_field_control) {
        // fill the common fields
        adaptHdr->adaptation_field_length         = 0;
        adaptHdr->adaptation_field_extension_flag = MPEG_TS_ADAPTATION_FIELD_EXTENSION_NOT_PRESENT;
        adaptHdr->transport_private_data_flag     = MPEG_TS_TRANSPORT_PRIVATE_DATA_NOT_PRESENT;
        adaptHdr->splicing_point_flag    = MPEG_TS_SPLICING_POINT_FLAG_NOT_PRESENT;
        adaptHdr->opcr_flag = MPEG_TS_OPCR_FLAG_NOT_PRESENT;
        adaptHdr->pcr_flag  = MPEG_TS_PCR_FLAG_NOT_PRESENT;
        adaptHdr->elementary_stream_priority_indicator = MPEG_TS_ELEMENTARY_STREAM_PRIORITY_NO_PRIORITY;
        adaptHdr->random_access_indicator   = MPEG_TS_RANDOM_ACCESS_INDICATOR_NOT_PRESENT;
        adaptHdr->discontinuity_indicator = MPEG_TS_DISCONTINUITY_INDICATOR_NO_DISCONTINUTY;

        //LOGM_DEBUG("remaining_size=%lld, PESPayloadSpace=%d, PESHdrSize=%d, stuffSize=%d, tmpWritePtr=%p, mpCacheBufferBaseAligned=%p\n",
        //remaining_size, PESPayloadSpace, PESHdrSize, stuffSize, tmpWritePtr, mpCacheBufferBaseAligned);

        if (stuffSize > 0) { // need stuffing

            if (stuffSize == 1) {
                adaptHdr->adaptation_field_length = 0;// write the adapt_field_length byte (=0)
                tmpWritePtr += 1; //0x00 ->tmpWritePtr_now
            } else if (stuffSize == 2) {
                adaptHdr->adaptation_field_length = 1;
                tmpWritePtr += 2;
            } else {
                adaptHdr->adaptation_field_length = stuffSize - 1;
                memset(tmpWritePtr + 2, 0xff, stuffSize - 2);
                tmpWritePtr += stuffSize;
            }
        }
    }

    pesPacket = tmpWritePtr;

    if (tsheader->payload_unit_start_indicator) {

        MPEG_TS_TP_PES_HEADER *pesHeader = (MPEG_TS_TP_PES_HEADER *)pesPacket;

        pesHeader->packet_start_code_23to16 = 0;
        pesHeader->packet_start_code_15to8  = 0;
        pesHeader->packet_start_code_7to0   = 0x01;
        pesHeader->marker_bits  = 2;
        pesHeader->pes_scrambling_control   = MPEG_TS_PES_SCRAMBLING_CTRL_NOT_SCRAMBLED;
        pesHeader->pes_priority             = 0;
        pesHeader->data_alignment_indicator = MPEG_TS_PES_ALIGNMENT_CONTROL_STARTCODE;
        pesHeader->copyright    = MPEG_TS_PES_COPYRIGHT_UNDEFINED;
        pesHeader->original_or_copy = MPEG_TS_PES_ORIGINAL_OR_COPY_COPY;
        pesHeader->escr_flag    = MPEG_TS_PES_ESCR_NOT_PRESENT;
        pesHeader->es_rate_flag = MPEG_TS_PES_ES_NOT_PRESENT;
        pesHeader->dsm_trick_mode_flag  = MPEG_TS_PES_DSM_TRICK_MODE_NOT_PRESENT;
        pesHeader->add_copy_info_flag   = MPEG_TS_PES_ADD_COPY_INFO_NOT_PRESENT;
        pesHeader->pes_crc_flag = MPEG_TS_PES_CRC_NOT_PRESENT;
        pesHeader->pes_ext_flag = MPEG_TS_PES_EXT_NOT_PRESENT;
        pesHeader->pts_dts_flags    = pesPtsDtsFlag;
        pesHeader->header_data_length   = pesHeaderDataLength;

        TU16 pesPacketSize;

        pesHeader->stream_id = MPEG_TS_STREAM_ID_AUDIO_00;
        pesPacketSize = 3 + pesHeader->header_data_length + (remaining_size);

        pesHeader->pes_packet_length_15to8 = ((pesPacketSize) >> 8) & 0xff;
        pesHeader->pes_packet_length_7to0  = (pesPacketSize) & 0xff;

        tmpWritePtr += PES_HEADER_LEN;

        switch (pesHeader->pts_dts_flags) {
            case MPEG_TS_PTS_DTS_PTS_ONLY:
                tmpWritePtr[0] = 0x21 | ((((TU64)pts >> 30) & 0x07) << 1);
                tmpWritePtr[1] = (((TU64)pts >> 22) & 0xff);
                tmpWritePtr[2] = ((((TU64)pts >> 15) & 0x7f) << 1) | 0x1;
                tmpWritePtr[3] = (((TU64)pts >> 7) & 0xff);
                tmpWritePtr[4] = (((TU64)pts & 0x7f) << 1) | 0x1;
                break;

            case MPEG_TS_PTS_DTS_PTSDTS_BOTH:
                tmpWritePtr[0]  = 0x31 | ((((TU64)pts >> 30) & 0x07) << 1);
                tmpWritePtr[1]  = (((TU64)pts >> 22) & 0xff);
                tmpWritePtr[2]  = ((((TU64)pts >> 15) & 0x7f) << 1) | 0x1;
                tmpWritePtr[3]  = (((TU64)pts >> 7) & 0xff);
                tmpWritePtr[4]  = (((TU64)pts & 0x7f) << 1) | 0x1;

                tmpWritePtr[5]  = 0x11 | ((((TU64)dts >> 30) & 0x07) << 1);
                tmpWritePtr[6]  = (((TU64)dts >> 22) & 0xff);
                tmpWritePtr[7]  = ((((TU64)dts >> 15) & 0x7f) << 1) | 0x1;
                tmpWritePtr[8]  = (((TU64)dts >> 7) & 0xff);
                tmpWritePtr[9]  = (((TU64)dts & 0x7f) << 1) | 0x1;
                break;

            case MPEG_TS_PTS_DTS_NO_PTSDTS:
                LOGM_ERROR("no pts, dts?\n");
                break;

            default:
                LOGM_FATAL("why comes here\n");
                break;
        }
        tmpWritePtr += pesHeader->header_data_length;
    }

    TFileSize payload_fillsize = MPEG_TS_TP_PACKET_SIZE - (tmpWritePtr - p_dst);

    DASSERT(payload_fillsize <= remaining_size);
    memcpy(tmpWritePtr, psrc, payload_fillsize);
    return payload_fillsize;
}

EECode CTSMuxer::writeToFile(TU8 *pstart, TFileSize size, TU8 sync)
{
    if (DLikely(mpIO)) {
        EECode ret = mpIO->Write(pstart, 1, size, sync);
        if (DUnlikely(EECode_OK != ret)) {
            LOGM_ERROR("mpIO->Write failed, ret=%d, %s, count=%lld\n", ret, gfGetErrorCodeString(ret), size);
            return ret;
        }
        return EECode_OK;
    }

    LOGM_FATAL("mpIO is null, cannot write to file.\n");
    return EECode_IOError;
}

EECode CTSMuxer::getMPEGPsiStreamInfo(const SStreamCodecInfos *infos)
{
    TUint i = 0;
    DASSERT(infos);

    if (DLikely(infos)) {

        for (i = 0; i < infos->total_stream_number; i++) {

            switch (infos->info[i].stream_format) {

                case StreamFormat_H264:
                    mPmtInfo.stream[i].pid = DTS_VIDEO_PID_START + i;
                    if (0x0 == mPrgInfo.pidPCR) {
                        mPrgInfo.pidPCR = mPmtInfo.stream[i].pid;
                    }
                    mPmtInfo.stream[i].descriptor_tag = DTS_DESCRIPTOR_TAG_VIDEO_START + i;
                    mPmtInfo.stream[i].descriptor_len = 0;//sizeof(mTSVideoInfo);//TODO: currently we fill no video descriptor
                    mPmtInfo.stream[i].pDescriptor = (TU8 *)&mTSVideoInfo;
                    mPmtInfo.stream[i].type = MPEG_SI_STREAM_TYPE_AVC_VIDEO;
                    /*mTSVideoInfo.format = infos->info[i].spec.video.;
                    mTSVideoInfo.frame_tick = infos->info[i].spec.video.;
                    mTSVideoInfo.time_scale = infos->info[i].spec.video.;
                    mTS VideoInfo.width = infos->info[i].spec.video.pic_width;
                    mTSVideoInfo.height = infos->info[i].spec.video.pic_height;
                    mTSVideoInfo.bitrate = infos->info[i].spec.video.bitrate;
                    mTSVideoInfo.framerate_num= infos->info[i].spec.video.framerate_num;
                    mTSVideoInfo.framerate_den= infos->info[i].spec.video.framerate_den;
                    mTSVideoInfo.idr_interval = infos->info[i].spec.video.IDRInterval;
                    LOGM_INFO("h264 param: width=%u, height=%u, bitrate=%u, framerate_num=%u, framerate_den=%u, idr_interval=%u\n",
                        infos->info[i].spec.video.pic_width, infos->info[i].spec.video.pic_height, infos->info[i].spec.video.bitrate, infos->info[i].spec.video.framerate_num, infos->info[i].spec.video.framerate_den, infos->info[i].spec.video.IDRInterval);*/
                    break;

                case StreamFormat_AAC:
                    mPmtInfo.stream[i].pid = DTS_AUDIO_PID_START + i;
                    if (0x0 == mPrgInfo.pidPCR) {
                        mPrgInfo.pidPCR = mPmtInfo.stream[i].pid;
                    }
                    mPmtInfo.stream[i].descriptor_tag = DTS_DESCRIPTOR_TAG_AUDIO_START + i;
                    mPmtInfo.stream[i].descriptor_len = 0;//sizeof(mTSAudioInfo);//TODO: currently we fill no AAC descriptor
                    mPmtInfo.stream[i].pDescriptor = (TU8 *)&mTSAudioInfo;
                    mPmtInfo.stream[i].type = MPEG_SI_STREAM_TYPE_AAC;
                    /*mTSAudioInfo.format = infos->info[i].spec.audio.codec_format;
                    mTSAudioInfo.sample_rate = infos->info[i].spec.audio.sample_rate;
                    mTSAudioInfo.channels = infos->info[i].spec.audio.channel_number;
                    mTSAudioInfo.sample_format = infos->info[i].spec.audio.sample_format;
                    mTSAudioInfo.chunk_size = infos->info[i].spec.audio.frame_size;
                    mTSAudioInfo.bitrate = infos->info[i].spec.audio.bitrate;
                    LOGM_ALWAYS("aac param: format=%u, sample_rate=%u, channels=%u, sample_format=%u, chunk_size=%u, bitrate=%u\n",
                        infos->info[i].spec.audio.codec_format,infos->info[i].spec.audio.sample_rate,infos->info[i].spec.audio.channel_number,
                        infos->info[i].spec.audio.sample_format,infos->info[i].spec.audio.frame_size, infos->info[i].spec.audio.bitrate)*/;
                    break;

                case StreamFormat_PCMA:
                case StreamFormat_PCMU:
                    mPmtInfo.stream[i].pid = DTS_AUDIO_PID_START + i;
                    if (0x0 == mPrgInfo.pidPCR) {
                        mPrgInfo.pidPCR = mPmtInfo.stream[i].pid;
                    }
                    mPmtInfo.stream[i].descriptor_tag = DTS_DESCRIPTOR_TAG_AUDIO_START + i;
                    mPmtInfo.stream[i].descriptor_len = sizeof(mTSAudioInfo);
                    mPmtInfo.stream[i].pDescriptor = (TU8 *)&mTSAudioInfo;
                    mPmtInfo.stream[i].type = MPEG_SI_STREAM_TYPE_LPCM_AUDIO;
                    break;

                default:
                    LOGM_FATAL("not supportted format %d\n", infos->info[i].stream_format);
                    return EECode_BadFormat;
                    break;
            }
        }
    }

    if (mbAddCustomizedPrivateData && infos->total_stream_number < EConstMaxDemuxerMuxerStreamNumber) {
        mPmtInfo.stream[infos->total_stream_number].pid = DTS_PRIDATA_PID_START;
        if (0x0 == mPrgInfo.pidPCR) {
            mPrgInfo.pidPCR = mPmtInfo.stream[infos->total_stream_number].pid;
        }
        mPmtInfo.stream[infos->total_stream_number].descriptor_tag = DTS_DESCRIPTOR_TAG_PRIVATE_START;
        mPmtInfo.stream[infos->total_stream_number].descriptor_len = 0;
        mPmtInfo.stream[infos->total_stream_number].pDescriptor = NULL;
        mPmtInfo.stream[infos->total_stream_number].type = MPEG_SI_STREAM_TYPE_PRIVATE_PES;
    }

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

