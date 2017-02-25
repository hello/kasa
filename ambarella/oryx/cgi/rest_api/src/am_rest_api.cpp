/**
 * am_rest_api.cpp
 *
 *  History:
 *		2015/08/19 - [Huaiqing Wang] created file
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
#include <stdio.h>
#include <string.h>
#include <mutex>
#include "am_rest_api.h"

static std::recursive_mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::recursive_mutex> lck (m_mtx);
AMRestAPI* AMRestAPI::m_instance = nullptr;
AMRestAPI::AMRestAPI():
    m_handle(nullptr),
    m_utils(nullptr)
{
}

AMRestAPI::~AMRestAPI()
{
}

AMRestAPI *AMRestAPI::create()
{
  DECLARE_MUTEX;
  if (!m_instance) {
    m_instance = new AMRestAPI();
  }
  return m_instance;
}

void AMRestAPI::destory()
{
  delete m_instance;
  m_instance = nullptr;

  m_handle->destroy();
  m_handle = nullptr;

  m_utils = nullptr;
}

void  AMRestAPI::run()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    if (!(m_utils = AMRestAPIUtils::get_instance())) {
      break;
    }
    if ((ret = m_utils->cgi_parse_client_params()) != AM_REST_RESULT_OK) {
      break;
    }
    std::string service;
    if (m_utils->get_value("arg0", service) != AM_REST_RESULT_OK) {
      ret = AM_REST_RESULT_ERR_URL;
      m_utils->set_response_msg(AM_REST_URL_ARG0_ERR, "no serive type");
      break;
    }

    if (!(m_handle = AMRestAPIHandle::create(service))) {
      ret = AM_REST_RESULT_ERR_SERVER;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "create %s service instance failed",
               service.c_str());
      m_utils->set_response_msg(AM_REST_SERVER_INIT_ERR, msg);
      break;
    }
    ret = m_handle->rest_api_handle();
  } while(0);

  if (m_utils) {
    m_utils->cgi_response_client(ret);
  }
}
