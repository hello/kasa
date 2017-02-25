/*******************************************************************************
 * mp4_demuxer.h
 *
 * History:
 *    2015/07/15 - [Zhi He] create file
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

#ifndef __MP4_DEMUXER_H__
#define __MP4_DEMUXER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

typedef struct {
    TTime base_time;
    TTime frame_tick;
    TU32 frame_start_index;
    TU32 frame_count;
} SMp4DTimeTree;

//-----------------------------------------------------------------------
//
// CMP4Demuxer
//
//-----------------------------------------------------------------------

#define DMAX_MEDIA_DATA_BOX_NUMBER 2

class CMP4Demuxer
    : public CActiveObject
    , public IDemuxer
    , public IEventListener
{
    typedef CActiveObject inherited;

public:
    static IDemuxer *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU32 index);
    virtual void Delete();

protected:
    CMP4Demuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU32 index);
    EECode Construct();
    virtual ~CMP4Demuxer();

public:
    virtual CObject *GetObject0() const;

public:
    virtual EECode SetupOutput(COutputPin *p_output_pins[], CBufferPool *p_bufferpools[], IMemPool *p_mempools[], IMsgSink *p_msg_sink);

    virtual EECode SetupContext(TChar *url, void *p_agent = NULL, TU8 priority = 0, TU32 request_receive_buffer_size = 0, TU32 request_send_buffer_size = 0);
    virtual EECode DestroyContext();
    virtual EECode ReconnectServer();

    virtual EECode Seek(TTime &target_time, ENavigationPosition position = ENavigationPosition_Invalid);
    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode Suspend();
    virtual EECode Pause();
    virtual EECode Resume();
    virtual EECode Flush();
    virtual EECode ResumeFlush();
    virtual EECode Purge();

    virtual EECode SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed);
    virtual EECode SetPbLoopMode(TU32 *p_loop_mode);

    virtual EECode EnableVideo(TU32 enable);
    virtual EECode EnableAudio(TU32 enable);

public:
    virtual EECode SetVideoPostProcessingCallback(void *callback_context, void *callback);
    virtual EECode SetAudioPostProcessingCallback(void *callback_context, void *callback);

public:
    virtual EECode QueryContentInfo(const SStreamCodecInfos *&pinfos) const;

public:
    virtual EECode UpdateContext(SContextInfo *pContext);
    virtual EECode GetExtraData(SStreamingSessionContent *pContent);

public:
    virtual EECode NavigationSeek(SContextInfo *info);
    virtual EECode ResumeChannel();

public:
    virtual void PrintStatus();

public:
    virtual void EventNotify(EEventType type, TU64 param1, TPointer param2);

public:
    virtual void OnRun();

private:
    TU32 readBaseBox(SISOMBox *p_box, TU8 *p);
    TU32 readFullBox(SISOMFullBox *p_box, TU8 *p);
    TU32 readFileTypeBox(SISOMFileTypeBox *p_box, TU8 *p);
    TU32 handleMediaDataBox(SISOMMediaDataBox *p_box, TU8 *p);
    TU32 readMovieHeaderBox(SISOMMovieHeaderBox *p_box, TU8 *p);
    TU32 readMovieTrackHeaderBox(SISOMTrackHeader *p_box, TU8 *p);
    TU32 readMediaHeaderBox(SISOMMediaHeaderBox *p_box, TU8 *p);
    TU32 readHanderReferenceBox(SISOMHandlerReferenceBox *p_box, TU8 *p);
    TU32 readChunkOffsetBox(SISOMChunkOffsetBox *p_box, TU8 *p);
    TU32 readSampleToChunkBox(SISOMSampleToChunkBox *p_box, TU8 *p);
    TU32 readSampleSizeBox(SISOMSampleSizeBox *p_box, TU8 *p);
    TU32 readAVCDecoderConfigurationRecordBox(SISOMAVCDecoderConfigurationRecordBox *p_box, TU8 *p);
    TU32 readHEVCDecoderConfigurationRecordBox(SISOMHEVCDecoderConfigurationRecordBox *p_box, TU8 *p);
    TU32 readVisualSampleDescriptionBox(SISOMVisualSampleDescriptionBox *p_box, TU8 *p);
    TU32 readAACElementaryStreamDescriptionBox(SISOMAACElementaryStreamDescriptor *p_box, TU8 *p);
    TU32 readAudioSampleDescriptionBox(SISOMAudioSampleDescriptionBox *p_box, TU8 *p);
    TU32 readDecodingTimeToSampleBox(SISOMDecodingTimeToSampleBox *p_box, TU8 *p);
    TU32 readVideoSampleTableBox(SISOMSampleTableBox *p_box, TU8 *p);
    TU32 readSoundSampleTableBox(SISOMSampleTableBox *p_box, TU8 *p);
    TU32 readDataReferenceBox(SISOMDataReferenceBox *p_box, TU8 *p);
    TU32 readDataInformationBox(SISOMDataInformationBox *p_box, TU8 *p);
    TU32 readVideoMediaHeaderBox(SISOMVideoMediaHeaderBox *p_box, TU8 *p);
    TU32 readSoundMediaHeaderBox(SISOMSoundMediaHeaderBox *p_box, TU8 *p);
    TU32 readVideoMediaInformationBox(SISOMMediaInformationBox *p_box, TU8 *p);
    TU32 readAudioMediaInformationBox(SISOMMediaInformationBox *p_box, TU8 *p);
    TU32 readVideoMediaBox(SISOMMediaBox *p_box, TU8 *p);
    TU32 readAudioMediaBox(SISOMMediaBox *p_box, TU8 *p);
    TU32 readMovieVideoTrackBox(SISOMTrackBox *p_box, TU8 *p);
    TU32 readMovieAudioTrackBox(SISOMTrackBox *p_box, TU8 *p);
    TU32 readMovieBox(SISOMMovieBox *p_box, TU8 *p);

private:
    TU32 readStrangeSize(TU8 *p);
    TU32 isFileHeaderValid();

    EECode parseFile();
    EECode getVideoOffsetSize(TU64 &offset, TU32 &size);
    EECode getAudioOffsetSize(TU64 &offset, TU32 &size);
    EECode queryVideoOffsetSize(TU64 &offset, TU32 &size);
    EECode queryAudioOffsetSize(TU64 &offset, TU32 &size);
    EECode copyFileData(TU8 *p_buf, TU64 offset, TU32 size);
    EECode readVideoFrame(TU8 *p_buf, TU32 &size);
    EECode readAudioFrame(TU8 *p_buf, TU32 &size);
    EECode nextPacketInfo(TU32 &size, StreamType &type, TTime &dts, TTime &pts);
    EECode nextPacketInfoTimeOrder(TU32 &size, StreamType &type, TTime &dts, TTime &pts);
    EECode nextVideoKeyFrameInfo(TU32 &size, TTime &dts, TTime &pts);
    EECode previousVideoKeyFrameInfo(TU32 &size, TTime &dts, TTime &pts);
    EECode updateVideoExtraData();
    EECode updateAudioExtraData();
    EECode sendExtraData();

private:
    EECode processCmd(SCMD &cmd);

private:
    EECode buildTimeTree();
    TU32 findVideoTimeIndex(TU32 index);
    EECode getVideoTimestamp(TU32 index, TTime &dts, TTime &pts);
    EECode getAudioTimestamp(TU32 index, TTime &dts, TTime &native_dts);
    EECode buildSampleOffset();

    TU32 timeToVideoFrameIndex(TTime dts);
    TU32 timeToAudioFrameIndex(TTime dts);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;

    COutputPin *mpOutputPins[EConstMaxDemuxerMuxerStreamNumber];
    CBufferPool *mpBufferPool[EConstMaxDemuxerMuxerStreamNumber];
    IMemPool *mpMemPool[EConstMaxDemuxerMuxerStreamNumber];

    COutputPin *mpCurOutputPin;
    IBufferPool *mpCurBufferPool;
    IMemPool *mpCurMemPool;

private:
    TU8 mbRun;
    TU8 mbPaused;
    TU8 mbTobeStopped;
    TU8 mbTobeSuspended;

    EModuleState msState;

private:
    SISOMSampleTableBox *mpVideoSampleTableBox;
    SISOMSampleTableBox *mpAudioSampleTableBox;
    TU32 mCurVideoFrameIndex;
    TU32 mCurAudioFrameIndex;

    TU32 mTotalVideoFrameCountMinus1;
    TU32 mTotalAudioFrameCountMinus1;

    SMp4DTimeTree *mpVideoDTimeTree;
    TU32 mnVideoDTimeTreeNodeCount;
    TU32 mnVideoDTimeTreeNodeBufferCount;

    SMp4DTimeTree *mpAudioDTimeTree;
    TU32 mnAudioDTimeTreeNodeCount;
    TU32 mnAudioDTimeTreeNodeBufferCount;

    TU32 *mpVideoCTimeIndexMap;
    TU32 mnVideoCTimeIndexMapCount;
    TU32 mnVideoCTimeIndexMapBufferCount;

    TU32 mLastVideoDTimeTreeNodeIndex;
    TU32 mLastAudioDTimeTreeNodeIndex;

    TU8 mbVideoOneSamplePerChunk;
    TU8 mbAudioOneSamplePerChunk;
    TU8 mbMultipleMediaDataBox;
    TU8 mnMediaDataBoxes;

    TU32 *mpVideoSampleOffset;
    TU32 mnVideoSampleOffsetBufferCount;
    TU32 *mpAudioSampleOffset;
    TU32 mnAudioSampleOffsetBufferCount;

    TU32 mVideoTimeScale;
    TU32 mAudioTimeScale;
    TU32 mVideoFrameTick;
    TU32 mAudioFrameTick;

private:
    SISOMFileTypeBox mFileTypeBox;
    SISOMMediaDataBox mMediaDataBox[DMAX_MEDIA_DATA_BOX_NUMBER];
    SISOMMovieBox mMovieBox;

private:
    StreamFormat mVideoFormat;
    StreamFormat mAudioFormat;

private:
    TU64 mCreationTime;
    TU64 mModificationTime;
    TU64 mTimeScale;
    TU64 mDuration;

private:
    TU8 mbVideoEnabled;
    TU8 mbAudioEnabled;
    TU8 mbHaveCTTS;
    TU8 mReserved0;

    TU8 mbHaveVideo;
    TU8 mbHaveAudio;
    TU8 mbVideoConstFrameRate;
    TU8 mbAudioConstFrameRate;

    TU8 mbBackward;
    TU8 mFeedingRule;
    TU8 mDebugFlag;
    TU8 mReserved2;

    TU32 mGuessedMinKeyFrameSize;

private:
    TU8 *mpVideoExtraData;
    TU32 mVideoExtraDataLength;
    TU32 mVideoExtraDataBufferLength;

private:
    TU16 mAudioChannelNumber;
    TU16 mAudioSampleSize;
    TU32 mAudioSampleRate;
    TU32 mAudioMaxBitrate;
    TU32 mAudioAvgBitrate;

    TU8 *mpAudioExtraData;
    TU32 mAudioExtraDataLength;
    TU32 mAudioExtraDataBufferLength;

private:
    TU8 *mpCacheBufferBase;
    TU8 *mpCacheBufferBaseAligned;
    TU8 *mpCacheBufferEnd;
    TU64 mCacheBufferSize;

    IIO *mpIO;
    TU64 mFileMovieBoxOffset;
    TU64 mCurFileOffset;
    TU64 mFileTotalSize;

private:
    TU8 *mpAudioTrackCachePtr;
    TU64 mAudioTrackCacheRemaingDataSize;
    TU64 mCurFileAudioOffset;
};
DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


