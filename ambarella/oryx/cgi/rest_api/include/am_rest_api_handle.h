/**
 * am_rest_api_handle.h
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
#ifndef ORYX_CGI_INCLUDE_AM_REST_API_HANDLE_H_
#define ORYX_CGI_INCLUDE_AM_REST_API_HANDLE_H_

#include "am_api_helper.h"
#include "am_rest_api_utils.h"

extern AMAPIHelperPtr gCGI_api_helper;
#define METHOD_CALL(cmd_id,data,data_size,result,result_size,state) \
      if (gCGI_api_helper) {\
        gCGI_api_helper->method_call(cmd_id,data,data_size,result,result_size);\
      } else {state = AM_REST_RESULT_ERR_PERM;}

//oryx services handle base class
class AMRestAPIHandle
{
  public:
    static AMRestAPIHandle *create(const std::string &service);

    void  destroy();
    virtual AM_REST_RESULT rest_api_handle() = 0;

  protected:
    AMRestAPIHandle();
    virtual ~AMRestAPIHandle();
    AMRestAPIUtilsPtr  m_utils;

  private:
    AM_REST_RESULT init();
    static AMRestAPIHandle *m_instance;
};


#endif /* ORYX_CGI_INCLUDE_AM_REST_API_HANDLE_H_ */
