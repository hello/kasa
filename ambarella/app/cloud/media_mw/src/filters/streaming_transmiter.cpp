/*******************************************************************************
 * streaming_transmiter.cpp
 *
 * History:
 *    2013/01/24 - [Zhi He] create file
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

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "mw_internal.h"
#include "codec_misc.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "streaming_transmiter.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IFilter *gfCreateStreamingTransmiterFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CStreamingTransmiter::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CStreamingTransmiter
//
//-----------------------------------------------------------------------
IFilter *CStreamingTransmiter::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CStreamingTransmiter *result = new CStreamingTransmiter(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CStreamingTransmiter::CStreamingTransmiter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mnInputPinsNumber(0)
    , mbVod(0)
    , mpBuffer(NULL)
{
#if 0
    if ((mpMutex = gfCreateMutex()) == NULL) {
        LOGM_FATAL("CStreamingTransmiter::CStreamingTransmiter(), gfCreateMutex() fail.\n");
        return;
    }
    AUTO_LOCK(mpMutex);
#endif
    TUint i = 0;
    for (i = 0; i < EConstStreamingTransmiterMaxInputPinNumber; i ++) {
        mpInputPins[i] = NULL;
        mpDataTransmiter[i] = NULL;
        mnDataTransmiterClientNumber[i] = 0;
        mbSetExtraData[i] = 0;
        mDataTransmiterState[i] = 0;

        mpSubSessionContent[i] = NULL;
    }
    mnInputPinsNumber = 0;
    mpBuffer = NULL;
}

EECode CStreamingTransmiter::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleStreamingTransmiterFilter);

    return inherited::Construct();
}

CStreamingTransmiter::~CStreamingTransmiter()
{
    //AUTO_LOCK(mpMutex);
    TUint i = 0;

    LOGM_RESOURCE("CStreamingTransmiter::~CStreamingTransmiter(), start.\n");

    for (i = 0; i < EConstStreamingTransmiterMaxInputPinNumber; i ++) {
        LOGM_RESOURCE("CStreamingTransmiter::~CStreamingTransmiter(), before delete mpInputPins[%d], %p.\n", i, mpInputPins[i]);
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }

        LOGM_RESOURCE("CStreamingTransmiter::~CStreamingTransmiter(), before delete mpDataTransmiter[%d], %p.\n", i, mpDataTransmiter[i]);
        if (mpDataTransmiter[i]) {
            mpDataTransmiter[i]->Delete();
            mpDataTransmiter[i] = NULL;
        }

        //todo
        if (mpSubSessionContent[i] && (mbVod == 0)) {
            free(mpSubSessionContent[i]);
            mpSubSessionContent[i] = NULL;
        } else {
            mpSubSessionContent[i] = NULL;
        }
    }

    if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }
    LOGM_RESOURCE("CStreamingTransmiter::~CStreamingTransmiter(), end.\n");

#if 0
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }
#endif
}

void CStreamingTransmiter::Delete()
{
    inherited::Delete();
}

EECode CStreamingTransmiter::Initialize(TChar *p_param)
{
    return EECode_OK;
}

EECode CStreamingTransmiter::Finalize()
{
    return EECode_OK;
}

EECode CStreamingTransmiter::Run()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    mpWorkQ->SendCmd(ECMDType_StartRunning);

    return EECode_OK;
}

EECode CStreamingTransmiter::Start()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    mpWorkQ->SendCmd(ECMDType_Start);

    return EECode_OK;
}

EECode CStreamingTransmiter::Stop()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    mpWorkQ->SendCmd(ECMDType_Stop);

    return EECode_OK;
}

void CStreamingTransmiter::Pause()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    mpWorkQ->PostMsg(ECMDType_Pause);

    return;
}

void CStreamingTransmiter::Resume()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    mpWorkQ->PostMsg(ECMDType_Resume);

    return;
}

void CStreamingTransmiter::Flush()
{
    //debug assert
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    mpWorkQ->SendCmd(ECMDType_Flush);

    return;
}

void CStreamingTransmiter::ResumeFlush()
{
    LOGM_FATAL("TO DO\n");
    return;
}

EECode CStreamingTransmiter::Suspend(TUint release_context)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CStreamingTransmiter::ResumeSuspend(TComponentIndex input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CStreamingTransmiter::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CStreamingTransmiter::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CStreamingTransmiter::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    LOGM_FATAL("CStreamingTransmiter do not have output pin\n");
    return EECode_InternalLogicalBug;
}

EECode CStreamingTransmiter::AddInputPin(TUint &index, TUint type)
{
    //AUTO_LOCK(mpMutex);
    //to add data transmiter module
    if (DLikely((StreamType_Video == type) || (StreamType_Audio == type))) {
        if (mnInputPinsNumber >= EConstStreamingTransmiterMaxInputPinNumber) {
            LOGM_ERROR("Max input pin number reached.\n");
            return EECode_InternalLogicalBug;
        }

        index = mnInputPinsNumber;
        DASSERT(!mpInputPins[mnInputPinsNumber]);
        if (mpInputPins[mnInputPinsNumber]) {
            LOGM_FATAL("mpInputPins[mnInputPinsNumber] here, must have problems!!! please check it\n");
        }

        mpInputPins[mnInputPinsNumber] = CQueueInputPin::Create("[ input pin for CStreamingTransmiter]", (IFilter *) this, (StreamType) type, mpWorkQ->MsgQ());
        DASSERT(mpInputPins[mnInputPinsNumber]);

        if (StreamType_Video == type) {
            mpDataTransmiter[mnInputPinsNumber] = gfModuleFactoryStreamingTransmiter(EStreamingTransmiterModuleID_RTPRTCP_Video, mpPersistMediaConfig, mpEngineMsgSink, mnInputPinsNumber);
        } else if (StreamType_Audio == type) {
            mpDataTransmiter[mnInputPinsNumber] = gfModuleFactoryStreamingTransmiter(EStreamingTransmiterModuleID_RTPRTCP_Audio, mpPersistMediaConfig, mpEngineMsgSink, mnInputPinsNumber);
        } else {
            LOGM_ERROR("bad pin type %d.\n", type);
            return EECode_InternalLogicalBug;
        }
        DASSERT(mpDataTransmiter[mnInputPinsNumber]);
        //mpInputPins[mnInputPinsNumber]->Enable(0);
        mnInputPinsNumber ++;
        return EECode_OK;
    } else {
        LOGM_FATAL("BAD input pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_InternalLogicalBug;
}

IOutputPin *CStreamingTransmiter::GetOutputPin(TUint index, TUint sub_index)
{
    LOGM_FATAL("CStreamingTransmiter do not have output pin\n");
    return NULL;
}

IInputPin *CStreamingTransmiter::GetInputPin(TUint index)
{
    //AUTO_LOCK(mpMutex);
    if (EConstStreamingTransmiterMaxInputPinNumber > index) {
        return mpInputPins[index];
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CStreamingTransmiter::GetInputPin()\n", index, EConstStreamingTransmiterMaxInputPinNumber);
    }

    return NULL;
}

EECode CStreamingTransmiter::FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param)
{
    EECode ret = EECode_OK;

    switch (property_type) {

        case EFilterPropertyType_streaming_add_subsession: {
                if (DLikely(mbRun)) {
                    SCMD cmd;
                    cmd.code = ECMDType_AddContent;
                    cmd.pExtra = p_param;
                    cmd.needFreePExtra = 0;
                    LOGM_INFO("DEBUG: EFilterPropertyType_streaming_add_subsession begin\n");
                    ret = mpWorkQ->SendCmd(cmd);
                    LOGM_INFO("DEBUG: EFilterPropertyType_streaming_add_subsession end\n");
                } else {
                    LOGM_ERROR("filter not run?\n");
                }
            }
            break;

        case EFilterPropertyType_streaming_remove_subsession: {
                if (DLikely(mbRun)) {
                    SCMD cmd;
                    cmd.code = ECMDType_RemoveContent;
                    cmd.pExtra = p_param;
                    cmd.needFreePExtra = 0;
                    LOGM_INFO("DEBUG: EFilterPropertyType_streaming_remove_subsession begin\n");
                    ret = mpWorkQ->SendCmd(cmd);
                    LOGM_INFO("DEBUG: EFilterPropertyType_streaming_remove_subsession end\n");
                } else  {
                    LOGM_ERROR("filter not run?\n");
                }
            }
            break;

        case EFilterPropertyType_streaming_query_subsession_sink: {
                SQueryInterface *p = (SQueryInterface *) p_param;
                if (DLikely(p)) {
                    //AUTO_LOCK(mpMutex);
                    if (DLikely(p->index < mnInputPinsNumber)) {
                        p->p_pointer = (TPointer) mpDataTransmiter[p->index];
                        if (!mpSubSessionContent[p->index]) {
                            newSubSessionContent(p->index, p->type, p->format);
                        }
                        p->p_agent_pointer = (TPointer) mpSubSessionContent[p->index];
                    } else {
                        LOGM_FATAL("BAD index %d, max value %d\n", p->index, mnInputPinsNumber);
                        ret = EECode_InternalLogicalBug;
                    }
                } else {
                    LOGM_FATAL("NULL param\n");
                    ret = EECode_InternalLogicalBug;
                }
            }
            break;

        case EFilterPropertyType_streaming_set_content: {
                if (DLikely(p_param)) {
                    SClientSessionInfo *p_client = (SClientSessionInfo *)p_param;
                    SVODPipeline *p_pipeline = (SVODPipeline *)p_client->p_pipeline;
                    TComponentIndex index;
                    if (DLikely(p_client)) {
                        if (p_client->enable_video) {
                            index = p_pipeline->p_video_flow_controller_connection[0]->down_pin_index;
                            p_client->sub_session[ESubSession_video].transmiter_input_pin_index = index;
                            p_client->sub_session[ESubSession_video].p_transmiter = mpDataTransmiter[index];
                            mpSubSessionContent[index] = p_client->sub_session[ESubSession_video].p_content;

                            p_client->sub_session[ESubSession_video].server_rtp_port = p_client->p_content->sub_session_content[ESubSession_video]->server_rtp_port;
                            p_client->sub_session[ESubSession_video].server_rtcp_port = p_client->p_content->sub_session_content[ESubSession_video]->server_rtcp_port;

                            mpDataTransmiter[index]->SetSrcPort(p_client->sub_session[ESubSession_video].server_rtp_port, p_client->sub_session[ESubSession_video].server_rtcp_port);
                        }
                        if (p_client->enable_audio) {
                            index = p_pipeline->p_audio_flow_controller_connection[0]->down_pin_index;
                            p_client->sub_session[ESubSession_audio].transmiter_input_pin_index = index;
                            p_client->sub_session[ESubSession_audio].p_transmiter = mpDataTransmiter[index];
                            mpSubSessionContent[index] = p_client->sub_session[ESubSession_audio].p_content;

                            p_client->sub_session[ESubSession_audio].server_rtp_port = p_client->p_content->sub_session_content[ESubSession_audio]->server_rtp_port;
                            p_client->sub_session[ESubSession_audio].server_rtcp_port = p_client->p_content->sub_session_content[ESubSession_audio]->server_rtcp_port;

                            mpDataTransmiter[index]->SetSrcPort(p_client->sub_session[ESubSession_audio].server_rtp_port, p_client->sub_session[ESubSession_audio].server_rtcp_port);
                        }
                        mbVod = 1;
                    }
                }
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property_type);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CStreamingTransmiter::GetInfo(INFO &info)
{
    //AUTO_LOCK(mpMutex);
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = mnInputPinsNumber;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CStreamingTransmiter::PrintStatus()
{
    if (!(mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE)) {
        LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    }
    mDebugHeartBeat = 0;
}

bool CStreamingTransmiter::processCmd(SCMD &cmd)
{
    LOGM_FLOW("cmd.code %d, state %d.\n", cmd.code, msState);
    switch (cmd.code) {
        case ECMDType_ExitRunning:
            mbRun = 0;
            flushInputData();
            msState = EModuleState_Halt;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            flushInputData();
            msState = EModuleState_WaitCmd;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Start:
            if (EModuleState_WaitCmd == msState) {
                msState = EModuleState_Idle;
            }
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Pause:
            //todo
            DASSERT(!mbPaused);
            mbPaused = 1;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Resume:
            //todo
            if (msState == EModuleState_Pending) {
                msState = EModuleState_Idle;
            }
            mbPaused = 0;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Flush:
            //should not comes here
            DASSERT(0);
            msState = EModuleState_Pending;
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_AddContent: {
                SStreamingTransmiterCombo *combo = (SStreamingTransmiterCombo *)cmd.pExtra;
                if (DLikely(combo)) {
                    if (DLikely(combo->input_index < mnInputPinsNumber)) {
                        //DASSERT(combo->p_transmiter);
                        DASSERT(combo->p_session);
                        DASSERT(combo->p_sub_session);
                        //DASSERT(combo->p_transmiter == mpDataTransmiter[combo->input_index]);
                        //todo
                        if (combo->p_session->vod_mode == 1) {
                            combo->input_index = combo->p_sub_session->transmiter_input_pin_index;
                        }
                        if (DLikely(mpDataTransmiter[combo->input_index])) {
                            mpDataTransmiter[combo->input_index]->AddSubSession(combo->p_sub_session);
                            mnDataTransmiterClientNumber[combo->input_index] ++;
                        }
                    } else {
                        LOGM_ERROR("BAD combo->input_index %d\n", combo->input_index);
                    }
                }
                mpWorkQ->CmdAck(EECode_OK);
            }
            break;

        case ECMDType_RemoveContent: {
                SStreamingTransmiterCombo *combo = (SStreamingTransmiterCombo *)cmd.pExtra;
                if (DLikely(combo)) {
                    if (DLikely(combo->input_index < mnInputPinsNumber)) {
                        //DASSERT(combo->p_transmiter);
                        DASSERT(combo->p_session);
                        DASSERT(combo->p_sub_session);
                        //DASSERT(combo->p_transmiter == mpDataTransmiter[combo->input_index]);
                        if (combo->p_session->vod_mode == 1) {
                            combo->input_index = combo->p_sub_session->transmiter_input_pin_index;
                        }
                        if (DLikely(mpDataTransmiter[combo->input_index])) {
                            mpDataTransmiter[combo->input_index]->RemoveSubSession(combo->p_sub_session);
                            if (DLikely(mnDataTransmiterClientNumber[combo->input_index])) {
                                mnDataTransmiterClientNumber[combo->input_index] --;
                            } else {
                                LOGM_ERROR("why the number is zero here\n");
                            }
                        }
                    } else {
                        LOGM_ERROR("BAD combo->input_index %d\n", combo->input_index);
                    }
                }
                mpWorkQ->CmdAck(EECode_OK);
            }
            break;

        default:
            LOGM_FATAL("wrong cmd %d.\n", cmd.code);
    }
    return false;
}

EECode CStreamingTransmiter::getPinIndex(CQueueInputPin *pin, TComponentIndex &index)
{
    for (index = 0; index < EConstStreamingTransmiterMaxInputPinNumber; index ++) {
        if (pin == mpInputPins[index]) {
            return EECode_OK;
        }
    }

    index = 0;
    LOGM_FATAL("bad pin %p\n", pin);
    return EECode_InternalLogicalBug;
}

void CStreamingTransmiter::OnRun()
{
    SCMD cmd;
    CIQueue::QType type;
    CIQueue::WaitResult result;
    CQueueInputPin *pPin = NULL;
    TComponentIndex pin_index = 0;
    IStreamingTransmiter *p_transmiter;

    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    mpWorkQ->CmdAck(EECode_OK);
    LOGM_INFO("OnRun loop, start\n");

    mbRun = 1;
    msState = EModuleState_Idle;

    while (mbRun) {

        LOGM_STATE("OnRun: start switch, msState=%d(%s), GetDataCnt() %d.\n", msState, gfGetModuleStateString(msState), mpInputPins[0]->GetBufferQ()->GetDataCnt());
        //AUTO_LOCK(mpMutex);
        switch (msState) {

            case EModuleState_Idle:
                DASSERT(!mpBuffer);

                type = mpWorkQ->WaitDataMsgCircularly(&cmd, sizeof(cmd), &result);
                if (type == CIQueue::Q_MSG) {
                    processCmd(cmd);
                } else {
                    pPin = (CQueueInputPin *)result.pOwner;
                    if (!pPin->PeekBuffer(mpBuffer)) {
                        LOGM_FATAL("No buffer?\n");
                        msState = EModuleState_Error;
                        break;
                    }

                    DASSERT(mpBuffer);
                    msState = EModuleState_Running;
                    if (DUnlikely(EECode_OK != getPinIndex(pPin, pin_index))) {
                        LOGM_FATAL("getPinIndex fail\n");
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        msState = EModuleState_Error;
                    }
                }
                break;

            case EModuleState_Running:
                DASSERT(mpBuffer);

                //if (10 == pin_index) {
                //    LOGM_PRINTF("pin 10 data comes\n");
                //}

                if (DUnlikely((EBufferType_VideoExtraData != mpBuffer->GetBufferType()) && (EBufferType_VideoES != mpBuffer->GetBufferType()) && (EBufferType_AudioES != mpBuffer->GetBufferType()) && (EBufferType_AudioExtraData != mpBuffer->GetBufferType()))) {
                    LOGM_WARN("TO DO, mpBuffer->GetBufferType() %d\n", mpBuffer->GetBufferType());
                    if (mpBuffer) {
                        mpBuffer->Release();
                        mpBuffer = NULL;
                    }
                    msState = EModuleState_Idle;
                    break;
                }

                if (DLikely(pin_index < EConstStreamingTransmiterMaxInputPinNumber)) {
                    p_transmiter = mpDataTransmiter[pin_index];
                    //LOGM_DEBUG("buffer type %d, flag %08x, size %ld\n", mpBuffer->GetBufferType(), mpBuffer->GetBufferFlags(), mpBuffer->GetDataSize());
                    if (DLikely(p_transmiter)) {
                        if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::SYNC_POINT)) {
                            processSyncBuffer(pin_index);
                        }
                        if (DUnlikely((EBufferType_VideoExtraData == mpBuffer->GetBufferType()) || (EBufferType_AudioExtraData == mpBuffer->GetBufferType()))) {
                            if (StreamType_Video == mpInputPins[pin_index]->GetPinType()) {
                                gfSDPProcessVideoExtraData(mpSubSessionContent[pin_index], mpBuffer->GetDataPtr(), (TU32)mpBuffer->GetDataSize());
                            } else if (StreamType_Audio == mpInputPins[pin_index]->GetPinType()) {
                                gfSDPProcessAudioExtraData(mpSubSessionContent[pin_index], mpBuffer->GetDataPtr(), (TU32)mpBuffer->GetDataSize());
                            } else {
                                LOGM_FATAL("must not comes here\n");
                            }
                            p_transmiter->SetExtraData(mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
                        } else if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::KEY_FRAME)) {
                            if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::BROKEN_FRAME)) {
                                LOGM_WARN("%lld,broken key frame detected, %d!\n", mpBuffer->GetBufferNativePTS(), pin_index);
                            } else if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::WITH_EXTRA_DATA)) {
                                //LOGM_DEBUG("extra data + idr, do not support this from now on\n");
                                if (DLikely(StreamType_Video == mpInputPins[pin_index]->GetPinType())) {
                                    TU8 *ptr_IDR_start = gfNALUFindIDR(mpBuffer->GetDataPtr(), (TU32)mpBuffer->GetDataSize());
                                    TMemSize skip_size = gfSkipDelimter(mpBuffer->GetDataPtr());
                                    TU8 *ptr_sps_start = mpBuffer->GetDataPtr() + skip_size;
                                    if (DLikely(ptr_sps_start)) {
                                        TU8 *ptmp1 = gfNALUFindSPSHeader(ptr_sps_start, (TU32)mpBuffer->GetDataSize() - skip_size);
                                        if (DUnlikely(ptmp1 != ptr_sps_start)) {
                                            LOGM_FATAL("why comes here sps?\n");
                                            skip_size += (ptmp1 - ptr_sps_start);
                                            ptr_sps_start = ptmp1;
                                        }
                                    }
                                    TU8 *ptr_pps_end = gfNALUFindPPSEnd(ptr_sps_start, (TU32)mpBuffer->GetDataSize());
                                    DASSERT(ptr_IDR_start > ptr_sps_start);
                                    if (ptr_IDR_start) {
                                        TUint extra_data_size = ptr_pps_end - ptr_sps_start;
                                        DASSERT(extra_data_size <= mpBuffer->GetDataSize());
                                        p_transmiter->SetExtraData(ptr_sps_start, extra_data_size);
                                        //LOGM_DEBUG("extra_data_size %d, skip_size %ld\n", extra_data_size, skip_size);
                                        gfSDPProcessVideoExtraData(mpSubSessionContent[pin_index], ptr_sps_start, extra_data_size);

                                        CIBuffer div_buffer;
                                        div_buffer.SetDataPtr(ptr_sps_start);
                                        div_buffer.SetDataSize((TMemSize)extra_data_size);
                                        div_buffer.SetDataPtrBase(NULL);
                                        div_buffer.SetDataMemorySize((TMemSize)0);
                                        div_buffer.SetBufferType(EBufferType_VideoExtraData);
                                        div_buffer.SetBufferFlags(0);
                                        p_transmiter->SendData(&div_buffer);

                                        div_buffer.SetDataPtr(ptr_IDR_start);
                                        div_buffer.SetDataSize((TMemSize)(mpBuffer->GetDataSize() - skip_size - extra_data_size));
                                        div_buffer.SetDataPtrBase(NULL);
                                        div_buffer.SetDataMemorySize((TMemSize)0);
                                        div_buffer.SetBufferType(EBufferType_VideoES);
                                        div_buffer.SetBufferFlags(CIBuffer::KEY_FRAME);

                                        p_transmiter->SendData(&div_buffer);
                                    } else {
                                        LOGM_WARN("%lld,ptr_IDR_start NULL detected, %d!\n", mpBuffer->GetBufferNativePTS(), pin_index);
                                        p_transmiter->SendData(mpBuffer);
                                    }
                                } else {
                                    LOGM_WARN("why comes here\n");
                                    p_transmiter->SendData(mpBuffer);
                                }
                            } else {
                                p_transmiter->SendData(mpBuffer);
                            }
                        } else {
                            if (DUnlikely(mpBuffer->GetBufferFlags() & CIBuffer::BROKEN_FRAME)) {
                                LOGM_WARN("broken non-key frame detected, %d!\n", pin_index);
                            } else {
                                p_transmiter->SendData(mpBuffer);
                            }
                        }
                    } else {
                        LOGM_FATAL("NULL p_transmiter, pin_index %d\n", pin_index);
                    }
                } else {
                    LOGM_FATAL("BAD pin_index %d, exceed max %d, pPin %p\n", pin_index, EConstStreamingTransmiterMaxInputPinNumber, pPin);
                }

                if (mpBuffer) {
                    mpBuffer->Release();
                    mpBuffer = NULL;
                }
                msState = EModuleState_Idle;
                break;

            case EModuleState_Completed:
            case EModuleState_Pending:
            case EModuleState_Stopped:
            case EModuleState_Halt:
            case EModuleState_Error:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Bypass:
                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
                if (type == CIQueue::Q_MSG) {
                    processCmd(cmd);
                } else {
                    pPin = (CQueueInputPin *)result.pOwner;
                    if (!pPin->PeekBuffer(mpBuffer)) {
                        LOGM_FATAL("No buffer?\n");
                        msState = EModuleState_Error;
                        break;
                    }
                    mpBuffer->Release();
                    mpBuffer = NULL;
                }
                break;

            default:
                LOGM_FATAL("CStreamingTransmiter: BAD state = 0x%x.\n", msState);
                msState = EModuleState_Error;
                break;
        }

        mDebugHeartBeat ++;
    }

    LOGM_INFO("OnRun loop end.\n");
}

EECode CStreamingTransmiter::flushInputData()
{
    TUint index = 0;

    if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    LOGM_INFO("before purge input pins\n");

    for (index = 0; index < EConstStreamingTransmiterMaxInputPinNumber; index ++) {
        if (mpInputPins[index]) {
            mpInputPins[index]->Purge(1);
        }
    }

    LOGM_INFO("after purge input pins\n");

    return EECode_OK;
}

EECode CStreamingTransmiter::newSubSessionContent(TComponentIndex index, StreamType type, StreamFormat format)
{
    if (DUnlikely(index >= EConstStreamingTransmiterMaxInputPinNumber)) {
        LOGM_FATAL("BAD index %d, max value %d\n", index, EConstStreamingTransmiterMaxInputPinNumber);
        return EECode_InternalLogicalBug;
    }

    if (DLikely(!mpSubSessionContent[index])) {
        mpSubSessionContent[index] = (SStreamingSubSessionContent *)DDBG_MALLOC(sizeof(SStreamingSubSessionContent), "STSC");
        if (DLikely(mpSubSessionContent[index])) {
            memset(mpSubSessionContent[index], 0x0, sizeof(SStreamingSubSessionContent));
            mpSubSessionContent[index]->transmiter_input_pin_index = index;
            mpSubSessionContent[index]->type = type;
            mpSubSessionContent[index]->format = format;

            mpSubSessionContent[index]->enabled = 0;
            mpSubSessionContent[index]->data_comes = 0;
            mpSubSessionContent[index]->parameters_setup = 0;
            mpSubSessionContent[index]->p_transmiter = mpDataTransmiter[index];
        }
    } else {
        DASSERT(mpSubSessionContent[index]->type == type);
        DASSERT(mpSubSessionContent[index]->format == format);
    }

    return EECode_OK;
}

EECode CStreamingTransmiter::processSyncBuffer(TComponentIndex pin_index)
{
    DASSERT(mpBuffer);

    if (DUnlikely(!mpSubSessionContent[pin_index])) {
        newSubSessionContent(pin_index, mpInputPins[pin_index]->GetPinType(), StreamFormat_Invalid);
    } else {
        DASSERT(mpInputPins[pin_index]->GetPinType() == mpSubSessionContent[pin_index]->type);
    }

    if (DUnlikely(!mpSubSessionContent[pin_index])) {
        LOGM_FATAL("no memory\n");
        msState = EModuleState_Error;
        return EECode_NoMemory;
    }

    if (StreamType_Video == mpInputPins[pin_index]->GetPinType()) {
        mpSubSessionContent[pin_index]->format = mpBuffer->mContentFormat;

        mpSubSessionContent[pin_index]->video_width = mpBuffer->mVideoWidth;
        mpSubSessionContent[pin_index]->video_height = mpBuffer->mVideoHeight;
        mpSubSessionContent[pin_index]->video_bit_rate = mpBuffer->mVideoBitrate;
        mpSubSessionContent[pin_index]->video_framerate_num = mpBuffer->mVideoFrameRateNum;
        mpSubSessionContent[pin_index]->video_framerate_den = mpBuffer->mVideoFrameRateDen;
        mpSubSessionContent[pin_index]->video_framerate = mpBuffer->mVideoRoughFrameRate;

        mpSubSessionContent[pin_index]->data_comes = 1;
        mpSubSessionContent[pin_index]->parameters_setup = 1;

        LOGM_DEBUG("[data flow]: sync pointer[%d] buffer comes, video parameters: format %d, width %d, height %d, bitrate %d, framerate num %d, framerate den %d, framerate %f, sub session %p\n", pin_index, \
                   mpSubSessionContent[pin_index]->format, \
                   mpSubSessionContent[pin_index]->video_width, \
                   mpSubSessionContent[pin_index]->video_height, \
                   mpSubSessionContent[pin_index]->video_bit_rate, \
                   mpSubSessionContent[pin_index]->video_framerate_num, \
                   mpSubSessionContent[pin_index]->video_framerate_den, \
                   mpSubSessionContent[pin_index]->video_framerate, mpSubSessionContent[pin_index]);

    } else if (StreamType_Audio == mpInputPins[pin_index]->GetPinType()) {
        LOGM_DEBUG("receive sync point buffer, mpBuffer->mContentFormat %d, index %d, sub session %p\n", mpBuffer->mContentFormat, pin_index, mpSubSessionContent[pin_index]);

        mpSubSessionContent[pin_index]->format = mpBuffer->mContentFormat;

        mpSubSessionContent[pin_index]->audio_channel_number = mpBuffer->mAudioChannelNumber;
        mpSubSessionContent[pin_index]->audio_sample_rate = mpBuffer->mAudioSampleRate;
        mpSubSessionContent[pin_index]->audio_bit_rate = mpBuffer->mAudioBitrate;
        mpSubSessionContent[pin_index]->audio_sample_format = mpBuffer->mAudioSampleFormat;
        mpSubSessionContent[pin_index]->data_comes = 1;
        mpSubSessionContent[pin_index]->parameters_setup = 1;

        if (mpDataTransmiter[pin_index]) {
            mpDataTransmiter[pin_index]->UpdateStreamFormat(StreamType_Audio, (StreamFormat)mpBuffer->mContentFormat);
        }

        LOGM_DEBUG("[data flow]: sync pointer[%d] buffer comes, audio parameters: format %d, channel number %d, sample rate %d, bitrate %d, sample format %d\n", pin_index, \
                   mpSubSessionContent[pin_index]->format, \
                   mpSubSessionContent[pin_index]->audio_channel_number, \
                   mpSubSessionContent[pin_index]->audio_sample_rate, \
                   mpSubSessionContent[pin_index]->audio_bit_rate, \
                   mpSubSessionContent[pin_index]->audio_sample_format);

    } else {
        LOGM_FATAL("BAD pin type %d\n", mpInputPins[pin_index]->GetPinType());
        return EECode_BadParam;
    }

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

