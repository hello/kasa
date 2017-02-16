// Copyright (c) 2010 Atheros Communications Inc.
// All rights reserved.
// 
//
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
//
//

/* dk.h - contains variable declarations */

#ifndef __DK_H_
#define __DK_H_

// Driver information
#define DRV_NAME    "linuxdk"
#define DRV_MAJOR_VERSION 1
#define DRV_MINOR_VERSION 2

#ifdef MODULE
#define MOD_LICENCE "GPL"
#define MOD_AUTHOR "sharmat@atheros.com"
#define MOD_DESCRIPTION "Linux MDK driver 1.0" 
#endif

// Common variable types are typedefed 
typedef short INT16;
typedef unsigned short UINT16;
typedef int INT32;
typedef unsigned int UINT32;
typedef char CHAR8;
typedef unsigned char UCHAR8;
typedef char INT8;
typedef unsigned char UINT8;
typedef void VOID;

#ifndef NULL
#define NULL (void *)0
#endif

#define MAX_BARS 6

typedef enum 
{
		FALSE=0,
		TRUE
} BOOLEAN;

#endif // __DK_H_
