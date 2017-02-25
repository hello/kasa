/*******************************************************************************
 * hm_video_decoder.cpp
 *
 * History:
 *    2014/11/28 - [Zhi He] create file
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

#ifdef BUILD_MODULE_HEVC_HM_DEC

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

#include "TLibVideoIO/TVideoIOYuv.h"
#include "TLibCommon/TComList.h"
#include "TLibCommon/TComPicYuv.h"
#include "TLibDecoder/TDecTop.h"
#include "TLibDecoder/AnnexBread.h"
#include "TLibDecoder/NALread.h"
#if RExt__DECODER_DEBUG_BIT_STATISTICS
#include "TComCodingStatistics.h"
#endif

#include "hm_video_decoder.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#define DBUFFER_ALIGNMENT 32
#define DBUFFER_EXT_EDGE_SIZE 16

static EECode _new_video_frame_buffer_non_ext(CIBuffer *buffer, EPixelFMT pix_format, TU32 width, TU32 height)
{
    TU32 size = 0;
    TU32 aligned_width = 0, aligned_height = 0;
    TU8 *p = NULL, *palign = NULL;

    if (DUnlikely(!buffer)) {
        LOG_FATAL("_new_video_frame_buffer_non_ext, NULL buffer pointer\n");
        return EECode_BadParam;
    }

    if (DUnlikely(!width || !height)) {
        LOG_ERROR("!!!_new_video_frame_buffer_non_ext, zero width %d or height %d\n", width, height);
        return EECode_BadParam;
    }

    if (DUnlikely(buffer->mbAllocated)) {
        LOG_ERROR("!!!_new_video_frame_buffer_non_ext, already allocated\n");
        return EECode_BadParam;
    }

    if (DLikely(EPixelFMT_yuv420p == pix_format)) {
        buffer->mVideoWidth = width;
        buffer->mVideoHeight = height;
        aligned_width = (width + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1));
        aligned_height = (height + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1));
        buffer->mExtVideoWidth = aligned_width;
        buffer->mExtVideoHeight = aligned_height;
        buffer->mbExtEdge = 0;

        size = (buffer->mExtVideoWidth * buffer->mExtVideoHeight) * 3 / 2 + (DBUFFER_ALIGNMENT * 4);

        p = (TU8 *)DDBG_MALLOC(size, "VDFB");
        if (DUnlikely(!p)) {
            LOG_ERROR("_new_video_frame_buffer_non_ext, not enough memory %u, width/height with ext %d/%d\n", size, buffer->mExtVideoWidth, buffer->mExtVideoHeight);
            return EECode_NoMemory;
        }
        buffer->SetDataPtrBase(p, 0);
        buffer->SetDataMemorySize(size, 0);
    } else {
        LOG_ERROR("!!!_new_video_frame_buffer_non_ext, only support 420p now\n");
        return EECode_NotSupported;
    }

    size = buffer->mExtVideoWidth * buffer->mExtVideoHeight;
    palign = (TU8 *)(((TULong)p + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1)));
    buffer->SetDataPtr(palign, 0);
    palign = (TU8 *)(((TULong)palign + size + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1)));
    buffer->SetDataPtr(palign, 1);
    palign = (TU8 *)(((TULong)palign + (size >> 2) + (DBUFFER_ALIGNMENT - 1)) & (~(DBUFFER_ALIGNMENT - 1)));
    buffer->SetDataPtr(palign, 2);

    buffer->SetDataLineSize(buffer->mExtVideoWidth, 0);
    buffer->SetDataLineSize(buffer->mExtVideoWidth / 2, 1);
    buffer->SetDataLineSize(buffer->mExtVideoWidth / 2, 2);
    buffer->mbAllocated = 1;

    return EECode_OK;
}

static TU8 *__next_start_code_hevc(TU8 *p, TU32 len, TU32 &prefix_len, TU8 &nal_type)
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

IVideoDecoder *gfCreateHMHEVCVideoDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    return SHMVideoDecoder::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// SHMVideoDecoder
//
//-----------------------------------------------------------------------
SHMVideoDecoder::SHMVideoDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
    : inherited(pname, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpEngineMsgSink(pMsgSink)
    , mpBufferPool(NULL)
    , mpOutputPin(NULL)
    , mpExtraData(NULL)
    , mExtraDataBufferSize(0)
    , mExtraDataSize(0)
    , mbDecoderSetup(0)
    , mbNeedSendSyncPointBuffer(1)
    , mbNewSequence(1)
    , mVideoFramerateNum(DDefaultVideoFramerateNum)
    , mVideoFramerateDen(DDefaultVideoFramerateDen)
    , mVideoFramerate((float)DDefaultVideoFramerateNum / (float)DDefaultVideoFramerateDen)
    , mVideoWidth(0)
    , mVideoHeight(0)
    , mpSendNal(NULL)
    , mpSendDupNal(NULL)
    , mpNalUnit(NULL)
{

}

SHMVideoDecoder::~SHMVideoDecoder()
{
    if (mpSendNal) {
        delete mpSendNal;
        mpSendNal = NULL;
    }

    if (mpSendDupNal) {
        delete mpSendDupNal;
        mpSendDupNal = NULL;
    }

    if (mpExtraData) {
        DDBG_FREE(mpExtraData, "VDVE");
        mpExtraData = NULL;
        mExtraDataBufferSize = 0;
    }
}

EECode SHMVideoDecoder::Construct()
{
    pcListPic = NULL;
    loopFiltered = false;
    m_iPOCLastDisplay = (-MAX_INT);
    m_iSkipFrame = 0;
    m_iMaxTemporalLayer = -1;

    mpSendNal = new vector<uint8_t>;
    mpSendDupNal = new vector<uint8_t>;
    return EECode_OK;
}

IVideoDecoder *SHMVideoDecoder::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
{
    SHMVideoDecoder *result = new SHMVideoDecoder(pname, pPersistMediaConfig, pMsgSink, index);
    if (DUnlikely(result && result->Construct() != EECode_OK)) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *SHMVideoDecoder::GetObject0() const
{
    return (CObject *) this;
}

void SHMVideoDecoder::PrintStatus()
{
}

EECode SHMVideoDecoder::SetupContext(SVideoDecoderInputParam *param)
{
    if (!mpBufferPool) {
        LOGM_ERROR("not specified buffer pool\n");
        return EECode_BadState;
    }

    EECode err = setupDecoder();
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("setupDecoder fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

EECode SHMVideoDecoder::DestroyContext()
{
    destroyDecoder();
    return EECode_OK;
}

EECode SHMVideoDecoder::SetBufferPool(IBufferPool *buffer_pool)
{
    mpBufferPool = buffer_pool;
    return EECode_OK;
}

EECode SHMVideoDecoder::Start()
{
    return EECode_OK;
}

EECode SHMVideoDecoder::Stop()
{
    return EECode_OK;
}

EECode SHMVideoDecoder::Decode(CIBuffer *in_buffer, CIBuffer *&out_buffer)
{
    TU8 *p_nal_header = NULL;
    TU8 *p_next_nal_header = NULL;
    TU32 prefix_len = 0;
    TU32 cur_nal_size = 0;
    TU8 nal_type;
    EECode err = EECode_OK;

    TU8 *pdata = in_buffer->GetDataPtr(0);
    if (DUnlikely(!pdata)) {
        LOGM_ERROR("NULL data pointer\n");
        return EECode_BadParam;
    }

    TU32 datasize = in_buffer->GetDataSize();
    p_nal_header = __next_start_code_hevc(pdata, datasize, prefix_len, nal_type);
    if (p_nal_header) {
        datasize -= (TU32)(p_nal_header - pdata);
        datasize -= prefix_len;
        pdata = p_nal_header + prefix_len;

        while (datasize > 0) {
            p_next_nal_header = __next_start_code_hevc(p_nal_header + prefix_len, datasize, prefix_len, nal_type);
            if (p_next_nal_header) {
                cur_nal_size = (TU32)(p_next_nal_header - pdata);
                datasize -= (TU32)(cur_nal_size + prefix_len);
                TU8 *pdup_data = pdata;
                TU32 dup_nal_size = cur_nal_size;
                mpSendNal->clear();
                while (cur_nal_size) {
                    mpSendNal->push_back(pdata[0]);
                    pdata ++;
                    cur_nal_size --;
                }
                err = decode(mpSendNal, out_buffer);
                if (EECode_TryAgain == err) {
                    mpSendDupNal->clear();
                    while (dup_nal_size) {
                        mpSendDupNal->push_back(pdup_data[0]);
                        pdup_data ++;
                        dup_nal_size --;
                    }
                    decode(mpSendDupNal, out_buffer);
                }
                p_nal_header = p_next_nal_header;
                pdata = p_nal_header + prefix_len;
            } else {
                cur_nal_size = (TU32)(datasize);
                TU8 *pdup_data = pdata;
                TU32 dup_nal_size = cur_nal_size;
                mpSendNal->clear();
                while (cur_nal_size) {
                    mpSendNal->push_back(pdata[0]);
                    pdata ++;
                    cur_nal_size --;
                }
                err = decode(mpSendNal, out_buffer);
                if (EECode_TryAgain == err) {
                    mpSendDupNal->clear();
                    while (dup_nal_size) {
                        mpSendDupNal->push_back(pdup_data[0]);
                        pdup_data ++;
                        dup_nal_size --;
                    }
                    decode(mpSendDupNal, out_buffer);
                }
                break;
            }
        }

    } else {
        LOGM_ERROR("data corruption!\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode SHMVideoDecoder::decode(vector<uint8_t> *nalUnit, CIBuffer *&out_buffer)
{
    EECode err = EECode_OK;
    mpNalUnit = new InputNALUnit();

    Bool bNewPicture = false;
    if (nalUnit->empty()) {
        LOGM_ERROR("Warning: Attempt to decode an empty NAL unit\n");
    } else {
#ifdef DDUMP_INPUT
        {
            if (gpDebugLogFile) {
                fprintf(gpDebugLogFile, "read nal(%d), size(%d)\n", gnNalCount, nalUnit->size());
                fflush(gpDebugLogFile);
            }
            char binfilename[256] = {0};
            sprintf(binfilename, "dump/readnal_%06d.bin", gnNalCount);
            FILE *binfile = fopen(binfilename, "wb+");
            if (binfile) {
                unsigned int bytecount = nalUnit->size();
                unsigned int ii = 0;
                unsigned char val = 0;
                for (ii = 0; ii < bytecount; ii ++) {
                    val = (*nalUnit)[ii];
                    fwrite(&val, 1, 1, binfile);
                }
                fclose(binfile);
            }
        }
#endif

        hm_read_pointer(mpNalUnit, nalUnit);
        if ((m_iMaxTemporalLayer >= 0 && mpNalUnit->m_temporalId > m_iMaxTemporalLayer) || !isNaluWithinTargetDecLayerIdSet(mpNalUnit)) {
            bNewPicture = false;
        } else {

#ifdef DDUMP_INPUT
            {
                std::vector<uint8_t> input_fifo = mpNalUnit->m_Bitstream->getFIFO();
                if (gpDebugLogFile) {
                    fprintf(gpDebugLogFile, "nal(%d), size(%d), type(%d), tempid(%d)\n", gnNalCount, input_fifo.size(), mpNalUnit->m_nalUnitType, mpNalUnit->m_temporalId);
                    fflush(gpDebugLogFile);
                }
                char binfilename[256] = {0};
                sprintf(binfilename, "dump/nal_%06d.bin", gnNalCount);
                FILE *binfile = fopen(binfilename, "wb+");
                if (binfile) {
                    unsigned int bytecount = input_fifo.size();
                    unsigned int ii = 0;
                    unsigned char val = 0;
                    for (ii = 0; ii < bytecount; ii ++) {
                        val = input_fifo[ii];
                        fwrite(&val, 1, 1, binfile);
                    }
                    fclose(binfile);
                }
                gnNalCount ++;
            }
#endif

            bNewPicture = m_cTDecTop.decode(*mpNalUnit, m_iSkipFrame, m_iPOCLastDisplay);
            if (bNewPicture) {
                //hm decoder bug
                err = EECode_TryAgain;
            }
        }
    }

    if ((bNewPicture || mpNalUnit->m_nalUnitType == NAL_UNIT_EOS) && !m_cTDecTop.getFirstSliceInSequence()) {
        if (!loopFiltered) {
            m_cTDecTop.executeLoopFilters(poc, pcListPic);
        }
        loopFiltered = (mpNalUnit->m_nalUnitType == NAL_UNIT_EOS);
        if (mpNalUnit->m_nalUnitType == NAL_UNIT_EOS) {
            m_cTDecTop.setFirstSliceInSequence(true);
        }
    } else if ((bNewPicture || mpNalUnit->m_nalUnitType == NAL_UNIT_EOS) && m_cTDecTop.getFirstSliceInSequence()) {
        m_cTDecTop.setFirstSliceInPicture(true);
    }

    if (pcListPic) {
        // write reconstruction to file
        if (bNewPicture) {
            if (!out_buffer) {
                mpBufferPool->AllocBuffer(out_buffer, sizeof(CIBuffer));
            }
            if (!out_buffer->GetDataPtrBase(0)) {
                if (!mVideoWidth || !mVideoHeight) {
                    TComSPS *sps = m_cTDecTop.getActiveSPS();
                    if (DLikely(sps)) {
                        mVideoWidth = sps->getPicWidthInLumaSamples();
                        mVideoHeight = sps->getPicHeightInLumaSamples();
                    }
                }
                _new_video_frame_buffer_non_ext(out_buffer, EPixelFMT_yuv420p, mVideoWidth, mVideoHeight);
            }
            xWriteOutput(pcListPic, mpNalUnit->m_temporalId, out_buffer);
        }
        if ((bNewPicture || mpNalUnit->m_nalUnitType == NAL_UNIT_CODED_SLICE_CRA) && m_cTDecTop.getNoOutputPriorPicsFlag()) {
            m_cTDecTop.checkNoOutputPriorPics(pcListPic);
            m_cTDecTop.setNoOutputPriorPicsFlag(false);
        }

        if (bNewPicture && (mpNalUnit->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
                            || mpNalUnit->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP
                            || mpNalUnit->m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP
                            || mpNalUnit->m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_RADL
                            || mpNalUnit->m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_LP)) {
            if (!out_buffer) {
                mpBufferPool->AllocBuffer(out_buffer, sizeof(CIBuffer));
            }
            if (!out_buffer->GetDataPtrBase(0)) {
                if (!mVideoWidth || !mVideoHeight) {
                    TComSPS *sps = m_cTDecTop.getActiveSPS();
                    if (DLikely(sps)) {
                        mVideoWidth = sps->getPicWidthInLumaSamples();
                        mVideoHeight = sps->getPicHeightInLumaSamples();
                    }
                }
                _new_video_frame_buffer_non_ext(out_buffer, EPixelFMT_yuv420p, mVideoWidth, mVideoHeight);
            }
            xFlushOutput(pcListPic, out_buffer);
        }
        if (mpNalUnit->m_nalUnitType == NAL_UNIT_EOS) {
            if (!out_buffer) {
                mpBufferPool->AllocBuffer(out_buffer, sizeof(CIBuffer));
            }
            if (!out_buffer->GetDataPtrBase(0)) {
                if (!mVideoWidth || !mVideoHeight) {
                    TComSPS *sps = m_cTDecTop.getActiveSPS();
                    if (DLikely(sps)) {
                        mVideoWidth = sps->getPicWidthInLumaSamples();
                        mVideoHeight = sps->getPicHeightInLumaSamples();
                    }
                }
                _new_video_frame_buffer_non_ext(out_buffer, EPixelFMT_yuv420p, mVideoWidth, mVideoHeight);
            }
            xWriteOutput(pcListPic, mpNalUnit->m_temporalId, out_buffer);
            m_cTDecTop.setFirstSliceInPicture(false);
        }

        // write reconstruction to file -- for additional bumping as defined in C.5.2.3
        if (!bNewPicture && mpNalUnit->m_nalUnitType >= NAL_UNIT_CODED_SLICE_TRAIL_N && mpNalUnit->m_nalUnitType <= NAL_UNIT_RESERVED_VCL31) {
            if (!out_buffer) {
                mpBufferPool->AllocBuffer(out_buffer, sizeof(CIBuffer));
            }
            if (!out_buffer->GetDataPtrBase(0)) {
                if (!mVideoWidth || !mVideoHeight) {
                    TComSPS *sps = m_cTDecTop.getActiveSPS();
                    if (DLikely(sps)) {
                        mVideoWidth = sps->getPicWidthInLumaSamples();
                        mVideoHeight = sps->getPicHeightInLumaSamples();
                    }
                }
                _new_video_frame_buffer_non_ext(out_buffer, EPixelFMT_yuv420p, mVideoWidth, mVideoHeight);
            }
            xWriteOutput(pcListPic, mpNalUnit->m_temporalId, out_buffer);
        }
    }

    delete mpNalUnit;
    mpNalUnit = NULL;
    return err;
}

EECode SHMVideoDecoder::decode_direct(vector<uint8_t> *nalUnit)
{
    EECode err = EECode_OK;
    mpNalUnit = new InputNALUnit();

    Bool bNewPicture = false;
    if (nalUnit->empty()) {
        LOGM_ERROR("Warning: Attempt to decode an empty NAL unit\n");
    } else {
        hm_read_pointer(mpNalUnit, nalUnit);
        if ((m_iMaxTemporalLayer >= 0 && mpNalUnit->m_temporalId > m_iMaxTemporalLayer) || !isNaluWithinTargetDecLayerIdSet(mpNalUnit)) {
            bNewPicture = false;
        } else {
            bNewPicture = m_cTDecTop.decode(*mpNalUnit, m_iSkipFrame, m_iPOCLastDisplay);
            if (bNewPicture) {
                //hm decoder bug
                err = EECode_TryAgain;
            }
        }
    }

    if ((bNewPicture || mpNalUnit->m_nalUnitType == NAL_UNIT_EOS) && !m_cTDecTop.getFirstSliceInSequence()) {
        if (!loopFiltered) {
            m_cTDecTop.executeLoopFilters(poc, pcListPic);
        }
        loopFiltered = (mpNalUnit->m_nalUnitType == NAL_UNIT_EOS);
        if (mpNalUnit->m_nalUnitType == NAL_UNIT_EOS) {
            m_cTDecTop.setFirstSliceInSequence(true);
        }
    } else if ((bNewPicture || mpNalUnit->m_nalUnitType == NAL_UNIT_EOS) && m_cTDecTop.getFirstSliceInSequence()) {
        m_cTDecTop.setFirstSliceInPicture(true);
    }

    if (pcListPic) {
        // write reconstruction to file
        if (bNewPicture) {
            xWriteOutput_direct(pcListPic, mpNalUnit->m_temporalId);
        }
        if ((bNewPicture || mpNalUnit->m_nalUnitType == NAL_UNIT_CODED_SLICE_CRA) && m_cTDecTop.getNoOutputPriorPicsFlag()) {
            m_cTDecTop.checkNoOutputPriorPics(pcListPic);
            m_cTDecTop.setNoOutputPriorPicsFlag(false);
        }

        if (bNewPicture && (mpNalUnit->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL
                            || mpNalUnit->m_nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP
                            || mpNalUnit->m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_N_LP
                            || mpNalUnit->m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_RADL
                            || mpNalUnit->m_nalUnitType == NAL_UNIT_CODED_SLICE_BLA_W_LP)) {
            xFlushOutput_direct(pcListPic);
        }
        if (mpNalUnit->m_nalUnitType == NAL_UNIT_EOS) {
            xWriteOutput_direct(pcListPic, mpNalUnit->m_temporalId);
            m_cTDecTop.setFirstSliceInPicture(false);
        }

        // write reconstruction to file -- for additional bumping as defined in C.5.2.3
        if (!bNewPicture && mpNalUnit->m_nalUnitType >= NAL_UNIT_CODED_SLICE_TRAIL_N && mpNalUnit->m_nalUnitType <= NAL_UNIT_RESERVED_VCL31) {
            xWriteOutput_direct(pcListPic, mpNalUnit->m_temporalId);
        }
    }

    delete mpNalUnit;
    mpNalUnit = NULL;
    return err;
}

EECode SHMVideoDecoder::Flush()
{
    LOGM_ERROR("SHMVideoDecoder::Flush TO DO\n");
    return EECode_NoImplement;
}

EECode SHMVideoDecoder::Suspend()
{
    LOGM_ERROR("SHMVideoDecoder::Suspend TO DO\n");
    return EECode_NoImplement;
}

EECode SHMVideoDecoder::SetExtraData(TU8 *p, TMemSize size)
{
    if (mpExtraData) {
        if (size > mExtraDataBufferSize) {
            free(mpExtraData);
            mpExtraData = NULL;
            mExtraDataBufferSize = size;
        }
    } else {
        mExtraDataBufferSize = size;
    }

    if (!mpExtraData) {
        mpExtraData = (TU8 *) DDBG_MALLOC(mExtraDataBufferSize, "VDVE");
        if (!mpExtraData) {
            LOG_FATAL("SHMVideoDecoder::SetExtraData no memory\n");
            return EECode_NoMemory;
        }
    }

    mExtraDataSize = size;
    memcpy(mpExtraData, p, mExtraDataSize);

    return EECode_OK;
}

EECode SHMVideoDecoder::SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed)
{
    LOGM_ERROR("SHMVideoDecoder::SetPbRule TO DO\n");
    return EECode_NoImplement;
}

EECode SHMVideoDecoder::SetFrameRate(TUint framerate_num, TUint framerate_den)
{
    mVideoFramerateNum = framerate_num;
    mVideoFramerateDen = framerate_den;
    if (mVideoFramerateNum && mVideoFramerateDen) {
        mVideoFramerate = (float)mVideoFramerateNum / (float)mVideoFramerateDen;
    } else {
        mVideoFramerate = (float)DDefaultVideoFramerateNum / (float)DDefaultVideoFramerateDen;
    }
    mbNewSequence = 1;
    mbNeedSendSyncPointBuffer = 1;
    return EECode_OK;
}

EDecoderMode SHMVideoDecoder::GetDecoderMode() const
{
    return EDecoderMode_Direct;
}

EECode SHMVideoDecoder::DecodeDirect(CIBuffer *in_buffer)
{
    TU8 *p_nal_header = NULL;
    TU8 *p_next_nal_header = NULL;
    TU32 prefix_len = 0;
    TU32 cur_nal_size = 0;
    TU8 nal_type;
    EECode err = EECode_OK;

    TU8 *pdata = in_buffer->GetDataPtr(0);
    if (DUnlikely(!pdata)) {
        LOGM_ERROR("NULL data pointer\n");
        return EECode_BadParam;
    }

    TU32 datasize = in_buffer->GetDataSize();
    p_nal_header = __next_start_code_hevc(pdata, datasize, prefix_len, nal_type);
    if (p_nal_header) {
        datasize -= (TU32)(p_nal_header - pdata);
        datasize -= prefix_len;
        pdata = p_nal_header + prefix_len;

        while (datasize > 0) {
            p_next_nal_header = __next_start_code_hevc(p_nal_header + prefix_len, datasize, prefix_len, nal_type);
            if (p_next_nal_header) {
                cur_nal_size = (TU32)(p_next_nal_header - pdata);
                datasize -= (TU32)(cur_nal_size + prefix_len);
                TU8 *pdup_data = pdata;
                TU32 dup_nal_size = cur_nal_size;
                mpSendNal->clear();
                while (cur_nal_size) {
                    mpSendNal->push_back(pdata[0]);
                    pdata ++;
                    cur_nal_size --;
                }
                err = decode_direct(mpSendNal);
                if (EECode_TryAgain == err) {
                    mpSendDupNal->clear();
                    while (dup_nal_size) {
                        mpSendDupNal->push_back(pdup_data[0]);
                        pdup_data ++;
                        dup_nal_size --;
                    }
                    decode_direct(mpSendDupNal);
                }
                p_nal_header = p_next_nal_header;
                pdata = p_nal_header + prefix_len;
            } else {
                cur_nal_size = (TU32)(datasize);
                TU8 *pdup_data = pdata;
                TU32 dup_nal_size = cur_nal_size;
                mpSendNal->clear();
                while (cur_nal_size) {
                    mpSendNal->push_back(pdata[0]);
                    pdata ++;
                    cur_nal_size --;
                }
                err = decode_direct(mpSendNal);
                if (EECode_TryAgain == err) {
                    mpSendDupNal->clear();
                    while (dup_nal_size) {
                        mpSendDupNal->push_back(pdup_data[0]);
                        pdup_data ++;
                        dup_nal_size --;
                    }
                    decode_direct(mpSendDupNal);
                }
                break;
            }
        }

    } else {
        LOGM_ERROR("data corruption!\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode SHMVideoDecoder::SetBufferPoolDirect(IOutputPin *p_output_pin, IBufferPool *p_bufferpool)
{
    mpBufferPool = p_bufferpool;
    mpOutputPin = p_output_pin;
    return EECode_OK;
}

EECode SHMVideoDecoder::QueryFreeZoom(TUint &free_zoom, TU32 &fullness)
{
    LOGM_ERROR("SHMVideoDecoder::QueryFreeZoom TO DO\n");
    return EECode_NoImplement;
}

EECode SHMVideoDecoder::QueryLastDisplayedPTS(TTime &pts)
{
    LOGM_ERROR("SHMVideoDecoder::QueryLastDisplayedPTS TO DO\n");
    return EECode_NoImplement;
}

EECode SHMVideoDecoder::PushData(CIBuffer *in_buffer, TInt flush)
{
    LOGM_ERROR("SHMVideoDecoder::PushData TO DO\n");
    return EECode_NoImplement;
}

EECode SHMVideoDecoder::PullDecodedFrame(CIBuffer *out_buffer, TInt block_wait, TInt &remaining)
{
    LOGM_ERROR("SHMVideoDecoder::PullDecodedFrame TO DO\n");
    return EECode_NoImplement;
}

EECode SHMVideoDecoder::TuningPB(TU8 fw, TU16 frame_tick)
{
    LOGM_ERROR("SHMVideoDecoder::TuningPB TO DO\n");
    return EECode_NoImplement;
}

EECode SHMVideoDecoder::setupDecoder()
{
    if (DUnlikely(mbDecoderSetup)) {
        LOGM_WARN("decoder already setup?\n");
        return EECode_BadState;
    }

    if (DUnlikely(!mpBufferPool)) {
        LOGM_WARN("not set buffer pool\n");
        return EECode_BadState;
    }

    m_cTDecTop.create();

    LOGM_NOTICE("[flow]: after m_cTDecTop.create().\n");

    m_cTDecTop.init();
    m_cTDecTop.setDecodedPictureHashSEIEnabled(0);

    pcListPic = NULL;
    loopFiltered = false;
    m_iPOCLastDisplay = (-MAX_INT);
    m_iSkipFrame = 0;
    m_iMaxTemporalLayer = -1;

    mbDecoderSetup = 1;

    return EECode_OK;
}

void SHMVideoDecoder::destroyDecoder()
{
    if (mbDecoderSetup) {
        mbDecoderSetup = 0;
        xCleanOutput(pcListPic);
        m_cTDecTop.deletePicBuffer();
        m_cTDecTop.destroy();
    }
}

Bool SHMVideoDecoder::isNaluWithinTargetDecLayerIdSet(InputNALUnit *nalu)
{
    if (m_targetDecLayerIdSet.size() == 0) {
        return true;
    }

    for (std::vector<Int>::iterator it = m_targetDecLayerIdSet.begin(); it != m_targetDecLayerIdSet.end(); it++) {
        if (nalu->m_reservedZero6Bits == (*it)) {
            return true;
        }
    }

    return false;
}

/** \param pcListPic list of pictures to be written to file
    \todo            DYN_REF_FREE should be revised
 */
