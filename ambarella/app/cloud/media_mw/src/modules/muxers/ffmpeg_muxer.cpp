/*******************************************************************************
 * ffmpeg_muxer.cpp
 *
 * History:
 *    2013/7/20 - [Zhi He] create file
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

#ifdef BUILD_MODULE_FFMPEG

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"
extern "C" {
#ifndef INT64_C
#define INT64_C
#endif
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}

#include "ffmpeg_muxer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

IMuxer *gfCreateFFMpegMuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
    return CFFMpegMuxer::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

IMuxer *CFFMpegMuxer::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CFFMpegMuxer *result = new CFFMpegMuxer(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CFFMpegMuxer::Destroy()
{
    Delete();
}

CFFMpegMuxer::CFFMpegMuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mContainerType(ContainerType_TS)
    , mpUrlName(NULL)
    , mpAllocatedUrlName(NULL)
    , mpThumbNailFileName(NULL)
    , mnMaxStreamNumber(0)
    , mpFormat(NULL)
    , mpOutFormat(NULL)
    , mH264DataFmt()
    , mH264AVCCNaluLen(0)
    , mbConvertH264DataFormat(0)
    , mbSuspend(0)
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
    , mFileDuration(0)
    , mEstimatedBitrate(0)
    , mFileBitrate(0)
{
    TUint i = 0;

    memset(&mAVFormatParam, 0, sizeof(mAVFormatParam));

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        mpStream[i] = NULL;
        mpExtraData[i] = NULL;
        mExtraDataSize[i] = 0;
        mpExtraDataOri[i] = NULL;
        mExtraDataSizeOri[i] = 0;
    }

}

EECode CFFMpegMuxer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleFFMpegMuxer);

    avcodec_init();
    av_register_all();

    return EECode_OK;
}

void CFFMpegMuxer::Delete()
{
    TUint i = 0;

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpExtraDataOri[i]) {
            av_free(mpExtraDataOri[i]);
            mpExtraDataOri[i] = NULL;
        }
        if (mpExtraData[i]) {
            av_free(mpExtraData[i]);
            mpExtraData[i] = NULL;
        }
        mExtraDataSizeOri[i] = 0;
        mExtraDataSize[i] = 0;
    }
    if (mpConversionBuffer) {
        DDBG_FREE(mpConversionBuffer, "MXUR");
        mpConversionBuffer = NULL;
    }
    mConversionBufferSize = 0;
    mConversionDataSize = 0;
    inherited::Delete();
}

CFFMpegMuxer::~CFFMpegMuxer()
{
    TUint i = 0;

    if (mpAllocatedUrlName) {
        free(mpAllocatedUrlName);
        mpAllocatedUrlName = NULL;
    }

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpExtraDataOri[i]) {
            av_free(mpExtraDataOri[i]);
            mpExtraDataOri[i] = NULL;
        }
        if (mpExtraData[i]) {
            av_free(mpExtraData[i]);
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
}

EECode CFFMpegMuxer::SetupContext()
{
    return EECode_OK;
}

EECode CFFMpegMuxer::DestroyContext()
{
    return EECode_OK;
}

EECode CFFMpegMuxer::SetSpecifiedInfo(SRecordSpecifiedInfo *info)
{
    return EECode_OK;
}

EECode CFFMpegMuxer::GetSpecifiedInfo(SRecordSpecifiedInfo *info)
{
    return EECode_OK;
}

EECode CFFMpegMuxer::SetExtraData(StreamType type, TUint stream_index, void *p_data, TUint data_size)
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
                av_free(mpExtraData[stream_index]);
                mpExtraData[stream_index] = (TU8 *)av_malloc(data_size);
                if (mpExtraData[stream_index]) {
                    memcpy(mpExtraData[stream_index], p_data, data_size);
                    mExtraDataSize[stream_index] = data_size;
                } else {
                    LOGM_FATAL("alloc(%d) fail\n", data_size);
                    return EECode_NoMemory;
                }
            }
        } else {
            mpExtraData[stream_index] = (TU8 *)av_malloc(data_size);
            if (mpExtraData[stream_index]) {
                memcpy(mpExtraData[stream_index], p_data, data_size);
                mExtraDataSize[stream_index] = data_size;
            } else {
                LOGM_FATAL("alloc(%d) fail\n", data_size);
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
            LOGM_INFO("extradata is AVCC format, mH264AVCCNaluLen %d.\n", mH264AVCCNaluLen);

            if (ContainerType_TS == mContainerType) {

                LOGM_INFO("[TS format], need change H264_FMT_AVCC to H264_FMT_ANNEXB\n");
                mbConvertH264DataFormat = 1;

                DASSERT(!mpExtraDataOri[stream_index]);
                DASSERT(!mExtraDataSizeOri[stream_index]);

                mpExtraDataOri[stream_index] = mpExtraData[stream_index];
                mExtraDataSizeOri[stream_index] = mExtraDataSize[stream_index];
                mpExtraData[stream_index] = NULL;
                mExtraDataSize[stream_index] = 0;

                mH264AVCCNaluLen = 1 + (p_extra[4] & 3);

                p = (TU8 *)av_malloc(mExtraDataSizeOri[stream_index] + 16);
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
            }
        } else {
            mH264DataFmt = H264_FMT_ANNEXB;
            LOGM_INFO("extradata is H264_FMT_ANNEXB format.\n");
        }
    }

    return EECode_OK;
}

EECode CFFMpegMuxer::InitializeFile(const SStreamCodecInfos *infos, TChar *url, ContainerType type, TChar *thumbnailname, TTime start_pts, TTime start_dts)
{
    TInt ret = 0, i = 0;
    EECode err = EECode_OK;
    ContainerType type_from_string = ContainerType_Invalid;

    if (!infos || !url) {
        LOGM_ERROR("NULL input: url %p, infos %p\n", url, infos);
        return EECode_BadParam;
    }

    mContainerType = type;

    type_from_string = __guessMuxContainer(url);
    LOGM_DEBUG("mContainerType %d(%s), type_from_string %d(%s)\n", mContainerType, gfGetContainerStringFromType(mContainerType), type_from_string, gfGetContainerStringFromType(type_from_string));
    if (ContainerType_AUTO == type_from_string) {

    } else if (ContainerType_Invalid == type_from_string) {
        LOGM_FATAL("should not comes here!\n");
        return EECode_InternalLogicalBug;
    } else {
        //DASSERT(mContainerType == type_from_string);
        mContainerType = type_from_string;
    }

#if 0
    if ((H264_FMT_AVCC == mH264DataFmt) && (ContainerType_TS == mContainerType) && !mbConvertH264DataFormat) {

        LOGM_INFO("extradata is AVCC format, mH264AVCCNaluLen %d, TS format, need change to annexb format.\n", mH264AVCCNaluLen);

        //debug assert
        DASSERT(mpExtraData[mVideoIndex]);
        DASSERT(!mpExtraDataOri[mVideoIndex]);
        DASSERT(mExtraDataSize[mVideoIndex]);
        DASSERT(!mExtraDataSizeOri[mVideoIndex]);

        if (mpExtraData[mVideoIndex] && mExtraDataSize[mVideoIndex]) {

            TU8 *p = NULL;
            TU8 *p_extra = mpExtraData[mVideoIndex];
            TU8 startcode[4] = {0, 0, 0, 0x01};
            TUint sps, pps;

            mbConvertH264DataFormat = 1;

            mpExtraDataOri[mVideoIndex] = mpExtraData[mVideoIndex];
            mExtraDataSizeOri[mVideoIndex] = mExtraDataSize[mVideoIndex];
            mpExtraData[mVideoIndex] = NULL;
            mExtraDataSize[mVideoIndex] = 0;

            mH264AVCCNaluLen = 1 + (p_extra[4] & 3);

            p = (TU8 *)av_malloc(mExtraDataSizeOri[mVideoIndex] + 16);
            if (p) {
                mpExtraData[mVideoIndex] = p;
                sps = BE_16(p_extra + 6);
                memcpy(p, startcode, sizeof(startcode));
                p += sizeof(startcode);
                mExtraDataSize[mVideoIndex] += sizeof(startcode);
                memcpy(p, p_extra + 8, sps);
                p += sps;
                mExtraDataSize[mVideoIndex] += sps;

                pps = BE_16(p_extra + 6 + 2 + sps + 1);
                memcpy(p, startcode, sizeof(startcode));
                p += sizeof(startcode);
                mExtraDataSize[mVideoIndex] += sizeof(startcode);
                memcpy(p, p_extra + 6 + 2 + sps + 1 + 2, pps);
                mExtraDataSize[mVideoIndex] += pps;
            } else {
                LOGM_FATAL("NO memory\n");
                return EECode_NoMemory;
            }

            LOGM_INFO("mExtraDataSize[stream_index] %d\n", mExtraDataSize[mVideoIndex]);
        } else {
            LOGM_FATAL("NULL mpExtraData[mVideoIndex]\n");
            return EECode_InternalLogicalBug;
        }
    }
#endif

    mpUrlName = url;
    mpThumbNailFileName = thumbnailname;

    if (mConfigLogLevel > ELogLevel_Notice) {
        gfPrintCodecInfoes((SStreamCodecInfos *)infos);
    }

    LOGM_INFO("InitializeFile, start, mpUrlName %s\n", mpUrlName);
    mpOutFormat = av_guess_format(NULL, mpUrlName, NULL);
    if (!mpOutFormat) {
        LOGM_WARN("Could not deduce output format from file(%s) extension: using extention from container type.\n", mpUrlName);
        i = strlen(url) + strlen(gfGetContainerStringFromType(mContainerType)) + 8;
        mpAllocatedUrlName = (TChar *) DDBG_MALLOC(i, "MXUR");
        if (mpAllocatedUrlName) {
            LOGM_FATAL("[000]:mpAllocatedUrlName %p.\n", mpAllocatedUrlName);
            memset(mpAllocatedUrlName, 0x0, i);
            snprintf(mpAllocatedUrlName, i - 1, "%s.%s", mpUrlName, gfGetContainerStringFromType(mContainerType));
            mpUrlName = mpAllocatedUrlName;
            mpOutFormat = av_guess_format(mpUrlName, NULL, NULL);
            if (!mpOutFormat) {
                LOGM_ERROR("output format not supported, type %d, .%s.\n", mContainerType, gfGetContainerStringFromType(mContainerType));
            }
            return EECode_Error;
        } else {
            LOGM_FATAL("no memory, request size %d\n", i);
            return EECode_NoMemory;
        }
    }

    mpFormat = avformat_alloc_context();
    if (!mpFormat) {
        LOGM_ERROR("avformat_alloc_context fail.\n");
        return EECode_NoMemory;
    }

    mpFormat->oformat = mpOutFormat;
    mpFormat->video_append_extradata = 1;
    snprintf(mpFormat->filename, sizeof(mpFormat->filename), "%s", mpUrlName);

    memset(&mAVFormatParam, 0, sizeof(mAVFormatParam));
    if (av_set_parameters(mpFormat, &mAVFormatParam) < 0) {
        LOGM_ERROR("av_set_parameters error.\n");
        return EECode_Error;
    }

    DASSERT(infos->total_stream_number <= EConstMaxDemuxerMuxerStreamNumber);

    mEstimatedBitrate = 0;
    mnMaxStreamNumber = infos->total_stream_number;

    //add streams
    for (i = 0; i < infos->total_stream_number; i++) {
        //donot set the first PTS to 0
        LOGM_INFO("** before av_new_stream.\n");
        mpStream[i] = av_new_stream(mpFormat, mpFormat->nb_streams);
        err = setParameterToMuxer(infos, i);
        DASSERT_OK(err);
        mpStream[i]->probe_data.filename = mpFormat->filename;
    }
    //setParametersToMuxer(infos);

    if (mConfigLogLevel > ELogLevel_Notice) {
        av_dump_format(mpFormat, 0, mpUrlName, 1);
    }

    LOGM_INFO("** before avio_open, mpOutputFileName %s.\n", mpUrlName);
    /* open the output file, if needed */
    if (!(mpOutFormat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&mpFormat->pb, mpUrlName, AVIO_FLAG_WRITE)) < 0) {
            LOGM_ERROR("Could not open '%s', ret %d, READ-ONLY file system, or BAD filepath/filename?\n", mpUrlName, ret);
            return EECode_Error;
        }
    } else {
        LOGM_WARN("not invoke avio_open()?\n");
    }
    LOGM_INFO("** before av_write_header.\n");

    ret = av_write_header(mpFormat);
    if (ret < 0) {
        LOGM_ERROR("Failed to write header, ret %d\n", ret);
        //some debug print
        LOGM_ERROR("Maybe in-correct container/codec type, for-example, 'AMR' cannot in 'TS', 'PrivateData' should in 'TS'.\n");
        LOGM_ERROR("Or Maybe container type not matched, SetContainerType(MP4) but with file name 'xxx.ts'.\n");
        return EECode_Error;
    }
    LOGM_INFO("InitializeFile, end, mpUrlName %s\n", mpUrlName);

    return EECode_OK;
}

