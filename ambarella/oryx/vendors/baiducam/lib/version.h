/*******************************************************************************
 * version.h
 *
 * History:
 *   2015-4-22 - [Shupeng Ren] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#ifndef __RTMP_BLS_VERSION_H__
#define __RTMP_BLS_VERSION_H__

#define RTMP_BLS_LIB_MAJOR      1
#define RTMP_BLS_LIB_MINOR      0
#define RTMP_BLS_LIB_PATCH      0
#define RTMP_BLS_LIB_VERSION ((RTMP_BLS_LIB_MAJOR << 16) | \
                              (RTMP_BLS_LIB_MINOR << 8)  | \
                               RTMP_BLS_LIB_PATCH)

#define RTMP_BLS_LIB_VERSION_STR "1.0.0"

#endif