void SHMVideoDecoder::xWriteOutput(TComList<TComPic *> *pcListPic, UInt tId, CIBuffer *&out_buffer)
{
    if (pcListPic->empty()) {
        return;
    }

    TComList<TComPic *>::iterator iterPic   = pcListPic->begin();
    Int numPicsNotYetDisplayed = 0;
    Int dpbFullness = 0;
    TComSPS *activeSPS = m_cTDecTop.getActiveSPS();
    UInt numReorderPicsHighestTid;
    UInt maxDecPicBufferingHighestTid;
    UInt maxNrSublayers = activeSPS->getMaxTLayers();

    if (m_iMaxTemporalLayer == -1 || m_iMaxTemporalLayer >= maxNrSublayers) {
        numReorderPicsHighestTid = activeSPS->getNumReorderPics(maxNrSublayers - 1);
        maxDecPicBufferingHighestTid =  activeSPS->getMaxDecPicBuffering(maxNrSublayers - 1);
    } else {
        numReorderPicsHighestTid = activeSPS->getNumReorderPics(m_iMaxTemporalLayer);
        maxDecPicBufferingHighestTid = activeSPS->getMaxDecPicBuffering(m_iMaxTemporalLayer);
    }

    while (iterPic != pcListPic->end()) {
        TComPic *pcPic = *(iterPic);
        if (pcPic->getOutputMark() && pcPic->getPOC() > m_iPOCLastDisplay) {
            numPicsNotYetDisplayed++;
            dpbFullness++;
        } else if (pcPic->getSlice(0)->isReferenced()) {
            dpbFullness++;
        }
        iterPic++;
    }

    iterPic = pcListPic->begin();

    if (numPicsNotYetDisplayed > 2) {
        iterPic++;
    }

    TComPic *pcPic = *(iterPic);
    if (numPicsNotYetDisplayed > 2 && pcPic->isField()) {
        TComList<TComPic *>::iterator endPic   = pcListPic->end();
        endPic--;
        iterPic   = pcListPic->begin();
        while (iterPic != endPic) {
            TComPic *pcPicTop = *(iterPic);
            iterPic++;
            TComPic *pcPicBottom = *(iterPic);

            if (pcPicTop->getOutputMark() && pcPicBottom->getOutputMark() &&
                    (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid) &&
                    (!(pcPicTop->getPOC() % 2) && pcPicBottom->getPOC() == pcPicTop->getPOC() + 1) &&
                    (pcPicTop->getPOC() == m_iPOCLastDisplay + 1 || m_iPOCLastDisplay < 0)) {
                // write to file
                numPicsNotYetDisplayed = numPicsNotYetDisplayed - 2;

                copyFrame(pcPicTop->getPicYuvRec(), out_buffer);

                // update POC of display order
                m_iPOCLastDisplay = pcPicBottom->getPOC();

                // erase non-referenced picture in the reference picture list after display
                if (!pcPicTop->getSlice(0)->isReferenced() && pcPicTop->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPicTop->setReconMark(false);

                    // mark it should be extended later
                    pcPicTop->getPicYuvRec()->setBorderExtension(false);
#else
                    pcPicTop->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    break;
#endif
                }

                if (!pcPicBottom->getSlice(0)->isReferenced() && pcPicBottom->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPicBottom->setReconMark(false);

                    // mark it should be extended later
                    pcPicBottom->getPicYuvRec()->setBorderExtension(false);

#else
                    pcPicBottom->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    break;
#endif
                }
                pcPicTop->setOutputMark(false);
                pcPicBottom->setOutputMark(false);
            }
        }
    } else if (!pcPic->isField()) {
        iterPic = pcListPic->begin();

        while (iterPic != pcListPic->end()) {
            pcPic = *(iterPic);

            if (pcPic->getOutputMark() && pcPic->getPOC() > m_iPOCLastDisplay &&
                    (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid)) {
                // write to file
                numPicsNotYetDisplayed--;
                if (pcPic->getSlice(0)->isReferenced() == false) {
                    dpbFullness--;
                }

                copyFrame(pcPic->getPicYuvRec(), out_buffer);

                // update POC of display order
                m_iPOCLastDisplay = pcPic->getPOC();

                // erase non-referenced picture in the reference picture list after display
                if (!pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPic->setReconMark(false);

                    // mark it should be extended later
                    pcPic->getPicYuvRec()->setBorderExtension(false);

#else
                    pcPic->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    break;
#endif
                }
                pcPic->setOutputMark(false);
            }

            iterPic++;
        }
    }
}

