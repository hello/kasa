/*******************************************************************************
 * version.h
 *
 * History:
 *   2014-9-10 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef VERSION_H_
#define VERSION_H_

#define STREAM_LIB_MAJOR 1
#define STREAM_LIB_MINOR 0
#define STREAM_LIB_PATCH 0
#define STREAM_LIB_VERSION ((STREAM_LIB_MAJOR << 16) | \
                            (STREAM_LIB_MINOR << 8)  | \
                            STREAM_LIB_PATCH)
#define STREAM_VERSION_STR "1.0.0"

#define PLAYBACK_LIB_MAJOR 1
#define PLAYBACK_LIB_MINOR 0
#define PLAYBACK_LIB_PATCH 0
#define PLAYBACK_LIB_VERSION ((PLAYBACK_LIB_MAJOR << 16) | \
                              (PLAYBACK_LIB_MINOR << 8)  | \
                              PLAYBACK_LIB_PATCH)
#define PLAYBACK_VERSION_STR "1.0.0"

#define RECORD_LIB_MAJOR 1
#define RECORD_LIB_MINOR 0
#define RECORD_LIB_PATCH 0
#define RECORD_LIB_VERSION ((RECORD_LIB_MAJOR << 16) | \
                            (RECORD_LIB_MINOR << 8)  | \
                            RECORD_LIB_PATCH)
#define RECORD_VERSION_STR "1.0.0"

#endif /* VERSION_H_ */
