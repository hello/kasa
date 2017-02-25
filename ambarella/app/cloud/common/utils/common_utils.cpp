/*******************************************************************************
 * common_utils.cpp
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

#include <semaphore.h>
#include <pthread.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#define NODE_SIZE       (sizeof(List) + mBlockSize)
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
            //LOG_RESOURCE(" ----delete node %p.\n", pNode);
            free(pNode);
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

//-----------------------------------------------------------------------
//
//  CISimpleQueue
//
//-----------------------------------------------------------------------
typedef struct SSimpleNode_s {
    TULong ctx;
    struct SSimpleNode_s *p_pre;
    struct SSimpleNode_s *p_next;
} SSimpleNode;

class CISimpleQueue : public ISimpleQueue
{
public:
    static ISimpleQueue *Create(TU32 num);

private:
    CISimpleQueue(TU32 num);
    EECode Construct();
    virtual ~CISimpleQueue();

public:
    virtual void Destroy();

public:
    virtual TU32 GetCnt();
    virtual void Lock();
    virtual void UnLock();

    virtual void Enqueue(TULong ctx);
    virtual TULong Dequeue();
    virtual TU32 TryDequeue(TULong &ret_ctx);

private:
    TU32 mMaxCnt;
    TU32 mCurCnt;
    SSimpleNode mHead;

    SSimpleNode *mpFreelist;

    pthread_mutex_t mMutex;
    pthread_cond_t mCondNotfull;
    pthread_cond_t mCondNotempty;
};

CISimpleQueue::CISimpleQueue(TUint num)
{
    mMaxCnt = num;
    mCurCnt = 0;
    mHead.p_next = mHead.p_pre = &mHead;
    mpFreelist = NULL;

    pthread_mutex_init(&mMutex, NULL);
    pthread_cond_init(&mCondNotempty, NULL);
    pthread_cond_init(&mCondNotfull, NULL);
}

CISimpleQueue::~CISimpleQueue()
{
    SSimpleNode *ptmp = NULL, *ptmp1 = NULL;

    ptmp1 = mHead.p_next;
    while (ptmp1 != &mHead) {
        ptmp = ptmp1;
        ptmp1 = ptmp1->p_next;
        free(ptmp);
    }
    while (mpFreelist) {
        ptmp = mpFreelist;
        mpFreelist = mpFreelist->p_next;
        free(ptmp);
    }
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCondNotempty);
    pthread_cond_destroy(&mCondNotfull);
}

ISimpleQueue *CISimpleQueue::Create(TUint num)
{
    CISimpleQueue *result = new CISimpleQueue(num);
    if (result && (EECode_OK != result->Construct())) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CISimpleQueue::Construct()
{
    return EECode_OK;
}

void CISimpleQueue::Destroy()
{
    delete this;
}

TU32 CISimpleQueue::GetCnt()
{
    return mCurCnt;
}

void CISimpleQueue::Lock()
{
    pthread_mutex_lock(&mMutex);
}

void CISimpleQueue::UnLock()
{
    pthread_mutex_unlock(&mMutex);
}

void CISimpleQueue::Enqueue(TULong ctx)
{
    SSimpleNode *ptmp = NULL;
    pthread_mutex_lock(&mMutex);

    if (mMaxCnt) {
        while (mCurCnt >= mMaxCnt) {
            pthread_cond_wait(&mCondNotfull, &mMutex);
        }
    }

    if (!mpFreelist) {
        ptmp = (SSimpleNode *)DDBG_MALLOC(sizeof(SSimpleNode), "Q1FL");
    } else {
        ptmp = mpFreelist;
        mpFreelist = mpFreelist->p_next;
    }

    ptmp->ctx = ctx;
    ptmp->p_pre = &mHead;
    ptmp->p_next = mHead.p_next;
    mHead.p_next->p_pre = ptmp;
    mHead.p_next = ptmp;
    mCurCnt++;
    if (mCurCnt < 2) {
        pthread_cond_signal(&mCondNotempty);
    }

    pthread_mutex_unlock(&mMutex);
}

TULong CISimpleQueue::Dequeue()
{
    SSimpleNode *ptmp = NULL;
    TULong ctx;

    pthread_mutex_lock(&mMutex);

    while (mCurCnt == 0) {
        pthread_cond_wait(&mCondNotempty, &mMutex);
    }

    ptmp = mHead.p_pre;
    if (ptmp == &mHead) {
        LOG_ERROR("fatal error: currupt in Dequeue.\n");
        pthread_mutex_unlock(&mMutex);
        return 0;
    }

    ptmp->p_pre->p_next = &mHead;
    mHead.p_pre = ptmp->p_pre;
    mCurCnt--;

    ptmp->p_next = mpFreelist;
    mpFreelist = ptmp;
    ctx = ptmp->ctx;

    if ((mMaxCnt) && ((mCurCnt + 1) >= mMaxCnt)) {
        pthread_cond_signal(&mCondNotfull);
    }

    pthread_mutex_unlock(&mMutex);

    return ctx;
}

TU32 CISimpleQueue::TryDequeue(TULong &ret)
{
    SSimpleNode *ptmp = NULL;

    pthread_mutex_lock(&mMutex);

    if (mCurCnt == 0) {
        ret = 0;
        pthread_mutex_unlock(&mMutex);
        return 0;
    }

    ptmp = mHead.p_pre;
    if (ptmp == &mHead) {
        LOG_ERROR("fatal error: CISimpleQueue currupt in TryDequeue.\n");
        pthread_mutex_unlock(&mMutex);
        return 0;
    }

    ptmp->p_pre->p_next = &mHead;
    mHead.p_pre = ptmp->p_pre;
    mCurCnt--;

    ptmp->p_next = mpFreelist;
    mpFreelist = ptmp;
    ret = ptmp->ctx;

    if ((mMaxCnt) && ((mCurCnt + 1) >= mMaxCnt)) {
        pthread_cond_signal(&mCondNotfull);
    }

    pthread_mutex_unlock(&mMutex);

    return 1;
}

ISimpleQueue *gfCreateSimpleQueue(TU32 num)
{
    return CISimpleQueue::Create(num);
}

//-----------------------------------------------------------------------
//
//  CIConditionSlot
//
//-----------------------------------------------------------------------
CIConditionSlot *CIConditionSlot::Create(IMutex *p_mutex)
{
    CIConditionSlot *result = new CIConditionSlot(p_mutex);
    if (result && (EECode_OK != result->Construct())) {
        delete result;
        result = NULL;
    }
    return result;
}

void CIConditionSlot::Delete()
{
    delete this;
}

CIConditionSlot::CIConditionSlot(IMutex *p_mutex)
    : mSessionNumber(0)
    , mnWaiters(0)
    , mpMutex(p_mutex)
    , mpCondition(NULL)
    , mpReplyContext(0)
    , mReplyType(0)
    , mnReplyCount(0)
{

}

EECode CIConditionSlot::Construct()
{
    mpCondition = gfCreateCondition();
    if (DUnlikely(!mpCondition)) {
        LOG_FATAL("gfCreateCondition fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CIConditionSlot::~CIConditionSlot()
{
    if (mpCondition) {
        mpCondition->Delete();
        mpCondition = NULL;
    }
}

void CIConditionSlot::Reset()
{
    mpReplyContext = 0;
    mReplyType = 0;
    mnReplyCount = 0;
    mReplyID = DInvalidUniqueID;
    mReplyCode = EECode_OK;
}

void CIConditionSlot::SetReplyCode(EECode ret_code)
{
    mReplyCode = ret_code;
}

void CIConditionSlot::GetReplyCode(EECode &ret_code) const
{
    ret_code = mReplyCode;
}

void CIConditionSlot::SetReplyContext(TPointer reply_context, TU32 reply_type, TU32 reply_count, TUniqueID reply_id)
{
    mpReplyContext = reply_context;
    mReplyType = reply_type;
    mnReplyCount = reply_count;
    mReplyID = reply_id;
}

void CIConditionSlot::GetReplyContext(TPointer &reply_context, TU32 &reply_type, TU32 &reply_count, TUniqueID &reply_id) const
{
    reply_context = mpReplyContext;
    reply_type = mReplyType;
    reply_count = mnReplyCount;
    reply_id = mReplyID;
}

EECode CIConditionSlot::Wait()
{
    mnWaiters ++;
    mpCondition->Wait(mpMutex);

    return EECode_OK;
}

void CIConditionSlot::Signal()
{
    if (mnWaiters > 1) {
        mnWaiters --;
    }
    mpCondition->Signal();
}

//void CIConditionSlot::SignalAll()
//{
//    mnWaiters = 0;
//    mpCondition->SignalAll();
//}

void CIConditionSlot::SetSessionNumber(TU32 session_number)
{
    mSessionNumber = session_number;
}

TU32 CIConditionSlot::GetSessionNumber() const
{
    return mSessionNumber;
}

void CIConditionSlot::SetCheckField(TU32 check_field)
{
    mCheckField = check_field;
}

TU32 CIConditionSlot::GetCheckField() const
{
    return mCheckField;
}

void CIConditionSlot::SetTime(SDateTime *p_time)
{
    if (DLikely(p_time)) {
        mTime = *p_time;
    } else {
        LOG_FATAL("NULL p_time\n");
    }
}

void CIConditionSlot::GetTime(SDateTime *&p_time) const
{
    p_time = (SDateTime *) &mTime;
}

void gfMovelisttolist(SListBase *dst, SListBase *src)
{
    if (src->first == NULL) {
        return;
    }

    if (dst->first == NULL) {
        dst->first = src->first;
        dst->last = src->last;
    } else {
        ((SLink *) dst->last)->next = (SLink *) src->first;
        ((SLink *) src->first)->prev = (SLink *) dst->last;
        dst->last = src->last;
    }
    src->first = src->last = NULL;
}

void gfLinkAddHead(SListBase *listbase, void *vlink)
{
    SLink *link = (SLink *) vlink;

    if (link == NULL) {
        return;
    }

    link->next = (SLink *) listbase->first;
    link->prev = NULL;

    if (listbase->first) {
        ((SLink *)listbase->first)->prev = link;
    }
    if (listbase->last == NULL) {
        listbase->last = link;
    }
    listbase->first = link;
}

void gfLinkAddTail(SListBase *listbase, void *vlink)
{
    SLink *link = (SLink *) vlink;

    if (link == NULL) {
        return;
    }

    link->next = NULL;
    link->prev = (SLink *) listbase->last;

    if (listbase->last) {
        ((SLink *)listbase->last)->next = link;
    }
    if (listbase->first == NULL) {
        listbase->first = link;
    }
    listbase->last = link;
}

void gfRemLink(SListBase *listbase, void *vlink)
{
    SLink *link = (SLink *) vlink;

    if (link == NULL) {
        return;
    }

    if (link->next) {
        link->next->prev = link->prev;
    }
    if (link->prev) {
        link->prev->next = link->next;
    }

    if (listbase->last == link) {
        listbase->last = link->prev;
    }
    if (listbase->first == link) {
        listbase->first = link->next;
    }
}

TInt gfLinkFindIndex(const SListBase *listbase, const void *vlink)
{
    SLink *link = NULL;
    TInt number = 0;

    if (vlink == NULL) {
        return -1;
    }

    link = (SLink *) listbase->first;
    while (link) {
        if (link == vlink) {
            return number;
        }

        number++;
        link = link->next;
    }

    return -1;
}

bool gfRemLinkSafe(SListBase *listbase, void *vlink)
{
    if (gfLinkFindIndex(listbase, vlink) != -1) {
        gfRemLink(listbase, vlink);
        return true;
    } else {
        return false;
    }
}

TInt gfLinkCount(const SListBase *listbase)
{
    SLink *link;
    TInt count = 0;

    for (link = (SLink *) listbase->first; link; link = link->next) {
        count++;
    }

    return count;
}

void *gfLinkPopHead(SListBase *listbase)
{
    SLink *link;
    if ((link = (SLink *) listbase->first)) {
        gfRemLink(listbase, link);
    }
    return link;
}

void *gfLinkPopTail(SListBase *listbase)
{
    SLink *link;
    if ((link = (SLink *) listbase->last)) {
        gfRemLink(listbase, link);
    }
    return link;
}

#ifdef BUILD_OS_WINDOWS
#define DSEP '\\'
#define DALTSEP '/'
#else
#define DSEP '/'
#define DALTSEP '\\'
#endif

void gfJoinDirFile(TChar *dst, const TMemSize maxlen, const TChar *dir, const TChar *file)
{
    TMemSize dirlen = strnlen(dir, maxlen);

    if (dirlen == maxlen) {
        memcpy(dst, dir, dirlen);
        dst[dirlen - 1] = '\0';
        return;
    } else {
        memcpy(dst, dir, dirlen + 1);
    }

    if (dirlen + 1 >= maxlen) {
        return;
    }

    if ((dirlen > 0) && (dst[dirlen - 1] != DSEP)) {
        dst[dirlen++] = DSEP;
        dst[dirlen] = '\0';
    }

    if (dirlen >= maxlen) {
        return;
    }

    strncpy(dst + dirlen, file, maxlen - dirlen);
}

#if 0
EECode gfDecodingBase64(TU8 *p_src, TU8 *p_dest, TMemSize src_size, TMemSize &output_size)
{
    TMemSize zero_count = 0;

    if (DUnlikely((!p_src) || (!p_dest) || (!src_size))) {
        LOG_FATAL("p_src %p, p_dest %p, src_size %ld\n", p_src, p_dest, src_size);
        return EECode_BadParam;
    }

    output_size = 0;

    //LOG_NOTICE("%s\n", p_src);

    while (src_size > 3) {
        LOG_NOTICE("src_size %ld\n", src_size);
        LOG_NOTICE("input %c %c %c %c\n", p_src[0], p_src[1], p_src[2], p_src[3]);
        LOG_NOTICE("input %02x %02x %02x %02x\n", p_src[0], p_src[1], p_src[2], p_src[3]);
#if 1
        p_dest[0] = ((p_src[0] << 2) & 0xfc) | ((p_src[1] >> 4) & 0x3);
        p_dest[1] = ((p_src[1] << 4) & 0xf0) | ((p_src[2] >> 2) & 0x0f);
        p_dest[2] = (p_src[3] & 0x3f) | ((p_src[2] << 6) & 0xc0);
#else
        p_dest[0] = ((p_src[0] << 2) & 0xfc) | ((p_src[1] >> 4) & 0x3);
        p_dest[1] = ((p_src[1] << 4) & 0xf0) | ((p_src[2] >> 2) & 0x0f);
        p_dest[2] = (p_src[3] & 0x3f) | ((p_src[2] << 6) & 0xc0);
#endif
        LOG_NOTICE("output %02x %02x %02x\n", p_dest[0], p_dest[1], p_dest[2]);

        src_size -= 4;
        output_size += 3;
        p_dest += 3;
        p_src += 4;
    }

    DASSERT(!src_size);
    LOG_NOTICE("%c %c %c %c\n", p_src[-3], p_src[-2], p_src[-1], p_src[0]);

    p_src --;
    while ('=' == (*p_src)) {
        p_src --;
        zero_count ++;
    }

    output_size -= zero_count;
    LOG_NOTICE("output_size %ld, zero count %ld\n", output_size, zero_count);

    return EECode_OK;
}
#endif

void gfEncodingBase16(TChar *out, const TU8 *in, TInt in_size)
{
    const TChar b16[32] =
        "0123456789ABCDEF";

    while (in_size > 0) {
        out[0] = b16[(in[0] >> 4) & 0x0f];
        out[1] = b16[in[0] & 0x0f];
        out += 2;
        in += 1;
        in_size --;
    }
}

void gfDecodingBase16(TU8 *out, const TU8 *in, TInt in_size)
{
    TChar hex[8] = {0};
    TU32 value = 0;

    while (in_size > 1) {
        hex[0] = in[0];
        hex[1] = in[1];
        sscanf(hex, "%x", &value);
        out[0] = value;
        out ++;
        in += 2;
        in_size -= 2;
    }
}

/* ---------------- private code */
static const TU8 _gBase64Map2[256] = {
    0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff,

    0x3e, 0xff, 0xff, 0xff, 0x3f, 0x34, 0x35, 0x36,
    0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff,
    0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1a, 0x1b,
    0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
    0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33,

    0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

#define DBASE64_DEC_STEP(i) do { \
        bits = _gBase64Map2[in[i]]; \
        if (bits & 0x80) \
            goto out ## i; \
        v = i ? (v << 6) + bits : bits; \
    } while(0)

