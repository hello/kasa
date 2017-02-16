/*
 * wifi_setup.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 15/01/2015 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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
