/*******************************************************************************
 * mp4_demuxer.cpp
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
#include "codec_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "iso_base_media_file_defs.h"

#include "mp4_demuxer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#define DIS_NOT_BOX_TAG(p, x) (DUnlikely(memcmp(p + 4, x, 4)))

#if 1
static void __check_demuxer_box_fail()
{
    static TU32 fail = 0;
    fail ++;
}

#define DCHECK_DEMUXER_BOX_BEGIN \
    TU8* p_check_base = p

#define DCHECK_DEMUXER_BOX_END \
    if ((p_check_base + p_box->base_box.size) != (p)) { \
        __check_demuxer_box_fail(); \
        LOGM_FATAL("box size check fail\n"); \
        return 0; \
    }

#define DCHECK_DEMUXER_FULL_BOX_END \
    if ((p_check_base + p_box->base_full_box.base_box.size) != (p)) { \
        __check_demuxer_box_fail(); \
        LOGM_FATAL("box size check fail\n"); \
        return 0; \
    }
#else
#define DCHECK_DEMUXER_BOX_BEGIN
#define DCHECK_DEMUXER_BOX_END
#define DCHECK_DEMUXER_FULL_BOX_END
#endif

static EECode __fast_get_handler_type(TU8 *p, TU8 *handler_type, TU32 tracks_remain_size)
{
    TU32 box_size = 0;
    TU32 remain_size = 0;

    DBER32(remain_size, p);
    if (DUnlikely(tracks_remain_size < remain_size)) {
        LOG_ERROR("size not expected, %d, %d\n", tracks_remain_size, remain_size);
        return EECode_ParseError;
    }

    if (DUnlikely(memcmp(p + 4, DISOM_TRACK_BOX_TAG, 4))) {
        LOG_ERROR("not track box\n");
        return EECode_ParseError;
    }
    remain_size -= 8;
    p += 8;

    DBER32(box_size, p);
    if ((box_size + 16) > remain_size) {
        LOG_ERROR("track header box size not expected, data corruption?\n");
        return EECode_ParseError;
    }
    if (DUnlikely(memcmp(p + 4, DISOM_TRACK_HEADER_BOX_TAG, 4))) {
        LOG_ERROR("not track header box\n");
        return EECode_ParseError;
    }
    remain_size -= box_size;
    p += box_size;

__till_media_box:

    DBER32(box_size, p);
    if (remain_size < box_size) {
        LOG_ERROR("track media box size not expected, data corruption?\n");
        return EECode_ParseError;
    }
    if (DUnlikely(memcmp(p + 4, DISOM_MEDIA_BOX_TAG, 4))) {
        LOG_NOTICE("skip till track media box\n");
        p += box_size;
        remain_size -= box_size;
        goto __till_media_box;
    }
    remain_size -= 8;
    p += 8;

    DBER32(box_size, p);
    if ((box_size + 16) > remain_size) {
        LOG_ERROR("track media header box size not expected, data corruption?\n");
        return EECode_ParseError;
    }
    if (DUnlikely(memcmp(p + 4, DISOM_MEDIA_HEADER_BOX_TAG, 4))) {
        LOG_ERROR("not track media header box\n");
        return EECode_ParseError;
    }
    remain_size -= box_size;
    p += box_size;

    DBER32(box_size, p);
    if ((box_size + 16) > remain_size) {
        LOG_ERROR("track handler ref box size not expected, data corruption?\n");
        return EECode_ParseError;
    }
    if (DUnlikely(memcmp(p + 4, DISOM_HANDLER_REFERENCE_BOX_TAG, 4))) {
        LOG_ERROR("not track handler ref box\n");
        return EECode_ParseError;
    }

    memcpy(handler_type, p + 16, 4);
    return EECode_OK;
}

static EECode __release_ring_buffer(CIBuffer *pBuffer)
{
    DASSERT(pBuffer);
    if (pBuffer) {
        if (EBufferCustomizedContentType_RingBuffer == pBuffer->mCustomizedContentType) {
            IMemPool *pool = (IMemPool *)pBuffer->mpCustomizedContent;
            DASSERT(pool);
            if (pool) {
                pool->ReleaseMemBlock((TULong)(pBuffer->GetDataMemorySize()), pBuffer->GetDataPtrBase());
            }
        }
    } else {
        LOG_FATAL("NULL pBuffer!\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

void __convert_to_annexb(TU8 *p_buf, TU32 size)
{
    TU32 cur_size = 0;
    TU8 *p = p_buf, * p_next = NULL;

    if ((!p[0]) && (!p[1]) && ((0x01 == p[2]) || ((!p[2]) && (0x01 == p[3])))) {
        return;
    }

    while (size) {
        DBER32(cur_size, p);
        if ((cur_size + 4) > size) {
            LOG_ERROR("data corruption\n");
            break;
        }
        p_next = p + cur_size;
        size -= cur_size + 4;
        p[0] = 0x0;
        p[1] = 0x0;
        p[2] = 0x0;
        p[3] = 0x1;
        p = p_next;
    }

    return;
}

IDemuxer *gfCreateMP4DemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CMP4Demuxer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

IDemuxer *CMP4Demuxer::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU32 index)
{
    CMP4Demuxer *result = new CMP4Demuxer(pname, pPersistMediaConfig, pMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CMP4Demuxer::GetObject0() const
{
    return (CObject *) this;
}

CMP4Demuxer::CMP4Demuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU32 index)
    : inherited(pname, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(NULL)
    , mpCurOutputPin(NULL)
    , mpCurBufferPool(NULL)
    , mpCurMemPool(NULL)
    , mbRun(1)
    , mbPaused(0)
    , mbTobeStopped(0)
    , mbTobeSuspended(0)
    , msState(EModuleState_Invalid)
    , mpVideoSampleTableBox(NULL)
    , mpAudioSampleTableBox(NULL)
    , mCurVideoFrameIndex(0)
    , mCurAudioFrameIndex(0)
    , mTotalVideoFrameCountMinus1(0)
    , mTotalAudioFrameCountMinus1(0)
    , mbVideoOneSamplePerChunk(0)
    , mbAudioOneSamplePerChunk(0)
    , mVideoFormat(StreamFormat_H264)
    , mAudioFormat(StreamFormat_AAC)
    , mCreationTime(0)
    , mModificationTime(0)
    , mTimeScale(DDefaultTimeScale)
    , mDuration(0)
    , mbVideoEnabled(0)
    , mbAudioEnabled(0)
    , mbHaveCTTS(0)
    , mbHaveVideo(0)
    , mbHaveAudio(0)
    , mbVideoConstFrameRate(1)
    , mbAudioConstFrameRate(1)
{
    mpVideoDTimeTree = NULL;
    mnVideoDTimeTreeNodeCount = 0;
    mnVideoDTimeTreeNodeBufferCount = 0;

    mpAudioDTimeTree = NULL;
    mnAudioDTimeTreeNodeCount = 0;
    mnAudioDTimeTreeNodeBufferCount = 0;

    mpVideoCTimeIndexMap = NULL;
    mnVideoCTimeIndexMapCount = 0;
    mnVideoCTimeIndexMapBufferCount = 0;

    mbMultipleMediaDataBox = 0;
    mnMediaDataBoxes = 0;

    mpVideoSampleOffset = NULL;
    mnVideoSampleOffsetBufferCount = 0;
    mpAudioSampleOffset = NULL;
    mnAudioSampleOffsetBufferCount = 0;

    mVideoTimeScale = 0;
    mAudioTimeScale = 0;
    mVideoFrameTick = 0;
    mAudioFrameTick = 0;

    mpVideoExtraData = NULL;
    mVideoExtraDataLength = 0;
    mVideoExtraDataBufferLength = 0;

    mAudioChannelNumber = DDefaultAudioChannelNumber;
    mAudioSampleSize = 16;
    mAudioSampleRate = DDefaultAudioSampleRate;
    mAudioMaxBitrate = 64000;
    mAudioAvgBitrate = 64000;

    mpAudioExtraData = NULL;
    mAudioExtraDataLength = 0;
    mAudioExtraDataBufferLength = 0;

    mpCacheBufferBase = NULL;
    mCacheBufferSize = 0;

    mpIO = NULL;
    mCurFileOffset = 0;
    mFileTotalSize = 0;

    mpAudioTrackCachePtr = NULL;
    mAudioTrackCacheRemaingDataSize = 0;
    mCurFileAudioOffset = 0;

    mbBackward = 0;
    mFeedingRule = DecoderFeedingRule_AllFrames;

    mGuessedMinKeyFrameSize = 256;

    memset(&mFileTypeBox, 0x0, sizeof(mFileTypeBox));
    memset(&mMediaDataBox, 0x0, sizeof(mMediaDataBox));
    memset(&mMovieBox, 0x0, sizeof(mMovieBox));

    for (TUint i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        mpOutputPins[i] = NULL;
        mpBufferPool[i] = NULL;
        mpMemPool[i] = NULL;
    }

}

EECode CMP4Demuxer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleNativeMP4Demuxer);

    mConfigLogLevel = ELogLevel_Info;

    mCacheBufferSize = 2 * 1024 * 1024;
    mpCacheBufferBase = (TU8 *) DDBG_MALLOC(mCacheBufferSize + 31, "M4CH");
    mpCacheBufferBaseAligned = (TU8 *)((((TULong)mpCacheBufferBase) + 31) & (~0x1f));

    if (!mpCacheBufferBase) {
        LOGM_FATAL("No Memory! request size %llu\n", mCacheBufferSize + 31);
        return EECode_NoMemory;
    }

    return inherited::Construct();
}

void CMP4Demuxer::Delete()
{
    if (mpCacheBufferBase) {
        DDBG_FREE(mpCacheBufferBase, "M4CH");
        mpCacheBufferBase = NULL;
    }
    mCacheBufferSize = 0;

    if (mpIO) {
        mpIO->Delete();
        mpIO = NULL;
    }

    if (mpVideoExtraData) {
        DDBG_FREE(mpVideoExtraData, "M4VE");
        mpVideoExtraData = NULL;
    }

    if (mpAudioExtraData) {
        DDBG_FREE(mpAudioExtraData, "M4AE");
        mpAudioExtraData = NULL;
    }

    if (mMovieBox.video_track.media.media_info.sample_table.chunk_offset.chunk_offset) {
        DDBG_FREE(mMovieBox.video_track.media.media_info.sample_table.chunk_offset.chunk_offset, "M4VC");
        mMovieBox.video_track.media.media_info.sample_table.chunk_offset.chunk_offset = NULL;
        mMovieBox.video_track.media.media_info.sample_table.chunk_offset.entry_buf_count = 0;
        mMovieBox.video_track.media.media_info.sample_table.chunk_offset.entry_count = 0;
    }

    if (mMovieBox.video_track.media.media_info.sample_table.sample_size.entry_size) {
        DDBG_FREE(mMovieBox.video_track.media.media_info.sample_table.sample_size.entry_size, "M4VS");
        mMovieBox.video_track.media.media_info.sample_table.sample_size.entry_size = NULL;
        mMovieBox.video_track.media.media_info.sample_table.sample_size.entry_buf_count = 0;
        mMovieBox.video_track.media.media_info.sample_table.sample_size.sample_count = 0;
    }

    if (mMovieBox.video_track.media.media_info.sample_table.ctts.p_entry) {
        DDBG_FREE(mMovieBox.video_track.media.media_info.sample_table.ctts.p_entry, "M4VT");
        mMovieBox.video_track.media.media_info.sample_table.ctts.p_entry = NULL;
        mMovieBox.video_track.media.media_info.sample_table.ctts.entry_count = 0;
    }

    if (mMovieBox.video_track.media.media_info.sample_table.stts.p_entry) {
        DDBG_FREE(mMovieBox.video_track.media.media_info.sample_table.stts.p_entry, "M4VS");
        mMovieBox.video_track.media.media_info.sample_table.stts.p_entry = NULL;
        mMovieBox.video_track.media.media_info.sample_table.stts.entry_count = 0;
    }

    if (mMovieBox.video_track.media.media_info.sample_table.sample_to_chunk.entrys) {
        DDBG_FREE(mMovieBox.video_track.media.media_info.sample_table.sample_to_chunk.entrys, "M4VE");
        mMovieBox.video_track.media.media_info.sample_table.sample_to_chunk.entrys = NULL;
        mMovieBox.video_track.media.media_info.sample_table.sample_to_chunk.entry_count = 0;
    }

    if (mMovieBox.audio_track.media.media_info.sample_table.chunk_offset.chunk_offset) {
        DDBG_FREE(mMovieBox.audio_track.media.media_info.sample_table.chunk_offset.chunk_offset, "M4AC");
        mMovieBox.audio_track.media.media_info.sample_table.chunk_offset.chunk_offset = NULL;
        mMovieBox.audio_track.media.media_info.sample_table.chunk_offset.entry_buf_count = 0;
        mMovieBox.audio_track.media.media_info.sample_table.chunk_offset.entry_count = 0;
    }

    if (mMovieBox.audio_track.media.media_info.sample_table.sample_size.entry_size) {
        DDBG_FREE(mMovieBox.audio_track.media.media_info.sample_table.sample_size.entry_size, "M4AS");
        mMovieBox.audio_track.media.media_info.sample_table.sample_size.entry_size = NULL;
        mMovieBox.audio_track.media.media_info.sample_table.sample_size.entry_buf_count = 0;
        mMovieBox.audio_track.media.media_info.sample_table.sample_size.sample_count = 0;
    }

    if (mMovieBox.audio_track.media.media_info.sample_table.ctts.p_entry) {
        DDBG_FREE(mMovieBox.audio_track.media.media_info.sample_table.ctts.p_entry, "M4AT");
        mMovieBox.audio_track.media.media_info.sample_table.ctts.p_entry = NULL;
        mMovieBox.audio_track.media.media_info.sample_table.ctts.entry_count = 0;
    }

    if (mMovieBox.audio_track.media.media_info.sample_table.stts.p_entry) {
        DDBG_FREE(mMovieBox.audio_track.media.media_info.sample_table.stts.p_entry, "M4AS");
        mMovieBox.audio_track.media.media_info.sample_table.stts.p_entry = NULL;
        mMovieBox.audio_track.media.media_info.sample_table.stts.entry_count = 0;
    }

    if (mMovieBox.audio_track.media.media_info.sample_table.sample_to_chunk.entrys) {
        DDBG_FREE(mMovieBox.audio_track.media.media_info.sample_table.sample_to_chunk.entrys, "M4AE");
        mMovieBox.audio_track.media.media_info.sample_table.sample_to_chunk.entrys = NULL;
        mMovieBox.audio_track.media.media_info.sample_table.sample_to_chunk.entry_count = 0;
    }

    if (mpVideoDTimeTree) {
        DDBG_FREE(mpVideoDTimeTree, "M4VD");
        mpVideoDTimeTree = NULL;
    }

    if (mpAudioDTimeTree) {
        DDBG_FREE(mpAudioDTimeTree, "M4AD");
        mpAudioDTimeTree = NULL;
    }

    if (mpVideoCTimeIndexMap) {
        DDBG_FREE(mpVideoCTimeIndexMap, "M4VC");
        mpVideoCTimeIndexMap = NULL;
    }

    if (mpVideoSampleOffset) {
        DDBG_FREE(mpVideoSampleOffset, "M4VS");
        mpVideoSampleOffset = NULL;
    }

    if (mpAudioSampleOffset) {
        DDBG_FREE(mpAudioSampleOffset, "M4AS");
        mpAudioSampleOffset = NULL;
    }

    inherited::Delete();
}

CMP4Demuxer::~CMP4Demuxer()
{

}

EECode CMP4Demuxer::SetupOutput(COutputPin *p_output_pins [ ], CBufferPool *p_bufferpools [ ],  IMemPool *p_mempools[], IMsgSink *p_msg_sink)
{
    DASSERT(!mpBufferPool[EDemuxerVideoOutputPinIndex]);
    DASSERT(!mpBufferPool[EDemuxerAudioOutputPinIndex]);
    DASSERT(!mpOutputPins[EDemuxerVideoOutputPinIndex]);
    DASSERT(!mpOutputPins[EDemuxerAudioOutputPinIndex]);
    DASSERT(!mpMsgSink);

    mpOutputPins[EDemuxerVideoOutputPinIndex] = (COutputPin *)p_output_pins[EDemuxerVideoOutputPinIndex];
    mpOutputPins[EDemuxerAudioOutputPinIndex] = (COutputPin *)p_output_pins[EDemuxerAudioOutputPinIndex];
    mpBufferPool[EDemuxerVideoOutputPinIndex] = (CBufferPool *)p_bufferpools[EDemuxerVideoOutputPinIndex];
    mpBufferPool[EDemuxerAudioOutputPinIndex] = (CBufferPool *)p_bufferpools[EDemuxerAudioOutputPinIndex];

    mpMsgSink = p_msg_sink;

    if (mpOutputPins[EDemuxerVideoOutputPinIndex] && mpBufferPool[EDemuxerVideoOutputPinIndex]) {
        mpBufferPool[EDemuxerVideoOutputPinIndex]->SetReleaseBufferCallBack(__release_ring_buffer);
        mpBufferPool[EDemuxerVideoOutputPinIndex]->AddBufferNotifyListener((IEventListener *)this);
        mpMemPool[EDemuxerVideoOutputPinIndex] = CRingMemPool::Create(2 * 1024 * 1024);
    }

    if (mpOutputPins[EDemuxerAudioOutputPinIndex] && mpBufferPool[EDemuxerAudioOutputPinIndex]) {
        mpBufferPool[EDemuxerAudioOutputPinIndex]->SetReleaseBufferCallBack(__release_ring_buffer);
        mpBufferPool[EDemuxerAudioOutputPinIndex]->AddBufferNotifyListener((IEventListener *)this);
        mpMemPool[EDemuxerAudioOutputPinIndex] = CRingMemPool::Create(128 * 1024);
    }

    return EECode_OK;
}

EECode CMP4Demuxer::SetupContext(TChar *url, void *p_agent, TU8 priority, TU32 request_receive_buffer_size, TU32 request_send_buffer_size)
{
    EECode err;
    TIOSize total_size = 0, cur_pos = 0;

    if (DUnlikely(!url)) {
        LOGM_ERROR("NULL url\n");
        return EECode_BadParam;
    }

    if (DLikely(!mpIO)) {
        mpIO = gfCreateIO(EIOType_File);
    } else {
        LOGM_WARN("mpIO is created yet?\n");
    }

    err = mpIO->Open(url, EIOFlagBit_Read | EIOFlagBit_Binary);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("open url (%s) fail, ret %d, %s\n", url, err, gfGetErrorCodeString(err));
        return err;
    }

    err = mpIO->Query(total_size, cur_pos);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("query (%s) fail, ret %d, %s\n", url, err, gfGetErrorCodeString(err));
        return err;
    }
    mFileTotalSize = total_size;
    DASSERT(0 == cur_pos);

    err = parseFile();
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("parseFile (%s) fail, ret %d, %s\n", url, err, gfGetErrorCodeString(err));
        return err;
    }

    if (mpWorkQ) {
        LOGM_INFO("before SendCmd(ECMDType_StartRunning)...\n");
        err = mpWorkQ->SendCmd(ECMDType_StartRunning);
        LOGM_INFO("after SendCmd(ECMDType_StartRunning)\n");
        DASSERT_OK(err);
    } else {
        LOGM_FATAL("NULL mpWorkQ?\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CMP4Demuxer::DestroyContext()
{
    if (mpWorkQ) {
        //LOGM_INFO("before SendCmd(ECMDType_ExitRunning)...\n");
        mpWorkQ->SendCmd(ECMDType_ExitRunning);
        //LOGM_INFO("after SendCmd(ECMDType_ExitRunning)\n");
    }

    if (DLikely(mpIO)) {
        mpIO->Delete();
        mpIO = NULL;
    } else {
        LOGM_WARN("mpIO is not created?\n");
    }

    return EECode_OK;
}

EECode CMP4Demuxer::ReconnectServer()
{
    return EECode_OK;
}

EECode CMP4Demuxer::Seek(TTime &target_time, ENavigationPosition position)
{
    SInternalCMDSeek seek;
    SCMD cmd;
    EECode err;

    seek.target = target_time;
    seek.position = position;

    cmd.code = ECMDType_Seek;
    cmd.pExtra = (void *) &seek;

    err = mpWorkQ->SendCmd(cmd);

    target_time = seek.target;

    return err;
}

EECode CMP4Demuxer::Start()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    LOGM_INFO("before SendCmd(ECMDType_Start)...\n");
    err = mpWorkQ->SendCmd(ECMDType_Start);
    LOGM_INFO("after SendCmd(ECMDType_Start)\n");

    DASSERT_OK(err);

    return err;
}

EECode CMP4Demuxer::Stop()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    LOGM_INFO("before SendCmd(ECMDType_Stop)...\n");
    err = mpWorkQ->SendCmd(ECMDType_Stop);
    LOGM_INFO("after SendCmd(ECMDType_Stop)\n");

    DASSERT_OK(err);

    return err;
}

EECode CMP4Demuxer::Suspend()
{
    return EECode_OK;
}

EECode CMP4Demuxer::Pause()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    err = mpWorkQ->PostMsg(ECMDType_Pause);

    DASSERT_OK(err);

    return err;
}

EECode CMP4Demuxer::Resume()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    err = mpWorkQ->PostMsg(ECMDType_Resume);

    DASSERT_OK(err);

    return err;
}

EECode CMP4Demuxer::Flush()
{
    return mpWorkQ->SendCmd(ECMDType_Flush);
}

EECode CMP4Demuxer::ResumeFlush()
{
    return mpWorkQ->SendCmd(ECMDType_ResumeFlush);
}

EECode CMP4Demuxer::Purge()
{
    return EECode_OK;
}

EECode CMP4Demuxer::SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed)
{
    EECode err = EECode_OK;
    SCMD cmd;

    if (DLikely(mpWorkQ)) {
        cmd.code = ECMDType_UpdatePlaybackSpeed;
        cmd.pExtra = NULL;
        cmd.res32_1 = direction;
        cmd.res64_1 = feeding_rule;
        err = mpWorkQ->PostMsg(cmd);
    } else {
        LOGM_FATAL("NULL mpWorkQ or NULL p_feedingrule\n");
        return EECode_BadState;
    }

    return err;
}

EECode CMP4Demuxer::SetPbLoopMode(TU32 *p_loop_mode)
{
    EECode err = EECode_OK;
    SCMD cmd;

    if (DLikely(mpWorkQ && p_loop_mode)) {
        cmd.code = ECMDType_UpdatePlaybackLoopMode;
        cmd.pExtra = NULL;
        cmd.res32_1 = *p_loop_mode;
        err = mpWorkQ->PostMsg(cmd);
    } else {
        LOGM_FATAL("NULL mpWorkQ or NULL p_loop_mode\n");
        return EECode_BadState;
    }

    return err;
}

EECode CMP4Demuxer::EnableVideo(TU32 enable)
{
    SCMD cmd;
    cmd.code = ECMDType_EnableVideo;
    cmd.res32_1 = enable;
    cmd.pExtra = NULL;

    mpWorkQ->PostMsg(cmd);

    return EECode_OK;
}

EECode CMP4Demuxer::EnableAudio(TU32 enable)
{
    SCMD cmd;
    cmd.code = ECMDType_EnableAudio;
    cmd.res32_1 = enable;
    cmd.pExtra = NULL;

    mpWorkQ->PostMsg(cmd);

    return EECode_OK;
}

EECode CMP4Demuxer::SetVideoPostProcessingCallback(void *callback_context, void *callback)
{
    return EECode_OK;
}

EECode CMP4Demuxer::SetAudioPostProcessingCallback(void *callback_context, void *callback)
{
    return EECode_OK;
}

void CMP4Demuxer::PrintStatus()
{
    LOGM_PRINTF("heart beat %d, msState %s, mDebugFlag %d\n", mDebugHeartBeat, gfGetModuleStateString(msState), mDebugFlag);
    if (mpMemPool[EDemuxerVideoOutputPinIndex]) {
        mpMemPool[EDemuxerVideoOutputPinIndex]->GetObject0()->PrintStatus();
    }
    if (mpMemPool[EDemuxerAudioOutputPinIndex]) {
        mpMemPool[EDemuxerAudioOutputPinIndex]->GetObject0()->PrintStatus();
    }
    mDebugHeartBeat = 0;
}

void CMP4Demuxer::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    EECode err = EECode_OK;
    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease)...\n");
    err = mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease, NULL);
    LOGM_FLOW("after mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease), ret %d\n", err);

    DASSERT_OK(err);

    return;
}

EECode CMP4Demuxer::QueryContentInfo(const SStreamCodecInfos *&pinfos) const
{
    return EECode_OK;
}

EECode CMP4Demuxer::UpdateContext(SContextInfo *pContext)
{
    return EECode_OK;
}

EECode CMP4Demuxer::GetExtraData(SStreamingSessionContent *pContent)
{
    EECode err;


    if (DUnlikely(!mpAudioExtraData)) {
        pContent->has_audio = 0;
        pContent->sub_session_content[ESubSession_audio]->enabled = 0;
    } else {
        err = gfSDPProcessAudioExtraData(pContent->sub_session_content[ESubSession_audio], mpAudioExtraData, mAudioExtraDataLength);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_FATAL("process audio extradata fail!\n");
        } else {
            pContent->sub_session_content[ESubSession_audio]->enabled = 1;
            pContent->sub_session_content[ESubSession_audio]->format = mAudioFormat;
            pContent->sub_session_content[ESubSession_audio]->audio_channel_number = 1;
            pContent->sub_session_content[ESubSession_audio]->audio_frame_size = 1024;
            pContent->sub_session_content[ESubSession_audio]->audio_sample_format = AudioSampleFMT_S16;
            pContent->sub_session_content[ESubSession_audio]->audio_sample_rate = 48000;
        }
    }

    if (DUnlikely(!mpVideoExtraData)) {
        pContent->has_video = 0;
        pContent->sub_session_content[ESubSession_video]->enabled = 0;
    } else {
        err = gfSDPProcessVideoExtraData(pContent->sub_session_content[ESubSession_video], mpVideoExtraData, mVideoExtraDataLength);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_FATAL("process video extradata fail!\n");
        } else {
            pContent->sub_session_content[ESubSession_video]->enabled = 1;
            pContent->sub_session_content[ESubSession_video]->format = mVideoFormat;
            pContent->sub_session_content[ESubSession_video]->video_framerate_den = 3003;
            pContent->sub_session_content[ESubSession_video]->video_framerate_num = 90000;
        }
    }

    return EECode_OK;
}

EECode CMP4Demuxer::NavigationSeek(SContextInfo *info)
{
    return EECode_OK;
}

EECode CMP4Demuxer::ResumeChannel()
{
    return EECode_OK;
}

void CMP4Demuxer::OnRun()
{
    SCMD cmd;
    CIBuffer *pBuffer = NULL;
    EECode err = EECode_OK;
    msState = EModuleState_WaitCmd;
    TU32 datasize = 0;
    TU8 *p;
    TTime dts = 0, pts = 0;
    StreamType type;

    mbRun = 1;

    CmdAck(EECode_OK);

    updateVideoExtraData();
    updateAudioExtraData();

    sendExtraData();

    while (mbRun) {

        LOGM_STATE("state(%s, %d)\n", gfGetModuleStateString(msState), msState);
        switch (msState) {

            case EModuleState_WaitCmd:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Idle:
                if (mpWorkQ->PeekCmd(cmd)) {
                    processCmd(cmd);
                } else {
                    if (DLikely((!mbBackward) && (DecoderFeedingRule_AllFrames == mFeedingRule))) {
                        if (mbMultipleMediaDataBox) {
                            err = nextPacketInfoTimeOrder(datasize, type, dts, pts);
                        } else {
                            err = nextPacketInfo(datasize, type, dts, pts);
                        }
                    } else if (DecoderFeedingRule_IDROnly == mFeedingRule) {
                        if (!mbBackward) {
                            err = nextVideoKeyFrameInfo(datasize, dts, pts);
                        } else {
                            err = previousVideoKeyFrameInfo(datasize, dts, pts);
                        }
                        if (EECode_OK == err) {
                            LOGM_DEBUG("key frame index %d, dts %lld\n", mCurVideoFrameIndex, dts);
                            type = StreamType_Video;
                        } else {
                            type = StreamType_Invalid;
                        }
                    } else {
                        msState = EModuleState_Error;
                        LOGM_ERROR("not support feeding rule %d\n", mFeedingRule);
                        break;
                    }

                    if (DUnlikely(EECode_OK_EOF == err)) {
                        msState = EModuleState_Pending;
                        LOGM_NOTICE("eof comes\n");
                        break;
                    } else if (DUnlikely(EECode_OK != err)) {
                        msState = EModuleState_Error;
                        LOGM_ERROR("parse fail\n");
                        break;
                    }

                    if (StreamType_Video == type) {
                        mpCurOutputPin = mpOutputPins[EDemuxerVideoOutputPinIndex];
                        mpCurBufferPool = mpBufferPool[EDemuxerVideoOutputPinIndex];
                        mpCurMemPool = mpMemPool[EDemuxerVideoOutputPinIndex];
                    } else if (StreamType_Audio == type) {
                        mpCurOutputPin = mpOutputPins[EDemuxerAudioOutputPinIndex];
                        mpCurBufferPool = mpBufferPool[EDemuxerAudioOutputPinIndex];
                        mpCurMemPool = mpMemPool[EDemuxerAudioOutputPinIndex];
                    } else {
                        LOGM_FATAL("bad type %d? must have internal bugs.\n", type);
                        msState = EModuleState_Error;
                        break;
                    }
                    msState = EModuleState_Running;
                }
                break;

            case EModuleState_HasInputData:
                DASSERT(mpCurBufferPool);
                if (mpCurBufferPool->GetFreeBufferCnt() > 0) {
                    msState = EModuleState_Running;
                } else {
                    mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                }
                break;

            case EModuleState_Running:
                DASSERT(!pBuffer);
                pBuffer = NULL;
                mDebugFlag = 1;
                if (!mpCurOutputPin->AllocBuffer(pBuffer)) {
                    LOGM_FATAL("AllocBuffer() fail? must have internal bugs.\n");
                    msState = EModuleState_Error;
                    mDebugFlag = 3;
                    break;
                }

                mDebugFlag = 2;
                p = mpCurMemPool->RequestMemBlock((TULong)datasize);

                if (StreamType_Video == type) {

                    err = readVideoFrame(p, datasize);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_FATAL("readVideoFrame() fail? must have internal bugs.\n");
                        msState = EModuleState_Error;
                        mDebugFlag = 3;
                        break;
                    }

                    pBuffer->SetBufferType(EBufferType_VideoES);
                    pBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                    pBuffer->mpCustomizedContent = (void *)mpCurMemPool;

                    if (DUnlikely(ENalType_IDR == (p[4] & 0x1f))) {
                        pBuffer->SetBufferFlagBits(CIBuffer::KEY_FRAME);
                        pBuffer->mVideoFrameType = EPredefinedPictureType_IDR;
                    } else if (ENalType_SPS == (p[4] & 0x1f)) {
                        pBuffer->SetBufferFlagBits(CIBuffer::KEY_FRAME);
                        pBuffer->mVideoFrameType = EPredefinedPictureType_IDR;
                    } else {
                        if ((DecoderFeedingRule_AllFrames != mFeedingRule) && (DecoderFeedingRule_RefOnly != mFeedingRule)) {
                            LOGM_NOTICE("skip non key frame\n");
                            pBuffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
                            pBuffer->mpCustomizedContent = (void *) NULL;
                            pBuffer->Release();
                            pBuffer = NULL;
                            mpCurMemPool->ReturnBackMemBlock((TULong)datasize, p);
                            mDebugFlag = 0;
                            msState = EModuleState_Idle;
                            break;
                        }
                        pBuffer->SetBufferFlags(0);
                        pBuffer->mVideoFrameType = EPredefinedPictureType_P;
                    }

                    pBuffer->SetDataPtr(p);
                    pBuffer->SetDataSize(datasize);
                    pBuffer->SetDataPtrBase(p);
                    pBuffer->SetDataMemorySize(datasize);

                    pBuffer->SetBufferLinearDTS(dts);
                    pBuffer->SetBufferLinearPTS(pts);
                    pBuffer->SetBufferNativeDTS(dts);
                    pBuffer->SetBufferNativePTS(pts);

                    mpCurOutputPin->SendBuffer(pBuffer);
                } else if (StreamType_Audio == type) {

                    err = readAudioFrame(p, datasize);
                    if (DUnlikely(EECode_OK != err)) {
                        LOGM_FATAL("readAudioFrame() fail? must have internal bugs.\n");
                        msState = EModuleState_Error;
                        mDebugFlag = 3;
                        break;
                    }

                    pBuffer->SetBufferType(EBufferType_AudioES);
                    pBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                    pBuffer->mpCustomizedContent = (void *)mpCurMemPool;

                    pBuffer->SetDataPtr(p);
                    pBuffer->SetDataSize(datasize);
                    pBuffer->SetDataPtrBase(p);
                    pBuffer->SetDataMemorySize(datasize);

                    pBuffer->SetBufferLinearDTS(dts);
                    pBuffer->SetBufferLinearPTS(dts);
                    pBuffer->SetBufferNativeDTS(pts);
                    pBuffer->SetBufferNativePTS(pts);

                    mpCurOutputPin->SendBuffer(pBuffer);
                }

                pBuffer = NULL;
                mpCurOutputPin = NULL;
                mpCurBufferPool = NULL;
                mpCurMemPool = NULL;
                mDebugFlag = 0;
                msState = EModuleState_Idle;
                break;

            case EModuleState_Completed:
            case EModuleState_Pending:
                pBuffer = NULL;
                datasize = 0;
            case EModuleState_Error:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            default:
                LOGM_ERROR("Check Me.\n");
                break;
        }
    }

    LOGM_INFO("MP4Demuxer OnRun Exit.\n");
    CmdAck(EECode_OK);
}

TU32 CMP4Demuxer::readBaseBox(SISOMBox *p_box, TU8 *p)
{
    DBER32(p_box->size, p);
    memcpy(p_box->type, p + 4, DISOM_TAG_SIZE);

    return DISOM_BOX_SIZE;
}

TU32 CMP4Demuxer::readFullBox(SISOMFullBox *p_box, TU8 *p)
{
    DBER32(p_box->base_box.size, p);
    memcpy(p_box->base_box.type, p + 4, DISOM_TAG_SIZE);

    p_box->flags = p[8];
    p += 9;
    DBER24(p_box->flags, p);

    return DISOM_FULL_BOX_SIZE;
}

TU32 CMP4Demuxer::readFileTypeBox(SISOMFileTypeBox *p_box, TU8 *p)
{
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readBaseBox(&p_box->base_box, p);

    memcpy(p_box->major_brand, p, DISOM_TAG_SIZE);
    p += DISOM_TAG_SIZE;

    DBER32(p_box->minor_version, p);
    p += 4;

    p_box->compatible_brands_number = (p_box->base_box.size - DISOM_BOX_SIZE - DISOM_TAG_SIZE - 4) >> 2;
    if (DISOM_MAX_COMPATIBLE_BRANDS >= p_box->compatible_brands_number) {
        for (TU32 i = 0; i < p_box->compatible_brands_number; i ++) {
            memcpy(p_box->compatible_brands[i], p, DISOM_TAG_SIZE);
            p += DISOM_TAG_SIZE;
        }
    } else {
        LOGM_ERROR("read file type box fail\n");
        return 0;
    }

    DCHECK_DEMUXER_BOX_END

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::handleMediaDataBox(SISOMMediaDataBox *p_box, TU8 *p)
{
    p += readBaseBox(&p_box->base_box, p);

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::readMovieHeaderBox(SISOMMovieHeaderBox *p_box, TU8 *p)
{
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    if (DLikely(!p_box->base_full_box.version)) {
        DBER32(p_box->creation_time, p);
        p += 4;
        DBER32(p_box->modification_time, p);
        p += 4;
        DBER32(p_box->timescale, p);
        p += 4;
        DBER32(p_box->duration, p);
        p += 4;
    } else {
        DBER64(p_box->creation_time, p);
        p += 8;
        DBER64(p_box->modification_time, p);
        p += 8;
        DBER32(p_box->timescale, p);
        p += 4;
        DBER64(p_box->duration, p);
        p += 8;
    }

    DBER32(p_box->rate, p);
    p += 4;
    DBER16(p_box->volume, p);
    p += 2;
    DBER16(p_box->reserved, p);
    p += 2;
    DBER32(p_box->reserved_1, p);
    p += 4;
    DBER32(p_box->reserved_2, p);
    p += 4;

    for (TU32 i = 0; i < 9; i ++) {
        DBER32(p_box->matrix[i], p);
        p += 4;
    }

    for (TU32 i = 0; i < 6; i ++) {
        DBER32(p_box->pre_defined[i], p);
        p += 4;
    }

    DBER32(p_box->next_track_ID, p);
    p += 4;

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readMovieTrackHeaderBox(SISOMTrackHeader *p_box, TU8 *p)
{
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    if (DLikely(!p_box->base_full_box.version)) {
        DBER32(p_box->creation_time, p);
        p += 4;
        DBER32(p_box->modification_time, p);
        p += 4;
        DBER32(p_box->track_ID, p);
        p += 4;
        DBER32(p_box->reserved, p);
        p += 4;
        DBER32(p_box->duration, p);
        p += 4;
    } else {
        DBER64(p_box->creation_time, p);
        p += 8;
        DBER64(p_box->modification_time, p);
        p += 8;
        DBER32(p_box->track_ID, p);
        p += 4;
        DBER32(p_box->reserved, p);
        p += 4;
        DBER64(p_box->duration, p);
        p += 8;
    }

    for (TU32 i = 0; i < 2; i ++) {
        DBER32(p_box->reserved_1[i], p);
        p += 4;
    }

    DBER16(p_box->layer, p);
    p += 2;
    DBER16(p_box->alternate_group, p);
    p += 2;
    DBER16(p_box->volume, p);
    p += 2;
    DBER16(p_box->reserved_2, p);
    p += 2;

    for (TU32 i = 0; i < 9; i ++) {
        DBER32(p_box->matrix[i], p);
        p += 4;
    }

    DBER32(p_box->width, p);
    p += 4;
    DBER32(p_box->height, p);
    p += 4;

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readMediaHeaderBox(SISOMMediaHeaderBox *p_box, TU8 *p)
{
    TU32 v;

    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    if (DLikely(!p_box->base_full_box.version)) {
        DBER32(p_box->creation_time, p);
        p += 4;
        DBER32(p_box->modification_time, p);
        p += 4;
        DBER32(p_box->timescale, p);
        p += 4;
        DBER32(p_box->duration, p);
        p += 4;
    } else {
        DBER64(p_box->creation_time, p);
        p += 8;
        DBER64(p_box->modification_time, p);
        p += 8;
        DBER32(p_box->timescale, p);
        p += 4;
        DBER64(p_box->duration, p);
        p += 8;
    }

    DBER32(v, p);
    p += 4;

    p_box->pad = (v >> 31) & 0x1;
    p_box->language[0] = (v >> 26) & 0x1f;
    p_box->language[1] = (v >> 21) & 0x1f;
    p_box->language[2] = (v >> 16) & 0x1f;
    p_box->pre_defined = v & 0xffff;

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readHanderReferenceBox(SISOMHandlerReferenceBox *p_box, TU8 *p)
{
    TU32 v = 0;
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    DBER32(p_box->pre_defined, p);
    p += 4;

    memcpy(p_box->handler_type, p, DISOM_TAG_SIZE);
    p += DISOM_TAG_SIZE;

    for (TU32 i = 0; i < 3; i ++) {
        DBER32(p_box->reserved[i], p);
        p += 4;
    }

    v = p_box->base_full_box.base_box.size - DISOM_FULL_BOX_SIZE - 4 - DISOM_TAG_SIZE - 12 - 1;

    if (256 > v) {
        p_box->name = (TChar *) DDBG_MALLOC(v + 1, "MPHR");
        if (p_box->name) {
            memcpy((void *)p_box->name, p, v);
            p_box->name[v] = 0x0;
            p += v;
        } else {
            LOGM_FATAL("no memory\n");
            return 0;
        }
    } else {
        LOGM_FATAL("too long %d\n", v);
        return 0;
    }

    p ++;

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readChunkOffsetBox(SISOMChunkOffsetBox *p_box, TU8 *p)
{
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    DBER32(p_box->entry_count, p);
    p += 4;

    p_box->chunk_offset = (TU32 *) DDBG_MALLOC(p_box->entry_count * 4, "MPCO");
    if (p_box->chunk_offset) {
        for (TU32 i = 0; i < p_box->entry_count; i ++) {
            DBER32(p_box->chunk_offset[i], p);
            p += 4;
        }
    } else {
        LOGM_FATAL("no memory\n");
        return 0;
    }

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readSampleToChunkBox(SISOMSampleToChunkBox *p_box, TU8 *p)
{
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    DBER32(p_box->entry_count, p);
    p += 4;

    p_box->entrys = (_SSampleToChunkEntry *) DDBG_MALLOC(p_box->entry_count * sizeof(_SSampleToChunkEntry), "MPSC");

    if (p_box->entrys) {
        for (TU32 i = 0; i < p_box->entry_count; i ++) {
            DBER32(p_box->entrys[i].first_chunk, p);
            p += 4;
            DBER32(p_box->entrys[i].sample_per_chunk, p);
            p += 4;
            DBER32(p_box->entrys[i].sample_description_index, p);
            p += 4;
        }
    } else {
        LOGM_FATAL("no memory\n");
        return 0;
    }

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readSampleSizeBox(SISOMSampleSizeBox *p_box, TU8 *p)
{
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    DBER32(p_box->sample_size, p);
    p += 4;

    DBER32(p_box->sample_count, p);
    p += 4;

    p_box->entry_size = (TU32 *) DDBG_MALLOC(p_box->sample_count * 4, "MPSS");

    if (p_box->entry_size) {
        for (TU32 i = 0; i < p_box->sample_count; i ++) {
            DBER32(p_box->entry_size[i], p);
            p += 4;
        }
    } else {
        LOGM_FATAL("no memory\n");
        return 0;
    }

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readAVCDecoderConfigurationRecordBox(SISOMAVCDecoderConfigurationRecordBox *p_box, TU8 *p)
{
    TU16 v = 0;
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readBaseBox(&p_box->base_box, p);

    p_box->configurationVersion = p[0];
    //memcpy(p_box->p_sps, p + 1, 3);
    p += 4;

    DBER16(v, p);
    p += 2;
    p_box->reserved = (v >> 10);
    p_box->lengthSizeMinusOne = (v >> 8) & 0x3;
    p_box->reserved_1 = (v >> 5) & 0x7;
    p_box->numOfSequenceParameterSets = v & 0x1f;

    DBER16(p_box->sequenceParametersSetLength, p);
    p += 2;

    p_box->p_sps = (TU8 *) DDBG_MALLOC(p_box->sequenceParametersSetLength, "M4AV");
    if (!p_box->p_sps) {
        LOGM_FATAL("no memory, %d\n", p_box->sequenceParametersSetLength);
        return 0;
    }
    memcpy(p_box->p_sps, p, p_box->sequenceParametersSetLength);
    p += p_box->sequenceParametersSetLength;

    p_box->numOfPictureParameterSets = p[0];
    p ++;

    DBER16(p_box->pictureParametersSetLength, p);
    p += 2;

    p_box->p_pps = (TU8 *) DDBG_MALLOC(p_box->pictureParametersSetLength, "M4AV");
    if (!p_box->p_pps) {
        LOGM_FATAL("no memory, %d\n", p_box->pictureParametersSetLength);
        return 0;
    }
    memcpy(p_box->p_pps, p, p_box->pictureParametersSetLength);
    p += p_box->pictureParametersSetLength;

    DCHECK_DEMUXER_BOX_END

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::readHEVCDecoderConfigurationRecordBox(SISOMHEVCDecoderConfigurationRecordBox *p_box, TU8 *p)
{
    TU16 v = 0;
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readBaseBox(&p_box->base_box, p);

    p_box->record.configurationVersion = p[0];
    p ++;

    v = p[0];
    p ++;
    p_box->record.general_profile_space = (v >> 6) & 0x3;
    p_box->record.general_tier_flag = (v >> 5) & 0x1;
    p_box->record.general_profile_idc = v & 0x1f;

    DBER32(p_box->record.general_profile_compatibility_flags, p);
    p += 4;

    DBER48(p_box->record.general_constraint_indicator_flags, p);
    p += 6;

    p_box->record.general_level_idc = p[0];
    p ++;

    DBER16(v, p);
    p += 2;

    p_box->record.min_spatial_segmentation_idc = v & 0xfff;
    p_box->record.parallelismType = p[0] & 0x3;
    p ++;

    p_box->record.chromaFormat = p[0] & 0x3;
    p ++;

    p_box->record.bitDepthLumaMinus8 = p[0] & 0x7;
    p ++;

    p_box->record.bitDepthChromaMinus8 = p[0] & 0x7;
    p ++;

    DBER16(p_box->record.avgFrameRate, p);
    p += 2;

    v = p[0];
    p ++;

    p_box->record.constantFrameRate = (v >> 6) & 0x3;
    p_box->record.numTemporalLayers = (v >> 3) & 0x7;
    p_box->record.temporalIdNested  = (v >> 2) & 0x1;
    p_box->record.lengthSizeMinusOne = v & 0x3;
    p_box->record.numOfArrays = p[0];
    p ++;

    if ((0x80 | EHEVCNalType_VPS) != p[0]) {
        LOGM_FATAL("not vps\n");
        return 0;
    }
    p += 3;

    DBER16(p_box->vps_length, p);
    p += 2;

    p_box->vps = (TU8 *) DDBG_MALLOC(p_box->vps_length, "M4HV");
    if (!p_box->vps) {
        LOGM_FATAL("no memory, %d\n", p_box->vps_length);
        return 0;
    }
    memcpy(p_box->vps, p, p_box->vps_length);
    p += p_box->vps_length;

    if ((0x80 | EHEVCNalType_SPS) != p[0]) {
        LOGM_FATAL("not sps\n");
        return 0;
    }
    p += 3;

    DBER16(p_box->sps_length, p);
    p += 2;

    p_box->sps = (TU8 *) DDBG_MALLOC(p_box->sps_length, "M4HS");
    if (!p_box->sps) {
        LOGM_FATAL("no memory, %d\n", p_box->sps_length);
        return 0;
    }
    memcpy(p_box->sps, p, p_box->sps_length);
    p += p_box->sps_length;

    if ((0x80 | EHEVCNalType_PPS) != p[0]) {
        LOGM_FATAL("not pps\n");
        return 0;
    }
    p += 3;

    DBER16(p_box->pps_length, p);
    p += 2;

    p_box->pps = (TU8 *) DDBG_MALLOC(p_box->pps_length, "M4HP");
    if (!p_box->pps) {
        LOGM_FATAL("no memory, %d\n", p_box->pps_length);
        return 0;
    }
    memcpy(p_box->pps, p, p_box->pps_length);
    p += p_box->pps_length;

    DCHECK_DEMUXER_BOX_END

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::readVisualSampleDescriptionBox(SISOMVisualSampleDescriptionBox *p_box, TU8 *p)
{
    TU32 read_size;
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    DBER32(p_box->entry_count, p);
    p += 4;

    p += readBaseBox(&p_box->visual_entry.base_box, p);

    memcpy(p_box->visual_entry.reserved_0, p, 6);
    p += 6;

    DBER16(p_box->visual_entry.data_reference_index, p);
    p += 2;

    DBER16(p_box->visual_entry.pre_defined, p);
    p += 2;

    DBER16(p_box->visual_entry.reserved, p);
    p += 2;

    for (TU32 i = 0; i < 3; i ++) {
        DBER32(p_box->visual_entry.pre_defined_1[i], p);
        p += 4;
    }

    DBER16(p_box->visual_entry.width, p);
    p += 2;

    DBER16(p_box->visual_entry.height, p);
    p += 2;

    DBER32(p_box->visual_entry.horizresolution, p);
    p += 4;

    DBER32(p_box->visual_entry.vertresolution, p);
    p += 4;

    DBER32(p_box->visual_entry.reserved_1, p);
    p += 4;

    DBER16(p_box->visual_entry.frame_count, p);
    p += 2;

    memcpy(p_box->visual_entry.compressorname, p, 32);
    p += 32;

    DBER16(p_box->visual_entry.depth, p);
    p += 2;

    DBER16(p_box->visual_entry.pre_defined_2, p);
    p += 2;

    if (!memcmp(p + 4, DISOM_AVC_DECODER_CONFIGURATION_RECORD_TAG, 4)) {
        read_size = readAVCDecoderConfigurationRecordBox(&p_box->visual_entry.avc_config, p);
        if (DUnlikely(!read_size)) {
            LOGM_ERROR("read avcc fail\n");
            return 0;
        }
        p += read_size;
        mVideoFormat = StreamFormat_H264;
    } else if (!memcmp(p + 4, DISOM_HEVC_DECODER_CONFIGURATION_RECORD_TAG, 4)) {
        read_size = readHEVCDecoderConfigurationRecordBox(&p_box->visual_entry.hevc_config, p);
        if (DUnlikely(!read_size)) {
            LOGM_ERROR("read hvcc fail\n");
            return 0;
        }
        p += read_size;
        mVideoFormat = StreamFormat_H265;
    } else {
        TChar tag[8] = {0};
        memcpy(tag, p + 4, 4);
        LOGM_FATAL("not supported: %s\n", tag);
        return 0;
    }

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readAACElementaryStreamDescriptionBox(SISOMAACElementaryStreamDescriptor *p_box, TU8 *p)
{
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    p_box->es_descriptor_type_tag = p[0];
    p ++;

    p_box->strange_size_0 = readStrangeSize(p);
    p += 4;

    DBER16(p_box->track_id, p);
    p += 2;

    p_box->flags = p[0];
    p ++;

    p_box->decoder_descriptor_type_tag = p[0];
    p ++;

    p_box->strange_size_1 = readStrangeSize(p);
    p += 4;

    p_box->object_type = p[0];
    p ++;

    p_box->stream_flag = p[0];
    p ++;

    DBER24(p_box->buffer_size, p);
    p += 3;

    DBER32(p_box->max_bitrate, p);
    p += 4;

    DBER32(p_box->avg_bitrate, p);
    p += 4;

    p_box->decoder_config_type_tag = p[0];
    p ++;

    p_box->strange_size_2 = readStrangeSize(p);
    p += 4;

    if (2 != p_box->strange_size_2) {
        LOGM_FATAL("aac config len(%d) is not 2\n", p_box->strange_size_2);
        return 0;
    }
    p_box->config[0] = p[0];
    p_box->config[1] = p[1];
    p += 2;

    p_box->SL_descriptor_type_tag = p[0];
    p ++;

    p_box->strange_size_3 = readStrangeSize(p);
    p += 4;

    p_box->SL_value = p[0];
    p ++;

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readAudioSampleDescriptionBox(SISOMAudioSampleDescriptionBox *p_box, TU8 *p)
{
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    DBER32(p_box->entry_count, p);
    p += 4;

    p += readBaseBox(&p_box->audio_entry.base_box, p);

    memcpy(p_box->audio_entry.reserved_0, p, 6);
    p += 6;

    DBER16(p_box->audio_entry.data_reference_index, p);
    p += 2;

    DBER32(p_box->audio_entry.reserved[0], p);
    p += 4;

    DBER32(p_box->audio_entry.reserved[1], p);
    p += 4;

    DBER16(p_box->audio_entry.channelcount, p);
    p += 2;

    DBER16(p_box->audio_entry.samplesize, p);
    p += 2;

    DBER16(p_box->audio_entry.pre_defined, p);
    p += 2;

    DBER16(p_box->audio_entry.reserved_1, p);
    p += 2;

    DBER32(p_box->audio_entry.samplerate, p);
    p += 4;

    if (DUnlikely(512000 < p_box->audio_entry.samplerate)) {
        LOGM_WARN("bad samplerate %d, 0x%08x, set it to default %d\n", p_box->audio_entry.samplerate, p_box->audio_entry.samplerate, DDefaultAudioSampleRate);
        p_box->audio_entry.samplerate = DDefaultAudioSampleRate;
    }

    if (!memcmp(p + 4, DISOM_ELEMENTARY_STREAM_DESCRIPTOR_BOX_TAG, 4)) {
        TU32 read_size = 0;
        read_size = readAACElementaryStreamDescriptionBox(&p_box->audio_entry.esd, p);
        if (DUnlikely(!read_size)) {
            LOGM_ERROR("read esd fail\n");
            return 0;
        }
        p += read_size;
        mAudioFormat = StreamFormat_AAC;
    }  else {
        TChar tag[8] = {0};
        memcpy(tag, p + 4, 4);
        LOGM_FATAL("not supported: %s\n", tag);
        return 0;
    }

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readDecodingTimeToSampleBox(SISOMDecodingTimeToSampleBox *p_box, TU8 *p)
{
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    DBER32(p_box->entry_count, p);
    p += 4;

    p_box->p_entry = (_STimeEntry *) DDBG_MALLOC(p_box->entry_count * sizeof(_STimeEntry), "M4DT");

    if (p_box->p_entry) {
        for (TU32 i = 0; i < p_box->entry_count; i ++) {
            DBER32(p_box->p_entry[i].sample_count, p);
            p += 4;
            DBER32(p_box->p_entry[i].sample_delta, p);
            p += 4;
        }
    } else {
        LOGM_FATAL("no memory, %d\n", p_box->entry_count);
        return 0;
    }

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readVideoSampleTableBox(SISOMSampleTableBox *p_box, TU8 *p)
{
    TU32 read_size = 0;
    TU32 remain_size = 0;
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readBaseBox(&p_box->base_box, p);

    if (p_box->base_box.size < DISOM_BOX_SIZE) {
        LOGM_FATAL("size not expected, data corruption\n");
        return 0;
    }

    remain_size = p_box->base_box.size - DISOM_BOX_SIZE;

    while (remain_size) {
        DBER32(read_size, p);
        if (DUnlikely(read_size > remain_size)) {
            LOGM_ERROR("data corruption, %d, %d\n", read_size, remain_size);
            return 0;
        }
        remain_size -= read_size;

        if (!memcmp(p + 4, DISOM_SAMPLE_DESCRIPTION_BOX_TAG, 4)) {
            read_size = readVisualSampleDescriptionBox(&p_box->visual_sample_description, p);
            if (DUnlikely(!read_size)) {
                LOGM_ERROR("read video sample desc box fail\n");
                return 0;
            }
            p += read_size;
        } else if (!memcmp(p + 4, DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG, 4)) {
            read_size = readDecodingTimeToSampleBox(&p_box->stts, p);
            if (DUnlikely(!read_size)) {
                LOGM_ERROR("read decoding time to sample box fail\n");
                return 0;
            }
            p += read_size;
        } else if (!memcmp(p + 4, DISOM_SAMPLE_SIZE_BOX_TAG, 4)) {
            read_size = readSampleSizeBox(&p_box->sample_size, p);
            if (DUnlikely(!read_size)) {
                LOGM_ERROR("read sample size box fail\n");
                return 0;
            }
            p += read_size;
        } else if (!memcmp(p + 4, DISOM_SAMPLE_TO_CHUNK_BOX_TAG, 4)) {
            read_size = readSampleToChunkBox(&p_box->sample_to_chunk, p);
            if (DUnlikely(!read_size)) {
                LOGM_ERROR("read sample to chunk box fail\n");
                return 0;
            }
            p += read_size;
        } else if (!memcmp(p + 4, DISOM_CHUNK_OFFSET_BOX_TAG, 4)) {
            read_size = readChunkOffsetBox(&p_box->chunk_offset, p);
            if (DUnlikely(!read_size)) {
                LOGM_ERROR("read chunk offset box fail\n");
                return 0;
            }
            p += read_size;
        } else {
            TChar tag[8] = {0};
            memcpy(tag, p + 4, 4);
            LOG_DEBUG("skip boxes, tag %s\n", tag);
            p += read_size;
        }
    }

    DCHECK_DEMUXER_BOX_END

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::readSoundSampleTableBox(SISOMSampleTableBox *p_box, TU8 *p)
{
    TU32 read_size = 0;
    TU32 remain_size = 0;
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readBaseBox(&p_box->base_box, p);

    if (p_box->base_box.size < DISOM_BOX_SIZE) {
        LOGM_FATAL("size not expected, data corruption\n");
        return 0;
    }

    remain_size = p_box->base_box.size - DISOM_BOX_SIZE;

    while (remain_size) {
        DBER32(read_size, p);
        if (DUnlikely(read_size > remain_size)) {
            LOGM_ERROR("data corruption, %d, %d\n", read_size, remain_size);
            return 0;
        }
        remain_size -= read_size;

        if (!memcmp(p + 4, DISOM_SAMPLE_DESCRIPTION_BOX_TAG, 4)) {
            read_size = readAudioSampleDescriptionBox(&p_box->audio_sample_description, p);
            if (DUnlikely(!read_size)) {
                LOGM_ERROR("read audio sample desc box fail\n");
                return 0;
            }
            p += read_size;
        } else if (!memcmp(p + 4, DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG, 4)) {
            read_size = readDecodingTimeToSampleBox(&p_box->stts, p);
            if (DUnlikely(!read_size)) {
                LOGM_ERROR("read decoding time to sample box fail\n");
                return 0;
            }
            p += read_size;
        } else if (!memcmp(p + 4, DISOM_SAMPLE_SIZE_BOX_TAG, 4)) {
            read_size = readSampleSizeBox(&p_box->sample_size, p);
            if (DUnlikely(!read_size)) {
                LOGM_ERROR("read sample size box fail\n");
                return 0;
            }
            p += read_size;
        } else if (!memcmp(p + 4, DISOM_SAMPLE_TO_CHUNK_BOX_TAG, 4)) {
            read_size = readSampleToChunkBox(&p_box->sample_to_chunk, p);
            if (DUnlikely(!read_size)) {
                LOGM_ERROR("read sample to chunk box fail\n");
                return 0;
            }
            p += read_size;
        } else if (!memcmp(p + 4, DISOM_CHUNK_OFFSET_BOX_TAG, 4)) {
            read_size = readChunkOffsetBox(&p_box->chunk_offset, p);
            if (DUnlikely(!read_size)) {
                LOGM_ERROR("read chunk offset box fail\n");
                return 0;
            }
            p += read_size;
        } else {
            TChar tag[8] = {0};
            memcpy(tag, p + 4, 4);
            LOG_WARN("skip boxes, tag %s\n", tag);
            p += read_size;
        }
    }

    DCHECK_DEMUXER_BOX_END

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::readDataReferenceBox(SISOMDataReferenceBox *p_box, TU8 *p)
{
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    DBER32(p_box->entry_count, p);
    p += 4;

    for (TU32 i = 0; i < p_box->entry_count; i ++) {
        p += readFullBox(&p_box->url, p);
    }

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readDataInformationBox(SISOMDataInformationBox *p_box, TU8 *p)
{
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readBaseBox(&p_box->base_box, p);

    p += readDataReferenceBox(&p_box->data_ref, p);

    DCHECK_DEMUXER_BOX_END

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::readVideoMediaHeaderBox(SISOMVideoMediaHeaderBox *p_box, TU8 *p)
{
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    DBER16(p_box->graphicsmode, p);
    p += 2;

    DBER16(p_box->opcolor[0], p);
    p += 2;

    DBER16(p_box->opcolor[1], p);
    p += 2;

    DBER16(p_box->opcolor[2], p);
    p += 2;

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readSoundMediaHeaderBox(SISOMSoundMediaHeaderBox *p_box, TU8 *p)
{
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readFullBox(&p_box->base_full_box, p);

    DBER16(p_box->balanse, p);
    p += 2;

    DBER16(p_box->reserved, p);
    p += 2;

    DCHECK_DEMUXER_FULL_BOX_END

    return p_box->base_full_box.base_box.size;
}

TU32 CMP4Demuxer::readVideoMediaInformationBox(SISOMMediaInformationBox *p_box, TU8 *p)
{
    TU32 read_size = 0;
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readBaseBox(&p_box->base_box, p);

    if (DIS_NOT_BOX_TAG(p, DISOM_VIDEO_MEDIA_HEADER_BOX_TAG)) {
        LOGM_ERROR("not video media header box\n");
        return 0;
    }
    read_size = readVideoMediaHeaderBox(&p_box->video_header, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read video media header box fail\n");
        return 0;
    }
    p += read_size;

    if (DIS_NOT_BOX_TAG(p, DISOM_DATA_INFORMATION_BOX_TAG)) {
        LOGM_ERROR("not data information box\n");
        return 0;
    }
    read_size = readDataInformationBox(&p_box->data_info, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read data information box fail\n");
        return 0;
    }
    p += read_size;

    if (DIS_NOT_BOX_TAG(p, DISOM_SAMPLE_TABLE_BOX_TAG)) {
        LOGM_ERROR("not video sample table box\n");
        return 0;
    }
    read_size = readVideoSampleTableBox(&p_box->sample_table, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read video sample table box fail\n");
        return 0;
    }
    p += read_size;

    DCHECK_DEMUXER_BOX_END

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::readAudioMediaInformationBox(SISOMMediaInformationBox *p_box, TU8 *p)
{
    TU32 read_size = 0;
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readBaseBox(&p_box->base_box, p);

    if (DIS_NOT_BOX_TAG(p, DISOM_SOUND_MEDIA_HEADER_BOX_TAG)) {
        LOGM_ERROR("not sound media header box\n");
        return 0;
    }
    read_size = readSoundMediaHeaderBox(&p_box->sound_header, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read sound media header box fail\n");
        return 0;
    }
    p += read_size;

    if (DIS_NOT_BOX_TAG(p, DISOM_DATA_INFORMATION_BOX_TAG)) {
        LOGM_ERROR("not data information box\n");
        return 0;
    }
    read_size = readDataInformationBox(&p_box->data_info, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read data information box fail\n");
        return 0;
    }
    p += read_size;

    if (DIS_NOT_BOX_TAG(p, DISOM_SAMPLE_TABLE_BOX_TAG)) {
        LOGM_ERROR("not sound sample table box\n");
        return 0;
    }
    read_size = readSoundSampleTableBox(&p_box->sample_table, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read sound sample table box fail\n");
        return 0;
    }
    p += read_size;

    DCHECK_DEMUXER_BOX_END

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::readVideoMediaBox(SISOMMediaBox *p_box, TU8 *p)
{
    TU32 read_size = 0;
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readBaseBox(&p_box->base_box, p);

    if (DIS_NOT_BOX_TAG(p, DISOM_MEDIA_HEADER_BOX_TAG)) {
        LOGM_ERROR("not media header box\n");
        return 0;
    }
    read_size = readMediaHeaderBox(&p_box->media_header, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read media header box fail\n");
        return 0;
    }
    p += read_size;

    if (DIS_NOT_BOX_TAG(p, DISOM_HANDLER_REFERENCE_BOX_TAG)) {
        LOGM_ERROR("not handler ref box\n");
        return 0;
    }
    read_size = readHanderReferenceBox(&p_box->media_handler, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read handler ref box fail\n");
        return 0;
    }
    p += read_size;

    if (DIS_NOT_BOX_TAG(p, DISOM_MEDIA_INFORMATION_BOX_TAG)) {
        LOGM_ERROR("not media info box\n");
        return 0;
    }
    read_size = readVideoMediaInformationBox(&p_box->media_info, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read video media info box fail\n");
        return 0;
    }
    p += read_size;

    DCHECK_DEMUXER_BOX_END

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::readAudioMediaBox(SISOMMediaBox *p_box, TU8 *p)
{
    TU32 read_size = 0;
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readBaseBox(&p_box->base_box, p);

    if (DIS_NOT_BOX_TAG(p, DISOM_MEDIA_HEADER_BOX_TAG)) {
        LOGM_ERROR("not media header box\n");
        return 0;
    }
    read_size = readMediaHeaderBox(&p_box->media_header, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read media header box fail\n");
        return 0;
    }
    p += read_size;

    if (DIS_NOT_BOX_TAG(p, DISOM_HANDLER_REFERENCE_BOX_TAG)) {
        LOGM_ERROR("not handler ref box\n");
        return 0;
    }
    read_size = readHanderReferenceBox(&p_box->media_handler, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read handler ref box fail\n");
        return 0;
    }
    p += read_size;

    if (DIS_NOT_BOX_TAG(p, DISOM_MEDIA_INFORMATION_BOX_TAG)) {
        LOGM_ERROR("not media info box\n");
        return 0;
    }
    read_size = readAudioMediaInformationBox(&p_box->media_info, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read audio media info box fail\n");
        return 0;
    }
    p += read_size;

    DCHECK_DEMUXER_BOX_END

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::readMovieVideoTrackBox(SISOMTrackBox *p_box, TU8 *p)
{
    TU32 read_size = 0;
    TU8 *p_ori = p;
    DCHECK_DEMUXER_BOX_BEGIN;

    p += readBaseBox(&p_box->base_box, p);

    if (DIS_NOT_BOX_TAG(p, DISOM_TRACK_HEADER_BOX_TAG)) {
        LOGM_ERROR("not track header box\n");
        return 0;
    }
    read_size = readMovieTrackHeaderBox(&p_box->track_header, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read movie track header box fail\n");
        return 0;
    }
    p += read_size;

    if (DIS_NOT_BOX_TAG(p, DISOM_MEDIA_BOX_TAG)) {
        LOGM_ERROR("not track media box\n");
        return 0;
    }
    read_size = readVideoMediaBox(&p_box->media, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read movie media box fail\n");
        return 0;
    }
    p += read_size;

    while (((TULong)(p_ori + p_box->base_box.size)) > (TULong) p) {
        if (!DIS_NOT_BOX_TAG(p, DISOM_EDIT_LIST_CONTAINER_BOX_TAG)) {
        } else if (!DIS_NOT_BOX_TAG(p, DISOM_USER_DATA_BOX_TAG)) {
        } else {
            TChar tag[8] = {0};
            memcpy(tag, p + 4, 4);
            LOG_ERROR("not expected box: %s\n", tag);
            return 0;
        }
        DBER32(read_size, p);
        p += read_size;
    }

    DCHECK_DEMUXER_BOX_END

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::readMovieAudioTrackBox(SISOMTrackBox *p_box, TU8 *p)
{
    TU32 read_size = 0;
    TU8 *p_ori = p;

    DCHECK_DEMUXER_BOX_BEGIN;

    p += readBaseBox(&p_box->base_box, p);

    if (DIS_NOT_BOX_TAG(p, DISOM_TRACK_HEADER_BOX_TAG)) {
        LOGM_ERROR("not track header box\n");
        return 0;
    }
    read_size = readMovieTrackHeaderBox(&p_box->track_header, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read movie track header box fail\n");
        return 0;
    }
    p += read_size;

    if (DIS_NOT_BOX_TAG(p, DISOM_MEDIA_BOX_TAG)) {
        LOGM_ERROR("not track media box\n");
        return 0;
    }
    read_size = readAudioMediaBox(&p_box->media, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read movie media box fail\n");
        return 0;
    }
    p += read_size;

    while (((TULong)(p_ori + p_box->base_box.size)) > (TULong) p) {
        if (!DIS_NOT_BOX_TAG(p, DISOM_EDIT_LIST_CONTAINER_BOX_TAG)) {
        } else if (!DIS_NOT_BOX_TAG(p, DISOM_USER_DATA_BOX_TAG)) {
        } else {
            TChar tag[8] = {0};
            memcpy(tag, p + 4, 4);
            LOG_ERROR("not expected box: %s\n", tag);
            return 0;
        }
        DBER32(read_size, p);
        p += read_size;
    }

    DCHECK_DEMUXER_BOX_END

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::readMovieBox(SISOMMovieBox *p_box, TU8 *p)
{
    TU32 read_size = 0;
    TU32 remain_size = 0;
    TU32 track_index = 0;
    TU8 handler_tag[4] = {0};
    EECode err = EECode_OK;

    DCHECK_DEMUXER_BOX_BEGIN;

    p += readBaseBox(&p_box->base_box, p);

    if (DIS_NOT_BOX_TAG(p, DISOM_MOVIE_HEADER_BOX_TAG)) {
        TChar tag[8] = {0};
        memcpy(tag, p + 4, 4);
        LOGM_FATAL("not movie header box, %s\n", tag);
        return 0;
    }

    read_size = readMovieHeaderBox(&p_box->movie_header_box, p);
    if (DUnlikely(!read_size)) {
        LOGM_ERROR("read movie header box fail\n");
        return 0;
    }
    p += read_size;

    if (p_box->base_box.size <= read_size) {
        LOGM_ERROR("bad movie box size, %d, header size %d\n", p_box->base_box.size, read_size);
        return 0;
    }

    if (p_box->base_box.size < (read_size + DISOM_BOX_SIZE)) {
        LOGM_FATAL("size not expected, data corruption\n");
        return 0;
    }

    remain_size = p_box->base_box.size - read_size - DISOM_BOX_SIZE;

    while (remain_size) {

        DBER32(read_size, p);
        if (read_size > remain_size) {
            LOGM_ERROR("bad track size %d, %d\n", read_size, remain_size);
            return 0;
        }

        if (DIS_NOT_BOX_TAG(p, DISOM_TRACK_BOX_TAG)) {
            TChar tag[8] = {0};
            memcpy(tag, p + 4, 4);
            LOGM_DEBUG("skip tag %s, size %d\n", tag, read_size);
            p += read_size;
            remain_size -= read_size;
            continue;
        }

        err = __fast_get_handler_type(p, handler_tag, remain_size);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("get handler type fail\n");
            return 0;
        }
        remain_size -= read_size;

        if (!memcmp(handler_tag, DISOM_VIDEO_HANDLER_REFERENCE_TAG, 4)) {
            read_size = readMovieVideoTrackBox(&p_box->video_track, p);
            if (DUnlikely(!read_size)) {
                LOGM_ERROR("read movie video track box fail\n");
                return 0;
            }
            p += read_size;
            mbHaveVideo = 1;
            track_index ++;
        } else if (!memcmp(handler_tag, DISOM_AUDIO_HANDLER_REFERENCE_TAG, 4)) {
            read_size = readMovieAudioTrackBox(&p_box->audio_track, p);
            if (DUnlikely(!read_size)) {
                LOGM_ERROR("read movie audio track box fail\n");
                return 0;
            }
            p += read_size;
            mbHaveAudio = 1;
            track_index ++;
        } else if (!memcmp(handler_tag, DISOM_HINT_HANDLER_REFERENCE_TAG, 4)) {
            DBER32(read_size, p);
            p += read_size;
        } else {
            LOGM_ERROR("get handler type fail\n");
            return 0;
        }
    }

    DCHECK_DEMUXER_BOX_END

    return p_box->base_box.size;
}

TU32 CMP4Demuxer::readStrangeSize(TU8 *p)
{
    return ((((TU32) p[0]) & 0x7f) << 21) | ((((TU32) p[1]) & 0x7f) << 14) | ((((TU32) p[2]) & 0x7f) << 7) | (((TU32) p[3]) & 0x7f);
}

TU32 CMP4Demuxer::isFileHeaderValid()
{
    if (strncmp(DISOM_BRAND_V0_TAG, mFileTypeBox.major_brand, DISOM_TAG_SIZE)) {
        return 1;
    }

    if (strncmp(DISOM_BRAND_V1_TAG, mFileTypeBox.major_brand, DISOM_TAG_SIZE)) {
        return 1;
    }

    return 0;
}

EECode CMP4Demuxer::parseFile()
{
    TU8 *p = NULL;
    TIOSize tsize = 0;
    TU32 read_size = 0;
    TU64 cur_file_offset = 0;
    TU32 mdbox_index = 0;
    TU32 has_movie_box = 0;
    TU32 has_mediadata_box = 0;
    EECode err;

#define DFILE_TRY_READ_HEADER_SIZE 256

    read_size = sizeof(SISOMFileTypeBox) + sizeof(SISOMMediaDataBox);

    if (DUnlikely(mFileTotalSize < (read_size + 128))) {
        LOGM_FATAL("file size (%lld) too small, it's not possible\n", mFileTotalSize);
        return EECode_BadFormat;
    }

    tsize = read_size;
    err = mpIO->Read(mpCacheBufferBaseAligned, 1, tsize);
    if (DUnlikely(EECode_OK != err) || (read_size != tsize)) {
        LOGM_FATAL("read first %lld bytes fail\n", tsize);
        return EECode_IOError;
    }

    p = mpCacheBufferBaseAligned;

    if (DIS_NOT_BOX_TAG(p, DISOM_FILE_TYPE_BOX_TAG)) {
        TChar tag[8] = {0};
        memcpy(tag, p + 4, 4);
        LOGM_FATAL("not file type box, %s\n", tag);
        return EECode_Error;
    }

    read_size = readFileTypeBox(&mFileTypeBox, p);
    if (DUnlikely(!read_size)) {
        LOGM_FATAL("read file type box fail, corrupted file header\n");
        return EECode_IOError;
    } else {
        LOG_NOTICE("file type: %c%c%c%c\n", mFileTypeBox.major_brand[0], mFileTypeBox.major_brand[1], mFileTypeBox.major_brand[2], mFileTypeBox.major_brand[3]);
        if (DUnlikely(0 == isFileHeaderValid())) {
            LOGM_FATAL("not valid file type\n");
            return EECode_Error;
        }
    }
    p += read_size;
    cur_file_offset = read_size;

    while (1) {
        if (!DIS_NOT_BOX_TAG(p, DISOM_MEDIA_DATA_BOX_TAG)) {
            if (DUnlikely(DMAX_MEDIA_DATA_BOX_NUMBER <= mdbox_index)) {
                LOGM_FATAL("too many media data boxes, %d\n", mdbox_index);
                return EECode_BadFormat;
            }
            read_size = handleMediaDataBox(&mMediaDataBox[mdbox_index], p);
            if (DUnlikely(!read_size)) {
                LOGM_FATAL("read media data box fail, corrupted file header\n");
                return EECode_IOError;
            }

            cur_file_offset += mMediaDataBox[mdbox_index].base_box.size;
            mdbox_index ++;
            mnMediaDataBoxes ++;

            has_mediadata_box = 1;
        } else if (!DIS_NOT_BOX_TAG(p, DISOM_MOVIE_BOX_TAG)) {
            DBER32(read_size, p);
            mFileMovieBoxOffset = cur_file_offset;
            err = mpIO->Seek((TIOSize) mFileMovieBoxOffset, EIOPosition_Begin);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("file seek error\n");
                return EECode_IOError;
            }

            if (read_size > mCacheBufferSize) {
                mCacheBufferSize = read_size;
                if (mpCacheBufferBase) {
                    DDBG_FREE(mpCacheBufferBase, "M4PF");
                    mpCacheBufferBase = NULL;
                }

                mpCacheBufferBase = (TU8 *) DDBG_MALLOC(mCacheBufferSize + 31, "M4CH");
                if (!mpCacheBufferBase) {
                    LOGM_FATAL("No Memory! request size %llu\n", mCacheBufferSize + 31);
                    return EECode_NoMemory;
                }

                mpCacheBufferBaseAligned = (TU8 *)((((TULong)mpCacheBufferBase) + 31) & (~0x1f));
            }

            tsize = read_size;
            err = mpIO->Read(mpCacheBufferBaseAligned, 1, tsize);
            if (DUnlikely(EECode_OK != err) || (read_size != tsize)) {
                LOGM_FATAL("read movie box %lld bytes fail\n", tsize);
                return EECode_IOError;
            }
            p = mpCacheBufferBaseAligned;

            read_size = readMovieBox(&mMovieBox, p);
            if (DUnlikely((!read_size) || (read_size != mMovieBox.base_box.size))) {
                LOGM_FATAL("movie box read fail, %d, %d\n", read_size, mMovieBox.base_box.size);
                return EECode_BadFormat;
            }
            cur_file_offset += mMovieBox.base_box.size;

            mpVideoSampleTableBox = &mMovieBox.video_track.media.media_info.sample_table;
            mpAudioSampleTableBox = &mMovieBox.audio_track.media.media_info.sample_table;

            if ((1 == mpVideoSampleTableBox->sample_to_chunk.entry_count) \
                    && (mpVideoSampleTableBox->sample_to_chunk.entrys) \
                    && (1 == mpVideoSampleTableBox->sample_to_chunk.entrys[0].first_chunk) \
                    && (1 == mpVideoSampleTableBox->sample_to_chunk.entrys[0].sample_per_chunk) \
                    && (1 == mpVideoSampleTableBox->sample_to_chunk.entrys[0].sample_description_index)) {
                mbVideoOneSamplePerChunk = 1;
            } else {
                mbVideoOneSamplePerChunk = 0;
            }
            mTotalVideoFrameCountMinus1 = mpVideoSampleTableBox->sample_size.sample_count - 1;

            if ((1 == mpAudioSampleTableBox->sample_to_chunk.entry_count) \
                    && (mpAudioSampleTableBox->sample_to_chunk.entrys) \
                    && (1 == mpAudioSampleTableBox->sample_to_chunk.entrys[0].first_chunk) \
                    && (1 == mpAudioSampleTableBox->sample_to_chunk.entrys[0].sample_per_chunk) \
                    && (1 == mpAudioSampleTableBox->sample_to_chunk.entrys[0].sample_description_index)) {
                mbAudioOneSamplePerChunk = 1;
            } else {
                mbAudioOneSamplePerChunk = 0;
            }
            if (!mpAudioSampleTableBox->sample_size.sample_size) {
                mTotalAudioFrameCountMinus1 = mpAudioSampleTableBox->sample_size.sample_count - 1;
            } else {
                LOGM_FATAL("todo\n");
                return EECode_NotSupported;
            }

            has_movie_box = 1;

        } else {
            TChar tag[8] = {0};
            memcpy(tag, p + 4, 4);
            LOGM_FATAL("not media data or movie box, %s\n", tag);
            return EECode_BadFormat;
        }

        if (cur_file_offset >= mFileTotalSize) {
            break;
        }

        err = mpIO->Seek((TIOSize) cur_file_offset, EIOPosition_Begin);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_ERROR("file seek error\n");
            return EECode_IOError;
        }

        tsize = DFILE_TRY_READ_HEADER_SIZE;
        err = mpIO->Read(mpCacheBufferBaseAligned, 1, tsize);
        if (DUnlikely(EECode_OK != err) || (DFILE_TRY_READ_HEADER_SIZE != tsize)) {
            LOGM_FATAL("read first %lld bytes fail\n", tsize);
            return EECode_IOError;
        }

        p = mpCacheBufferBaseAligned;
    }

    if ((!has_movie_box) || (!has_mediadata_box)) {
        LOGM_FATAL("no media data or movie box\n");
        return EECode_BadFormat;
    }

    if (mbHaveVideo && mbHaveAudio && (mdbox_index > 1)) {
        mbMultipleMediaDataBox = 1;
    }

    if (mbHaveVideo) {
        if (mMovieBox.video_track.media.media_header.timescale) {
            mVideoTimeScale = mMovieBox.video_track.media.media_header.timescale;
        } else if (mMovieBox.movie_header_box.timescale) {
            mVideoTimeScale = mMovieBox.movie_header_box.timescale;
        } else {
            mVideoTimeScale = DDefaultTimeScale;
            LOGM_WARN("no video time scale? use default\n");
        }

        if (1 == mpVideoSampleTableBox->stts.entry_count) {
            mVideoFrameTick = mpVideoSampleTableBox->stts.p_entry[0].sample_delta;
        } else {
            mVideoFrameTick = mpVideoSampleTableBox->stts.p_entry[0].sample_delta;
        }
        mGuessedMinKeyFrameSize = mpVideoSampleTableBox->sample_size.entry_size[0] * 3 / 8;
        LOGM_INFO("video timescale %d, frame tick %d, min size %d\n", mVideoTimeScale, mVideoFrameTick, mGuessedMinKeyFrameSize);
    }

    if (mbHaveAudio) {
        if (mMovieBox.audio_track.media.media_header.timescale) {
            mAudioTimeScale = mMovieBox.audio_track.media.media_header.timescale;
        } else if (mMovieBox.movie_header_box.timescale) {
            mAudioTimeScale = mMovieBox.movie_header_box.timescale;
        } else {
            mVideoTimeScale = DDefaultAudioSampleRate;
            LOGM_WARN("no audio time scale? use default\n");
        }

        if (mpAudioSampleTableBox->stts.p_entry) {
            if (1 == mpAudioSampleTableBox->stts.entry_count) {
                mAudioFrameTick = mpAudioSampleTableBox->stts.p_entry[0].sample_delta;
            } else {
                mAudioFrameTick = mpAudioSampleTableBox->stts.p_entry[0].sample_delta;
            }
        } else {
            mAudioFrameTick = DDefaultAudioFrameSize;
        }
        LOGM_INFO("audio timescale %d, frame tick %d\n", mAudioTimeScale, mAudioFrameTick);
    }

    err = buildSampleOffset();
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("build sample offset fail\n");
        return err;
    }

    err = buildTimeTree();
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("build time fail\n");
        return err;
    }

    return EECode_OK;
}

EECode CMP4Demuxer::getVideoOffsetSize(TU64 &offset, TU32 &size)
{
    if (DUnlikely(mCurVideoFrameIndex > mTotalVideoFrameCountMinus1)) {
        LOGM_NOTICE("read video eof\n");
        return EECode_OK_EOF;
    }

    if (!mpVideoSampleTableBox->sample_size.sample_size) {
        size = mpVideoSampleTableBox->sample_size.entry_size[mCurVideoFrameIndex];
    } else {
        size = mpVideoSampleTableBox->sample_size.sample_size;
    }

    if (mbVideoOneSamplePerChunk) {
        offset = mpVideoSampleTableBox->chunk_offset.chunk_offset[mCurVideoFrameIndex];
    } else {
        offset = mpVideoSampleOffset[mCurVideoFrameIndex];
    }

    if (!mbBackward) {
        mCurVideoFrameIndex ++;
    } else if (mCurVideoFrameIndex) {
        mCurVideoFrameIndex --;
    }

    return EECode_OK;
}

EECode CMP4Demuxer::getAudioOffsetSize(TU64 &offset, TU32 &size)
{
    if (DUnlikely(mCurAudioFrameIndex > mTotalAudioFrameCountMinus1)) {
        LOGM_NOTICE("read audio eof\n");
        return EECode_OK_EOF;
    }

    if (!mpAudioSampleTableBox->sample_size.sample_size) {
        size = mpAudioSampleTableBox->sample_size.entry_size[mCurAudioFrameIndex];
    } else {
        size = mpAudioSampleTableBox->sample_size.sample_size;
    }

    if (mbAudioOneSamplePerChunk) {
        offset = mpAudioSampleTableBox->chunk_offset.chunk_offset[mCurAudioFrameIndex];
    } else {
        offset = mpAudioSampleOffset[mCurAudioFrameIndex];
    }

    mCurAudioFrameIndex ++;

    return EECode_OK;
}

EECode CMP4Demuxer::queryVideoOffsetSize(TU64 &offset, TU32 &size)
{
    if (DUnlikely(mCurVideoFrameIndex > mTotalVideoFrameCountMinus1)) {
        LOGM_NOTICE("read video eof\n");
        return EECode_OK_EOF;
    }

    if (!mpVideoSampleTableBox->sample_size.sample_size) {
        size = mpVideoSampleTableBox->sample_size.entry_size[mCurVideoFrameIndex];
    } else {
        size = mpVideoSampleTableBox->sample_size.sample_size;
    }

    if (mbVideoOneSamplePerChunk) {
        offset = mpVideoSampleTableBox->chunk_offset.chunk_offset[mCurVideoFrameIndex];
    } else {
        offset = mpVideoSampleOffset[mCurVideoFrameIndex];
    }

    return EECode_OK;
}

EECode CMP4Demuxer::queryAudioOffsetSize(TU64 &offset, TU32 &size)
{
    if (DUnlikely(mCurAudioFrameIndex > mTotalAudioFrameCountMinus1)) {
        LOGM_NOTICE("read audio eof\n");
        return EECode_OK_EOF;
    }

    if (!mpAudioSampleTableBox->sample_size.sample_size) {
        size = mpAudioSampleTableBox->sample_size.entry_size[mCurAudioFrameIndex];
    } else {
        size = mpAudioSampleTableBox->sample_size.sample_size;
    }

    if (mbAudioOneSamplePerChunk) {
        offset = mpAudioSampleTableBox->chunk_offset.chunk_offset[mCurAudioFrameIndex];
    } else {
        offset = mpAudioSampleOffset[mCurAudioFrameIndex];
    }

    return EECode_OK;
}

EECode CMP4Demuxer::copyFileData(TU8 *p_buf, TU64 offset, TU32 size)
{
    EECode err;
    TIOSize iosize = 0;

    if (DUnlikely(mCurFileOffset != offset)) {
        err = mpIO->Seek(offset, EIOPosition_Begin);
        if (EECode_OK != err) {
            LOGM_FATAL("seek fail\n");
            return err;
        }
        mCurFileOffset = offset;
    }

    iosize = size;
    err = mpIO->Read(p_buf, 1, iosize);
    if (DLikely((EECode_OK == err) && (size == iosize))) {
        mCurFileOffset += size;
        return EECode_OK;
    }

    LOGM_FATAL("read fail\n");
    return EECode_IOError;
}

EECode CMP4Demuxer::readVideoFrame(TU8 *p_buf, TU32 &size)
{
    if (DLikely(mbHaveVideo)) {
        TU64 frame_offset = 0;
        TU32 frame_size = 0;
        EECode err;

        err = getVideoOffsetSize(frame_offset, frame_size);
        if (DLikely(EECode_OK == err)) {
            size = frame_size;
            copyFileData(p_buf, frame_offset, frame_size);
            //LOGM_INFO("video cnt %d, frame_offset %llx, frame size %d\n", mCurVideoFrameIndex, frame_offset, frame_size);
        } else if (EECode_OK_EOF == err) {
            return err;
        } else {
            LOGM_ERROR("read video offset size fail\n");
            return err;
        }
    } else {
        LOGM_FATAL("not video data\n");
        return EECode_BadState;
    }

    __convert_to_annexb(p_buf, size);

    return EECode_OK;
}

EECode CMP4Demuxer::readAudioFrame(TU8 *p_buf, TU32 &size)
{
    if (DLikely(mbHaveAudio)) {
        TU64 frame_offset = 0;
        TU32 frame_size = 0;
        EECode err;

        err = getAudioOffsetSize(frame_offset, frame_size);
        if (DLikely(EECode_OK == err)) {
            size = frame_size;
            copyFileData(p_buf, frame_offset, frame_size);
            //LOGM_INFO("audio cnt %d, frame_offset %llx, frame size %d\n", mCurAudioFrameIndex, frame_offset, frame_size);
        } else if (EECode_OK_EOF == err) {
            return err;
        } else {
            LOGM_ERROR("read video offset size fail\n");
            return err;
        }
    } else {
        LOGM_FATAL("not video data\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CMP4Demuxer::nextPacketInfo(TU32 &size, StreamType &type, TTime &dts, TTime &pts)
{
    EECode err;
    TU64 audio_offset = 0, video_offset = 0;
    TU32 audio_size = 0;

    if (mbAudioEnabled && mbVideoEnabled) {
        err = queryAudioOffsetSize(audio_offset, audio_size);
        if (DLikely(EECode_OK == err)) {
            err = queryVideoOffsetSize(video_offset, size);
            if (DLikely(EECode_OK == err)) {
                if (video_offset < audio_offset) {
                    type = StreamType_Video;
                    getVideoTimestamp(mCurVideoFrameIndex, dts, pts);
                } else {
                    size = audio_size;
                    type = StreamType_Audio;
                    getAudioTimestamp(mCurAudioFrameIndex, dts, pts);
                }
            } else {
                size = audio_size;
                type = StreamType_Audio;
                getAudioTimestamp(mCurAudioFrameIndex, dts, pts);
            }
        } else if (EECode_OK_EOF == err) {
            err = queryVideoOffsetSize(video_offset, size);
            if (DLikely(EECode_OK == err)) {
                type = StreamType_Video;
                getVideoTimestamp(mCurVideoFrameIndex, dts, pts);
            } else if (EECode_OK_EOF == err) {
                size = 0;
                type = StreamType_Invalid;
                LOGM_WARN("return eof here 1\n");
                return EECode_OK_EOF;
            } else {
                LOGM_ERROR("fail here, video\n");
                return err;
            }
        } else {
            LOGM_ERROR("fail here, audio\n");
            return err;
        }
    } else if (mbVideoEnabled) {
        err = queryVideoOffsetSize(video_offset, size);
        if (DLikely(EECode_OK == err)) {
            type = StreamType_Video;
            getVideoTimestamp(mCurVideoFrameIndex, dts, pts);
        } else if (EECode_OK_EOF == err) {
            size = 0;
            type = StreamType_Invalid;
            LOGM_WARN("return eof here 2\n");
            return EECode_OK_EOF;
        } else {
            LOGM_ERROR("fail here, video\n");
            return err;
        }
    } else if (mbAudioEnabled) {
        err = queryAudioOffsetSize(audio_offset, size);
        if (DLikely(EECode_OK == err)) {
            type = StreamType_Audio;
            getAudioTimestamp(mCurAudioFrameIndex, dts, pts);
        } else if (EECode_OK_EOF == err) {
            size = 0;
            type = StreamType_Invalid;
            LOGM_WARN("return eof here 3\n");
            return EECode_OK_EOF;
        } else {
            LOGM_ERROR("fail here, audio\n");
            return err;
        }
    } else {
        LOGM_FATAL("no audio and no video\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CMP4Demuxer::nextPacketInfoTimeOrder(TU32 &size, StreamType &type, TTime &dts, TTime &pts)
{
    EECode err;
    TTime video_dts = 0, video_pts = 0, audio_dts = 0, audio_native_dts = 0;
    TU64 offset = 0;

    if (mbAudioEnabled && mbVideoEnabled) {
        if (mCurVideoFrameIndex < mTotalVideoFrameCountMinus1) {
            getVideoTimestamp(mCurVideoFrameIndex, video_dts, video_pts);
            if (mCurAudioFrameIndex < mTotalAudioFrameCountMinus1) {
                getAudioTimestamp(mCurAudioFrameIndex, audio_dts, audio_native_dts);
                if (audio_native_dts < video_dts) {
                    type = StreamType_Audio;
                    queryAudioOffsetSize(offset, size);
                    dts = audio_dts;
                    pts = audio_native_dts;
                } else {
                    type = StreamType_Video;
                    queryVideoOffsetSize(offset, size);
                    dts = video_dts;
                    pts = video_pts;
                }
            } else {
                type = StreamType_Video;
                queryVideoOffsetSize(offset, size);
                dts = video_dts;
                pts = video_pts;
            }
        } else if (mCurAudioFrameIndex < mTotalAudioFrameCountMinus1) {
            getAudioTimestamp(mCurAudioFrameIndex, dts, pts);
            err = queryAudioOffsetSize(offset, size);
            type = StreamType_Audio;
            return EECode_OK;
        } else {
            size = 0;
            type = StreamType_Invalid;
            LOGM_WARN("return eof here 1\n");
            return EECode_OK_EOF;
        }

    } else if (mbVideoEnabled) {
        err = queryVideoOffsetSize(offset, size);
        if (DLikely(EECode_OK == err)) {
            type = StreamType_Video;
            getVideoTimestamp(mCurVideoFrameIndex, dts, pts);
        } else if (EECode_OK_EOF == err) {
            size = 0;
            type = StreamType_Invalid;
            LOGM_WARN("return eof here 2\n");
            return EECode_OK_EOF;
        } else {
            LOGM_ERROR("fail here, video\n");
            return err;
        }
    } else if (mbAudioEnabled) {
        err = queryAudioOffsetSize(offset, size);
        if (DLikely(EECode_OK == err)) {
            type = StreamType_Audio;
            getAudioTimestamp(mCurAudioFrameIndex, dts, pts);
        } else if (EECode_OK_EOF == err) {
            size = 0;
            type = StreamType_Invalid;
            LOGM_WARN("return eof here 3\n");
            return EECode_OK_EOF;
        } else {
            LOGM_ERROR("fail here, audio\n");
            return err;
        }
    } else {
        LOGM_FATAL("no audio and no video\n");
        return EECode_BadState;
    }

    return EECode_OK;
}

EECode CMP4Demuxer::nextVideoKeyFrameInfo(TU32 &size, TTime &dts, TTime &pts)
{
    TU32 max_framecount = mTotalVideoFrameCountMinus1 + 1;

    if (DUnlikely(mCurVideoFrameIndex > mTotalVideoFrameCountMinus1)) {
        LOGM_NOTICE("read video eof\n");
        return EECode_OK_EOF;
    }

    while (mCurVideoFrameIndex < max_framecount) {
        size = mpVideoSampleTableBox->sample_size.entry_size[mCurVideoFrameIndex];
        if (size > mGuessedMinKeyFrameSize) {
            getVideoTimestamp(mCurVideoFrameIndex, dts, pts);
            return EECode_OK;
        }
        mCurVideoFrameIndex ++;
    }
    LOGM_NOTICE("read video eof\n");
    return EECode_OK_EOF;
}

EECode CMP4Demuxer::previousVideoKeyFrameInfo(TU32 &size, TTime &dts, TTime &pts)
{
    if (DUnlikely(!mCurVideoFrameIndex)) {
        LOGM_NOTICE("read video eof\n");
        return EECode_OK_EOF;
    }

    while (mCurVideoFrameIndex) {
        size = mpVideoSampleTableBox->sample_size.entry_size[mCurVideoFrameIndex];
        if (size > mGuessedMinKeyFrameSize) {
            getVideoTimestamp(mCurVideoFrameIndex, dts, pts);
            return EECode_OK;
        }
        mCurVideoFrameIndex --;
    }

    if (!mCurVideoFrameIndex) {
        size = mpVideoSampleTableBox->sample_size.entry_size[0];
        getVideoTimestamp(0, dts, pts);
        return EECode_OK;
    }

    LOGM_WARN("read video eof\n");
    return EECode_OK_EOF;
}

EECode CMP4Demuxer::updateVideoExtraData()
{
    TU32 len = 0;
    TU8 *p = NULL;
    TU8 start_code_prefix[4] = {0x0, 0x0, 0x0, 0x1};

    if (mpOutputPins[EDemuxerVideoOutputPinIndex] && mbHaveVideo) {

        if (StreamFormat_H264 == mVideoFormat) {
            SISOMAVCDecoderConfigurationRecordBox *avc_config = &mMovieBox.video_track.media.media_info.sample_table.visual_sample_description.visual_entry.avc_config;
            len = avc_config->sequenceParametersSetLength + avc_config->pictureParametersSetLength + 4 + 4;

            if (mpVideoExtraData && (mVideoExtraDataBufferLength < len)) {
                DDBG_FREE(mpVideoExtraData, "M4VE");
                mpVideoExtraData = NULL;
            }

            if (!mpVideoExtraData) {
                mVideoExtraDataBufferLength = len;
                mpVideoExtraData = (TU8 *) DDBG_MALLOC(mVideoExtraDataBufferLength, "M4VE");
                if (DUnlikely(!mpVideoExtraData)) {
                    LOGM_FATAL("no memory, %d\n", mVideoExtraDataBufferLength);
                    return EECode_NoMemory;
                }
            }

            p = mpVideoExtraData;
            memcpy(p, start_code_prefix, 4);
            p += 4;
            memcpy(p, avc_config->p_sps, avc_config->sequenceParametersSetLength);
            p += avc_config->sequenceParametersSetLength;
            memcpy(p, start_code_prefix, 4);
            p += 4;
            memcpy(p, avc_config->p_pps, avc_config->pictureParametersSetLength);
            mVideoExtraDataLength = len;
        } else if (StreamFormat_H265 == mVideoFormat) {
            SISOMHEVCDecoderConfigurationRecordBox *hevc_config = &mMovieBox.video_track.media.media_info.sample_table.visual_sample_description.visual_entry.hevc_config;
            len = hevc_config->vps_length + hevc_config->sps_length + hevc_config->pps_length + 4 + 4 + 4;

            if (mpVideoExtraData && (mVideoExtraDataBufferLength < len)) {
                DDBG_FREE(mpVideoExtraData, "M4VE");
                mpVideoExtraData = NULL;
            }

            if (!mpVideoExtraData) {
                mVideoExtraDataBufferLength = len;
                mpVideoExtraData = (TU8 *) DDBG_MALLOC(mVideoExtraDataBufferLength, "M4VE");
                if (DUnlikely(!mpVideoExtraData)) {
                    LOGM_FATAL("no memory, %d\n", mVideoExtraDataBufferLength);
                    return EECode_NoMemory;
                }
            }

            p = mpVideoExtraData;
            memcpy(p, start_code_prefix, 4);
            p += 4;
            memcpy(p, hevc_config->vps, hevc_config->vps_length);
            p += hevc_config->vps_length;
            memcpy(p, start_code_prefix, 4);
            p += 4;
            memcpy(p, hevc_config->sps, hevc_config->sps_length);
            p += hevc_config->sps_length;
            memcpy(p, start_code_prefix, 4);
            p += 4;
            memcpy(p, hevc_config->pps, hevc_config->pps_length);
            p += hevc_config->pps_length;
            mVideoExtraDataLength = len;
        } else {
            LOGM_FATAL("bad video format %d\n", mVideoFormat);
            return EECode_BadState;
        }

        mbVideoEnabled = 1;
    }

    return EECode_OK;
}

EECode CMP4Demuxer::updateAudioExtraData()
{
    if (mpOutputPins[EDemuxerAudioOutputPinIndex] && mbHaveAudio) {

        if (StreamFormat_AAC == mAudioFormat) {
            SISOMAACElementaryStreamDescriptor *aac_config = &mMovieBox.audio_track.media.media_info.sample_table.audio_sample_description.audio_entry.esd;

            if (mpAudioExtraData && (mAudioExtraDataBufferLength < aac_config->strange_size_2)) {
                DDBG_FREE(mpAudioExtraData, "M4AE");
                mpAudioExtraData = NULL;
            }

            if (!mpAudioExtraData) {
                mAudioExtraDataBufferLength = aac_config->strange_size_2;
                mpAudioExtraData = (TU8 *) DDBG_MALLOC(mAudioExtraDataBufferLength, "M4AE");
                if (DUnlikely(!mpAudioExtraData)) {
                    LOGM_FATAL("no memory, %d\n", mAudioExtraDataBufferLength);
                    return EECode_NoMemory;
                }
            }

            mAudioExtraDataLength = 2;
            mpAudioExtraData[0] = aac_config->config[0];
            mpAudioExtraData[1] = aac_config->config[1];

            mbAudioEnabled = 1;
        } else {
            LOGM_FATAL("bad audio format %d\n", mVideoFormat);
            return EECode_BadState;
        }
    }

    return EECode_OK;
}

EECode CMP4Demuxer::sendExtraData()
{
    if (mpOutputPins[EDemuxerVideoOutputPinIndex] && mbVideoEnabled && mpVideoExtraData && mVideoExtraDataLength) {
        CIBuffer *buffer = NULL;
        mpOutputPins[EDemuxerVideoOutputPinIndex]->AllocBuffer(buffer);

        buffer->SetDataPtr(mpVideoExtraData);
        buffer->SetDataSize(mVideoExtraDataLength);
        buffer->SetBufferType(EBufferType_VideoExtraData);
        buffer->SetBufferFlags(CIBuffer::SYNC_POINT);

        buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
        buffer->mpCustomizedContent = NULL;

        buffer->mVideoOffsetX = 0;
        buffer->mVideoOffsetY = 0;
        buffer->mVideoWidth = mMovieBox.video_track.media.media_info.sample_table.visual_sample_description.visual_entry.width;
        buffer->mVideoHeight = mMovieBox.video_track.media.media_info.sample_table.visual_sample_description.visual_entry.height;
        buffer->mVideoBitrate = 1000000;//hard code here
        buffer->mContentFormat = mVideoFormat;
        buffer->mCustomizedContentFormat = 0;

        buffer->mVideoFrameRateNum = 90000;//hard code here
        buffer->mVideoFrameRateDen = 3003;

        if (buffer->mVideoFrameRateNum && buffer->mVideoFrameRateDen) {
            buffer->mVideoRoughFrameRate = (float)buffer->mVideoFrameRateNum / (float)buffer->mVideoFrameRateDen;
        }

        buffer->mVideoSampleAspectRatioNum = 1;
        buffer->mVideoSampleAspectRatioDen = 1;

        LOGM_NOTICE("before send video extra data, %dx%d, rough framerate %f\n", buffer->mVideoWidth, buffer->mVideoHeight, buffer->mVideoRoughFrameRate);
        mpOutputPins[EDemuxerVideoOutputPinIndex]->SendBuffer(buffer);
        buffer = NULL;
    } else {
        LOGM_WARN("video disabled\n");
    }

    if (mpOutputPins[EDemuxerAudioOutputPinIndex] && mbAudioEnabled && mpAudioExtraData && mAudioExtraDataLength) {
        CIBuffer *buffer = NULL;
        mpOutputPins[EDemuxerAudioOutputPinIndex]->AllocBuffer(buffer);

        buffer->SetDataPtr(mpAudioExtraData);
        buffer->SetDataSize(mAudioExtraDataLength);
        buffer->SetBufferType(EBufferType_AudioExtraData);
        buffer->SetBufferFlags(CIBuffer::SYNC_POINT);

        buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
        buffer->mpCustomizedContent = NULL;

        buffer->mAudioChannelNumber = mMovieBox.audio_track.media.media_info.sample_table.audio_sample_description.audio_entry.channelcount;
        buffer->mAudioSampleRate = mMovieBox.audio_track.media.media_info.sample_table.audio_sample_description.audio_entry.samplerate;
        buffer->mAudioFrameSize = mMovieBox.audio_track.media.media_info.sample_table.audio_sample_description.audio_entry.samplesize;
        buffer->mAudioSampleFormat = AudioSampleFMT_S16;

        buffer->mAudioBitrate = 64000;

        buffer->mbAudioPCMChannelInterlave = 0;
        buffer->mpCustomizedContent = NULL;
        buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
        buffer->mContentFormat = mAudioFormat;
        buffer->mCustomizedContentFormat = 0;

        LOGM_NOTICE("before send audio extra data, channel %d, sample rate %d\n", buffer->mAudioChannelNumber, buffer->mAudioSampleRate);
        mpOutputPins[EDemuxerAudioOutputPinIndex]->SendBuffer(buffer);
        buffer = NULL;
    } else {
        mbAudioEnabled = 0;
        LOGM_WARN("audio disabled\n");
    }

    return EECode_OK;
}

EECode CMP4Demuxer::processCmd(SCMD &cmd)
{
    switch (cmd.code) {
        case ECMDType_ExitRunning:
            mbRun = 0;
            break;

        case ECMDType_Start:
            DASSERT(EModuleState_WaitCmd == msState);
            msState = EModuleState_Idle;
            CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            msState = EModuleState_Pending;
            CmdAck(EECode_OK);
            break;

        case ECMDType_Pause:
            mbPaused = 1;
            msState = EModuleState_Pending;
            break;

        case ECMDType_Resume:
            if (EModuleState_Pending == msState) {
                msState = EModuleState_Idle;
            } else if (EModuleState_Completed == msState) {
                msState = EModuleState_Idle;
            } else if (EModuleState_Stopped == msState) {
                msState = EModuleState_Idle;
            }
            mbPaused = 0;
            break;

        case ECMDType_Flush:
            msState = EModuleState_Pending;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_ResumeFlush:
            msState = EModuleState_Idle;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_EnableVideo: {
                if ((mbVideoEnabled && cmd.res32_1) || (!mbVideoEnabled && !cmd.res32_1)) {
                    LOGM_NOTICE("video enable(%d) not changed\n", mbVideoEnabled);
                } else if (mbVideoEnabled && !cmd.res32_1) {
                    if (!mbAudioEnabled) {
                        LOGM_NOTICE("audio also disabled, reset all\n");
                        mCurVideoFrameIndex = 0;
                        mCurAudioFrameIndex = 0;
                    } else {
                        LOGM_NOTICE("video disabled\n");
                    }
                    mbVideoEnabled = 0;
                } else {
                    if (mbAudioEnabled) {
                        LOGM_NOTICE("audio enabled, sync video to audio\n");
                        TTime dts, native_dts;
                        getAudioTimestamp(mCurAudioFrameIndex, dts, native_dts);
                        mCurVideoFrameIndex = timeToVideoFrameIndex(native_dts);
                    } else {
                        LOGM_NOTICE("video enabled\n");
                    }
                    mbVideoEnabled = 1;
                }
            }
            break;

        case ECMDType_EnableAudio: {
                if ((mbAudioEnabled && cmd.res32_1) || (!mbAudioEnabled && !cmd.res32_1)) {
                    LOGM_NOTICE("audio enable(%d) not changed\n", mbAudioEnabled);
                } else if (mbAudioEnabled && !cmd.res32_1) {
                    if (!mbVideoEnabled) {
                        LOGM_NOTICE("video also disabled, reset all\n");
                        mCurVideoFrameIndex = 0;
                        mCurAudioFrameIndex = 0;
                    } else {
                        LOGM_NOTICE("audio disabled\n");
                    }
                    mbAudioEnabled = 0;
                } else {
                    if (mbVideoEnabled) {
                        LOGM_NOTICE("video enabled, sync audio to video\n");
                        TTime dts, pts;
                        getVideoTimestamp(mCurVideoFrameIndex, dts, pts);
                        mCurAudioFrameIndex = timeToAudioFrameIndex(dts);
                    } else {
                        LOGM_NOTICE("audio enabled\n");
                    }
                    mbAudioEnabled = 1;
                }
            }
            break;

        case ECMDType_Seek: {
                SInternalCMDSeek *p_seek = (SInternalCMDSeek *) cmd.pExtra;
                TTime dts = 0;

                if (ENavigationPosition_Begining == p_seek->position) {
                    if (mbHaveVideo) {
                        dts = p_seek->target * (mVideoTimeScale) / 1000;
                        mCurVideoFrameIndex = timeToVideoFrameIndex(dts);
                        LOGM_NOTICE("seek video: target %lld ms, dts %lld, frame index %d\n", p_seek->target, dts, mCurVideoFrameIndex);
                    }
                    if (mbHaveAudio) {
                        dts = p_seek->target * (mAudioTimeScale) / 1000;
                        mCurAudioFrameIndex = timeToAudioFrameIndex(dts);
                        LOGM_NOTICE("seek audio: target %lld ms, dts %lld, frame index %d\n", p_seek->target, dts, mCurAudioFrameIndex);
                    }
                } else if (ENavigationPosition_End == p_seek->position) {
                    if (mbHaveVideo) {
                        mCurVideoFrameIndex = mTotalVideoFrameCountMinus1;
                        LOGM_NOTICE("seek video to end %d\n", mCurVideoFrameIndex);
                    }
                    if (mbHaveAudio) {
                        mCurAudioFrameIndex = mTotalAudioFrameCountMinus1;
                        LOGM_NOTICE("seek audio to end %d\n", mCurAudioFrameIndex);
                    }
                } else {
                    LOGM_FATAL("bad position %d\n", p_seek->position);
                }
            }
            CmdAck(EECode_OK);
            break;

        case ECMDType_NotifyBufferRelease:
            if (mpCurBufferPool && mpCurBufferPool->GetFreeBufferCnt() > 0 && msState == EModuleState_HasInputData) {
                msState = EModuleState_Running;
            }
            break;

        case ECMDType_UpdatePlaybackSpeed: {
                mbBackward = cmd.res32_1;
                mFeedingRule = cmd.res64_1;
            }
            break;

        case ECMDType_UpdatePlaybackLoopMode:
            break;

        case ECMDType_ResumeChannel:
            msState = EModuleState_Idle;
            CmdAck(EECode_OK);
            break;

        default:
            LOG_WARN("not processed cmd %x\n", cmd.code);
            break;
    }

    return EECode_OK;
}

EECode CMP4Demuxer::buildTimeTree()
{
    TU32 i = 0, j = 0;
    TU32 frame_index = 0;
    TTime time = 0;
    SMp4DTimeTree *dt = NULL;
    _STimeEntry *entry = NULL;
    TU32 *pv = NULL;

    if ((!mpVideoDTimeTree) || (mnVideoDTimeTreeNodeBufferCount < mpVideoSampleTableBox->stts.entry_count)) {
        if (mpVideoDTimeTree) {
            DDBG_FREE(mpVideoDTimeTree, "M4VD");
            mpVideoDTimeTree = NULL;
        }

        mnVideoDTimeTreeNodeBufferCount = mpVideoSampleTableBox->stts.entry_count;
        mpVideoDTimeTree = (SMp4DTimeTree *) DDBG_MALLOC(mnVideoDTimeTreeNodeBufferCount * sizeof(SMp4DTimeTree), "M4VD");
        if (!mpVideoDTimeTree) {
            LOGM_FATAL("no memory\n");
            return EECode_NoMemory;
        }
    }

    if ((!mpAudioDTimeTree) || (mnAudioDTimeTreeNodeBufferCount < mpAudioSampleTableBox->stts.entry_count)) {
        if (mpAudioDTimeTree) {
            DDBG_FREE(mpAudioDTimeTree, "M4AD");
            mpAudioDTimeTree = NULL;
        }

        mnAudioDTimeTreeNodeBufferCount = mpAudioSampleTableBox->stts.entry_count;
        mpAudioDTimeTree = (SMp4DTimeTree *) DDBG_MALLOC(mnAudioDTimeTreeNodeBufferCount * sizeof(SMp4DTimeTree), "M4AD");
        if (!mpAudioDTimeTree) {
            LOGM_FATAL("no memory\n");
            return EECode_NoMemory;
        }
    }

    mnVideoDTimeTreeNodeCount = mpVideoSampleTableBox->stts.entry_count;
    entry = mpVideoSampleTableBox->stts.p_entry;
    time = 0;
    frame_index = 0;
    for (i = 0, dt = mpVideoDTimeTree; i < mnVideoDTimeTreeNodeCount; i ++, dt ++) {
        dt->base_time = time;
        dt->frame_start_index = frame_index;
        dt->frame_tick = entry[i].sample_delta;
        dt->frame_count = entry[i].sample_count;

        time += dt->frame_tick * dt->frame_count;
        frame_index += entry[i].sample_count;
    }

    mnAudioDTimeTreeNodeCount = mpAudioSampleTableBox->stts.entry_count;
    entry = mpAudioSampleTableBox->stts.p_entry;
    time = 0;
    frame_index = 0;
    for (i = 0, dt = mpAudioDTimeTree; i < mnAudioDTimeTreeNodeCount; i ++, dt ++) {
        dt->base_time = time;
        dt->frame_start_index = frame_index;
        dt->frame_tick = entry[i].sample_delta;
        dt->frame_count = entry[i].sample_count;

        time += dt->frame_tick * dt->frame_count;
        frame_index += entry[i].sample_count;
    }

    if (mbHaveCTTS) {
        if ((!mpVideoCTimeIndexMap) || (mnVideoCTimeIndexMapBufferCount < mpVideoSampleTableBox->sample_size.sample_count)) {
            if (mpVideoCTimeIndexMap) {
                DDBG_FREE(mpVideoCTimeIndexMap, "M4VC");
                mpVideoCTimeIndexMap = NULL;
            }

            mnVideoCTimeIndexMapBufferCount = mpVideoSampleTableBox->stts.entry_count;
            mpVideoCTimeIndexMap = (TU32 *) DDBG_MALLOC(mnVideoCTimeIndexMapBufferCount * sizeof(TU32), "M4VC");
            if (!mpVideoCTimeIndexMap) {
                LOGM_FATAL("no memory\n");
                return EECode_NoMemory;
            }
        }

        mnVideoCTimeIndexMapCount = mpVideoSampleTableBox->sample_size.sample_count;

        pv = mpVideoCTimeIndexMap;
        for (i = 0; i < mpVideoSampleTableBox->ctts.entry_count; i ++) {
            for (j = 0; j < mpVideoSampleTableBox->ctts.p_entry[i].sample_count; j ++) {
                pv[0] = mpVideoSampleTableBox->ctts.p_entry[i].sample_delta;
                pv ++;
            }
        }
    }

    return EECode_OK;
}

TU32 CMP4Demuxer::findVideoTimeIndex(TU32 index)
{
    TU32 i = 0;
    for (i = 0; i < mnVideoDTimeTreeNodeCount; i ++) {
        if ((mpVideoDTimeTree[i].frame_start_index + mpVideoDTimeTree[i].frame_count) > index) {
            return i;
        }
    }

    LOGM_FATAL("bug\n");
    return 0;
}

EECode CMP4Demuxer::getVideoTimestamp(TU32 index, TTime &dts, TTime &pts)
{
    if (mpVideoDTimeTree) {
        TU32 start_index = mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].frame_start_index;
        TU32 count = mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].frame_count;
        if ((start_index <= index) && ((start_index + count) > index)) {
            dts = mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].base_time + mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].frame_tick * (index - start_index);
        } else if (start_index > index) {
            if (mLastVideoDTimeTreeNodeIndex) {
                mLastVideoDTimeTreeNodeIndex --;
            } else {
                mLastVideoDTimeTreeNodeIndex = 0;
            }
            start_index = mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].frame_start_index;
            count = mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].frame_count;
            if ((start_index <= index) && ((start_index + count) > index)) {
                dts = mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].base_time + mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].frame_tick * (index - start_index);
            } else {
                mLastVideoDTimeTreeNodeIndex = findVideoTimeIndex(index);
                start_index = mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].frame_start_index;
                dts = mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].base_time + mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].frame_tick * (index - start_index);
            }
        } else if ((start_index + count) <= index) {
            if ((mLastVideoDTimeTreeNodeIndex + 1) < mnVideoDTimeTreeNodeCount) {
                mLastVideoDTimeTreeNodeIndex ++;
            } else {
                mLastVideoDTimeTreeNodeIndex = 0;
            }
            start_index = mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].frame_start_index;
            count = mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].frame_count;
            if ((start_index <= index) && ((start_index + count) > index)) {
                dts = mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].base_time + mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].frame_tick * (index - start_index);
            } else {
                mLastVideoDTimeTreeNodeIndex = findVideoTimeIndex(index);
                start_index = mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].frame_start_index;
                dts = mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].base_time + mpVideoDTimeTree[mLastVideoDTimeTreeNodeIndex].frame_tick * (index - start_index);
            }
        }

        if (!mbHaveCTTS) {
            pts = dts;
        } else {
            pts = dts + mpVideoCTimeIndexMap[index];
        }
    } else {
        dts = pts = index * DDefaultVideoFramerateDen;
    }

    return EECode_OK;
}

EECode CMP4Demuxer::getAudioTimestamp(TU32 index, TTime &dts, TTime &native_dts)
{
    if (mpAudioDTimeTree) {
        TU32 start_index = mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].frame_start_index;
        TU32 count = mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].frame_count;
        if ((start_index <= index) && ((start_index + count) > index)) {
            dts = mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].base_time + mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].frame_tick * (index - start_index);
        } else if (start_index > index) {
            if (mLastAudioDTimeTreeNodeIndex) {
                mLastAudioDTimeTreeNodeIndex --;
            } else {
                mLastAudioDTimeTreeNodeIndex = 0;
            }
            start_index = mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].frame_start_index;
            count = mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].frame_count;
            if ((start_index <= index) && ((start_index + count) > index)) {
                dts = mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].base_time + mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].frame_tick * (index - start_index);
            } else {
                mLastAudioDTimeTreeNodeIndex = findVideoTimeIndex(index);
                dts = mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].base_time + mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].frame_tick * (index - start_index);
            }
        } else if ((start_index + count) <= index) {
            if ((mLastAudioDTimeTreeNodeIndex + 1) < mnAudioDTimeTreeNodeCount) {
                mLastAudioDTimeTreeNodeIndex ++;
            } else {
                mLastAudioDTimeTreeNodeIndex = 0;
            }
            start_index = mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].frame_start_index;
            count = mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].frame_count;
            if ((start_index <= index) && ((start_index + count) > index)) {
                dts = mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].base_time + mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].frame_tick * (index - start_index);
            } else {
                mLastAudioDTimeTreeNodeIndex = findVideoTimeIndex(index);
                dts = mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].base_time + mpAudioDTimeTree[mLastAudioDTimeTreeNodeIndex].frame_tick * (index - start_index);
            }
        }

    } else {
        dts = index * DDefaultAudioFrameSize;
    }

    native_dts = (dts * 90000) / 48000;

    return EECode_OK;
}

EECode CMP4Demuxer::buildSampleOffset()
{
    TU32 i = 0, j = 0, k = 0;
    TU32 chunks = 0;
    TU32 frame_index = 0;
    TU32 chunk_index = 0;
    TU32 chunk_offset = 0, ac_offset_in_chunk = 0;
    TU32 remain_frames = 0, samples_per_chunk = 0;;

    if ((!mbVideoOneSamplePerChunk) && mpVideoSampleTableBox) {

        if ((!mpVideoSampleOffset) || (mnVideoSampleOffsetBufferCount < (mTotalVideoFrameCountMinus1 + 1))) {
            if (mpVideoSampleOffset) {
                DDBG_FREE(mpVideoSampleOffset, "M4VS");
                mpVideoSampleOffset = NULL;
            }
        }
        if (!mpVideoSampleOffset) {
            mnVideoSampleOffsetBufferCount = (mTotalVideoFrameCountMinus1 + 1);
            mpVideoSampleOffset = (TU32 *) DDBG_MALLOC(mnVideoSampleOffsetBufferCount * sizeof(TU32), "M4VS");
            if (DUnlikely(!mpVideoSampleOffset)) {
                LOGM_FATAL("No memory\n");
                return EECode_NoMemory;
            }
        }

        frame_index = 0;
        chunk_index = 0;
        for (i = 0; i < (mpVideoSampleTableBox->sample_to_chunk.entry_count - 1); i ++) {
            chunks = mpVideoSampleTableBox->sample_to_chunk.entrys[i + 1].first_chunk - mpVideoSampleTableBox->sample_to_chunk.entrys[i].first_chunk;
            samples_per_chunk = mpVideoSampleTableBox->sample_to_chunk.entrys[i].sample_per_chunk;
            LOGM_NOTICE("%d's entry, chunks %d, sample per chunk %d, first_chunk %d, next first_chunk %d\n", i, chunks, samples_per_chunk, mpVideoSampleTableBox->sample_to_chunk.entrys[i].first_chunk, mpVideoSampleTableBox->sample_to_chunk.entrys[i + 1].first_chunk);
            for (j = 0; j < chunks; j ++) {
                chunk_offset = mpVideoSampleTableBox->chunk_offset.chunk_offset[chunk_index];
                ac_offset_in_chunk = 0;
                for (k = 0; k < samples_per_chunk; k ++) {
                    mpVideoSampleOffset[frame_index] = chunk_offset + ac_offset_in_chunk;
                    ac_offset_in_chunk += mpVideoSampleTableBox->sample_size.entry_size[frame_index];
                    frame_index ++;
                }
                chunk_index ++;
            }
        }

        samples_per_chunk = mpVideoSampleTableBox->sample_to_chunk.entrys[i].sample_per_chunk;
        remain_frames = mTotalVideoFrameCountMinus1 + 1 - frame_index;
        chunks = remain_frames / samples_per_chunk;
        if (DUnlikely(remain_frames != (samples_per_chunk * chunks))) {
            LOGM_ERROR("last chunk error, %d, %d, %d?\n", remain_frames, samples_per_chunk, chunks);
            return EECode_ParseError;
        }

        for (j = 0; j < chunks; j ++) {
            chunk_offset = mpVideoSampleTableBox->chunk_offset.chunk_offset[chunk_index];
            ac_offset_in_chunk = 0;
            for (k = 0; k < samples_per_chunk; k ++) {
                mpVideoSampleOffset[frame_index] = chunk_offset + ac_offset_in_chunk;
                ac_offset_in_chunk += mpVideoSampleTableBox->sample_size.entry_size[frame_index];
                frame_index ++;
            }
            chunk_index ++;
        }

    }

    if ((!mbAudioOneSamplePerChunk) && mpAudioSampleTableBox) {

        if ((!mpAudioSampleOffset) || (mnAudioSampleOffsetBufferCount < (mTotalAudioFrameCountMinus1 + 1))) {
            if (mpAudioSampleOffset) {
                DDBG_FREE(mpAudioSampleOffset, "M4AS");
                mpAudioSampleOffset = NULL;
            }
        }
        if (!mpAudioSampleOffset) {
            mnAudioSampleOffsetBufferCount = (mTotalAudioFrameCountMinus1 + 1);
            mpAudioSampleOffset = (TU32 *) DDBG_MALLOC(mnAudioSampleOffsetBufferCount * sizeof(TU32), "M4AS");
            if (DUnlikely(!mpAudioSampleOffset)) {
                LOGM_FATAL("No memory\n");
                return EECode_NoMemory;
            }
        }

        frame_index = 0;
        chunk_index = 0;
        for (i = 0; i < (mpAudioSampleTableBox->sample_to_chunk.entry_count - 1); i ++) {
            chunks = mpAudioSampleTableBox->sample_to_chunk.entrys[i + 1].first_chunk - mpAudioSampleTableBox->sample_to_chunk.entrys[i].first_chunk;
            samples_per_chunk = mpAudioSampleTableBox->sample_to_chunk.entrys[i].sample_per_chunk;
            for (j = 0; j < chunks; j ++) {
                chunk_offset = mpAudioSampleTableBox->chunk_offset.chunk_offset[chunk_index];
                ac_offset_in_chunk = 0;
                for (k = 0; k < samples_per_chunk; k ++) {
                    mpAudioSampleOffset[frame_index] = chunk_offset + ac_offset_in_chunk;
                    ac_offset_in_chunk += mpAudioSampleTableBox->sample_size.entry_size[frame_index];
                    frame_index ++;
                }
                chunk_index ++;
            }
        }

        samples_per_chunk = mpAudioSampleTableBox->sample_to_chunk.entrys[i].sample_per_chunk;
        remain_frames = mTotalAudioFrameCountMinus1 + 1 - frame_index;
        chunks = remain_frames / samples_per_chunk;
        if (DUnlikely(remain_frames != (samples_per_chunk * chunks))) {
            LOGM_ERROR("last chunk error, %d, %d, %d?\n", remain_frames, samples_per_chunk, chunks);
            return EECode_ParseError;
        }

        for (j = 0; j < chunks; j ++) {
            chunk_offset = mpAudioSampleTableBox->chunk_offset.chunk_offset[chunk_index];
            ac_offset_in_chunk = 0;
            for (k = 0; k < samples_per_chunk; k ++) {
                mpAudioSampleOffset[frame_index] = chunk_offset + ac_offset_in_chunk;
                ac_offset_in_chunk += mpAudioSampleTableBox->sample_size.entry_size[frame_index];
                frame_index ++;
            }
            chunk_index ++;
        }

    }

    return EECode_OK;
}

TU32 CMP4Demuxer::timeToVideoFrameIndex(TTime dts)
{
    TU32 i = 0, offset = 0;

    if (mpVideoDTimeTree) {
        for (i = mnVideoDTimeTreeNodeCount - 1; i > 0; i --) {
            if (mpVideoDTimeTree[i].base_time <= dts) {
                offset = (dts - mpVideoDTimeTree[i].base_time) / mVideoFrameTick;
                return (offset + mpVideoDTimeTree[i].frame_start_index);
            }
        }
    }
    return (dts / mVideoFrameTick);
}

TU32 CMP4Demuxer::timeToAudioFrameIndex(TTime dts)
{
    TU32 i = 0, offset = 0;

    if (mpAudioDTimeTree) {
        for (i = mnAudioDTimeTreeNodeCount - 1; i > 0; i --) {
            if (mpAudioDTimeTree[i].base_time <= dts) {
                offset = (dts - mpAudioDTimeTree[i].base_time) / mAudioFrameTick;
                return (offset + mpAudioDTimeTree[i].frame_start_index);
            }
        }
    }
    return (dts) / mAudioFrameTick;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END
