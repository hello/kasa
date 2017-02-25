/*
 * wifi_setup.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 15/01/2015 [Created]
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
#ifndef __WIFI_SETUP_H__
#define __WIFI_SETUP_H__

enum WIFI_SETUP_GUIDE_VOICE {
  VOICE_NORMAL = 0,
  VOICE_QRCODE_CONFIG,
  VOICE_WIFI_SETUP_NOT_DONE,
  VOICE_WIFI_SETUP_WRONG,
  VOICE_WIFI_SETUP_FINISHED,
};

enum WIFI_SETUP_STATUS {
  WIFI_CONFIG_FETCH_MODE = 0,
  WIFI_CONFIG_EXISTS_MODE,
  NORMAL_RUNNING_MODE,
};

enum WIFI_SETUP_AUDIO_TYPE {
  MUSIC_VOICE = 0,
  SOUND_EFFECTS,
};

enum WIFI_SETUP_RESOURCE_ID
{
  SYS_HELLO_VOICE = 0,
  SYS_MODE_NORMAL_VOICE,
  SYS_MODE_QRCODE_WIFI_CONFIG_VOICE,
  WIFI_SETUP_FINISH_VOICE,
  WIFI_SETUP_MODE_AP_VOICE,
  WIFI_SETUP_MODE_STATION_VOICE,
  WIFI_SETUP_NOT_DONE_VOICE,
  WIFI_SETUP_WRONG_VOICE,
  ERROR1_EFFECT,
  ERROR2_EFFECT,
  ERROR3_EFFECT,
  ERROR4_EFFECT,
  FAILURE_EFFECT,
  SHUTTER1_EFFECT,
  SHUTTER2_EFFECT,
  SUCCESS1_EFFECT,
  SUCCESS2_EFFECT,
  SUCCESS3_EFFECT,
  SUCCESS4_EFFECT,
  RESOURCE_NUM,
};

#endif
