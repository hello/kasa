/*
 * kernel/private/include/iav_devnum.h
 *
 * History:
 *    2012/10/25 - [Cao Rongrong] Create
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __IAV_DEV_NUM_H__
#define __IAV_DEV_NUM_H__

#include <mach/hardware.h>

#define IAV_DEV_MAJOR		AMBA_DEV_MAJOR
#define IAV_DEV_MINOR		0

#define UCODE_DEV_MAJOR		AMBA_DEV_MAJOR
#define UCODE_DEV_MINOR		1

#define DSPLOG_DEV_MAJOR	AMBA_DEV_MAJOR
#define DSPLOG_DEV_MINOR	(AMBA_DEV_MINOR_PUBLIC_END + 9)

#endif

