/*******************************************************************************
 * testamlog.h
 *
 * Histroy:
 *  2012-2-28 2012 - [ypchang] created file
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

#ifndef TESTAMLOG_H_
#define TESTAMLOG_H_

#include <unistd.h>
class TestAmlogA
{
  public:
    TestAmlogA(){}
    virtual ~TestAmlogA()
    {
      DEBUG("~TestAmlogA deleted");
    }

  public:
    void a();
    void b();
    void c();
    void d();

    static void *thread_entry(void *data)
    {
      for (int i = 0; i < 50; ++ i) {
        ((TestAmlogA *)data)->a();
        usleep(10);
        ((TestAmlogA *)data)->b();
        usleep(10);
        ((TestAmlogA *)data)->c();
        usleep(10);
        ((TestAmlogA *)data)->d();
        usleep(10);
      }
      return 0;
    }
};

class TestAmlogB
{
  public:
    TestAmlogB(){}
    virtual ~TestAmlogB()
    {
      DEBUG("~TestAmlogB deleted");
    }

  public:
    void a();
    void b();
    void c();
    void d();

    static void *thread_entry(void *data)
    {
      sleep(1);
      for (int i = 0; i < 50; ++ i) {
        ((TestAmlogB *)data)->a();
        usleep(10);
        ((TestAmlogB *)data)->b();
        usleep(10);
        ((TestAmlogB *)data)->c();
        usleep(10);
        ((TestAmlogB *)data)->d();
        usleep(10);
      }
      return 0;
    }
};

#endif /* TESTAMLOG_H_ */
