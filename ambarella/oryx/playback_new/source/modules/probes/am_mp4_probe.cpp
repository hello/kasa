/**
 * am_mp4_probe.cpp
 *
 * History:
 *    2016/03/09 - [Zhi He] create file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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
 */

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"
#include "am_codec_interface.h"

#include "iso_base_media_file_defs.h"

#include "am_mp4_probe.h"

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
    TU32 next_size = 0; \
    TChar next_box_name[8] = {0}; \
    __check_demuxer_box_fail(); \
    DBER32(next_size, p); \
    memcpy(next_box_name, p + 4, 4); \
    LOG_FATAL("box size check fail, expect size %d, handled size %d. next box size %d, name %s\n", p_box->base_box.size, (TU32) (p - p_check_base), next_size, next_box_name); \
    return 0; \
  }

#define DCHECK_DEMUXER_FULL_BOX_END \
  if ((p_check_base + p_box->base_full_box.base_box.size) != (p)) { \
    TU32 next_size = 0; \
    TChar next_box_name[8] = {0}; \
    __check_demuxer_box_fail(); \
    DBER32(next_size, p); \
    memcpy(next_box_name, p + 4, 4); \
    LOG_FATAL("box size check fail, expect size %d, handled size %d. next box size %d, name %s\n", p_box->base_full_box.base_box.size, (TU32) (p - p_check_base), next_size, next_box_name); \
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

static void __convert_to_annexb(TU8 *p_buf, TU32 size)
{
  TU32 cur_size = 0;
  TU8 *p = p_buf, * p_next = NULL;
  if ((!p[0]) && (!p[1]) && ((!p[2]) && (0x01 == p[3]))) {
    return;
  }
  while (size) {
    DBER32(cur_size, p);
    if ((cur_size + 4) > size) {
      LOG_ERROR("data corruption\n");
      break;
    }
    p_next = p + cur_size + 4;
    size -= cur_size + 4;
    p[0] = 0x0;
    p[1] = 0x0;
    p[2] = 0x0;
    p[3] = 0x1;
    p = p_next;
  }
  return;
}

IMediaProbe *gfCreateMP4ProbeModule()
{
  return CMP4Probe::Create();
}

IMediaProbe *CMP4Probe::Create()
{
  CMP4Probe *result = new CMP4Probe();
  if (result && result->Construct() != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

CMP4Probe::CMP4Probe()
  : mpVideoSampleTableBox(NULL)
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
  mLastVideoDTimeTreeNodeIndex = 0;
  mLastAudioDTimeTreeNodeIndex = 0;
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
  mpCacheBufferBaseAligned = NULL;
  mpCacheBufferEnd = NULL;
  mCacheBufferSize = 0;
  mpIO = NULL;
  mCurFileOffset = 0;
  mFileTotalSize = 0;
  mpMutex = NULL;
  mpBitsBufferList = NULL;
  mpAudioTrackCachePtr = NULL;
  mAudioTrackCacheRemaingDataSize = 0;
  mCurFileAudioOffset = 0;
  mbBackward = 0;
  mFeedingRule = DecoderFeedingRule_AllFrames;
  mDebugFlag = 0;
  mGuessedMinKeyFrameSize = 256;
  memset(&mFileTypeBox, 0x0, sizeof(mFileTypeBox));
  memset(&mMediaDataBox, 0x0, sizeof(mMediaDataBox));
  memset(&mMovieBox, 0x0, sizeof(mMovieBox));
  mVideoWidthFromExtradata = 0;
  mVideoHeightFromExtradata = 0;
}

EECode CMP4Probe::Construct()
{
  mCacheBufferSize = 2 * 1024 * 1024;
  mpCacheBufferBase = (TU8 *) DDBG_MALLOC(mCacheBufferSize + 31, "M4CH");
  mpCacheBufferBaseAligned = (TU8 *)((((TULong)mpCacheBufferBase) + 31) & (~0x1f));
  if (!mpCacheBufferBase) {
    LOG_FATAL("No Memory! request size %" DPrint64u "\n", mCacheBufferSize + 31);
    return EECode_NoMemory;
  }
  mpBitsBufferList = new CIDoubleLinkedList();
  mpMutex = gfCreateMutex();
  return EECode_OK;
}

void CMP4Probe::Delete()
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
  if (mMovieBox.audio_track.media.media_info.sample_table.chunk_offset64.chunk_offset) {
    DDBG_FREE(mMovieBox.audio_track.media.media_info.sample_table.chunk_offset64.chunk_offset, "M4AC");
    mMovieBox.audio_track.media.media_info.sample_table.chunk_offset64.chunk_offset = NULL;
    mMovieBox.audio_track.media.media_info.sample_table.chunk_offset64.entry_buf_count = 0;
    mMovieBox.audio_track.media.media_info.sample_table.chunk_offset64.entry_count = 0;
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
  if (mpBitsBufferList) {
    SGenericDataBuffer *buffer = NULL;
    CIDoubleLinkedList::SNode *p_node = mpBitsBufferList->FirstNode();
    while (p_node) {
      buffer = (SGenericDataBuffer *) p_node->p_context;
      if (DLikely(buffer)) {
        if (buffer->p_buffer) {
          DDBG_FREE(buffer->p_buffer, "M4BB");
          buffer->p_buffer = NULL;
        }
      } else {
        LOG_FATAL("internal logic bug\n");
      }
      p_node = mpBitsBufferList->NextNode(p_node);
    }
    delete mpBitsBufferList;
    mpBitsBufferList = NULL;
  }
  delete this;
}

CMP4Probe::~CMP4Probe()
{
}

EECode CMP4Probe::Open(TChar *filename, SMediaProbedInfo *info)
{
  EECode err = EECode_OK;
  if (filename && info) {
    err = setupContext(filename);
    if (DLikely(EECode_OK == err)) {
      LOG_CONFIGURATION("video resolution from track_header: %dx%d\n", mMovieBox.video_track.track_header.width, mMovieBox.video_track.track_header.height);
      LOG_CONFIGURATION("video resolution from extradata: %dx%d\n", mVideoWidthFromExtradata, mVideoHeightFromExtradata);
      info->info.video_width = mVideoWidthFromExtradata;
      info->info.video_height = mVideoHeightFromExtradata;
      info->info.video_framerate_num = TimeUnitDen_90khz;
      info->info.video_framerate_den = mVideoFrameTick;
      info->info.video_format = mVideoFormat;
      info->info.audio_channel_number = mAudioChannelNumber;
      info->info.audio_sample_rate = mAudioSampleRate;
      info->info.audio_frame_size = 0;
      info->info.format = mAudioFormat;
      info->p_video_extradata = mpVideoExtraData;
      info->video_extradata_len = mVideoExtraDataLength;
    } else {
      LOG_ERROR("NULL params\n");
      return EECode_BadParam;
    }
  } else {
    LOG_ERROR("NULL params\n");
    return EECode_BadParam;
  }
  return EECode_OK;
}

EECode CMP4Probe::Close()
{
  destroyContext();
  return EECode_OK;
}

SGenericDataBuffer *CMP4Probe::GetKeyFrame(TU32 key_frame_index)
{
  TU32 size = 0;
  TU64 offset = 0;
  EECode err = EECode_OK;
  SGenericDataBuffer *buffer = NULL;
  mCurVideoFrameIndex = 0;
  err = getVideoOffsetSize(offset, size);
  if (DUnlikely(EECode_OK != err)) {
    LOG_ERROR("get frame(%d)'s offset size fail\n", mCurVideoFrameIndex);
    return NULL;
  }
  buffer = allocBitsBuffer(size);
  if (DUnlikely(!buffer)) {
    LOG_ERROR("allocBitsBuffer fail, size %d\n", size);
    return NULL;
  }
  copyFileData(buffer->p_buffer, offset, size);
  buffer->data_size = size;
  __convert_to_annexb(buffer->p_buffer, size);
  return buffer;
}

SGenericDataBuffer *CMP4Probe::GetFrame(TU32 frame_index)
{
  LOG_ERROR("to do\n");
  return NULL;
}

void CMP4Probe::ReleaseFrame(SGenericDataBuffer *frame)
{
  releaseBitBuffer(frame);
  return;
}

EECode CMP4Probe::setupContext(TChar *url)
{
  EECode err;
  TIOSize total_size = 0, cur_pos = 0;
  if (DUnlikely(!url)) {
    LOG_ERROR("NULL url\n");
    return EECode_BadParam;
  }
  if (DLikely(!mpIO)) {
    mpIO = gfCreateIO(EIOType_File);
  } else {
    LOG_WARN("mpIO is created yet?\n");
  }
  err = mpIO->Open(url, EIOFlagBit_Read | EIOFlagBit_Binary);
  if (DUnlikely(EECode_OK != err)) {
    LOG_ERROR("open url (%s) fail, ret 0x%08x, %s\n", url, err, gfGetErrorCodeString(err));
    return err;
  }
  err = mpIO->Query(total_size, cur_pos);
  if (DUnlikely(EECode_OK != err)) {
    LOG_ERROR("query (%s) fail, ret 0x%08x, %s\n", url, err, gfGetErrorCodeString(err));
    return err;
  }
  mFileTotalSize = total_size;
  DASSERT(0 == cur_pos);
  err = parseFile();
  if (DUnlikely(EECode_OK != err)) {
    LOG_ERROR("parseFile (%s) fail, ret %x, %s\n", url, err, gfGetErrorCodeString(err));
    return err;
  }
  return EECode_OK;
}

EECode CMP4Probe::destroyContext()
{
  if (DLikely(mpIO)) {
    mpIO->Delete();
    mpIO = NULL;
  } else {
    LOG_WARN("mpIO is not created?\n");
  }
  return EECode_OK;
}

SGenericDataBuffer *CMP4Probe::allocBitsBuffer(TU32 max_buffer_size)
{
  AUTO_LOCK(mpMutex);
  SGenericDataBuffer *buffer = NULL;
  CIDoubleLinkedList::SNode *p_node = mpBitsBufferList->FirstNode();
  if (p_node) {
    buffer = (SGenericDataBuffer *) p_node->p_context;
    if (DLikely(buffer)) {
      if (buffer->buffer_size < max_buffer_size) {
        if (buffer->p_buffer) {
          DDBG_FREE(buffer->p_buffer, "M4BB");
          buffer->p_buffer = NULL;
        }
        buffer->buffer_size = 0;
      }
      if (!buffer->p_buffer) {
        buffer->p_buffer = (TU8 *) DDBG_MALLOC(max_buffer_size, "M4BB");
        if (DUnlikely(!buffer->p_buffer)) {
          LOG_FATAL("no memory, request size %d\n", max_buffer_size);
          return NULL;
        }
        buffer->buffer_size = max_buffer_size;
      }
      mpBitsBufferList->RemoveContent((void *) buffer);
      return buffer;
    } else {
      LOG_FATAL("internal logic bug\n");
      return NULL;
    }
  } else {
    buffer = (SGenericDataBuffer *) DDBG_MALLOC(sizeof(SGenericDataBuffer), "M4Bb");
    if (DUnlikely(!buffer)) {
      LOG_FATAL("no memory, request size %ld\n", (TULong)sizeof(SGenericDataBuffer));
      return NULL;
    }
    buffer->p_buffer = (TU8 *) DDBG_MALLOC(max_buffer_size, "M4BB");
    if (DUnlikely(!buffer->p_buffer)) {
      LOG_FATAL("no memory, request size %d\n", max_buffer_size);
      DDBG_FREE(buffer, "M4BB");
      return NULL;
    }
    buffer->buffer_size = max_buffer_size;
  }
  return buffer;
}

void CMP4Probe::releaseBitBuffer(SGenericDataBuffer *buffer)
{
  AUTO_LOCK(mpMutex);
  mpBitsBufferList->InsertContent(NULL, (void *) buffer, 0);
}

TU32 CMP4Probe::readBaseBox(SISOMBox *p_box, TU8 *p)
{
  DBER32(p_box->size, p);
  memcpy(p_box->type, p + 4, DISOM_TAG_SIZE);
  return DISOM_BOX_SIZE;
}

TU32 CMP4Probe::readFullBox(SISOMFullBox *p_box, TU8 *p)
{
  DBER32(p_box->base_box.size, p);
  memcpy(p_box->base_box.type, p + 4, DISOM_TAG_SIZE);
  p_box->flags = p[8];
  p += 9;
  DBER24(p_box->flags, p);
  return DISOM_FULL_BOX_SIZE;
}

TU32 CMP4Probe::readFileTypeBox(SISOMFileTypeBox *p_box, TU8 *p)
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
    LOG_ERROR("read file type box fail\n");
    return 0;
  }
  DCHECK_DEMUXER_BOX_END
  return p_box->base_box.size;
}

