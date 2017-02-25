/*******************************************************************************
 * common_base.cpp
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
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

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

const TChar *gfGetErrorCodeString(EECode code)
{
    switch (code) {
        case EECode_OK:
            return "(EECode_OK)";
            break;
        case EECode_Error:
            return "(EECode_Error)";
            break;
        case EECode_Closed:
            return "(EECode_Closed)";
            break;
        case EECode_Busy:
            return "(EECode_Busy)";
            break;
        case EECode_OSError:
            return "(EECode_OSError)";
            break;
        case EECode_IOError:
            return "(EECode_IOError)";
            break;
        case EECode_TimeOut:
            return "(EECode_TimeOut)";
            break;
        case EECode_TooMany:
            return "(EECode_TooMany)";
            break;
        case EECode_OutOfCapability:
            return "(EECode_OutOfCapability)";
            break;
        case EECode_NoMemory:
            return "(EECode_NoMemory)";
            break;
        case EECode_NoPermission:
            return "(EECode_NoPermission)";
            break;
        case EECode_NoImplement:
            return "(EECode_NoImplement)";
            break;
        case EECode_NoInterface:
            return "(EECode_NoInterface)";
            break;
        case EECode_NotExist:
            return "(EECode_NotExist)";
            break;
        case EECode_NotSupported:
            return "(EECode_NotSupported)";
            break;
        case EECode_BadState:
            return "(EECode_BadState)";
            break;
        case EECode_BadParam:
            return "(EECode_BadParam)";
            break;
        case EECode_BadCommand:
            return "(EECode_BadCommand)";
            break;
        case EECode_BadFormat:
            return "(EECode_BadFormat)";
            break;
        case EECode_BadMsg:
            return "(EECode_BadMsg)";
            break;
        case EECode_BadSessionNumber:
            return "(EECode_BadSessionNumber)";
            break;
        case EECode_TryAgain:
            return "(EECode_TryAgain)";
            break;
        case EECode_DataCorruption:
            return "(EECode_DataCorruption)";
            break;
        case EECode_DataMissing:
            return "(EECode_DataMissing)";
            break;
        case EECode_InternalMemoryBug:
            return "(EECode_InternalMemoryBug)";
            break;
        case EECode_InternalLogicalBug:
            return "(EECode_InternalLogicalBug)";
            break;
        case EECode_InternalParamsBug:
            return "(EECode_InternalParamsBug)";
            break;
        case EECode_ProtocolCorruption:
            return "(EECode_ProtocolCorruption)";
            break;
        case EECode_AbortTimeOutAPI:
            return "(EECode_AbortTimeOutAPI)";
            break;
        case EECode_AbortSessionQuitAPI:
            return "(EECode_AbortSessionQuitAPI)";
            break;
        case EECode_ParseError:
            return "(EECode_ParseError)";
            break;
        case EECode_UnknownError:
            return "(EECode_UnknownError)";
            break;
        case EECode_GLError:
            return "(EECode_GLError)";
            break;
        case EECode_NetSendHeader_Fail:
            return "(EECode_NetSendHeader_Fail)";
            break;
        case EECode_NetSendPayload_Fail:
            return "(EECode_NetSendPayload_Fail)";
            break;
        case EECode_NetReceiveHeader_Fail:
            return "(EECode_NetReceiveHeader_Fail)";
            break;
        case EECode_NetReceivePayload_Fail:
            return "(EECode_NetReceivePayload_Fail)";
            break;
        case EECode_NetConnectFail:
            return "(EECode_NetConnectFail)";
            break;
        case EECode_ServerReject_NoSuchChannel:
            return "(EECode_ServerReject_NoSuchChannel)";
            break;
        case EECode_ServerReject_ChannelIsBusy:
            return "(EECode_ServerReject_ChannelIsBusy)";
            break;
        case EECode_ServerReject_BadRequestFormat:
            return "(EECode_ServerReject_BadRequestFormat)";
            break;
        case EECode_ServerReject_CorruptedProtocol:
            return "(EECode_ServerReject_CorruptedProtocol)";
            break;
        case EECode_ServerReject_AuthenticationDataTooLong:
            return "(EECode_ServerReject_AuthenticationDataTooLong)";
            break;
        case EECode_ServerReject_NotProperPassword:
            return "(EECode_ServerReject_NotProperPassword)";
            break;
        case EECode_ServerReject_NotSupported:
            return "(EECode_ServerReject_NotSupported)";
            break;
        case EECode_ServerReject_AuthenticationFail:
            return "(EECode_ServerReject_AuthenticationFail)";
            break;
        case EECode_ServerReject_TimeOut:
            return "(EECode_ServerReject_TimeOut)";
            break;
        case EECode_ServerReject_Unknown:
            return "(EECode_ServerReject_Unknown)";
            break;
        case EECode_NetSocketSend_Error:
            return "(EECode_NetSocketSend_Error)";
            break;
        case EECode_NetSocketRecv_Error:
            return "(EECode_NetSocketRecv_Error)";
            break;

#if 0
        case EECode_AuthenticateFail_NoSuchAccount:
            return "(EECode_AuthenticateFail_NoSuchAccount)";
            break;
        case EECode_AuthenticateFail_BadAccountID:
            return "(EECode_AuthenticateFail_BadAccountID)";
            break;
        case EECode_AuthenticateFail_WrongPassword:
            return "(EECode_AuthenticateFail_WrongPassword)";
            break;
#endif

        case EECode_NotLogin:
            return "(EECode_NotLogin)";
            break;
        case EECode_AlreadyExist:
            return "(EECode_AlreadyExist)";
            break;
        case EECode_PossibleAttackFromNetwork:
            return "(EECode_PossibleAttackFromNetwork)";
            break;
        case EECode_NoRelatedComponent:
            return "(EECode_NoRelatedComponent)";
            break;

#if 0
        case EECode_ServerReject_NoUsername:
            return "(EECode_ServerReject_NoUsername)";
            break;
        case EECode_ServerReject_TooLongUsername:
            return "(EECode_ServerReject_TooLongUsername)";
            break;
        case EECode_ServerReject_NoPassword:
            return "(EECode_ServerReject_NoPassword)";
            break;
        case EECode_ServerReject_TooLongPassword:
            return "(EECode_ServerReject_TooLongPassword)";
            break;
#endif

        case EECode_OK_IsolateAccess:
            return "(EECode_OK_IsolateAccess)";
            break;
        case EECode_OK_NeedHardwareAuthenticate:
            return "(EECode_OK_NeedHardwareAuthenticate)";
            break;
        case EECode_OK_NeedSetOwner:
            return "(EECode_OK_NeedSetOwner)";
            break;

        default:
            break;
    }

    return "(Unknown error code)";
}

const TChar *gfGetModuleStateString(EModuleState state)
{
    switch (state) {
        case EModuleState_Invalid:
            return "(EModuleState_Invalid)";
            break;
        case EModuleState_Idle:
            return "(EModuleState_Idle)";
            break;
        case EModuleState_Preparing:
            return "(EModuleState_Preparing)";
            break;
        case EModuleState_Running:
            return "(EModuleState_Running)";
            break;
        case EModuleState_HasInputData:
            return "(EModuleState_HasInputData)";
            break;
        case EModuleState_HasOutputBuffer:
            return "(EModuleState_HasOutputBuffer)";
            break;
        case EModuleState_Completed:
            return "(EModuleState_Completed)";
            break;
        case EModuleState_Bypass:
            return "(EModuleState_Bypass)";
            break;
        case EModuleState_Pending:
            return "(EModuleState_Pending)";
            break;
        case EModuleState_Stopped:
            return "EModuleState_Stopped";
        case EModuleState_Error:
            return "(EModuleState_Error)";
            break;
        case EModuleState_WaitCmd:
            return "(EModuleState_WaitCmd)";
            break;
        case EModuleState_WaitTiming:
            return "(EModuleState_WaitTiming)";
            break;
        case EModuleState_Muxer_WaitExtraData:
            return "(EModuleState_Muxer_WaitExtraData)";
            break;
        case EModuleState_Muxer_SavingPartialFile:
            return "(EModuleState_Muxer_SavingPartialFile)";
            break;
        case EModuleState_Muxer_SavingPartialFilePeekRemainingOnMasterPin:
            return "(EModuleState_Muxer_SavingPartialFilePeekRemainingOnMasterPin)";
            break;
        case EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin:
            return "(EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin)";
            break;
        case EModuleState_Muxer_SavingPartialFileWaitMasterPin:
            return "(EModuleState_Muxer_SavingPartialFileWaitMasterPin)";
            break;
        case EModuleState_Muxer_SavingPartialFileWaitNonMasterPin:
            return "(EModuleState_Muxer_SavingPartialFileWaitNonMasterPin)";
            break;
        case EModuleState_Muxer_SavingWholeFile:
            return "(EModuleState_Muxer_SavingWholeFile)";
            break;
        case EModuleState_Muxer_FlushExpiredFrame:
            return "(EModuleState_Muxer_FlushExpiredFrame)";
            break;
        case EModuleState_Renderer_PreBuffering:
            return "(EModuleState_Renderer_PreBuffering)";
        case EModuleState_Renderer_PreBufferingDone:
            return "(EModuleState_Renderer_PreBufferingDone)";
        case EModuleState_Renderer_WaitVoutDormant:
            return "(EModuleState_Renderer_WaitVoutDormant)";
        case EModuleState_DirectEncoding:
            return "(EModuleState_DirectEncoding)";
        default:
            LOG_ERROR("Unknown module state %d\n", state);
            break;
    }

    return "(Unknown module state)";
}

const TChar *gfGetCMDTypeString(ECMDType type)
{
    switch (type) {
        case ECMDType_Invalid:
            return "(ECMDType_Invalid)";
            break;
        case ECMDType_Terminate:
            return "(ECMDType_Terminate)";
            break;
        case ECMDType_StartRunning:
            return "(ECMDType_StartRunning)";
            break;
        case ECMDType_ExitRunning:
            return "(ECMDType_ExitRunning)";
            break;
        case ECMDType_Stop:
            return "(ECMDType_Stop)";
            break;
        case ECMDType_Start:
            return "(ECMDType_Start)";
            break;
        case ECMDType_Pause:
            return "(ECMDType_Pause)";
            break;
        case ECMDType_Resume:
            return "(ECMDType_Resume)";
            break;
        case ECMDType_ResumeFlush:
            return "(ECMDType_ResumeFlush)";
            break;
        case ECMDType_Flush:
            return "(ECMDType_Flush)";
            break;
        case ECMDType_Suspend:
            return "(ECMDType_Suspend)";
            break;
        case ECMDType_ResumeSuspend:
            return "(ECMDType_ResumeSuspend)";
            break;
        case ECMDType_FlowControl:
            return "(ECMDType_FlowControl)";
            break;
        case ECMDType_Step:
            return "(ECMDType_Step)";
            break;
        case ECMDType_DebugDump:
            return "(ECMDType_DebugDump)";
            break;
        case ECMDType_AddContent:
            return "(ECMDType_AddContent)";
        case ECMDType_RemoveContent:
            return "(ECMDType_RemoveContent)";
            break;
        case ECMDType_ForceLowDelay:
            return "(ECMDType_ForceLowDelay)";
            break;
        case ECMDType_Speedup:
            return "(ECMDType_Speedup)";
            break;
        case ECMDType_DeleteFile:
            return "(ECMDType_DeleteFile)";
            break;
        case ECMDType_UpdatePlaybackSpeed:
            return "(ECMDType_UpdatePlaybackSpeed)";
            break;
        case ECMDType_NotifySynced:
            return "(ECMDType_NotifySynced)";
            break;
        case ECMDType_NotifySourceFilterBlocked:
            return "(ECMDType_NotifySourceFilterBlocked)";
        case ECMDType_NotifyUDECInRuningState:
            return "(ECMDType_NotifyUDECInRuningState)";
        case ECMDType_NotifyBufferRelease:
            return "(ECMDType_NotifyBufferRelease)";
        case ECMDType_MuteAudio:
            return "(ECMDType_MuteAudio)";
        case ECMDType_UnMuteAudio:
            return "(ECMDType_UnMuteAudio)";
        default:
            LOG_ERROR("Unknown CMD type %d\n", type);
            break;
    }

    return "(Unknown CMD Type)";
}

//-----------------------------------------------------------------------
//
//  CObject
//
//-----------------------------------------------------------------------

void CObject::Delete()
{
    delete this;
}

CObject::~CObject()
{
}

CObject::CObject(const TChar *name, TUint index)
    : mConfigLogLevel(ELogLevel_Notice)
    , mConfigLogOption(0)
    , mConfigLogOutput(DCAL_BITMASK(ELogOutput_Console) | DCAL_BITMASK(ELogOutput_File))
    , mIndex(index)
    , mDebugHeartBeat(0)
    , mpLogOutputFile(NULL)
{
    if (name) {
        mpModuleName = name;
    }
}

void CObject::PrintStatus()
{
    LOGM_WARN("I(%s) have not implemented PrintStatus()!It's should be blamed.\n", mpModuleName);
}

void CObject::SetLogLevel(TUint level)
{
    mConfigLogLevel = level;
}

void CObject::SetLogOption(TUint option)
{
    mConfigLogOption = option;
}

void CObject::SetLogOutput(TUint output)
{
    mConfigLogOutput = output;
}

const TChar *CObject::GetModuleName() const
{
    return mpModuleName;
}

TUint CObject::GetModuleIndex() const
{
    return mIndex;
}

//-----------------------------------------------------------------------
//
//  CIDoubleLinkedList
//
//-----------------------------------------------------------------------
CIDoubleLinkedList::CIDoubleLinkedList()
    : mNumberOfNodes(0)
    , mpFreeList(NULL)
{
    mHead.p_context = NULL;
    mHead.p_next = mHead.p_pre = &mHead;
}

CIDoubleLinkedList::~CIDoubleLinkedList()
{
    SNodePrivate *node = mHead.p_next, *tmp;
    while (node != &mHead) {
        DASSERT(node);
        if (node) {
            tmp = node;
            node = tmp->p_next;
            DDBG_FREE(tmp, "C0DL");
        } else {
            LOG_ERROR("cyclic list broken.\n");
            break;
        }
    }

    mHead.p_context = NULL;
    mHead.p_next = mHead.p_pre = &mHead;

    node = mpFreeList;
    while (node) {
        tmp = node;
        node = tmp->p_next;
        DDBG_FREE(tmp, "C0DL");
    }
    mpFreeList = NULL;
}

CIDoubleLinkedList::SNode *CIDoubleLinkedList::InsertContent(SNode *target_node, void *content, TUint after)
{
    SNodePrivate *new_node = NULL;
    SNodePrivate *tmp_node = NULL;
    DASSERT(content);
    if (DUnlikely(NULL == content)) {
        LOG_ERROR(" NULL content in CIDoubleLinkedList::InsertContent.\n");
        return NULL;
    }

    //DASSERT(false == IsContentInList(content));
    if (true == IsContentInList(content)) {
        LOG_WARN("try to insert duplicated content %p!\n", content);
        return NULL;
    }

    allocNode(new_node);
    if (DUnlikely(NULL == new_node)) {
        LOG_ERROR(" allocNode fail in CIDoubleLinkedList::InsertContent, must have error.\n");
        return NULL;
    }

    new_node->p_context = content;

    if (NULL == target_node) {
        DASSERT(mHead.p_next);
        DASSERT(mHead.p_pre);
        DASSERT(mHead.p_next->p_pre == &mHead);
        DASSERT(mHead.p_pre->p_next == &mHead);
        //head
        if (after == 0) {
            //insert before
            new_node->p_next = &mHead;
            new_node->p_pre = mHead.p_pre;

            mHead.p_pre->p_next = new_node;
            mHead.p_pre = new_node;
        } else {
            //insert after
            new_node->p_pre = &mHead;
            new_node->p_next = mHead.p_next;

            mHead.p_next->p_pre = new_node;
            mHead.p_next = new_node;
        }
    } else {
        tmp_node = (SNodePrivate *)target_node;
        DASSERT(tmp_node->p_next);
        DASSERT(tmp_node->p_pre);
        DASSERT(tmp_node->p_next->p_pre = tmp_node);
        DASSERT(tmp_node->p_pre->p_next = tmp_node);

        if (after == 0) {
            //insert before
            new_node->p_next = tmp_node;
            new_node->p_pre = tmp_node->p_pre;

            tmp_node->p_pre->p_next = new_node;
            tmp_node->p_pre = new_node;
        } else {
            //insert after
            new_node->p_pre = tmp_node;
            new_node->p_next = tmp_node->p_next;

            new_node->p_next->p_pre = new_node;
            new_node->p_next = new_node;
        }
    }
    mNumberOfNodes ++;
    return (CIDoubleLinkedList::SNode *) new_node;
}

void CIDoubleLinkedList::RemoveContent(void *content)
{
    SNodePrivate *tmp_node = NULL;
    SNodePrivate *tobe_removed;
    DASSERT(content);
    if (DUnlikely(NULL == content)) {
        LOG_ERROR(" NULL content in CIDoubleLinkedList::RemoveContent.\n");
        return;
    }

    tmp_node = mHead.p_next;
    if (DUnlikely(NULL == tmp_node || tmp_node == &mHead)) {
        LOG_WARN("BAD pointer(%p) in CIDoubleLinkedList::RemoveContent.\n", tmp_node);
        return;
    }

    while (tmp_node != &mHead) {
        DASSERT(tmp_node);
        if (NULL == tmp_node) {
            LOG_ERROR("!!!cyclic list Broken.\n");
            return;
        }

        if (tmp_node->p_context == content) {
            tobe_removed = tmp_node;
            tobe_removed->p_pre->p_next = tobe_removed->p_next;
            tobe_removed->p_next->p_pre = tobe_removed->p_pre;

            tobe_removed->p_next = mpFreeList;
            mpFreeList = tobe_removed;
            mNumberOfNodes --;
            return;
        }

        tmp_node = tmp_node->p_next;
    }
    return;
}

void CIDoubleLinkedList::FastRemoveContent(SNode *target_node, void *content)
{
    SNodePrivate *tobe_removed = (SNodePrivate *) target_node;

    if (DUnlikely(NULL == tobe_removed || tobe_removed == &mHead)) {
        LOG_ERROR("BAD pointer(%p) in CIDoubleLinkedList::FastRemoveContent.\n", tobe_removed);
        return;
    }
    DASSERT(target_node->p_context == content);

    tobe_removed->p_pre->p_next = tobe_removed->p_next;
    tobe_removed->p_next->p_pre = tobe_removed->p_pre;

    tobe_removed->p_next = mpFreeList;
    mpFreeList = tobe_removed;
    mNumberOfNodes --;

    return;
}

CIDoubleLinkedList::SNode *CIDoubleLinkedList::FirstNode() const
{
    //debug assert
    DASSERT(mHead.p_next);
    DASSERT(mHead.p_pre);
    DASSERT(mHead.p_next->p_pre == &mHead);
    DASSERT(mHead.p_pre->p_next == &mHead);

    if (mHead.p_next == &mHead) {
        //no node in list
        return NULL;
    }

    return (CIDoubleLinkedList::SNode *)mHead.p_next;
}

CIDoubleLinkedList::SNode *CIDoubleLinkedList::LastNode() const
{
    //debug assert
    DASSERT(mHead.p_next);
    DASSERT(mHead.p_pre);
    DASSERT(mHead.p_next->p_pre == &mHead);
    DASSERT(mHead.p_pre->p_next == &mHead);

    if (mHead.p_pre == &mHead) {
        //no node in list
        return NULL;
    }

    return (CIDoubleLinkedList::SNode *)mHead.p_pre;
}

CIDoubleLinkedList::SNode *CIDoubleLinkedList::NextNode(SNode *node) const
{
    SNodePrivate *tmp_node = (SNodePrivate *)node;
    if (DUnlikely(NULL == tmp_node || tmp_node == &mHead)) {
        LOG_ERROR("BAD pointer(%p) in CIDoubleLinkedList::NextNode.\n", node);
        return NULL;
    }

    DASSERT(tmp_node->p_next);
    DASSERT(tmp_node->p_pre);
    DASSERT(tmp_node->p_next->p_pre == tmp_node);
    DASSERT(tmp_node->p_pre->p_next == tmp_node);

    DASSERT(true == IsNodeInList(node));

    if (tmp_node->p_next == &mHead) {
        //last node
        return NULL;
    }

    return (CIDoubleLinkedList::SNode *)tmp_node->p_next;
}

CIDoubleLinkedList::SNode *CIDoubleLinkedList::PreNode(SNode *node) const
{
    SNodePrivate *tmp_node = (SNodePrivate *)node;
    if (DUnlikely(NULL == tmp_node || tmp_node == &mHead)) {
        LOG_ERROR("BAD pointer(%p) in CIDoubleLinkedList::PreNode.\n", tmp_node);
        return NULL;
    }

    DASSERT(tmp_node->p_next);
    DASSERT(tmp_node->p_pre);
    DASSERT(tmp_node->p_next->p_pre == tmp_node);
    DASSERT(tmp_node->p_pre->p_next == tmp_node);

    DASSERT(true == IsNodeInList(node));

    if (tmp_node->p_pre == &mHead) {
        //first node
        return NULL;
    }

    return (CIDoubleLinkedList::SNode *)tmp_node->p_pre;
}

TUint CIDoubleLinkedList::NumberOfNodes() const
{
    return mNumberOfNodes;
}

void CIDoubleLinkedList::RemoveAllNodes()
{
    if (mNumberOfNodes == 0)
    { return; }

    SNodePrivate *node = mHead.p_next, *tmp;
    while (node != &mHead) {
        if (node) {
            tmp = node;
            node = tmp->p_next;

            tmp->p_next = mpFreeList;
            mpFreeList = tmp;
        } else {
            LOG_ERROR("cyclic list broken.\n");
            break;
        }
    }
    mHead.p_next = &mHead;
    mHead.p_pre = &mHead;
    mHead.p_context = NULL;
    mNumberOfNodes = 0;
    return;
}

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE
void *CIDoubleLinkedList::operator new(size_t, void *p)
{
    CIDoubleLinkedList *thiz = (CIDoubleLinkedList *)p;

    thiz->mNumberOfNodes = 0;
    thiz->mpFreeList = NULL;

    thiz->mHead.p_context = NULL;
    thiz->mHead.p_next = thiz->mHead.p_pre = &thiz->mHead;

    return thiz;
}

void *CIDoubleLinkedList::operator new(size_t)
{
    CIDoubleLinkedList *thiz = (CIDoubleLinkedList *) malloc(sizeof(CIDoubleLinkedList));
    memset(thiz, 0x0, sizeof(CIDoubleLinkedList));

    thiz->mNumberOfNodes = 0;
    thiz->mpFreeList = NULL;

    thiz->mHead.p_context = NULL;
    thiz->mHead.p_next = thiz->mHead.p_pre = &thiz->mHead;

    return thiz;
}
#endif

bool CIDoubleLinkedList::IsContentInList(void *content) const
{
    SNodePrivate *tmp_node = mHead.p_next;
    if (DUnlikely(NULL == tmp_node)) {
        LOG_ERROR("NULL header in CIDoubleLinkedList::IsContentInList.\n");
        return false;
    }

    while (tmp_node != &mHead) {
        DASSERT(tmp_node);
        if (NULL == tmp_node) {
            LOG_ERROR("!!!cyclic list Broken.\n");
            return false;
        }

        if (tmp_node->p_context == content) {
            return true;
        }
        tmp_node = tmp_node->p_next;
    }
    return false;
}

bool CIDoubleLinkedList::IsNodeInList(SNode *node) const
{
    SNodePrivate *tmp_node = mHead.p_next;
    if (DUnlikely(NULL == tmp_node || tmp_node == &mHead)) {
        LOG_ERROR("BAD pointer(%p) in CIDoubleLinkedList::IsNodeInList.\n", node);
        return false;
    }

    while (tmp_node != &mHead) {
        DASSERT(tmp_node);
        if (NULL == tmp_node) {
            LOG_ERROR("!!!cyclic list Broken.\n");
            return false;
        }

        if ((SNode *)tmp_node == node) {
            return true;
        }

        tmp_node = tmp_node->p_next;
    }
    return false;
}

void CIDoubleLinkedList::allocNode(SNodePrivate *&pnode)
{
    if (mpFreeList) {
        pnode = mpFreeList;
        mpFreeList = mpFreeList->p_next;
    } else {
        pnode = (SNodePrivate *) DDBG_MALLOC(sizeof(SNodePrivate), "C0DL");
    }
}

//-----------------------------------------------------------------------
//
//  CIWorkQueue
//
//-----------------------------------------------------------------------
CIWorkQueue::CIWorkQueue(AO *pAO)
    : inherited("CIWorkQueue")
    , mpAO(pAO)
    , mpMsgQ(NULL)
    , mpThread(NULL)
    , mpDebugModuleName(NULL)
{
}

// return receive's reply
EECode CIWorkQueue::SendCmd(TUint cid, void *pExtra)
{
    SCMD cmd(cid);
    cmd.pExtra = pExtra;
    return mpMsgQ->SendMsg(&cmd, sizeof(cmd));
}

EECode CIWorkQueue::SendCmd(SCMD &cmd)
{
    return mpMsgQ->SendMsg(&cmd, sizeof(cmd));
}

void CIWorkQueue::GetCmd(SCMD &cmd)
{
    mpMsgQ->GetMsg(&cmd, sizeof(cmd));
}

bool CIWorkQueue::PeekCmd(SCMD &cmd)
{
    return mpMsgQ->PeekMsg(&cmd, sizeof(cmd));
}

EECode CIWorkQueue::PostMsg(TUint cid, void *pExtra)
{
    SCMD cmd(cid);
    cmd.pExtra = pExtra;
    return mpMsgQ->PostMsg(&cmd, sizeof(cmd));
}

EECode CIWorkQueue::PostMsg(SCMD &cmd)
{
    return mpMsgQ->PostMsg(&cmd, sizeof(cmd));
}

EECode CIWorkQueue::Run()
{
    return SendCmd(ECMDType_StartRunning);
}

EECode CIWorkQueue::Exit()
{
    return SendCmd(ECMDType_ExitRunning);
}

EECode CIWorkQueue::Stop()
{
    return SendCmd(ECMDType_Stop);
}

EECode CIWorkQueue::Start()
{
    return SendCmd(ECMDType_Start);
}

void CIWorkQueue::CmdAck(EECode result)
{
    mpMsgQ->Reply(result);
}

CIQueue *CIWorkQueue::MsgQ() const
{
    return mpMsgQ;
}

CIQueue::QType CIWorkQueue::WaitDataMsg(void *pMsg, TUint msgSize, CIQueue::WaitResult *pResult)
{
    return mpMsgQ->WaitDataMsg(pMsg, msgSize, pResult);
}

CIQueue::QType CIWorkQueue::WaitDataMsgCircularly(void *pMsg, TUint msgSize, CIQueue::WaitResult *pResult)
{
    return mpMsgQ->WaitDataMsgCircularly(pMsg, msgSize, pResult);
}

CIQueue::QType CIWorkQueue::WaitDataMsgWithSpecifiedQueue(void *pMsg, TUint msgSize, const CIQueue *pQueue)
{
    return mpMsgQ->WaitDataMsgWithSpecifiedQueue(pMsg, msgSize, pQueue);
}

void CIWorkQueue::WaitMsg(void *pMsg, TUint msgSize)
{
    mpMsgQ->WaitMsg(pMsg, msgSize);
}

CIWorkQueue *CIWorkQueue::Create(AO *pAO)
{
    CIWorkQueue *result = new CIWorkQueue(pAO);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CIWorkQueue::Delete()
{
    if (mpThread) {
        Terminate();
        if (mpThread) {
            mpThread->Delete();
            mpThread = NULL;
        }
    }
    if (mpMsgQ) {
        mpMsgQ->Delete();
        mpMsgQ = NULL;
    }

    inherited::Delete();
}

EECode CIWorkQueue::Construct()
{
    if ((mpMsgQ = CIQueue::Create(NULL, this, sizeof(SCMD), 16)) == NULL) {
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    if ((mpThread = gfCreateThread(mpAO->GetObject()->GetModuleName(), ThreadEntry, this)) == NULL) {
        LOG_FATAL("No memory\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CIWorkQueue::~CIWorkQueue()
{

}

EECode CIWorkQueue::ThreadEntry(void *p)
{
    //LOG_DEBUG("CIWorkQueue::ThreadEntry %p->MainLoop() begin\n", p);
    ((CIWorkQueue *)p)->MainLoop();
    //LOG_DEBUG("CIWorkQueue::ThreadEntry %p->MainLoop() end\n", p);
    return EECode_OK;
}

void CIWorkQueue::Terminate()
{
    LOGM_INFO("[%s]: Terminate: SendCmd(ECMDType_Terminate)\n", mpAO->GetObject()->GetModuleName());
    EECode err = SendCmd((TUint)ECMDType_Terminate);
    DASSERT_OK(err);
}

void CIWorkQueue::MainLoop()
{
    SCMD cmd;
    mpDebugModuleName = mpAO->GetObject()->GetModuleName();

    while (1) {
        GetCmd(cmd);
        //LOGM_NOTICE("cmd.code: %d, (this %p, name:%s, index %d)\n", cmd.code, this, mpAO->GetModuleName(), mpAO->GetModuleIndex());
        switch (cmd.code) {
            case ECMDType_Terminate:
                LOGM_INFO("[%s] recieve ECMDType_Terminate, exit\n", mpAO->GetObject()->GetModuleName());
                CmdAck(EECode_OK);
                return;

            case ECMDType_ExitRunning:
                LOGM_INFO("[%s] receieve ECMDType_ExitRunning, should not comes here\n", mpAO->GetObject()->GetModuleName());
                CmdAck(EECode_OK);
                break;

            case ECMDType_StartRunning:
                LOGM_INFO("[%s] receieve ECMDType_StartRunning\n", mpAO->GetObject()->GetModuleName());
                mpAO->OnRun();
                LOGM_INFO("[%s] OnRun exit!\n", mpAO->GetObject()->GetModuleName());
                break;

            default:
                LOGM_INFO("[%s]: not expect cmd 0x%08x here\n", mpAO->GetObject()->GetModuleName(), cmd.code);
                break;
        }
    }

    //LOGM_NOTICE("[%s], exit done\n", mpAO->GetModuleName());
}

CIClockReference::CIClockReference()
    : inherited("CIClockReference", 0)
    , mpMutex(NULL)
    , mBeginSourceTime(0)
    , mCurrentSourceTime(0)
    , mBeginTime(0)
    , mCurrentTime(0)
    , mbPaused(0)
    , mbStarted(0)
    , mbSilent(0)
    , mbForward(1)
    , mSpeed(1)
    , mSpeedFrac(0)
    , mSpeedCombo(1 << 8)
    , mpFreeClockListenerList(NULL)
{
    mpClockListenerList = NULL;
}

CIClockReference::~CIClockReference()
{
}

EECode CIClockReference::Construct()
{
    mpMutex = gfCreateMutex();
    if (!mpMutex) {
        LOG_FATAL("CIClockReference::Construct(): gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    mpClockListenerList = new CIDoubleLinkedList();
    if (!mpClockListenerList) {
        LOG_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CIClockReference *CIClockReference::Create()
{
    CIClockReference *result = new CIClockReference;
    if (result && (EECode_OK != result->Construct())) {
        delete result;
        result = NULL;
    }
    DASSERT(result);
    return result;
}

void CIClockReference::Delete()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    clearAllClockListener();
    if (mpClockListenerList) {
        delete mpClockListenerList;
        mpClockListenerList = NULL;
    }
}

void CIClockReference::SetBeginTime(TTime start_time)
{
    AUTO_LOCK(mpMutex);

    mBeginTime = start_time;
}

void CIClockReference::SetDirection(TU8 forward)
{
    AUTO_LOCK(mpMutex);
    mbForward = forward;
}

void CIClockReference::SetSpeed(TU8 speed, TU8 speed_frac)
{
    AUTO_LOCK(mpMutex);
    mSpeed = speed;
    mSpeedFrac = speed_frac;

    mSpeedCombo = ((TU16) mSpeed << 8) + mSpeedFrac;
}

EECode CIClockReference::Start()
{
    AUTO_LOCK(mpMutex);
    DASSERT(0 == mbStarted);
    mbStarted = 1;

    return EECode_OK;
}

EECode CIClockReference::Pause()
{
    AUTO_LOCK(mpMutex);
    DASSERT(0 == mbPaused);
    mbPaused = 1;

    return EECode_OK;
}

EECode CIClockReference::Resume()
{
    AUTO_LOCK(mpMutex);
    DASSERT(1 == mbPaused);
    mbPaused = 0;

    return EECode_OK;
}

EECode CIClockReference::Stop()
{
    AUTO_LOCK(mpMutex);
    DASSERT(1 == mbStarted);
    mbStarted = 0;

    return EECode_OK;
}

EECode CIClockReference::Reset(TTime source_time, TTime target_time, TU8 sync_source_time, TU8 sync_target_time, TU8 speed, TU8 speed_frac, TU8 forward)
{
    AUTO_LOCK(mpMutex);
    if (sync_source_time) {
        mBeginSourceTime = mCurrentSourceTime = source_time;
    }

    if (sync_target_time) {
        mBeginTime = mCurrentTime = target_time;
    }

    mbForward = forward;
    mSpeed = speed;
    mSpeedFrac = speed_frac;
    mSpeedCombo = ((TU16)speed << 8) + mSpeedFrac;

    return EECode_OK;
}

void CIClockReference::updateTime(TTime target_time)
{
    mCurrentSourceTime = target_time;
    if (DLikely((1 == mSpeed) && (0 == mSpeedFrac))) {
        mCurrentTime = mBeginTime + (mCurrentSourceTime - mBeginSourceTime);
    } else {
        mCurrentTime = mBeginTime + (((mCurrentSourceTime - mBeginSourceTime) * mSpeedCombo) >> 8);
    }
}

TTime CIClockReference::GetCurrentTime() const
{
    //AUTO_LOCK(mpMutex);
    return mCurrentTime;
}

void CIClockReference::Heartbeat(TTime target_time)
{
    AUTO_LOCK(mpMutex);
    if (DUnlikely(!mpClockListenerList)) {
        return;
    }
    CIDoubleLinkedList::SNode *pnode = NULL;
    SClockListener *p_clock_listener = NULL;

    updateTime(target_time);
    mDebugHeartBeat ++;

    pnode = mpClockListenerList->FirstNode();
    while (pnode) {
        p_clock_listener = (SClockListener *)pnode->p_context;
        DASSERT(p_clock_listener);
        if (NULL == p_clock_listener) {
            LOG_FATAL("Fatal error(NULL == p_clock_listener), must not get here.\n");
            continue;
        }
        pnode = mpClockListenerList->NextNode(pnode);

        if (DUnlikely(((mbForward) && (mCurrentTime >= p_clock_listener->event_time)) || ((!mbForward) && (mCurrentTime <= p_clock_listener->event_time)))) {
            DASSERT(p_clock_listener->p_listener);
            //__UNLOCK(mpMutex);
            p_clock_listener->p_listener->EventNotify(EEventType_Timer, (TU64)p_clock_listener->event_time, p_clock_listener->user_context);
            //__LOCK(mpMutex);

            if (EClockTimerType_once == p_clock_listener->type) {
                mpClockListenerList->RemoveContent((void *)p_clock_listener);
                releaseClockListener(p_clock_listener);
            } else if (EClockTimerType_repeat_count == p_clock_listener->type) {
                if (p_clock_listener->event_remaining_count) {
                    p_clock_listener->event_remaining_count --;
                    p_clock_listener->event_time += p_clock_listener->event_duration;
                } else {
                    mpClockListenerList->RemoveContent((void *)p_clock_listener);
                    releaseClockListener(p_clock_listener);
                }
            } else {
                DASSERT(EClockTimerType_repeat_infinite == p_clock_listener->type);
                LOGM_DEBUG("current time %lld, next time %lld\n", p_clock_listener->event_time, p_clock_listener->event_time + p_clock_listener->event_duration);
                p_clock_listener->event_time += p_clock_listener->event_duration;
            }
        }

    }
}

void CIClockReference::ClearAllTimers()
{
    AUTO_LOCK(mpMutex);

    CIDoubleLinkedList::SNode *pnode = NULL, *tobe_removed = NULL;
    SClockListener *p_clock_listener = NULL;

    pnode = mpClockListenerList->FirstNode();
    while (pnode) {
        tobe_removed = pnode;
        pnode = mpClockListenerList->NextNode(pnode);
        p_clock_listener = (SClockListener *)tobe_removed->p_context;

        DASSERT(p_clock_listener);
        if (NULL == p_clock_listener) {
            LOG_FATAL("Fatal error(NULL == p_clock_listener), must not get here.\n");
            continue;
        }
        mpClockListenerList->FastRemoveContent(tobe_removed, (void *)p_clock_listener);
        releaseClockListener(p_clock_listener);
    }
}

SClockListener *CIClockReference::AddTimer(IEventListener *p_listener, EClockTimerType type, TTime time, TTime interval, TUint count)
{
    AUTO_LOCK(mpMutex);

    SClockListener *p_clock_listener = NULL;

    if (!p_listener) {
        LOG_ERROR("NULL p_listener in CIClockReference::AddTimer\n");
        return NULL;
    }

    p_clock_listener = allocClockListener();
    DASSERT(p_clock_listener);

    p_clock_listener->p_listener = p_listener;
    p_clock_listener->type = type;
    p_clock_listener->event_time = time;
    p_clock_listener->event_duration = interval;
    p_clock_listener->event_remaining_count = count;

    mpClockListenerList->InsertContent(NULL, (void *)p_clock_listener);

    return p_clock_listener;
}

EECode CIClockReference::RemoveTimer(SClockListener *listener)
{
    AUTO_LOCK(mpMutex);

    if (!listener) {
        LOG_ERROR("NULL p_listener in CIClockReference::AddTimer\n");
        return EECode_BadParam;
    }

    if (!mpClockListenerList->IsContentInList((void *)listener)) {
        LOG_ERROR("listener not in list?\n");
        return EECode_InternalLogicalBug;
    }

    mpClockListenerList->RemoveContent((void *)listener);
    releaseClockListener(listener);

    return EECode_OK;
}

EECode CIClockReference::GuardedRemoveTimer(SClockListener *listener)
{
    if (!listener) {
        LOG_ERROR("NULL p_listener in CIClockReference::AddTimer\n");
        return EECode_BadParam;
    }

    if (!mpClockListenerList->IsContentInList((void *)listener)) {
        LOG_ERROR("listener not in list?\n");
        return EECode_InternalLogicalBug;
    }

    mpClockListenerList->RemoveContent((void *)listener);
    releaseClockListener(listener);

    return EECode_OK;
}


EECode CIClockReference::RemoveAllTimers(IEventListener *p_listener)
{
    AUTO_LOCK(mpMutex);

    CIDoubleLinkedList::SNode *pnode = NULL;
    SClockListener *p_clock_listener = NULL;

    pnode = mpClockListenerList->FirstNode();
    while (pnode) {
        p_clock_listener = (SClockListener *)pnode->p_context;
        DASSERT(p_clock_listener);
        if (NULL == p_clock_listener) {
            LOG_FATAL("Fatal error(NULL == p_clock_listener), must not get here.\n");
            continue;
        }
        pnode = mpClockListenerList->NextNode(pnode);
        if (p_listener == p_clock_listener->p_listener) {
            mpClockListenerList->RemoveContent((void *)p_clock_listener);
            releaseClockListener(p_clock_listener);
        }
    }

    return EECode_OK;
}

SClockListener *CIClockReference::allocClockListener()
{
    SClockListener *p;

    if (!mpFreeClockListenerList) {
        p = (SClockListener *) DDBG_MALLOC(sizeof(SClockListener), "C0CL");
        if (!p) {
            LOG_FATAL("NO memory\n");
            return NULL;
        }
        p->p_next = NULL;
        return p;
    }

    p = mpFreeClockListenerList;
    mpFreeClockListenerList = mpFreeClockListenerList->p_next;
    p->p_next = NULL;
    return p;
}

void CIClockReference::releaseClockListener(SClockListener *p)
{
    //    AUTO_LOCK(mpMutex);
    if (p) {
        p->p_next = mpFreeClockListenerList;
        mpFreeClockListenerList = p;
    } else {
        LOG_ERROR("NULL input in CIClockReference::releaseClockListener.\n");
    }
}

void CIClockReference::clearAllClockListener()
{
    //    AUTO_LOCK(mpMutex);
    SClockListener *p = mpFreeClockListenerList;
    while (p) {
        mpFreeClockListenerList = p->p_next;
        DDBG_FREE(p, "C0CL");
        p = mpFreeClockListenerList;
    }
}

void CIClockReference::PrintStatus()
{
    AUTO_LOCK(mpMutex);
    LOGM_PRINTF("mDebugHeartBeat %d, mClockListenerList.NumberOfNodes() %d\n", mDebugHeartBeat, mpClockListenerList->NumberOfNodes());

    CIDoubleLinkedList::SNode *pnode = NULL;
    SClockListener *p_clock_listener = NULL;
    TUint i = 0;

    pnode = mpClockListenerList->FirstNode();
    while (pnode) {
        p_clock_listener = (SClockListener *)pnode->p_context;
        DASSERT(p_clock_listener);
        if (NULL == p_clock_listener) {
            LOG_FATAL("Fatal error(NULL == p_clock_listener), must not get here.\n");
            continue;
        }
        pnode = mpClockListenerList->NextNode(pnode);

        LOGM_PRINTF("i %d, mCurrentTime %lld, mbForward %d\n", i, mCurrentTime, mbForward);
        LOGM_PRINTF("p_clock_listener: type %d, event_time %lld, event_duration %lld.\n", p_clock_listener->type, p_clock_listener->event_time, p_clock_listener->event_duration);
        i ++;
    }

    mDebugHeartBeat = 0;

}

//-----------------------------------------------------------------------
//
// CIClockManager
//
//-----------------------------------------------------------------------
CIClockManager *CIClockManager::Create()
{
    CIClockManager *result = new CIClockManager();
    if (result && (EECode_OK != result->Construct())) {
        delete result;
        result = NULL;
    }
    return result;
}

void CIClockManager::Delete()
{
    if (mpWorkQ) {
        mpWorkQ->Delete();
        mpWorkQ = NULL;
    }

    if (mpEvent) {
        mpEvent->Delete();
        mpEvent = NULL;
    }

    if (mpClockReferenceList) {
        delete mpClockReferenceList;
        mpClockReferenceList = NULL;
    }
}

CObject *CIClockManager::GetObject() const
{
    return (CObject *) this;
}

CIClockManager::CIClockManager()
    : inherited("CIClockManager")
    , mpWorkQ(NULL)
    , mpClockSource(NULL)
    , mpEvent(NULL)
    , mbRunning(0)
    , mbHaveClockSource(0)
    , mbPaused(0)
    , mbClockReferenceListDestroyed(0)
{
    mpClockReferenceList = NULL;
}

EECode CIClockManager::Construct()
{
    mpClockReferenceList = new CIDoubleLinkedList();
    if (!mpClockReferenceList) {
        LOG_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    if (NULL == (mpWorkQ = CIWorkQueue::Create((IActiveObject *)this))) {
        LOGM_FATAL("CIClockManager::Construct(), CIWorkQueue::Create fail\n");
        return EECode_NoMemory;
    }

    if (NULL == (mpEvent = gfCreateEvent())) {
        LOGM_FATAL("CIClockManager::Construct(), gfCreateEvent() fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CIClockManager::~CIClockManager()
{


}

EECode CIClockManager::RegisterClockReference(CIClockReference *p_clock_reference)
{
    EECode ret;

    DASSERT(p_clock_reference);
    if (!p_clock_reference) {
        LOG_ERROR("NULL p_clock_reference\n");
        return EECode_BadParam;
    }

    ret = mpWorkQ->SendCmd(ECMDType_AddContent, (void *)p_clock_reference);

    return ret;
}

EECode CIClockManager::UnRegisterClockReference(CIClockReference *p_clock_reference)
{
    EECode ret = EECode_OK;

    DASSERT(p_clock_reference);
    if (!p_clock_reference) {
        LOG_ERROR("NULL p_clock_reference\n");
        return EECode_BadParam;
    }

    if (mbRunning) {
        ret = mpWorkQ->SendCmd(ECMDType_RemoveContent, (void *)p_clock_reference);
    }

    return ret;
}

IClockSource *CIClockManager::GetClockSource() const
{
    return mpClockSource;
}

EECode CIClockManager::SetClockSource(IClockSource *pClockSource)
{
    DASSERT(!mpClockSource);
    DASSERT(pClockSource);

    mpClockSource = pClockSource;
    return EECode_OK;
}

EECode CIClockManager::Start()
{
    DASSERT(mpWorkQ);
    if (mpWorkQ) {
        if (!mbRunning) {
            mbRunning = 1;
            mpWorkQ->Run();
        } else {
            LOGM_WARN("already running\n");
        }
    } else {
        LOGM_FATAL("running without mpWorkQ?\n");
        return EECode_BadState;
    }

    DASSERT(mpClockSource);
    if (mpClockSource) {
        mpClockSource->SetClockState(EClockSourceState_running);
    }

    return EECode_OK;
}

EECode CIClockManager::Stop()
{
    DASSERT(mpWorkQ);
    if (mpWorkQ) {
        if (mbRunning) {
            mbRunning = 0;
            mpWorkQ->Exit();
        } else {
            LOGM_WARN("not in running state\n");
        }
    } else {
        LOGM_FATAL("running without mpWorkQ?\n");
        return EECode_BadState;
    }
    return EECode_OK;
}

void CIClockManager::PrintStatus()
{
    LOGM_ALWAYS("mDebugHeartBeat %d, mbRunning %d, mbPaused %d\n", mDebugHeartBeat, mbRunning, mbPaused);
    mDebugHeartBeat = 0;
}

EECode CIClockManager::processCMD(SCMD &cmd)
{
    EECode err = EECode_OK;

    switch (cmd.code) {

        case ECMDType_Terminate:
            LOG_ERROR("ECMDType_Terminate, should not comes here, in OnCmd\n");
            mbRunning = 0;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_StartRunning:
            LOG_ERROR("ECMDType_StartRunning, should not comes here, in OnCmd\n");
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_ExitRunning:
            mbRunning = 0;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            mbRunning = 0;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Pause:
            mbPaused = 1;
            break;
        case ECMDType_Resume:
            mbPaused = 0;
            break;

        case ECMDType_AddContent: {
#ifdef DCONFIG_ENABLE_DEBUG_CHECK
                bool is_inlist = false;
                is_inlist = mpClockReferenceList->IsContentInList(cmd.pExtra);
                DASSERT(false == is_inlist);

                if (true == is_inlist) {
                    LOGM_ERROR("p_clock_reference %p already in list, please check code.\n", cmd.pExtra);
                    mpWorkQ->CmdAck(EECode_InternalLogicalBug);
                    break;
                }
#endif
                mpClockReferenceList->InsertContent(NULL, cmd.pExtra, 1);
            }
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_RemoveContent: {
#ifdef DCONFIG_ENABLE_DEBUG_CHECK
                bool is_inlist = false;
                is_inlist = mpClockReferenceList->IsContentInList(cmd.pExtra);
                DASSERT(true == is_inlist);

                if (false == is_inlist) {
                    LOGM_ERROR("p_clock_reference %p not in list, please check code.\n", cmd.pExtra);
                    mpWorkQ->CmdAck(EECode_InternalLogicalBug);
                    break;
                }
#endif
                mpClockReferenceList->RemoveContent(cmd.pExtra);
            }
            mpWorkQ->CmdAck(EECode_OK);
            break;

        default:
            LOG_ERROR("unknown Cmd comes here, code %d\n", cmd.code);
            break;
    }

    return err;
}

void CIClockManager::updateClockReferences()
{
    CIDoubleLinkedList::SNode *pnode;
    CIClockReference *p_clock_reference = NULL;

    DASSERT(mpClockSource);
    mpClockSource->UpdateTime();

    TTime source_time = mpClockSource->GetClockTime();

    pnode = mpClockReferenceList->FirstNode();
    while (pnode) {
        p_clock_reference = (CIClockReference *)pnode->p_context;
        DASSERT(p_clock_reference);
        if (NULL == p_clock_reference) {
            LOG_FATAL("Fatal error(NULL == p_clock_reference), must not get here.\n");
            break;
        }

        p_clock_reference->Heartbeat(source_time);
        pnode = mpClockReferenceList->NextNode(pnode);
    }
}

void CIClockManager::OnRun()
{
    SCMD cmd;

    mpWorkQ->CmdAck(EECode_OK);
    while (mbRunning) {
        mDebugHeartBeat ++;
        if (mbPaused) {
            mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
            processCMD(cmd);
            continue;
        }

        if (mpWorkQ->PeekCmd(cmd)) {
            processCMD(cmd);
            continue;
        }

        if (EECode_OK == mpEvent->Wait(10)) {
            LOG_NOTICE("CIClockManager::OnRun, signal comes, exit\n");
            break;
        }

        updateClockReferences();
    }
}

//-----------------------------------------------------------------------
//
// CRingMemPool
//
//-----------------------------------------------------------------------
IMemPool *CRingMemPool::Create(TMemSize size)
{
    CRingMemPool *thiz = new CRingMemPool();
    if (thiz && (EECode_OK == thiz->Construct(size))) {
        return thiz;
    } else {
        if (thiz) {
            LOG_FATAL("CRingMemPool->Construct(size = %ld) fail\n", size);
            delete thiz;
        } else {
            LOG_FATAL("new CRingMemPool() fail\n");
        }
    }

    return NULL;
}

CObject *CRingMemPool::GetObject0() const
{
    return (CObject *) this;
}

void CRingMemPool::PrintStatus()
{
    LOGM_PRINTF("mem base %p, end %p, size %ld\n", mpMemoryBase, mpMemoryEnd, mMemoryTotalSize);
    LOGM_PRINTF("mRequestWrapCount %d, mReleaseWrapCount %d, mnWaiters %d\n", mRequestWrapCount, mReleaseWrapCount, mnWaiters);
    LOGM_PRINTF("mpFreeMemStart %p, mpUsedMemStart %p\n", mpFreeMemStart, mpUsedMemStart);
}

void CRingMemPool::Delete()
{
    if (mpMemoryBase) {
        DDBG_FREE(mpMemoryBase, "C0RM");
        mpMemoryBase = NULL;
    }

    mMemoryTotalSize = 0;

    mpFreeMemStart = NULL;
    mpUsedMemStart = NULL;

    inherited::Delete();
}

CRingMemPool::CRingMemPool()
    : inherited("CRingMemPool")
    , mpMutex(NULL)
    , mpCond(NULL)
    , mRequestWrapCount(0)
    , mReleaseWrapCount(1)
    , mpMemoryBase(NULL)
    , mpMemoryEnd(NULL)
    , mMemoryTotalSize(0)
    , mpFreeMemStart(NULL)
    , mpUsedMemStart(NULL)
    , mnWaiters(0)
{

}

EECode CRingMemPool::Construct(TMemSize size)
{
    DASSERT(!mpMemoryBase);

    mpMemoryBase = (TU8 *) DDBG_MALLOC(size, "C0RM");
    if (mpMemoryBase) {
        mMemoryTotalSize = size;
        mpFreeMemStart = mpMemoryBase;
        mpMemoryEnd = mpMemoryBase + size;
        mpUsedMemStart = mpMemoryBase;
    } else {
        LOG_FATAL("alloc mem(size = %lu) fail\n", size);
        return EECode_NoMemory;
    }

    DASSERT(!mpMutex);
    mpMutex = gfCreateMutex();
    if (!mpMutex) {
        LOG_FATAL("gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    DASSERT(!mpCond);
    mpCond = gfCreateCondition();
    if (!mpCond) {
        LOG_FATAL("gfCreateCondition() fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CRingMemPool::~CRingMemPool()
{
    if (mpMemoryBase) {
        DDBG_FREE(mpMemoryBase, "C0RM");
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpCond) {
        mpCond->Delete();
        mpCond = NULL;
    }
}

TU8 *CRingMemPool::RequestMemBlock(TMemSize size, TU8 *start_pointer)
{
    TU8 *p_ret = NULL;
    TMemSize currentFreeSize1 = 0;
    TMemSize currentFreeSize2 = 0;

    AUTO_LOCK(mpMutex);

    do {
        DASSERT(mpFreeMemStart <= mpMemoryEnd);
        DASSERT(mpFreeMemStart >= mpMemoryBase);
        if (DUnlikely((mpFreeMemStart > mpMemoryEnd) || (mpFreeMemStart < mpMemoryBase))) {
            LOG_ERROR("mpFreeMemStart %p not in valid range[%p, %p)\n", mpFreeMemStart, mpMemoryBase, mpMemoryEnd);
        }

        DASSERT(mpUsedMemStart <= mpMemoryEnd);
        DASSERT(mpUsedMemStart >= mpMemoryBase);
        if (DUnlikely((mpUsedMemStart > mpMemoryEnd) || (mpUsedMemStart < mpMemoryBase))) {
            LOG_ERROR("mpUsedMemStart %p not in valid range[%p, %p)\n", mpUsedMemStart, mpMemoryBase, mpMemoryEnd);
        }

        if (DUnlikely(((TMemSize)mpFreeMemStart) == ((TMemSize)mpUsedMemStart))) {
            if (mRequestWrapCount == mReleaseWrapCount) {
                //LOGM_NOTICE("hit\n");
            } else {
                if (DUnlikely(((mRequestWrapCount + 1) == mReleaseWrapCount) || ((255 == mRequestWrapCount) && (0 == mReleaseWrapCount)))) {
                    currentFreeSize1 = mpMemoryEnd - mpFreeMemStart;
                    currentFreeSize2 = mpUsedMemStart - mpMemoryBase;

                    if (currentFreeSize1 >= size) {
                        p_ret = mpFreeMemStart;
                        mpFreeMemStart += size;
                        return p_ret;
                    } else if (currentFreeSize2 >= size) {
                        p_ret = mpMemoryBase;
                        mpFreeMemStart = mpMemoryBase + size;

                        mRequestWrapCount ++;
                        return p_ret;
                    } else {
                        LOG_FATAL("both currentFreeSize1 %ld and currentFreeSize2 %ld, not fit request size %ld\n", currentFreeSize1, currentFreeSize2, size);
                    }
                } else {
                    LOG_WARN("in exit flow? mRequestWrapCount %d, mReleaseWrapCount %d\n", mRequestWrapCount, mReleaseWrapCount);
                }
            }
        } else if (((TMemSize)mpFreeMemStart) > ((TMemSize)mpUsedMemStart)) {

            //debug assert
            //DASSERT((mReleaseWrapCount == (mRequestWrapCount + 1)) || ((255 == mRequestWrapCount) && (0 == mReleaseWrapCount)));

            currentFreeSize1 = mpMemoryEnd - mpFreeMemStart;
            currentFreeSize2 = mpUsedMemStart - mpMemoryBase;

            if (currentFreeSize1 >= size) {
                p_ret = mpFreeMemStart;
                mpFreeMemStart += size;
                return p_ret;
            } else if (currentFreeSize2 >= size) {
                p_ret = mpMemoryBase;
                mpFreeMemStart = mpMemoryBase + size;

                mRequestWrapCount ++;
                return p_ret;
            } else {
                //LOG_FATAL("both currentFreeSize1 %ld and currentFreeSize2 %ld, not fit request size %ld\n", currentFreeSize1, currentFreeSize2, size);
            }

        } else {
            //debug assert
            //DASSERT(mRequestWrapCount == mReleaseWrapCount);

            currentFreeSize1 = mpUsedMemStart - mpFreeMemStart;
            if (currentFreeSize1 >= size) {
                p_ret = mpFreeMemStart;
                mpFreeMemStart += size;
                return p_ret;
            }
        }

#if 0
        if (((TMemSize)mpFreeMemStart) >= ((TMemSize)mpUsedMemStart)) {
            currentFreeSize1 = mpMemoryEnd - mpFreeMemStart;
            currentFreeSize2 = mpUsedMemStart - mpMemoryBase;

            if (currentFreeSize1 >= size) {
                p_ret = mpFreeMemStart;
                mpFreeMemStart += size;
                //LOGM_NOTICE("RequestMemBlock ret %p\n", p_ret);
                return p_ret;
            } else if (currentFreeSize2 >= size) {
                p_ret = mpMemoryBase;
                mpFreeMemStart = mpMemoryBase + size;
                //LOGM_NOTICE("RequestMemBlock ret %p\n", p_ret);
                return p_ret;
            }
        } else {
            currentFreeSize1 = mpUsedMemStart - mpFreeMemStart;
            if (currentFreeSize1 >= size) {
                p_ret = mpFreeMemStart;
                mpFreeMemStart += size;
                //LOGM_NOTICE("RequestMemBlock ret %p\n", p_ret);
                return p_ret;
            }
        }
#endif

        mnWaiters ++;
        mpCond->Wait(mpMutex);
    } while (1);

    LOG_FATAL("must not comes here\n");
    return NULL;
}

void CRingMemPool::ReturnBackMemBlock(TMemSize size, TU8 *start_pointer)
{
    AUTO_LOCK(mpMutex);

    DASSERT((start_pointer + size) == mpFreeMemStart);
    DASSERT((start_pointer + size) <= (mpMemoryEnd));
    DASSERT(start_pointer >= (mpMemoryBase));

    mpFreeMemStart = start_pointer;
    if (mnWaiters) {
        mnWaiters --;
        mpCond->Signal();
    }
}

void CRingMemPool::ReleaseMemBlock(TMemSize size, TU8 *start_pointer)
{
    AUTO_LOCK(mpMutex);

    if (DLikely(start_pointer != mpMemoryBase)) {
        if (DUnlikely(start_pointer != mpUsedMemStart)) {
            LOG_WARN("in exit flow? start_pointer %p, mpUsedMemStart %p\n", start_pointer, mpUsedMemStart);
        }
    }
    DASSERT((start_pointer + size) <= (mpMemoryEnd));

    if (DUnlikely(start_pointer < (mpMemoryBase))) {
        LOG_FATAL("start_pointer %p, mpMemoryBase %p\n", start_pointer, mpMemoryBase);
        return;
    } else if (DUnlikely((start_pointer + size) > (mpMemoryEnd))) {
        LOG_FATAL("start_pointer %p, size %ld, mpMemoryBase %p\n", start_pointer, size, mpMemoryBase);
        return;
    }

    if (mpFreeMemStart > mpUsedMemStart) {
        if ((TULong)(start_pointer + size) > (TULong)mpFreeMemStart) {
            LOG_FATAL("application bug! size %ld large than expected %ld\n", size, (TULong)(mpFreeMemStart - mpUsedMemStart));
        }
    }

    if (DUnlikely((start_pointer + size) < mpUsedMemStart)) {
        if (start_pointer == mpMemoryBase) {
            mReleaseWrapCount ++;
        } else {
            LOG_WARN("in exit flow? start_pointer %p not equal to mpMemoryBase %p\n", start_pointer, mpMemoryBase);
            return;
        }
    }
    mpUsedMemStart = start_pointer + size;
    if (mnWaiters) {
        mnWaiters --;
        mpCond->Signal();
    }
}

//-----------------------------------------------------------------------
//
// CFixedMemPool
//
//-----------------------------------------------------------------------
IMemPool *CFixedMemPool::Create(const TChar *name, TMemSize size, TUint tot_count)
{
    CFixedMemPool *thiz = new CFixedMemPool(name);
    if (thiz && (EECode_OK == thiz->Construct(size, tot_count))) {
        return thiz;
    } else {
        if (thiz) {
            LOG_FATAL("CFixedMemPool->Construct(size = %ld, count =%d) fail\n", size, tot_count);
            delete thiz;
        } else {
            LOG_FATAL("new CFixedMemPool() fail\n");
        }
    }

    return NULL;
}

CObject *CFixedMemPool::GetObject0() const
{
    return (CObject *) this;
}

void CFixedMemPool::Delete()
{
    if (mpMemoryBase) {
        free(mpMemoryBase);
        mpMemoryBase = NULL;
    }

    mMemoryTotalSize = 0;
    mMemBlockSize = 0;

    mpFreeMemStart = NULL;
    mpUsedMemStart = NULL;

    inherited::Delete();
}

CFixedMemPool::CFixedMemPool(const TChar *name)
    : inherited(name)
    , mpMutex(NULL)
    , mpCond(NULL)
    , mpMemoryBase(NULL)
    , mpMemoryEnd(NULL)
    , mMemoryTotalSize(0)
    , mpFreeMemStart(NULL)
    , mpUsedMemStart(NULL)
    , mMemBlockSize(0)
    , mnTotalBlockNumber(0)
    , mnCurrentFreeBlockNumber(0)
    , mnWaiters(0)
{

}

EECode CFixedMemPool::Construct(TMemSize size, TUint count)
{
    DASSERT(!mpMemoryBase);

    mMemoryTotalSize = size * count;
    mpMemoryBase = (TU8 *) DDBG_MALLOC(mMemoryTotalSize, "C0FM");
    if (mpMemoryBase) {
        mMemBlockSize = size;
        mnTotalBlockNumber = count;
        mnCurrentFreeBlockNumber = count;
        mpFreeMemStart = mpMemoryBase;
        mpUsedMemStart = mpMemoryBase;
        mpMemoryEnd = mpMemoryBase + mMemoryTotalSize;
    } else {
        LOG_FATAL("alloc mem(size = %lu, total_count %d) fail\n", size, count);
        return EECode_NoMemory;
    }

    DASSERT(!mpMutex);
    mpMutex = gfCreateMutex();
    if (!mpMutex) {
        LOG_FATAL("gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    DASSERT(!mpCond);
    mpCond = gfCreateCondition();
    if (!mpCond) {
        LOG_FATAL("gfCreateCondition() fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CFixedMemPool::~CFixedMemPool()
{
    if (mpMemoryBase) {
        DDBG_FREE(mpMemoryBase, "C0FM");
    }
}

TU8 *CFixedMemPool::RequestMemBlock(TMemSize size, TU8 *start_pointer)
{
    TU8 *p_ret = NULL;

    AUTO_LOCK(mpMutex);

    DASSERT(size == mMemBlockSize);
    DASSERT(NULL == start_pointer);
    DASSERT(mnCurrentFreeBlockNumber <= mnTotalBlockNumber);

    do {
        DASSERT(mpFreeMemStart <= mpMemoryEnd);
        DASSERT(mpFreeMemStart >= mpMemoryBase);

        if (mnCurrentFreeBlockNumber) {
            p_ret = mpFreeMemStart;
            mpFreeMemStart += mMemBlockSize;
            mnCurrentFreeBlockNumber --;
            if (mpFreeMemStart >= mpMemoryEnd) {
                DASSERT(mpFreeMemStart == mpMemoryEnd);
                mpFreeMemStart = mpMemoryBase;
            }
            return p_ret;
        }

        mnWaiters ++;
        mpCond->Wait(mpMutex);
    } while (1);

    LOG_FATAL("must not comes here\n");
    return NULL;
}

void CFixedMemPool::ReturnBackMemBlock(TMemSize size, TU8 *start_pointer)
{
    LOGM_FATAL("must NOT come here\n");
}

void CFixedMemPool::ReleaseMemBlock(TMemSize size, TU8 *start_pointer)
{
    AUTO_LOCK(mpMutex);

    DASSERT(start_pointer == mpUsedMemStart);
    DASSERT(size == mMemBlockSize);

    mpUsedMemStart = start_pointer;
    mnCurrentFreeBlockNumber ++;
    if (mnWaiters) {
        mnWaiters --;
        mpCond->Signal();
    }
}

//-----------------------------------------------------------------------
//
//  CISimplePool
//
//-----------------------------------------------------------------------
CISimplePool *CISimplePool::Create(TUint max_count)
{
    CISimplePool *result = new CISimplePool();
    if (result && result->Construct(max_count) != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

void CISimplePool::PrintStatus()
{
    TUint v = 0;

    DASSERT(mpBufferQ);
    if (mpBufferQ) {
        v = mpBufferQ->GetDataCnt();
    }
    LOG_PRINTF("free data piece count %d.\n", v);
}

void CISimplePool::Delete()
{
    if (mpBufferQ) {
        mpBufferQ->Delete();
        mpBufferQ = NULL;
    }

    if (mpDataPieceStructMemory) {
        free(mpDataPieceStructMemory);
        mpDataPieceStructMemory = NULL;
    }
}

CISimplePool::CISimplePool()
    : mpBufferQ(NULL)
    , mpDataPieceStructMemory(NULL)
{
}

TUint CISimplePool::GetFreeDataPieceCnt() const
{
    DASSERT(mpBufferQ);
    if (mpBufferQ) {
        return mpBufferQ->GetDataCnt();
    }

    return 0;
}

EECode CISimplePool::Construct(TUint nMaxBuffers)
{
    EECode err;

    mpBufferQ = CIQueue::Create(NULL, this, sizeof(SDataPiece *), nMaxBuffers);
    if (DUnlikely(!mpBufferQ)) {
        LOG_FATAL("CISimplePool::Construct, this %p, nMaxBuffers %d, no memory!\n", this, nMaxBuffers);
        return EECode_NoMemory;
    }

    if (nMaxBuffers) {
        mpDataPieceStructMemory = (TU8 *) DDBG_MALLOC(sizeof(SDataPiece) * nMaxBuffers, "C0SP");
        if (mpDataPieceStructMemory == NULL) {
            return EECode_NoMemory;
        }

        memset(mpDataPieceStructMemory, 0x0, sizeof(SDataPiece) * nMaxBuffers);

        SDataPiece *dataPiece = (SDataPiece *)mpDataPieceStructMemory;
        for (TUint i = 0; i < nMaxBuffers; i++, dataPiece++) {
            err = mpBufferQ->PostMsg(&dataPiece, sizeof(dataPiece));
        }

        LOG_INFO("count %d, mpBufferQ %p, CISimplePool %p\n", mpBufferQ->GetDataCnt(), mpBufferQ, this);

        DASSERT_OK(err);
    }

    return EECode_OK;
}

CISimplePool::~CISimplePool()
{
    if (mpBufferQ) {
        mpBufferQ->Delete();
        mpBufferQ = NULL;
    }

    if (mpDataPieceStructMemory) {
        DDBG_FREE(mpDataPieceStructMemory, "C0SP");
        mpDataPieceStructMemory = NULL;
    }
}

bool CISimplePool::AllocDataPiece(SDataPiece *&pDataPiece, TUint size)
{
    if (DLikely(mpBufferQ->GetMsgEx(&pDataPiece, sizeof(SDataPiece)))) {
        return true;
    }
    return false;
}

void CISimplePool::ReleaseDataPiece(SDataPiece *pDataPiece)
{
    DASSERT(pDataPiece);
    EECode err = mpBufferQ->PostMsg((void *)&pDataPiece, sizeof(SDataPiece *));
    DASSERT_OK(err);
}


CIResourceAllocator::CIResourceAllocator()
    : mMax(0)
    , mMin(0)
    , mRefreshThreshold(0)
    , mCurrentBufferLength(0)
    , mpSlots(NULL)
{
    mpMutex = gfCreateMutex();
}

CIResourceAllocator::~CIResourceAllocator()
{
    if (mpSlots) {
        free(mpSlots);
        mpSlots = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }
}

EECode CIResourceAllocator::SetLimit(TResourceType max, TResourceType min, TResourceType refresh_threshold)
{
    AUTO_LOCK(mpMutex);

    if ((TU32)max < (TU32)min + (TU32)refresh_threshold) {
        LOG_FATAL("bad value\n");
        return EECode_BadParam;
    }

    mCurrentBufferLength = max - min + 1;
    mpSlots = (SResourceSlot *) malloc(mCurrentBufferLength * sizeof(SResourceSlot));
    if (!mpSlots) {
        LOG_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    mRefreshThreshold = refresh_threshold;
    mMax = max;
    mMin = min;

    memset(mpSlots, 0x0, (mCurrentBufferLength * sizeof(SResourceSlot)));
    reset();

    return EECode_OK;
}

TResourceType CIResourceAllocator::PreAllocate(TResourceType v)
{
    AUTO_LOCK(mpMutex);

    if (mCurPreAllocateNumber > mRefreshThreshold) {
        clearPreAllocated();
    }

    if ((v >= mMin) && (v <= mMax)) {
        if (0 == mpSlots[v - mMin].state) {
            mpSlots[v - mMin].state = 1;
            mCurPreAllocateNumber ++;
            return v;
        }
    }

    for (TResourceType i = mMin; i <= mMax; i ++) {
        if (0 == mpSlots[i - mMin].state) {
            mpSlots[i - mMin].state = 1;
            mCurPreAllocateNumber ++;
            return mpSlots[i - mMin].res;
        }
    }

    LOG_FATAL("must not comes here, mCurPreAllocateNumber %d, mMin %d, mMax %d\n", mCurPreAllocateNumber, mMin, mMax);
    return mMax;
}

EECode CIResourceAllocator::Allocate(TResourceType v)
{
    AUTO_LOCK(mpMutex);

    if ((v < mMin)  || (v > mMax)) {
        LOG_FATAL("bad value, %d\n", v);
        return EECode_BadParam;
    }

    if (1 == mpSlots[v - mMin].state) {
        mpSlots[v - mMin].state = 2;
        return EECode_OK;
    } else if (0 == mpSlots[v - mMin].state) {
        mpSlots[v - mMin].state = 2;
        mCurPreAllocateNumber ++;
        LOG_WARN("allocated directly, %d\n", v);
        return EECode_OK;
    }

    LOG_FATAL("already allocated, %d, v %d\n", mpSlots[v - mMin].state, v);
    return EECode_BadParam;
}

EECode CIResourceAllocator::Release(TResourceType v)
{
    AUTO_LOCK(mpMutex);

    if ((v < mMin)  || (v > mMax)) {
        LOG_FATAL("bad value, %d\n", v);
        return EECode_BadParam;
    }

    if (0 == mpSlots[v - mMin].state) {
        LOG_WARN("already released, v %d\n", v);
        return EECode_BadParam;
    }

    mpSlots[v - mMin].state = 0;
    if (mCurPreAllocateNumber > 0) {
        mCurPreAllocateNumber --;
    } else {
        LOG_FATAL("logic error\n");
    }
    return EECode_InternalLogicalBug;
}

void CIResourceAllocator::Reset()
{
    AUTO_LOCK(mpMutex);
    reset();
}

void CIResourceAllocator::clearPreAllocated()
{
    for (TResourceType i = mMin; i <= mMax; i ++) {
        if (1 == mpSlots[i - mMin].state) {
            mpSlots[i - mMin].state = 0;
            if (mCurPreAllocateNumber > 0) {
                mCurPreAllocateNumber --;
            } else {
                LOG_FATAL("logic error\n");
            }
        }
    }
}

void CIResourceAllocator::reset()
{
    mCurPreAllocateNumber = 0;
    for (TResourceType i = 0; i < mCurrentBufferLength; i ++) {
        mpSlots[i].res = mMin + i;
        mpSlots[i].state = 0;
    }
}

extern ICustomizedCodec *gfCreateCustomizedADPCMCodec(TUint index);

ICustomizedCodec *gfCustomizedCodecFactory(TChar *codec_name, TUint index)
{
    if (DLikely(codec_name)) {
        if (!strcmp(codec_name, "customized_adpcm")) {
            return gfCreateCustomizedADPCMCodec(0);
        } else {
            LOG_ERROR("not supported codec %s\n", codec_name);
        }
    }

    return NULL;
}

const TChar *gfGetServerManagerStateString(EServerManagerState state)
{
    switch (state) {
        case EServerManagerState_idle:
            return "(EServerManagerState_idle)";
            break;
        case EServerManagerState_noserver_alive:
            return "(EServerManagerState_noserver_alive)";
            break;
        case EServerManagerState_running:
            return "(EServerManagerState_running)";
            break;
        case EServerManagerState_halt:
            return "(EServerManagerState_halt)";
            break;
        case EServerManagerState_error:
            return "(EServerManagerState_error)";
            break;
        default:
            LOG_ERROR("Unknown module state %d\n", state);
            break;
    }

    return "(Unknown module state)";
}

extern IIPCAgent *gfCreateUnixDomainSocketIPCAgent();

IIPCAgent *gfIPCAgentFactory(EIPCAgentType type)
{
    IIPCAgent *thiz = NULL;

    switch (type) {
        case EIPCAgentType_UnixDomainSocket:
#ifdef BUILD_OS_WINDOWS
            LOG_ERROR("windows not implement it yet\n");
            return NULL;
#else
            thiz = gfCreateUnixDomainSocketIPCAgent();
#endif
            break;

        default:
            LOG_FATAL("not support type %d\n", type);
            break;
    }

    return thiz;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

