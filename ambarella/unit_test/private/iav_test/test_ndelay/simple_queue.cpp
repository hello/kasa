/**
 * simple_queue.cpp
 *
 * History:
 *	2015/09/01 - [Zhi He] create file
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
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include "pthread.h"
#include "semaphore.h"

#include "simple_log.h"
#include "simple_queue.h"

//-----------------------------------------------------------------------
//
//  CISimpleQueue
//
//-----------------------------------------------------------------------
typedef struct SSimpleNode_s
{
    unsigned long ctx;
    struct SSimpleNode_s* p_pre;
    struct SSimpleNode_s* p_next;
} SSimpleNode;

class CISimpleQueue : public ISimpleQueue
{
public:
    static ISimpleQueue* Create(unsigned int num);

private:
    CISimpleQueue(unsigned int num);
    int Construct();
    virtual ~CISimpleQueue();

public:
    virtual void Destroy();

public:
    virtual unsigned int GetCnt();
    virtual void Lock();
    virtual void UnLock();

    virtual void Enqueue(unsigned long ctx);
    virtual unsigned long Dequeue();
    virtual unsigned int TryDequeue(unsigned long& ret_ctx);

private:
    unsigned int mMaxCnt;
    unsigned int mCurCnt;
    SSimpleNode mHead;

    SSimpleNode* mpFreelist;

    pthread_mutex_t mMutex;
    pthread_cond_t mCondNotfull;
    pthread_cond_t mCondNotempty;
};

CISimpleQueue::CISimpleQueue(unsigned int num)
{
    mMaxCnt = num;
    mCurCnt = 0;
    mHead.p_next = mHead.p_pre= &mHead;
    mpFreelist=NULL;

    pthread_mutex_init(&mMutex, NULL);
    pthread_cond_init(&mCondNotempty, NULL);
    pthread_cond_init(&mCondNotfull, NULL);
}

CISimpleQueue::~CISimpleQueue()
{
    SSimpleNode* ptmp=NULL, *ptmp1=NULL;

    ptmp1 = mHead.p_next;
    while (ptmp1 != &mHead) {
        ptmp=ptmp1;
        ptmp1=ptmp1->p_next;
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

ISimpleQueue* CISimpleQueue::Create(unsigned int num)
{
    CISimpleQueue* result = new CISimpleQueue(num);
    if (result && (0 != result->Construct())) {
        delete result;
        result = NULL;
    }
    return result;
}

int CISimpleQueue::Construct()
{
    return 0;
}

void CISimpleQueue::Destroy()
{
    delete this;
}

unsigned int CISimpleQueue::GetCnt()
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

void CISimpleQueue::Enqueue(unsigned long ctx)
{
    SSimpleNode* ptmp=NULL;
    pthread_mutex_lock(&mMutex);

    if (mMaxCnt) {
        while(mCurCnt >= mMaxCnt) {
            pthread_cond_wait(&mCondNotfull, &mMutex);
        }
    }

    if(!mpFreelist) {
        ptmp = (SSimpleNode*)malloc(sizeof(SSimpleNode));
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

unsigned long CISimpleQueue::Dequeue()
{
    SSimpleNode* ptmp=NULL;
    unsigned long ctx;

    pthread_mutex_lock(&mMutex);

    while (mCurCnt == 0) {
        pthread_cond_wait(&mCondNotempty,&mMutex);
    }

    ptmp = mHead.p_pre;
    if(ptmp == &mHead) {
        LOG_ERROR("fatal error: currupt in Dequeue.\n");
        pthread_mutex_unlock(&mMutex);
        return 0;
    }

    ptmp->p_pre->p_next = &mHead;
    mHead.p_pre = ptmp->p_pre;
    mCurCnt--;

    ptmp->p_next = mpFreelist;
    mpFreelist = ptmp;
    ctx=ptmp->ctx;

    if ((mMaxCnt) && ((mCurCnt + 1) >= mMaxCnt)) {
        pthread_cond_signal(&mCondNotfull);
    }

    pthread_mutex_unlock(&mMutex);

    return ctx;
}

unsigned int CISimpleQueue::TryDequeue(unsigned long& ret)
{
    SSimpleNode* ptmp=NULL;

    pthread_mutex_lock(&mMutex);

    if (mCurCnt == 0) {
        ret = 0;
        pthread_mutex_unlock(&mMutex);
        return 0;
    }

    ptmp = mHead.p_pre;
    if(ptmp == &mHead) {
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

ISimpleQueue* gfCreateSimpleQueue(unsigned int num)
{
    return CISimpleQueue::Create(num);
}

//-----------------------------------------------------------------------
//
// CSimpleRingMemPool
//
//-----------------------------------------------------------------------
class CSimpleRingMemPool: public IMemPool
{
public:
    static IMemPool* Create(unsigned long size);
    virtual void Destroy();

    virtual unsigned char* RequestMemBlock(unsigned long size, unsigned char* start_pointer = NULL);
    virtual void ReturnBackMemBlock(unsigned long size, unsigned char* start_pointer);
    virtual void ReleaseMemBlock(unsigned long size, unsigned char* start_pointer);

protected:
    CSimpleRingMemPool();
    int Construct(unsigned long size);
    virtual ~CSimpleRingMemPool();

private:
    pthread_mutex_t mMutex;
    pthread_cond_t mCond;

private:
    unsigned char mRequestWrapCount;
    unsigned char mReleaseWrapCount;
    unsigned char mReserved1;
    unsigned char mReserved2;

private:
    unsigned char* mpMemoryBase;
    unsigned char* mpMemoryEnd;
    unsigned long mMemoryTotalSize;

    unsigned char* volatile mpFreeMemStart;
    unsigned char* volatile mpUsedMemStart;

private:
    volatile unsigned int mnWaiters;
};

//-----------------------------------------------------------------------
//
// CSimpleRingMemPool
//
//-----------------------------------------------------------------------
IMemPool* CSimpleRingMemPool::Create(unsigned long size)
{
    CSimpleRingMemPool* thiz = new CSimpleRingMemPool();
    if (thiz && (0 == thiz->Construct(size))) {
        return thiz;
    } else {
        if (thiz) {
            LOG_FATAL("CSimpleRingMemPool->Construct(size = %ld) fail\n", size);
            delete thiz;
        } else {
            LOG_FATAL("new CSimpleRingMemPool() fail\n");
        }
    }

    return NULL;
}

void CSimpleRingMemPool::Destroy()
{
    if (mpMemoryBase) {
        free(mpMemoryBase);
        mpMemoryBase = NULL;
    }

    mMemoryTotalSize = 0;

    mpFreeMemStart = NULL;
    mpUsedMemStart = NULL;
}

CSimpleRingMemPool::CSimpleRingMemPool()
    : mRequestWrapCount(0)
    , mReleaseWrapCount(1)
    , mpMemoryBase(NULL)
    , mpMemoryEnd(NULL)
    , mMemoryTotalSize(0)
    , mpFreeMemStart(NULL)
    , mpUsedMemStart(NULL)
    , mnWaiters(0)
{

}

int CSimpleRingMemPool::Construct(unsigned long size)
{
    DASSERT(!mpMemoryBase);

    mpMemoryBase = (unsigned char*) malloc(size);
    if (mpMemoryBase) {
        mMemoryTotalSize = size;
        mpFreeMemStart = mpMemoryBase;
        mpMemoryEnd = mpMemoryBase + size;
        mpUsedMemStart = mpMemoryBase;
    } else {
        LOG_FATAL("alloc mem(size = %lu) fail\n", size);
        return (-1);
    }

    pthread_mutex_init(&mMutex, NULL);
    pthread_cond_init(&mCond, NULL);

    return 0;
}

CSimpleRingMemPool::~CSimpleRingMemPool()
{
    if (mpMemoryBase) {
        free(mpMemoryBase);
        mpMemoryBase = NULL;
    }

    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCond);
}

unsigned char* CSimpleRingMemPool::RequestMemBlock(unsigned long size, unsigned char* start_pointer)
{
    unsigned char* p_ret = NULL;
    unsigned long currentFreeSize1 = 0;
    unsigned long currentFreeSize2 = 0;

    pthread_mutex_lock(&mMutex);

    do {
        DASSERT(mpFreeMemStart <= mpMemoryEnd);
        DASSERT(mpFreeMemStart >= mpMemoryBase);
        if ((mpFreeMemStart > mpMemoryEnd) || (mpFreeMemStart < mpMemoryBase)) {
            LOG_ERROR("mpFreeMemStart %p not in valid range[%p, %p)\n", mpFreeMemStart, mpMemoryBase, mpMemoryEnd);
        }

        DASSERT(mpUsedMemStart <= mpMemoryEnd);
        DASSERT(mpUsedMemStart >= mpMemoryBase);
        if ((mpUsedMemStart > mpMemoryEnd) || (mpUsedMemStart < mpMemoryBase)) {
            LOG_ERROR("mpUsedMemStart %p not in valid range[%p, %p)\n", mpUsedMemStart, mpMemoryBase, mpMemoryEnd);
        }

        if (((unsigned long)mpFreeMemStart) == ((unsigned long)mpUsedMemStart)) {
            if (mRequestWrapCount == mReleaseWrapCount) {
                //LOGM_NOTICE("hit\n");
            } else {
                if (((mRequestWrapCount + 1) == mReleaseWrapCount) || ((255 == mRequestWrapCount) && (0 == mReleaseWrapCount))) {
                    currentFreeSize1 = mpMemoryEnd - mpFreeMemStart;
                    currentFreeSize2 = mpUsedMemStart - mpMemoryBase;

                    if (currentFreeSize1 >= size) {
                        p_ret = mpFreeMemStart;
                        mpFreeMemStart += size;
                        pthread_mutex_unlock(&mMutex);
                        return p_ret;
                    } else if (currentFreeSize2 >= size) {
                        p_ret = mpMemoryBase;
                        mpFreeMemStart = mpMemoryBase + size;

                        mRequestWrapCount ++;
                        pthread_mutex_unlock(&mMutex);
                        return p_ret;
                    } else {
                        LOG_FATAL("both currentFreeSize1 %ld and currentFreeSize2 %ld, not fit request size %ld\n", currentFreeSize1, currentFreeSize2, size);
                    }
                } else {
                    LOG_WARN("in exit flow? mRequestWrapCount %d, mReleaseWrapCount %d\n", mRequestWrapCount, mReleaseWrapCount);
                }
            }
        } else if (((unsigned long)mpFreeMemStart) > ((unsigned long)mpUsedMemStart)) {

            //debug assert
            //DASSERT((mReleaseWrapCount == (mRequestWrapCount + 1)) || ((255 == mRequestWrapCount) && (0 == mReleaseWrapCount)));

            currentFreeSize1 = mpMemoryEnd - mpFreeMemStart;
            currentFreeSize2 = mpUsedMemStart - mpMemoryBase;

            if (currentFreeSize1 >= size) {
                p_ret = mpFreeMemStart;
                mpFreeMemStart += size;
                pthread_mutex_unlock(&mMutex);
                return p_ret;
            } else if (currentFreeSize2 >= size) {
                p_ret = mpMemoryBase;
                mpFreeMemStart = mpMemoryBase + size;

                mRequestWrapCount ++;
                pthread_mutex_unlock(&mMutex);
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
                pthread_mutex_unlock(&mMutex);
                return p_ret;
            }
        }

        mnWaiters ++;
        pthread_cond_wait(&mCond, &mMutex);
    } while (1);

    pthread_mutex_unlock(&mMutex);
    LOG_FATAL("must not comes here\n");
    return NULL;
}

void CSimpleRingMemPool::ReturnBackMemBlock(unsigned long size, unsigned char* start_pointer)
{
    pthread_mutex_lock(&mMutex);

    DASSERT((start_pointer + size) == mpFreeMemStart);
    DASSERT((start_pointer + size) <= (mpMemoryEnd));
    DASSERT(start_pointer >= (mpMemoryBase));

    mpFreeMemStart = start_pointer;
    if (mnWaiters) {
        mnWaiters --;
        pthread_cond_signal(&mCond);
    }

    pthread_mutex_unlock(&mMutex);
}

void CSimpleRingMemPool::ReleaseMemBlock(unsigned long size, unsigned char* start_pointer)
{
    pthread_mutex_lock(&mMutex);

    if (start_pointer != mpMemoryBase) {
        if (start_pointer != mpUsedMemStart) {
            LOG_WARN("in exit flow? start_pointer %p, mpUsedMemStart %p\n", start_pointer, mpUsedMemStart);
        }
    }
    DASSERT((start_pointer + size) <= (mpMemoryEnd));

    if (start_pointer < (mpMemoryBase)) {
        LOG_FATAL("start_pointer %p, mpMemoryBase %p\n", start_pointer, mpMemoryBase);
        pthread_mutex_unlock(&mMutex);
        return;
    } else if ((start_pointer + size) > (mpMemoryEnd)) {
        LOG_FATAL("start_pointer %p, size %ld, mpMemoryBase %p\n", start_pointer, size, mpMemoryBase);
        pthread_mutex_unlock(&mMutex);
        return;
    }

    //LOGM_NOTICE("ReleaseMemBlock size %lu, pointer %p, current %p\n", size, start_pointer, start_pointer + size);
    if ((start_pointer + size) < mpUsedMemStart) {
        if (start_pointer == mpMemoryBase) {
            mReleaseWrapCount ++;
        } else {
            LOG_WARN("in exit flow? start_pointer %p not equal to mpMemoryBase %p\n", start_pointer, mpMemoryBase);
            pthread_mutex_unlock(&mMutex);
            return;
        }
    }
    mpUsedMemStart = start_pointer + size;
    if (mnWaiters) {
        mnWaiters --;
        pthread_cond_signal(&mCond);
    }

    pthread_mutex_unlock(&mMutex);
}

IMemPool* gfCreateSimpleRingMemPool(unsigned long size)
{
    return CSimpleRingMemPool::Create(size);
}


