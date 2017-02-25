/*******************************************************************************
 * test_mutex.cpp
 *
 * History:
 *   2014-8-13 - [ypchang] created file
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_mutex.h"
#include <sys/time.h>

#include <mutex>
#include <thread>
#include <atomic>

static std::mutex g_mutex;
static AMMemLock lock;
static AMMutex *p_mutex = nullptr;

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
      lock.lock();
      ++ value;
      lock.unlock();
    } else {
      lock.lock();
      -- value;
      lock.unlock();
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
      fprintf(stdout, "Value is %lld, Used %llus:%lluus\n",
              value, (endUs - startUs) / 1000000, (endUs - startUs) % 1000000);
    }
    delete thread1;
    delete thread2;
    if (p_mutex) {
      p_mutex->destroy();
    }
  }

  return 0;
}
