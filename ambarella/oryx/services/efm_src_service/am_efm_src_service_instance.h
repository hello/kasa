/*******************************************************************************
 * am_efm_src_service_instance.h
 *
 * History:
 *   2016-05-04 - [Zhi He] created file
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef AM_EFM_SRC_SERVICE_INSTANCE_H_
#define AM_EFM_SRC_SERVICE_INSTANCE_H_
#include "am_api_efm_src.h"

enum {
  AM_EFM_SRC_SERVICE_INSTANCE_STATE_IDLE = 0x0,
  AM_EFM_SRC_SERVICE_INSTANCE_STATE_RUNING = 0x1,
  AM_EFM_SRC_SERVICE_INSTANCE_STATE_DEC_ERROR = 0x2,
  AM_EFM_SRC_SERVICE_INSTANCE_STATE_COMPLETED = 0x3,
};

class AMEFMSourceServiceInstance
{
public:
  AMEFMSourceServiceInstance(int index);
  virtual ~AMEFMSourceServiceInstance();

public:
  int feed_efm_from_es(char *input_url, uint8_t stream_index);
  int feed_efm_from_usb_camera(char *input_url, uint8_t stream_index,
    char *dump_efm_src_filename, uint8_t format, uint32_t width, uint32_t height, uint32_t fps);
  int end_feed();

  int setup_yuv_capture_pipeline(uint8_t buffer_index, uint8_t stream_index);
  int destroy_yuv_capture_pipeline();
  int yuv_capture_for_efm(uint8_t number_of_yuv_frames);

protected:
  void clear();

protected:
  IGenericEngineControl *m_engine_control;
  int m_index;
  int m_state;

protected:
  TGenericID m_source_id;
  TGenericID m_video_decoder_id;
  TGenericID m_efm_injecter_id;
  TGenericID m_dump_efm_src_id;

protected:
  std::mutex m_mtx;
};

#endif /* AM_EFM_SRC_SERVICE_INSTANCE_H_ */