TU32 CMP4Probe::handleMediaDataBox(SISOMMediaDataBox *p_box, TU8 *p)
{
  p += readBaseBox(&p_box->base_box, p);
  return p_box->base_box.size;
}

TU32 CMP4Probe::readMovieHeaderBox(SISOMMovieHeaderBox *p_box, TU8 *p)
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

TU32 CMP4Probe::readMovieTrackHeaderBox(SISOMTrackHeader *p_box, TU8 *p)
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
  if ((0 == (p_box->width & 0xffff)) && (p_box->width & 0xffff0000)) {
    p_box->width = p_box->width >> 16;
  }
  p += 4;
  DBER32(p_box->height, p);
  if ((0 == (p_box->height & 0xffff)) && (p_box->height & 0xffff0000)) {
    p_box->height = p_box->height >> 16;
  }
  p += 4;
  DCHECK_DEMUXER_FULL_BOX_END
  return p_box->base_full_box.base_box.size;
}

TU32 CMP4Probe::readMediaHeaderBox(SISOMMediaHeaderBox *p_box, TU8 *p)
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

TU32 CMP4Probe::readHanderReferenceBox(SISOMHandlerReferenceBox *p_box, TU8 *p)
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
      LOG_FATAL("no memory\n");
      return 0;
    }
  } else {
    LOG_FATAL("too long %d\n", v);
    return 0;
  }
  p ++;
  DCHECK_DEMUXER_FULL_BOX_END
  return p_box->base_full_box.base_box.size;
}

TU32 CMP4Probe::readChunkOffsetBox(SISOMChunkOffsetBox *p_box, TU8 *p)
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
    LOG_FATAL("no memory\n");
    return 0;
  }
  DCHECK_DEMUXER_FULL_BOX_END
  return p_box->base_full_box.base_box.size;
}

TU32 CMP4Probe::readChunkOffset64Box(SISOMChunkOffset64Box *p_box, TU8 *p)
{
  DCHECK_DEMUXER_BOX_BEGIN;
  p += readFullBox(&p_box->base_full_box, p);
  DBER32(p_box->entry_count, p);
  p += 4;
  p_box->chunk_offset = (TU64 *) DDBG_MALLOC(p_box->entry_count * 8, "MPCO");
  if (p_box->chunk_offset) {
    for (TU32 i = 0; i < p_box->entry_count; i ++) {
      DBER64(p_box->chunk_offset[i], p);
      p += 8;
    }
  } else {
    LOG_FATAL("no memory\n");
    return 0;
  }
  DCHECK_DEMUXER_FULL_BOX_END
  return p_box->base_full_box.base_box.size;
}

TU32 CMP4Probe::readSampleToChunkBox(SISOMSampleToChunkBox *p_box, TU8 *p)
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
    LOG_FATAL("no memory\n");
    return 0;
  }
  DCHECK_DEMUXER_FULL_BOX_END
  return p_box->base_full_box.base_box.size;
}

TU32 CMP4Probe::readSampleSizeBox(SISOMSampleSizeBox *p_box, TU8 *p)
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
    LOG_FATAL("no memory\n");
    return 0;
  }
  DCHECK_DEMUXER_FULL_BOX_END
  return p_box->base_full_box.base_box.size;
}

