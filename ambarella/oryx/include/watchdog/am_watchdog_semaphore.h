/*
 * am_watchdog.h
 *
 * @Author: Yang Wang
 * @Email : ywang@ambarella.com
 * @Time  : 09/09/2013 [Created]
 *
 * Copyright (C) 20013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AM_WATCHDOG_H__
#define __AM_WATCHDOG_H__

#define HEART_BEAT_INTERVAL  2
#define SEM_SYS_SERVICE   (const char*)"/System.Service"
#define SEM_MED_SERVICE   (const char*)"/Media.Service"
#define SEM_EVT_SERVICE   (const char*)"/Event.Service"
#define SEM_IMG_SERVICE   (const char*)"/Image.Service"
#define SEM_VCTRL_SERVICE (const char*)"/Video.Service"
#define SEM_NET_SERVICE   (const char*)"/Network.Service"
#define SEM_AUD_SERVICE (const char*)"/Audio.Service"
#define SEM_RTSP_SERVICE (const char*)"/RTSP.Service"
#define SEM_SIP_SERVICE (const char*)"/SIP.Service"

#endif