void SHMVideoDecoder::xWriteOutput_direct(TComList<TComPic *> *pcListPic, UInt tId)
{
    if (pcListPic->empty()) {
        return;
    }

    TComList<TComPic *>::iterator iterPic   = pcListPic->begin();
    Int numPicsNotYetDisplayed = 0;
    Int dpbFullness = 0;
    TComSPS *activeSPS = m_cTDecTop.getActiveSPS();
    UInt numReorderPicsHighestTid;
    UInt maxDecPicBufferingHighestTid;
    UInt maxNrSublayers = activeSPS->getMaxTLayers();

    if (m_iMaxTemporalLayer == -1 || m_iMaxTemporalLayer >= maxNrSublayers) {
        numReorderPicsHighestTid = activeSPS->getNumReorderPics(maxNrSublayers - 1);
        maxDecPicBufferingHighestTid =  activeSPS->getMaxDecPicBuffering(maxNrSublayers - 1);
    } else {
        numReorderPicsHighestTid = activeSPS->getNumReorderPics(m_iMaxTemporalLayer);
        maxDecPicBufferingHighestTid = activeSPS->getMaxDecPicBuffering(m_iMaxTemporalLayer);
    }

    while (iterPic != pcListPic->end()) {
        TComPic *pcPic = *(iterPic);
        if (pcPic->getOutputMark() && pcPic->getPOC() > m_iPOCLastDisplay) {
            numPicsNotYetDisplayed++;
            dpbFullness++;
        } else if (pcPic->getSlice(0)->isReferenced()) {
            dpbFullness++;
        }
        iterPic++;
    }

    iterPic = pcListPic->begin();

    if (numPicsNotYetDisplayed > 2) {
        iterPic++;
    }

    TComPic *pcPic = *(iterPic);
    if (numPicsNotYetDisplayed > 2 && pcPic->isField()) {
        TComList<TComPic *>::iterator endPic   = pcListPic->end();
        endPic--;
        iterPic   = pcListPic->begin();
        while (iterPic != endPic) {
            TComPic *pcPicTop = *(iterPic);
            iterPic++;
            TComPic *pcPicBottom = *(iterPic);

            if (pcPicTop->getOutputMark() && pcPicBottom->getOutputMark() &&
                    (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid) &&
                    (!(pcPicTop->getPOC() % 2) && pcPicBottom->getPOC() == pcPicTop->getPOC() + 1) &&
                    (pcPicTop->getPOC() == m_iPOCLastDisplay + 1 || m_iPOCLastDisplay < 0)) {
                // write to file
                numPicsNotYetDisplayed = numPicsNotYetDisplayed - 2;

                CIBuffer *out_buffer = allocBuffer();
                copyFrame(pcPicTop->getPicYuvRec(), out_buffer);
                sendBuffer(out_buffer);

                // update POC of display order
                m_iPOCLastDisplay = pcPicBottom->getPOC();

                // erase non-referenced picture in the reference picture list after display
                if (!pcPicTop->getSlice(0)->isReferenced() && pcPicTop->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPicTop->setReconMark(false);

                    // mark it should be extended later
                    pcPicTop->getPicYuvRec()->setBorderExtension(false);
#else
                    pcPicTop->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    continue;
#endif
                }

                if (!pcPicBottom->getSlice(0)->isReferenced() && pcPicBottom->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPicBottom->setReconMark(false);

                    // mark it should be extended later
                    pcPicBottom->getPicYuvRec()->setBorderExtension(false);

#else
                    pcPicBottom->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    continue;
#endif
                }
                pcPicTop->setOutputMark(false);
                pcPicBottom->setOutputMark(false);
            }
        }
    } else if (!pcPic->isField()) {
        iterPic = pcListPic->begin();

        while (iterPic != pcListPic->end()) {
            pcPic = *(iterPic);

            if (pcPic->getOutputMark() && pcPic->getPOC() > m_iPOCLastDisplay &&
                    (numPicsNotYetDisplayed >  numReorderPicsHighestTid || dpbFullness > maxDecPicBufferingHighestTid)) {
                // write to file
                numPicsNotYetDisplayed--;
                if (pcPic->getSlice(0)->isReferenced() == false) {
                    dpbFullness--;
                }

                CIBuffer *out_buffer = allocBuffer();
                copyFrame(pcPic->getPicYuvRec(), out_buffer);
                sendBuffer(out_buffer);

                // update POC of display order
                m_iPOCLastDisplay = pcPic->getPOC();

                // erase non-referenced picture in the reference picture list after display
                if (!pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPic->setReconMark(false);

                    // mark it should be extended later
                    pcPic->getPicYuvRec()->setBorderExtension(false);

#else
                    pcPic->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    continue;
#endif
                }
                pcPic->setOutputMark(false);
            }

            iterPic++;
        }
    }
}

