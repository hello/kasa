/*******************************************************************************
 * version.h
 *
 * Histroy:
 *  2014-11-17 [dgwu] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_EVENT_VERSION_H_
#define AM_EVENT_VERSION_H_

#define EVENT_MONITOR_LIB_MAJOR 0
#define EVENT_MONITOR_LIB_MINOR 1
#define EVENT_MONITOR_LIB_PATCH 0
#define EVENT_MONITOR_LIB_VERSION ((EVENT_MONITOR_LIB_MAJOR << 16) | \
                           (EVENT_MONITOR_LIB_MINOR << 8)  | \
                           EVENT_MONITOR_LIB_PATCH)

#define EVENT_MONITOR_VERSION_STR "1.0.0"


#define AUDIO_ALERT_LIB_MAJOR 1
#define AUDIO_ALERT_LIB_MINOR 0
#define AUDIO_ALERT_LIB_PATCH 0
#define AUDIO_ALERT_LIB_VERSION ((AUDIO_ALERT_LIB_MAJOR << 16) | \
                             (AUDIO_ALERT_LIB_MINOR << 8)  | \
                             AUDIO_ALERT_LIB_PATCH)

#define KEY_INPUT_VERSION_STR "1.0.0"

#define KEY_INPUT_LIB_MAJOR 1
#define KEY_INPUT_LIB_MINOR 0
#define KEY_INPUT_LIB_PATCH 0
#define KEY_INPUT_LIB_VERSION ((KEY_INPUT_LIB_MAJOR << 16) | \
                             (KEY_INPUT_LIB_MINOR << 8)  | \
                             KEY_INPUT_LIB_PATCH)

#define KEY_INPUT_VERSION_STR "1.0.0"

#endif /* AM_EVENT_VERSION_H_ */
