/**
 * hwio.h
 *
 * History:
 *    2004/11/29 - [Charles Chiou] created file
 *
 * This file provides I/O operation wrappers for OS'es that do not already
 * provide it (such as PrKERNELv4). In fact, even if it did, we'd like to
 * ideally use the following ones to be more uniform/portable.
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __HWIO_H__
#define __HWIO_H__

/*===========================================================================*/
#ifndef __ASM__

#define __raw_writeb(v, a)	(*(volatile unsigned char  *)(a) = (v))
#define __raw_writew(v, a)	(*(volatile unsigned short *)(a) = (v))
#define __raw_writel(v, a)	(*(volatile unsigned int   *)(a) = (v))

#define __raw_readb(a)		(*(volatile unsigned char  *)(a))
#define __raw_readw(a)		(*(volatile unsigned short *)(a))
#define __raw_readl(a)		(*(volatile unsigned int   *)(a))

#define writeb(p, v)		__raw_writeb(v, p)
#define writew(p, v)		__raw_writew(v, p)
#define writel(p, v)		__raw_writel(v, p)

#define readb(p)		__raw_readb(p)
#define readw(p)		__raw_readw(p)
#define readl(p)		__raw_readl(p)

#define setbitsb(p, mask)	writeb((p),(readb(p) | (mask)))
#define clrbitsb(p, mask)	writeb((p),(readb(p) & ~(mask)))
#define setbitsw(p, mask)	writew((p),(readw(p) | (mask)))
#define clrbitsw(p, mask)	writew((p),(readw(p) & ~(mask)))
#define setbitsl(p, mask)	writel((p),(readl(p) | (mask)))
#define clrbitsl(p, mask)	writel((p),(readl(p) & ~(mask)))

#endif /* !__ASM__ */

/*===========================================================================*/

#endif

