/*******************************************************************************
 * ts_muxer_v2.h
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

#ifndef __TS_MUXER_V2_H__
#define __TS_MUXER_V2_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CTSMuxer
//
//-----------------------------------------------------------------------
class CTSMuxer: virtual public CObject, virtual public IMuxer
{
    typedef CObject inherited;

public:
    static IMuxer *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();

protected:
    CTSMuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    EECode Construct();
    virtual ~CTSMuxer();

public:
    virtual void Delete();

    virtual EECode SetupContext();
    virtual EECode DestroyContext();

    virtual EECode SetSpecifiedInfo(SRecordSpecifiedInfo *info);
    virtual EECode GetSpecifiedInfo(SRecordSpecifiedInfo *info);

    virtual EECode SetExtraData(StreamType type, TUint stream_index, void *p_data, TUint data_size);
    virtual EECode SetPrivateDataDurationSeconds(void *p_data, TUint data_size);
    virtual EECode SetPrivateDataChannelName(void *p_data, TUint data_size);

    virtual EECode InitializeFile(const SStreamCodecInfos *infos, TChar *url, ContainerType type = ContainerType_AUTO, TChar *thumbnailname = NULL, TTime start_pts = 0, TTime start_dts = 0);
    virtual EECode FinalizeFile(SMuxerFileInfo *p_file_info);

    virtual EECode WriteVideoBuffer(CIBuffer *p_buffer, SMuxerDataTrunkInfo *info);
    virtual EECode WriteAudioBuffer(CIBuffer *p_buffer, SMuxerDataTrunkInfo *info);
    virtual EECode WritePridataBuffer(CIBuffer *p_buffer, SMuxerDataTrunkInfo *info);

    virtual EECode SetState(TU8 b_suspend);

    virtual void PrintStatus();

private:
    EECode createPATPacket(TU8 *pcurrent);
    EECode createPMTPacket(TU8 *pcurrent);

    void insertPATPacket(TU8 *pcurrent);
    void insertPMTPacket(TU8 *pcurrent);
    EECode insertCustomizedPrivateDataHeadPacket(TU8 *pcurrent);
    EECode insertCustomizedPrivateDataTailPacket();

    TFileSize insertVideoPacket(TU8 *p_dst, TU8 *psrc, TFileSize remaining_size, TU8 with_pcr, TU8 with_pts, TU8 with_dts, TU8 is_key_frame, TU8 stream_index, TU8 firstslice, TTime pts, TTime dts);
    TFileSize insertAudioPacket(TU8 *p_dst, TU8 *psrc, TFileSize remaining_size, TU8 with_pts, TU8 with_dts, TU8 firstslice, TTime pts, TTime dts);

    EECode writeToFile(TU8 *pstart, TFileSize size, TU8 sync = 1);

    EECode getMPEGPsiStreamInfo(const SStreamCodecInfos *infos);

private:
    void convertH264Data2annexb(TU8 *p_input, TUint size);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;

private:
    TU8 *mpCacheBufferBase;
    TU8 *mpCacheBufferBaseAligned;
    TU8 *mpCurrentPosInCache;
    TU32 mCacheBufferSize;
    TU32 mPatPmtInterval;

    TU32 mnTsPacketSize;
    TU32 mnMaxTsPacketNumber;
    TU32 mCurrentTsPacketIndex;

    IIO *mpIO;

private:
    TU8 mH264DataFmt;
    TU8 mH264AVCCNaluLen;
    TU8 mbConvertH264DataFormat;
    TU8 mbIOOpend;

    TChar *mpUrlName;
    TUint mnMaxStreamNumber;

    TU8    *mpExtraData[EConstMaxDemuxerMuxerStreamNumber];
    TUint   mExtraDataSize[EConstMaxDemuxerMuxerStreamNumber];

    TU8    *mpExtraDataOri[EConstMaxDemuxerMuxerStreamNumber];
    TUint   mExtraDataSizeOri[EConstMaxDemuxerMuxerStreamNumber];

    TU8 *mpConversionBuffer;
    TUint mConversionDataSize;
    TUint mConversionBufferSize;

private:
    //only check use
    TU8 mVideoIndex;
    TU8 mAudioIndex;
    TU8 mPrivDataIndex;
    TU8 mSubtitleIndex;

    TU8 mbVideoKeyFrameComes;
    TU8 mbAudioKeyFrameComes;
    TU8 mnTotalStreamNumber;
    TU8 mbNeedFindSPSPPSInBitstream;

    TUint mnVideoFrameCount;
    TUint mnAudioFrameCount;
    TUint mnPrivdataFrameCount;
    TUint mnSubtitleFrameCount;

    //file information, for debug only
private:
    TU64 mEstimatedFileSize;
    TU64 mFileDurationSeconds;
    TUint mEstimatedBitrate;
    TUint mFileBitrate;

private:
    STSMuxPsiPatInfo mPatInfo;
    STSMuxPsiPmtInfo mPmtInfo;
    STSMuxPsiPrgInfo mPrgInfo;

private:
    TU16 mnPatContinuityCounter;
    TU16 mnPmtContinuityCounter;
    TU16 mnVideoContinuityCounter;
    TU16 mnAudioContinuityCounter;
    TU16 mnCustomizedPrivateDataContinuityCounter;
    TU32 mPcrPatPmtFrameCount;

private:
    STSMuxPsiVersion mPSIVersion;
    STSVideoInfo mTSVideoInfo;
    STSAudioInfo mTSAudioInfo;
    TU8 mbAddCustomizedPrivateData;
    TU8 mChannelNameLength;
    TU8 mbSuspend;
    TU8 mReserved0;
    TChar mChannelName[DMaxChannelNameLength];
    TU32 mMaxGopInFile;
    TU32 mCurrentGopIndex;
    TU8 *mpGopIndexBaseUnaligned;
    TU32 *mpGopIndexBase;

private:
    TU8 mPatPacket[MPEG_TS_TP_PACKET_SIZE];
    TU8 mPmtPacket[MPEG_TS_TP_PACKET_SIZE];
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