TInt gfDecodingBase64(TU8 *out, const TU8 *in_str, TInt out_size)
{
    TU8 *dst = out;
    TU8 *end = out + out_size;
    // no sign extension
    const TU8 *in = in_str;
    unsigned bits = 0xff;
    unsigned v;

    while (end - dst > 3) {
        DBASE64_DEC_STEP(0);
        DBASE64_DEC_STEP(1);
        DBASE64_DEC_STEP(2);
        DBASE64_DEC_STEP(3);
        // Using AV_WB32 directly confuses compiler
        //v = DBSWAP32C(v << 8);
        dst[2] = (v & 0xff);
        dst[1] = ((v >> 8) & 0xff);
        dst[0] = ((v >> 16) & 0xff);
        dst += 3;
        in += 4;
    }
    if (end - dst) {
        DBASE64_DEC_STEP(0);
        DBASE64_DEC_STEP(1);
        DBASE64_DEC_STEP(2);
        DBASE64_DEC_STEP(3);
        *dst++ = v >> 16;
        if (end - dst)
        { *dst++ = v >> 8; }
        if (end - dst)
        { *dst++ = v; }
        in += 4;
    }
    while (1) {
        DBASE64_DEC_STEP(0);
        in++;
        DBASE64_DEC_STEP(0);
        in++;
        DBASE64_DEC_STEP(0);
        in++;
        DBASE64_DEC_STEP(0);
        in++;
    }

out3:
    *dst++ = v >> 10;
    v <<= 2;
out2:
    *dst++ = v >> 4;
out1:
out0:
    return bits & 1 ? -1 : dst - out;
}