/** \param pcListPic list of pictures to be written to file
    \todo            DYN_REF_FREE should be revised
 */
void SHMVideoDecoder::xFlushOutput(TComList<TComPic *> *pcListPic, CIBuffer *&out_buffer)
{
    if (!pcListPic || pcListPic->empty()) {
        return;
    }
    TComList<TComPic *>::iterator iterPic   = pcListPic->begin();

    iterPic   = pcListPic->begin();
    TComPic *pcPic = *(iterPic);

    if (pcPic->isField()) {
        TComList<TComPic *>::iterator endPic   = pcListPic->end();
        endPic--;
        TComPic *pcPicTop, *pcPicBottom = NULL;
        while (iterPic != endPic) {
            pcPicTop = *(iterPic);
            iterPic++;
            pcPicBottom = *(iterPic);

            if (pcPicTop->getOutputMark() && pcPicBottom->getOutputMark() && !(pcPicTop->getPOC() % 2) && (pcPicBottom->getPOC() == pcPicTop->getPOC() + 1)) {
                copyFrame(pcPicTop->getPicYuvRec(), out_buffer);

                // update POC of display order
                m_iPOCLastDisplay = pcPicBottom->getPOC();

                // erase non-referenced picture in the reference picture list after display
                if (!pcPicTop->getSlice(0)->isReferenced() && pcPicTop->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPicTop->setReconMark(false);

                    // mark it should be extended later
                    pcPicTop->getPicYuvRec()->setBorderExtension(false);
#else
                    pcPicTop->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    break;
#endif
                }

                if (!pcPicBottom->getSlice(0)->isReferenced() && pcPicBottom->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPicBottom->setReconMark(false);

                    // mark it should be extended later
                    pcPicBottom->getPicYuvRec()->setBorderExtension(false);
#else
                    pcPicBottom->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    break;
#endif
                }
                pcPicTop->setOutputMark(false);
                pcPicBottom->setOutputMark(false);

#if !DYN_REF_FREE
                if (pcPicTop) {
                    pcPicTop->destroy();
                    delete pcPicTop;
                    pcPicTop = NULL;
                }
#endif
            }
        }

        if (pcPicBottom) {
            pcPicBottom->destroy();
            delete pcPicBottom;
            pcPicBottom = NULL;
        }
    } else {
        while (iterPic != pcListPic->end()) {
            pcPic = *(iterPic);

            if (pcPic->getOutputMark()) {
                copyFrame(pcPic->getPicYuvRec(), out_buffer);

                // update POC of display order
                m_iPOCLastDisplay = pcPic->getPOC();

                // erase non-referenced picture in the reference picture list after display
                if (!pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPic->setReconMark(false);

                    // mark it should be extended later
                    pcPic->getPicYuvRec()->setBorderExtension(false);
#else
                    pcPic->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    break;
#endif
                }
                pcPic->setOutputMark(false);
            }
#if !DYN_REF_FREE
            if (pcPic != NULL) {
                pcPic->destroy();
                delete pcPic;
                pcPic = NULL;
            }
#endif
            iterPic++;
        }
    }
    pcListPic->clear();
    m_iPOCLastDisplay = -MAX_INT;
}

