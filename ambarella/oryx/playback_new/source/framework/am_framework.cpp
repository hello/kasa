/**
 * am_framework.cpp
 *
 * History:
 *  2015/07/27 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
    case EECode_ParseError:
      return "(EECode_ParseError)";
      break;
    case EECode_UnknownError:
      return "(EECode_UnknownError)";
      break;
    default:
      break;
  }
  return "(Unknown error code)";
}

const TChar *gfGetComponentStringFromGenericComponentType(TComponentType type)
{
  switch (type) {
    case EGenericComponentType_Demuxer:
      return "EGenericComponentType_Demuxer";
    case EGenericComponentType_VideoDecoder:
      return "EGenericComponentType_VideoDecoder";
    case EGenericComponentType_AudioDecoder:
      return "EGenericComponentType_AudioDecoder";
    case EGenericComponentType_VideoEncoder:
      return "EGenericComponentType_VideoEncoder";
    case EGenericComponentType_AudioEncoder:
      return "EGenericComponentType_AudioEncoder";
    case EGenericComponentType_VideoRenderer:
      return "EGenericComponentType_VideoRenderer";
    case EGenericComponentType_AudioRenderer:
      return "EGenericComponentType_AudioRenderer";
    case EGenericComponentType_Muxer:
      return "EGenericComponentType_Muxer";
    case EGenericComponentType_StreamingServer:
      return "EGenericComponentType_StreamingServer";
    case EGenericComponentType_StreamingTransmiter:
      return "EGenericComponentType_StreamingTransmiter";
    case EGenericComponentType_StreamingContent:
      return "EGenericComponentType_StreamingContent";
    case EGenericComponentType_FlowController:
      return "EGenericComponentType_FlowController";
    case EGenericComponentType_VideoCapture:
      return "EGenericComponentType_VideoCapture";
    case EGenericComponentType_AudioCapture:
      return "EGenericComponentType_AudioCapture";
    case EGenericComponentType_VideoInjecter:
      return "EGenericComponentType_VideoInjecter";
    case EGenericComponentType_VideoRawSink:
      return "EGenericComponentType_VideoRawSink";
    case EGenericComponentType_VideoESSink:
      return "EGenericComponentType_VideoESSink";
    case EGenericComponentType_ConnectionPin:
      return "EGenericComponentType_ConnectionPin";
    case EGenericPipelineType_Playback:
      return "EGenericPipelineType_Playback";
      break;
    case EGenericPipelineType_Recording:
      return "EGenericPipelineType_Recording";
      break;
    case EGenericPipelineType_Streaming:
      return "EGenericPipelineType_Streaming";
      break;
    default:
      LOG_FATAL("unknown EGenericComponentType %d.\n", type);
      break;
  }
  return "UnknownComponentType";
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
    case EModuleState_Renderer_WaitDecoderReady:
      return "(EModuleState_Renderer_WaitDecoderReady)";
    case EModuleState_Renderer_WaitVoutDormant:
      return "(EModuleState_Renderer_WaitVoutDormant)";
    case EModuleState_Renderer_WaitVoutEOSFlag:
      return "(EModuleState_Renderer_WaitVoutEOSFlag)";
    case EModuleState_Renderer_WaitVoutLastFrameDisplayed:
      return "(EModuleState_Renderer_WaitVoutLastFrameDisplayed)";
    case EModuleState_DirectEncoding:
      return "(EModuleState_DirectEncoding)";
    default:
      LOG_ERROR("Unknown module state %x\n", state);
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
      LOG_ERROR("Unknown CMD type 0x%08x\n", type);
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

#define NODE_SIZE   (sizeof(List) + mBlockSize)
#define NODE_BUFFER(_pNode) ((TU8*)_pNode + sizeof(List))

// blockSize - bytes of each queue item
// nReservedSlots - number of reserved nodes
CIQueue *CIQueue::Create(CIQueue *pMainQ, void *pOwner, TUint blockSize, TUint nReservedSlots)
{
  CIQueue *result = new CIQueue(pMainQ, pOwner);
  if (result && result->Construct(blockSize, nReservedSlots) != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

void CIQueue::Delete()
{
  delete this;
}

CIQueue::CIQueue(CIQueue *pMainQ, void *pOwner)
  : mpOwner(pOwner)
  , mbDisabled(false)
  , mpMainQ(pMainQ)
  , mpPrevQ(this)
  , mpNextQ(this)

  , mpMutex(NULL)
  , mpCondReply(NULL)
  , mpCondGet(NULL)
  , mpCondSendMsg(NULL)

  , mnGet(0)
  , mnSendMsg(0)
  , mBlockSize(0)
  , mnData(0)

  , mpFreeList(NULL)

  , mpSendBuffer(NULL)
  , mpReservedMemory(NULL)

  , mpMsgResult(NULL)
  , mpCurrentCircularlyQueue(NULL)
{
  mHead.pNext = NULL;
  mHead.bAllocated = false;
  mpTail = (List *)&mHead;
}

EECode CIQueue::Construct(TUint blockSize, TUint nReservedSlots)
{
  mBlockSize = DROUND_UP(blockSize, 4);
  mpReservedMemory = (TU8 *) DDBG_MALLOC(NODE_SIZE * (nReservedSlots + 1), "Q0RM");
  if (mpReservedMemory == NULL) {
    LOG_FATAL("no memory\n");
    return EECode_NoMemory;
  }
  // for SendMsg()
  mpSendBuffer = (List *)mpReservedMemory;
  mpSendBuffer->pNext = NULL;
  mpSendBuffer->bAllocated = false;
  // reserved nodes, keep in free-list
  List *pNode = (List *)(mpReservedMemory + NODE_SIZE);
  for (; nReservedSlots > 0; nReservedSlots--) {
    pNode->bAllocated = false;
    pNode->pNext = mpFreeList;
    mpFreeList = pNode;
    pNode = (List *)((TU8 *)pNode + NODE_SIZE);
  }
  if (IsMain()) {
    if (NULL == (mpMutex = gfCreateMutex())) {
      LOG_FATAL("no memory: gfCreateMutex()\n");
      return EECode_NoMemory;
    }
    if (NULL == (mpCondGet = gfCreateCondition())) {
      LOG_FATAL("no memory: gfCreateCondition()\n");
      return EECode_NoMemory;
    }
    if (NULL == (mpCondReply = gfCreateCondition())) {
      LOG_FATAL("no memory: gfCreateCondition()\n");
      return EECode_NoMemory;
    }
    if (NULL == (mpCondSendMsg = gfCreateCondition())) {
      LOG_FATAL("no memory: gfCreateCondition()\n");
      return EECode_NoMemory;
    }
  } else {
    mpMutex = mpMainQ->mpMutex;
    mpCondGet = mpMainQ->mpCondGet;
    mpCondReply = mpMainQ->mpCondReply;
    mpCondSendMsg = mpMainQ->mpCondSendMsg;
    // attach to main-Q
    AUTO_LOCK(mpMainQ->mpMutex);
    mpPrevQ = mpMainQ->mpPrevQ;
    mpNextQ = mpMainQ;
    mpPrevQ->mpNextQ = this;
    mpNextQ->mpPrevQ = this;
  }
  return EECode_OK;
}

CIQueue::~CIQueue()
{
  if (mpMutex)
  { __LOCK(mpMutex); }
  DASSERT(mnGet == 0);
  DASSERT(mnSendMsg == 0);
  if (IsSub()) {
    // detach from main-Q
    mpPrevQ->mpNextQ = mpNextQ;
    mpNextQ->mpPrevQ = mpPrevQ;
    if (mpMainQ->mpCurrentCircularlyQueue == this) {
      mpMainQ->mpCurrentCircularlyQueue = NULL;
    }
  } else {
    // all sub-Qs should be removed
    if (DUnlikely(mpPrevQ != this)) {
      LOG_ERROR("sub queue is not destroyed?\n");
    }
    if (DUnlikely(mpNextQ != this)) {
      LOG_ERROR("sub queue is not destroyed?\n");
    }
    DASSERT(mpMsgResult == NULL);
  }
  LOG_RESOURCE("CQueue0x%p: before mHead.Delete(), mHead.pNext %p.\n", this, mHead.pNext);
  mHead.Delete();
  LOG_RESOURCE("CQueue0x%p: before AM_DELETE(mpFreeList), mpFreeList %p.\n", this, mpFreeList);
  if (mpFreeList) {
    mpFreeList->Delete();
    mpFreeList = NULL;
  }
  LOG_RESOURCE("after mpFreeList delete.\n");
  if (mpReservedMemory) {
    DDBG_FREE(mpReservedMemory, "Q0RM");
    mpReservedMemory = NULL;
  }
  LOG_RESOURCE("after delete mpReservedMemory.\n");
  if (mpMutex)
  { __UNLOCK(mpMutex); }
  if (IsMain()) {
    if (mpCondSendMsg) {
      mpCondSendMsg->Delete();
      mpCondSendMsg = NULL;
    }
    LOG_RESOURCE("after delete mpCondSendMsg.\n");
    if (mpCondReply) {
      mpCondReply->Delete();
      mpCondReply = NULL;
    }
    LOG_RESOURCE("after delete mpCondReply.\n");
    if (mpCondGet) {
      mpCondGet->Delete();
      mpCondGet = NULL;
    }
    LOG_RESOURCE("after delete mpCondGet.\n");
    if (mpMutex) {
      mpMutex->Delete();
      mpMutex = NULL;
    }
    LOG_RESOURCE("after delete mpMutex.\n");
  }
}

void CIQueue::List::Delete()
{
  List *pNode = this;
  while (pNode) {
    List *pNext = pNode->pNext;
    if (pNode->bAllocated) {
      DDBG_FREE(pNode, "Q0ND");
    }
    pNode = pNext;
  }
}

EECode CIQueue::PostMsg(const void *pMsg, TUint msgSize)
{
  DASSERT(IsMain());
  AUTO_LOCK(mpMutex);
  List *pNode = AllocNode();
  if (pNode == NULL) {
    LOG_FATAL("no memory\n");
    return EECode_NoMemory;
  }
  WriteData(pNode, pMsg, msgSize);
  if (mnGet > 0) {
    mnGet--;
    mpCondGet->Signal();
  }
  return EECode_OK;
}

EECode CIQueue::SendMsg(const void *pMsg, TUint msgSize)
{
  DASSERT(IsMain());
  AUTO_LOCK(mpMutex);
  while (1) {
    if (mpMsgResult == NULL) {
      WriteData(mpSendBuffer, pMsg, msgSize);
      if (mnGet > 0) {
        mnGet--;
        mpCondGet->Signal();
      }
      EECode result;
      mpMsgResult = &result;
      mpCondReply->Wait(mpMutex);
      mpMsgResult = NULL;
      if (mnSendMsg > 0) {
        mnSendMsg--;
        mpCondSendMsg->Signal();
      }
      return result;
    }
    mnSendMsg++;
    mpCondSendMsg->Wait(mpMutex);
  }
}

void CIQueue::Reply(EECode result)
{
  AUTO_LOCK(mpMutex);
  DASSERT(IsMain());
  DASSERT(mpMsgResult);
  if (!mpMsgResult) {
    LOG_ERROR("logic error\n");
    return;
  }
  *mpMsgResult = result;
  mpCondReply->Signal();
}

void CIQueue::GetMsg(void *pMsg, TUint msgSize)
{
  DASSERT(IsMain());
  AUTO_LOCK(mpMutex);
  while (1) {
    if (mnData > 0) {
      ReadData(pMsg, msgSize);
      return;
    }
    mnGet++;
    mpCondGet->Wait(mpMutex);
  }
}

bool CIQueue::GetMsgEx(void *pMsg, TUint msgSize)
{
  DASSERT(IsMain());
  AUTO_LOCK(mpMutex);
  while (1) {
    if (mbDisabled)
    { return false; }
    if (mnData > 0) {
      ReadData(pMsg, msgSize);
      return true;
    }
    mnGet++;
    mpCondGet->Wait(mpMutex);
  }
}

void CIQueue::Enable(bool bEnabled)
{
  DASSERT(IsMain());
  AUTO_LOCK(mpMutex);
  mbDisabled = !bEnabled;
  if (mnGet > 0) {
    mnGet = 0;
    mpCondGet->SignalAll();
  }
}

bool CIQueue::PeekMsg(void *pMsg, TUint msgSize)
{
  DASSERT(IsMain());
  AUTO_LOCK(mpMutex);
  if (mnData > 0) {
    if (pMsg) {
      ReadData(pMsg, msgSize);
    }
    return true;
  }
  return false;
}

EECode CIQueue::PutData(const void *pBuffer, TUint size)
{
  DASSERT(IsSub());
  AUTO_LOCK(mpMutex);
  List *pNode = AllocNode();
  if (pNode == NULL) {
    LOG_FATAL("CIQueue::PutData: AllocNode fail, no memory\n");
    return EECode_NoMemory;
  }
  WriteData(pNode, pBuffer, size);
  if (mpMainQ->mnGet > 0) {
    mpMainQ->mnGet--;
    mpMainQ->mpCondGet->Signal();
  }
  return EECode_OK;
}

// wait this main-Q and all its sub-Qs
CIQueue::QType CIQueue::WaitDataMsg(void *pMsg, TUint msgSize, WaitResult *pResult)
{
  DASSERT(IsMain());
  AUTO_LOCK(mpMutex);
  while (1) {
    if (mnData > 0) {
      ReadData(pMsg, msgSize);
      return Q_MSG;
    }
    for (CIQueue *q = mpNextQ; q != this; q = q->mpNextQ) {
      if (q->mnData > 0) {
        pResult->pDataQ = q;
        pResult->pOwner = q->mpOwner;
        pResult->blockSize = q->mBlockSize;
        return Q_DATA;
      }
    }
    mnGet++;
    mpCondGet->Wait(mpMutex);
  }
}

// wait this main-Q and specified sub-Q
CIQueue::QType CIQueue::WaitDataMsgWithSpecifiedQueue(void *pMsg, TUint msgSize, const CIQueue *pQueue)
{
  DASSERT(IsMain());
  if (pQueue->mpMainQ != this) {
    WaitMsg(pMsg, msgSize);
    return Q_MSG;
  }
  AUTO_LOCK(mpMutex);
  while (1) {
    if (mnData > 0) {
      ReadData(pMsg, msgSize);
      return Q_MSG;
    }
    if (pQueue->mnData > 0) {
      return Q_DATA;
    }
    mnGet++;
    mpCondGet->Wait(mpMutex);
  }
}

// wait only MSG queue(main queue)
void CIQueue::WaitMsg(void *pMsg, TUint msgSize)
{
  DASSERT(IsMain());
  AUTO_LOCK(mpMutex);
  while (1) {
    if (mnData > 0) {
      ReadData(pMsg, msgSize);
      return;
    }
    mnGet++;
    mpCondGet->Wait(mpMutex);
  }
}

EECode CIQueue::swicthToNextDataQueue(CIQueue *pCurrent)
{
  DASSERT(IsMain());
  if (NULL == pCurrent) {
    DASSERT(NULL == mpCurrentCircularlyQueue);
    pCurrent = mpNextQ;
    if (pCurrent == this || NULL == pCurrent) {
      LOG_ERROR("!!!There's no sub queue(%p)? fatal error here, no-sub queue, must not come here.\n", pCurrent);
      //need return something to notify error?
      return EECode_NoMemory;
    }
    mpCurrentCircularlyQueue = pCurrent;
    //LOG_DEBUG("first time, choose next queue(%p).\n", pCurrent);
    return EECode_OK;
  } else if (this == pCurrent) {
    LOG_ERROR(" this == pCurrent? would have logical error before.\n");
    mpCurrentCircularlyQueue = mpNextQ;
    return EECode_OK;
  }
  DASSERT(pCurrent);
  DASSERT(pCurrent != this);
  pCurrent = pCurrent->mpNextQ;
  DASSERT(pCurrent->mpNextQ);
  if (pCurrent == this) {
    mpCurrentCircularlyQueue = mpNextQ;
    DASSERT(mpCurrentCircularlyQueue != this);
    return EECode_OK;
  }
  mpCurrentCircularlyQueue = pCurrent;
  DASSERT(mpCurrentCircularlyQueue != this);
  return EECode_OK;
}

//add here?
//except msg, make pins without priority
CIQueue::QType CIQueue::WaitDataMsgCircularly(void *pMsg, TUint msgSize, WaitResult *pResult)
{
  EECode err;
  DASSERT(IsMain());
  AUTO_LOCK(mpMutex);
  while (1) {
    if (mnData > 0) {
      ReadData(pMsg, msgSize);
      return Q_MSG;
    }
    if (mpNextQ == this) {
      mnGet++;
      mpCondGet->Wait(mpMutex);
      continue;
    }
    DASSERT(mpCurrentCircularlyQueue != this);
    if (mpCurrentCircularlyQueue == this || NULL == mpCurrentCircularlyQueue) {
      err = swicthToNextDataQueue(mpCurrentCircularlyQueue);
      //DASSERT(EECode_OK == err);
      if (EECode_OK != err) {
        DASSERT(EECode_NotExist == err);
        //AM_ERROR("!!!Internal error must not comes here.\n");
        //need return some error code? only mainQ, and no msg
        DASSERT(0);
        return Q_NONE;
      }
    }
    //peek mpCurrentCircularlyQueue first
    DASSERT(mpCurrentCircularlyQueue);
    DASSERT(mpCurrentCircularlyQueue != this);
    if (mpCurrentCircularlyQueue->mnData > 0) {
      pResult->pDataQ = mpCurrentCircularlyQueue;
      pResult->pOwner = mpCurrentCircularlyQueue->mpOwner;
      pResult->blockSize = mpCurrentCircularlyQueue->mBlockSize;
      err = swicthToNextDataQueue(mpCurrentCircularlyQueue);
      DASSERT(EECode_OK == err);
      return Q_DATA;
    } else {
      //AM_INFO("Queue Warning Debug: Selected Queue has no Data\n");
    }
    for (CIQueue *q = mpCurrentCircularlyQueue->mpNextQ; q != mpCurrentCircularlyQueue; q = q->mpNextQ) {
      if (q->mnData > 0) {
        pResult->pDataQ = q;
        pResult->pOwner = q->mpOwner;
        pResult->blockSize = q->mBlockSize;
        err = swicthToNextDataQueue(q);
        DASSERT(EECode_OK == err);
        return Q_DATA;
      }
    }
    mnGet++;
    mpCondGet->Wait(mpMutex);
  }
}

bool CIQueue::PeekData(void *pBuffer, TUint size)
{
  DASSERT(IsSub());
  AUTO_LOCK(mpMutex);
  if (mnData > 0) {
    ReadData(pBuffer, size);
    return true;
  }
  return false;
}

CIQueue::List *CIQueue::AllocNode()
{
  if (mpFreeList) {
    List *pNode = mpFreeList;
    mpFreeList = mpFreeList->pNext;
    pNode->pNext = NULL;
    return pNode;
  }
  List *pNode = (List *)DDBG_MALLOC(NODE_SIZE, "Q0ND");
  if (pNode == NULL) {
    LOG_FATAL("no memory, NODE_SIZE %lu\n", (TULong)NODE_SIZE);
    return NULL;
  }
  pNode->pNext = NULL;
  pNode->bAllocated = true;
  //LOG_RESOURCE("CQueue0x%p alloc new node %p.\n", this, pNode);
  return pNode;
}

void CIQueue::WriteData(List *pNode, const void *pBuffer, TUint size)
{
  Copy(NODE_BUFFER(pNode), pBuffer, DCAL_MIN(mBlockSize, size));
  mnData++;
#ifdef DCONFIG_ENABLE_DEBUG_CHECK
  DASSERT(mpTail->pNext == NULL);
#endif
  pNode->pNext = NULL;
  mpTail->pNext = pNode;
  mpTail = pNode;
}

void CIQueue::ReadData(void *pBuffer, TUint size)
{
  List *pNode = mHead.pNext;
  DASSERT(pNode);
#ifdef DCONFIG_ENABLE_DEBUG_CHECK
  DASSERT(mpTail->pNext == NULL);
#endif
  mHead.pNext = mHead.pNext->pNext;
  //tail is read out, change tail
  if (mHead.pNext == NULL) {
    DASSERT(mpTail == pNode);
    DASSERT(mnData == 1);
    mpTail = (List *)&mHead;
  }
  if (pNode != mpSendBuffer) {
    pNode->pNext = mpFreeList;
    mpFreeList = pNode;
  }
  Copy(pBuffer, NODE_BUFFER(pNode), DCAL_MIN(mBlockSize, size));
  mnData--;
}


CIBuffer::CIBuffer()
  : mRefCount(0)
  , mpNext(NULL)
  , mBufferType(EBufferType_Invalid)
  , mVideoFrameType(0)
  , mbAudioPCMChannelInterlave(0)
  , mAudioChannelNumber(0)
  , mAudioSampleFormat(0)
  , mbAllocated(0)
  , mbExtEdge(0)
  , mContentFormat(0)
  , mbDataSegment(0)
  , mFlags(0)
  , mCustomizedContentFormat(0)
  , mCustomizedContentType(EBufferCustomizedContentType_Invalid)
  , mpCustomizedContent(NULL)
  , mSubPacketNumber(0)
  , mpPool(NULL)
  , mAudioBitrate(0)
  , mVideoBitrate(0)
  , mPTS(0)
  , mVideoWidth(0)
  , mVideoHeight(0)
  , mVideoOffsetX(0)
  , mVideoOffsetY(0)
  , mAudioSampleRate(0)
  , mAudioFrameSize(0)
  , mBufferSeqNumber0(0)
  , mBufferSeqNumber1(0)
  , mExpireTime(0)
  , mBufferId(0)
  , mEncId(0)
{
  TUint i = 0;
  for (i = 0; i < MEMORY_BLOCK_NUMBER; i++) {
    mpData[i] = NULL;
    mpMemoryBase[i] = NULL;
    mMemorySize[i] = 0;
    mDataSize[i] = 0;
  }
  for (i = 0; i < MEMORY_BLOCK_NUMBER; i++) {
    mVideoBufferLinesize[i] = 0;
  }
  for (i = 0; i < DMAX_MEDIA_SUB_PACKET_NUMBER; i++) {
    mOffsetHint[i] = 0;
  }
  mMaxVideoGopSize = 0;
  mCurVideoGopSize = 0;
}

CIBuffer::~CIBuffer()
{
}

void CIBuffer::AddRef()
{
  if (mpPool) {
    mpPool->AddRef(this);
  }
}

void CIBuffer::Release()
{
  if (mpPool) {
    mpPool->Release(this);
  }
}

void CIBuffer::ReleaseAllocatedMemory()
{
  if (mbAllocated) {
    mbAllocated = 0;
    TU32 i = 0;
    for (i = 0; i < MEMORY_BLOCK_NUMBER; i ++) {
      if (mpMemoryBase[i]) {
        DDBG_FREE(mpMemoryBase[i], "BPBM");
        mpMemoryBase[i] = NULL;
        mMemorySize[i] = 0;
      }
    }
  }
}

EBufferType CIBuffer::GetBufferType() const
{
  return mBufferType;
}

void CIBuffer::SetBufferType(EBufferType type)
{
  mBufferType = type;
}

TUint CIBuffer::GetBufferFlags() const
{
  return mFlags;
}

void CIBuffer::SetBufferFlags(TUint flags)
{
  mFlags = flags;
}

void CIBuffer::SetBufferFlagBits(TUint flags)
{
  mFlags |= flags;
}

void CIBuffer::ClearBufferFlagBits(TUint flags)
{
  mFlags &= ~flags;
}

TU8 *CIBuffer::GetDataPtr(TUint index) const
{
  if (MEMORY_BLOCK_NUMBER <= index) {
    LOG_ERROR("BAD input index %d\n", index);
    return NULL;
  }
  return mpData[index];
}

void CIBuffer::SetDataPtr(TU8 *pdata, TUint index)
{
  if (MEMORY_BLOCK_NUMBER <= index) {
    LOG_ERROR("BAD input index %d\n", index);
    return;
  }
  mpData[index] = pdata;
}

TMemSize CIBuffer::GetDataSize(TUint index) const
{
  if (MEMORY_BLOCK_NUMBER <= index) {
    LOG_ERROR("BAD input index %d\n", index);
    return 0;
  }
  return mDataSize[index];
}

void CIBuffer::SetDataSize(TMemSize size, TUint index)
{
  if (MEMORY_BLOCK_NUMBER <= index) {
    LOG_ERROR("BAD input index %d\n", index);
    return;
  }
  mDataSize[index] = size;
}

TU32 CIBuffer::GetDataLineSize(TUint index) const
{
  if (MEMORY_BLOCK_NUMBER <= index) {
    LOG_ERROR("BAD input index %d\n", index);
    return 0;
  }
  return mDataLineSize[index];
}

void CIBuffer::SetDataLineSize(TU32 size, TUint index)
{
  if (MEMORY_BLOCK_NUMBER <= index) {
    LOG_ERROR("BAD input index %d\n", index);
    return;
  }
  mDataLineSize[index] = size;
}

TU8 *CIBuffer::GetDataPtrBase(TUint index) const
{
  if (MEMORY_BLOCK_NUMBER <= index) {
    LOG_ERROR("BAD input index %d\n", index);
    return NULL;
  }
  return mpMemoryBase[index];
}

void CIBuffer::SetDataPtrBase(TU8 *pdata, TUint index)
{
  if (MEMORY_BLOCK_NUMBER <= index) {
    LOG_ERROR("BAD input index %d\n", index);
    return;
  }
  mpMemoryBase[index] = pdata;
}

TMemSize CIBuffer::GetDataMemorySize(TUint index) const
{
  if (MEMORY_BLOCK_NUMBER <= index) {
    LOG_ERROR("BAD input index %d\n", index);
    return 0;
  }
  return mMemorySize[index];
}

void CIBuffer::SetDataMemorySize(TMemSize size, TUint index)
{
  if (MEMORY_BLOCK_NUMBER <= index) {
    LOG_ERROR("BAD input index %d\n", index);
    return;
  }
  mMemorySize[index] = size;
}

TTime CIBuffer::GetBufferPTS() const
{
  return mPTS;
}

void CIBuffer::SetBufferPTS(TTime pts)
{
  mPTS = pts;
}

TTime CIBuffer::GetBufferDTS() const
{
  return mDTS;
}

void CIBuffer::SetBufferDTS(TTime dts)
{
  mDTS = dts;
}

TTime CIBuffer::GetBufferNativePTS() const
{
  return mNativePTS;
}

void CIBuffer::SetBufferNativePTS(TTime pts)
{
  mNativePTS = pts;
}

TTime CIBuffer::GetBufferNativeDTS() const
{
  return mNativeDTS;
}

void CIBuffer::SetBufferNativeDTS(TTime dts)
{
  mNativeDTS = dts;
}

TTime CIBuffer::GetBufferLinearPTS() const
{
  return mLinearPTS;
}

void CIBuffer::SetBufferLinearPTS(TTime pts)
{
  mLinearPTS = pts;
}

TTime CIBuffer::GetBufferLinearDTS() const
{
  return mLinearDTS;
}

void CIBuffer::SetBufferLinearDTS(TTime dts)
{
  mLinearDTS = dts;
}

//-----------------------------------------------------------------------
//
//  CBufferPool
//
//-----------------------------------------------------------------------
CBufferPool *CBufferPool::Create(const TChar *name, TUint max_count)
{
  CBufferPool *result = new CBufferPool(name);
  if (result && result->Construct(max_count) != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

void CBufferPool::PrintStatus()
{
  TUint v = 0;
  DASSERT(mpBufferQ);
  if (mpBufferQ) {
    v = mpBufferQ->GetDataCnt();
  }
  LOGM_PRINTF("\t\t[PrintStatus] CBufferPool: free buffer count %d.\n", v);
}

void CBufferPool::Delete()
{
  LOGM_RESOURCE("buffer pool '%s' destroyed\n", mpName);
  if (mpBufferQ) {
    mpBufferQ->Delete();
    mpBufferQ = NULL;
  }
  destroyBuffers();
  if (mpBufferMemory) {
    DDBG_FREE(mpBufferMemory, "MFBP");
    mpBufferMemory = NULL;
  }
  inherited::Delete();
}

CBufferPool::CBufferPool(const char *pName)
  : inherited(pName)
  , mpBufferQ(NULL)
  , mRefCount(1)
  , mpName(pName)
  , mpEventListener(NULL)
  , mpReleaseBufferCallBack(NULL)
  , mpBufferMemory(NULL)
{
}

const TChar *CBufferPool::GetName() const
{
  return mpName;
}

TUint CBufferPool::GetFreeBufferCnt() const
{
  DASSERT(mpBufferQ);
  if (mpBufferQ) {
    return mpBufferQ->GetDataCnt();
  }
  return 0;
}

void CBufferPool::AddBufferNotifyListener(IEventListener *p_listener)
{
  //DASSERT(!mpEventListener);
  mpEventListener = p_listener;
}

void CBufferPool::OnReleaseBuffer(CIBuffer *pBuffer)
{
  if (mpReleaseBufferCallBack) {
    mpReleaseBufferCallBack(pBuffer);
  }
}

EECode CBufferPool::Construct(TUint nMaxBuffers)
{
  EECode err = EECode_OK;
  if ((mpBufferQ = CIQueue::Create(NULL, this, sizeof(CIBuffer *), nMaxBuffers)) == NULL) {
    LOGM_FATAL("CBufferPool::Construct, this %p, nMaxBuffers %d, no memory!\n", this, nMaxBuffers);
    return EECode_NoMemory;
  }
  if (nMaxBuffers) {
    mpBufferMemory = (TU8 *) DDBG_MALLOC(sizeof(CIBuffer) * nMaxBuffers, "MFBP");
    if (mpBufferMemory == NULL) {
      return EECode_NoMemory;
    }
    mnTotalBufferNumber = nMaxBuffers;
    memset(mpBufferMemory, 0x0, sizeof(CIBuffer) * nMaxBuffers);
    CIBuffer *pBuffer = (CIBuffer *)mpBufferMemory;
    for (TUint i = 0; i < mnTotalBufferNumber; i++, pBuffer++) {
      pBuffer->mpPool = this;
      err = mpBufferQ->PostMsg(&pBuffer, sizeof(pBuffer));
      DASSERT_OK(err);
    }
    LOGM_INFO("count %d, mpBufferQ %p, CBufferPool %p\n", mpBufferQ->GetDataCnt(), mpBufferQ, this);
  }
  return EECode_OK;
}

CBufferPool::~CBufferPool()
{
}

void CBufferPool::Enable(bool bEnabled)
{
  mpBufferQ->Enable(bEnabled);
}

bool CBufferPool::AllocBuffer(CIBuffer *&pBuffer, TUint size)
{
  if (mpBufferQ->GetMsgEx(&pBuffer, sizeof(pBuffer))) {
    pBuffer->mRefCount = 1;
    pBuffer->mpPool = this;
    pBuffer->mpNext = NULL;
    pBuffer->mFlags = 0;
    return true;
  }
  return false;
}

void CBufferPool::AddRef(CIBuffer *pBuffer)
{
  __atomic_inc(&pBuffer->mRefCount);
}

void CBufferPool::Release(CIBuffer *pBuffer)
{
  //LOGM_VERBOSE("--mRefCount:%d\n",pBuffer->mRefCount);
  if (__atomic_dec(&pBuffer->mRefCount) == 1) {
    OnReleaseBuffer(pBuffer);
    EECode err = mpBufferQ->PostMsg((void *)&pBuffer, sizeof(CIBuffer *));
    if (mpEventListener) {
      mpEventListener->EventNotify(EEventType_BufferReleaseNotify, 0, (TPointer)this);
    }
    DASSERT_OK(err);
  }
  //LOGM_VERBOSE("--after Release\n");
}

void CBufferPool::SetReleaseBufferCallBack(TBufferPoolReleaseBufferCallBack callback)
{
  mpReleaseBufferCallBack = callback;
}

void CBufferPool::destroyBuffers()
{
  TU32 i = 0, j = 0;
  CIBuffer *pBuffer = (CIBuffer *)mpBufferMemory;
  for (i = 0; i < mnTotalBufferNumber; i++, pBuffer++) {
    if (pBuffer->mbAllocated) {
      pBuffer->mbAllocated = 0;
      for (j = 0; j < CIBuffer::MEMORY_BLOCK_NUMBER; j++) {
        if (pBuffer->mpMemoryBase[j]) {
          DDBG_FREE(pBuffer->mpMemoryBase[j], "BPBM");
          pBuffer->mpMemoryBase[j] = NULL;
          pBuffer->mMemorySize[j] = 0;
        }
      }
    }
  }
}

CActiveObject::CActiveObject(const TChar *pName, TUint index)
  : inherited(pName, index)
  , mpWorkQ(NULL)
  , mbRun(1)
  , mbPaused(0)
  , mbTobeStopped(0)
  , mbTobeSuspended(0)
{
}

EECode CActiveObject::Construct()
{
  if (NULL == (mpWorkQ = CIWorkQueue::Create((IActiveObject *)this))) {
    LOGM_FATAL("CActiveObject::Construct: CIWorkQueue::Create fail\n");
    return EECode_NoMemory;
  }
  return EECode_OK;
}

CActiveObject::~CActiveObject()
{
  LOGM_RESOURCE("~CActiveObject\n");
  if (mpWorkQ) {
    mpWorkQ->Delete();
    mpWorkQ = NULL;
  }
  LOGM_RESOURCE("~CActiveObject Done\n");
}

void CActiveObject::Delete()
{
  inherited::Delete();
}

CObject *CActiveObject::GetObject() const
{
  return (CObject *) this;
}

EECode CActiveObject::SendCmd(TUint cmd)
{
  return mpWorkQ->SendCmd(cmd);
}

EECode CActiveObject::PostCmd(TUint cmd)
{
  return mpWorkQ->PostMsg(cmd);
}

void CActiveObject::OnRun()
{
}

CIQueue *CActiveObject::MsgQ()
{
  return mpWorkQ->MsgQ();
}

void CActiveObject::GetCmd(SCMD &cmd)
{
  mpWorkQ->GetCmd(cmd);
}

bool CActiveObject::PeekCmd(SCMD &cmd)
{
  return mpWorkQ->PeekCmd(cmd);
}

void CActiveObject::CmdAck(EECode err)
{
  mpWorkQ->CmdAck(err);
}

CFilter::CFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
  : inherited(pName)
  , mpPersistMediaConfig(pPersistMediaConfig)
  , mpEngineMsgSink(pEngineMsgSink)
{
}

CFilter::~CFilter()
{
}

void CFilter::Delete()
{
  inherited::Delete();
}

void CFilter::Pause()
{
  LOGM_ERROR("please implement this api.\n");
}

void CFilter::Resume()
{
  LOGM_ERROR("please implement this api.\n");
}

void CFilter::Flush()
{
  LOGM_ERROR("please implement this api.\n");
}

void CFilter::PrintState()
{
  LOGM_ERROR("please implement this api.\n");
}

EECode CFilter::Start()
{
  return EECode_OK;
}

EECode CFilter::SendCmd(TUint cmd)
{
  LOGM_ERROR("please implement this api.\n");
  return EECode_NoImplement;
}

void CFilter::PostMsg(TUint cmd)
{
  LOGM_ERROR("please implement this api.\n");
}

EECode CFilter::FlowControl(EBufferType type)
{
  LOGM_ERROR("please implement this api, type %d.\n", type);
  return EECode_NoImplement;
}

IInputPin *CFilter::GetInputPin(TUint index)
{
  LOGM_ERROR("please implement this api.\n");
  return NULL;
}

IOutputPin *CFilter::GetOutputPin(TUint index, TUint sub_index)
{
  LOGM_ERROR("!!!must not comes here, not implement.\n");
  return NULL;
}

EECode CFilter::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
  LOGM_ERROR("!!!must not comes here, not implement.\n");
  return EECode_NoImplement;
}

EECode CFilter::AddInputPin(TUint &index, TUint type)
{
  LOGM_ERROR("!!!must not comes here, not implement.\n");
  return EECode_NoImplement;
}

void CFilter::EventNotify(EEventType type, TUint param1, TPointer param2)
{
  LOGM_ERROR("no event listener.\n");
  return;
}

EECode CFilter::PostEngineMsg(SMSG &msg)
{
  if (mpEngineMsgSink) {
    return mpEngineMsgSink->MsgProc(msg);
  } else {
    LOGM_ERROR("NULL mpEngineMsgSink, in CFilter::PostEngineMsg\n");
  }
  return EECode_BadState;
}

EECode CFilter::PostEngineMsg(TUint code)
{
  SMSG msg;
  msg.code = code;
  msg.p1  = (TULong)static_cast<IFilter *>(this);
  return PostEngineMsg(msg);
}

//-----------------------------------------------------------------------
//
//  CInputPin
//
//-----------------------------------------------------------------------

CInputPin::CInputPin(const TChar *name, IFilter *pFilter, StreamType type)
  : inherited(name)
  , mpFilter(pFilter)
  , mpPeer(NULL)
  , mpBufferPool(NULL)
  , mPeerSubIndex(0)
  , mbEnable(1)
  , mbSyncedBufferComes(0)
  , mpMutex(NULL)
  , mpExtraData(NULL)
  , mExtraDataSize(0)
  , mExtraDataBufferSize(0)
  , mPinType(type)
  , mCodecFormat(StreamFormat_Invalid)
  , mVideoWidth(0)
  , mVideoHeight(0)
  , mVideoFrameRateNum(30)
  , mVideoFrameRateDen(1)
  , mVideoOffsetX(0)
  , mVideoOffsetY(0)
  , mVideoSampleAspectRatioNum(1)
  , mVideoSampleAspectRatioDen(1)
  , mAudioSampleRate(0)
  , mAudioFrameSize(0)
  , mAudioChannelNumber(2)
  , mAudioLayout(0)
  , mSessionNumber(0)
{
  mpMutex = gfCreateMutex();
  if (!mpMutex) {
    LOG_FATAL("CInputPin: gfCreateMutex() fail\n");
  }
}

void CInputPin::Delete()
{
  if (mpMutex) {
    mpMutex->Delete();
    mpMutex = NULL;
  }
  if (mpExtraData) {
    DDBG_FREE(mpExtraData, "MIEX");
    mpExtraData = NULL;
  }
  mExtraDataSize = 0;
  mExtraDataBufferSize = 0;
  inherited::Delete();
}

CInputPin::~CInputPin()
{
}

void CInputPin::PrintStatus()
{
  LOGM_PRINTF("[CInputPin::PrintStatus()], mPeerSubIndex %d, mbEnable %d\n", mPeerSubIndex, mbEnable);
}

void CInputPin::Enable(TU8 bEnable)
{
  AUTO_LOCK(mpMutex);
  mbEnable = bEnable;
}

IOutputPin *CInputPin::GetPeer() const
{
  return mpPeer;
}

IFilter *CInputPin::GetFilter() const
{
  return mpFilter;
}

EECode CInputPin::Connect(IOutputPin *pPeer, TComponentIndex index)
{
  DASSERT(!mpPeer);
  if (!mpPeer) {
    mpPeer = pPeer;
    mPeerSubIndex = index;
    DASSERT(!mpBufferPool);
    mpBufferPool = mpPeer->GetBufferPool();
    if (!mpBufferPool) {
      //LOGM_FATAL("NULL mpBufferPool %p\n", mpBufferPool);
      //return EECode_InternalLogicalBug;
    }
    return EECode_OK;
  }
  LOGM_FATAL("already connected!\n");
  return EECode_InternalLogicalBug;
}

EECode CInputPin::Disconnect(TComponentIndex index)
{
  DASSERT(mpPeer);
  DASSERT(index == mPeerSubIndex);
  if (mpPeer) {
    mpPeer = NULL;
    DASSERT(mpBufferPool);
    mpBufferPool = NULL;
    return EECode_OK;
  }
  LOGM_FATAL("not connected!\n");
  return EECode_InternalLogicalBug;
}

StreamType CInputPin::GetPinType() const
{
  return mPinType;
}

EECode CInputPin::GetVideoParams(StreamFormat &format, TU32 &video_width, TU32 &video_height, TU8 *&p_extra_data, TMemSize &extra_data_size, TU32 &session_number) const
{
  AUTO_LOCK(mpMutex);
  if (DLikely(StreamType_Video == mPinType)) {
    if (DLikely(mbSyncedBufferComes)) {
      format = mCodecFormat;
      video_width = mVideoWidth;
      video_height = mVideoHeight;
      p_extra_data = mpExtraData;
      extra_data_size = mExtraDataSize;
      session_number = mSessionNumber;
    } else {
      LOGM_FATAL("sync buffer not comes yet?\n");
      return EECode_BadState;
    }
  } else {
    LOGM_FATAL("it's not a video pin\n");
    return EECode_BadParam;
  }
  return EECode_OK;
}

EECode CInputPin::GetAudioParams(StreamFormat &format, TU32 &channel_number, TU32 &sample_frequency, AudioSampleFMT &fmt, TU8 *&p_extra_data, TMemSize &extra_data_size, TU32 &session_number) const
{
  AUTO_LOCK(mpMutex);
  if (DLikely(StreamType_Audio == mPinType)) {
    if (DLikely(mbSyncedBufferComes)) {
      format = mCodecFormat;
      channel_number = mAudioChannelNumber;
      sample_frequency = mAudioSampleRate;
      fmt = mAudioSampleFmt;
      p_extra_data = mpExtraData;
      extra_data_size = mExtraDataSize;
      session_number = mSessionNumber;
    } else {
      LOGM_WARN("sync buffer not comes yet?\n");
      return EECode_BadState;
    }
  } else {
    LOGM_FATAL("it's not a audio pin\n");
    return EECode_BadParam;
  }
  return EECode_OK;
}

EECode CInputPin::GetExtraData(TU8 *&p_extra_data, TMemSize &extra_data_size, TU32 &session_number) const
{
  AUTO_LOCK(mpMutex);
  p_extra_data = mpExtraData;
  extra_data_size = mExtraDataSize;
  session_number = mSessionNumber;
  return EECode_OK;
}

void CInputPin::updateSyncBuffer(CIBuffer *p_sync_Buffer)
{
  if (StreamType_Video == mPinType) {
    mVideoWidth = p_sync_Buffer->mVideoWidth;
    mVideoHeight = p_sync_Buffer->mVideoHeight;
    mVideoFrameRateNum = p_sync_Buffer->mVideoFrameRateNum;
    mVideoFrameRateDen = p_sync_Buffer->mVideoFrameRateDen;
    mVideoOffsetX = p_sync_Buffer->mVideoOffsetX;
    mVideoOffsetY = p_sync_Buffer->mVideoOffsetY;
    mVideoSampleAspectRatioNum = p_sync_Buffer->mVideoSampleAspectRatioNum;
    mVideoSampleAspectRatioDen = p_sync_Buffer->mVideoSampleAspectRatioDen;
  } else if (StreamType_Audio == mPinType) {
    mAudioSampleRate = p_sync_Buffer->mAudioSampleRate;
    mAudioFrameSize = p_sync_Buffer->mAudioFrameSize;
    mAudioChannelNumber = p_sync_Buffer->mAudioChannelNumber;
  }
  mSessionNumber = p_sync_Buffer->mSessionNumber;
}

void CInputPin::updateExtraData(CIBuffer *p_extradata_Buffer)
{
  mSessionNumber = p_extradata_Buffer->mSessionNumber;
  TU8 *p = p_extradata_Buffer->GetDataPtr();
  TUint size = p_extradata_Buffer->GetDataSize();
  if (DLikely(p && size)) {
    if (mpExtraData && (size > mExtraDataBufferSize)) {
      DDBG_FREE(mpExtraData, "FIEX");
      mpExtraData = NULL;
      mExtraDataBufferSize = 0;
    }
    if (!mpExtraData) {
      mpExtraData = (TU8 *) DDBG_MALLOC(size, "FIEX");
      if (DLikely(mpExtraData)) {
        mExtraDataBufferSize = size;
      }
    }
    if (mpExtraData) {
      memcpy(mpExtraData, p, size);
      mExtraDataSize = size;
      LOG_INFO("inputPin %p received %s ExtraData size %d, first 8 bytes: 0x%02x 0x%02x 0x%02x 0x%02x, 0x%02x 0x%02x 0x%02x 0x%02x\n", this, (EBufferType_VideoExtraData == p_extradata_Buffer->GetBufferType()) ? "video" : "audio", size, \
               p[0], p[1], p[2], p[3], \
               p[4], p[5], p[6], p[7]);
    } else {
      LOG_FATAL("inputPin %p received %s ExtraData %p size %u, NO memory to save it.\n", this, (EBufferType_VideoExtraData == p_extradata_Buffer->GetBufferType()) ? "video" : "audio", p, size);
    }
  } else {
    LOG_FATAL("inputPin %p received %s ExtraData %p size %u, can't save.\n", this, (EBufferType_VideoExtraData == p_extradata_Buffer->GetBufferType()) ? "video" : "audio", p, size);
  }
}

//-----------------------------------------------------------------------
//
//  COutputPin
//
//-----------------------------------------------------------------------

COutputPin::COutputPin(const TChar *pname, IFilter *pFilter)
  : inherited(pname)
  , mpFilter(pFilter)
  , mpBufferPool(NULL)
  , mbSetBP(0)
  , mbEnabledAllSubpin(1)
  , mnCurrentSubPinNumber(0)
{
  TUint i = 0;
  for (i = 0; i < DMAX_CONNECTIONS_OUTPUTPIN; i ++) {
    mpPeers[i] = NULL;
    //mbEnabled[i] = 0;
  }
}

COutputPin::~COutputPin()
{
}

void COutputPin::PrintStatus()
{
  TUint i = 0;
  LOGM_PRINTF("\t\t[PrintStatus] COutputPin: mnCurrentSubPinNumber %d\n", mnCurrentSubPinNumber);
  for (i = 0; i < mnCurrentSubPinNumber; i ++) {
    LOGM_PRINTF("\t\t\tsub[%d], peer %p\n", i, mpPeers[i]);
  }
}


COutputPin *COutputPin::Create(const TChar *pname, IFilter *pfilter)
{
  COutputPin *result = new COutputPin(pname, pfilter);
#if 0
  if (result && result->Construct() != EECode_OK) {
    LOGM_FATAL("COutputPin::Construct() fail.\n");
    delete result;
    result = NULL;
  }
#endif
  return result;
}

bool COutputPin::AllocBuffer(CIBuffer *&pBuffer, TUint size)
{
  DASSERT(mpBufferPool);
  if (mpBufferPool) {
    return mpBufferPool->AllocBuffer(pBuffer, size);
  } else {
    LOGM_FATAL("NULL pointer in COutputPin::AllocBuffer\n");
    return false;
  }
}

void COutputPin::SendBuffer(CIBuffer *pBuffer)
{
  if (mbEnabledAllSubpin) {
    TU16 index = 0;
    for (index = 0; index < mnCurrentSubPinNumber; index ++) {
      //if (mbEnabled[index] && mpPeers[index]) {
      pBuffer->AddRef();
      mpPeers[index]->Receive(pBuffer);
      //}
    }
  }
  pBuffer->Release();
}

/*
void COutputPin::Enable(TUint index, TU8 bEnable)
{
    if (index < mnCurrentSubPinNumber) {
        mbEnabled[index] = bEnable;
    } else {
        LOGM_FATAL("BAD index %u, exceed max %hu\n", index, mnCurrentSubPinNumber);
    }
}
*/

