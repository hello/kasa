/*******************************************************************************
 * lib_dewarp_header.h
 *
 * History:
 *  Oct 16, 2013 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef LIB_DEWARP_HEADER_H_
#define LIB_DEWARP_HEADER_H_

#include <basetypes.h>
#include "utils.h"

#ifndef NO_GLIBC

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#define ATTR_UNUSED __attribute__((unused))

#else

#define NULL        0
#define ATTR_UNUSED

typedef unsigned long size_t ;

// math.h
extern float sqrtf(float x);
extern float asinf(float x);
extern float atanf(float x);
extern float sinf(float x);
extern float cosf(float x);
extern float tanf(float x);
extern void sincosf(float x, float* sin, float* cos);

// stdlib.h
extern void* malloc(size_t size);
extern void free(void *ptr);

// string.h
extern void* memcpy(void* dest, const void* src, size_t n);
extern void* memset(void* s, int c, size_t n);

#endif


#endif /* LIB_DEWARP_HEADER_H_ */
