/*******************************************************************************
 * connecter_pinmuxer.cpp
 *
 * History:
 *    2013/11/26 - [Zhi He] create file
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#include <pthread.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "mw_internal.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "connecter_pinmuxer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

IFilter *gfCreateConnecterPinmuxerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CConnecterPinmuxer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CConnecterPinmuxer
//
//-----------------------------------------------------------------------

IFilter *CConnecterPinmuxer::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CConnecterPinmuxer *result = new CConnecterPinmuxer(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Delete();
        result = NULL;
    }
    return result;
}

CObject *CConnecterPinmuxer::GetObject0() const
{
    return (CObject *) this;
}

CConnecterPinmuxer::CConnecterPinmuxer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index)
    , mpMutex(NULL)
    , mnCurrentInputPinIndex(0)
    , mnCurrentInputPinNumber(0)
    //, mnCurrentSubOutputPinNumber(0)
    , mpCurInputPin(NULL)
    , mpLastCurInputPin(NULL)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , mType(StreamType_Audio)
    , mpExtraData(NULL)
    , mExtraDataSize(0)
    , mExtraDataBufferSize(0)
    , mbStopped(0)
{
    TUint i = 0;
    for (i = 0; i < EConstPinmuxerMaxInputPinNumber; i ++) {
        mpInputPins[i] = NULL;
    }
}

EECode CConnecterPinmuxer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleConnecterPinmuxer);
    mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);

    mpMutex = gfCreateMutex();
    if (!mpMutex) {
        LOG_FATAL("CConnecterPinmuxer: gfCreateMutex() fail\n");
    }

    return EECode_OK;
}

CConnecterPinmuxer::~CConnecterPinmuxer()
{
    TUint i = 0;

    LOGM_RESOURCE("CConnecterPinmuxer::~CConnecterPinmuxer(), before delete inputpins.\n");
    for (i = 0; i < EConstPinmuxerMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
        mnCurrentInputPinNumber = 0;
    }

    LOGM_RESOURCE("CConnecterPinmuxer::~CConnecterPinmuxer(), before delete output pin.\n");
    if (mpOutputPin) {
        mpOutputPin->Delete();
        mpOutputPin = NULL;
    }

    if (mpExtraData) {
        free(mpExtraData);
        mpExtraData = NULL;
    }
    mExtraDataBufferSize = 0;

    LOGM_RESOURCE("CConnecterPinmuxer::~CConnecterPinmuxer(), end.\n");
}

void CConnecterPinmuxer::Delete()
{
    TUint i = 0;

    LOGM_RESOURCE("CConnecterPinmuxer::Delete(), before delete inputpins.\n");
    for (i = 0; i < EConstPinmuxerMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
        mnCurrentInputPinNumber = 0;
    }

    LOGM_RESOURCE("CConnecterPinmuxer::Delete(), before delete output pin.\n");
    if (mpOutputPin) {
        mpOutputPin->Delete();
        mpOutputPin = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    LOGM_RESOURCE("CConnecterPinmuxer::Delete(), before inherited::Delete().\n");
    inherited::Delete();
}

EECode CConnecterPinmuxer::Initialize(TChar *p_param)
{
    LOGM_INFO("[Pin Muxer flow], CConnecterPinmuxer::Initialize()\n");

    return EECode_OK;
}

EECode CConnecterPinmuxer::Finalize()
{
    LOGM_INFO("[Pin Muxer flow], CConnecterPinmuxer::Finalize()\n");
    return EECode_OK;
}

EECode CConnecterPinmuxer::Run()
{
    //debug assert
    DASSERT(mpOutputPin);
    DASSERT(mpInputPins[0]);

    return EECode_OK;
}

EECode CConnecterPinmuxer::Exit()
{
    return EECode_OK;
}

EECode CConnecterPinmuxer::Start()
{
    //debug assert
    DASSERT(mpOutputPin);
    DASSERT(mpInputPins[0]);
    mbStopped = 0;

    return EECode_OK;
}

EECode CConnecterPinmuxer::Stop()
{
    //debug assert
    DASSERT(mpOutputPin);
    DASSERT(mpInputPins[0]);

    AUTO_LOCK(mpMutex);
    TUint i = 0;
    mbStopped = 1;
    for (i = 0; i < EConstPinmuxerMaxInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Purge(1);
        }
    }

    return EECode_OK;
}

void CConnecterPinmuxer::Pause()
{

}

void CConnecterPinmuxer::Resume()
{

}

void CConnecterPinmuxer::Flush()
{

}

void CConnecterPinmuxer::ResumeFlush()
{

}

EECode CConnecterPinmuxer::Suspend(TUint release_flag)
{
    return EECode_OK;
}

EECode CConnecterPinmuxer::ResumeSuspend(TComponentIndex input_index)
{
    return EECode_OK;
}

EECode CConnecterPinmuxer::SendCmd(TUint cmd)
{
    return EECode_OK;
}

void CConnecterPinmuxer::PostMsg(TUint cmd)
{
}

void CConnecterPinmuxer::saveExtraData(TU8 *p, TMemSize size)
{
    if (DLikely(p && size)) {
        if (mpExtraData && (size > mExtraDataBufferSize)) {
            DDBG_FREE(mpExtraData, "CPEX");
            mpExtraData = NULL;
            mExtraDataBufferSize = 0;
        }

        if (!mpExtraData) {
            mpExtraData = (TU8 *) DDBG_MALLOC(size, "CPEX");
            if (DLikely(mpExtraData)) {
                mExtraDataBufferSize = size;
            }
        }

        if (mpExtraData) {
            memcpy(mpExtraData, p, size);
            mExtraDataSize = size;
#if 0
            LOG_INFO("inputPin %p received %s ExtraData size %d, first 8 bytes: 0x%02x 0x%02x 0x%02x 0x%02x, 0x%02x 0x%02x 0x%02x 0x%02x\n", this, (EBufferType_VideoExtraData == pBuffer->GetBufferType()) ? "video" : "audio", size, \
                     p[0], p[1], p[2], p[3], \
                     p[4], p[5], p[6], p[7]);
#endif
        } else {
            LOGM_FATAL("save ExtraData %p size %lu, NO memory to save it.\n", p, size);
        }
    }

}

EECode CConnecterPinmuxer::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_NOTICE("SwitchInput(%d) start\n", focus_input_index);

    AUTO_LOCK(mpMutex);

    DASSERT(mpBufferPool);
    if (DLikely(mpBufferPool)) {
        if (DUnlikely(0 == mpBufferPool->GetFreeBufferCnt())) {
            LOGM_WARN("do not invoke this api too frequently, or the pileline is blocked now, please check code\n");
            return EECode_BadState;
        }
    } else {
        LOGM_FATAL("NULL mpBufferPool\n");
        return EECode_BadState;
    }

    if (DLikely(focus_input_index < mnCurrentInputPinNumber)) {
        if (DLikely(focus_input_index != mnCurrentInputPinIndex)) {
            mpInputPins[mnCurrentInputPinIndex]->Purge(1);

            LOGM_NOTICE("SwitchInput: after mnCurrentInputPinIndex %d purge\n", mnCurrentInputPinIndex);

            if (DLikely(mpOutputPin)) {
                CIBuffer *sync_buffer = NULL;
                StreamFormat format;
                EECode err = EECode_OK;

                if (DLikely(StreamType_Audio == mType)) {
                    TU32 channels = 2;
                    TU32 sample_frequency;
                    AudioSampleFMT sample_fmt;
                    TU8 *p_extra_data = NULL;
                    TMemSize extra_data_size = 0;
                    TU32 session_number = 0;

                    err = mpInputPins[focus_input_index]->GetAudioParams(format, channels, sample_frequency, sample_fmt, p_extra_data, extra_data_size, session_number);
                    if (DLikely(EECode_OK == err)) {
                        saveExtraData(p_extra_data, extra_data_size);
                        mpOutputPin->AllocBuffer(sync_buffer);
                        sync_buffer->SetDataPtrBase(NULL);
                        sync_buffer->SetDataPtr(mpExtraData);
                        sync_buffer->SetDataSize(mExtraDataSize);
                        sync_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA);
                        sync_buffer->SetBufferType(EBufferType_AudioExtraData);

                        sync_buffer->mAudioChannelNumber = channels;
                        sync_buffer->mAudioSampleRate = sample_frequency;
                        sync_buffer->mAudioSampleFormat = (TU8)sample_fmt;
                        mpOutputPin->SendBuffer(sync_buffer);
                    } else {
                        LOGM_WARN("GetAudioParams fail\n");
                        //return EECode_BadState;
                    }
                } else if (DLikely(StreamType_Video == mType)) {
                    TU32 video_width, video_height;
                    TU8 *p_extra_data = NULL;
                    TMemSize extra_data_size = 0;
                    TU32 session_number = 0;

                    err = mpInputPins[focus_input_index]->GetVideoParams(format, video_width, video_height, p_extra_data, extra_data_size, session_number);
                    if (DLikely(EECode_OK == err)) {
                        saveExtraData(p_extra_data, extra_data_size);
                        mpOutputPin->AllocBuffer(sync_buffer);
                        sync_buffer->SetDataPtrBase(NULL);
                        sync_buffer->SetDataPtr(mpExtraData);
                        sync_buffer->SetDataSize(mExtraDataSize);
                        sync_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA);
                        sync_buffer->SetBufferType(EBufferType_VideoExtraData);

                        sync_buffer->mVideoWidth = video_width;
                        sync_buffer->mVideoHeight = video_height;
                        mpOutputPin->SendBuffer(sync_buffer);
                    } else {
                        LOGM_FATAL("GetVideoParams fail\n");
                        return EECode_BadState;
                    }
                } else {
                    LOGM_FATAL("bad mType %d\n", mType);
                    return EECode_InternalLogicalBug;
                }
            } else {
                LOGM_FATAL("NULL output pin\n");
                return EECode_InternalLogicalBug;
            }

            mpInputPins[focus_input_index]->Enable(1);
            LOGM_NOTICE("SwitchInput: after focus_input_index %d Enable\n", focus_input_index);
            mnCurrentInputPinIndex = focus_input_index;
            mpCurInputPin = mpInputPins[mnCurrentInputPinIndex];
        } else {
            mpInputPins[focus_input_index]->Enable(1);
        }
        LOGM_NOTICE("SwitchInput done\n");
        return EECode_OK;
    } else {
        LOGM_FATAL("BAD pin index %d, max value %d\n", focus_input_index, mnCurrentInputPinNumber);
    }

    LOGM_NOTICE("SwitchInput fail\n");
    return EECode_BadParam;
}

EECode CConnecterPinmuxer::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CConnecterPinmuxer::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    if (DUnlikely(((StreamType)type) != mType)) {
        LOGM_ERROR("bad pin type %d, %d in CConnecterPinmuxer\n", type, mType);
        return EECode_InternalLogicalBug;
    }

    if (!mpOutputPin) {
        mpOutputPin = COutputPin::Create("[output pin for CConnecterPinmuxer filter]", (IFilter *) this);

        if (!mpOutputPin)  {
            LOGM_FATAL("COutputPin::Create() fail?\n");
            return EECode_NoMemory;
        }

        mpBufferPool = CBufferPool::Create("[Buffer pool for pin muxer output pin]", 16);
        if (!mpBufferPool)  {
            LOGM_FATAL("CBufferPool::Create() fail?\n");
            return EECode_NoMemory;
        }

        mpOutputPin->SetBufferPool(mpBufferPool);
    }

    index = 0;
    EECode err = mpOutputPin->AddSubPin(sub_index);
    //mnCurrentSubOutputPinNumber = sub_index + 1;

    return err;
}

EECode CConnecterPinmuxer::AddInputPin(TUint &index, TUint type)
{
    if (DLikely(mType == (StreamType)type)) {
        if (mnCurrentInputPinNumber >= EConstPinmuxerMaxInputPinNumber) {
            LOGM_ERROR("Max input pin number reached, mnCurrentInputPinNumber %d.\n", mnCurrentInputPinNumber);
            return EECode_InternalLogicalBug;
        }

        index = mnCurrentInputPinNumber;
        DASSERT(!mpInputPins[mnCurrentInputPinNumber]);
        if (mpInputPins[mnCurrentInputPinNumber]) {
            LOGM_FATAL("mpInputPins[mnCurrentInputPinNumber] here, must have problems!!! please check it\n");
        }

        mpInputPins[mnCurrentInputPinNumber] = CBypassInputPin::Create("[input pin for CConnecterPinmuxer]", (IFilter *) this, (StreamType) type);
        DASSERT(mpInputPins[mnCurrentInputPinNumber]);

        if (!mpCurInputPin) {
            mpCurInputPin = mpInputPins[mnCurrentInputPinNumber];
        } else {
            mpInputPins[mnCurrentInputPinNumber]->Enable(0);
        }

        mnCurrentInputPinNumber ++;
        return EECode_OK;
    } else {
        LOGM_FATAL("BAD input pin type %d, mType %d\n", type, mType);
        return EECode_InternalLogicalBug;
    }

    return EECode_InternalLogicalBug;
}

IOutputPin *CConnecterPinmuxer::GetOutputPin(TUint index, TUint sub_index)
{
    return mpOutputPin;
}

IInputPin *CConnecterPinmuxer::GetInputPin(TUint index)
{
    if (index < mnCurrentInputPinNumber) {
        return mpInputPins[index];
    }

    LOGM_FATAL("BAD index %d, exceed mnCurrentInputPinNumber %d\n", index, mnCurrentInputPinNumber);
    return NULL;
}

EECode CConnecterPinmuxer::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    SProcessBuffer *p = NULL;

    AUTO_LOCK(mpMutex);

    if (DUnlikely(mbStopped)) {
        return EECode_OK;
    }

    if (DLikely(EFilterPropertyType_process_buffer == property_type)) {
        p = (SProcessBuffer *)p_param;
        if (DLikely(p)) {
            if (DLikely(p->pin_owner == mpCurInputPin)) {
                if (DLikely(mpOutputPin)) {
                    DASSERT(p->p_buffer);
                    mpOutputPin->SendBuffer(p->p_buffer);
                    mDebugHeartBeat ++;
                    return EECode_OK;
                } else {
                    LOGM_FATAL("no output pin\n");
                }
            } else {
                LOGM_WARN("not expected input pin\n");
            }

            if (p->p_buffer) {
                p->p_buffer->Release();
            }
        } else {
            LOGM_FATAL("NULL params\n");
        }
    } else if (EFilterPropertyType_pinmuxer_set_type == property_type) {
        SFilterGenericPropertyParams *params = (SFilterGenericPropertyParams *)p_param;
        if (DLikely(params)) {
            if (StreamType_Video == params->param0) {
                mType = StreamType_Video;
            } else if (StreamType_Audio == params->param0) {
                mType = StreamType_Audio;
            } else {
                LOGM_ERROR("BAD pin type %d\n", params->param0);
                return EECode_BadParam;
            }
        } else {
            LOGM_FATAL("NULL params\n");
        }
    } else {
        LOGM_FATAL("not processed property_type%d\n", property_type);
    }

    return EECode_Error;
}

void CConnecterPinmuxer::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = mnCurrentInputPinNumber;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CConnecterPinmuxer::PrintStatus()
{
    TUint i = 0;

    LOGM_PRINTF("mnCurrentInputPinNumber %d, mnCurrentInputPinIndex %d, heart beat %d\n", mnCurrentInputPinNumber, mnCurrentInputPinIndex, mDebugHeartBeat);

    for (i = 0; i < mnCurrentInputPinNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->PrintStatus();
        }
    }

    if (mpCurInputPin) {
        mpCurInputPin->PrintStatus();
    }

    if (mpOutputPin) {
        mpOutputPin->PrintStatus();
    }

    mDebugHeartBeat = 0;

    return;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

