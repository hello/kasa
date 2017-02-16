/**
 * am_rest_api_handle.cpp
 *
 *  History:
 *		2015年8月24日 - [Huaiqing Wang] created file
 *
 * Copyright (C) 2007-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <mutex>
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
    if (!(gCGI_api_helper = AMAPIHelper::get_instance())) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_SERVER_INIT_ERR, "get AMAPIHelper instance failed");
      break;
    }
  } while(0);

  return ret;
}
