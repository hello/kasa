/*******************************************************************************
 * mw_version.c
 *
 * Histroy:
 *  2014/07/14 2014 - [Lei Hong] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include "mw_struct.h"

/*
 * 2.5.1 2015/08/05
 *      Support load/save img_mw config file.
 *      Support dynamic load adj parameters file.
 *      Sort out the global variables.
 *      Add protect for slow shutter.
 * 2.5.2 2015/09/08
 *      Support P-iris lens.(m13vp288ir, mz128bp2810icr)
 */
#define AMP_LIB_MAJOR 2
#define AMP_LIB_MINOR 5
#define AMP_LIB_PATCH 2
#define AMP_LIB_VERSION ((AMP_LIB_MAJOR << 16) | \
                             (AMP_LIB_MINOR << 8)  | \
                             AMP_LIB_PATCH)

mw_version_info mw_version =
{
	.major		= AMP_LIB_MAJOR,
	.minor		= AMP_LIB_MINOR,
	.patch		= AMP_LIB_PATCH,
	.update_time	= 0x20150908,
};
