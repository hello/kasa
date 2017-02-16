/*
 * Filename : mt9t002_pri.h
 *
 * History:
 *    2011/01/12 - [Haowei Lo] Create
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */
#ifndef __MT9T002_PRI_H__
#define __MT9T002_PRI_H__

#define MT9T002_CHIP_VERSION_REG			0x3000

#define MT9T002_FRAME_LENGTH_LINES			0x300A
#define MT9T002_LINE_LENGTH_PCK				0x300C

#define MT9T002_COARSE_INTEGRATION_TIME			0x3012

/* MT9T002 mirror mode */
#define MT9T002_H_MIRROR			(0x01 << 14)
#define MT9T002_V_FLIP 			(0x01 << 15)
#define MT9T002_MIRROR_MASK			(MT9T002_H_MIRROR + MT9T002_V_FLIP)

#endif /* __MT9T002_PRI_H__ */