void COutputPin::EnableAllSubPin(TU8 bEnable)
{
  mbEnabledAllSubpin = bEnable;
}

IInputPin *COutputPin::GetPeer(TUint index)
{
  if (index < mnCurrentSubPinNumber) {
    return mpPeers[index];
  } else {
    LOGM_FATAL("BAD index %u, exceed max %hu\n", index, mnCurrentSubPinNumber);
  }
  return NULL;
}

IFilter *COutputPin::GetFilter()
{
  return mpFilter;
}

EECode COutputPin::Connect(IInputPin *pPeer, TUint index)
{
  DASSERT(mnCurrentSubPinNumber >= index);
  DASSERT(DMAX_CONNECTIONS_OUTPUTPIN >= mnCurrentSubPinNumber);
  if ((mnCurrentSubPinNumber >= index) && (DMAX_CONNECTIONS_OUTPUTPIN >= mnCurrentSubPinNumber)) {
    DASSERT(!mpPeers[index]);
    if (!mpPeers[index]) {
      mpPeers[index] = pPeer;
      //mbEnabled[index] = 1;
      return EECode_OK;
    } else {
      LOGM_FATAL("already connected? COutputPin this %p, mnCurrentSubPinNumber %d, request index %d, mpBufferPool %p, mpPeers[index] %p\n", this, mnCurrentSubPinNumber, index, mpBufferPool, mpPeers[index]);
      return EECode_InternalLogicalBug;
    }
  }
  LOGM_FATAL("BAD input parameters index %d, mnCurrentSubPinNumber %d, DMAX_CONNECTIONS_OUTPUTPIN %d\n", index, mnCurrentSubPinNumber, DMAX_CONNECTIONS_OUTPUTPIN);
  return EECode_BadParam;
}

