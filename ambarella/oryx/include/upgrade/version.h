/*******************************************************************************
 * version.h
 *
 * History:
 *   2015-1-12 - [longli] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_INCLUDE_UPGRADE_VERSION_H_
#define ORYX_INCLUDE_UPGRADE_VERSION_H_

#define UPGRADE_LIB_MAJOR 1
#define UPGRADE_LIB_MINOR 0
#define UPGRADE_LIB_PATCH 0
#define UPGRADE_LIB_VERSION ((UPGRADE_LIB_MAJOR << 16) | \
                             (UPGRADE_LIB_MINOR << 8)  | \
                             UPGRADE_LIB_PATCH)

#define UPGRADE_VERSION_STR "1.0.0"

#define AMDOWNLOAD_LIB_MAJOR 1
#define AMDOWNLOAD_LIB_MINOR 0
#define AMDOWNLOAD_LIB_PATCH 0
#define AMDOWNLOAD_LIB_VERSION ((AMDOWNLOAD_LIB_MAJOR << 16) | \
                                (AMDOWNLOAD_LIB_MINOR << 8)  | \
                                AMDOWNLOAD_LIB_PATCH)

#define AMDOWNLOAD_VERSION_STR "1.0.0"

#endif /* ORYX_INCLUDE_UPGRADE_VERSION_H_ */