TChar *gfEncodingBase64(TChar *out, TInt out_size, const TU8 *in, TInt in_size)
{
    static const TChar b64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    TChar *ret, *dst;
    unsigned i_bits = 0;
    TInt i_shift = 0;
    TInt bytes_remaining = in_size;

    ret = dst = out;
    while (bytes_remaining > 3) {
        i_bits = ((in[0] << 16) | (in[1] << 8) | in[2]);
        in += 3; bytes_remaining -= 3;
        *dst++ = b64[ i_bits >> 18        ];
        *dst++ = b64[(i_bits >> 12) & 0x3F];
        *dst++ = b64[(i_bits >> 6) & 0x3F];
        *dst++ = b64[(i_bits) & 0x3F];
    }
    i_bits = 0;
    while (bytes_remaining) {
        i_bits = (i_bits << 8) + *in++;
        bytes_remaining--;
        i_shift += 8;
    }
    while (i_shift > 0) {
        *dst++ = b64[(i_bits << 6 >> i_shift) & 0x3f];
        i_shift -= 6;
    }
    while ((dst - ret) & 3) {
        *dst++ = '=';
    }
    *dst = '\0';

    return ret;
}

//version related
#define DCommonVesionMajor 0
#define DCommonVesionMinor 6
#define DCommonVesionPatch 11
#define DCommonVesionYear 2015
#define DCommonVesionMonth 9
#define DCommonVesionDay 7

