/*******************************************************************************
 * am_video_edit_service_instance.h
 *
 * History:
 *   2016-04-28 - [Zhi He] created file
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

#ifndef AM_VIDEO_EDIT_SERVICE_INSTANCE_H_
#define AM_VIDEO_EDIT_SERVICE_INSTANCE_H_

enum {
  AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_IDLE = 0x0,
  AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_RUNING = 0x1,
  AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_DEC_ERROR = 0x2,
  AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_COMPLETED = 0x3,
};

class AMVideoEditServiceInstance
{
public:
  AMVideoEditServiceInstance(int index);
  virtual ~AMVideoEditServiceInstance();

public:
  int es_2_es(char *input_url, char *output_url, uint8_t stream_index);
  int es_2_mp4(char *input_url, char *output_url, uint8_t stream_index);
  int feed_efm(char *input_url, uint8_t stream_index);
  int end_processing();

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
  TGenericID m_video_encoder_id;
  TGenericID m_sink_id;

protected:
  std::mutex m_mtx;
};

#endif /* AM_VIDEO_EDIT_SERVICE_INSTANCE_H_ */