EECode COutputPin::Disconnect(TUint index)
{
  DASSERT(mnCurrentSubPinNumber > index);
  DASSERT(DMAX_CONNECTIONS_OUTPUTPIN >= mnCurrentSubPinNumber);
  if ((mnCurrentSubPinNumber > index) && (DMAX_CONNECTIONS_OUTPUTPIN >= mnCurrentSubPinNumber)) {
    DASSERT(mpPeers[index]);
    if (mpPeers[index]) {
      mpPeers[index] = NULL;
      //mbEnabled[index] = 0;
      return EECode_OK;
    } else {
      LOGM_FATAL("not connected? COutputPin this %p, mnCurrentSubPinNumber %d, request index %d, mpBufferPool %p, mpPeers[index] %p\n", this, mnCurrentSubPinNumber, index, mpBufferPool, mpPeers[index]);
      return EECode_InternalLogicalBug;
    }
  }
  LOGM_FATAL("BAD input parameters index %d, mnCurrentSubPinNumber %d, DMAX_CONNECTIONS_OUTPUTPIN %d\n", index, mnCurrentSubPinNumber, DMAX_CONNECTIONS_OUTPUTPIN);
  return EECode_BadParam;
}

TUint COutputPin::NumberOfPins()
{
  return mnCurrentSubPinNumber;
}