void gfGetCommonVersion(TU32 &major, TU32 &minor, TU32 &patch, TU32 &year, TU32 &month, TU32 &day)
{
    major = DCommonVesionMajor;
    minor = DCommonVesionMinor;
    patch = DCommonVesionPatch;

    year = DCommonVesionYear;
    month = DCommonVesionMonth;
    day = DCommonVesionDay;
}

TU16 gfEndianNativeToNetworkShort(TU16 input)
{
    TU8 *p = (TU8 *) &input;
    return ((((TU16)p[0]) << 8) | ((TU16)p[1]));
}

void gfFillTLV16Struct(TU8 *p_payload, const ETLV16Type type[], const TU16 length[], void *value[], TInt count)
{
    TU8 *p_value = NULL;
    DASSERT(p_payload);
    for (TInt i = 0; i < count; i++) {
        STLV16Header *p_header = (STLV16Header *)p_payload;
        p_header->type = gfEndianNativeToNetworkShort(type[i]);
        p_header->length = gfEndianNativeToNetworkShort(length[i]);
        p_value = p_payload + sizeof(STLV16Header);
        switch (type[i]) {
                //string
            case ETLV16Type_String:
            case ETLV16Type_SourceDeviceDataServerAddress:
            case ETLV16Type_SourceDeviceStreamingTag:
            case ETLV16Type_DeviceNickName:
            case ETLV16Type_AccountName:
            case ETLV16Type_DeviceName:
                snprintf((TChar *)p_value, length[i] + 1, "%s", (TChar *)value[i]);
                break;

                //TU64
            case ETLV16Type_AccountID:
                DMAP_UNIQUEID_TO_MEM(p_value, *(TUniqueID *)value[i]);
                break;
                //TU64
            case ETLV16Type_DynamicInput:
                DMAP_TU64_TO_MEM(p_value, *(TU64 *)value[i]);
                break;

                //id list
            case ETLV16Type_IDList:
            case ETLV16Type_UserFriendList:
            case ETLV16Type_UserOwnedDeviceList:
            case ETLV16Type_UserSharedDeviceList: {
                    TUniqueID *p_id_list = (TUniqueID *)value[i];
                    TU32 step = sizeof(TUniqueID);
                    TU32 count = length[i] / step;
                    for (TU32 j = 0; j < count; j++) {
                        DMAP_UNIQUEID_TO_MEM(p_value, p_id_list[j]);
                        p_value += step;
                    }
                }
                break;

                //TU16
            case ETLV16Type_SourceDeviceUploadingPort:
            case ETLV16Type_SourceDeviceStreamingPort: {
                    TSocketPort port = *(TSocketPort *)value[i];
                    TU32 step = sizeof(TSocketPort);
                    DASSERT(step == length[i]);
                    for (TU32 j = 0; j < step; j++) {
                        p_value[j] = (port >> 8 * (step - 1 - j)) & 0xff;
                    }
                }
                break;

                //TU8
            case ETLV16Type_DeviceStatus: {
                    TU8 on_line = *(TU8 *)value[i];
                    p_value[0] = on_line;
                }
                break;

                //TU32
            case ETLV16Type_DynamicCode:
            case ETLV16Type_DeviceStorageCapacity: {
                    TU32 capacity = *(TU32 *)value[i];
                    TU32 step = sizeof(capacity);
                    DASSERT(step == length[i]);
                    for (TU32 j = 0; j < step; j++) {
                        p_value[j] = (capacity >> 8 * (step - 1 - j)) & 0xff;
                    }
                }
                break;

            default:
                LOG_FATAL("Unknown type: 0x%x, check me!\n", type[i]);
                break;
        }
        p_payload = p_value + length[i];
    }

    return;
}

