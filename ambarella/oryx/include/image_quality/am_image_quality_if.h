/*******************************************************************************
 * am_image_quality_if.h
 *
 * History:
 *   Dec 31, 2014 - [binwang] created file
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
#ifndef AM_IMAGE_QUALITY_IF_H_
#define AM_IMAGE_QUALITY_IF_H_

#include "am_iq_param.h"
#include "am_pointer.h"

#define FILE_NAME_LENGTH 64

class AMIImageQuality;

typedef AMPointer<AMIImageQuality> AMIImageQualityPtr;

class AMIImageQuality
{
    friend AMIImageQualityPtr;
  public:
    static AMIImageQualityPtr get_instance();
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool load_config() = 0;
    virtual bool save_config() = 0;
    virtual bool set_config(AM_IQ_CONFIG *config) = 0;
    virtual bool get_config(AM_IQ_CONFIG *config) = 0;
    virtual bool need_notify() = 0;
  protected:
    virtual void release() = 0;
    virtual void inc_ref() = 0;
    virtual ~AMIImageQuality()
    {
    }
};

#endif /* AM_IMAGE_QUALITY_IF_H_ */