EECode COutputPin::AddSubPin(TUint &sub_index)
{
  if (DMAX_CONNECTIONS_OUTPUTPIN > mnCurrentSubPinNumber) {
    sub_index = mnCurrentSubPinNumber;
    mnCurrentSubPinNumber ++;
  } else {
    LOGM_FATAL("COutputPin::AddSubPin(), too many sub pins(%d) now\n", mnCurrentSubPinNumber);
    return EECode_TooMany;
  }
  return EECode_OK;
}

IBufferPool *COutputPin::GetBufferPool() const
{
  return mpBufferPool;
}

void COutputPin::SetBufferPool(IBufferPool *pBufferPool)
{
  DASSERT(!mpBufferPool);
  if (!mpBufferPool) {
    mpBufferPool = pBufferPool;
  } else {
    LOG_FATAL("already set buffer pool!\n");
  }
}

//-----------------------------------------------------------------------
//
//  CQueueInputPin
//
//-----------------------------------------------------------------------

CQueueInputPin *CQueueInputPin::Create(const TChar *pname, IFilter *pfilter, StreamType type, CIQueue *pMsgQ)
{
  CQueueInputPin *result = new CQueueInputPin(pname, pfilter, type);
  if (result && result->Construct(pMsgQ) != EECode_OK) {
    LOG_FATAL("CQueueInputPin::Construct() fail.\n");
    delete result;
    result = NULL;
  }
  return result;
}