void gfParseTLV16Struct(TU8 *p_payload, ETLV16Type type[], TU16 length[], void *value[], TInt count)
{
    DASSERT(p_payload);
    TU8 *p_value = NULL;
    TU16 id_length = 0;
    for (TInt i = 0; i < count; i++) {
        STLV16Header *p_header = (STLV16Header *)p_payload;
        id_length = length[i];//only used when type is ID List
        type[i] = (ETLV16Type)(gfEndianNativeToNetworkShort(p_header->type));
        length[i] = gfEndianNativeToNetworkShort(p_header->length);
        p_value = p_payload + sizeof(STLV16Header);
        switch (type[i]) {
            case ETLV16Type_String:
            case ETLV16Type_SourceDeviceDataServerAddress:
            case ETLV16Type_SourceDeviceStreamingTag:
            case ETLV16Type_DeviceNickName:
            case ETLV16Type_AccountName:
            case ETLV16Type_DeviceName:
                value[i] = (void *)p_value;
                break;

            case ETLV16Type_AccountID:
                DMAP_MEM_TO_UNIQUEID(p_value, *(TUniqueID *)value[i]);
                break;

            case ETLV16Type_DynamicInput:
                DMAP_MEM_TO_TU64(p_value, *(TU64 *)value[i]);
                break;

            case ETLV16Type_IDList:
            case ETLV16Type_UserFriendList:
            case ETLV16Type_UserOwnedDeviceList:
            case ETLV16Type_UserSharedDeviceList: {
                    TU8 *p_id = (TU8 *)value[i];
                    if (p_id == NULL) {
                        p_id = (TU8 *) DDBG_MALLOC(length[i], "C0PT");
                        value[i] = (void *)p_id;
                    } else if (id_length < length[i]) {
                        free(p_id);
                        p_id = (TU8 *) DDBG_MALLOC(length[i], "C0PT");
                        value[i] = (void *)p_id;
                    }

                    if (!p_id) {
                        LOG_FATAL("gfParseTLV16Struct, alloc fail\n");
                        value[i] = NULL;
                        break;
                    }
                    TU32 step = sizeof(TUniqueID);
                    TU8 *p_end = p_value + length[i];
                    while (p_value < p_end) {
                        DMAP_MEM_TO_UNIQUEID(p_value, *(TUniqueID *)p_id);
                        p_value += step;
                        p_id += step;
                    }
                }
                break;

            case ETLV16Type_SourceDeviceUploadingPort:
            case ETLV16Type_SourceDeviceStreamingPort: {
                    TSocketPort port = 0;
                    TU32 step = sizeof(TSocketPort);
                    for (TU32 j = 0; j < step; j++) {
                        port |= (p_value[j] & 0xff) << 8 * (step - 1 - j);
                    }
                    *(TSocketPort *)value[i] = port;
                }
                break;

                //TU8
            case ETLV16Type_DeviceStatus: {
                    TU8 on_line = p_value[0];
                    *(TU8 *)value[i] = on_line;
                }
                break;

                //TU32
            case ETLV16Type_DynamicCode:
            case ETLV16Type_DeviceStorageCapacity: {
                    TU32 capacity = 0;
                    TU32 step = sizeof(capacity);
                    for (TU32 j = 0; j < step; j++) {
                        capacity |= (p_value[j] & 0xff) << 8 * (step - 1 - j);
                    }
                    *(TU32 *)value[i] = capacity;
                }
                break;

            default:
                LOG_FATAL("Unknown type: 0x%x, check me!\n", type[i]);
                break;
        }
        p_payload = p_value + length[i];
    }

    return;
}