void SHMVideoDecoder::xFlushOutput_direct(TComList<TComPic *> *pcListPic)
{
    if (!pcListPic || pcListPic->empty()) {
        return;
    }
    TComList<TComPic *>::iterator iterPic   = pcListPic->begin();

    iterPic   = pcListPic->begin();
    TComPic *pcPic = *(iterPic);

    if (pcPic->isField()) {
        TComList<TComPic *>::iterator endPic   = pcListPic->end();
        endPic--;
        TComPic *pcPicTop, *pcPicBottom = NULL;
        while (iterPic != endPic) {
            pcPicTop = *(iterPic);
            iterPic++;
            pcPicBottom = *(iterPic);

            if (pcPicTop->getOutputMark() && pcPicBottom->getOutputMark() && !(pcPicTop->getPOC() % 2) && (pcPicBottom->getPOC() == pcPicTop->getPOC() + 1)) {

                CIBuffer *out_buffer = allocBuffer();
                copyFrame(pcPicTop->getPicYuvRec(), out_buffer);
                sendBuffer(out_buffer);

                // update POC of display order
                m_iPOCLastDisplay = pcPicBottom->getPOC();

                // erase non-referenced picture in the reference picture list after display
                if (!pcPicTop->getSlice(0)->isReferenced() && pcPicTop->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPicTop->setReconMark(false);

                    // mark it should be extended later
                    pcPicTop->getPicYuvRec()->setBorderExtension(false);
#else
                    pcPicTop->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    continue;
#endif
                }

                if (!pcPicBottom->getSlice(0)->isReferenced() && pcPicBottom->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPicBottom->setReconMark(false);

                    // mark it should be extended later
                    pcPicBottom->getPicYuvRec()->setBorderExtension(false);
#else
                    pcPicBottom->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    continue;
#endif
                }
                pcPicTop->setOutputMark(false);
                pcPicBottom->setOutputMark(false);

#if !DYN_REF_FREE
                if (pcPicTop) {
                    pcPicTop->destroy();
                    delete pcPicTop;
                    pcPicTop = NULL;
                }
#endif
            }
        }

        if (pcPicBottom) {
            pcPicBottom->destroy();
            delete pcPicBottom;
            pcPicBottom = NULL;
        }
    } else {
        while (iterPic != pcListPic->end()) {
            pcPic = *(iterPic);

            if (pcPic->getOutputMark()) {

                CIBuffer *out_buffer = allocBuffer();
                copyFrame(pcPic->getPicYuvRec(), out_buffer);
                sendBuffer(out_buffer);

                // update POC of display order
                m_iPOCLastDisplay = pcPic->getPOC();

                // erase non-referenced picture in the reference picture list after display
                if (!pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPic->setReconMark(false);

                    // mark it should be extended later
                    pcPic->getPicYuvRec()->setBorderExtension(false);
#else
                    pcPic->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    continue;
#endif
                }
                pcPic->setOutputMark(false);
            }
#if !DYN_REF_FREE
            if (pcPic != NULL) {
                pcPic->destroy();
                delete pcPic;
                pcPic = NULL;
            }
#endif
            iterPic++;
        }
    }
    pcListPic->clear();
    m_iPOCLastDisplay = -MAX_INT;
}