CQueueInputPin::CQueueInputPin(const TChar *name, IFilter *filter, StreamType type)
  : inherited(name, filter, type)
  , mpBufferQ(NULL)
{
}

EECode CQueueInputPin::Construct(CIQueue *pMsgQ)
{
  if ((mpBufferQ = CIQueue::Create(pMsgQ, this, sizeof(CIBuffer *), 16)) == NULL) {
    LOGM_FATAL("CQueueInputPin::Construct: CIQueue::Create fail.\n");
    return EECode_NoMemory;
  }
  return EECode_OK;
}

void CQueueInputPin::PrintStatus()
{
  LOGM_PRINTF("\t\t[PrintStatus]: CQueueInputPin: mPeerSubIndex %d, mbEnable %d, mpBufferQ's data count %d\n", mPeerSubIndex, mbEnable, mpBufferQ->GetDataCnt());
}

CQueueInputPin::~CQueueInputPin()
{
  if (mpBufferQ) {
    mpBufferQ->Delete();
    mpBufferQ = NULL;
  }
}

void CQueueInputPin::Delete()
{
  if (mpBufferQ) {
    mpBufferQ->Delete();
    mpBufferQ = NULL;
  }
  inherited::Delete();
}

void CQueueInputPin::Receive(CIBuffer *pBuffer)
{
  AUTO_LOCK(mpMutex);
  if (DUnlikely(CIBuffer::SYNC_POINT & pBuffer->GetBufferFlags())) {
    updateSyncBuffer(pBuffer);
  }
  if (DUnlikely((EBufferType_VideoExtraData == pBuffer->GetBufferType()) || (EBufferType_AudioExtraData == pBuffer->GetBufferType()))) {
    updateExtraData(pBuffer);
  }
  if (mbEnable) {
    EECode err = EECode_OK;
    err = mpBufferQ->PutData(&pBuffer, sizeof(pBuffer));
    DASSERT_OK(err);
  } else {
    pBuffer->Release();
  }
  return;
}