TU8 *gfGetNextTLV8(TU8 &type, TU8 &length, TU8 *&p_next_unit, TU32 &remain_size)
{
    if (DUnlikely(!p_next_unit || (remain_size < 2))) {
        LOG_FATAL("corrupted tlv8, %p, %d\n", p_next_unit, remain_size);
        return NULL;
    }

    TU8 *p_payload = p_next_unit + 2;
    type = p_next_unit[0];
    length = p_next_unit[1];

    if (DUnlikely(remain_size < ((TU32)2 + length))) {
        LOG_FATAL("corrupted tlv8, remain_size %d, expected %d\n", remain_size, 2 + length);
        return NULL;
    }

    remain_size -= 2 + length;
    p_next_unit += 2 + length;

    return p_payload;
}

TU8 *gfGenerateAACExtraData(TU32 samplerate, TU32 channel_number, TU32 &size)
{
    SSimpleAudioSpecificConfig *p_simple_header = NULL;

    size = 2;
    p_simple_header = (SSimpleAudioSpecificConfig *) DDBG_MALLOC((size + 3) & (~3), "GAAE");
    p_simple_header->audioObjectType = eAudioObjectType_AAC_LC;//hard code here
    switch (samplerate) {
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
            LOG_ERROR("NOT support sample rate (%d) here.\n", samplerate);
            break;
    }
    p_simple_header->samplingFrequencyIndex_high = samplerate >> 1;
    p_simple_header->samplingFrequencyIndex_low = samplerate & 0x1;
    p_simple_header->channelConfiguration = channel_number;
    p_simple_header->bitLeft = 0;

    return (TU8 *)p_simple_header;
}

EECode gfGetH264Extradata(TU8 *data_base, TU32 data_size, TU8 *&p_extradata, TU32 &extradata_size)
{
    TU8 has_sps = 0, has_pps = 0;

    TU8 *ptr = data_base, *ptr_end = data_base + data_size;
    TUint nal_type = 0;

    if (DUnlikely((NULL == ptr) || (!data_size))) {
        LOG_FATAL("NULL pointer(%p) or zero size(%d)\n", data_base, data_size);
        return EECode_BadParam;
    }

    p_extradata = NULL;
    extradata_size = 0;

    while (ptr < ptr_end) {
        if (*ptr == 0x00) {
            if (*(ptr + 1) == 0x00) {
                if (*(ptr + 2) == 0x00) {
                    if (*(ptr + 3) == 0x01) {
                        nal_type = ptr[4] & 0x1F;
                        if (ENalType_IDR == nal_type) {
                            DASSERT(has_sps);
                            DASSERT(has_pps);
                            DASSERT(p_extradata);
                            extradata_size = (TU32)(ptr - p_extradata);
                            return EECode_OK;
                        } else if (ENalType_SPS == nal_type) {
                            has_sps = 1;
                            p_extradata = ptr;
                            ptr += 3;
                        } else if (ENalType_PPS == nal_type) {
                            DASSERT(has_sps);
                            DASSERT(p_extradata);
                            has_pps = 1;
                            ptr += 3;
                        } else if (has_sps && has_pps) {
                            DASSERT(p_extradata);
                            extradata_size = (TU32)(ptr - p_extradata);
                            return EECode_OK;
                        }
                    }
                } else if (*(ptr + 2) == 0x01) {
                    nal_type = ptr[3] & 0x1F;
                    if (ENalType_IDR == nal_type) {
                        DASSERT(has_sps);
                        DASSERT(has_pps);
                        DASSERT(p_extradata);
                        extradata_size = (TU32)(ptr - p_extradata);
                        return EECode_OK;
                    } else if (ENalType_SPS == nal_type) {
                        has_sps = 1;
                        p_extradata = ptr;
                        ptr += 2;
                    } else if (ENalType_PPS == nal_type) {
                        DASSERT(has_sps);
                        DASSERT(p_extradata);
                        has_pps = 1;
                        ptr += 2;
                    } else if (has_sps && has_pps) {
                        DASSERT(p_extradata);
                        extradata_size = (TU32)(ptr - p_extradata);
                        return EECode_OK;
                    }
                }
            }
        }
        ++ptr;
    }

    if (p_extradata) {
        extradata_size = (TU32)(data_base + data_size - p_extradata);
        return EECode_OK;
    }

    return EECode_NotExist;
}

EECode gfGetH264SPSPPS(TU8 *data_base, TU32 data_size, TU8 *&p_sps, TU32 &sps_size, TU8 *&p_pps, TU32 &pps_size)
{
    TU8 has_sps = 0, has_pps = 0;

    TU8 *ptr = data_base, *ptr_end = data_base + data_size;
    TUint nal_type = 0;

    if (DUnlikely((NULL == ptr) || (!data_size))) {
        LOG_FATAL("NULL pointer(%p) or zero size(%d)\n", data_base, data_size);
        return EECode_BadParam;
    }

    while (ptr < ptr_end) {
        if (*ptr == 0x00) {
            if (*(ptr + 1) == 0x00) {
                if (*(ptr + 2) == 0x00) {
                    if (*(ptr + 3) == 0x01) {
                        nal_type = ptr[4] & 0x1F;
                        if (ENalType_SPS == nal_type) {
                            has_sps = 1;
                            p_sps = ptr;
                            ptr += 3;
                        } else if (ENalType_PPS == nal_type) {
                            DASSERT(has_sps);
                            DASSERT(p_sps);
                            p_pps = ptr;
                            has_pps = 1;
                            ptr += 3;
                        } else if (has_sps && has_pps) {
                            sps_size = (TU32)(p_pps - p_sps);
                            pps_size = (TU32)(ptr - p_pps);
                            return EECode_OK;
                        }
                    }
                } else if (*(ptr + 2) == 0x01) {
                    nal_type = ptr[3] & 0x1F;
                    if (ENalType_SPS == nal_type) {
                        has_sps = 1;
                        p_sps = ptr;
                        ptr += 2;
                    } else if (ENalType_PPS == nal_type) {
                        DASSERT(has_sps);
                        has_pps = 1;
                        p_pps = ptr;
                        ptr += 2;
                    } else if (has_sps && has_pps) {
                        sps_size = (TU32)(p_pps - p_sps);
                        pps_size = (TU32)(ptr - p_pps);
                        return EECode_OK;
                    }
                }
            }
        }
        ++ptr;
    }

    if (has_sps && has_pps) {
        sps_size = (TU32)(p_pps - p_sps);
        pps_size = (TU32)(ptr - p_pps);
        return EECode_OK;
    }

    return EECode_NotExist;
}

