/*******************************************************************************
 * version.h
 *
 * Histroy:
 *  2014-7-25 2014 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_VIDEO_VERSION_H_
#define AM_VIDEO_VERSION_H_

#define VIDEO_LIB_MAJOR 1
#define VIDEO_LIB_MINOR 0
#define VIDEO_LIB_PATCH 0
#define VIDEO_LIB_VERSION ((VIDEO_LIB_MAJOR << 16) | \
                           (VIDEO_LIB_MINOR << 8)  | \
                            VIDEO_LIB_PATCH)
#define VIDEO_VERSION_STR "1.0.0"

#define READER_LIB_MAJOR 1
#define READER_LIB_MINOR 0
#define READER_LIB_PATCH 0
#define READER_LIB_VERSION ((READER_LIB_MAJOR << 16) | \
                            (READER_LIB_MINOR << 8>) | \
                            READER_LIB_PATCH)
#define READER_VERSION_STR "1.0.0"

#define ADDRESS_LIB_MAJOR 1
#define ADDRESS_LIB_MINOR 0
#define ADDRESS_LIB_PATCH 0
#define ADDRESS_LIB_VERSION ((ADDRESS_LIB_MAJOR << 16) | \
                             (ADDRESS_LIB_MINOR << 8>) | \
                             ADDRESS_LIB_PATCH)
#define ADDRESS_VERSION_STR "1.0.0"

#endif /* AM_VIDEO_VERSION_H_ */
