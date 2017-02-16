/**
 * bld/piggy_ramdisk.S
 *
 * History:
 *    2005/02/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <amboot.h>
#include <fio/firmfl.h>

piggy_ramdisk:

#ifdef VERSION_INFO
#include xstr(VERSION_INFO)
#endif

.text

#ifdef INCBIN

#ifdef CRC32
.word	CRC32
#else
#error "CRC32 not defined!"
#endif

#ifdef FIRM_VER_NUM
.word	FIRM_VER_NUM
#else
.word	0x0
#endif

#ifdef FIRM_VER_DATE
.word	FIRM_VER_DATE
#else
.word	0x0
#endif

#ifdef IMG_LEN
.word	IMG_LEN
#else
.word	0x0
#endif

#ifdef FIRM_MEM_ADDR
.word	FIRM_MEM_ADDR
#else
.word	0x0
#endif

#ifdef FIRM_FLAG
.word	FIRM_FLAG
#else
.word	0x0
#endif

.word	PARTHD_MAGIC

.rept 57
.word	0x0
.endr

.incbin xstr(INCBIN)

#endif

.end