TU32 CMP4Probe::readAVCDecoderConfigurationRecordBox(SISOMAVCDecoderConfigurationRecordBox *p_box, TU8 *p)
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
    LOG_FATAL("no memory, %d\n", p_box->sequenceParametersSetLength);
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
    LOG_FATAL("no memory, %d\n", p_box->pictureParametersSetLength);
    return 0;
  }
  memcpy(p_box->p_pps, p, p_box->pictureParametersSetLength);
  p += p_box->pictureParametersSetLength;
  DCHECK_DEMUXER_BOX_END
  return p_box->base_box.size;
}

TU32 CMP4Probe::readHEVCDecoderConfigurationRecordBox(SISOMHEVCDecoderConfigurationRecordBox *p_box, TU8 *p)
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
    LOG_FATAL("not vps\n");
    return 0;
  }
  p += 3;
  DBER16(p_box->vps_length, p);
  p += 2;
  p_box->vps = (TU8 *) DDBG_MALLOC(p_box->vps_length, "M4HV");
  if (!p_box->vps) {
    LOG_FATAL("no memory, %d\n", p_box->vps_length);
    return 0;
  }
  memcpy(p_box->vps, p, p_box->vps_length);
  p += p_box->vps_length;
  if ((0x80 | EHEVCNalType_SPS) != p[0]) {
    LOG_FATAL("not sps\n");
    return 0;
  }
  p += 3;
  DBER16(p_box->sps_length, p);
  p += 2;
  p_box->sps = (TU8 *) DDBG_MALLOC(p_box->sps_length, "M4HS");
  if (!p_box->sps) {
    LOG_FATAL("no memory, %d\n", p_box->sps_length);
    return 0;
  }
  memcpy(p_box->sps, p, p_box->sps_length);
  p += p_box->sps_length;
  if ((0x80 | EHEVCNalType_PPS) != p[0]) {
    LOG_FATAL("not pps\n");
    return 0;
  }
  p += 3;
  DBER16(p_box->pps_length, p);
  p += 2;
  p_box->pps = (TU8 *) DDBG_MALLOC(p_box->pps_length, "M4HP");
  if (!p_box->pps) {
    LOG_FATAL("no memory, %d\n", p_box->pps_length);
    return 0;
  }
  memcpy(p_box->pps, p, p_box->pps_length);
  p += p_box->pps_length;
  DCHECK_DEMUXER_BOX_END
  return p_box->base_box.size;
}

TU32 CMP4Probe::readVisualSampleDescriptionBox(SISOMVisualSampleDescriptionBox *p_box, TU8 *p)
{
  TU32 read_size;
  TU8 *p_ori = p;
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
      LOG_ERROR("read avcc fail\n");
      return 0;
    }
    p += read_size;
    mVideoFormat = StreamFormat_H264;
  } else if (!memcmp(p + 4, DISOM_HEVC_DECODER_CONFIGURATION_RECORD_TAG, 4)) {
    read_size = readHEVCDecoderConfigurationRecordBox(&p_box->visual_entry.hevc_config, p);
    if (DUnlikely(!read_size)) {
      LOG_ERROR("read hvcc fail\n");
      return 0;
    }
    p += read_size;
    mVideoFormat = StreamFormat_H265;
  } else {
    TChar tag[8] = {0};
    memcpy(tag, p + 4, 4);
    LOG_FATAL("not supported: %s\n", tag);
    return 0;
  }
  if ((p_ori + p_box->base_full_box.base_box.size) > p) {
    p += gfIOSMSkipBox(p);
  }
  DCHECK_DEMUXER_FULL_BOX_END
  return p_box->base_full_box.base_box.size;
}

TU32 CMP4Probe::readAACElementaryStreamDescriptionBox(SISOMAACElementaryStreamDescriptor *p_box, TU8 *p)
{
  //DCHECK_DEMUXER_BOX_BEGIN;
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
    LOG_WARN("aac config len(%d) is not 2\n", p_box->strange_size_2);
    //return 0;
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
  //DCHECK_DEMUXER_FULL_BOX_END
  return p_box->base_full_box.base_box.size;
}

TU32 CMP4Probe::readAudioSampleDescriptionBox(SISOMAudioSampleDescriptionBox *p_box, TU8 *p)
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
  p_box->audio_entry.samplerate = p_box->audio_entry.samplerate >> 16;
  p += 4;
  if (DUnlikely(512000 < p_box->audio_entry.samplerate)) {
    LOG_WARN("bad samplerate %d, 0x%08x, set it to default %d\n", p_box->audio_entry.samplerate, p_box->audio_entry.samplerate, DDefaultAudioSampleRate);
    p_box->audio_entry.samplerate = DDefaultAudioSampleRate;
  }
  if (!memcmp(p + 4, DISOM_ELEMENTARY_STREAM_DESCRIPTOR_BOX_TAG, 4)) {
    TU32 read_size = 0;
    read_size = readAACElementaryStreamDescriptionBox(&p_box->audio_entry.esd, p);
    if (DUnlikely(!read_size)) {
      LOG_ERROR("read esd fail\n");
      return 0;
    }
    p += read_size;
    mAudioFormat = StreamFormat_AAC;
  }  else {
    TChar tag[8] = {0};
    memcpy(tag, p + 4, 4);
    LOG_FATAL("not supported: %s\n", tag);
    return 0;
  }
  DCHECK_DEMUXER_FULL_BOX_END
  return p_box->base_full_box.base_box.size;
}

TU32 CMP4Probe::readDecodingTimeToSampleBox(SISOMDecodingTimeToSampleBox *p_box, TU8 *p)
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
    LOG_FATAL("no memory, %d\n", p_box->entry_count);
    return 0;
  }
  DCHECK_DEMUXER_FULL_BOX_END
  return p_box->base_full_box.base_box.size;
}

