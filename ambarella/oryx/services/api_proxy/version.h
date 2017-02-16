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

#ifndef AM_API_PROXY_VERSION_H_
#define AM_API_PROXY_VERSION_H_

#define API_PROXY_LIB_MAJOR 1
#define API_PROXY_LIB_MINOR 0
#define API_PROXY_LIB_PATCH 0
#define API_PROXY_LIB_VERSION ((API_PROXY_LIB_MAJOR << 16) | \
                             (API_PROXY_LIB_MINOR << 8)  | \
                             API_PROXY_LIB_PATCH)
#define API_PROXY_VERSION_STR "1.0.0"

#define OSAL_LIB_MAJOR 1
#define OSAL_LIB_MINOR 0
#define OSAL_LIB_PATCH 0
#define OSAL_LIB_VERSION ((OSAL_LIB_MAJOR << 16) | \
                          (OSAL_LIB_MINOR << 8>) | \
                          OSAL_LIB_PATH)
#define OSAL_VERSION_STR "1.0.0"

#endif /* AM_API_PROXY_VERSION_H_ */