void SHMVideoDecoder::xCleanOutput(TComList<TComPic *> *pcListPic)
{
    if (!pcListPic || pcListPic->empty()) {
        return;
    }
    TComList<TComPic *>::iterator iterPic   = pcListPic->begin();

    iterPic   = pcListPic->begin();
    TComPic *pcPic = *(iterPic);

    if (pcPic->isField()) {
        TComList<TComPic *>::iterator endPic   = pcListPic->end();
        endPic--;
        TComPic *pcPicTop, *pcPicBottom = NULL;
        while (iterPic != endPic) {
            pcPicTop = *(iterPic);
            iterPic++;
            pcPicBottom = *(iterPic);

            if (pcPicTop->getOutputMark() && pcPicBottom->getOutputMark() && !(pcPicTop->getPOC() % 2) && (pcPicBottom->getPOC() == pcPicTop->getPOC() + 1)) {
                // update POC of display order
                m_iPOCLastDisplay = pcPicBottom->getPOC();

                // erase non-referenced picture in the reference picture list after display
                if (!pcPicTop->getSlice(0)->isReferenced() && pcPicTop->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPicTop->setReconMark(false);

                    // mark it should be extended later
                    pcPicTop->getPicYuvRec()->setBorderExtension(false);
#else
                    pcPicTop->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    continue;
#endif
                }

                if (!pcPicBottom->getSlice(0)->isReferenced() && pcPicBottom->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPicBottom->setReconMark(false);

                    // mark it should be extended later
                    pcPicBottom->getPicYuvRec()->setBorderExtension(false);
#else
                    pcPicBottom->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    continue;
#endif
                }
                pcPicTop->setOutputMark(false);
                pcPicBottom->setOutputMark(false);

#if !DYN_REF_FREE
                if (pcPicTop) {
                    pcPicTop->destroy();
                    delete pcPicTop;
                    pcPicTop = NULL;
                }
#endif
            }
        }

        if (pcPicBottom) {
            pcPicBottom->destroy();
            delete pcPicBottom;
            pcPicBottom = NULL;
        }
    } else {
        while (iterPic != pcListPic->end()) {
            pcPic = *(iterPic);

            if (pcPic->getOutputMark()) {

                // update POC of display order
                m_iPOCLastDisplay = pcPic->getPOC();

                // erase non-referenced picture in the reference picture list after display
                if (!pcPic->getSlice(0)->isReferenced() && pcPic->getReconMark() == true) {
#if !DYN_REF_FREE
                    pcPic->setReconMark(false);

                    // mark it should be extended later
                    pcPic->getPicYuvRec()->setBorderExtension(false);
#else
                    pcPic->destroy();
                    pcListPic->erase(iterPic);
                    iterPic = pcListPic->begin(); // to the beginning, non-efficient way, have to be revised!
                    continue;
#endif
                }
                pcPic->setOutputMark(false);
            }
#if !DYN_REF_FREE
            if (pcPic != NULL) {
                pcPic->destroy();
                delete pcPic;
                pcPic = NULL;
            }
#endif
            iterPic++;
        }
    }
    pcListPic->clear();
    m_iPOCLastDisplay = -MAX_INT;
}