void CQueueInputPin::Purge(TU8 disable_pin)
{
  AUTO_LOCK(mpMutex);
  if (mpBufferQ) {
    CIBuffer *pBuffer;
    while (mpBufferQ->PeekData((void *)&pBuffer, sizeof(pBuffer))) {
      pBuffer->Release();
    }
  }
  if (disable_pin) {
    mbEnable = 0;
  }
}

bool CQueueInputPin::PeekBuffer(CIBuffer *&pBuffer)
{
  AUTO_LOCK(mpMutex);
  if (!mbEnable) {
    return false;
  }
  return mpBufferQ->PeekData((void *)&pBuffer, sizeof(pBuffer));
}

CIQueue *CQueueInputPin::GetBufferQ() const
{
  return mpBufferQ;
}

//-----------------------------------------------------------------------
//
// CBypassInputPin
//
//-----------------------------------------------------------------------

CBypassInputPin *CBypassInputPin::Create(const TChar *pname, IFilter *pfilter, StreamType type)
{
  CBypassInputPin *result = new CBypassInputPin(pname, pfilter, type);
  if (result && result->Construct() != EECode_OK) {
    LOG_FATAL("CBypassInputPin::Construct() fail.\n");
    delete result;
    result = NULL;
  }
  return result;
}

CBypassInputPin::CBypassInputPin(const TChar *name, IFilter *filter, StreamType type)
  : inherited(name, filter, type)
{
}

