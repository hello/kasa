/**
 * am_rest_api_handle.cpp
 *
 *  History:
 *		2015/08/24 - [Huaiqing Wang] created file
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
 */
#include <mutex>
#include <unistd.h>
#include "am_rest_api_handle.h"
#include "am_rest_api_video.h"
#include "am_rest_api_audio.h"
#include "am_rest_api_event.h"
#include "am_rest_api_image.h"
#include "am_rest_api_media.h"
#include "am_rest_api_sip.h"
#include "am_rest_api_system.h"

static std::recursive_mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::recursive_mutex> lck (m_mtx);
AMAPIHelperPtr   gCGI_api_helper = nullptr;
AMRestAPIHandle *AMRestAPIHandle::m_instance = nullptr;
AMRestAPIHandle::AMRestAPIHandle():
    m_utils(nullptr)
{
}

AMRestAPIHandle::~AMRestAPIHandle()
{
  gCGI_api_helper = nullptr;
}

AMRestAPIHandle* AMRestAPIHandle::create(const std::string &service)
{
  do {
    DECLARE_MUTEX;
    if ("video" == service) {
      m_instance = new AMRestAPIVideo();
    } else if ("media" == service) {
      m_instance = new AMRestAPIMedia();
    } else if ("image" == service) {
      m_instance = new AMRestAPIImage();
    } else if ("audio" == service) {
      m_instance = new AMRestAPIAudio();
    } else if ("event" == service) {
      m_instance = new AMRestAPIEvent();
    } else if ("sip" == service) {
      m_instance = new AMRestAPISip();
    } else if ("system" == service) {
      m_instance = new AMRestAPISystem();
    }
  } while(0);
  if (m_instance && (m_instance->init() != AM_REST_RESULT_OK)) {
    delete m_instance;
    m_instance = nullptr;
  }
  return m_instance;
}

void AMRestAPIHandle::destroy()
{
  delete m_instance;
  m_instance = nullptr;
}

AM_REST_RESULT AMRestAPIHandle::init()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    if (!(m_utils = AMRestAPIUtils::get_instance()))
    {
      break;
    }
    if (access("/tmp/apps_launcher.pid", F_OK) == 0) {
      if (!(gCGI_api_helper = AMAPIHelper::get_instance())) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_SERVER_INIT_ERR,
                                  "get AMAPIHelper instance failed");
        break;
      }
    }
  } while(0);

  return ret;
}
