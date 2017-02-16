/**
 * am_rest_api.cpp
 *
 *  History:
 *		2015年8月18日 - [Huaiqing Wang] created file
 *
 * Copyright (C) 2007-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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
      snprintf(msg, MAX_MSG_LENGHT, "create %s service instance failed", service.c_str());
      m_utils->set_response_msg(AM_REST_SERVER_INIT_ERR, msg);
      break;
    }
    ret = m_handle->rest_api_handle();
  } while(0);

  if (m_utils) {
    m_utils->cgi_response_client(ret);
  }
}
