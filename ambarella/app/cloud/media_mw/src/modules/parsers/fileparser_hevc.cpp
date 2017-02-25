/*******************************************************************************
 * fileparser_hevc.cpp
 *
 * History:
 *  2014/12/12 - [Zhi He] create file
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

#include "media_mw_if.h"
#include "codec_interface.h"

#include "fileparser_hevc.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IMediaFileParser *gfCreateBasicHEVCParser()
{
    return CHEVCFileParser::Create();
}

static TU8 *__next_start_code_hevc_1(TU8 *p, TIOSize len, TU32 &prefix_len, TU8 &nal_type, TU8 &first_byte)
{
    TUint state = 0;
    first_byte = 0;

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
                    first_byte = p[3];
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
                    first_byte = p[3];
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

CHEVCFileParser *CHEVCFileParser::Create()
{
    CHEVCFileParser *result = new CHEVCFileParser();
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

void CHEVCFileParser::Delete()
{
    if (mpIO) {
        mpIO->Delete();
        mpIO = NULL;
    }

    if (mpReadBuffer) {
        DDBG_FREE(mpReadBuffer, "FPHV");
        mpReadBuffer = NULL;
    }
}

CHEVCFileParser::CHEVCFileParser()
    : mpIO(NULL)
    , mpReadBuffer(NULL)
    , mReadBufferSize(0)
    , mReadBlockSize(0)
    , mReadNextThreashold(0)
    , mpCurrentPtr(NULL)
    , mBufRemaingSize(0)
    , mFileTotalSize(0)
    , mFileCurrentOffset(0)
    , mbFileOpend(0)
    , mbFileEOF(0)
    , mbParseMediaInfo(0)
    , mbStartReadFile(0)
    , mbRemoveBuggyFileBytes(0)
    , mRemoveBuggyFileByteNumber(0)
    , mProfileIndicator(0)
    , mLevelIndicator(0)
    , mVideoWidth(1920)
    , mVideoHeight(1080)
    , mVideoFramerateNum(DDefaultVideoFramerateNum)
    , mVideoFramerateDen(DDefaultVideoFramerateDen)
{
}

CHEVCFileParser::~CHEVCFileParser()
{

}

EECode CHEVCFileParser::Construct()
{
    mReadBlockSize = 16 * 1024 * 1024;
    mReadNextThreashold = 6 * 1024 * 1024;

    mReadBufferSize = mReadBlockSize + mReadNextThreashold;
    mpReadBuffer = (TU8 *) DDBG_MALLOC(mReadBufferSize, "FPHV");
    if (DUnlikely(!mpReadBuffer)) {
        LOG_FATAL("CHEVCFileParser::Construct(), no memory, request size %lld\n", mReadBufferSize);
        mReadBufferSize = 0;
        return EECode_NoMemory;
    }

    mpIO = gfCreateIO(EIOType_File);

    if (DUnlikely(!mpIO)) {
        LOG_FATAL("CHEVCFileParser::Construct(), gfCreateIO() fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

EECode CHEVCFileParser::OpenFile(TChar *name)
{
    if (DUnlikely(mbFileOpend)) {
        LOG_WARN("close previous file first\n");
        mpIO->Close();
        mbFileOpend = 0;
    }

    EECode err = mpIO->Open(name, EIOFlagBit_Read | EIOFlagBit_Binary);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("CHEVCFileParser::OpenFile(%s) fail, ret %d, %s\n", name, err, gfGetErrorCodeString(err));
        return err;
    }

    mpIO->Query(mFileTotalSize, mFileCurrentOffset);

    mbFileOpend = 1;

    return EECode_OK;
}

EECode CHEVCFileParser::CloseFile()
{
    if (DUnlikely(!mbFileOpend)) {
        LOG_WARN("file not opened, or already closed\n");
        return EECode_OK;
    }

    mpIO->Close();
    mbFileOpend = 0;

    return EECode_OK;
}

EECode CHEVCFileParser::SeekTo(TIOSize offset)
{
    if (DUnlikely(!mbFileOpend)) {
        LOG_ERROR("file not opened, or already closed\n");
        return EECode_BadState;
    }

    if (DUnlikely(offset >= mFileTotalSize)) {
        LOG_ERROR("CHEVCFileParser::SeekTo, offset >= total size\n");
        return EECode_BadParam;
    }

    mpIO->Seek(offset, EIOPosition_Begin);
    mFileCurrentOffset = offset;
    return EECode_OK;
}

EECode CHEVCFileParser::ReadPacket(SMediaPacket *packet)
{
    if ((!mbFileEOF) && (mBufRemaingSize < mReadNextThreashold)) {
        readDataFromFile();
    }

    TU8 *p_src = mpCurrentPtr;
    TU8 *p_src_cur = p_src;
    TU8 *p_end = NULL;
    TIOSize processing_size = 0;
    TU32 prefix_len;
    TU8 nal_type = ENalType_unspecified;
    TU8 first_byte = 0;
    TU32 next_is_first_slice = 0;
    TU32 read_frame = 0;
    //TU32 with_vps = 0;
    TU32 remaining_size = 0;

    packet->sub_packet_number = 0;

    p_src_cur = __next_start_code_hevc_1(p_src, mBufRemaingSize, prefix_len, nal_type, first_byte);
    if (DUnlikely(!p_src_cur)) {
        LOG_ERROR("ReadPacket() fail, do not find first start code prefix?\n");
        return EECode_BadParam;
    }

    if (DUnlikely(EHEVCNalType_VCL_END > nal_type)) {
        read_frame = 1;
        packet->paket_type = nal_type;
        if ((EHEVCNalType_IDR_W_RADL == nal_type) || (EHEVCNalType_IDR_N_LP == nal_type)) {
            packet->frame_type = EPredefinedPictureType_IDR;
        } else {
            packet->frame_type = EPredefinedPictureType_P;
        }
    } else if (DUnlikely(EHEVCNalType_VPS == nal_type)) {
        //with_vps = 1;
    }

    processing_size = (TIOSize)(p_src_cur - p_src);
    if (DUnlikely(processing_size)) {
        if (4 == processing_size) {
            LOG_ERROR("skip data size %lld:, file maybe buggy, will remove bytes at tail\n", processing_size);
        } else {
            LOG_WARN("skip data size %lld\n", processing_size);
        }
        gfPrintMemory(p_src, (TU32) processing_size);

        mBufRemaingSize -= processing_size;
        mpCurrentPtr += processing_size;
        p_src = p_src_cur;
        mbRemoveBuggyFileBytes = 1;
        mRemoveBuggyFileByteNumber = processing_size;
    }

    p_src_cur += prefix_len;
    remaining_size = mBufRemaingSize - prefix_len;

    do {

        packet->offset_hint[packet->sub_packet_number] = (TIOSize)(p_src_cur - prefix_len - p_src);
        packet->sub_packet_number ++;

        p_end = __next_start_code_hevc_1(p_src_cur, remaining_size, prefix_len, nal_type, first_byte);

        if (DUnlikely(!p_end)) {
            if (mbFileEOF) {
                LOG_NOTICE("last packet\n");
                packet->p_data = p_src;
                packet->data_len = mBufRemaingSize;
                packet->have_pts = 0;
                packet->paket_type = p_src[prefix_len] & 0x1f;
                mpCurrentPtr += mBufRemaingSize;
                mBufRemaingSize = 0;
                return EECode_OK_EOF;
            } else {
                LOG_ERROR("why here?\n");
                return EECode_DataCorruption;
            }
        } else {
            next_is_first_slice = first_byte & 0x80;

            if (read_frame && next_is_first_slice) {
                break;
            }

            if (DUnlikely(EHEVCNalType_VCL_END > nal_type)) {
                read_frame = 1;
                packet->paket_type = nal_type;
                if ((EHEVCNalType_IDR_W_RADL == nal_type) || (EHEVCNalType_IDR_N_LP == nal_type)) {
                    packet->frame_type = EPredefinedPictureType_IDR;
                } else {
                    packet->frame_type = EPredefinedPictureType_P;
                }
            } else if (DUnlikely(EHEVCNalType_VPS == nal_type)) {
                //with_vps = 1;
            }
            remaining_size -= (p_end + prefix_len - p_src_cur);
            p_src_cur = p_end + prefix_len;
        }

    } while (1);

    packet->p_data = p_src;
    processing_size = (TIOSize)(p_end - p_src);
    packet->data_len = processing_size;
    packet->have_pts = 0;

    mpCurrentPtr += processing_size;
    mBufRemaingSize -= processing_size;

    if (mbRemoveBuggyFileBytes) {
        packet->data_len -= mRemoveBuggyFileByteNumber;
    }

    return EECode_OK;
}

EECode CHEVCFileParser::GetMediaInfo(SMediaInfo *info)
{
    if (!mbStartReadFile) {
        readDataFromFile();
    }

    if (!mbParseMediaInfo) {
        LOG_ERROR("can not find extradata, this file maybe not h264 file, or file is corrupted\n");
        return EECode_DataCorruption;
    }

    info->video_width = mVideoWidth;
    info->video_height = mVideoHeight;

    info->video_framerate_num = mVideoFramerateNum;
    info->video_framerate_den = mVideoFramerateDen;

    info->profile_indicator = mProfileIndicator;
    info->level_indicator = mLevelIndicator;

    info->format = StreamFormat_H265;

    return EECode_OK;
}

EECode CHEVCFileParser::getMediaInfo()
{
    TU8 *p_vps = NULL, *p_sps = NULL, *p_pps = NULL;
    TU32 vps_size = 0, sps_size = 0, pps_size = 0;

    EECode err = gfGetH265VPSSPSPPS(mpCurrentPtr, (TU32)mBufRemaingSize, p_vps, vps_size, p_sps, sps_size, p_pps, pps_size);
    if (DLikely(EECode_OK == err)) {
        gfGetH265SizeFromSPS(p_sps + 6, sps_size - 6, mVideoWidth, mVideoHeight);
    } else {
        LOG_ERROR("do not get extra data?\n");
        return EECode_DataCorruption;
    }

    return EECode_OK;
}

EECode CHEVCFileParser::readDataFromFile()
{
    TIOSize read_buf_size = mReadBlockSize;
    memcpy(mpReadBuffer, mpCurrentPtr, mBufRemaingSize);
    mpCurrentPtr = mpReadBuffer;

    EECode err = mpIO->Read(mpReadBuffer + mBufRemaingSize, 1, read_buf_size);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("CHEVCFileParser::ReadDataFromFile(), mpIO->Read() ret %d, %s\n", err, gfGetErrorCodeString(err));
        mbFileEOF = 1;
        return err;
    }

    mbStartReadFile = 1;
    mBufRemaingSize += read_buf_size;

    if (DUnlikely(!mbParseMediaInfo)) {
        mbParseMediaInfo = 1;
        err = getMediaInfo();
    }

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

