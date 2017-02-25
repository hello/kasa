/**
 * am_filters_interface.h
 *
 * History:
 *  2015/07/27 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#ifndef __AM_FILTERS_INTERFACES_H__
#define __AM_FILTERS_INTERFACES_H__

typedef enum {
  EFilterType_Invalid = 0,

  EFilterType_Demuxer = 0x01,
  EFilterType_VideoDecoder = 0x02,
  EFilterType_ScheduledVideoDecoder = 0x03,
  EFilterType_AudioDecoder = 0x04,
  EFilterType_VideoEncoder = 0x05,
  EFilterType_AudioEncoder = 0x06,
  EFilterType_Muxer = 0x07,
  EFilterType_ScheduledMuxer = 0x08,
  EFilterType_VideoPreProcessor = 0x09,
  EFilterType_AudioPreProcessor = 0x0a,
  EFilterType_VideoPostProcessor = 0x0b,
  EFilterType_AudioPostProcessor = 0x0c,
  EFilterType_VideoCapture = 0x0d,
  EFilterType_AudioCapture = 0x0e,
  EFilterType_VideoRenderer = 0x0f,
  EFilterType_AudioRenderer = 0x10,
  EFilterType_RTSPTransmiter = 0x11,
  EFilterType_ConnecterPinmuxer = 0x12,
  EFilterType_CloudConnecterServerAgent = 0x13,
  EFilterType_CloudConnecterClientAgent = 0x14,
  EFilterType_CloudConnecterCmdAgent = 0x15,
  EFilterType_FlowController = 0x16,
  EFilterType_VideoOutput = 0x17,
  EFilterType_AudioOutput = 0x18,
  EFilterType_ExtVideoSource = 0x19,
  EFilterType_ExtAudioSource = 0x1a,
  EFilterType_VideoInjecter = 0x1b,
  EFilterType_CloudUploader = 0x1c,
  EFilterType_VideoRawSink = 0x1d,
  EFilterType_VideoESSink = 0x1e,
} EFilterType;

extern IFilter *gfFilterFactory(EFilterType type, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU32 index = 0);

#endif

