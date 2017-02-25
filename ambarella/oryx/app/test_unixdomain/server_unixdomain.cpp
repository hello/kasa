/*******************************************************************************
 * server_unixdomain.cpp
 *
 * History:
 *   2016-07-18 - [Guohua Zheng]  created file
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

#include "export_UDS.h"
#include "am_muxer_codec_if.h"
#include "am_log.h"
#include <signal.h>
#include <condition_variable>
bool running_flag = true;
std::condition_variable           m_wait_cond;
std::mutex m_wait;

static void __sigstop(int i)
{
  m_wait_cond.notify_one();
}


int main(int argc, char* argv[])
{
  std::unique_lock<std::mutex> lk(m_wait);
  signal(SIGINT, __sigstop);
  signal(SIGQUIT, __sigstop);
  signal(SIGTERM, __sigstop);
  AMMuxerExportUDS *m_server = new AMMuxerExportUDS();
  m_server->init();
  m_server->start();
  if (running_flag) {
    m_wait_cond.wait(lk);
  }
  m_server->stop();
  delete m_server;
  return 0;
}
