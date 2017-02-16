/*******************************************************************************
 * version.c
 *
 * Histroy:
 *  2015/04/02 - [Zhaoyang Chen] created file
 *
 * Copyright (C) 2015-2019, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc
 *
 ******************************************************************************/
#include <basetypes.h>
#include "iav_ioctl.h"
#include "lib_vproc.h"
/*
 *  1.0.0	2015/04/03
 *         Add MCTF type PM support and synchronize with DZ operation.
 *  1.1.0	2015/07/23
 *         Add support for PM direct Draw and Clear.
 *  1.2.0	2015/08/03
 *         Add support for multiple PM operation.
 *  1.3.0	2015/08/07
 *         Add support for circle PM operation.
 *  1.3.1	2015/09/23
 *         Update MCTF type PM for Blend ISO mode.
 */

#define MAINPP_LIB_MAJOR 1
#define MAINPP_LIB_MINOR 3
#define MAINPP_LIB_PATCH 1
#define MAINPP_LIB_VERSION ((MAINPP_LIB_MAJOR << 16) | \
                             (MAINPP_LIB_MINOR << 8)  | \
                             MAINPP_LIB_PATCH)

version_t G_mainpp_version = {
	.major = MAINPP_LIB_MAJOR,
	.minor = MAINPP_LIB_MINOR,
	.patch = MAINPP_LIB_PATCH,
	.mod_time = 0x20150923,
	.description = "Ambarella S2L Video Process Library",
};

