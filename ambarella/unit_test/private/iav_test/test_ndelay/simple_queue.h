/**
 * simple_queue.h
 *
 * History:
 *	2015/08/31 - [Zhi He] create file
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

#ifndef __SIMPLE_QUEUE_H__
#define __SIMPLE_QUEUE_H__

//-----------------------------------------------------------------------
//
//  ISimpleQueue
//
//-----------------------------------------------------------------------

class ISimpleQueue
{
public:
  virtual void Destroy() = 0;

protected:
  virtual ~ISimpleQueue() {}

public:
  virtual unsigned int GetCnt() = 0;
  virtual void Lock() = 0;
  virtual void UnLock() = 0;

  virtual void Enqueue(unsigned long ctx) = 0;
  virtual unsigned long Dequeue() = 0;
  virtual unsigned int TryDequeue(unsigned long &ret_ctx) = 0;
};

extern ISimpleQueue* gfCreateSimpleQueue(unsigned int num);

//-----------------------------------------------------------------------
//
// IMemPool
//
//-----------------------------------------------------------------------
class IMemPool
{
public:
  virtual void Destroy() = 0;

public:
  virtual unsigned char *RequestMemBlock(unsigned long size, unsigned char *start_pointer = NULL) = 0;
  virtual void ReturnBackMemBlock(unsigned long size, unsigned char *start_pointer) = 0;
  virtual void ReleaseMemBlock(unsigned long size, unsigned char *start_pointer) = 0;

protected:
  virtual ~IMemPool() {}
};

extern IMemPool* gfCreateSimpleRingMemPool(unsigned long size);

#endif


