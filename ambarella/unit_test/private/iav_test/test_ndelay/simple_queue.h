/**
 * simple_queue.h
 *
 * History:
 *	2015/08/31 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
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


