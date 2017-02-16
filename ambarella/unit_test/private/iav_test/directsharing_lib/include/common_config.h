/**
 * common_config.h
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __COMMON_CONFIG_H__
#define __COMMON_CONFIG_H__

//common header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN \
namespace mw_cg {

#define DCONFIG_COMPILE_OPTION_HEADERFILE_END \
}

#define DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN \
namespace mw_cg {

#define DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END \
}


#define DCONFIG_COMPILE_OPTION_CPPFILE_BEGIN \
using namespace mw_cg;

#define DCODE_DELIMITER

//#define DCONFIG_BIG_ENDIAN

//debug related
//#define DCONFIG_ENABLE_DEBUG_CHECK

//#define DCONFIG_ENABLE_REALTIME_PRINT

//old version file
//#define DCONFIG_COMPILE_OLD_VERSION_FILE

//obsolete module
//#define DCONFIG_COMPILE_OBSOLETE

//not finished module
//#define DCONFIG_COMPILE_NOT_FINISHED_MODULE

//cross platform: os
//#define BUILD_OS_WINDOWS
//#define BUILD_OS_IOS
//#define BUILD_OS_ANDROID

//log system
#define DCONFIG_USE_NATIVE_LOG_SYSTEM
//#define DCONFIG_LOG_SYSTEM_ENABLE_TRACE
#define DCONFIG_ENALDE_DEBUG_LOG

//
#ifdef BUILD_OS_WINDOWS
#define snprintf _snprintf
#include<winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#define BUILD_MODULE_OPTCODEC_ENC_AVC
//#define BUILD_MODULE_OPTCODEC_ENC_AAC
//#define BUILD_MODULE_OPTCODEC_DEC
//#define BUILD_MODULE_HEVC_HM_DEC
//#define BUILD_MODULE_WDUP
//#define BUILD_MODULE_WMMDC
#define BUILD_MODULE_WDS
//#define BUILD_MODULE_X264
//#define BUILD_MODULE_X265
#define BUILD_MODULE_NEW_FFMPEG
//#define BUILD_MODULE_FAAC
//#define BUILD_MODULE_FREE_GULT
typedef SOCKET TSocketHandler;
#define DInvalidSocketHandler (INVALID_SOCKET)
#define DIsSocketHandlerValid(x) (INVALID_SOCKET != x)
#else
typedef int TSocketHandler;
#define DInvalidSocketHandler (-1)
#define DIsSocketHandlerValid(x) (0 <= x)
#endif

#endif