EECode CFFMpegMuxer::FinalizeFile(SMuxerFileInfo *p_file_info)
{
    DASSERT(mpFormat);
    DASSERT(mpOutFormat);
    DASSERT(p_file_info);
    if (!mpFormat || !mpOutFormat || !p_file_info) {
        LOGM_ERROR("BAD input parameters\n");
        return EECode_Error;
    }

    //estimate some info for file
    //getFileInformation();

    //AV_TIME_BASE
    mpFormat->duration = p_file_info->file_duration;
    mpFormat->file_size = (TS64)p_file_info->file_size;
    mpFormat->bit_rate = p_file_info->file_bitrate;

    LOGM_INFO("start write trailer, duration %lld, size %lld, bitrate %d.\n", mpFormat->duration, mpFormat->file_size, mpFormat->bit_rate);
    if (av_write_trailer(mpFormat) < 0) {
        LOGM_ERROR(" av_write_trailer err\n");
        return EECode_Error;
    }
    LOGM_INFO("write trailer done.\n");

    clearFFMpegContext();
    LOGM_INFO("clearFFMpegContext done.\n");

    return EECode_OK;
}

void CFFMpegMuxer::convertH264Data2annexb(TU8 *p_input, TUint size)
{
    DASSERT(p_input);
    DASSERT(size);

    if (p_input && size) {
        if (size > mConversionBufferSize) {
            if (mpConversionBuffer) {
                DDBG_FREE(mpConversionBuffer);
                mpConversionBuffer = NULL;
            }

            mConversionBufferSize = size + 512;
            LOGM_ALWAYS("[memory]: alloc more memory for h264 conversion, request size %d\n", mConversionBufferSize);
            mpConversionBuffer = (TU8 *) DDBG_MALLOC(mConversionBufferSize, "MXCB");
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

void CFFMpegMuxer::clearFFMpegContext()
{
    TUint i;

    DASSERT(mpOutFormat);
    DASSERT(mpFormat);
    if (!mpFormat || !mpOutFormat) {
        return;
    }

    //free each inputpin's context
    for (i = 0; i < mnMaxStreamNumber; i++) {
        DASSERT(mpStream[i]);
        DASSERT(mpStream[i]->codec);
        if (mpStream[i] && mpStream[i]->codec) {
            if (AVMEDIA_TYPE_VIDEO == mpStream[i]->codec->codec_type) {
            } else if (AVMEDIA_TYPE_AUDIO == mpStream[i]->codec->codec_type) {
                avcodec_close(mpStream[i]->codec);
            } else if (AVMEDIA_TYPE_SUBTITLE == mpStream[i]->codec->codec_type) {
            } else if (AVMEDIA_TYPE_DATA == mpStream[i]->codec->codec_type) {
            } else {
                LOGM_FATAL("BAD codec type %d\n", mpStream[i]->codec->codec_type);
            }
            mpStream[i] = NULL;
        }
    }

    if (!(mpOutFormat->flags & AVFMT_NOFILE)) {
        /* close the output file */
        avio_close(mpFormat->pb);
    }

    avformat_free_context(mpFormat);
    mpFormat = NULL;
    mpOutFormat = NULL;
}

EECode CFFMpegMuxer::setParameterToMuxer(const SStreamCodecInfos *infos, TUint i)
{
    EECode err = EECode_OK;

    DASSERT(infos);
    DASSERT(infos->total_stream_number > i);
    if (infos->total_stream_number <= i) {
        LOGM_FATAL("BAD index %d, infos->total_stream_number %d\n", i, infos->total_stream_number);
        return EECode_BadParam;
    }

    if (StreamType_Video == infos->info[i].stream_type) {
        err = setVideoParametersToMuxer((SStreamCodecInfo *)(&infos->info[i]), i);
        mEstimatedBitrate += infos->info[i].spec.video.bitrate;
    } else if (StreamType_Audio == infos->info[i].stream_type) {
        err = setAudioParametersToMuxer((SStreamCodecInfo *)(&infos->info[i]), i);
        mEstimatedBitrate += infos->info[i].spec.audio.bitrate;
    } else if (StreamType_PrivateData == infos->info[i].stream_type) {
        err = setPrivateDataParametersToMuxer((SStreamCodecInfo *)(&infos->info[i]), i);
    } else if (StreamType_Subtitle == infos->info[i].stream_type) {
        err = setSubtitleParametersToMuxer((SStreamCodecInfo *)(&infos->info[i]), i);
    } else {
        LOGM_FATAL("BAD stream_type %d, index %d\n", infos->info[i].stream_type, i);
        return EECode_BadParam;
    }

    return err;
}

EECode CFFMpegMuxer::setParametersToMuxer(const SStreamCodecInfos *infos)
{
    TUint i = 0;
    EECode err = EECode_OK;

    mEstimatedBitrate = 0;
    mnMaxStreamNumber = infos->total_stream_number;

    for (i = 0; i < infos->total_stream_number; i ++) {
        if (StreamType_Video == infos->info[i].stream_type) {
            err = setVideoParametersToMuxer((SStreamCodecInfo *)(&infos->info[i]), i);
            mEstimatedBitrate += infos->info[i].spec.video.bitrate;

        } else if (StreamType_Audio == infos->info[i].stream_type) {
            err = setAudioParametersToMuxer((SStreamCodecInfo *)(&infos->info[i]), i);
            mEstimatedBitrate += infos->info[i].spec.audio.bitrate;
        } else if (StreamType_PrivateData == infos->info[i].stream_type) {
            err = setPrivateDataParametersToMuxer((SStreamCodecInfo *)(&infos->info[i]), i);
        } else if (StreamType_Subtitle == infos->info[i].stream_type) {
            err = setSubtitleParametersToMuxer((SStreamCodecInfo *)(&infos->info[i]), i);
        } else {
            LOGM_FATAL("BAD stream_type %d, index %d\n", infos->info[i].stream_type, i);
            return EECode_BadParam;
        }
    }

    return err;
}

EECode CFFMpegMuxer::setVideoParametersToMuxer(SStreamCodecInfo *video_param, TUint index)
{
    AVStream *pstream = NULL;
    AVCodecContext *video_enc = NULL;
    //AVCodec *codec;

    DASSERT(mpFormat);
    if (!video_param || !mpFormat) {
        LOGM_ERROR("CFFMpegMuxer::setVideoParametersToMuxer NULL pointer video_param %p, mpFormat %p.\n", video_param, mpFormat);
        return EECode_BadParam;
    }

    DASSERT(StreamType_Video == video_param->stream_type);
    if (StreamType_Video != video_param->stream_type) {
        LOGM_ERROR("CFFMpegMuxer::setVideoParametersToMuxer: pInput's type(%d) is not video.\n", video_param->stream_type);
        return EECode_BadParam;
    }

    DASSERT(EConstMaxDemuxerMuxerStreamNumber > index);
    if (EConstMaxDemuxerMuxerStreamNumber <= index) {
        LOGM_ERROR("CFFMpegMuxer::setVideoParametersToMuxer index (%u) out of range (%u).\n", index, EConstMaxDemuxerMuxerStreamNumber);
        return EECode_BadParam;
    }

    pstream = mpStream[index];
    DASSERT(pstream);
    if (!pstream || !pstream->codec) {
        LOGM_ERROR("CFFMpegMuxer::setVideoParametersToMuxer NULL pointer pstream %p, pstream->codec %p.\n", pstream, pstream->codec);
        return EECode_BadParam;
    }

    pstream->probe_data.filename = mpFormat->filename;

    DASSERT(pstream->codec);
    avcodec_get_context_defaults2(pstream->codec, AVMEDIA_TYPE_VIDEO);
    video_enc = pstream->codec;
    if (mpFormat->oformat->flags & AVFMT_GLOBALHEADER)
    { video_enc->flags |= CODEC_FLAG_GLOBAL_HEADER; }

    switch (video_param->stream_format) {
        case StreamFormat_H264:
            LOGM_INFO("video format is AVC(h264).\n");
            video_enc->codec_id = CODEC_ID_H264;
            //mpOutFormat->video_codec = CODEC_ID_H264;
            //video_enc->codec_tag = MK_FOURCC_TAG('H', '2', '6', '4');
            break;
        default:
            DASSERT(0);
            LOGM_ERROR("only support h264 now, unsupported video_param->stream_format %d.\n", video_param->stream_format);
            return EECode_NotSupported;
    }

    /**
     * "codec->time_base"
     * This is the fundamental unit of time (in seconds) in terms
     * of which frame timestamps are represented. For fixed-fps content,
     * timebase should be 1/framerate and timestamp increments should be
     * identically 1.
     */
    video_enc->time_base.num = DDefaultVideoFramerateDen; //video_param->spec.video.framerate_den;
    video_enc->time_base.den = DDefaultTimeScale; //video_param->spec.video.framerate_num;

    pstream->time_base.num = 1;
    pstream->time_base.den = DDefaultTimeScale;

    LOGM_INFO("       video_enc->time_base.num %d, video_enc->time_base.den %d.\n", video_enc->time_base.num, video_enc->time_base.den);

    video_enc->width = video_param->spec.video.pic_width;
    video_enc->height = video_param->spec.video.pic_height;

    //video_enc->sample_aspect_ratio = pstream->sample_aspect_ratio = (AVRational) {video_param->spec.video.sample_aspect_ratio_num, video_param->spec.video.sample_aspect_ratio_den};
    video_enc->sample_aspect_ratio = pstream->sample_aspect_ratio = (AVRational) {1, 1};
    //video_enc->has_b_frames = 2;
    video_enc->bit_rate = 400000;// 4M
    video_enc->gop_size = 30;
    video_enc->pix_fmt = PIX_FMT_YUV420P;
    video_enc->codec_type = AVMEDIA_TYPE_VIDEO;

    if (NULL == mpExtraData[index]) {
        LOGM_FATAL("alloc extra data size!\n");
        mpExtraData[index] = (TU8 *)av_mallocz(200);
    }
    DASSERT(mpExtraData[index]);

    //copy extra data
    video_enc->extradata_size = mExtraDataSize[index];
    video_enc->extradata = (TU8 *)av_mallocz(mExtraDataSize[index]);
    memcpy(video_enc->extradata, mpExtraData[index], mExtraDataSize[index]);

    LOGM_INFO("video_enc->extradata: [%02x %02x %02x %02x, %02x %02x %02x %02x]\n", mpExtraData[index][0], mpExtraData[index][1], mpExtraData[index][2], mpExtraData[index][3], mpExtraData[index][4], mpExtraData[index][5], mpExtraData[index][6], mpExtraData[index][7]);

    mbNeedFindSPSPPSInBitstream = 0;

    LOGM_INFO("       width %d, height %d, sample_aspect_ratio %d %d, extradata_size %d, pointer %p.\n", video_enc->width, video_enc->height, video_enc->sample_aspect_ratio.num, video_enc->sample_aspect_ratio.den, video_enc->extradata_size, video_enc->extradata);
    return EECode_OK;
}

EECode CFFMpegMuxer::setAudioParametersToMuxer(SStreamCodecInfo *audio_param, TUint index)
{
    AVStream *pstream = NULL;
    AVCodecContext *audio_enc = NULL;
    AVCodec *codec;
    SSimpleAudioSpecificConfig *p_simple_header = NULL;
    TU8 samplerate = 0;

    DASSERT(mpFormat);
    if (!audio_param || !mpFormat) {
        LOGM_ERROR("CFFMpegMuxer::setAudioParametersToMuxer NULL pointer audio_param %p, mpFormat %p.\n", audio_param, mpFormat);
        return EECode_BadParam;
    }

    DASSERT(StreamType_Audio == audio_param->stream_type);
    if (StreamType_Audio != audio_param->stream_type) {
        LOGM_ERROR("CFFMpegMuxer::setAudioParametersToMuxer: type(%d) is not audio.\n", audio_param->stream_type);
        return EECode_BadParam;
    }

    DASSERT(EConstMaxDemuxerMuxerStreamNumber > index);
    if (EConstMaxDemuxerMuxerStreamNumber <= index) {
        LOGM_ERROR("CFFMpegMuxer::setAudioParametersToMuxer index (%u) out of range (%u).\n", index, EConstMaxDemuxerMuxerStreamNumber);
        return EECode_BadParam;
    }

    pstream = mpStream[index];
    DASSERT(pstream);
    if (!pstream || !pstream->codec) {
        LOGM_ERROR("CFFMpegMuxer::setAudioParametersToMuxer NULL pointer pstream %p, pstream->codec %p.\n", pstream, pstream->codec);
        return EECode_BadParam;
    }

    pstream->probe_data.filename = mpFormat->filename;

    DASSERT(pstream->codec);
    avcodec_get_context_defaults2(pstream->codec, AVMEDIA_TYPE_AUDIO);
    audio_enc = pstream->codec;
    if (mpFormat->oformat->flags & AVFMT_GLOBALHEADER)
    { audio_enc->flags |= CODEC_FLAG_GLOBAL_HEADER; }

    switch (audio_param->stream_format) {

        case StreamFormat_MP2:
            LOGM_INFO("audio format is MP2.\n");
            audio_enc->codec_id = CODEC_ID_MP2;
            //mpOutFormat->video_codec = CODEC_ID_MP2;
            //audio_enc->frame_size = 1024;
            break;

        case StreamFormat_AC3:
            LOGM_INFO("audio format is AC3.\n");
            audio_enc->codec_id = CODEC_ID_AC3;
            break;

        case StreamFormat_ADPCM:
            LOGM_INFO("audio format is ADPCM.\n");
            audio_enc->codec_id = CODEC_ID_ADPCM_EA;
            break;

        case StreamFormat_AMR_NB:
            LOGM_INFO("audio format is AMR_NB.\n");
            audio_enc->codec_id = CODEC_ID_AMR_NB;
            break;

        case StreamFormat_AMR_WB:
            LOGM_INFO("audio format is AMR_WB.\n");
            audio_enc->codec_id = CODEC_ID_AMR_WB;
            break;

        default:
            DASSERT(0);
            LOGM_ERROR("only support aac now, unsupported SStreamCodecInfo->stream_format %d.\n", audio_param->stream_format);
        case StreamFormat_AAC:
            LOGM_INFO("audio format is AAC.\n");
            audio_enc->codec_id = CODEC_ID_AAC;
            //mpOutFormat->video_codec = CODEC_ID_AAC;
            //audio_enc->codec_tag = 0x00ff;

            if (!mpExtraData[index] && !mExtraDataSize[index]) {
                //extra data
                mpExtraData[index] = (TU8 *)av_mallocz(2);
                mExtraDataSize[index] = 2;

                LOGM_INFO("generate simple aac exta data\n");

                p_simple_header = (SSimpleAudioSpecificConfig *)mpExtraData[index];
                samplerate = 0;

                p_simple_header->audioObjectType = eAudioObjectType_AAC_LC;//hard code here
                switch (audio_param->spec.audio.sample_rate) {
                    case 44100:
                        samplerate = eSamplingFrequencyIndex_44100;
                        break;
                    case 48000:
                        samplerate = eSamplingFrequencyIndex_48000;
                        break;
                    case 24000:
                        samplerate = eSamplingFrequencyIndex_24000;
                        break;
                    case 16000:
                        samplerate = eSamplingFrequencyIndex_16000;
                        break;
                    case 8000:
                        samplerate = eSamplingFrequencyIndex_8000;
                        break;
                    case 12000:
                        samplerate = eSamplingFrequencyIndex_12000;
                        break;
                    case 32000:
                        samplerate = eSamplingFrequencyIndex_32000;
                        break;
                    case 22050:
                        samplerate = eSamplingFrequencyIndex_22050;
                        break;
                    case 11025:
                        samplerate = eSamplingFrequencyIndex_11025;
                        break;
                    default:
                        LOGM_FATAL("ADD support here, audio sample rate %d.\n", audio_param->spec.audio.sample_rate);
                        break;
                }
                p_simple_header->samplingFrequencyIndex_high = samplerate >> 1;
                p_simple_header->samplingFrequencyIndex_low = samplerate & 0x1;
                p_simple_header->channelConfiguration = audio_param->spec.audio.channel_number;
                p_simple_header->bitLeft = 0;

                LOGM_INFO("audio_param->spec.audio.sample_rate %d, channel_number %d.\n", audio_param->spec.audio.sample_rate, audio_param->spec.audio.channel_number);
                LOGM_INFO("samplingFrequencyIndex_high %x, samplingFrequencyIndex_low %x.\n", (TU8)p_simple_header->samplingFrequencyIndex_high, (TU8)p_simple_header->samplingFrequencyIndex_low);
                LOGM_INFO("audioObjectType %x.\n", (TU8)p_simple_header->audioObjectType);
                LOGM_INFO("channelConfiguration %x.\n", (TU8)p_simple_header->channelConfiguration);
                LOGM_INFO("bitLeft %x.\n", (TU8)p_simple_header->bitLeft);
                LOGM_INFO("Audio AAC config data %x, %x.\n", mpExtraData[index][0], mpExtraData[index][1]);
                //audio_enc->frame_size = 1024;
            } else {
                DASSERT(mpExtraData[index]);
                DASSERT(mExtraDataSize[index]);
            }
            break;
    }

    audio_enc->extradata_size = mExtraDataSize[index];
    audio_enc->extradata = mpExtraData[index];
    mpExtraData[index] = NULL;
    mExtraDataSize[index] = 0;
    //audio_enc->bit_rate = pInput->mParams.bitrate;
    audio_enc->sample_rate = audio_param->spec.audio.sample_rate;
    audio_enc->channels = audio_param->spec.audio.channel_number;

    if (1 == audio_enc->channels) {
        audio_enc->channel_layout = CH_LAYOUT_MONO;
    } else if (2 == audio_enc->channels) {
        audio_enc->channel_layout = CH_LAYOUT_STEREO;
    } else {
        LOGM_FATAL("channel >2 is not supported yet.\n");
        audio_enc->channels = 2;
        audio_enc->channel_layout = CH_LAYOUT_STEREO;
    }

    audio_enc->sample_fmt = (AVSampleFormat)audio_param->spec.audio.sample_format;
    audio_enc->bit_rate = audio_param->spec.audio.bitrate;
    audio_enc->codec_type = AVMEDIA_TYPE_AUDIO;

    DASSERT(AV_SAMPLE_FMT_S16 == audio_enc->sample_fmt);

    if (!(codec = avcodec_find_encoder(audio_enc->codec_id))) {
        LOGM_ERROR("Failed to find audio codec\n");
        return EECode_Error;
    }
    if (avcodec_open(audio_enc, codec) < 0) {
        LOGM_ERROR("Failed to open audio codec\n");
        return EECode_Error;
    }

    LOGM_INFO("audio enc info: bit_rate %d, sample_rate %d, channels %d, channel_layout %lld, frame_size %d, flag 0x%x.\n", audio_enc->bit_rate, audio_enc->sample_rate, audio_enc->channels, audio_enc->channel_layout, audio_enc->frame_size, audio_enc->flags);
    LOGM_INFO("extradata_size %d, codec_type %d, pix_fmt %d, time base num %d, den %d.\n", audio_enc->extradata_size, audio_enc->codec_type, audio_enc->pix_fmt, audio_enc->time_base.num, audio_enc->time_base.den);
    return EECode_OK;
}

EECode CFFMpegMuxer::setSubtitleParametersToMuxer(SStreamCodecInfo *subtitle_param, TUint index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NotSupported;
}

EECode CFFMpegMuxer::setPrivateDataParametersToMuxer(SStreamCodecInfo *pridata_param, TUint index)
{
    AVStream *pstream = NULL;

    DASSERT(mpFormat);
    if (!pridata_param || !mpFormat) {
        LOGM_ERROR("CFFMpegMuxer::setAudioParametersToMuxer NULL pointer pridata_param %p, mpFormat %p.\n", pridata_param, mpFormat);
        return EECode_BadParam;
    }

    DASSERT(StreamType_PrivateData == pridata_param->stream_type);
    if (StreamType_PrivateData != pridata_param->stream_type) {
        LOGM_ERROR("CFFMpegMuxer::setPrivayeDataParametersToMuxer: pridata_param(%d) is not private data.\n", pridata_param->stream_type);
        return EECode_BadParam;
    }

    DASSERT(EConstMaxDemuxerMuxerStreamNumber > index);
    if (EConstMaxDemuxerMuxerStreamNumber <= index) {
        LOGM_ERROR("CFFMpegMuxer::setPrivateDataParametersToMuxer index (%u) out of range (%u).\n", index, EConstMaxDemuxerMuxerStreamNumber);
        return EECode_BadParam;
    }

    pstream = mpStream[index];
    DASSERT(pstream);
    if (!pstream || !pstream->codec) {
        LOGM_ERROR("CFFMpegMuxer::setPrivateDataParametersToMuxer NULL pointer pstream %p, pstream->codec %p.\n", pstream, pstream->codec);
        return EECode_BadParam;
    }

    pstream->probe_data.filename = mpFormat->filename;
    LOGM_INFO("[pridata mux]: framerate num %d, den %d.\n", pstream->r_frame_rate.num, pstream->r_frame_rate.den);
    LOGM_INFO("[pridata mux]: time_base num %d, den %d, pInput->mDuration.\n", pstream->time_base.num, pstream->time_base.den);

    pstream->time_base.num = 1;
    pstream->time_base.den = DDefaultTimeScale;

    return EECode_OK;
}

EECode CFFMpegMuxer::WriteVideoBuffer(CIBuffer *pBuffer, SMuxerDataTrunkInfo *info)
{
    AVPacket packet;
    TInt ret = 0;
    TU8 index = 0;

    DASSERT(pBuffer);
    DASSERT(info);
    DASSERT(pBuffer->GetDataPtr());
    index = info->stream_index;
    DASSERT(index == mVideoIndex);

    LOGM_DEBUG("WriteVideoBuffer(index %d): mbConvertH264DataFormat %d, size %ld info->pts %lld, info->dts %lld, info->native_pts %lld, info->native_dts %lld, duration %lld, normalized duration %lld\n", index, mbConvertH264DataFormat, pBuffer->GetDataSize(), info->pts, info->dts, info->native_pts, info->native_dts, info->duration, info->av_normalized_duration);
    mDebugHeartBeat ++;

    av_init_packet(&packet);
    packet.stream_index = index;
    if (!mbConvertH264DataFormat) {
        packet.size = pBuffer->GetDataSize();
        packet.data = pBuffer->GetDataPtr();
    } else {
        DASSERT(H264_FMT_AVCC == mH264DataFmt);
        convertH264Data2annexb(pBuffer->GetDataPtr(), pBuffer->GetDataSize());
        packet.size = mConversionDataSize;
        packet.data = mpConversionBuffer;
    }

    //ensure first frame are key frame
    if (!mbVideoKeyFrameComes) {
        DASSERT(pBuffer->mVideoFrameType == EPredefinedPictureType_IDR);
        if (pBuffer->mVideoFrameType == EPredefinedPictureType_IDR) {
            if (mbNeedFindSPSPPSInBitstream) {
                TU8 *pseq = packet.data;
                //extradata sps
                LOGM_INFO("Beginning to calculate sps-pps's length.\n");

                //get seq data for safe, not exceed boundary when data is corrupted by DSP
                TU8 *pIDR = NULL;
                TU8 *pstart = NULL;
                bool find_seq = gfGetSpsPps(pBuffer->GetDataPtr(), pBuffer->GetDataSize(), pseq, mExtraDataSize[index], pIDR, pstart);

                if (true == find_seq) {
                    if (!pstart) {
                        DASSERT(pBuffer->GetDataPtr() == pseq);
                        packet.data = pseq;
                        packet.size  -= (TU32)pseq - (TU32)pBuffer->GetDataPtr();
                        pBuffer->SetDataSize(packet.size);
                        pBuffer->SetDataPtr(packet.data);
                    } else {
                        packet.data = pstart;
                        packet.size  -= (TU32)pstart - (TU32)pBuffer->GetDataPtr();
                        pBuffer->SetDataSize(packet.size);
                        pBuffer->SetDataPtr(packet.data);
                    }
                    LOGM_INFO("[debug info], in %p, size %ld, pseq %p, pIDR %p, pstart %p\n", pBuffer->GetDataPtr(), pBuffer->GetDataSize(), pseq, pIDR, pstart);
                    LOGM_INFO("pseq: [%02x %02x %02x %02x, %02x %02x %02x %02x]\n", pseq[0], pseq[1], pseq[2], pseq[3], pseq[4], pseq[5], pseq[6], pseq[7]);
                    LOGM_INFO("pIDR: [%02x %02x %02x %02x, %02x %02x %02x %02x]\n", pIDR[0], pIDR[1], pIDR[2], pIDR[3], pIDR[4], pIDR[5], pIDR[6], pIDR[7]);
                    if (pstart) {
                        LOGM_INFO("pstart: [%02x %02x %02x %02x, %02x %02x %02x %02x]\n", pstart[0], pstart[1], pstart[2], pstart[3], pstart[4], pstart[5], pstart[6], pstart[7]);
                    }
                } else {
                    //LOGM_FATAL("IDR data buffer corrupted!!, how to handle it?\n");
                    //msState = STATE_ERROR;
                    //return;
                }

                LOGM_INFO("Calculate sps-pps's length completely, len=%d.\n", mExtraDataSize[index]);
                DASSERT(mExtraDataSize[index] <= SPS_PPS_LEN);

                if (!mpExtraData[index]) {
                    mpExtraData[index] = (TU8 *)av_mallocz(mExtraDataSize[index] + 8);
                }

                DASSERT(mpExtraData[index]);
                if (mpExtraData[index]) {
                    memcpy(mpExtraData[index], pseq, mExtraDataSize[index]);
                } else {
                    LOGM_FATAL("NULL mpExtraData[mVideoIndex(%d)], %p\n", index, mpExtraData[index]);
                }
            }
            LOGM_INFO("Key frame comes, info->duration %lld, pBuffer->GetBufferPTS() %lld.\n", info->duration, pBuffer->GetBufferPTS());
            mbVideoKeyFrameComes = 1;
        } else {
            LOGM_INFO("non-IDR frame (%d), data comes here, seq num [%d, %d], when stream not started, discard them.\n", pBuffer->mVideoFrameType, pBuffer->mBufferSeqNumber0, pBuffer->mBufferSeqNumber1);
            return EECode_OK;
        }
    }

    if (pBuffer->mVideoFrameType == EPredefinedPictureType_IDR || pBuffer->mVideoFrameType == EPredefinedPictureType_I) {
        packet.flags |= AV_PKT_FLAG_KEY;
        packet.convergence_duration = DDefaultTimeScale * 15;
    }

    packet.dts = info->dts;
    //packet.dts = AV_NOPTS_VALUE;
    packet.pts = info->pts;
    packet.duration = info->duration;

    //mLastTimestamp[index] = packet.pts;
    //mWriteSize[index] += (TU64)packet.size;
    LOGM_DEBUG("packet.data, size %d: [%02x %02x %02x %02x, %02x %02x %02x %02x]\n", packet.size, packet.data[0], packet.data[1], packet.data[2], packet.data[3], packet.data[4], packet.data[5], packet.data[6], packet.data[7]);

    //LOGM_INFO("packet->data %p, packet.size %d.\n", packet.data, packet.size);
    if (mpPersistMediaConfig->muxer_skip_video == 0) {
        LOGM_DEBUG("before av_write_frame(video)\n");
        if ((ret = av_write_frame(mpFormat, &packet)) != 0) {
            //tmp code here, need modify av_write_frame's ret value, get correct status of storage full?
            if (mpFormat->pb->error) {
                LOGM_ERROR("IO error in av_write_frame, ret %d\n", ret);
                return EECode_IOError;
            } else {
                LOGM_ERROR("fail in av_write_frame, ret %d\n", ret);
                return EECode_IOError;
            }
        }
    }

    LOGM_DEBUG("WriteVideoBuffer(index %d) done, size:%d, pts:%lld, dts %lld, duration %d, time base num %d, den %d, frame rate num %d, den %d.\n", index, packet.size, packet.pts, packet.dts, packet.duration, mpFormat->streams[packet.stream_index]->time_base.num, mpFormat->streams[packet.stream_index]->time_base.den, mpFormat->streams[packet.stream_index]->r_frame_rate.num, mpFormat->streams[packet.stream_index]->r_frame_rate.den);
    return EECode_OK;
}

EECode CFFMpegMuxer::WriteAudioBuffer(CIBuffer *pBuffer, SMuxerDataTrunkInfo *info)
{
    AVPacket packet;
    TInt ret = 0;
    TUint header_length = 0;
    TU8 index = 0;

    DASSERT(pBuffer);
    DASSERT(info);
    index = info->stream_index;
    DASSERT(index == mAudioIndex);

    LOGM_DEBUG("WriteAudioBuffer(index %d): size %ld info->pts %lld, info->dts %lld, info->native_pts %lld, info->native_dts %lld, duration %lld, normalized duration %lld\n", index, pBuffer->GetDataSize(), info->pts, info->dts, info->native_pts, info->native_dts, info->duration, info->av_normalized_duration);
    av_init_packet(&packet);
    mDebugHeartBeat ++;

    packet.stream_index = mAudioIndex;
    packet.size = pBuffer->GetDataSize() - header_length;
    packet.data = pBuffer->GetDataPtr() + header_length;
    packet.dts = info->dts;
    packet.pts = info->pts;
    packet.duration = info->av_normalized_duration;
    packet.flags |= AV_PKT_FLAG_KEY;
    //LOGM_PTS("audio pts %lld.\n", packet.pts);

    //LOGM_PTS("[PTS]:audio write to file pts %lld, dts %lld, pts gap %lld, duration %d, pInput->mPTSDTSGap %d.\n", packet.pts, packet.dts, packet.pts - pInput->mLastPTS, pInput->mDuration, pInput->mPTSDTSGap);
    //mLastTimestamp[index] = packet.pts;
    //mWriteSize[index] += packet.size;

    if (mpPersistMediaConfig->muxer_skip_audio == 0) {
        LOGM_DEBUG("before av_write_frame(audio)\n");
        if ((ret = av_write_frame(mpFormat, &packet)) != 0) {
            //tmp code here, need modify av_write_frame's ret value, get correct status of storage full?
            if (mpFormat->pb->error) {
                LOGM_ERROR("IO error in av_write_frame, ret %d\n", ret);
                return EECode_IOError;
            } else {
                LOGM_ERROR("fail in av_write_frame, ret %d\n", ret);
                return EECode_IOError;
            }
        }
    }

    LOGM_DEBUG("WriteAudioBuffer(index %d) done, size:%d, pts:%lld, time base num %d, den %d.\n", index, packet.size, packet.pts, mpFormat->streams[packet.stream_index]->time_base.num, mpFormat->streams[packet.stream_index]->time_base.den);
    return EECode_OK;
}

EECode CFFMpegMuxer::WritePridataBuffer(CIBuffer *pBuffer, SMuxerDataTrunkInfo *info)
{
    AVPacket packet;
    TInt ret = 0;
    TU8 index = 0;

    DASSERT(pBuffer);
    DASSERT(info);
    index = info->stream_index;
    DASSERT(index == mPrivDataIndex);

    LOGM_DEBUG("WritePridataBuffer(index %d): size %ld info->pts %lld, info->dts %lld, info->native_pts %lld, info->native_dts %lld, duration %lld, normalized duration %lld\n", index, pBuffer->GetDataSize(), info->pts, info->dts, info->native_pts, info->native_dts, info->duration, info->av_normalized_duration);

    av_init_packet(&packet);

    packet.stream_index = mPrivDataIndex;
    packet.size = pBuffer->GetDataSize();
    packet.data = pBuffer->GetDataPtr();
    packet.pts = info->pts;
    packet.dts = info->dts;
    packet.flags |= AV_PKT_FLAG_KEY;
    packet.duration = info->duration;

    //mLastTimestamp[index] = packet.pts;
    //mWriteSize[index] += packet.size;

    if (mpPersistMediaConfig->muxer_skip_pridata == 0) {
        if ((ret = av_write_frame(mpFormat, &packet)) != 0) {
            //tmp code here, need modify av_write_frame's ret value, get correct status of storage full?
            if (mpFormat->pb->error) {
                LOGM_ERROR("IO error in av_write_frame, ret %d\n", ret);
                return EECode_IOError;
            } else {
                LOGM_ERROR("fail in av_write_frame, ret %d\n", ret);
                return EECode_IOError;
            }
        }
    }

    LOGM_DEBUG("WritePridataBuffer(index %d) done, size:%d, pts:%lld, time base num %d, den %d.\n", index, packet.size, packet.pts, mpFormat->streams[packet.stream_index]->time_base.num, mpFormat->streams[packet.stream_index]->time_base.den);
    return EECode_OK;
}

EECode CFFMpegMuxer::SetState(TU8 b_suspend)
{
    mbSuspend = b_suspend;
    return EECode_OK;
}

void CFFMpegMuxer::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    mDebugHeartBeat ++;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

#endif