EECode CBypassInputPin::Construct()
{
  return EECode_OK;
}

CBypassInputPin::~CBypassInputPin()
{
}

void CBypassInputPin::Delete()
{
  inherited::Delete();
}

void CBypassInputPin::Receive(CIBuffer *pBuffer)
{
  AUTO_LOCK(mpMutex);
  DASSERT(pBuffer);
  if (mbEnable) {
    if (DLikely(mpFilter)) {
      SProcessBuffer processbuffer;
      processbuffer.pin_owner = (void *)this;
      processbuffer.p_buffer = pBuffer;
      mpFilter->FilterProperty(EFilterPropertyType_process_buffer, 1, (void *)&processbuffer);
    } else {
      LOGM_FATAL("NULL filter\n");
      pBuffer->Release();
    }
  } else {
    pBuffer->Release();
  }
}

void CBypassInputPin::Purge(TU8 disable_pin)
{
  AUTO_LOCK(mpMutex);
  if (disable_pin) {
    mbEnable = 0;
  }
}

//-----------------------------------------------------------------------
//
//  CActiveFilter
//
//-----------------------------------------------------------------------
CActiveFilter::CActiveFilter(const TChar *pName, TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
  : inherited(pName, index)
  , mpPersistMediaConfig(pPersistMediaConfig)
  , mpEngineMsgSink(pEngineMsgSink)
  , mpWorkQ(NULL)
  , mbRun(0)
  , mbPaused(0)
  , mbStarted(0)
  , mbSuspended(0)
  , msState(EModuleState_Invalid)
{
  if (mpPersistMediaConfig) {
    mpClockReference = mpPersistMediaConfig->p_system_clock_reference;
  } else {
    mpClockReference = NULL;
  }
}

EECode CActiveFilter::Construct()
{
  if (NULL == (mpWorkQ = CIWorkQueue::Create((IActiveObject *)this))) {
    LOGM_FATAL("CActiveFilter::Construct: CIWorkQueue::Create fail\n");
    return EECode_NoMemory;
  }
  return EECode_OK;
}

CActiveFilter::~CActiveFilter()
{
}

void CActiveFilter::Delete()
{
  if (mpWorkQ) {
    mpWorkQ->Delete();
    mpWorkQ = NULL;
  }
  inherited::Delete();
}

CObject *CActiveFilter::GetObject0() const
{
  return (CObject *) this;
}

CObject *CActiveFilter::GetObject() const
{
  return (CObject *) this;
}

EECode CActiveFilter::Run()
{
  return mpWorkQ->Run();
}

EECode CActiveFilter::Exit()
{
  return mpWorkQ->Exit();
}

EECode CActiveFilter::Start()
{
  return mpWorkQ->Start();
}

EECode CActiveFilter::Stop()
{
  return mpWorkQ->Stop();
}

void CActiveFilter::Pause()
{
  mpWorkQ->PostMsg(ECMDType_Pause);
}

void CActiveFilter::Resume()
{
  mpWorkQ->PostMsg(ECMDType_Resume);
}

void CActiveFilter::Flush()
{
  mpWorkQ->SendCmd(ECMDType_Flush);
}

void CActiveFilter::ResumeFlush()
{
  mpWorkQ->SendCmd(ECMDType_ResumeFlush);
}

EECode CActiveFilter::Suspend(TUint release_flag)
{
  SCMD cmd;
  cmd.code = ECMDType_Suspend;
  cmd.flag = release_flag;
  return mpWorkQ->SendCmd(cmd);
}

EECode CActiveFilter::ResumeSuspend(TComponentIndex input_index)
{
  SCMD cmd;
  cmd.code = ECMDType_ResumeSuspend;
  cmd.flag = input_index;
  return mpWorkQ->SendCmd(cmd);
}

EECode CActiveFilter::SendCmd(TUint cmd)
{
  return mpWorkQ->SendCmd(cmd);
}

void CActiveFilter::PostMsg(TUint cmd)
{
  mpWorkQ->PostMsg(cmd);
}

void CActiveFilter::GetInfo(INFO &info)
{
  info.nInput = 0;
  info.nOutput = 0;
  info.pName = mpModuleName;
}

CIQueue *CActiveFilter::MsgQ()
{
  return mpWorkQ->MsgQ();
}

EECode CActiveFilter::OnCmd(SCMD &cmd)
{
  LOGM_FATAL("TO DO\n");
  return EECode_NoImplement;
}

const TChar *CActiveFilter::GetName() const
{
  return GetModuleName();
}

void CActiveFilter::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
  LOGM_FATAL("TO DO\n");
}

//-----------------------------------------------------------------------
//
//  CActiveEngine
//
//-----------------------------------------------------------------------
CActiveEngine::CActiveEngine(const TChar *pname, TUint index)
  : inherited(pname, index)
  , mSessionID(0)
  , mpMutex(NULL)
  , mpWorkQ(NULL)
  , mpCmdQueue(NULL)
  , mpFilterMsgQ(NULL)
  , mpAppMsgQueue(NULL)
  , mpAppMsgSink(NULL)
  , mpAppMsgProc(NULL)
  , mpAppMsgContext(NULL)
{
}

CActiveEngine::~CActiveEngine()
{
}

void CActiveEngine::Delete()
{
  if (mpMutex) {
    mpMutex->Delete();
    mpMutex = NULL;
  }
  if (mpFilterMsgQ) {
    mpFilterMsgQ->Delete();
    mpFilterMsgQ = NULL;
  }
  if (mpWorkQ) {
    mpWorkQ->Delete();
    mpWorkQ = NULL;
  }
  inherited::Delete();
}

CObject *CActiveEngine::GetObject() const
{
  return (CObject *) this;
}

EECode CActiveEngine::MsgProc(SMSG &msg)
{
  EECode ret = EECode_OK;
  DASSERT(mpWorkQ);
  if (mpFilterMsgQ) {
    AUTO_LOCK(mpMutex);
    msg.sessionID = mSessionID;
    ret = mpFilterMsgQ->PutData(&msg, sizeof(msg));
    DASSERT_OK(ret);
  } else {
    LOGM_ERROR("CActiveEngine::MsgProc, NULL mpFilterMsgQ\n");
    return EECode_Error;
  }
  return ret;
}

void CActiveEngine::OnRun()
{
  LOGM_FATAL("CActiveEngine::OnRun(), shoud not comes here.\n");
}

void CActiveEngine::NewSession()
{
  AUTO_LOCK(mpMutex);
  mSessionID++;
}

bool CActiveEngine::IsSessionMsg(SMSG &msg)
{
  return msg.sessionID == mSessionID;
}

EECode CActiveEngine::Construct()
{
  if (NULL == (mpMutex = gfCreateMutex())) {
    LOGM_FATAL("CActiveEngine::Construct(), gfCreateMutex() fail\n");
    return EECode_NoMemory;
  }
  if (NULL == (mpWorkQ = CIWorkQueue::Create((IActiveObject *)this))) {
    LOGM_FATAL("CActiveEngine::Construct(), CIWorkQueue::Create fail\n");
    if (mpMutex) {
      mpMutex->Delete();
      mpMutex = NULL;
    }
    return EECode_NoMemory;
  }
  mpCmdQueue = mpWorkQ->MsgQ();
  if (NULL == (mpFilterMsgQ = CIQueue::Create(mpCmdQueue, this, sizeof(SMSG), 1))) {
    LOGM_FATAL("CActiveEngine::Construct(), CIQueue::Create fail\n");
    if (mpMutex) {
      mpMutex->Delete();
      mpMutex = NULL;
    }
    if (mpWorkQ) {
      mpWorkQ->Delete();
      mpWorkQ = NULL;
    }
    return EECode_NoMemory;
  }
  return EECode_OK;
}

EECode CActiveEngine::PostEngineMsg(SMSG &msg)
{
  DASSERT(mpFilterMsgQ);
  DASSERT(mpWorkQ);
  {
    AUTO_LOCK(mpMutex);
    msg.sessionID = mSessionID;
  }
  return mpFilterMsgQ->PutData(&msg, sizeof(SMSG));
}

EECode CActiveEngine::SetAppMsgQueue(CIQueue *p_msg_queue)
{
  mpAppMsgQueue = p_msg_queue;
  return EECode_OK;
}

EECode CActiveEngine::SetAppMsgSink(IMsgSink *pAppMsgSink)
{
  AUTO_LOCK(mpMutex);
  mpAppMsgSink = pAppMsgSink;
  return EECode_OK;
}

EECode CActiveEngine::SetAppMsgCallback(void (*MsgProc)(void *, SMSG &), void *context)
{
  AUTO_LOCK(mpMutex);
  mpAppMsgProc = MsgProc;
  mpAppMsgContext = context;
  return EECode_OK;
}

EECode CActiveEngine::PostAppMsg(SMSG &msg)
{
  //AUTO_LOCK(mpMutex);
  if (msg.sessionID == mSessionID) {
    if (mpAppMsgQueue) {
      mpAppMsgQueue->PostMsg(&msg, sizeof(SMSG));
    } else if (mpAppMsgSink) {
      mpAppMsgSink->MsgProc(msg);
    } else if (mpAppMsgProc) {
      mpAppMsgProc(mpAppMsgContext, msg);
    } else {
      LOGM_NOTICE("no app msg sink or proc in CActiveEngine::PostAppMsg\n");
    }
  } else {
    LOGM_WARN("should not comes here, not correct seesion id %d, engine's session id %d.\n", msg.sessionID, mSessionID);
  }
  return EECode_OK;
}