EECode gfGetH265Extradata(TU8 *data_base, TU32 data_size, TU8 *&p_extradata, TU32 &extradata_size)
{
    TU8 has_vps = 0, has_sps = 0, has_pps = 0;

    TU8 *ptr = data_base, *ptr_end = data_base + data_size;
    TUint nal_type = 0;

    if (DUnlikely((NULL == ptr) || (!data_size))) {
        LOG_FATAL("NULL pointer(%p) or zero size(%d)\n", data_base, data_size);
        return EECode_BadParam;
    }

    p_extradata = NULL;
    extradata_size = 0;

    while (ptr < ptr_end) {
        if (*ptr == 0x00) {
            if (*(ptr + 1) == 0x00) {
                if (*(ptr + 2) == 0x00) {
                    if (*(ptr + 3) == 0x01) {
                        nal_type = (ptr[4] >> 1) & 0x3F;
                        if ((EHEVCNalType_IDR_W_RADL == nal_type) || (EHEVCNalType_IDR_N_LP == nal_type)) {
                            DASSERT(has_vps);
                            DASSERT(has_sps);
                            DASSERT(has_pps);
                            DASSERT(p_extradata);
                            extradata_size = (TU32)(ptr - p_extradata);
                            return EECode_OK;
                        } else if (EHEVCNalType_VPS == nal_type) {
                            has_vps = 1;
                            p_extradata = ptr;
                            ptr += 3;
                        } else if (EHEVCNalType_SPS == nal_type) {
                            DASSERT(has_vps);
                            DASSERT(p_extradata);
                            has_sps = 1;
                            ptr += 3;
                        } else if (EHEVCNalType_PPS == nal_type) {
                            DASSERT(has_vps);
                            DASSERT(has_sps);
                            DASSERT(p_extradata);
                            has_pps = 1;
                            ptr += 3;
                        } else if (has_vps && has_sps && has_pps) {
                            DASSERT(p_extradata);
                            extradata_size = (TU32)(ptr - p_extradata);
                            return EECode_OK;
                        }
                    }
                } else if (*(ptr + 2) == 0x01) {
                    nal_type = (ptr[3] >> 1) & 0x3F;
                    if ((EHEVCNalType_IDR_W_RADL == nal_type) || (EHEVCNalType_IDR_N_LP == nal_type)) {
                        DASSERT(has_vps);
                        DASSERT(has_sps);
                        DASSERT(has_pps);
                        DASSERT(p_extradata);
                        extradata_size = (TU32)(ptr - p_extradata);
                        return EECode_OK;
                    } else if (EHEVCNalType_VPS == nal_type) {
                        has_vps = 1;
                        p_extradata = ptr;
                        ptr += 2;
                    } else if (EHEVCNalType_SPS == nal_type) {
                        DASSERT(has_vps);
                        has_sps = 1;
                        p_extradata = ptr;
                        ptr += 2;
                    } else if (EHEVCNalType_PPS == nal_type) {
                        DASSERT(has_vps);
                        DASSERT(has_sps);
                        DASSERT(p_extradata);
                        has_pps = 1;
                        ptr += 2;
                    } else if (has_vps && has_sps && has_pps) {
                        DASSERT(p_extradata);
                        extradata_size = (TU32)(ptr - p_extradata);
                        return EECode_OK;
                    }
                }
            }
        }
        ++ptr;
    }

    if (p_extradata) {
        extradata_size = (TU32)(data_base + data_size - p_extradata);
        return EECode_OK;
    }

    return EECode_NotExist;
}

