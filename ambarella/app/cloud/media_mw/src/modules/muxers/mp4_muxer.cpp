/*******************************************************************************
 * mp4_muxer.cpp
 *
 * History:
 *    2014/12/03 - [Zhi He] create file
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

#include "mp4_muxer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//#define DWRITE_ANNEXB_FORMAT

//debug check box size related bugs
#if 0
static void __check_box_fail()
{
    static TU32 fail = 0;
    fail ++;
}

#define DCHECK_BOX_BEGIN \
    TU64 check_end_offset = mCurFileOffset + p_box->base_box.size

#define DCHECK_FULL_BOX_BEGIN \
    TU64 check_end_offset = mCurFileOffset + p_box->base_full_box.base_box.size

#define DCHECK_BOX_END \
    if (check_end_offset != mCurFileOffset) { \
        __check_box_fail(); \
    }
#else
#define DCHECK_BOX_BEGIN
#define DCHECK_FULL_BOX_BEGIN
#define DCHECK_BOX_END
#endif

TU64 __gmtime(SDateTime *date_time)
{
    TU64 t;

    TU64 y = date_time->year, m = date_time->month, d = date_time->day;

    if (m < 3) {
        m += 12;
        y--;
    }

    t = 86400LL * (d + (153 * m - 457) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 719469);

    t += 3600 * date_time->hour + 60 * date_time->minute + date_time->seconds;

    return t;
}

#if 0
TU8 *__next_start_code(TU8 *p, TU32 len, TU32 &prefix_len, TU8 &nal_type)
{
    TUint state = 0;

    while (len) {
        switch (state) {
            case 0:
                if (!(*p)) {
                    state = 1;
                }
                break;

            case 1: //0
                if (!(*p)) {
                    state = 2;
                } else {
                    state = 0;
                }
                break;

            case 2: //0 0
                if (!(*p)) {
                    state = 3;
                } else if (1 == (*p)) {
                    prefix_len = 3;
                    nal_type = p[1] & 0x1f;
                    return (p - 2);
                } else {
                    state = 0;
                }
                break;

            case 3: //0 0 0
                if (!(*p)) {
                    state = 3;
                } else if (1 == (*p)) {
                    prefix_len = 4;
                    nal_type = p[1] & 0x1f;
                    return (p - 3);
                } else {
                    state = 0;
                }
                break;

            default:
                LOG_FATAL("impossible to comes here\n");
                break;

        }
        p++;
        len --;
    }

    return NULL;
}

static TU8 *__next_start_code_hevc(TU8 *p, TMemSize len, TU32 &prefix_len, TU8 &nal_type)
{
    TUint state = 0;

    while (len) {
        switch (state) {
            case 0:
                if (!(*p)) {
                    state = 1;
                }
                break;

            case 1: //0
                if (!(*p)) {
                    state = 2;
                } else {
                    state = 0;
                }
                break;

            case 2: //0 0
                if (!(*p)) {
                    state = 3;
                } else if (1 == (*p)) {
                    prefix_len = 3;
                    nal_type = (p[1] >> 1) & 0x3f;
                    return (p - 2);
                } else {
                    state = 0;
                }
                break;

            case 3: //0 0 0
                if (!(*p)) {
                    state = 3;
                } else if (1 == (*p)) {
                    prefix_len = 4;
                    nal_type = (p[1] >> 1) & 0x3f;
                    return (p - 3);
                } else {
                    state = 0;
                }
                break;

            default:
                LOG_FATAL("impossible to comes here\n");
                break;

        }
        p++;
        len --;
    }

    return NULL;
}
#endif

static TU32 _prefix_byte_number(TU8 *input, TU32 size, TU8 &nal_type)
{
    TU32 len = 0;
    while (!input[0] && size) {
        input ++;
        size --;
        len ++;
    }

    if (size) {
        if (0x01 == input[0]) {
            nal_type = (input[1] >> 1) & 0x3f;
            return len + 1;
        }
    }

    LOG_FATAL("[data corruption]: in _prefix_byte_number()\n");
    nal_type = 0;
    return 0;
}

IMuxer *gfCreateMP4MuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CMP4Muxer::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

IMuxer *CMP4Muxer::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CMP4Muxer *result = new CMP4Muxer(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CMP4Muxer::Destroy()
{
    Delete();
}

CMP4Muxer::CMP4Muxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mVideoFormat(StreamFormat_H264)
    , mAudioFormat(StreamFormat_AAC)
    , mOverallVideoDataLen(0)
    , mOverallAudioDataLen(0)
    , mOverallMediaDataLen(0)
    , mVideoDuration(0)
    , mAudioDuration(0)
    , mCreationTime(0)
    , mModificationTime(0)
    , mTimeScale(DDefaultTimeScale)
    , mDuration(0)
    , mVideoFrameTick(DDefaultVideoFramerateDen)
    , mAudioFrameTick(0)
    , mVideoFrameNumber(0)
    , mAudioFrameNumber(0)
    , mRate(0)
    , mVolume(0)
    , mUsedVersion(0)
    , mbVideoEnabled(1)
    , mbAudioEnabled(1)
    , mbConstFrameRate(1)
    , mbHaveBframe(0)
    , mbIOOpend(0)
    , mbGetVideoExtradata(0)
    , mbGetAudioExtradata(0)
    , mVideoTrackID(1)
    , mAudioTrackID(2)
    , mNextTrackID(3)
    , mbGetAudioParameters(0)
    , mFlags(0)
{
    mVideoWidth = 720;
    mVideoHeight = 480;
    mpVideoCodecName = DISOM_CODEC_AVC_NAME;
    mpAudioCodecName = DISOM_CODEC_AAC_NAME;

    mpVideoVPS = NULL;
    mVideoVPSLength = 0;

    mpVideoSPS = NULL;
    mVideoSPSLength = 0;

    mpVideoPPS = NULL;
    mVideoPPSLength = 0;

    mbVideoKeyFrameComes = 0;
    mbWriteMediaDataStarted = 0;
    mbSuspend = 0;
    mbCorrupted = 0;

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
    mpCurrentPosInCache = NULL;
    mCacheBufferSize = 0;

    mpIO = NULL;
    mCurFileOffset = 0;

    memset(&mFileTypeBox, 0x0, sizeof(mFileTypeBox));
    memset(&mMediaDataBox, 0x0, sizeof(mMediaDataBox));
    memset(&mMovieBox, 0x0, sizeof(mMovieBox));

}

EECode CMP4Muxer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleNativeMP4Muxer);

    mCacheBufferSize = 4 * 1024 * 1024;
    mpCacheBufferBase = (TU8 *) DDBG_MALLOC(mCacheBufferSize + 31, "M4CH");
    mpCacheBufferBaseAligned = (TU8 *)((((TULong)mpCacheBufferBase) + 31) & (~0x1f));
    mpCacheBufferEnd = mpCacheBufferBaseAligned + mCacheBufferSize;

    if (!mpCacheBufferBase) {
        LOGM_FATAL("No Memory! request size %llu\n", mCacheBufferSize + 31);
        return EECode_NoMemory;
    }

    mpCurrentPosInCache = mpCacheBufferBaseAligned;

    memset(mVideoCompressorName, 0x0, sizeof(mVideoCompressorName));
    memset(mAudioCompressorName, 0x0, sizeof(mAudioCompressorName));

    defaultMP4FileSetting();

    return EECode_OK;
}

void CMP4Muxer::Delete()
{
    if (mpCacheBufferBase) {
        DDBG_FREE(mpCacheBufferBase, "M4CH");
        mpCacheBufferBase = NULL;
    }
    mpCacheBufferBaseAligned = NULL;
    mCacheBufferSize = 0;

    if (mpIO) {
        finalizeFile();
        mpIO->Delete();
        mpIO = NULL;
    }

    if (mpVideoVPS) {
        DDBG_FREE(mpVideoVPS, "M4VP");
        mpVideoVPS = NULL;
    }

    if (mpVideoSPS) {
        DDBG_FREE(mpVideoSPS, "M4SP");
        mpVideoSPS = NULL;
    }

    if (mpVideoPPS) {
        DDBG_FREE(mpVideoPPS, "M4PP");
        mpVideoPPS = NULL;
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

    inherited::Delete();
}

CMP4Muxer::~CMP4Muxer()
{

}

EECode CMP4Muxer::SetupContext()
{
    if (DLikely(!mpIO)) {
        mpIO = gfCreateIO(EIOType_File);
    } else {
        LOGM_WARN("mpIO is created yet?\n");
    }

    return EECode_OK;
}

EECode CMP4Muxer::DestroyContext()
{
    if (DLikely(mpIO)) {
        mpIO->Delete();
        mpIO = NULL;
    } else {
        LOGM_WARN("mpIO is not created?\n");
    }

    return EECode_OK;
}

EECode CMP4Muxer::SetSpecifiedInfo(SRecordSpecifiedInfo *info)
{
    if (info->video_compressorname[0]) {
        snprintf(mVideoCompressorName, 32, "%s", info->video_compressorname);
    }

    if (info->audio_compressorname[0]) {
        snprintf(mAudioCompressorName, 32, "%s", info->audio_compressorname);
    }

    mbConstFrameRate = info->b_video_const_framerate;
    mbHaveBframe = info->b_video_have_b_frame;

    mCreationTime = __gmtime(&info->create_time);
    mModificationTime = __gmtime(&info->modification_time);

    mTimeScale = info->timescale;
    mVideoFrameTick = info->video_frametick;
    mAudioFrameTick = info->audio_frametick;

    mbVideoEnabled = info->enable_video;
    mbAudioEnabled = info->enable_audio;

    if (mbVideoEnabled) {
        if (StreamFormat_H264 == info->video_format) {
            mVideoFormat = StreamFormat_H264;
            mpVideoCodecName = DISOM_CODEC_AVC_NAME;
        } else if (StreamFormat_H265 == info->video_format) {
            mVideoFormat = StreamFormat_H265;
            mpVideoCodecName = DISOM_CODEC_HEVC_NAME;
        } else if (StreamFormat_Invalid == info->video_format) {
            LOG_WARN("not specified video format in CMP4Muxer::SetSpecifiedInfo()\n");
        } else {
            LOG_ERROR("not supported video format(%d) in CMP4Muxer::SetSpecifiedInfo()\n", info->video_format);
            return EECode_BadParam;
        }
    }

    if (mbAudioEnabled) {
        if (StreamFormat_AAC == info->audio_format) {
            mpAudioCodecName = DISOM_CODEC_AAC_NAME;
        } else if (StreamFormat_Invalid == info->audio_format) {
            LOG_WARN("not specified audio format in CMP4Muxer::SetSpecifiedInfo()\n");
        } else {
            LOG_ERROR("not supported audio format(%d) in CMP4Muxer::SetSpecifiedInfo()\n", info->audio_format);
            return EECode_BadParam;
        }
    }

    return EECode_OK;
}

EECode CMP4Muxer::GetSpecifiedInfo(SRecordSpecifiedInfo *info)
{
    return EECode_OK;
}

EECode CMP4Muxer::SetExtraData(StreamType type, TUint stream_index, void *p_data, TUint data_size)
{
    DASSERT(stream_index < EConstMaxDemuxerMuxerStreamNumber);

    DASSERT(p_data);
    DASSERT(data_size);
    if (!p_data || !data_size) {
        LOGM_FATAL("[stream_index %d] p_data %p, data_size %d\n", stream_index, p_data, data_size);
        return EECode_BadParam;
    }

    if (StreamType_Video == type) {
        if (StreamFormat_H264 == mVideoFormat) {
            processAVCExtradata((TU8 *)p_data, data_size);
        } else if (StreamFormat_H265 == mVideoFormat) {
            processHEVCExtradata((TU8 *)p_data, data_size);
        }
        mbGetVideoExtradata = 1;
    } else if (StreamType_Audio == type) {
        if (mAudioExtraDataBufferLength < data_size) {
            if (mpAudioExtraData) {
                DDBG_FREE(mpAudioExtraData, "M4AE");
            }
            mAudioExtraDataBufferLength = data_size;
            mpAudioExtraData = (TU8 *) DDBG_MALLOC(mAudioExtraDataBufferLength, "M4AE");
            if (DUnlikely(!mpAudioExtraData)) {
                LOGM_FATAL("not enough memory, request size %d\n", mAudioExtraDataBufferLength);
                return EECode_NoMemory;
            }
        }
        mAudioExtraDataLength = data_size;
        memcpy(mpAudioExtraData, p_data, mAudioExtraDataLength);
        mbGetAudioExtradata = 1;
    }

    return EECode_OK;
}

EECode CMP4Muxer::SetPrivateDataDurationSeconds(void *p_data, TUint data_size)
{
    return EECode_OK;
}

EECode CMP4Muxer::SetPrivateDataChannelName(void *p_data, TUint data_size)
{
    return EECode_OK;
}

EECode CMP4Muxer::InitializeFile(const SStreamCodecInfos *infos, TChar *url, ContainerType type, TChar *thumbnailname, TTime start_pts, TTime start_dts)
{
    if (mbSuspend) {
        return EECode_OK;
    }

    EECode err = EECode_OK;

    if (DUnlikely(!infos || !url)) {
        LOGM_ERROR("NULL input: url %p, infos %p\n", url, infos);
        return EECode_BadParam;
    }

    if (DUnlikely(mConfigLogLevel > ELogLevel_Notice)) {
        gfPrintCodecInfoes((SStreamCodecInfos *)infos);
    }

    DASSERT(mpIO);
    if (mbIOOpend) {
        LOGM_WARN("mpIO is opened here? close it first\n");
        mpIO->Close();
        mbIOOpend = 0;
    }

    if (mpIO) {
        err = mpIO->Open(url, EIOFlagBit_Write | EIOFlagBit_Binary);
        if (EECode_OK != err) {
            LOGM_INFO("IIO->Open(%s) fail, return %d, %s.\n", url, err, gfGetErrorCodeString(err));
            return err;
        }
        mbIOOpend = 1;
        mbCorrupted = 0;
    }

    beginFile();

    return EECode_OK;
}

EECode CMP4Muxer::FinalizeFile(SMuxerFileInfo *p_file_info)
{
    if (DLikely(mpIO)) {
        finalizeFile();
    }

    return EECode_OK;
}

EECode CMP4Muxer::WriteVideoBuffer(CIBuffer *pBuffer, SMuxerDataTrunkInfo *info)
{
    if (!mbIOOpend) {
        return EECode_OK;
    }

    if (DUnlikely(!mbVideoEnabled)) {
        return EECode_OK;
    }

    if (DUnlikely(mbCorrupted)) {
        return EECode_DataCorruption;
    }

    TMemSize packet_size = pBuffer->GetDataSize();
    TU8 *packet_data = pBuffer->GetDataPtr();
    mDebugHeartBeat ++;

    if (DUnlikely(!mbVideoKeyFrameComes)) {
        if (pBuffer->mVideoFrameType == EPredefinedPictureType_IDR) {
            mbVideoKeyFrameComes = 1;
            if (!mbGetVideoExtradata) {
                if (StreamFormat_H264 == mVideoFormat) {
                    processAVCExtradata(packet_data, (TU32) packet_size);
                } else if (StreamFormat_H265 == mVideoFormat) {
                    processHEVCExtradata(packet_data, (TU32) packet_size);
                } else {
                    LOG_ERROR("BAD video format %d\n", mVideoFormat);
                }
            }
        } else {
            LOGM_WARN("non-IDR frame discarded.\n");
            return EECode_OK;
        }
    }

#ifdef DWRITE_ANNEXB_FORMAT

    mOverallVideoDataLen += packet_size;
    mOverallMediaDataLen += packet_size;
    mVideoDuration += mVideoFrameTick;
    mVideoFrameNumber ++;

    writeVideoFrame(mCurFileOffset, (TU64)packet_size);
    writeData(packet_data, packet_size);

#else

#if 1
    TU32 len = 0;
    TU32 cur_write_len = 0;
    TU32 i = 0;
    TU32 write_len = 0;
    TU8 *p_cur = NULL;
    TU8 nal_type;

    if ((StreamFormat_H265 == mVideoFormat) && (1 < pBuffer->mSubPacketNumber)) {
        TU64 ori_file_offset = mCurFileOffset;

        while (i < pBuffer->mSubPacketNumber) {
            p_cur = packet_data + pBuffer->mOffsetHint[i];
            len = _prefix_byte_number(p_cur, 16, nal_type);
            if (DUnlikely(!len)) {
                mbCorrupted = 1;
                return EECode_DataCorruption;
            }

            if ((EHEVCNalType_VPS == nal_type) || (EHEVCNalType_SPS == nal_type) || (EHEVCNalType_PPS == nal_type)) {
                i ++;
                continue;
            }

            if (i < (pBuffer->mSubPacketNumber - 1)) {
                if (DUnlikely(pBuffer->mOffsetHint[i + 1] < (pBuffer->mOffsetHint[i] + 5))) {
                    LOGM_FATAL("bad offset hint\n");
                    return EECode_DataCorruption;
                }
                cur_write_len = pBuffer->mOffsetHint[i + 1] - pBuffer->mOffsetHint[i] - len;
            } else {
                if (DUnlikely((TIOSize)packet_size < (pBuffer->mOffsetHint[i] + 5))) {
                    LOGM_FATAL("bad offset hint\n");
                    return EECode_DataCorruption;
                }
                cur_write_len = packet_size - pBuffer->mOffsetHint[i] - len;
            }

            //TU8 nal_type = (p_cur[len] >> 1) & 0x3f;
            //if (EHEVCNalType_VCL_END > nal_type) {
            write_len += cur_write_len + 4;
            writeBE32(cur_write_len);
            writeData(p_cur + len, cur_write_len);
            //}

            i ++;
        }

        mOverallVideoDataLen += write_len;
        mOverallMediaDataLen += write_len;
        mVideoDuration += mVideoFrameTick;
        mVideoFrameNumber ++;
        writeVideoFrame(ori_file_offset, (TU64)write_len);
    } else {
        len = _prefix_byte_number(packet_data, (TU32) packet_size, nal_type);
        if (DUnlikely(!len)) {
            mbCorrupted = 1;
            return EECode_DataCorruption;
        }

        write_len = packet_size - len + 4;

        mOverallVideoDataLen += write_len;
        mOverallMediaDataLen += write_len;
        mVideoDuration += mVideoFrameTick;
        mVideoFrameNumber ++;

        writeVideoFrame(mCurFileOffset, (TU64)write_len);
        writeBE32(write_len - 4);
        writeData(packet_data + len, packet_size - len);
    }

#else
    TU8 nal_type;
    TU32 len = _prefix_byte_number(packet_data, (TU32) packet_size, nal_type);
    TU32 write_len = packet_size - len + 4;

    mOverallVideoDataLen += write_len;
    mOverallMediaDataLen += write_len;
    mVideoDuration += mVideoFrameTick;
    mVideoFrameNumber ++;

    writeVideoFrame(mCurFileOffset, (TU64)write_len);
    writeBE32(write_len - 4);
    writeData(packet_data + len, packet_size - len);
#endif

#endif

    return EECode_OK;
}

EECode CMP4Muxer::WriteAudioBuffer(CIBuffer *pBuffer, SMuxerDataTrunkInfo *info)
{
    if (!mbIOOpend) {
        return EECode_OK;
    }

    if (DUnlikely(!mbAudioEnabled)) {
        return EECode_OK;
    }

    if (DUnlikely(mbCorrupted)) {
        return EECode_DataCorruption;
    }

    TMemSize packet_size = pBuffer->GetDataSize();
    TU8 *packet_data = pBuffer->GetDataPtr();

    if (0xff == packet_data[0] && 0xf0 == (packet_data[1] & 0xf0)) {
        if (!mbGetAudioParameters) {
            parseADTSHeader(packet_data);
            mbGetAudioParameters = 1;
            if (!mpAudioExtraData) {
                mpAudioExtraData = gfGenerateAACExtraData(mAudioSampleRate, mAudioChannelNumber, mAudioExtraDataBufferLength);
            }
        }
        packet_size -= 7;
        packet_data += 7;
    }

    mDebugHeartBeat ++;

    mOverallAudioDataLen += packet_size;
    mOverallMediaDataLen += packet_size;
    mAudioDuration += mAudioFrameTick;
    mAudioFrameNumber ++;

    writeAudioFrame(mCurFileOffset, (TU64)packet_size);
    writeData(packet_data, packet_size);

    return EECode_OK;
}

EECode CMP4Muxer::WritePridataBuffer(CIBuffer *pBuffer, SMuxerDataTrunkInfo *info)
{
    LOGM_FATAL("TO DO\n");
    return EECode_OK;
}

EECode CMP4Muxer::SetState(TU8 b_suspend)
{
    mbSuspend = b_suspend;
    return EECode_OK;
}

void CMP4Muxer::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    mDebugHeartBeat = 0;
}

void CMP4Muxer::defaultMP4FileSetting()
{
    mUsedVersion = 0;
    mFlags = 0;

    mRate = (1 << 16);
    mVolume = (1 << 8);

    mMatrix[1] = mMatrix[2] = mMatrix[3] = mMatrix[5] = mMatrix[6] = mMatrix[7] = 0;
    mMatrix[0] = mMatrix[4] = 0x00010000;
    mMatrix[8] = 0x40000000;

    mAudioChannelNumber = DDefaultAudioChannelNumber;
    mAudioSampleSize = 16;
    mAudioSampleRate = DDefaultAudioSampleRate;

    snprintf(mVideoCompressorName, 32, "%s", DVIDEO_COMPRESSORNAME);
    snprintf(mAudioCompressorName, 32, "%s", DAUDIO_COMPRESSORNAME);
}

void CMP4Muxer::fillFileTypeBox(SISOMFileTypeBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_FILE_TYPE_BOX_TAG, DISOM_TAG_SIZE);

    memcpy(p_box->major_brand, DISOM_BRAND_V0_TAG, DISOM_TAG_SIZE);
    p_box->minor_version = 0;

    memcpy(p_box->compatible_brands[0], DISOM_BRAND_V0_TAG, DISOM_TAG_SIZE);
    memcpy(p_box->compatible_brands[1], DISOM_BRAND_V1_TAG, DISOM_TAG_SIZE);
    if (StreamFormat_H264 == mVideoFormat) {
        memcpy(p_box->compatible_brands[2], DISOM_BRAND_AVC_TAG, DISOM_TAG_SIZE);
    } else if (StreamFormat_H265 == mVideoFormat) {
        memcpy(p_box->compatible_brands[2], DISOM_BRAND_HEVC_TAG, DISOM_TAG_SIZE);
    } else {
        LOG_ERROR("fatal error, bad video format %d\n", mVideoFormat);
    }
    memcpy(p_box->compatible_brands[3], DISOM_BRAND_MPEG4_TAG, DISOM_TAG_SIZE);
    memcpy(p_box->compatible_brands[4], DISOM_BRAND_MSMPEG4_TAG, DISOM_TAG_SIZE);

    p_box->compatible_brands_number = 5;

    p_box->base_box.size = sizeof(p_box->base_box) + sizeof(p_box->major_brand) + sizeof(p_box->minor_version) + (p_box->compatible_brands_number * DISOM_TAG_SIZE);
}

void CMP4Muxer::fillMediaDataBox(SISOMMediaDataBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_MEDIA_DATA_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_box.size = sizeof(SISOMMediaDataBox);
}

void CMP4Muxer::fillMovieHeaderBox(SISOMMovieHeaderBox *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_MOVIE_HEADER_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = mFlags;

    p_box->creation_time = mCreationTime;
    p_box->modification_time = mModificationTime;
    p_box->timescale = mTimeScale;
    p_box->duration = mDuration;

    p_box->rate = mRate;
    p_box->volume = mVolume;

    for (TU32 i = 0; i < 9; i ++) {
        p_box->matrix[i] = mMatrix[i];
    }

    p_box->next_track_ID = mNextTrackID;

    if (!mUsedVersion) {
        p_box->base_full_box.base_box.size = DISOMConstMovieHeaderSize;
    } else {
        p_box->base_full_box.base_box.size = DISOMConstMovieHeader64Size;
    }
}

void CMP4Muxer::fillMovieVideoTrackHeaderBox(SISOMTrackHeader *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_TRACK_HEADER_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = DISOM_TRACK_ENABLED_FLAG;

    p_box->creation_time = mCreationTime;
    p_box->modification_time = mModificationTime;
    p_box->track_ID = mVideoTrackID;
    p_box->duration = mVideoDuration;

    for (TU32 i = 0; i < 9; i ++) {
        p_box->matrix[i] = mMatrix[i];
    }

    p_box->width = mVideoWidth;
    p_box->height = mVideoHeight;

    if (!mUsedVersion) {
        p_box->base_full_box.base_box.size = DISOMConstTrackHeaderSize;
    } else {
        p_box->base_full_box.base_box.size = DISOMConstTrackHeader64Size;
    }
}

void CMP4Muxer::fillMovieAudioTrackHeaderBox(SISOMTrackHeader *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_TRACK_HEADER_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = DISOM_TRACK_ENABLED_FLAG;

    p_box->creation_time = mCreationTime;
    p_box->modification_time = mModificationTime;
    p_box->track_ID = mAudioTrackID;
    p_box->duration = mAudioDuration;

    p_box->volume = mVolume;

    for (TU32 i = 0; i < 9; i ++) {
        p_box->matrix[i] = mMatrix[i];
    }

    if (!mUsedVersion) {
        p_box->base_full_box.base_box.size = DISOMConstTrackHeaderSize;
    } else {
        p_box->base_full_box.base_box.size = DISOMConstTrackHeader64Size;
    }
}

void CMP4Muxer::fillMediaHeaderBox(SISOMMediaHeaderBox *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_MEDIA_HEADER_BOX_TAG, DISOM_TAG_SIZE);

    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = mFlags;

    p_box->creation_time = mCreationTime;
    p_box->modification_time = mModificationTime;

    p_box->timescale = mTimeScale;
    p_box->duration = mDuration;

    if (!mUsedVersion) {
        p_box->base_full_box.base_box.size = DISOMConstMediaHeaderSize;
    } else {
        p_box->base_full_box.base_box.size = DISOMConstMediaHeader64Size;
    }
}

void CMP4Muxer::fillHanderReferenceBox(SISOMHandlerReferenceBox *p_box, const TChar *handler, const TChar *name)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_HANDLER_REFERENCE_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = mFlags;

    memcpy(p_box->handler_type, handler, DISOM_TAG_SIZE);

    p_box->name = (TChar *) name;
    if (p_box->name) {
        p_box->base_full_box.base_box.size = DISOMConstHandlerReferenceSize + strlen(p_box->name) + 1;
    } else {
        p_box->base_full_box.base_box.size = DISOMConstHandlerReferenceSize + 1;
    }
}

void CMP4Muxer::fillChunkOffsetBox(SISOMChunkOffsetBox *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_CHUNK_OFFSET_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = mFlags;
}

void CMP4Muxer::fillSampleToChunkBox(SISOMSampleToChunkBox *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_SAMPLE_TO_CHUNK_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = mFlags;

    p_box->entry_count = 1;
    if (!p_box->entrys) {
        p_box->entrys = (_SSampleToChunkEntry *) DDBG_MALLOC(p_box->entry_count * sizeof(_SSampleToChunkEntry), "M4VS");
    }
    if (DUnlikely(!p_box->entrys)) {
        LOG_ERROR("fillSampleToChunkBox: no memory\n");
        return;
    }

    p_box->entrys[0].first_chunk = 1;
    p_box->entrys[0].sample_per_chunk = 1;
    p_box->entrys[0].sample_description_index = 1;
}

void CMP4Muxer::fillSampleSizeBox(SISOMSampleSizeBox *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_SAMPLE_SIZE_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = mFlags;
}

void CMP4Muxer::fillAVCDecoderConfigurationRecordBox(SISOMAVCDecoderConfigurationRecordBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_AVC_DECODER_CONFIGURATION_RECORD_TAG, DISOM_TAG_SIZE);
    p_box->configurationVersion = 1;
    p_box->AVCProfileIndication = 0;
    p_box->profile_compatibility = 0;
    p_box->AVCLevelIndication = 0;

    p_box->reserved = 0xff;
    p_box->lengthSizeMinusOne = 3;
    p_box->reserved_1 = 0xff;

    p_box->numOfSequenceParameterSets = 1;
    p_box->sequenceParametersSetLength = mVideoSPSLength;
    p_box->p_sps = mpVideoSPS;

    p_box->numOfPictureParameterSets = 1;
    p_box->pictureParametersSetLength = mVideoPPSLength;
    p_box->p_pps = mpVideoPPS;

    p_box->base_box.size = DISOMConstAVCDecoderConfiurationRecordSize_FixPart + p_box->sequenceParametersSetLength + p_box->pictureParametersSetLength;
}

void CMP4Muxer::fillHEVCDecoderConfigurationRecordBox(SISOMHEVCDecoderConfigurationRecordBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_HEVC_DECODER_CONFIGURATION_RECORD_TAG, DISOM_TAG_SIZE);
    p_box->record.numOfArrays = 3;
    p_box->vps = mpVideoVPS;
    p_box->vps_length = mVideoVPSLength;
    p_box->sps = mpVideoSPS;
    p_box->sps_length = mVideoSPSLength;
    p_box->pps = mpVideoPPS;
    p_box->pps_length = mVideoPPSLength;
    p_box->num_of_sei = 0;

    p_box->base_box.size = DISOMConstHEVCDecoderConfiurationRecordSize_FixPart + (5 * 3) + p_box->vps_length + p_box->sps_length + p_box->pps_length;
}

void CMP4Muxer::fillVisualSampleDescriptionBox(SISOMVisualSampleDescriptionBox *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_SAMPLE_DESCRIPTION_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = mFlags;

    p_box->entry_count = 1;

    if (mpVideoCodecName) {
        memcpy(p_box->visual_entry.base_box.type, mpVideoCodecName, DISOM_TAG_SIZE);
    } else {
        LOG_ERROR("not specify video codec name? please check code.\n");
        memcpy(p_box->visual_entry.base_box.type, DISOM_CODEC_AVC_NAME, DISOM_TAG_SIZE);
    }

    p_box->visual_entry.data_reference_index = 1;
    p_box->visual_entry.width = mVideoWidth;
    p_box->visual_entry.height = mVideoHeight;

    p_box->visual_entry.horizresolution = 0x00480000;
    p_box->visual_entry.vertresolution = 0x00480000;

    p_box->visual_entry.frame_count = 1;

    strcpy(p_box->visual_entry.compressorname, mVideoCompressorName);
    p_box->visual_entry.depth = 0x0018;
    p_box->visual_entry.pre_defined_2 = -1;

    if (StreamFormat_H264 == mVideoFormat) {
        fillAVCDecoderConfigurationRecordBox(&p_box->visual_entry.avc_config);
        p_box->visual_entry.base_box.size = DISOMConstVisualSampleEntrySize_FixPart + p_box->visual_entry.avc_config.base_box.size;
    } else if (StreamFormat_H265 == mVideoFormat) {
        fillHEVCDecoderConfigurationRecordBox(&p_box->visual_entry.hevc_config);
        p_box->visual_entry.base_box.size = DISOMConstVisualSampleEntrySize_FixPart + p_box->visual_entry.hevc_config.base_box.size;
    } else {
        LOG_ERROR("bad video format %d in fillVisualSampleDescriptionBox\n", mVideoFormat);
    }

    p_box->base_full_box.base_box.size = DISOM_FULL_BOX_SIZE + sizeof(TU32) + p_box->visual_entry.base_box.size;
}

void CMP4Muxer::fillAACElementaryStreamDescriptionBox(SISOMAACElementaryStreamDescriptor *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_ELEMENTARY_STREAM_DESCRIPTOR_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = mFlags;

    p_box->SL_descriptor_type_tag = 0x06;
    p_box->SL_value = 0x02;
    p_box->strange_size_3 = 1;

    p_box->decoder_config_type_tag = 0x05;
    p_box->strange_size_2 = mAudioExtraDataLength;
    DASSERT(2 == mAudioExtraDataLength);
    DASSERT(mpAudioExtraData);
    if (mpAudioExtraData) {
        p_box->config[0] = mpAudioExtraData[0];
        p_box->config[1] = mpAudioExtraData[1];
    }

    p_box->decoder_descriptor_type_tag = 0x04;
    p_box->strange_size_1 = 13 + mAudioExtraDataLength + 5;
    p_box->object_type = 0x40;
    p_box->stream_flag = 0x15;
    p_box->buffer_size = 8192;
    p_box->max_bitrate = mAudioMaxBitrate;
    p_box->avg_bitrate = mAudioAvgBitrate;

    p_box->es_descriptor_type_tag = 0x03;

    p_box->strange_size_0 = 8 + 13 + mAudioExtraDataLength + 5 + 6;
    p_box->track_id = mAudioTrackID;
    p_box->flags = 0;

    p_box->base_full_box.base_box.size = DISOMConstAACElementaryStreamDescriptorSize_FixPart + mAudioExtraDataLength;
}

void CMP4Muxer::fillAudioSampleDescriptionBox(SISOMAudioSampleDescriptionBox *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_SAMPLE_DESCRIPTION_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = mFlags;

    p_box->entry_count = 1;

    if (mpAudioCodecName) {
        memcpy(p_box->audio_entry.base_box.type, mpAudioCodecName, DISOM_TAG_SIZE);
    } else {
        LOG_ERROR("not specify audio codec name? please check code.\n");
        memcpy(p_box->audio_entry.base_box.type, "mp4a", DISOM_TAG_SIZE);
    }

    p_box->audio_entry.data_reference_index = 1;
    p_box->audio_entry.channelcount = mAudioChannelNumber;
    p_box->audio_entry.samplesize = mAudioSampleSize;
    p_box->audio_entry.samplerate = mAudioSampleRate;

    fillAACElementaryStreamDescriptionBox(&p_box->audio_entry.esd);

    p_box->audio_entry.base_box.size = DISOMConstAudioSampleEntrySize + p_box->audio_entry.esd.base_full_box.base_box.size;

    p_box->base_full_box.base_box.size = DISOM_FULL_BOX_SIZE + sizeof(TU32) + p_box->audio_entry.base_box.size;
}

void CMP4Muxer::fillVideoDecodingTimeToSampleBox(SISOMDecodingTimeToSampleBox *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG, DISOM_TAG_SIZE);

    if (mbConstFrameRate) {
        if (p_box->p_entry && p_box->entry_buf_count >= 1) {
            p_box->entry_count = 1;
            p_box->p_entry[0].sample_count = mVideoFrameNumber;
            p_box->p_entry[0].sample_delta = mVideoFrameTick;
        } else {
            if (p_box->p_entry) {
                DDBG_FREE(p_box->p_entry, "M4VS");
            }
            p_box->entry_buf_count = 1;
            p_box->p_entry = (_STimeEntry *) DDBG_MALLOC(p_box->entry_buf_count * sizeof(_STimeEntry), "M4VS");
            if (p_box->p_entry) {
                p_box->entry_count = 1;
                p_box->p_entry[0].sample_count = mVideoFrameNumber;
                p_box->p_entry[0].sample_delta = mVideoFrameTick;
            } else {
                LOG_FATAL("no memory\n");
            }
        }
    } else {
        p_box->entry_buf_count = DVIDEO_FRAME_COUNT;
        p_box->p_entry = (_STimeEntry *) DDBG_MALLOC(p_box->entry_buf_count * sizeof(_STimeEntry), "M4VS");
        if (!p_box->p_entry) {
            LOG_FATAL("no memory\n");
        } else {
            p_box->entry_count = 0;
        }
    }
}

void CMP4Muxer::fillAudioDecodingTimeToSampleBox(SISOMDecodingTimeToSampleBox *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG, DISOM_TAG_SIZE);

    if (p_box->p_entry && p_box->entry_buf_count >= 1) {
        p_box->entry_count = 1;
        p_box->p_entry[0].sample_count = mAudioFrameNumber;
        p_box->p_entry[0].sample_delta = mAudioFrameTick;
    } else {
        if (p_box->p_entry) {
            DDBG_FREE(p_box->p_entry, "M4AS");
        }
        p_box->entry_buf_count = 1;
        p_box->p_entry = (_STimeEntry *) DDBG_MALLOC(p_box->entry_buf_count * sizeof(_STimeEntry), "M4AS");
        if (p_box->p_entry) {
            p_box->entry_count = 1;
            p_box->p_entry[0].sample_count = mAudioFrameNumber;
            p_box->p_entry[0].sample_delta = mAudioFrameTick;
        } else {
            LOG_FATAL("no memory\n");
        }
    }

}

void CMP4Muxer::fillVideoSampleTableBox(SISOMSampleTableBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_SAMPLE_TABLE_BOX_TAG, DISOM_TAG_SIZE);

    fillVisualSampleDescriptionBox(&p_box->visual_sample_description);
    fillVideoDecodingTimeToSampleBox(&p_box->stts);
    fillSampleSizeBox(&p_box->sample_size);
    fillSampleToChunkBox(&p_box->sample_to_chunk);
    fillChunkOffsetBox(&p_box->chunk_offset);
}

void CMP4Muxer::fillSoundSampleTableBox(SISOMSampleTableBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_SAMPLE_TABLE_BOX_TAG, DISOM_TAG_SIZE);

    fillAudioSampleDescriptionBox(&p_box->audio_sample_description);
    fillAudioDecodingTimeToSampleBox(&p_box->stts);
    fillSampleSizeBox(&p_box->sample_size);
    fillSampleToChunkBox(&p_box->sample_to_chunk);
    fillChunkOffsetBox(&p_box->chunk_offset);
}

void CMP4Muxer::fillDataReferenceBox(SISOMDataReferenceBox *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_DATA_REFERENCE_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = mFlags;

    p_box->entry_count = 1;

    memcpy(p_box->url.base_box.type, DISOM_DATA_ENTRY_URL_BOX_TAG, DISOM_TAG_SIZE);
    p_box->url.version = mUsedVersion;
    p_box->url.flags = mFlags;

    p_box->url.base_box.size = DISOM_FULL_BOX_SIZE;

    p_box->base_full_box.base_box.size = DISOM_FULL_BOX_SIZE + p_box->entry_count * p_box->url.base_box.size + sizeof(TU32);
}

void CMP4Muxer::fillDataInformationBox(SISOMDataInformationBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_DATA_INFORMATION_BOX_TAG, DISOM_TAG_SIZE);
    fillDataReferenceBox(&p_box->data_ref);

    p_box->base_box.size = p_box->data_ref.base_full_box.base_box.size + DISOM_BOX_SIZE;
}

void CMP4Muxer::fillVideoMediaHeaderBox(SISOMVideoMediaHeaderBox *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_VIDEO_MEDIA_HEADER_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = mFlags;

    p_box->graphicsmode = 0;
    p_box->opcolor[0] = p_box->opcolor[1] = p_box->opcolor[2] = 0;
    p_box->base_full_box.base_box.size = DISOMConstVideoMediaHeaderSize;
}

void CMP4Muxer::fillSoundMediaHeaderBox(SISOMSoundMediaHeaderBox *p_box)
{
    memcpy(p_box->base_full_box.base_box.type, DISOM_SOUND_MEDIA_HEADER_BOX_TAG, DISOM_TAG_SIZE);
    p_box->base_full_box.version = mUsedVersion;
    p_box->base_full_box.flags = mFlags;

    p_box->balanse = 0;
    p_box->base_full_box.base_box.size = DISOMConstSoundMediaHeaderSize;
}

void CMP4Muxer::fillVideoMediaInformationBox(SISOMMediaInformationBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_MEDIA_INFORMATION_BOX_TAG, DISOM_TAG_SIZE);
    fillVideoMediaHeaderBox(&p_box->video_header);
    fillDataInformationBox(&p_box->data_info);
    fillVideoSampleTableBox(&p_box->sample_table);
}

void CMP4Muxer::fillAudioMediaInformationBox(SISOMMediaInformationBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_MEDIA_INFORMATION_BOX_TAG, DISOM_TAG_SIZE);
    fillSoundMediaHeaderBox(&p_box->sound_header);
    fillDataInformationBox(&p_box->data_info);
    fillSoundSampleTableBox(&p_box->sample_table);
}

void CMP4Muxer::fillVideoMediaBox(SISOMMediaBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_MEDIA_BOX_TAG, DISOM_TAG_SIZE);

    fillMediaHeaderBox(&p_box->media_header);

    fillHanderReferenceBox(&p_box->media_handler, DISOM_VIDEO_HANDLER_REFERENCE_TAG, NULL);

    fillVideoMediaInformationBox(&p_box->media_info);
}

void CMP4Muxer::fillAudioMediaBox(SISOMMediaBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_MEDIA_BOX_TAG, DISOM_TAG_SIZE);

    fillMediaHeaderBox(&p_box->media_header);

    fillHanderReferenceBox(&p_box->media_handler, DISOM_AUDIO_HANDLER_REFERENCE_TAG, NULL);

    fillAudioMediaInformationBox(&p_box->media_info);
}

void CMP4Muxer::fillMovieVideoTrackBox(SISOMTrackBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_TRACK_BOX_TAG, DISOM_TAG_SIZE);

    fillMovieVideoTrackHeaderBox(&p_box->track_header);

    fillVideoMediaBox(&p_box->media);
}

void CMP4Muxer::fillMovieAudioTrackBox(SISOMTrackBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_TRACK_BOX_TAG, DISOM_TAG_SIZE);

    fillMovieAudioTrackHeaderBox(&p_box->track_header);

    fillAudioMediaBox(&p_box->media);
}

void CMP4Muxer::fillMovieBox(SISOMMovieBox *p_box)
{
    memcpy(p_box->base_box.type, DISOM_MOVIE_BOX_TAG, DISOM_TAG_SIZE);

    fillMovieHeaderBox(&p_box->movie_header_box);
    p_box->base_box.size = sizeof(SISOMBox) + p_box->movie_header_box.base_full_box.base_box.size;


    if (mbVideoEnabled) {
        fillMovieVideoTrackBox(&p_box->video_track);
        p_box->base_box.size += p_box->video_track.base_box.size;
    }

    if (mbAudioEnabled) {
        fillMovieAudioTrackBox(&p_box->audio_track);
        p_box->base_box.size += p_box->audio_track.base_box.size;
    }
}

void CMP4Muxer::updateChunkOffsetBoxSize(SISOMChunkOffsetBox *p_box)
{
    p_box->base_full_box.base_box.size = DISOM_FULL_BOX_SIZE + sizeof(TU32) + p_box->entry_count * sizeof(TU32);
}

void CMP4Muxer::updateSampleToChunkBoxSize(SISOMSampleToChunkBox *p_box)
{
    p_box->base_full_box.base_box.size = DISOM_FULL_BOX_SIZE + sizeof(TU32) + p_box->entry_count * sizeof(_SSampleToChunkEntry);
}

void CMP4Muxer::updateSampleSizeBoxSize(SISOMSampleSizeBox *p_box)
{
    p_box->base_full_box.base_box.size = DISOM_FULL_BOX_SIZE + sizeof(TU32) + sizeof(TU32) + p_box->sample_count * sizeof(TU32);
}

void CMP4Muxer::updateDecodingTimeToSampleBoxSize(SISOMDecodingTimeToSampleBox *p_box)
{
    p_box->base_full_box.base_box.size = DISOM_FULL_BOX_SIZE + sizeof(TU32) + p_box->entry_count * sizeof(_STimeEntry);
}

void CMP4Muxer::updateVideoSampleTableBoxSize(SISOMSampleTableBox *p_box)
{
    updateDecodingTimeToSampleBoxSize(&p_box->stts);
    updateSampleSizeBoxSize(&p_box->sample_size);
    updateSampleToChunkBoxSize(&p_box->sample_to_chunk);
    updateChunkOffsetBoxSize(&p_box->chunk_offset);

    p_box->base_box.size = DISOM_BOX_SIZE + \
                           p_box->stts.base_full_box.base_box.size + \
                           p_box->visual_sample_description.base_full_box.base_box.size + \
                           p_box->sample_size.base_full_box.base_box.size + \
                           p_box->sample_to_chunk.base_full_box.base_box.size + \
                           p_box->chunk_offset.base_full_box.base_box.size;

}

void CMP4Muxer::updateSoundSampleTableBoxSize(SISOMSampleTableBox *p_box)
{
    updateDecodingTimeToSampleBoxSize(&p_box->stts);
    updateSampleSizeBoxSize(&p_box->sample_size);
    updateSampleToChunkBoxSize(&p_box->sample_to_chunk);
    updateChunkOffsetBoxSize(&p_box->chunk_offset);

    p_box->base_box.size = DISOM_BOX_SIZE + \
                           p_box->stts.base_full_box.base_box.size + \
                           p_box->audio_sample_description.base_full_box.base_box.size + \
                           p_box->sample_size.base_full_box.base_box.size + \
                           p_box->sample_to_chunk.base_full_box.base_box.size + \
                           p_box->chunk_offset.base_full_box.base_box.size;
}

void CMP4Muxer::updateVideoMediaInformationBoxSize(SISOMMediaInformationBox *p_box)
{
    updateVideoSampleTableBoxSize(&p_box->sample_table);

    p_box->base_box.size = DISOM_BOX_SIZE + p_box->sample_table.base_box.size + p_box->video_header.base_full_box.base_box.size + p_box->data_info.base_box.size;
}

void CMP4Muxer::updateAudioMediaInformationBoxSize(SISOMMediaInformationBox *p_box)
{
    updateSoundSampleTableBoxSize(&p_box->sample_table);

    p_box->base_box.size = DISOM_BOX_SIZE + p_box->sample_table.base_box.size + p_box->sound_header.base_full_box.base_box.size + p_box->data_info.base_box.size;
}

void CMP4Muxer::updateVideoMediaBoxSize(SISOMMediaBox *p_box)
{
    updateVideoMediaInformationBoxSize(&p_box->media_info);

    p_box->base_box.size = DISOM_BOX_SIZE + \
                           p_box->media_header.base_full_box.base_box.size + \
                           p_box->media_handler.base_full_box.base_box.size + \
                           p_box->media_info.base_box.size;
}

void CMP4Muxer::updateAudioMediaBoxSize(SISOMMediaBox *p_box)
{
    updateAudioMediaInformationBoxSize(&p_box->media_info);

    p_box->base_box.size = DISOM_BOX_SIZE + \
                           p_box->media_header.base_full_box.base_box.size + \
                           p_box->media_handler.base_full_box.base_box.size + \
                           p_box->media_info.base_box.size;
}

void CMP4Muxer::updateMovieVideoTrackBoxSize(SISOMTrackBox *p_box)
{
    updateVideoMediaBoxSize(&p_box->media);

    p_box->base_box.size = DISOM_BOX_SIZE + \
                           p_box->track_header.base_full_box.base_box.size + \
                           p_box->media.base_box.size;
}

void CMP4Muxer::updateMovieAudioTrackBoxSize(SISOMTrackBox *p_box)
{
    updateAudioMediaBoxSize(&p_box->media);

    p_box->base_box.size = DISOM_BOX_SIZE + \
                           p_box->track_header.base_full_box.base_box.size + \
                           p_box->media.base_box.size;
}

void CMP4Muxer::updateMovieBoxSize(SISOMMovieBox *p_box)
{
    TU32 size = DISOM_BOX_SIZE + p_box->movie_header_box.base_full_box.base_box.size;

    if (mbVideoEnabled) {
        updateMovieVideoTrackBoxSize(&p_box->video_track);
        size += p_box->video_track.base_box.size;
    }

    if (mbAudioEnabled) {
        updateMovieAudioTrackBoxSize(&p_box->audio_track);
        size += p_box->audio_track.base_box.size;
    }

    p_box->base_box.size = size;
}

void CMP4Muxer::writeBaseBox(SISOMBox *p_box)
{
    writeBE32(p_box->size);
    writeData((TU8 *)p_box->type, DISOM_TAG_SIZE);
}

void CMP4Muxer::writeFullBox(SISOMFullBox *p_box)
{
    writeBE32(p_box->base_box.size);
    writeData((TU8 *)p_box->base_box.type, DISOM_TAG_SIZE);
    writeBE32(((TU32)p_box->version << 24) | ((TU32)p_box->flags));
}

void CMP4Muxer::writeFileTypeBox(SISOMFileTypeBox *p_box)
{
    DCHECK_BOX_BEGIN;

    writeBaseBox(&p_box->base_box);

    writeData((TU8 *)p_box->major_brand, DISOM_TAG_SIZE);
    writeBE32(p_box->minor_version);
    for (TU32 i = 0; i < p_box->compatible_brands_number; i ++) {
        writeData((TU8 *)p_box->compatible_brands[i], DISOM_TAG_SIZE);
    }

    DCHECK_BOX_END
}

void CMP4Muxer::writeMediaDataBox(SISOMMediaDataBox *p_box)
{
    writeBaseBox(&p_box->base_box);
}

void CMP4Muxer::writeMovieHeaderBox(SISOMMovieHeaderBox *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);

    if (DLikely(!p_box->base_full_box.version)) {
        writeBE32((TU32)p_box->creation_time);
        writeBE32((TU32)p_box->modification_time);
        writeBE32(p_box->timescale);
        writeBE32((TU32)p_box->duration);
    } else {
        writeBE64(p_box->creation_time);
        writeBE64(p_box->modification_time);
        writeBE32(p_box->timescale);
        writeBE64(p_box->duration);
    }

    writeBE32(p_box->rate);
    writeBE16(p_box->volume);
    writeBE16(p_box->reserved);
    writeBE32(p_box->reserved_1);
    writeBE32(p_box->reserved_2);

    for (TU32 i = 0; i < 9; i ++) {
        writeBE32(p_box->matrix[i]);
    }

    for (TU32 i = 0; i < 6; i ++) {
        writeBE32(p_box->pre_defined[i]);
    }

    writeBE32(p_box->next_track_ID);

    DCHECK_BOX_END
}

void CMP4Muxer::writeMovieTrackHeaderBox(SISOMTrackHeader *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);

    if (DLikely(!p_box->base_full_box.version)) {
        writeBE32((TU32)p_box->creation_time);
        writeBE32((TU32)p_box->modification_time);
        writeBE32(p_box->track_ID);
        writeBE32(p_box->reserved);
        writeBE32((TU32)p_box->duration);
    } else {
        writeBE64(p_box->creation_time);
        writeBE64(p_box->modification_time);
        writeBE32(p_box->track_ID);
        writeBE32(p_box->reserved);
        writeBE64(p_box->duration);
    }

    for (TU32 i = 0; i < 2; i ++) {
        writeBE32(p_box->reserved_1[i]);
    }

    writeBE16(p_box->layer);
    writeBE16(p_box->alternate_group);
    writeBE16(p_box->volume);
    writeBE16(p_box->reserved_2);

    for (TU32 i = 0; i < 9; i ++) {
        writeBE32(p_box->matrix[i]);
    }

    writeBE32(p_box->width);
    writeBE32(p_box->height);

    DCHECK_BOX_END
}

void CMP4Muxer::writeMediaHeaderBox(SISOMMediaHeaderBox *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);

    if (DLikely(!p_box->base_full_box.version)) {
        writeBE32((TU32)p_box->creation_time);
        writeBE32((TU32)p_box->modification_time);
        writeBE32(p_box->timescale);
        writeBE32((TU32)p_box->duration);
    } else {
        writeBE64(p_box->creation_time);
        writeBE64(p_box->modification_time);
        writeBE32(p_box->timescale);
        writeBE64(p_box->duration);
    }

    writeBE32(((TU32)p_box->pad << 31) | ((TU32)p_box->language[0] << 26) | ((TU32)p_box->language[1] << 21) | ((TU32)p_box->language[2] << 16) | ((TU32)p_box->pre_defined));

    DCHECK_BOX_END
}

void CMP4Muxer::writeHanderReferenceBox(SISOMHandlerReferenceBox *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);
    writeBE32(p_box->pre_defined);
    writeData((TU8 *)p_box->handler_type, DISOM_TAG_SIZE);

    for (TU32 i = 0; i < 3; i ++) {
        writeBE32(p_box->reserved[i]);
    }

    if (p_box->name) {
        writeData((TU8 *)p_box->name, strlen(p_box->name));
    }
    TU8 t = 0;
    writeData(&t, 1);

    DCHECK_BOX_END
}

void CMP4Muxer::writeChunkOffsetBox(SISOMChunkOffsetBox *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);

    writeBE32(p_box->entry_count);
    for (TU32 i = 0; i < p_box->entry_count; i ++) {
        writeBE32(p_box->chunk_offset[i]);
    }

    DCHECK_BOX_END
}

void CMP4Muxer::writeSampleToChunkBox(SISOMSampleToChunkBox *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);

    writeBE32(p_box->entry_count);
    for (TU32 i = 0; i < p_box->entry_count; i ++) {
        writeBE32(p_box->entrys[i].first_chunk);
        writeBE32(p_box->entrys[i].sample_per_chunk);
        writeBE32(p_box->entrys[i].sample_description_index);
    }

    DCHECK_BOX_END
}

void CMP4Muxer::writeSampleSizeBox(SISOMSampleSizeBox *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);

    writeBE32(p_box->sample_size);
    writeBE32(p_box->sample_count);
    for (TU32 i = 0; i < p_box->sample_count; i ++) {
        writeBE32(p_box->entry_size[i]);
    }

    DCHECK_BOX_END
}

void CMP4Muxer::writeAVCDecoderConfigurationRecordBox(SISOMAVCDecoderConfigurationRecordBox *p_box)
{
    DCHECK_BOX_BEGIN;

    writeBaseBox(&p_box->base_box);

    writeU8(p_box->configurationVersion);
    writeU8(p_box->p_sps[1]);
    writeU8(p_box->p_sps[2]);
    writeU8(p_box->p_sps[3]);

    writeBE16(((TU16) p_box->reserved << 10) | ((TU16) p_box->lengthSizeMinusOne << 8) | ((TU16) p_box->reserved_1 << 5) | ((TU16) p_box->numOfSequenceParameterSets));
    writeBE16(p_box->sequenceParametersSetLength);
    writeData(p_box->p_sps, p_box->sequenceParametersSetLength);

    writeData(&p_box->numOfPictureParameterSets, 1);
    writeBE16(p_box->pictureParametersSetLength);
    writeData(p_box->p_pps, p_box->pictureParametersSetLength);

    DCHECK_BOX_END
}

void CMP4Muxer::writeHEVCDecoderConfigurationRecordBox(SISOMHEVCDecoderConfigurationRecordBox *p_box)
{
    DCHECK_BOX_BEGIN;

    writeBaseBox(&p_box->base_box);

    writeU8(p_box->record.configurationVersion);
    writeU8(((p_box->record.general_profile_space & 0x03) << 6) | ((p_box->record.general_tier_flag & 0x01) << 5) | (p_box->record.general_profile_idc & 0x1f));
    writeBE32(p_box->record.general_profile_compatibility_flags);
    writeBE16((TU16)(((p_box->record.general_constraint_indicator_flags) >> 32) & 0xffff));
    writeBE32((TU32)((p_box->record.general_constraint_indicator_flags) & 0xffffffff));
    writeU8(p_box->record.general_level_idc);

    writeBE16(p_box->record.min_spatial_segmentation_idc | 0xf000);
    writeU8(p_box->record.parallelismType | 0xfc);// 16
    //writeBE16(0xf000);
    //writeU8(0xfc);// 16

    writeU8(p_box->record.chromaFormat | 0xfc);
    writeU8(p_box->record.bitDepthLumaMinus8 | 0xf8);
    writeU8(p_box->record.bitDepthChromaMinus8 | 0xf8);

    writeBE16(p_box->record.avgFrameRate);// 21
    writeU8(((p_box->record.constantFrameRate & 0x03) << 6) | ((p_box->record.numTemporalLayers & 0x07) << 3) | ((p_box->record.temporalIdNested & 0x01) << 2) | (p_box->record.lengthSizeMinusOne & 0x03));
    writeU8(p_box->record.numOfArrays);

    writeU8(0x80 | EHEVCNalType_VPS);// 24

    writeBE16(1);
    writeBE16(p_box->vps_length);
    writeData(p_box->vps, p_box->vps_length);

    writeU8(0x80 | EHEVCNalType_SPS);
    writeBE16(1);
    writeBE16(p_box->sps_length);
    writeData(p_box->sps, p_box->sps_length);

    writeU8(0x80 | EHEVCNalType_PPS);
    writeBE16(1);
    writeBE16(p_box->pps_length);
    writeData(p_box->pps, p_box->pps_length);

    DCHECK_BOX_END
}

void CMP4Muxer::writeVisualSampleDescriptionBox(SISOMVisualSampleDescriptionBox *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);

    writeBE32(p_box->entry_count);

    writeBaseBox(&p_box->visual_entry.base_box);
    writeData(p_box->visual_entry.reserved_0, 6);
    writeBE16(p_box->visual_entry.data_reference_index);
    writeBE16(p_box->visual_entry.pre_defined);
    writeBE16(p_box->visual_entry.reserved);

    for (TU32 i = 0; i < 3; i ++) {
        writeBE32(p_box->visual_entry.pre_defined_1[i]);
    }

    writeBE16(p_box->visual_entry.width);
    writeBE16(p_box->visual_entry.height);

    writeBE32(p_box->visual_entry.horizresolution);
    writeBE32(p_box->visual_entry.vertresolution);
    writeBE32(p_box->visual_entry.reserved_1);
    writeBE16(p_box->visual_entry.frame_count);
    writeData((TU8 *)p_box->visual_entry.compressorname, 32);
    writeBE16(p_box->visual_entry.depth);
    writeBE16(p_box->visual_entry.pre_defined_2);

    if (StreamFormat_H264 == mVideoFormat) {
        writeAVCDecoderConfigurationRecordBox(&p_box->visual_entry.avc_config);
    } else if (StreamFormat_H265 == mVideoFormat) {
        writeHEVCDecoderConfigurationRecordBox(&p_box->visual_entry.hevc_config);
    } else {
        LOG_FATAL("writeVisualSampleDescriptionBox(), should not comes here, mVideoFormat %d\n", mVideoFormat);
    }

    DCHECK_BOX_END
}

void CMP4Muxer::writeAACElementaryStreamDescriptionBox(SISOMAACElementaryStreamDescriptor *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);

    writeU8(p_box->es_descriptor_type_tag);
    writeStrangeSize(p_box->strange_size_0);
    writeBE16(p_box->track_id);
    writeU8(p_box->flags);

    writeU8(p_box->decoder_descriptor_type_tag);
    writeStrangeSize(p_box->strange_size_1);
    writeU8(p_box->object_type);
    writeU8(p_box->stream_flag);
    writeBE24(p_box->buffer_size);
    writeBE32(p_box->max_bitrate);
    writeBE32(p_box->avg_bitrate);

    writeU8(p_box->decoder_config_type_tag);
    writeStrangeSize(p_box->strange_size_2);
    //writeData(mpAudioExtraData, mAudioExtraDataLength);
    writeData(p_box->config, 2);

    writeU8(p_box->SL_descriptor_type_tag);
    writeStrangeSize(p_box->strange_size_3);
    writeU8(p_box->SL_value);

    DCHECK_BOX_END
}

void CMP4Muxer::writeAudioSampleDescriptionBox(SISOMAudioSampleDescriptionBox *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);

    writeBE32(p_box->entry_count);

    writeBaseBox(&p_box->audio_entry.base_box);
    writeData(p_box->audio_entry.reserved_0, 6);
    writeBE16(p_box->audio_entry.data_reference_index);

    for (TU32 i = 0; i < 2; i ++) {
        writeBE32(p_box->audio_entry.reserved[i]);
    }

    writeBE16(p_box->audio_entry.channelcount);
    writeBE16(p_box->audio_entry.samplesize);
    writeBE16(p_box->audio_entry.pre_defined);
    writeBE16(p_box->audio_entry.reserved_1);
    writeBE32(p_box->audio_entry.samplerate);

    writeAACElementaryStreamDescriptionBox(&p_box->audio_entry.esd);

    DCHECK_BOX_END
}

void CMP4Muxer::writeDecodingTimeToSampleBox(SISOMDecodingTimeToSampleBox *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);

    writeBE32(p_box->entry_count);
    for (TU32 i = 0; i < p_box->entry_count; i ++) {
        writeBE32(p_box->p_entry[i].sample_count);
        writeBE32(p_box->p_entry[i].sample_delta);
    }

    DCHECK_BOX_END
}

void CMP4Muxer::writeVideoSampleTableBox(SISOMSampleTableBox *p_box)
{
    DCHECK_BOX_BEGIN;

    writeBaseBox(&p_box->base_box);

    writeVisualSampleDescriptionBox(&p_box->visual_sample_description);
    writeDecodingTimeToSampleBox(&p_box->stts);
    writeSampleSizeBox(&p_box->sample_size);
    writeSampleToChunkBox(&p_box->sample_to_chunk);
    writeChunkOffsetBox(&p_box->chunk_offset);

    DCHECK_BOX_END
}

void CMP4Muxer::writeSoundSampleTableBox(SISOMSampleTableBox *p_box)
{
    DCHECK_BOX_BEGIN;

    writeBaseBox(&p_box->base_box);

    writeAudioSampleDescriptionBox(&p_box->audio_sample_description);
    writeDecodingTimeToSampleBox(&p_box->stts);
    writeSampleSizeBox(&p_box->sample_size);
    writeSampleToChunkBox(&p_box->sample_to_chunk);
    writeChunkOffsetBox(&p_box->chunk_offset);

    DCHECK_BOX_END
}

void CMP4Muxer::writeDataReferenceBox(SISOMDataReferenceBox *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);

    writeBE32(p_box->entry_count);
    for (TU32 i = 0; i < p_box->entry_count; i ++) {
        writeFullBox(&p_box->url);
    }

    DCHECK_BOX_END
}

void CMP4Muxer::writeDataInformationBox(SISOMDataInformationBox *p_box)
{
    DCHECK_BOX_BEGIN;

    writeBaseBox(&p_box->base_box);

    writeDataReferenceBox(&p_box->data_ref);

    DCHECK_BOX_END
}

void CMP4Muxer::writeVideoMediaHeaderBox(SISOMVideoMediaHeaderBox *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);

    writeBE16(p_box->graphicsmode);
    writeBE16(p_box->opcolor[0]);
    writeBE16(p_box->opcolor[1]);
    writeBE16(p_box->opcolor[2]);

    DCHECK_BOX_END
}

void CMP4Muxer::writeSoundMediaHeaderBox(SISOMSoundMediaHeaderBox *p_box)
{
    DCHECK_FULL_BOX_BEGIN;

    writeFullBox(&p_box->base_full_box);

    writeBE16(p_box->balanse);
    writeBE16(p_box->reserved);

    DCHECK_BOX_END
}

void CMP4Muxer::writeVideoMediaInformationBox(SISOMMediaInformationBox *p_box)
{
    DCHECK_BOX_BEGIN;

    writeBaseBox(&p_box->base_box);

    writeVideoMediaHeaderBox(&p_box->video_header);
    writeDataInformationBox(&p_box->data_info);
    writeVideoSampleTableBox(&p_box->sample_table);

    DCHECK_BOX_END
}

void CMP4Muxer::writeAudioMediaInformationBox(SISOMMediaInformationBox *p_box)
{
    DCHECK_BOX_BEGIN;

    writeBaseBox(&p_box->base_box);

    writeSoundMediaHeaderBox(&p_box->sound_header);
    writeDataInformationBox(&p_box->data_info);
    writeSoundSampleTableBox(&p_box->sample_table);

    DCHECK_BOX_END
}

void CMP4Muxer::writeVideoMediaBox(SISOMMediaBox *p_box)
{
    DCHECK_BOX_BEGIN;

    writeBaseBox(&p_box->base_box);

    writeMediaHeaderBox(&p_box->media_header);

    writeHanderReferenceBox(&p_box->media_handler);

    writeVideoMediaInformationBox(&p_box->media_info);

    DCHECK_BOX_END
}

void CMP4Muxer::writeAudioMediaBox(SISOMMediaBox *p_box)
{
    DCHECK_BOX_BEGIN;

    writeBaseBox(&p_box->base_box);

    writeMediaHeaderBox(&p_box->media_header);

    writeHanderReferenceBox(&p_box->media_handler);

    writeAudioMediaInformationBox(&p_box->media_info);

    DCHECK_BOX_END
}

void CMP4Muxer::writeMovieVideoTrackBox(SISOMTrackBox *p_box)
{
    DCHECK_BOX_BEGIN;

    writeBaseBox(&p_box->base_box);

    writeMovieTrackHeaderBox(&p_box->track_header);

    writeVideoMediaBox(&p_box->media);

    DCHECK_BOX_END
}

void CMP4Muxer::writeMovieAudioTrackBox(SISOMTrackBox *p_box)
{
    DCHECK_BOX_BEGIN;

    writeBaseBox(&p_box->base_box);

    writeMovieTrackHeaderBox(&p_box->track_header);

    writeAudioMediaBox(&p_box->media);

    DCHECK_BOX_END
}

void CMP4Muxer::writeMovieBox(SISOMMovieBox *p_box)
{
    DCHECK_BOX_BEGIN;

    writeBaseBox(&p_box->base_box);

    writeMovieHeaderBox(&p_box->movie_header_box);

    if (mbVideoEnabled) {
        writeMovieVideoTrackBox(&p_box->video_track);
    }

    if (mbAudioEnabled) {
        writeMovieAudioTrackBox(&p_box->audio_track);
    }

    DCHECK_BOX_END
}

void CMP4Muxer::insertVideoChunkOffsetBox(SISOMChunkOffsetBox *p_box, TU64 offset)
{
    if (p_box->entry_count < p_box->entry_buf_count) {
        p_box->chunk_offset[p_box->entry_count] = (TU32) offset;
        p_box->entry_count ++;
    } else {
        TU32 *ptmp = (TU32 *) DDBG_MALLOC((p_box->entry_buf_count + DVIDEO_FRAME_COUNT) * sizeof(TU32), "M4VC");
        if (ptmp) {
            if (p_box->entry_buf_count && p_box->chunk_offset) {
                memcpy(ptmp, p_box->chunk_offset, p_box->entry_buf_count * sizeof(TU32));
                DDBG_FREE(p_box->chunk_offset, "M4VC");
            }
            p_box->entry_buf_count += DVIDEO_FRAME_COUNT;
            p_box->chunk_offset = ptmp;
            p_box->chunk_offset[p_box->entry_count] = (TU32) offset;
            p_box->entry_count ++;
        } else {
            LOG_FATAL("no memory\n");
        }
    }
}

void CMP4Muxer::insertAudioChunkOffsetBox(SISOMChunkOffsetBox *p_box, TU64 offset)
{
    if (p_box->entry_count < p_box->entry_buf_count) {
        p_box->chunk_offset[p_box->entry_count] = (TU32) offset;
        p_box->entry_count ++;
    } else {
        TU32 *ptmp = (TU32 *) DDBG_MALLOC((p_box->entry_buf_count + DAUDIO_FRAME_COUNT) * sizeof(TU32), "M4AC");
        if (ptmp) {
            if (p_box->entry_buf_count && p_box->chunk_offset) {
                memcpy(ptmp, p_box->chunk_offset, p_box->entry_buf_count * sizeof(TU32));
                DDBG_FREE(p_box->chunk_offset, "M4AC");
            }
            p_box->entry_buf_count += DAUDIO_FRAME_COUNT;
            p_box->chunk_offset = ptmp;
            p_box->chunk_offset[p_box->entry_count] = (TU32) offset;
            p_box->entry_count ++;
        } else {
            LOG_FATAL("no memory\n");
        }
    }
}

void CMP4Muxer::insertVideoSampleSizeBox(SISOMSampleSizeBox *p_box, TU32 size)
{
    if (p_box->sample_count < p_box->entry_buf_count) {
        p_box->entry_size[p_box->sample_count] = (TU32) size;
        p_box->sample_count ++;
    } else {
        TU32 *ptmp = (TU32 *) DDBG_MALLOC((p_box->entry_buf_count + DVIDEO_FRAME_COUNT) * sizeof(TU32), "M4VS");
        if (ptmp) {
            if (p_box->entry_buf_count && p_box->entry_size) {
                memcpy(ptmp, p_box->entry_size, p_box->entry_buf_count * sizeof(TU32));
                DDBG_FREE(p_box->entry_size, "M4VS");
            }
            p_box->entry_buf_count += DVIDEO_FRAME_COUNT;
            p_box->entry_size = ptmp;
            p_box->entry_size[p_box->sample_count] = (TU32) size;
            p_box->sample_count ++;
        } else {
            LOG_FATAL("no memory\n");
        }
    }
}

void CMP4Muxer::insertAudioSampleSizeBox(SISOMSampleSizeBox *p_box, TU32 size)
{
    if (p_box->sample_count < p_box->entry_buf_count) {
        p_box->entry_size[p_box->sample_count] = (TU32) size;
        p_box->sample_count ++;
    } else {
        TU32 *ptmp = (TU32 *) DDBG_MALLOC((p_box->entry_buf_count + DAUDIO_FRAME_COUNT) * sizeof(TU32), "M4AS");
        if (ptmp) {
            if (p_box->entry_buf_count && p_box->entry_size) {
                memcpy(ptmp, p_box->entry_size, p_box->entry_buf_count * sizeof(TU32));
                DDBG_FREE(p_box->entry_size, "M4AS");
            }
            p_box->entry_buf_count += DAUDIO_FRAME_COUNT;
            p_box->entry_size = ptmp;
            p_box->entry_size[p_box->sample_count] = (TU32) size;
            p_box->sample_count ++;
        } else {
            LOG_FATAL("no memory\n");
        }
    }
}

EECode CMP4Muxer::writeStrangeSize(TU32 size)
{
    TU8 buf[4] = {0};
    buf[0] = (size >> 21) | 0x80;
    buf[1] = (size >> 14) | 0x80;
    buf[2] = (size >> 7) | 0x80;
    buf[3] = size & 0x7f;

    return writeData(buf, 4);
}

EECode CMP4Muxer::writeData(TU8 *pdata, TU64 size)
{
    mCurFileOffset += size;

    if (DLikely((TU64)(mpCurrentPosInCache + size) < (TU64)mpCacheBufferEnd)) {
        memcpy(mpCurrentPosInCache, pdata, size);
        mpCurrentPosInCache += size;
    } else {
        TU64 copy_size = (TU64)(mpCacheBufferEnd - mpCurrentPosInCache);
        memcpy(mpCurrentPosInCache, pdata, copy_size);
        mpIO->Write(mpCacheBufferBaseAligned, 1, mCacheBufferSize);
        pdata += copy_size;
        size -= copy_size;
        while (size > mCacheBufferSize) {
            mpIO->Write(pdata, 1, mCacheBufferSize);
            pdata += mCacheBufferSize;
            size -= mCacheBufferSize;
        }
        memcpy(mpCacheBufferBaseAligned, pdata, size);
        mpCurrentPosInCache = mpCacheBufferBaseAligned + size;
    }

    return EECode_OK;
}

void CMP4Muxer::writeCachedData()
{

    TU64 data_size = (TU64)(mpCurrentPosInCache - mpCacheBufferBaseAligned);
    if (DLikely(data_size)) {
        mpIO->Write(mpCacheBufferBaseAligned, 1, data_size);
    }

    mpCurrentPosInCache = mpCacheBufferBaseAligned;
}

void CMP4Muxer::writeVideoFrame(TU64 offset, TU64 size)
{
    insertVideoChunkOffsetBox(&mMovieBox.video_track.media.media_info.sample_table.chunk_offset, offset);
    insertVideoSampleSizeBox(&mMovieBox.video_track.media.media_info.sample_table.sample_size, size);
}

void CMP4Muxer::writeAudioFrame(TU64 offset, TU64 size)
{
    insertAudioChunkOffsetBox(&mMovieBox.audio_track.media.media_info.sample_table.chunk_offset, offset);
    insertAudioSampleSizeBox(&mMovieBox.audio_track.media.media_info.sample_table.sample_size, size);
}

void CMP4Muxer::writeU8(TU8 value)
{
    mCurFileOffset ++;

    if (DLikely((TU64)(mpCurrentPosInCache + 1) < (TU64)mpCacheBufferEnd)) {
        *mpCurrentPosInCache = value;
        mpCurrentPosInCache ++;
    } else {
        mpIO->Write(mpCacheBufferBaseAligned, 1, mCacheBufferSize);
        mpCurrentPosInCache = mpCacheBufferBaseAligned;
        *mpCurrentPosInCache = value;
        mpCurrentPosInCache ++;
    }
}

EECode CMP4Muxer::writeBE16(TU16 value)
{
    TU8 buf[4];
    DBEW16(value, buf);
    return writeData(buf, 2);
}

EECode CMP4Muxer::writeBE24(TU32 value)
{
    TU8 buf[4];
    DBEW32(value, buf);
    return writeData(buf + 1, 3);
}

EECode CMP4Muxer::writeBE32(TU32 value)
{
    TU8 buf[4];
    DBEW32(value, buf);
    return writeData(buf, 4);
}

EECode CMP4Muxer::writeBE64(TU64 value)
{
    TU8 buf[8];
    DBEW64(value, buf);
    return writeData(buf, 8);
}

void CMP4Muxer::updateMeidaBoxSize()
{
    TU64 offset = mFileTypeBox.base_box.size;
    TU8 buf[4] = {0};

    mMediaDataBox.base_box.size += (TU32) mOverallMediaDataLen;

    mpIO->Seek(offset, EIOPosition_Begin);
    DBEW32(mMediaDataBox.base_box.size, buf);
    mpIO->Write(buf, 1, 4);
    mpIO->Sync();
}

void CMP4Muxer::writeHeader()
{
    writeFileTypeBox(&mFileTypeBox);
    writeMediaDataBox(&mMediaDataBox);
}

void CMP4Muxer::writeTail()
{
    if (mVideoDuration > mAudioDuration) {
        mDuration = mVideoDuration;
    } else {
        mDuration = mAudioDuration;
    }

    fillMovieBox(&mMovieBox);
    updateMovieBoxSize(&mMovieBox);
    writeMovieBox(&mMovieBox);
}

void CMP4Muxer::beginFile()
{
    fillFileTypeBox(&mFileTypeBox);
    fillMediaDataBox(&mMediaDataBox);
    writeHeader();
    mbWriteMediaDataStarted = 1;
}

void CMP4Muxer::finalizeFile()
{
    if (DLikely(mbIOOpend)) {
        mbIOOpend = 0;
        if (DLikely(mbWriteMediaDataStarted)) {
            mbWriteMediaDataStarted = 0;

            if (mbCorrupted) {
                LOG_ERROR("corrupted file\n");
                return;
            }

            writeTail();
            writeCachedData();

            mpIO->Sync();
            updateMeidaBoxSize();
        }
        mpIO->Close();
    }
}

void CMP4Muxer::processAVCExtradata(TU8 *pdata, TU32 len)
{
    TU8 *p_sps = NULL, *p_pps = NULL;
    TU32 sps_size = 0, pps_size = 0;

    EECode err = gfGetH264SPSPPS(pdata, len, p_sps, sps_size, p_pps, pps_size);

    if (DLikely(EECode_OK == err)) {
        SCodecVideoCommon *v = gfGetVideoCodecParser(p_sps + 5, (TMemSize) sps_size - 5,  StreamFormat_H264, err);
        if (DLikely(EECode_OK == err)) {
            mVideoWidth = v->max_width;
            mVideoHeight = v->max_height;
        }
        if (v) {
            gfReleaseVideoCodecParser(v);
        }

        mVideoSPSLength = sps_size - 4;
        if (mpVideoSPS) {
            DDBG_FREE(mpVideoSPS, "M4SP");
            mpVideoSPS = NULL;
        }
        mpVideoSPS = (TU8 *) DDBG_MALLOC(mVideoSPSLength, "M4SP");
        if (mpVideoSPS) {
            memcpy(mpVideoSPS, p_sps + 4, mVideoSPSLength);
        }

        mVideoPPSLength = pps_size - 4;
        if (mpVideoPPS) {
            DDBG_FREE(mpVideoPPS, "M4PP");
            mpVideoPPS = NULL;
        }
        mpVideoPPS = (TU8 *) DDBG_MALLOC(mVideoPPSLength, "M4PP");
        if (mpVideoPPS) {
            memcpy(mpVideoPPS, p_pps + 4, mVideoPPSLength);
        }
    }

}

void CMP4Muxer::processHEVCExtradata(TU8 *pdata, TU32 len)
{
    TU8 *p_vps = NULL, *p_sps = NULL, *p_pps = NULL;
    TU32 vps_size = 0, sps_size = 0, pps_size = 0;

    EECode err = gfGetH265VPSSPSPPS(pdata, len, p_vps, vps_size, p_sps, sps_size, p_pps, pps_size);

    if (DLikely(EECode_OK == err)) {
        err = gfGenerateHEVCDecoderConfigurationRecord(&mMovieBox.video_track.media.media_info.sample_table.visual_sample_description.visual_entry.hevc_config.record, p_vps + 4, vps_size - 4, p_sps + 4, sps_size - 4, p_pps + 4, pps_size - 4, mVideoWidth, mVideoHeight);
        if (DLikely(EECode_OK != err)) {
            LOG_ERROR("gfGenerateHEVCDecoderConfigurationRecord() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        }

        mVideoVPSLength = vps_size - 4;
        if (mpVideoVPS) {
            DDBG_FREE(mpVideoVPS, "M4VP");
            mpVideoVPS = NULL;
        }
        mpVideoVPS = (TU8 *) DDBG_MALLOC(mVideoVPSLength, "M4VP");
        if (mpVideoVPS) {
            memcpy(mpVideoVPS, p_vps + 4, mVideoVPSLength);
        }

        mVideoSPSLength = sps_size - 4;
        if (mpVideoSPS) {
            DDBG_FREE(mpVideoSPS, "M4SP");
            mpVideoSPS = NULL;
        }
        mpVideoSPS = (TU8 *) DDBG_MALLOC(mVideoSPSLength, "M4SP");
        if (mpVideoSPS) {
            memcpy(mpVideoSPS, p_sps + 4, mVideoSPSLength);
        }

        mVideoPPSLength = pps_size - 4;
        if (mpVideoPPS) {
            DDBG_FREE(mpVideoPPS, "M4PP");
            mpVideoPPS = NULL;
        }
        mpVideoPPS = (TU8 *) DDBG_MALLOC(mVideoPPSLength, "M4PP");
        if (mpVideoPPS) {
            memcpy(mpVideoPPS, p_pps + 4, mVideoPPSLength);
        }
    }

}

void CMP4Muxer::parseADTSHeader(TU8 *pdata)
{
    SADTSHeader header;
    EECode err = gfParseADTSHeader(pdata, &header);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("not aac file? gfParseADTSHeader fail ret %d, %s\n", err, gfGetErrorCodeString(err));
        return;
    }

    mAudioChannelNumber = header.channel_configuration;
    mAudioSampleRate = gfGetADTSSamplingFrequency(header.sampling_frequency_index);
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