EECode CActiveEngine::Connect(IFilter *up, TUint uppin_index, IFilter *down, TUint downpin_index, TUint uppin_sub_index)
{
  EECode err;
  IOutputPin *pOutput;
  IInputPin *pInput;
  AUTO_LOCK(mpMutex);
  DASSERT(up && down);
  if (up && down) {
    pOutput = up->GetOutputPin(uppin_index, uppin_sub_index);
    if (NULL == pOutput) {
      LOGM_FATAL("No output pin: %p, index %d, sub index %d\n", up, uppin_index, uppin_sub_index);
      return EECode_InternalLogicalBug;
    }
    pInput = down->GetInputPin(downpin_index);
    if (NULL == pInput) {
      LOGM_FATAL("No input pin: %p, index %d\n", down, downpin_index);
      return EECode_InternalLogicalBug;
    }
    LOGM_INFO("{uppin_index %d, uppin_sub_index %d, downpin_index %d} connect start.\n", uppin_index, uppin_sub_index, downpin_index);
    err = pInput->Connect(pOutput, uppin_sub_index);
    if (EECode_OK == err) {
      err = pOutput->Connect(pInput, uppin_sub_index);
      if (EECode_OK == err) {
        LOGM_INFO("{uppin_index %d, uppin_sub_index %d, downpin_index %d} connect done.\n", uppin_index, uppin_sub_index, downpin_index);
        return EECode_OK;
      }
      LOGM_FATAL("pOutput->Connect() fail, return %d\n", err);
      err = pInput->Disconnect(uppin_sub_index);
      DASSERT_OK(err);
      return EECode_InternalLogicalBug;
    }
    LOGM_FATAL("pInput->Connect() fail, return %d\n", err);
    return EECode_InternalLogicalBug;
  }
  LOGM_FATAL("NULL pointer detected! up %p, down %p\n", up, down);
  return EECode_BadParam;
}

EECode CActiveEngine::OnCmd(SCMD &cmd)
{
  LOGM_FATAL("TO DO!\n");
  return EECode_NoImplement;
}

const TChar *CActiveEngine::GetName() const
{
  return GetModuleName();
}

//-----------------------------------------------------------------------
//
//  CScheduledFilter
//
//-----------------------------------------------------------------------
CScheduledFilter::CScheduledFilter(const TChar *pName, TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
  : inherited(pName, index)
  , mpPersistMediaConfig(pPersistMediaConfig)
  , mpEngineMsgSink(pEngineMsgSink)
  , mpCmdQueue(NULL)
  , mbRun(0)
  , mbPaused(0)
  , mbStarted(0)
  , mbSuspended(0)
  , msState(EModuleState_Invalid)
{
}

EECode CScheduledFilter::Construct()
{
  if ((mpCmdQueue = CIQueue::Create(NULL, this, sizeof(SCMD), 16)) == NULL) {
    LOG_FATAL("No memory\n");
    return EECode_NoMemory;
  }
  return EECode_OK;
}

CScheduledFilter::~CScheduledFilter()
{
}

void CScheduledFilter::Delete()
{
  if (mpCmdQueue) {
    mpCmdQueue->Delete();
    mpCmdQueue = NULL;
  }
  inherited::Delete();
}

CObject *CScheduledFilter::GetObject0() const
{
  return (CObject *) this;
}

EECode CScheduledFilter::Run()
{
  DASSERT(!mbRun);
  mbRun = 1;
  return EECode_OK;
}

EECode CScheduledFilter::Exit()
{
  return EECode_OK;
}

EECode CScheduledFilter::EventHandling(EECode err)
{
  return EECode_OK;
}

void CScheduledFilter::Pause()
{
  if (mpCmdQueue && mbRun && mbStarted) {
    SCMD cmd;
    cmd.code = ECMDType_Pause;
    cmd.needFreePExtra = 0;
    mpCmdQueue->PostMsg((const void *)&cmd, sizeof(cmd));
  }
}

void CScheduledFilter::Resume()
{
  if (mpCmdQueue && mbRun && mbStarted) {
    SCMD cmd;
    cmd.code = ECMDType_Resume;
    cmd.needFreePExtra = 0;
    mpCmdQueue->PostMsg((const void *)&cmd, sizeof(cmd));
  }
}

void CScheduledFilter::Flush()
{
  if (mpCmdQueue && mbRun && mbStarted) {
    SCMD cmd;
    cmd.code = ECMDType_Flush;
    cmd.needFreePExtra = 0;
    mpCmdQueue->SendMsg((const void *)&cmd, sizeof(cmd));
  }
}

void CScheduledFilter::ResumeFlush()
{
  if (mpCmdQueue && mbRun && mbStarted) {
    SCMD cmd;
    cmd.code = ECMDType_ResumeFlush;
    cmd.needFreePExtra = 0;
    mpCmdQueue->SendMsg((const void *)&cmd, sizeof(cmd));
  }
}

EECode CScheduledFilter::Suspend(TUint release_context)
{
  if (mpCmdQueue && mbRun && mbStarted) {
    SCMD cmd;
    cmd.code = ECMDType_Suspend;
    cmd.needFreePExtra = 0;
    return mpCmdQueue->SendMsg((const void *)&cmd, sizeof(cmd));
  } else {
    return EECode_BadState;
  }
}

EECode CScheduledFilter::ResumeSuspend(TComponentIndex input_index)
{
  if (mpCmdQueue && mbRun && mbStarted) {
    SCMD cmd;
    cmd.code = ECMDType_ResumeSuspend;
    cmd.needFreePExtra = 0;
    return mpCmdQueue->SendMsg((const void *)&cmd, sizeof(cmd));
  } else {
    return EECode_BadState;
  }
}

EECode CScheduledFilter::SendCmd(TUint cmd_code)
{
  if (DLikely(mpCmdQueue && mbRun && mbStarted)) {
    SCMD cmd;
    cmd.code = cmd_code;
    cmd.needFreePExtra = 0;
    return mpCmdQueue->SendMsg((const void *)&cmd, sizeof(cmd));
  } else {
    LOGM_FATAL("why comes here\n");
  }
  return EECode_BadState;
}

void CScheduledFilter::PostMsg(TUint cmd_code)
{
  if (DLikely(mpCmdQueue && mbRun && mbStarted)) {
    SCMD cmd;
    cmd.code = cmd_code;
    cmd.needFreePExtra = 0;
    mpCmdQueue->PostMsg((const void *)&cmd, sizeof(cmd));
  } else {
    LOGM_FATAL("why comes here\n");
  }
}

CIQueue *CScheduledFilter::MsgQ()
{
  return mpCmdQueue;
}

void CScheduledFilter::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
  LOGM_FATAL("TO DO\n");
}

void gfClearBufferMemory(IBufferPool *p_buffer_pool)
{
  if (p_buffer_pool) {
    CIBuffer *p_buffer = NULL;
    while (p_buffer_pool->GetFreeBufferCnt()) {
      p_buffer_pool->AllocBuffer(p_buffer, sizeof(CIBuffer));
      if (p_buffer->mbAllocated) {
        p_buffer->ReleaseAllocatedMemory();
      }
    }
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
        LOGM_DEBUG("current time %" DPrint64d ", next time %" DPrint64d "\n", p_clock_listener->event_time, p_clock_listener->event_time + p_clock_listener->event_duration);
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
    LOG_WARN("listener not in list?\n");
    return EECode_InternalLogicalBug;
  }
  mpClockListenerList->RemoveContent((void *)listener);
  releaseClockListener(listener);
  return EECode_OK;
}

SClockListener *CIClockReference::GuardedAddTimer(IEventListener *p_listener, EClockTimerType type, TTime time, TTime interval, TUint count)
{
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
    LOGM_PRINTF("i %d, mCurrentTime %" DPrint64d ", mbForward %d\n", i, mCurrentTime, mbForward);
    LOGM_PRINTF("p_clock_listener: type %d, event_time %" DPrint64d ", event_duration %" DPrint64d ".\n", p_clock_listener->type, p_clock_listener->event_time, p_clock_listener->event_duration);
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

TU32 CRingMemPool::IsEnoughRoom(TMemSize size)
{
  TMemSize currentFreeSize1 = 0;
  TMemSize currentFreeSize2 = 0;
  AUTO_LOCK(mpMutex);
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
          return 1;
        } else if (currentFreeSize2 >= size) {
          return 1;
        }
      }
    }
  } else if (((TMemSize)mpFreeMemStart) > ((TMemSize)mpUsedMemStart)) {
    currentFreeSize1 = mpMemoryEnd - mpFreeMemStart;
    currentFreeSize2 = mpUsedMemStart - mpMemoryBase;
    if (currentFreeSize1 >= size) {
      return 1;
    } else if (currentFreeSize2 >= size) {
      return 1;
    }
  } else {
    currentFreeSize1 = mpUsedMemStart - mpFreeMemStart;
    if (currentFreeSize1 >= size) {
      return 1;
    }
  }
  return 0;
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

TU32 CFixedMemPool::IsEnoughRoom(TMemSize size)
{
  return 1;
}

IStreamingServerManager* gfCreateStreamingServerManager(const volatile SPersistMediaConfig* pconfig, IMsgSink* pMsgSink, const CIClockReference* p_system_clock_reference)
{
    return NULL;
}

