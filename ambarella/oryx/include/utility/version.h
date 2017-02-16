/*******************************************************************************
 * version.h
 *
 * Histroy:
 *  2012-3-7 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_UTILS_VERSION_H_
#define AM_UTILS_VERSION_H_

#define UTILITY_LIB_MAJOR 1
#define UTILITY_LIB_MINOR 0
#define UTILITY_LIB_PATCH 0
#define UTILITY_LIB_VERSION ((UTILITY_LIB_MAJOR << 16) | \
                             (UTILITY_LIB_MINOR << 8)  | \
                             UTILITY_LIB_PATCH)
#define UTILITY_VERSION_STR "1.0.0"

#define OSAL_LIB_MAJOR 1
#define OSAL_LIB_MINOR 0
#define OSAL_LIB_PATCH 0
#define OSAL_LIB_VERSION ((OSAL_LIB_MAJOR << 16) | \
                          (OSAL_LIB_MINOR << 8>) | \
                          OSAL_LIB_PATH)
#define OSAL_VERSION_STR "1.0.0"

#endif /* AM_UTILS_VERSION_H_ */
