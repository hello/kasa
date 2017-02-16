//------------------------------------------------------------------------------
// <copyright file="host_version.h" company="Atheros">
//    Copyright (c) 2004-2010 Atheros Corporation.  All rights reserved.
// 
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
// </copyright>
// 
// <summary>
// 	Wifi driver for AR6002
// </summary>
//
//------------------------------------------------------------------------------
//==============================================================================
// This file contains version information for the sample host driver for the
// AR6000 chip
//
// Author(s): ="Atheros"
//==============================================================================
#ifndef _HOST_VERSION_H_
#define _HOST_VERSION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <AR6002/AR6K_version.h>

/*
 * The version number is made up of major, minor, patch and build
 * numbers. These are 16 bit numbers.  The build and release script will
 * set the build number using a Perforce counter.  Here the build number is
 * set to 9999 so that builds done without the build-release script are easily
 * identifiable.
 */

#define ATH_SW_VER_MAJOR      __VER_MAJOR_
#define ATH_SW_VER_MINOR      __VER_MINOR_
#define ATH_SW_VER_PATCH      __VER_PATCH_
#define ATH_SW_VER_BUILD      __BUILD_NUMBER_ 

#ifdef __cplusplus
}
#endif

#endif /* _HOST_VERSION_H_ */
