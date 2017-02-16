/*******************************************************************************
 * test_mutex.cpp
 *
 * History:
 *   2014-8-13 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_mutex.h"
#include <sys/time.h>

#include <mutex>
#include <thread>
#include <atomic>

static std::mutex g_mutex;
AMSpinLock *lock = NULL;
AMMutex *p_mutex = NULL;

int64_t value = 0;

void print_block_mutex(uint64_t n, bool c)
{
  for (uint64_t i = 0; i < n; ++ i) {
    if (c) {
      g_mutex.lock();
      ++ value;
      g_mutex.unlock();
    } else {
      g_mutex.lock();
      -- value;
      g_mutex.unlock();
    }
  }
}

void print_block_atomic(uint64_t n, char c)
{
  for (uint64_t i = 0; i < n; ++ i) {
    if (c) {
      lock->lock();
      ++ value;
      lock->unlock();
    } else {
      lock->lock();
      -- value;
      lock->unlock();
    }
  }
}

void print_block_pthread(uint64_t n, char c)
{
  for (uint64_t i = 0; i < n; ++ i) {
    if (c) {
      p_mutex->lock();
      ++ value;
      p_mutex->unlock();
    } else {
      p_mutex->lock();
      -- value;
      p_mutex->unlock();
    }
  }
}

int main(int argc, char* argv[])
{
  if (argc != 3) {
    fprintf(stderr, "Usage: %s [atomic | mutex | pthread] times\n", argv[0]);
  } else {
    std::thread *thread1 = NULL;
    std::thread *thread2 = NULL;
    struct timeval start;
    struct timeval end;
    uint64_t times = strtoul((const char*)argv[2], (char**)NULL, 10);

    value = times;
    if (is_str_equal(argv[1], "atomic")) {
      lock = AMSpinLock::create();
      gettimeofday(&start, NULL);
      thread1 = new std::thread(print_block_atomic, times, true);
      thread2 = new std::thread(print_block_atomic, times, false);
    } else if (is_str_equal(argv[1], "mutex")) {
      gettimeofday(&start, NULL);
      thread1 = new std::thread(print_block_mutex, times, true);
      thread2 = new std::thread(print_block_mutex, times, false);
    } else if (is_str_equal(argv[1], "pthread")) {
      p_mutex = AMMutex::create();
      gettimeofday(&start, NULL);
      thread1 = new std::thread(print_block_pthread, times, true);
      thread2 = new std::thread(print_block_pthread, times, false);
    } else {
      fprintf(stderr, "Unknown test cast \"%s\", "
              "valid cases are \"atomic | mutex | pthread\"\n", argv[1]);
    }
    if (thread1 && thread2) {
      uint64_t startUs = 0;
      uint64_t endUs = 0;
      thread1->join();
      thread2->join();
      gettimeofday(&end, NULL);
      startUs = start.tv_sec * 1000000 + start.tv_usec;
      endUs = end.tv_sec * 1000000 + end.tv_usec;
      fprintf(stdout, "Value is %lld, Used %llu s - %llu us\n",
              value, (endUs - startUs) / 1000000, (endUs - startUs) % 1000000);
    }
    delete thread1;
    delete thread2;
    if (lock) {
      lock->destroy();
    }
    if (p_mutex) {
      p_mutex->destroy();
    }
  }

  return 0;
}
