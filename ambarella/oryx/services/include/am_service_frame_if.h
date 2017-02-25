/*******************************************************************************
 * am_service_frame_if.h
 *
 * History:
 *   2015-1-26 - [ypchang] created file
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
#ifndef ORYX_SERVICES_INCLUDE_AM_SERVICE_FRAME_IF_H_
#define ORYX_SERVICES_INCLUDE_AM_SERVICE_FRAME_IF_H_

typedef void (*AMServiceFrameCallback)(char cmd);

class AMIServiceFrame
{
  public:
    static AMIServiceFrame* create(const std::string& service_name);

  public:
    virtual void destroy() = 0;
    /*
     * This function blocks until interrupts are received or quit is called
     */
    virtual void run()     = 0;
    /*
     * This function will make run() return
     */
    virtual bool quit()    = 0;
    virtual void set_user_input_callback(AMServiceFrameCallback cb) = 0;
    virtual uint32_t version() = 0;

  protected:
    virtual ~AMIServiceFrame(){}
};

#endif /* ORYX_SERVICES_INCLUDE_AM_SERVICE_FRAME_IF_H_ */