EECode gfGetH265VPSSPSPPS(TU8 *data_base, TU32 data_size, TU8 *&p_vps, TU32 &vps_size, TU8 *&p_sps, TU32 &sps_size, TU8 *&p_pps, TU32 &pps_size)
{
    TU8 has_vps = 0, has_sps = 0, has_pps = 0;

    TU8 *ptr = data_base, *ptr_end = data_base + data_size;
    TUint nal_type = 0;

    if (DUnlikely((NULL == ptr) || (!data_size))) {
        LOG_FATAL("NULL pointer(%p) or zero size(%d)\n", data_base, data_size);
        return EECode_BadParam;
    }

    while (ptr < ptr_end) {
        if (*ptr == 0x00) {
            if (*(ptr + 1) == 0x00) {
                if (*(ptr + 2) == 0x00) {
                    if (*(ptr + 3) == 0x01) {
                        nal_type = (ptr[4] >> 1) & 0x3F;
                        if (EHEVCNalType_VPS == nal_type) {
                            has_vps = 1;
                            p_vps = ptr;
                            ptr += 3;
                        } else if (EHEVCNalType_SPS == nal_type) {
                            DASSERT(has_vps);
                            p_sps = ptr;
                            has_sps = 1;
                            ptr += 3;
                        } else if (EHEVCNalType_PPS == nal_type) {
                            DASSERT(has_vps);
                            DASSERT(has_sps);
                            p_pps = ptr;
                            has_pps = 1;
                            ptr += 3;
                        } else if (has_vps && has_sps && has_pps) {
                            vps_size = (TU32)(p_sps - p_vps);
                            sps_size = (TU32)(p_pps - p_sps);
                            pps_size = (TU32)(ptr - p_pps);
                            return EECode_OK;
                        }
                    }
                } else if (*(ptr + 2) == 0x01) {
                    nal_type = (ptr[3] >> 1) & 0x3F;
                    if (EHEVCNalType_VPS == nal_type) {
                        has_vps = 1;
                        p_vps = ptr;
                        ptr += 2;
                    } else if (EHEVCNalType_SPS == nal_type) {
                        DASSERT(has_vps);
                        has_sps = 1;
                        p_sps = ptr;
                        ptr += 2;
                    } else if (EHEVCNalType_PPS == nal_type) {
                        DASSERT(has_vps);
                        DASSERT(has_sps);
                        has_pps = 1;
                        p_pps = ptr;
                        ptr += 2;
                    } else if (has_vps && has_sps && has_pps) {
                        vps_size = (TU32)(p_sps - p_vps);
                        sps_size = (TU32)(p_pps - p_sps);
                        pps_size = (TU32)(ptr - p_pps);
                        return EECode_OK;
                    }
                }
            }
        }
        ++ptr;
    }

    if (has_vps && has_sps && has_pps) {
        vps_size = (TU32)(p_sps - p_vps);
        sps_size = (TU32)(p_pps - p_sps);
        pps_size = (TU32)(ptr - p_pps);
        return EECode_OK;
    }

    return EECode_NotExist;
}

TU8 *gfNALUFindFirstAVCSliceHeader(TU8 *p, TU32 len)
{
    if (DUnlikely(NULL == p)) {
        return NULL;
    }

    while (len) {
        if (*p == 0x00) {
            if (*(p + 1) == 0x00) {
                if (*(p + 2) == 0x00) {
                    if (*(p + 3) == 0x01) {
                        if (((*(p + 4)) & 0x1F) <= ENalType_IDR) {
                            return p;
                        }
                    }
                } else if (*(p + 2) == 0x01) {
                    if (((*(p + 3)) & 0x1F) <= ENalType_IDR) {
                        return p;
                    }
                }
            }
        }
        ++ p;
        len --;
    }
    return NULL;
}

TU8 *gfNALUFindFirstAVCSliceHeaderType(TU8 *p, TU32 len, TU8 &nal_type)
{
    if (DUnlikely(NULL == p)) {
        return NULL;
    }

    while (len) {
        if (*p == 0x00) {
            if (*(p + 1) == 0x00) {
                if (*(p + 2) == 0x00) {
                    if (*(p + 3) == 0x01) {
                        if (((*(p + 4)) & 0x1F) <= ENalType_IDR) {
                            nal_type = (*(p + 4)) & 0x1F;
                            return p;
                        }
                    }
                } else if (*(p + 2) == 0x01) {
                    if (((*(p + 3)) & 0x1F) <= ENalType_IDR) {
                        nal_type = (*(p + 3)) & 0x1F;
                        return p;
                    }
                }
            }
        }
        ++ p;
        len --;
    }

    return NULL;
}

TU32 gfMethNextP2(TU32 x)
{
    x -= 1;
    x |= (x >> 16);
    x |= (x >> 8);
    x |= (x >> 4);
    x |= (x >> 2);
    x |= (x >> 1);
    x += 1;
    return x;
}

TU32 gfSimpleHash(TU32 val)
{
    TU32 key;
    key = val;
    key += ~(key << 16);
    key ^= (key >> 5);
    key += (key << 3);
    key ^= (key >> 13);
    key += ~(key << 9);
    key ^= (key >> 17);
    return key % 257;
}

bool gfRctfInsideRctf(SRectF *rct_a, const SRectF *rct_b)
{
    return ((rct_a->xmin <= rct_b->xmin) &&
            (rct_a->xmax >= rct_b->xmax) &&
            (rct_a->ymin <= rct_b->ymin) &&
            (rct_a->ymax >= rct_b->ymax));
}

bool gfRctiInsideRcti(SRectI *rct_a, const SRectI *rct_b)
{
    return ((rct_a->xmin <= rct_b->xmin) &&
            (rct_a->xmax >= rct_b->xmax) &&
            (rct_a->ymin <= rct_b->ymin) &&
            (rct_a->ymax >= rct_b->ymax));
}

void gfRctfTranslate(SRectF *rect, float x, float y)
{
    rect->xmin += x;
    rect->ymin += y;
    rect->xmax += x;
    rect->ymax += y;
}

void gfRctiTranslate(SRectI *rect, TInt x, TInt y)
{
    rect->xmin += x;
    rect->ymin += y;
    rect->xmax += x;
    rect->ymax += y;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

