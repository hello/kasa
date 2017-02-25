/*******************************************************************************
 * ffmpeg_muxer.h
 *
 * History:
 *    2013/02/20 - [Zhi He] create file
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

#ifndef __FFMPEG_MUXER_H__
#define __FFMPEG_MUXER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CFFMpegMuxer
//
//-----------------------------------------------------------------------
class CFFMpegMuxer: public CObject, public IMuxer
{
    typedef CObject inherited;

public:
    static IMuxer *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();

protected:
    CFFMpegMuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    EECode Construct();
    virtual ~CFFMpegMuxer();

public:
    virtual void Delete();

    virtual EECode SetupContext();
    virtual EECode DestroyContext();

    virtual EECode SetSpecifiedInfo(SRecordSpecifiedInfo *info);
    virtual EECode GetSpecifiedInfo(SRecordSpecifiedInfo *info);

    virtual EECode SetExtraData(StreamType type, TUint stream_index, void *p_data, TUint data_size);
    virtual EECode SetPrivateDataDurationSeconds(void *p_data, TUint data_size) {return EECode_NoImplement;};
    virtual EECode SetPrivateDataChannelName(void *p_data, TUint data_size) {return EECode_NoImplement;};

    virtual EECode InitializeFile(const SStreamCodecInfos *infos, TChar *url, ContainerType type = ContainerType_AUTO, TChar *thumbnailname = NULL, TTime start_pts = 0, TTime start_dts = 0);
    virtual EECode FinalizeFile(SMuxerFileInfo *p_file_info);

    virtual EECode WriteVideoBuffer(CIBuffer *p_buffer, SMuxerDataTrunkInfo *info);
    virtual EECode WriteAudioBuffer(CIBuffer *p_buffer, SMuxerDataTrunkInfo *info);
    virtual EECode WritePridataBuffer(CIBuffer *p_buffer, SMuxerDataTrunkInfo *info);

    virtual EECode SetState(TU8 b_suspend);

    virtual void PrintStatus();

private:
    EECode setVideoParametersToMuxer(SStreamCodecInfo *video_param, TUint index);
    EECode setAudioParametersToMuxer(SStreamCodecInfo *audio_param, TUint index);
    EECode setSubtitleParametersToMuxer(SStreamCodecInfo *subtitle_param, TUint index);
    EECode setPrivateDataParametersToMuxer(SStreamCodecInfo *privatedata_param, TUint index);

    void clearFFMpegContext();
    //void getFileInformation();

    EECode setParameterToMuxer(const SStreamCodecInfos *infos, TUint i);
    EECode setParametersToMuxer(const SStreamCodecInfos *infos);

    void convertH264Data2annexb(TU8 *p_input, TUint size);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;

private:
    ContainerType mContainerType;
    TChar *mpUrlName;
    TChar *mpAllocatedUrlName;
    TChar *mpThumbNailFileName;
    TUint mnMaxStreamNumber;

    AVFormatContext *mpFormat;
    AVFormatParameters mAVFormatParam;
    AVOutputFormat *mpOutFormat;

    TU8 mH264DataFmt;
    TU8 mH264AVCCNaluLen;
    TU8 mbConvertH264DataFormat;
    TU8 mbSuspend;

    AVStream *mpStream[EConstMaxDemuxerMuxerStreamNumber];
    TU8        *mpExtraData[EConstMaxDemuxerMuxerStreamNumber];
    TUint       mExtraDataSize[EConstMaxDemuxerMuxerStreamNumber];

    TU8        *mpExtraDataOri[EConstMaxDemuxerMuxerStreamNumber];
    TUint       mExtraDataSizeOri[EConstMaxDemuxerMuxerStreamNumber];

    TU8 *mpConversionBuffer;
    TUint mConversionDataSize;
    TUint mConversionBufferSize;

    //for debug only
    //TTime   mFirstTimestamp[EConstMaxDemuxerMuxerStreamNumber];
    //TTime   mLastTimestamp[EConstMaxDemuxerMuxerStreamNumber];
    //TU64    mWriteSize[EConstMaxDemuxerMuxerStreamNumber];

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

    //    TU8 mbEnabledStream[EConstMaxDemuxerMuxerStreamNumber];

    TUint mnVideoFrameCount;
    TUint mnAudioFrameCount;
    TUint mnPrivdataFrameCount;
    TUint mnSubtitleFrameCount;

    //file information, for debug only
private:
    TU64 mEstimatedFileSize;
    TU64 mFileDuration;
    TUint mEstimatedBitrate;
    TUint mFileBitrate;

private:
    SMuxerInfo mInfo;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