TU32 CMP4Probe::readVideoSampleTableBox(SISOMSampleTableBox *p_box, TU8 *p)
{
  TU32 read_size = 0;
  TU32 remain_size = 0;
  DCHECK_DEMUXER_BOX_BEGIN;
  p += readBaseBox(&p_box->base_box, p);
  if (p_box->base_box.size < DISOM_BOX_SIZE) {
    LOG_FATAL("size not expected, data corruption\n");
    return 0;
  }
  remain_size = p_box->base_box.size - DISOM_BOX_SIZE;
  while (remain_size) {
    DBER32(read_size, p);
    if (DUnlikely(read_size > remain_size)) {
      LOG_ERROR("data corruption, %d, %d\n", read_size, remain_size);
      return 0;
    }
    remain_size -= read_size;
    if (!memcmp(p + 4, DISOM_SAMPLE_DESCRIPTION_BOX_TAG, 4)) {
      read_size = readVisualSampleDescriptionBox(&p_box->visual_sample_description, p);
      if (DUnlikely(!read_size)) {
        LOG_ERROR("read video sample desc box fail\n");
        return 0;
      }
      p += read_size;
    } else if (!memcmp(p + 4, DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG, 4)) {
      read_size = readDecodingTimeToSampleBox(&p_box->stts, p);
      if (DUnlikely(!read_size)) {
        LOG_ERROR("read decoding time to sample box fail\n");
        return 0;
      }
      p += read_size;
    } else if (!memcmp(p + 4, DISOM_SAMPLE_SIZE_BOX_TAG, 4)) {
      read_size = readSampleSizeBox(&p_box->sample_size, p);
      if (DUnlikely(!read_size)) {
        LOG_ERROR("read sample size box fail\n");
        return 0;
      }
      p += read_size;
    } else if (!memcmp(p + 4, DISOM_SAMPLE_TO_CHUNK_BOX_TAG, 4)) {
      read_size = readSampleToChunkBox(&p_box->sample_to_chunk, p);
      if (DUnlikely(!read_size)) {
        LOG_ERROR("read sample to chunk box fail\n");
        return 0;
      }
      p += read_size;
    } else if (!memcmp(p + 4, DISOM_CHUNK_OFFSET_BOX_TAG, 4)) {
      read_size = readChunkOffsetBox(&p_box->chunk_offset, p);
      if (DUnlikely(!read_size)) {
        LOG_ERROR("read chunk offset box fail\n");
        return 0;
      }
      p_box->b_use_offset64 = 0;
      p += read_size;
    } else if (!memcmp(p + 4, DISOM_CHUNK_OFFSET64_BOX_TAG, 4)) {
      read_size = readChunkOffset64Box(&p_box->chunk_offset64, p);
      if (DUnlikely(!read_size)) {
        LOG_ERROR("read chunk offset box 64 fail\n");
        return 0;
      }
      p_box->b_use_offset64 = 1;
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

TU32 CMP4Probe::readSoundSampleTableBox(SISOMSampleTableBox *p_box, TU8 *p)
{
  TU32 read_size = 0;
  TU32 remain_size = 0;
  DCHECK_DEMUXER_BOX_BEGIN;
  p += readBaseBox(&p_box->base_box, p);
  if (p_box->base_box.size < DISOM_BOX_SIZE) {
    LOG_FATAL("size not expected, data corruption\n");
    return 0;
  }
  remain_size = p_box->base_box.size - DISOM_BOX_SIZE;
  while (remain_size) {
    DBER32(read_size, p);
    if (DUnlikely(read_size > remain_size)) {
      LOG_ERROR("data corruption, %d, %d\n", read_size, remain_size);
      return 0;
    }
    remain_size -= read_size;
    if (!memcmp(p + 4, DISOM_SAMPLE_DESCRIPTION_BOX_TAG, 4)) {
      read_size = readAudioSampleDescriptionBox(&p_box->audio_sample_description, p);
      if (DUnlikely(!read_size)) {
        LOG_ERROR("read audio sample desc box fail\n");
        return 0;
      }
      p += read_size;
    } else if (!memcmp(p + 4, DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG, 4)) {
      read_size = readDecodingTimeToSampleBox(&p_box->stts, p);
      if (DUnlikely(!read_size)) {
        LOG_ERROR("read decoding time to sample box fail\n");
        return 0;
      }
      p += read_size;
    } else if (!memcmp(p + 4, DISOM_SAMPLE_SIZE_BOX_TAG, 4)) {
      read_size = readSampleSizeBox(&p_box->sample_size, p);
      if (DUnlikely(!read_size)) {
        LOG_ERROR("read sample size box fail\n");
        return 0;
      }
      p += read_size;
    } else if (!memcmp(p + 4, DISOM_SAMPLE_TO_CHUNK_BOX_TAG, 4)) {
      read_size = readSampleToChunkBox(&p_box->sample_to_chunk, p);
      if (DUnlikely(!read_size)) {
        LOG_ERROR("read sample to chunk box fail\n");
        return 0;
      }
      p += read_size;
    } else if (!memcmp(p + 4, DISOM_CHUNK_OFFSET_BOX_TAG, 4)) {
      read_size = readChunkOffsetBox(&p_box->chunk_offset, p);
      if (DUnlikely(!read_size)) {
        LOG_ERROR("read chunk offset box fail\n");
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

TU32 CMP4Probe::readDataReferenceBox(SISOMDataReferenceBox *p_box, TU8 *p)
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

TU32 CMP4Probe::readDataInformationBox(SISOMDataInformationBox *p_box, TU8 *p)
{
  DCHECK_DEMUXER_BOX_BEGIN;
  p += readBaseBox(&p_box->base_box, p);
  p += readDataReferenceBox(&p_box->data_ref, p);
  DCHECK_DEMUXER_BOX_END
  return p_box->base_box.size;
}

TU32 CMP4Probe::readVideoMediaHeaderBox(SISOMVideoMediaHeaderBox *p_box, TU8 *p)
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

TU32 CMP4Probe::readSoundMediaHeaderBox(SISOMSoundMediaHeaderBox *p_box, TU8 *p)
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

TU32 CMP4Probe::readVideoMediaInformationBox(SISOMMediaInformationBox *p_box, TU8 *p)
{
  TU32 read_size = 0;
  DCHECK_DEMUXER_BOX_BEGIN;
  p += readBaseBox(&p_box->base_box, p);
  if (DIS_NOT_BOX_TAG(p, DISOM_VIDEO_MEDIA_HEADER_BOX_TAG)) {
    LOG_ERROR("not video media header box\n");
    return 0;
  }
  read_size = readVideoMediaHeaderBox(&p_box->video_header, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read video media header box fail\n");
    return 0;
  }
  p += read_size;
  if (DIS_NOT_BOX_TAG(p, DISOM_DATA_INFORMATION_BOX_TAG)) {
    LOG_ERROR("not data information box\n");
    return 0;
  }
  read_size = readDataInformationBox(&p_box->data_info, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read data information box fail\n");
    return 0;
  }
  p += read_size;
  if (DIS_NOT_BOX_TAG(p, DISOM_SAMPLE_TABLE_BOX_TAG)) {
    LOG_ERROR("not video sample table box\n");
    return 0;
  }
  read_size = readVideoSampleTableBox(&p_box->sample_table, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read video sample table box fail\n");
    return 0;
  }
  p += read_size;
  DCHECK_DEMUXER_BOX_END
  return p_box->base_box.size;
}

TU32 CMP4Probe::readAudioMediaInformationBox(SISOMMediaInformationBox *p_box, TU8 *p)
{
  TU32 read_size = 0;
  DCHECK_DEMUXER_BOX_BEGIN;
  p += readBaseBox(&p_box->base_box, p);
  if (DIS_NOT_BOX_TAG(p, DISOM_SOUND_MEDIA_HEADER_BOX_TAG)) {
    LOG_ERROR("not sound media header box\n");
    return 0;
  }
  read_size = readSoundMediaHeaderBox(&p_box->sound_header, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read sound media header box fail\n");
    return 0;
  }
  p += read_size;
  if (DIS_NOT_BOX_TAG(p, DISOM_DATA_INFORMATION_BOX_TAG)) {
    LOG_ERROR("not data information box\n");
    return 0;
  }
  read_size = readDataInformationBox(&p_box->data_info, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read data information box fail\n");
    return 0;
  }
  p += read_size;
  if (DIS_NOT_BOX_TAG(p, DISOM_SAMPLE_TABLE_BOX_TAG)) {
    LOG_ERROR("not sound sample table box\n");
    return 0;
  }
  read_size = readSoundSampleTableBox(&p_box->sample_table, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read sound sample table box fail\n");
    return 0;
  }
  p += read_size;
  DCHECK_DEMUXER_BOX_END
  return p_box->base_box.size;
}

TU32 CMP4Probe::readVideoMediaBox(SISOMMediaBox *p_box, TU8 *p)
{
  TU32 read_size = 0;
  DCHECK_DEMUXER_BOX_BEGIN;
  p += readBaseBox(&p_box->base_box, p);
  if (DIS_NOT_BOX_TAG(p, DISOM_MEDIA_HEADER_BOX_TAG)) {
    LOG_ERROR("not media header box\n");
    return 0;
  }
  read_size = readMediaHeaderBox(&p_box->media_header, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read media header box fail\n");
    return 0;
  }
  p += read_size;
  if (DIS_NOT_BOX_TAG(p, DISOM_HANDLER_REFERENCE_BOX_TAG)) {
    LOG_ERROR("not handler ref box\n");
    return 0;
  }
  read_size = readHanderReferenceBox(&p_box->media_handler, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read handler ref box fail\n");
    return 0;
  }
  p += read_size;
  if (DIS_NOT_BOX_TAG(p, DISOM_MEDIA_INFORMATION_BOX_TAG)) {
    LOG_ERROR("not media info box\n");
    return 0;
  }
  read_size = readVideoMediaInformationBox(&p_box->media_info, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read video media info box fail\n");
    return 0;
  }
  p += read_size;
  DCHECK_DEMUXER_BOX_END
  return p_box->base_box.size;
}

TU32 CMP4Probe::readAudioMediaBox(SISOMMediaBox *p_box, TU8 *p)
{
  TU32 read_size = 0;
  DCHECK_DEMUXER_BOX_BEGIN;
  p += readBaseBox(&p_box->base_box, p);
  if (DIS_NOT_BOX_TAG(p, DISOM_MEDIA_HEADER_BOX_TAG)) {
    LOG_ERROR("not media header box\n");
    return 0;
  }
  read_size = readMediaHeaderBox(&p_box->media_header, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read media header box fail\n");
    return 0;
  }
  p += read_size;
  if (DIS_NOT_BOX_TAG(p, DISOM_HANDLER_REFERENCE_BOX_TAG)) {
    LOG_ERROR("not handler ref box\n");
    return 0;
  }
  read_size = readHanderReferenceBox(&p_box->media_handler, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read handler ref box fail\n");
    return 0;
  }
  p += read_size;
  if (DIS_NOT_BOX_TAG(p, DISOM_MEDIA_INFORMATION_BOX_TAG)) {
    LOG_ERROR("not media info box\n");
    return 0;
  }
  read_size = readAudioMediaInformationBox(&p_box->media_info, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read audio media info box fail\n");
    return 0;
  }
  p += read_size;
  DCHECK_DEMUXER_BOX_END
  return p_box->base_box.size;
}

TU32 CMP4Probe::readMovieVideoTrackBox(SISOMTrackBox *p_box, TU8 *p)
{
  TU32 read_size = 0;
  TU8 *p_ori = p;
  DCHECK_DEMUXER_BOX_BEGIN;
  p += readBaseBox(&p_box->base_box, p);
  if (DIS_NOT_BOX_TAG(p, DISOM_TRACK_HEADER_BOX_TAG)) {
    LOG_ERROR("not track header box\n");
    return 0;
  }
  read_size = readMovieTrackHeaderBox(&p_box->track_header, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read movie track header box fail\n");
    return 0;
  }
  p += read_size;
  if (DIS_NOT_BOX_TAG(p, DISOM_MEDIA_BOX_TAG)) {
    LOG_ERROR("not track media box\n");
    return 0;
  }
  read_size = readVideoMediaBox(&p_box->media, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read movie media box fail\n");
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

TU32 CMP4Probe::readMovieAudioTrackBox(SISOMTrackBox *p_box, TU8 *p)
{
  TU32 read_size = 0;
  TU8 *p_ori = p;
  DCHECK_DEMUXER_BOX_BEGIN;
  p += readBaseBox(&p_box->base_box, p);
  if (DIS_NOT_BOX_TAG(p, DISOM_TRACK_HEADER_BOX_TAG)) {
    LOG_ERROR("not track header box\n");
    return 0;
  }
  read_size = readMovieTrackHeaderBox(&p_box->track_header, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read movie track header box fail\n");
    return 0;
  }
  p += read_size;
  if (DIS_NOT_BOX_TAG(p, DISOM_MEDIA_BOX_TAG)) {
    LOG_ERROR("not track media box\n");
    return 0;
  }
  read_size = readAudioMediaBox(&p_box->media, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read movie media box fail\n");
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

TU32 CMP4Probe::readMovieBox(SISOMMovieBox *p_box, TU8 *p)
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
    LOG_FATAL("not movie header box, %s\n", tag);
    return 0;
  }
  read_size = readMovieHeaderBox(&p_box->movie_header_box, p);
  if (DUnlikely(!read_size)) {
    LOG_ERROR("read movie header box fail\n");
    return 0;
  }
  p += read_size;
  if (p_box->base_box.size <= read_size) {
    LOG_ERROR("bad movie box size, %d, header size %d\n", p_box->base_box.size, read_size);
    return 0;
  }
  if (p_box->base_box.size < (read_size + DISOM_BOX_SIZE)) {
    LOG_FATAL("size not expected, data corruption\n");
    return 0;
  }
  remain_size = p_box->base_box.size - read_size - DISOM_BOX_SIZE;
  while (remain_size) {
    DBER32(read_size, p);
    if (read_size > remain_size) {
      LOG_ERROR("bad track size %d, %d\n", read_size, remain_size);
      return 0;
    }
    if (DIS_NOT_BOX_TAG(p, DISOM_TRACK_BOX_TAG)) {
      TChar tag[8] = {0};
      memcpy(tag, p + 4, 4);
      LOG_DEBUG("skip tag %s, size %d\n", tag, read_size);
      p += read_size;
      remain_size -= read_size;
      continue;
    }
    err = __fast_get_handler_type(p, handler_tag, remain_size);
    if (DUnlikely(EECode_OK != err)) {
      LOG_ERROR("get handler type fail\n");
      return 0;
    }
    remain_size -= read_size;
    if (!memcmp(handler_tag, DISOM_VIDEO_HANDLER_REFERENCE_TAG, 4)) {
      read_size = readMovieVideoTrackBox(&p_box->video_track, p);
      if (DUnlikely(!read_size)) {
        LOG_ERROR("read movie video track box fail\n");
        return 0;
      }
      p += read_size;
      mbHaveVideo = 1;
      track_index ++;
    } else if (!memcmp(handler_tag, DISOM_AUDIO_HANDLER_REFERENCE_TAG, 4)) {
      read_size = readMovieAudioTrackBox(&p_box->audio_track, p);
      if (DUnlikely(!read_size)) {
        LOG_ERROR("read movie audio track box fail\n");
        return 0;
      }
      p += read_size;
      mbHaveAudio = 1;
      track_index ++;
    } else if (!memcmp(handler_tag, DISOM_HINT_HANDLER_REFERENCE_TAG, 4)) {
      DBER32(read_size, p);
      p += read_size;
    } else {
      LOG_ERROR("get handler type fail\n");
      return 0;
    }
  }
  DCHECK_DEMUXER_BOX_END
  return p_box->base_box.size;
}

TU32 CMP4Probe::readStrangeSize(TU8 *p)
{
  return ((((TU32) p[0]) & 0x7f) << 21) | ((((TU32) p[1]) & 0x7f) << 14) | ((((TU32) p[2]) & 0x7f) << 7) | (((TU32) p[3]) & 0x7f);
}

TU32 CMP4Probe::isFileHeaderValid()
{
  if (strncmp(DISOM_BRAND_V0_TAG, mFileTypeBox.major_brand, DISOM_TAG_SIZE)) {
    return 1;
  }
  if (strncmp(DISOM_BRAND_V1_TAG, mFileTypeBox.major_brand, DISOM_TAG_SIZE)) {
    return 1;
  }
  return 0;
}

EECode CMP4Probe::parseFile()
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
    LOG_FATAL("file size (%" DPrint64u ") too small, it's not possible\n", mFileTotalSize);
    return EECode_BadFormat;
  }
  tsize = read_size;
  err = mpIO->Read(mpCacheBufferBaseAligned, 1, tsize);
  if (DUnlikely(EECode_OK != err) || (read_size != tsize)) {
    LOG_FATAL("read first %" DPrint64u " bytes fail\n", tsize);
    return EECode_IOError;
  }
  p = mpCacheBufferBaseAligned;
  if (DIS_NOT_BOX_TAG(p, DISOM_FILE_TYPE_BOX_TAG)) {
    TChar tag[8] = {0};
    memcpy(tag, p + 4, 4);
    LOG_FATAL("not file type box, %s\n", tag);
    return EECode_Error;
  }
  read_size = readFileTypeBox(&mFileTypeBox, p);
  if (DUnlikely(!read_size)) {
    LOG_FATAL("read file type box fail, corrupted file header\n");
    return EECode_IOError;
  } else {
    LOG_NOTICE("file type: %c%c%c%c\n", mFileTypeBox.major_brand[0], mFileTypeBox.major_brand[1], mFileTypeBox.major_brand[2], mFileTypeBox.major_brand[3]);
    if (DUnlikely(0 == isFileHeaderValid())) {
      LOG_FATAL("not valid file type\n");
      return EECode_Error;
    }
  }
  p += read_size;
  cur_file_offset = read_size;
  mbHaveAudio = 0;
  mbHaveVideo = 0;
  while (1) {
    if (!DIS_NOT_BOX_TAG(p, DISOM_MEDIA_DATA_BOX_TAG)) {
      if (DUnlikely(DMAX_MEDIA_DATA_BOX_NUMBER <= mdbox_index)) {
        LOG_FATAL("too many media data boxes, %d\n", mdbox_index);
        return EECode_BadFormat;
      }
      read_size = handleMediaDataBox(&mMediaDataBox[mdbox_index], p);
      if (DUnlikely(!read_size)) {
        LOG_FATAL("read media data box fail, corrupted file header\n");
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
        LOG_ERROR("file seek error\n");
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
          LOG_FATAL("No Memory! request size %" DPrint64u "\n", mCacheBufferSize + 31);
          return EECode_NoMemory;
        }
        mpCacheBufferBaseAligned = (TU8 *)((((TULong)mpCacheBufferBase) + 31) & (~0x1f));
      }
      tsize = read_size;
      err = mpIO->Read(mpCacheBufferBaseAligned, 1, tsize);
      if (DUnlikely(EECode_OK != err) || (read_size != tsize)) {
        LOG_FATAL("read movie box %" DPrint64u " bytes fail\n", tsize);
        return EECode_IOError;
      }
      p = mpCacheBufferBaseAligned;
      read_size = readMovieBox(&mMovieBox, p);
      if (DUnlikely((!read_size) || (read_size != mMovieBox.base_box.size))) {
        LOG_FATAL("movie box read fail, %d, %d\n", read_size, mMovieBox.base_box.size);
        return EECode_BadFormat;
      }
      cur_file_offset += mMovieBox.base_box.size;
      if (mbHaveVideo) {
        mpVideoSampleTableBox = &mMovieBox.video_track.media.media_info.sample_table;
        if ((1 == mpVideoSampleTableBox->sample_to_chunk.entry_count) \
            && (mpVideoSampleTableBox->sample_to_chunk.entrys) \
            && (1 == mpVideoSampleTableBox->sample_to_chunk.entrys[0].first_chunk) \
            && (1 == mpVideoSampleTableBox->sample_to_chunk.entrys[0].sample_per_chunk) \
            && (1 == mpVideoSampleTableBox->sample_to_chunk.entrys[0].sample_description_index)) {
          mbVideoOneSamplePerChunk = 1;
        } else {
          mbVideoOneSamplePerChunk = 0;
        }
        if (mpVideoSampleTableBox->sample_size.sample_count) {
          mTotalVideoFrameCountMinus1 = mpVideoSampleTableBox->sample_size.sample_count - 1;
        } else {
          mTotalVideoFrameCountMinus1 = 0;
          mbHaveVideo = 0;
          LOG_WARN("zero video sampe count?\n");
        }
      } else {
        mpVideoSampleTableBox = NULL;
      }
      if (mbHaveAudio) {
        mpAudioSampleTableBox = &mMovieBox.audio_track.media.media_info.sample_table;
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
          if (mpAudioSampleTableBox->sample_size.sample_count) {
            mTotalAudioFrameCountMinus1 = mpAudioSampleTableBox->sample_size.sample_count - 1;
          } else {
            mTotalAudioFrameCountMinus1 = 0;
            mbHaveAudio = 0;
            LOG_WARN("zero audio sampe count?\n");
          }
        } else {
          LOG_FATAL("todo\n");
          return EECode_NotSupported;
        }
      } else {
        mpAudioSampleTableBox = NULL;
      }
      has_movie_box = 1;
    } else {
      TChar tag[8] = {0};
      memcpy(tag, p + 4, 4);
      LOG_WARN("not media data or movie box, %s, skip\n", tag);
      DBER32(read_size, p);
      //p += read_size;
      cur_file_offset += read_size;
      //if (cur_file_offset >= mFileTotalSize) {
      //    break;
      //}
      //continue;
    }
    if (cur_file_offset >= mFileTotalSize) {
      break;
    }
    err = mpIO->Seek((TIOSize) cur_file_offset, EIOPosition_Begin);
    if (DUnlikely(EECode_OK != err)) {
      LOG_ERROR("file seek error\n");
      return EECode_IOError;
    }
    if ((cur_file_offset + DFILE_TRY_READ_HEADER_SIZE) <= mFileTotalSize) {
      tsize = DFILE_TRY_READ_HEADER_SIZE;
    } else {
      tsize = mFileTotalSize - cur_file_offset;
    }
    err = mpIO->Read(mpCacheBufferBaseAligned, 1, tsize);
    if (DUnlikely(EECode_OK != err)) {
      LOG_FATAL("read first %" DPrint64u " bytes fail\n", tsize);
      return EECode_IOError;
    }
    p = mpCacheBufferBaseAligned;
  }
  if ((!has_movie_box) || (!has_mediadata_box)) {
    LOG_FATAL("no media data or movie box\n");
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
      LOG_WARN("no video time scale? use default\n");
    }
    if (1 == mpVideoSampleTableBox->stts.entry_count) {
      mVideoFrameTick = mpVideoSampleTableBox->stts.p_entry[0].sample_delta;
    } else {
      mVideoFrameTick = mpVideoSampleTableBox->stts.p_entry[0].sample_delta;
    }
    mGuessedMinKeyFrameSize = mpVideoSampleTableBox->sample_size.entry_size[0] * 3 / 8;
    LOG_INFO("video timescale %d, frame tick %d, min size %d\n", mVideoTimeScale, mVideoFrameTick, mGuessedMinKeyFrameSize);
  }
  if (mbHaveAudio) {
    if (mMovieBox.audio_track.media.media_header.timescale) {
      mAudioTimeScale = mMovieBox.audio_track.media.media_header.timescale;
    } else if (mMovieBox.movie_header_box.timescale) {
      mAudioTimeScale = mMovieBox.movie_header_box.timescale;
    } else {
      mVideoTimeScale = DDefaultAudioSampleRate;
      LOG_WARN("no audio time scale? use default\n");
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
    LOG_INFO("audio timescale %d, frame tick %d\n", mAudioTimeScale, mAudioFrameTick);
  }
  err = buildSampleOffset();
  if (DUnlikely(EECode_OK != err)) {
    LOG_ERROR("build sample offset fail\n");
    return err;
  }
  err = buildTimeTree();
  if (DUnlikely(EECode_OK != err)) {
    LOG_ERROR("build time fail\n");
    return err;
  }
  updateVideoExtraData();
  if (mpVideoExtraData && mVideoExtraDataLength) {
    if (StreamFormat_H264 == mVideoFormat) {
      gfGetH264SizeFromSPS(mpVideoExtraData, mVideoWidthFromExtradata, mVideoHeightFromExtradata);
    } else if (StreamFormat_H265 == mVideoFormat) {
      gfGetH265SizeFromExtradata(mpVideoExtraData, mVideoExtraDataLength, mVideoWidthFromExtradata, mVideoHeightFromExtradata);
    } else {
      LOG_FATAL("bad format\n");
    }
  }
  return EECode_OK;
}

EECode CMP4Probe::getVideoOffsetSize(TU64 &offset, TU32 &size)
{
  if (DUnlikely(mCurVideoFrameIndex > mTotalVideoFrameCountMinus1)) {
    LOG_NOTICE("read video eof\n");
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
  //if (!mbBackward) {
  //    mCurVideoFrameIndex ++;
  //} else if (mCurVideoFrameIndex) {
  //    mCurVideoFrameIndex --;
  //}
  return EECode_OK;
}

EECode CMP4Probe::getAudioOffsetSize(TU64 &offset, TU32 &size)
{
  if (DUnlikely(mCurAudioFrameIndex > mTotalAudioFrameCountMinus1)) {
    LOG_NOTICE("read audio eof\n");
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

EECode CMP4Probe::queryVideoOffsetSize(TU64 &offset, TU32 &size)
{
  if (DUnlikely(mCurVideoFrameIndex > mTotalVideoFrameCountMinus1)) {
    LOG_NOTICE("read video eof\n");
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

EECode CMP4Probe::queryAudioOffsetSize(TU64 &offset, TU32 &size)
{
  if (DUnlikely(mCurAudioFrameIndex > mTotalAudioFrameCountMinus1)) {
    LOG_NOTICE("read audio eof\n");
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

EECode CMP4Probe::copyFileData(TU8 *p_buf, TU64 offset, TU32 size)
{
  EECode err;
  TIOSize iosize = 0;
  if (DUnlikely(mCurFileOffset != offset)) {
    err = mpIO->Seek(offset, EIOPosition_Begin);
    if (EECode_OK != err) {
      LOG_FATAL("seek fail\n");
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
  LOG_FATAL("read fail\n");
  return EECode_IOError;
}

EECode CMP4Probe::readVideoFrame(TU8 *p_buf, TU32 &size)
{
  if (DLikely(mbHaveVideo)) {
    TU64 frame_offset = 0;
    TU32 frame_size = 0;
    EECode err;
    err = getVideoOffsetSize(frame_offset, frame_size);
    if (DLikely(EECode_OK == err)) {
      size = frame_size;
      copyFileData(p_buf, frame_offset, frame_size);
      //LOG_INFO("video cnt %d, frame_offset %" DPrint64x ", frame size %d\n", mCurVideoFrameIndex, frame_offset, frame_size);
    } else if (EECode_OK_EOF == err) {
      return err;
    } else {
      LOG_ERROR("read video offset size fail\n");
      return err;
    }
  } else {
    LOG_FATAL("not video data\n");
    return EECode_BadState;
  }
  __convert_to_annexb(p_buf, size);
  return EECode_OK;
}

EECode CMP4Probe::readAudioFrame(TU8 *p_buf, TU32 &size)
{
  if (DLikely(mbHaveAudio)) {
    TU64 frame_offset = 0;
    TU32 frame_size = 0;
    EECode err;
    err = getAudioOffsetSize(frame_offset, frame_size);
    if (DLikely(EECode_OK == err)) {
      size = frame_size;
      copyFileData(p_buf, frame_offset, frame_size);
      //LOG_INFO("audio cnt %d, frame_offset %" DPrint64x ", frame size %d\n", mCurAudioFrameIndex, frame_offset, frame_size);
    } else if (EECode_OK_EOF == err) {
      return err;
    } else {
      LOG_ERROR("read video offset size fail\n");
      return err;
    }
  } else {
    LOG_FATAL("not video data\n");
    return EECode_BadState;
  }
  return EECode_OK;
}

EECode CMP4Probe::nextPacketInfo(TU32 &size, StreamType &type, TTime &dts, TTime &pts)
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
        LOG_WARN("return eof here 1\n");
        return EECode_OK_EOF;
      } else {
        LOG_ERROR("fail here, video\n");
        return err;
      }
    } else {
      LOG_ERROR("fail here, audio\n");
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
      LOG_WARN("return eof here 2\n");
      return EECode_OK_EOF;
    } else {
      LOG_ERROR("fail here, video\n");
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
      LOG_WARN("return eof here 3\n");
      return EECode_OK_EOF;
    } else {
      LOG_ERROR("fail here, audio\n");
      return err;
    }
  } else {
    LOG_FATAL("no audio and no video\n");
    return EECode_BadState;
  }
  return EECode_OK;
}

EECode CMP4Probe::nextPacketInfoTimeOrder(TU32 &size, StreamType &type, TTime &dts, TTime &pts)
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
      LOG_WARN("return eof here 1\n");
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
      LOG_WARN("return eof here 2\n");
      return EECode_OK_EOF;
    } else {
      LOG_ERROR("fail here, video\n");
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
      LOG_WARN("return eof here 3\n");
      return EECode_OK_EOF;
    } else {
      LOG_ERROR("fail here, audio\n");
      return err;
    }
  } else {
    LOG_FATAL("no audio and no video\n");
    return EECode_BadState;
  }
  return EECode_OK;
}

EECode CMP4Probe::nextVideoKeyFrameInfo(TU32 &size, TTime &dts, TTime &pts)
{
  TU32 max_framecount = mTotalVideoFrameCountMinus1 + 1;
  if (DUnlikely(mCurVideoFrameIndex > mTotalVideoFrameCountMinus1)) {
    LOG_NOTICE("read video eof\n");
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
  LOG_NOTICE("read video eof\n");
  return EECode_OK_EOF;
}

EECode CMP4Probe::previousVideoKeyFrameInfo(TU32 &size, TTime &dts, TTime &pts)
{
  if (DUnlikely(!mCurVideoFrameIndex)) {
    LOG_NOTICE("read video eof\n");
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
  LOG_WARN("read video eof\n");
  return EECode_OK_EOF;
}

EECode CMP4Probe::updateVideoExtraData()
{
  TU32 len = 0;
  TU8 *p = NULL;
  TU8 start_code_prefix[4] = {0x0, 0x0, 0x0, 0x1};
  if (mbHaveVideo) {
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
          LOG_FATAL("no memory, %d\n", mVideoExtraDataBufferLength);
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
          LOG_FATAL("no memory, %d\n", mVideoExtraDataBufferLength);
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
      LOG_FATAL("bad video format %d\n", mVideoFormat);
      return EECode_BadState;
    }
    mbVideoEnabled = 1;
  }
  return EECode_OK;
}

EECode CMP4Probe::updateAudioExtraData()
{
  if (mbHaveAudio) {
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
          LOG_FATAL("no memory, %d\n", mAudioExtraDataBufferLength);
          return EECode_NoMemory;
        }
      }
      mAudioExtraDataLength = 2;
      mpAudioExtraData[0] = aac_config->config[0];
      mpAudioExtraData[1] = aac_config->config[1];
      mbAudioEnabled = 1;
    } else {
      LOG_FATAL("bad audio format %d\n", mVideoFormat);
      return EECode_BadState;
    }
  }
  return EECode_OK;
}

EECode CMP4Probe::buildTimeTree()
{
  TU32 i = 0, j = 0;
  TU32 frame_index = 0;
  TTime time = 0;
  SMp4DTimeTree *dt = NULL;
  _STimeEntry *entry = NULL;
  TU32 *pv = NULL;
  if (mbHaveVideo && mpVideoSampleTableBox) {
    if ((!mpVideoDTimeTree) || (mnVideoDTimeTreeNodeBufferCount < mpVideoSampleTableBox->stts.entry_count)) {
      if (mpVideoDTimeTree) {
        DDBG_FREE(mpVideoDTimeTree, "M4VD");
        mpVideoDTimeTree = NULL;
      }
      mnVideoDTimeTreeNodeBufferCount = mpVideoSampleTableBox->stts.entry_count;
      mpVideoDTimeTree = (SMp4DTimeTree *) DDBG_MALLOC(mnVideoDTimeTreeNodeBufferCount * sizeof(SMp4DTimeTree), "M4VD");
      if (!mpVideoDTimeTree) {
        LOG_FATAL("no memory\n");
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
  }
  if (mbHaveAudio && mpAudioSampleTableBox) {
    if ((!mpAudioDTimeTree) || (mnAudioDTimeTreeNodeBufferCount < mpAudioSampleTableBox->stts.entry_count)) {
      if (mpAudioDTimeTree) {
        DDBG_FREE(mpAudioDTimeTree, "M4AD");
        mpAudioDTimeTree = NULL;
      }
      mnAudioDTimeTreeNodeBufferCount = mpAudioSampleTableBox->stts.entry_count;
      mpAudioDTimeTree = (SMp4DTimeTree *) DDBG_MALLOC(mnAudioDTimeTreeNodeBufferCount * sizeof(SMp4DTimeTree), "M4AD");
      if (!mpAudioDTimeTree) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
      }
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
        LOG_FATAL("no memory\n");
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

TU32 CMP4Probe::findVideoTimeIndex(TU32 index)
{
  TU32 i = 0;
  for (i = 0; i < mnVideoDTimeTreeNodeCount; i ++) {
    if ((mpVideoDTimeTree[i].frame_start_index + mpVideoDTimeTree[i].frame_count) > index) {
      return i;
    }
  }
  LOG_FATAL("bug\n");
  return 0;
}

EECode CMP4Probe::getVideoTimestamp(TU32 index, TTime &dts, TTime &pts)
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

EECode CMP4Probe::getAudioTimestamp(TU32 index, TTime &dts, TTime &native_dts)
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

EECode CMP4Probe::buildSampleOffset()
{
  TU32 i = 0, j = 0, k = 0;
  TU32 chunks = 0;
  TU32 frame_index = 0;
  TU32 chunk_index = 0;
  TU32 chunk_offset = 0, ac_offset_in_chunk = 0;
  TU32 remain_frames = 0, samples_per_chunk = 0;;
  TU64 chunk_offset_64 = 0;
  if (mbHaveVideo && ((!mbVideoOneSamplePerChunk) && mpVideoSampleTableBox)) {
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
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
      }
    }
    frame_index = 0;
    chunk_index = 0;
    for (i = 0; i < (mpVideoSampleTableBox->sample_to_chunk.entry_count - 1); i ++) {
      chunks = mpVideoSampleTableBox->sample_to_chunk.entrys[i + 1].first_chunk - mpVideoSampleTableBox->sample_to_chunk.entrys[i].first_chunk;
      samples_per_chunk = mpVideoSampleTableBox->sample_to_chunk.entrys[i].sample_per_chunk;
      LOG_NOTICE("%d's entry, chunks %d, sample per chunk %d, first_chunk %d, next first_chunk %d\n", i, chunks, samples_per_chunk, mpVideoSampleTableBox->sample_to_chunk.entrys[i].first_chunk, mpVideoSampleTableBox->sample_to_chunk.entrys[i + 1].first_chunk);
      for (j = 0; j < chunks; j ++) {
        if (!mpVideoSampleTableBox->b_use_offset64) {
          DASSERT(mpVideoSampleTableBox->chunk_offset.chunk_offset);
          chunk_offset = mpVideoSampleTableBox->chunk_offset.chunk_offset[chunk_index];
          ac_offset_in_chunk = 0;
          for (k = 0; k < samples_per_chunk; k ++) {
            mpVideoSampleOffset[frame_index] = chunk_offset + ac_offset_in_chunk;
            ac_offset_in_chunk += mpVideoSampleTableBox->sample_size.entry_size[frame_index];
            frame_index ++;
          }
        } else {
          DASSERT(mpVideoSampleTableBox->chunk_offset64.chunk_offset);
          chunk_offset_64 = mpVideoSampleTableBox->chunk_offset64.chunk_offset[chunk_index];
          ac_offset_in_chunk = 0;
          for (k = 0; k < samples_per_chunk; k ++) {
            mpVideoSampleOffset[frame_index] = chunk_offset_64 + ac_offset_in_chunk;
            ac_offset_in_chunk += mpVideoSampleTableBox->sample_size.entry_size[frame_index];
            frame_index ++;
          }
        }
        chunk_index ++;
      }
    }
    samples_per_chunk = mpVideoSampleTableBox->sample_to_chunk.entrys[i].sample_per_chunk;
    remain_frames = mTotalVideoFrameCountMinus1 + 1 - frame_index;
    chunks = remain_frames / samples_per_chunk;
    if (DUnlikely(remain_frames != (samples_per_chunk * chunks))) {
      LOG_ERROR("last chunk error, %d, %d, %d?\n", remain_frames, samples_per_chunk, chunks);
      return EECode_ParseError;
    }
    for (j = 0; j < chunks; j ++) {
      if (!mpVideoSampleTableBox->b_use_offset64) {
        DASSERT(mpVideoSampleTableBox->chunk_offset.chunk_offset);
        chunk_offset = mpVideoSampleTableBox->chunk_offset.chunk_offset[chunk_index];
        ac_offset_in_chunk = 0;
        for (k = 0; k < samples_per_chunk; k ++) {
          mpVideoSampleOffset[frame_index] = chunk_offset + ac_offset_in_chunk;
          ac_offset_in_chunk += mpVideoSampleTableBox->sample_size.entry_size[frame_index];
          frame_index ++;
        }
      } else {
        DASSERT(mpVideoSampleTableBox->chunk_offset64.chunk_offset);
        chunk_offset_64 = mpVideoSampleTableBox->chunk_offset64.chunk_offset[chunk_index];
        ac_offset_in_chunk = 0;
        for (k = 0; k < samples_per_chunk; k ++) {
          mpVideoSampleOffset[frame_index] = chunk_offset_64 + ac_offset_in_chunk;
          ac_offset_in_chunk += mpVideoSampleTableBox->sample_size.entry_size[frame_index];
          frame_index ++;
        }
      }
      chunk_index ++;
    }
  }
  if (mbHaveAudio && ((!mbAudioOneSamplePerChunk) && mpAudioSampleTableBox)) {
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
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
      }
    }
    frame_index = 0;
    chunk_index = 0;
    for (i = 0; i < (mpAudioSampleTableBox->sample_to_chunk.entry_count - 1); i ++) {
      chunks = mpAudioSampleTableBox->sample_to_chunk.entrys[i + 1].first_chunk - mpAudioSampleTableBox->sample_to_chunk.entrys[i].first_chunk;
      samples_per_chunk = mpAudioSampleTableBox->sample_to_chunk.entrys[i].sample_per_chunk;
      for (j = 0; j < chunks; j ++) {
        if (!mpAudioSampleTableBox->b_use_offset64) {
          DASSERT(mpAudioSampleTableBox->chunk_offset.chunk_offset);
          chunk_offset = mpAudioSampleTableBox->chunk_offset.chunk_offset[chunk_index];
          ac_offset_in_chunk = 0;
          for (k = 0; k < samples_per_chunk; k ++) {
            mpAudioSampleOffset[frame_index] = chunk_offset + ac_offset_in_chunk;
            ac_offset_in_chunk += mpAudioSampleTableBox->sample_size.entry_size[frame_index];
            frame_index ++;
          }
        } else {
          DASSERT(mpAudioSampleTableBox->chunk_offset64.chunk_offset);
          chunk_offset_64 = mpAudioSampleTableBox->chunk_offset64.chunk_offset[chunk_index];
          ac_offset_in_chunk = 0;
          for (k = 0; k < samples_per_chunk; k ++) {
            mpAudioSampleOffset[frame_index] = chunk_offset_64 + ac_offset_in_chunk;
            ac_offset_in_chunk += mpAudioSampleTableBox->sample_size.entry_size[frame_index];
            frame_index ++;
          }
        }
        chunk_index ++;
      }
    }
    samples_per_chunk = mpAudioSampleTableBox->sample_to_chunk.entrys[i].sample_per_chunk;
    remain_frames = mTotalAudioFrameCountMinus1 + 1 - frame_index;
    chunks = remain_frames / samples_per_chunk;
    if (DUnlikely(remain_frames != (samples_per_chunk * chunks))) {
      LOG_ERROR("last chunk error, %d, %d, %d?\n", remain_frames, samples_per_chunk, chunks);
      return EECode_ParseError;
    }
    for (j = 0; j < chunks; j ++) {
      if (!mpAudioSampleTableBox->b_use_offset64) {
        DASSERT(mpAudioSampleTableBox->chunk_offset.chunk_offset);
        chunk_offset = mpAudioSampleTableBox->chunk_offset.chunk_offset[chunk_index];
        ac_offset_in_chunk = 0;
        for (k = 0; k < samples_per_chunk; k ++) {
          mpAudioSampleOffset[frame_index] = chunk_offset + ac_offset_in_chunk;
          ac_offset_in_chunk += mpAudioSampleTableBox->sample_size.entry_size[frame_index];
          frame_index ++;
        }
      } else {
        DASSERT(mpAudioSampleTableBox->chunk_offset64.chunk_offset);
        chunk_offset_64 = mpAudioSampleTableBox->chunk_offset64.chunk_offset[chunk_index];
        ac_offset_in_chunk = 0;
        for (k = 0; k < samples_per_chunk; k ++) {
          mpAudioSampleOffset[frame_index] = chunk_offset_64 + ac_offset_in_chunk;
          ac_offset_in_chunk += mpAudioSampleTableBox->sample_size.entry_size[frame_index];
          frame_index ++;
        }
      }
      chunk_index ++;
    }
  }
  return EECode_OK;
}

TU32 CMP4Probe::timeToVideoFrameIndex(TTime dts)
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

TU32 CMP4Probe::timeToAudioFrameIndex(TTime dts)
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


