/*******************************************************************************
 * am_playback_service_instance.h
 *
 * History:
 *   2016-04-14 - [Zhi He] created file
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

#ifndef AM_PLAYBACK_SERVICE_INSTANCE_H_
#define AM_PLAYBACK_SERVICE_INSTANCE_H_

enum {
  AM_PLAYBACK_SERVICE_INSTANCE_STATE_IDLE = 0x0,
  AM_PLAYBACK_SERVICE_INSTANCE_STATE_RUNING = 0x1,
  AM_PLAYBACK_SERVICE_INSTANCE_STATE_DEC_ERROR = 0x2,
  AM_PLAYBACK_SERVICE_INSTANCE_STATE_COMPLETED = 0x3,
};

enum {
  AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_RUN = 0x0,
  AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_PAUSED = 0x1,
  AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_STEP = 0x2,
};

class AMPlaybackServiceInstance
{
public:
  AMPlaybackServiceInstance(int index);
  virtual ~AMPlaybackServiceInstance();

public:
  int play(char *url, int enable_audio, char *prefer_demuxer,
  char *prefer_video_decoder, char *prefer_video_output,
  char *prefer_audio_decoder, char *prefer_audio_output,
  unsigned char use_hdmi, unsigned char use_digital, unsigned char use_cvbs, unsigned char rtsp_tcp);
  int end_play();

  int pause();
  int resume();
  int step();

  int seek(unsigned long long target);
  int query_current_position(unsigned long long *pos);

  int fast_forward(unsigned short speed, unsigned char scan_mode, int from_begining);
  int fast_backward(unsigned short speed, unsigned char scan_mode, int from_end);

protected:
  void clear();
  void resume_pipeline();

protected:
  IGenericEngineControl *m_engine_control;
  int m_index;
  int m_state;

protected:
  TGenericID m_demuxer_id;
  TGenericID m_video_decoder_id;
  TGenericID m_audio_decoder_id;
  TGenericID m_video_renderer_id;
  TGenericID m_audio_renderer_id;
  TGenericID m_playback_pipeline_id;

protected:
  unsigned char m_is_live_streaming;
  unsigned char m_has_decode_error;
  unsigned char m_playback_complete;
  unsigned char m_trick_mode;

protected:
  unsigned short m_speed;
  unsigned char m_direction;
  unsigned char m_scan_mode;

protected:
  std::mutex m_mtx;
};

#endif /* AM_PLAYBACK_SERVICE_INSTANCE_H_ */