void SHMVideoDecoder::copyFrame(TComPicYuv *pic_yuv, CIBuffer *&out_buffer)
{
    TU32 i = 0, j = 0, k = 0;
    TU32 height = 0;
    TU32 width = 0;
    Pel *psrc = NULL;
    TU8 *pdst = NULL;
    TU32 src_stride = 0, dst_stride = 0;

    for (i = 0; i < MAX_NUM_COMPONENT; i ++) {
        if (!i) {
            width = out_buffer->mVideoWidth;
            height = out_buffer->mVideoHeight;
        } else {
            width = out_buffer->mVideoWidth / 2;
            height = out_buffer->mVideoHeight / 2;
        }
        psrc = pic_yuv->getAddr((const ComponentID)i);
        pdst = out_buffer->GetDataPtr(i);
        src_stride = pic_yuv->getStride((const ComponentID)i);
        dst_stride = out_buffer->GetDataLineSize(i);
        for (j = 0; j < height; j ++) {
            for (k = 0; k < width; k ++) {
                pdst[k] = psrc[k];
            }
            psrc += src_stride;
            pdst += dst_stride;
        }
    }
}

CIBuffer *SHMVideoDecoder::allocBuffer()
{
    CIBuffer *buffer = NULL;
    mpBufferPool->AllocBuffer(buffer, sizeof(CIBuffer));
    if (DUnlikely(!buffer->GetDataPtrBase(0))) {
        if (!mVideoWidth || !mVideoHeight) {
            TComSPS *sps = m_cTDecTop.getActiveSPS();
            if (DLikely(sps)) {
                mVideoWidth = sps->getPicWidthInLumaSamples();
                mVideoHeight = sps->getPicHeightInLumaSamples();
            }
        }
        _new_video_frame_buffer_non_ext(buffer, EPixelFMT_yuv420p, mVideoWidth, mVideoHeight);
    }

    return buffer;
}

void SHMVideoDecoder::sendBuffer(CIBuffer *out_buffer)
{
    if (!mbNeedSendSyncPointBuffer) {
        out_buffer->SetBufferFlags(0);
    } else {
        if (mbNewSequence) {
            out_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::NEW_SEQUENCE);
            mbNewSequence = 0;
        } else {
            out_buffer->SetBufferFlags(CIBuffer::SYNC_POINT);
        }
        out_buffer->mVideoFrameRateNum = mVideoFramerateNum;
        out_buffer->mVideoFrameRateDen = mVideoFramerateDen;
        out_buffer->mVideoRoughFrameRate = mVideoFramerate;
        if (!mVideoWidth || !mVideoHeight) {
            TComSPS *sps = m_cTDecTop.getActiveSPS();
            if (DLikely(sps)) {
                mVideoWidth = sps->getPicWidthInLumaSamples();
                mVideoHeight = sps->getPicHeightInLumaSamples();
            }
        }
        out_buffer->mVideoWidth = mVideoWidth;
        out_buffer->mVideoHeight = mVideoHeight;
        mbNeedSendSyncPointBuffer = 0;
    }

    mpOutputPin->SendBuffer(out_buffer);
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

