/**
 * am_native_log.h
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __AM_NATIVE_LOG_H__
#define __AM_NATIVE_LOG_H__

//-----------------------------------------------------------------------
//
//  log system
//
//-----------------------------------------------------------------------
#define DCONFIG_USE_NATIVE_LOG_SYSTEM
#define DCONFIG_ENALDE_DEBUG_LOG
#define DMAX_LOGTAG_NAME_LEN 64

#ifdef DCONFIG_USE_NATIVE_LOG_SYSTEM

extern volatile TUint *const pgConfigLogLevel;
extern volatile TUint *const pgConfigLogOption;
extern volatile TUint *const pgConfigLogOutput;

extern const TChar gcConfigLogLevelTag[ELogLevel_TotalCount][DMAX_LOGTAG_NAME_LEN];
extern const TChar gcConfigLogOptionTag[ELogOption_TotalCount][DMAX_LOGTAG_NAME_LEN];
extern const TChar gcConfigLogOutputTag[ELogOutput_TotalCount][DMAX_LOGTAG_NAME_LEN];

extern FILE *gpLogOutputFile;
extern FILE *gpSnapshotOutputFile;

//-----------------------------------------------------------------------
//
//  log macros
//
//-----------------------------------------------------------------------
//#define _DLOG_GLOBAL_MUTEX_NAME AUTO_LOCK(gpLogMutex)
#define _DLOG_GLOBAL_MUTEX_NAME

#define _DLOGCOLOR_HEADER_FATAL "\033[40;31m\033[1m"
#define _DLOGCOLOR_HEADER_ERROR "\033[40;31m\033[1m"
#define _DLOGCOLOR_HEADER_WARN "\033[40;33m\033[1m"
#define _DLOGCOLOR_HEADER_NOTICE "\033[40;35m\033[1m"
#define _DLOGCOLOR_HEADER_INFO "\033[40;37m\033[1m"
#define _DLOGCOLOR_HEADER_DETAIL "\033[40;32m"

#define _DLOGCOLOR_HEADER_OPTION "\033[40;36m"

#define _DLOGCOLOR_TAIL_FATAL "\033[0m"
#define _DLOGCOLOR_TAIL_ERROR "\033[0m"
#define _DLOGCOLOR_TAIL_WARN "\033[0m"
#define _DLOGCOLOR_TAIL_NOTICE "\033[0m"
#define _DLOGCOLOR_TAIL_INFO "\033[0m"
#define _DLOGCOLOR_TAIL_DETAIL "\033[0m"

#define _DLOGCOLOR_TAIL_OPTION "\033[0m"

#ifdef BUILD_OS_WINDOWS

#define _NOLOG(format, ...) (void)0

#define _LOG_LEVEL_MODULE(level, format, ...)  do { \
    if (DUnlikely(mConfigLogLevel >= level)) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
        printf("%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
        printf(format, __VA_ARGS__);  \
      } \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
        if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
          fflush(gpLogOutputFile); \
        } \
        if (DUnlikely(gpSnapshotOutputFile)) { \
          fprintf(gpSnapshotOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(gpSnapshotOutputFile, format, __VA_ARGS__);  \
          fflush(gpSnapshotOutputFile); \
        } \
      } \
      if (DUnlikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_ModuleFile))) { \
        if (DUnlikely(mpLogOutputFile)) { \
          fprintf(mpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(mpLogOutputFile, format, __VA_ARGS__);  \
        } else if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
          fflush(gpLogOutputFile); \
        } \
      } \
    } \
  } while (0)

#define _LOG_LEVEL(level, format, ...)  do { \
    if (DUnlikely((*pgConfigLogLevel) >= level)) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_Console))) { \
        printf("%s\t", (const TChar*)gcConfigLogLevelTag[level]); \
        printf(format, __VA_ARGS__);  \
      } \
      if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_File))) { \
        if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t", (const TChar*)gcConfigLogLevelTag[level]); \
          fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
          fflush(gpLogOutputFile); \
        } \
        if (DUnlikely(gpSnapshotOutputFile)) { \
          fprintf(gpSnapshotOutputFile, "%s\t", (const TChar*)gcConfigLogLevelTag[level]); \
          fprintf(gpSnapshotOutputFile, format, __VA_ARGS__);  \
          fflush(gpSnapshotOutputFile); \
        } \
      } \
    } \
  } while (0)

#define _LOG_LEVEL_MODULE_TRACE(level, format, ...)  do { \
    if (DLikely(mConfigLogLevel >= level)) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
        printf("%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], GetModuleName(), GetModuleIndex()); \
        printf(format, __VA_ARGS__);  \
        printf("\t\t\t\t\t[trace]: file %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
      } \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
        if (DUnlikely(mpLogOutputFile)) { \
          fprintf(mpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], GetModuleName(), GetModuleIndex()); \
          fprintf(mpLogOutputFile, format, __VA_ARGS__);  \
          fprintf(mpLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
        } else if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], GetModuleName(), GetModuleIndex()); \
          fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
          fprintf(gpLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
          fflush(gpLogOutputFile); \
        } \
        if (DUnlikely(gpSnapshotOutputFile)) { \
          fprintf(gpSnapshotOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], GetModuleName(), GetModuleIndex()); \
          fprintf(gpSnapshotOutputFile, format, __VA_ARGS__);  \
          fprintf(gpSnapshotOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
          fflush(gpSnapshotOutputFile); \
        } \
      } \
    } \
  } while (0)

#define _LOG_LEVEL_TRACE(level, format, ...)  do { \
    if (DLikely((*pgConfigLogLevel) >= level)) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_Console))) { \
        printf("%s\t", (const TChar*)gcConfigLogLevelTag[level]); \
        printf(format, __VA_ARGS__);  \
        printf("\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
      } \
      if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_File))) { \
        if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t", (const TChar*)gcConfigLogLevelTag[level]); \
          fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
          fprintf(gpLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
          fflush(gpLogOutputFile); \
        } \
        if (DUnlikely(gpSnapshotOutputFile)) { \
          fprintf(gpSnapshotOutputFile, "%s\t", (const TChar*)gcConfigLogLevelTag[level]); \
          fprintf(gpSnapshotOutputFile, format, __VA_ARGS__);  \
          fprintf(gpSnapshotOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
          fflush(gpSnapshotOutputFile); \
        } \
      } \
    } \
  } while (0)


#define _LOG_OPTION_MODULE(option, format, ...)  do { \
    if (DUnlikely(mConfigLogOption & DCAL_BITMASK(option))) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
        printf("%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], (const char*)GetModuleName(), GetModuleIndex()); \
        printf(format, __VA_ARGS__);  \
      } \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
        if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
          fflush(gpLogOutputFile); \
        } \
      } \
      if (DUnlikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_ModuleFile))) { \
        if (DUnlikely(mpLogOutputFile)) { \
          fprintf(mpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(mpLogOutputFile, format, __VA_ARGS__);  \
        } else if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
          fflush(gpLogOutputFile); \
        } \
      } \
    } \
  } while (0)

#define _LOG_OPTION(option, format, ...)  do { \
    if (DUnlikely((*pgConfigLogOption) & DCAL_BITMASK(option))) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_Console))) { \
        printf("%s\t", (const TChar*)gcConfigLogOptionTag[option]); \
        printf(format, __VA_ARGS__);  \
      } \
      if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_File))) { \
        if(gpLogOutputFile) { \
          fprintf(gpLogOutputFile, "%s\t", (const TChar*)gcConfigLogOptionTag[option]); \
          fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
          fflush(gpLogOutputFile); \
        } \
      } \
    } \
  } while (0)

#define _LOG_OPTION_MODULE_TRACE(option, format, ...)  do { \
    if (DUnlikely(mConfigLogOption & DCAL_BITMASK(option))) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
        printf("%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], GetModuleName(), GetModuleIndex()); \
        printf(format, __VA_ARGS__);  \
        printf("\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
      } \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
        if (DUnlikely(mpLogOutputFile)) { \
          fprintf(mpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], GetModuleName(), GetModuleIndex()); \
          fprintf(mpLogOutputFile, format, __VA_ARGS__);  \
          fprintf(mpLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
        } else if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], GetModuleName(), GetModuleIndex()); \
          fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
          fprintf(gpLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
          fflush(gpLogOutputFile); \
        } \
      } \
    } \
  } while (0)

#define _LOG_OPTION_TRACE(option, format, ...)  do { \
    if (DUnlikely(gConfigLogOption & DCAL_BITMASK(option))) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely(gConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
        printf("%s\t", (const TChar*)gcConfigLogOptionTag[option]); \
        printf(format, __VA_ARGS__);  \
        printf("\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
      } \
      if (DLikely(gConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
        if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t", (const TChar*)gcConfigLogOptionTag[option]); \
          fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
          fprintf(gpLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
          fflush(gpLogOutputFile); \
        } \
      } \
    } \
  } while (0)

#define __CLEAN_PRINT(format, ...)  do { \
    printf(format, __VA_ARGS__);  \
    if (DLikely(gpLogOutputFile)) { \
      fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
    } \
  } while (0)

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define LOGM_VERBOSE(format, ...)       _LOG_LEVEL_MODULE(ELogLevel_Verbose, format, __VA_ARGS__)
#define LOGM_DEBUG(format, ...)         _LOG_LEVEL_MODULE(ELogLevel_Debug, format, __VA_ARGS__)
#define LOGM_INFO(format, ...)          _LOG_LEVEL_MODULE(ELogLevel_Info, format, __VA_ARGS__)
#else
#define LOGM_VERBOSE        _NOLOG
#define LOGM_DEBUG          _NOLOG
#define LOGM_INFO           _NOLOG
#endif

#ifdef DCONFIG_LOG_SYSTEM_ENABLE_TRACE
#define LOGM_NOTICE(format, ...)        _LOG_LEVEL_MODULE_TRACE(ELogLevel_Notice, format, __VA_ARGS__)
#else
#define LOGM_NOTICE(format, ...)        _LOG_LEVEL_MODULE(ELogLevel_Notice, format, __VA_ARGS__)
#endif

#define LOGM_WARN(format, ...)          _LOG_LEVEL_MODULE_TRACE(ELogLevel_Warn, format, __VA_ARGS__)
#define LOGM_ERROR(format, ...)         _LOG_LEVEL_MODULE_TRACE(ELogLevel_Error, format, __VA_ARGS__)
#define LOGM_FATAL(format, ...)         _LOG_LEVEL_MODULE_TRACE(ELogLevel_Fatal, format , __VA_ARGS__)
#define LOGM_ALWAYS(format, ...)         _LOG_LEVEL_MODULE(ELogLevel_None, format, __VA_ARGS__)
#define LOGM_CONFIGURATION(format, ...)         _LOG_OPTION_MODULE(ELogOption_Congifuration, format, __VA_ARGS__)

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define LOG_VERBOSE(format, ...)        _LOG_LEVEL(ELogLevel_Verbose, format, __VA_ARGS__)
#define LOG_DEBUG(format, ...)          _LOG_LEVEL(ELogLevel_Debug, format, __VA_ARGS__)
#define LOG_INFO(format, ...)           _LOG_LEVEL(ELogLevel_Info, format, __VA_ARGS__)
#else
#define LOG_VERBOSE         _NOLOG
#define LOG_DEBUG           _NOLOG
#define LOG_INFO            _NOLOG
#endif

#ifdef DCONFIG_LOG_SYSTEM_ENABLE_TRACE
#define LOG_NOTICE(format, ...)         _LOG_LEVEL_TRACE(ELogLevel_Notice, format, __VA_ARGS__)
#else
#define LOG_NOTICE(format, ...)         _LOG_LEVEL(ELogLevel_Notice, format, __VA_ARGS__)
#endif

#define LOG_WARN(format, ...)           _LOG_LEVEL_TRACE(ELogLevel_Warn, format, __VA_ARGS__)
#define LOG_ERROR(format, ...)          _LOG_LEVEL_TRACE(ELogLevel_Error, format, __VA_ARGS__)
#define LOG_FATAL(format, ...)          _LOG_LEVEL_TRACE(ELogLevel_Fatal, format, __VA_ARGS__)
#define LOG_ALWAYS(format, ...)         _LOG_LEVEL(ELogLevel_None, format, __VA_ARGS__)
#define LOG_CONFIGURATION(format, ...)          _LOG_OPTION(ELogOption_Congifuration, format, __VA_ARGS__)

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define LOGM_STATE(format, ...)         _LOG_OPTION_MODULE(ELogOption_State, format, __VA_ARGS__)
#define LOGM_FLOW(format, ...)          _LOG_OPTION_MODULE(ELogOption_Flow, format, __VA_ARGS__)
#define LOGM_PTS(format, ...)           _LOG_OPTION_MODULE(ELogOption_PTS, format, __VA_ARGS__)
#define LOGM_RESOURCE(format, ...)      _LOG_OPTION_MODULE(ELogOption_Resource, format, __VA_ARGS__)
#define LOGM_TIMING(format, ...)        _LOG_OPTION_MODULE(ELogOption_Timing, format, __VA_ARGS__)
#define LOGM_BINARY(format, ...)        _LOG_OPTION_MODULE(ELogOption_BinaryHeader, format, __VA_ARGS__)
#else
#define LOGM_STATE          _NOLOG
#define LOGM_FLOW           _NOLOG
#define LOGM_PTS            _NOLOG
#define LOGM_RESOURCE       _NOLOG
#define LOGM_TIMING         _NOLOG
#define LOGM_BINARY         _NOLOG
#endif

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define LOG_STATE(format, ...)          _LOG_OPTION(ELogOption_State, format, __VA_ARGS__)
#define LOG_FLOW(format, ...)           _LOG_OPTION(ELogOption_Flow, format, __VA_ARGS__)
#define LOG_PTS(format, ...)            _LOG_OPTION(ELogOption_PTS, format, __VA_ARGS__)
#define LOG_RESOURCE(format, ...)       _LOG_OPTION(ELogOption_Resource, format, __VA_ARGS__)
#define LOG_TIMING(format, ...)         _LOG_OPTION(ELogOption_Timing, format, __VA_ARGS__)
#define LOG_BINARY(format, ...)         _LOG_OPTION(ELogOption_BinaryHeader, format, __VA_ARGS__)
#else
#define LOG_STATE           _NOLOG
#define LOG_FLOW            _NOLOG
#define LOG_PTS             _NOLOG
#define LOG_RESOURCE        _NOLOG
#define LOG_TIMING          _NOLOG
#define LOG_BINARY          _NOLOG
#endif

#define LOGM_PRINTF(format, ...)          _LOG_LEVEL_MODULE(ELogLevel_None, format, __VA_ARGS__)
#define LOG_PRINTF(format, ...)          _LOG_LEVEL(ELogLevel_None, format, __VA_ARGS__)

#define DASSERT(expr) do { \
    if (DUnlikely(!(expr))) { \
      printf("assertion failed: %s\n\tAt file: %s\n\tfunction: %s, line %d\n", #expr, __FILE__, __FUNCTION__, __LINE__); \
      if (gpLogOutputFile) { \
        fprintf(gpLogOutputFile,"assertion failed: %s\n\tAt file: %s\n\tfunction: %s: line %d\n", #expr, __FILE__, __FUNCTION__, __LINE__); \
        fflush(gpLogOutputFile); \
      } \
    } \
  } while (0)

#define DASSERT_OK(code) do { \
    if (DUnlikely(EECode_OK != code)) { \
      printf("ERROR code %s\n\tAt file: %s\n\tfunction: %s, line %d\n", gfGetErrorCodeString(code), __FILE__, __FUNCTION__, __LINE__); \
      if (gpLogOutputFile) { \
        fprintf(gpLogOutputFile, "ERROR code %s\n\tAt file: %s\n\tfunction: %s, line %d\n", gfGetErrorCodeString(code), __FILE__, __FUNCTION__, __LINE__); \
        fflush(gpLogOutputFile); \
      } \
    } \
  } while (0)

#else

#define _NOLOG(format, args...) (void)0

#define _LOG_LEVEL_MODULE(level, header, tail, format, args...)  do { \
    if (DUnlikely(mConfigLogLevel >= level)) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
        printf(header "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
        printf(format, ##args);  \
        printf(tail); \
        fflush(stdout); \
      } \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
        if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(gpLogOutputFile, format,##args);  \
          fflush(gpLogOutputFile); \
        } \
        if (DUnlikely(gpSnapshotOutputFile)) { \
          fprintf(gpSnapshotOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(gpSnapshotOutputFile, format,##args);  \
          fflush(gpSnapshotOutputFile); \
        } \
      } \
      if (DUnlikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_ModuleFile))) { \
        if (DUnlikely(mpLogOutputFile)) { \
          fprintf(mpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(mpLogOutputFile, format,##args);  \
        } else if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(gpLogOutputFile, format,##args);  \
          fflush(gpLogOutputFile); \
        } \
      } \
    } \
  } while (0)

#define _LOG_LEVEL(level, header, tail, format, args...)  do { \
    if (DUnlikely((*pgConfigLogLevel) >= level)) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_Console))) { \
        printf(header "%s\t", (const TChar*)gcConfigLogLevelTag[level]); \
        printf(format, ##args);  \
        printf(tail); \
        fflush(stdout); \
      } \
      if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_File))) { \
        if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t", (const TChar*)gcConfigLogLevelTag[level]); \
          fprintf(gpLogOutputFile, format,##args);  \
          fflush(gpLogOutputFile); \
        } \
        if (DUnlikely(gpSnapshotOutputFile)) { \
          fprintf(gpSnapshotOutputFile, "%s\t", (const TChar*)gcConfigLogLevelTag[level]); \
          fprintf(gpSnapshotOutputFile, format,##args);  \
          fflush(gpSnapshotOutputFile); \
        } \
      } \
    } \
  } while (0)

#define _LOG_LEVEL_MODULE_TRACE(level, header, tail, format, args...)  do { \
    if (DLikely(mConfigLogLevel >= level)) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
        printf(header "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], GetModuleName(), GetModuleIndex()); \
        printf(format, ##args);  \
        printf(tail); \
        fflush(stdout); \
        printf("\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
      } \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
        if (DUnlikely(mpLogOutputFile)) { \
          fprintf(mpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], GetModuleName(), GetModuleIndex()); \
          fprintf(mpLogOutputFile, format,##args);  \
          fprintf(mpLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
        } else if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], GetModuleName(), GetModuleIndex()); \
          fprintf(gpLogOutputFile, format,##args);  \
          fprintf(gpLogOutputFile, "\t\t\t\t\t[trace] file %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
          fflush(gpLogOutputFile); \
        } \
        if (DUnlikely(gpSnapshotOutputFile)) { \
          fprintf(gpSnapshotOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], GetModuleName(), GetModuleIndex()); \
          fprintf(gpSnapshotOutputFile, format,##args);  \
          fprintf(gpSnapshotOutputFile, "\t\t\t\t\t[trace] file %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
          fflush(gpSnapshotOutputFile); \
        } \
      } \
    } \
  } while (0)

#define _LOG_LEVEL_TRACE(level, header, tail, format, args...)  do { \
    if (DLikely((*pgConfigLogLevel) >= level)) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_Console))) { \
        printf(header "%s\t", (const TChar*)gcConfigLogLevelTag[level]); \
        printf(format, ##args);  \
        printf(tail); \
        fflush(stdout); \
        printf("\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
      } \
      if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_File))) { \
        if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t", (const TChar*)gcConfigLogLevelTag[level]); \
          fprintf(gpLogOutputFile, format,##args);  \
          fprintf(gpLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
          fflush(gpLogOutputFile); \
        } \
        if (DUnlikely(gpSnapshotOutputFile)) { \
          fprintf(gpSnapshotOutputFile, "%s\t", (const TChar*)gcConfigLogLevelTag[level]); \
          fprintf(gpSnapshotOutputFile, format,##args);  \
          fprintf(gpSnapshotOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
          fflush(gpSnapshotOutputFile); \
        } \
      } \
    } \
  } while (0)


#define _LOG_OPTION_MODULE(option, format, args...)  do { \
    if (DUnlikely(mConfigLogOption & DCAL_BITMASK(option))) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
        printf(_DLOGCOLOR_HEADER_OPTION "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], (const char*)GetModuleName(), GetModuleIndex()); \
        printf(format _DLOGCOLOR_TAIL_OPTION, ##args);  \
        fflush(stdout); \
      } \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
        if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(gpLogOutputFile, format,##args);  \
          fflush(gpLogOutputFile); \
        } \
      } \
      if (DUnlikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_ModuleFile))) { \
        if (DUnlikely(mpLogOutputFile)) { \
          fprintf(mpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(mpLogOutputFile, format,##args);  \
        } else if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], (const char*)GetModuleName(), GetModuleIndex()); \
          fprintf(gpLogOutputFile, format,##args);  \
          fflush(gpLogOutputFile); \
        } \
      } \
    } \
  } while (0)

#define _LOG_OPTION(option, format, args...)  do { \
    if (DUnlikely((*pgConfigLogOption) & DCAL_BITMASK(option))) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_Console))) { \
        printf(_DLOGCOLOR_HEADER_OPTION "%s\t", (const TChar*)gcConfigLogOptionTag[option]); \
        printf(format _DLOGCOLOR_TAIL_OPTION, ##args);  \
        fflush(stdout); \
      } \
      if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_File))) { \
        if(gpLogOutputFile) { \
          fprintf(gpLogOutputFile, "%s\t", (const TChar*)gcConfigLogOptionTag[option]); \
          fprintf(gpLogOutputFile, format,##args);  \
          fflush(gpLogOutputFile); \
        } \
      } \
    } \
  } while (0)

#define _LOG_OPTION_MODULE_TRACE(option, format, args...)  do { \
    if (DUnlikely(mConfigLogOption & DCAL_BITMASK(option))) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
        printf(_DLOGCOLOR_HEADER_OPTION "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], GetModuleName(), GetModuleIndex()); \
        printf(format _DLOGCOLOR_TAIL_OPTION, ##args);  \
        fflush(stdout); \
        printf("\t\t\t\t\t[trace]: file %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
      } \
      if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
        if (DUnlikely(mpLogOutputFile)) { \
          fprintf(mpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], GetModuleName(), GetModuleIndex()); \
          fprintf(mpLogOutputFile, format,##args);  \
          fprintf(mpLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
        } else if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], GetModuleName(), GetModuleIndex()); \
          fprintf(gpLogOutputFile, format,##args);  \
          fprintf(gpLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
          fflush(gpLogOutputFile); \
        } \
      } \
    } \
  } while (0)

#define _LOG_OPTION_TRACE(option, format, args...)  do { \
    if (DUnlikely(gConfigLogOption & DCAL_BITMASK(option))) { \
      _DLOG_GLOBAL_MUTEX_NAME; \
      if (DLikely(gConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
        printf(_DLOGCOLOR_HEADER_OPTION "%s\t", (const TChar*)gcConfigLogOptionTag[option]); \
        printf(format _DLOGCOLOR_TAIL_OPTION, ##args);  \
        fflush(stdout); \
        printf("\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
      } \
      if (DLikely(gConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
        if (DLikely(gpLogOutputFile)) { \
          fprintf(gpLogOutputFile, "%s\t", (const TChar*)gcConfigLogOptionTag[option]); \
          fprintf(gpLogOutputFile, format,##args);  \
          fprintf(gpLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
          fflush(gpLogOutputFile); \
        } \
      } \
    } \
  } while (0)

#define __CLEAN_PRINT(format, args...)  do { \
    printf(format, ##args);  \
    if (DLikely(gpLogOutputFile)) { \
      fprintf(gpLogOutputFile, format, ##args);  \
    } \
  } while (0)

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define LOGM_VERBOSE(format, args...)       _LOG_LEVEL_MODULE(ELogLevel_Verbose, _DLOGCOLOR_HEADER_DETAIL, _DLOGCOLOR_TAIL_DETAIL , format, ##args)
#define LOGM_DEBUG(format, args...)         _LOG_LEVEL_MODULE(ELogLevel_Debug, _DLOGCOLOR_HEADER_DETAIL, _DLOGCOLOR_TAIL_DETAIL, format, ##args)
#define LOGM_INFO(format, args...)          _LOG_LEVEL_MODULE(ELogLevel_Info, _DLOGCOLOR_HEADER_DETAIL, _DLOGCOLOR_TAIL_DETAIL, format, ##args)
#else
#define LOGM_VERBOSE        _NOLOG
#define LOGM_DEBUG          _NOLOG
#define LOGM_INFO           _NOLOG
#endif

#ifdef DCONFIG_LOG_SYSTEM_ENABLE_TRACE
#define LOGM_NOTICE(format, args...)        _LOG_LEVEL_MODULE_TRACE(ELogLevel_Notice, _DLOGCOLOR_HEADER_NOTICE, _DLOGCOLOR_TAIL_NOTICE, format, ##args)
#else
#define LOGM_NOTICE(format, args...)        _LOG_LEVEL_MODULE(ELogLevel_Notice, _DLOGCOLOR_HEADER_NOTICE, _DLOGCOLOR_TAIL_NOTICE, format, ##args)
#endif

#define LOGM_WARN(format, args...)          _LOG_LEVEL_MODULE_TRACE(ELogLevel_Warn, _DLOGCOLOR_HEADER_WARN, _DLOGCOLOR_TAIL_WARN, format, ##args)
#define LOGM_ERROR(format, args...)         _LOG_LEVEL_MODULE_TRACE(ELogLevel_Error, _DLOGCOLOR_HEADER_ERROR, _DLOGCOLOR_TAIL_ERROR, format, ##args)
#define LOGM_FATAL(format, args...)         _LOG_LEVEL_MODULE_TRACE(ELogLevel_Fatal, _DLOGCOLOR_HEADER_FATAL, _DLOGCOLOR_TAIL_FATAL, format , ##args)
#define LOGM_ALWAYS(format, args...)         _LOG_LEVEL_MODULE(ELogLevel_None, _DLOGCOLOR_HEADER_INFO, _DLOGCOLOR_TAIL_INFO, format, ##args)
#define LOGM_CONFIGURATION(format, args...)         _LOG_OPTION_MODULE(ELogOption_Congifuration, format, ##args)

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define LOG_VERBOSE(format, args...)        _LOG_LEVEL(ELogLevel_Verbose, _DLOGCOLOR_HEADER_DETAIL, _DLOGCOLOR_TAIL_DETAIL, format, ##args)
#define LOG_DEBUG(format, args...)          _LOG_LEVEL(ELogLevel_Debug, _DLOGCOLOR_HEADER_DETAIL, _DLOGCOLOR_TAIL_DETAIL, format, ##args)
#define LOG_INFO(format, args...)           _LOG_LEVEL(ELogLevel_Info, _DLOGCOLOR_HEADER_DETAIL, _DLOGCOLOR_TAIL_DETAIL, format, ##args)
#else
#define LOG_VERBOSE         _NOLOG
#define LOG_DEBUG           _NOLOG
#define LOG_INFO            _NOLOG
#endif

#ifdef DCONFIG_LOG_SYSTEM_ENABLE_TRACE
#define LOG_NOTICE(format, args...)         _LOG_LEVEL_TRACE(ELogLevel_Notice, _DLOGCOLOR_HEADER_NOTICE, _DLOGCOLOR_TAIL_NOTICE, format, ##args)
#else
#define LOG_NOTICE(format, args...)         _LOG_LEVEL(ELogLevel_Notice, _DLOGCOLOR_HEADER_NOTICE, _DLOGCOLOR_TAIL_NOTICE, format, ##args)
#endif

#define LOG_WARN(format, args...)           _LOG_LEVEL_TRACE(ELogLevel_Warn, _DLOGCOLOR_HEADER_WARN, _DLOGCOLOR_TAIL_WARN, format, ##args)
#define LOG_ERROR(format, args...)          _LOG_LEVEL_TRACE(ELogLevel_Error, _DLOGCOLOR_HEADER_ERROR, _DLOGCOLOR_TAIL_ERROR, format, ##args)
#define LOG_FATAL(format, args...)          _LOG_LEVEL_TRACE(ELogLevel_Fatal, _DLOGCOLOR_HEADER_FATAL, _DLOGCOLOR_TAIL_FATAL, format, ##args)
#define LOG_ALWAYS(format, args...)         _LOG_LEVEL(ELogLevel_None, _DLOGCOLOR_HEADER_INFO, _DLOGCOLOR_TAIL_INFO, format, ##args)
#define LOG_CONFIGURATION(format, args...)          _LOG_OPTION(ELogOption_Congifuration, format, ##args)

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define LOGM_STATE(format, args...)         _LOG_OPTION_MODULE(ELogOption_State, format, ##args)
#define LOGM_FLOW(format, args...)          _LOG_OPTION_MODULE(ELogOption_Flow, format, ##args)
#define LOGM_PTS(format, args...)           _LOG_OPTION_MODULE(ELogOption_PTS, format, ##args)
#define LOGM_RESOURCE(format, args...)      _LOG_OPTION_MODULE(ELogOption_Resource, format, ##args)
#define LOGM_TIMING(format, args...)        _LOG_OPTION_MODULE(ELogOption_Timing, format, ##args)
#define LOGM_BINARY(format, args...)        _LOG_OPTION_MODULE(ELogOption_BinaryHeader, format, ##args)
#else
#define LOGM_STATE          _NOLOG
#define LOGM_FLOW           _NOLOG
#define LOGM_PTS            _NOLOG
#define LOGM_RESOURCE       _NOLOG
#define LOGM_TIMING         _NOLOG
#define LOGM_BINARY         _NOLOG
#endif

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define LOG_STATE(format, args...)          _LOG_OPTION(ELogOption_State, format, ##args)
#define LOG_FLOW(format, args...)           _LOG_OPTION(ELogOption_Flow, format, ##args)
#define LOG_PTS(format, args...)            _LOG_OPTION(ELogOption_PTS, format, ##args)
#define LOG_RESOURCE(format, args...)       _LOG_OPTION(ELogOption_Resource, format, ##args)
#define LOG_TIMING(format, args...)         _LOG_OPTION(ELogOption_Timing, format, ##args)
#define LOG_BINARY(format, args...)         _LOG_OPTION(ELogOption_BinaryHeader, format, ##args)
#else
#define LOG_STATE           _NOLOG
#define LOG_FLOW            _NOLOG
#define LOG_PTS             _NOLOG
#define LOG_RESOURCE        _NOLOG
#define LOG_TIMING          _NOLOG
#define LOG_BINARY          _NOLOG
#endif

#define LOGM_PRINTF(format, args...)          _LOG_LEVEL_MODULE(ELogLevel_None, _DLOGCOLOR_HEADER_INFO, _DLOGCOLOR_TAIL_INFO, format, ##args)
#define LOG_PRINTF(format, args...)          _LOG_LEVEL(ELogLevel_None, _DLOGCOLOR_HEADER_INFO, _DLOGCOLOR_TAIL_INFO, format, ##args)

#define DASSERT(expr) do { \
    if (DUnlikely(!(expr))) { \
      printf(_DLOGCOLOR_HEADER_FATAL "assertion failed: %s\n\tAt file: %s.\n\tfunction: %s: line %d\n" _DLOGCOLOR_TAIL_FATAL, #expr, __FILE__, __FUNCTION__, __LINE__); \
      if (gpLogOutputFile) { \
        fprintf(gpLogOutputFile,"assertion failed: %s\n\tAt file: %s.\n\tfunction: %s, line %d\n", #expr, __FILE__, __FUNCTION__, __LINE__); \
        fflush(gpLogOutputFile); \
      } \
    } \
  } while (0)

#define DASSERT_OK(code) do { \
    if (DUnlikely(EECode_OK != code)) { \
      printf(_DLOGCOLOR_HEADER_FATAL "ERROR code %s\n\tAt file: %s.\n\tfunction: %s, line%d\n" _DLOGCOLOR_TAIL_FATAL, gfGetErrorCodeString(code), __FILE__, __FUNCTION__, __LINE__); \
      if (gpLogOutputFile) { \
        fprintf(gpLogOutputFile, "ERROR code %s\n\tAt file %s.\n\tfunction: %s, line %d\n", gfGetErrorCodeString(code), __FILE__, __FUNCTION__, __LINE__); \
        fflush(gpLogOutputFile); \
      } \
    } \
  } while (0)

#endif

#else

#include <stdarg.h>

extern void gfSubstitutionLogFatal(const TChar *log, ...);
extern void gfSubstitutionLogError(const TChar *log, ...);
extern void gfSubstitutionLogWarning(const TChar *log, ...);
extern void gfSubstitutionLogNotice(const TChar *log, ...);
extern void gfSubstitutionLogInfo(const TChar *log, ...);
extern void gfSubstitutionLogDebug(const TChar *log, ...);
extern void gfSubstitutionLogVerbose(const TChar *log, ...);
extern void gfSubstitutionLogPrintf(const TChar *log, ...);
extern void gfSubstitutionLogDetailedPrintf(const TChar *log, ...);
extern void gfExternSubstitutionLogPrintf(void *p_unused, int i_level, const TChar *log, va_list arg);

extern void gfSubstitutionAssertFail();


#define LOGM_FATAL gfSubstitutionLogFatal
#define LOGM_ERROR gfSubstitutionLogError
#define LOGM_WARN gfSubstitutionLogWarning
#define LOGM_NOTICE gfSubstitutionLogNotice

#define LOGM_INFO gfSubstitutionLogInfo
#define LOGM_DEBUG gfSubstitutionLogDebug
#define LOGM_VERBOSE gfSubstitutionLogVerbose

#define LOG_FATAL gfSubstitutionLogFatal
#define LOG_ERROR gfSubstitutionLogError
#define LOG_WARN gfSubstitutionLogWarning
#define LOG_NOTICE gfSubstitutionLogNotice

#define LOG_INFO gfSubstitutionLogInfo
#define LOG_DEBUG gfSubstitutionLogDebug
#define LOG_VERBOSE gfSubstitutionLogVerbose

#define LOGM_STATE           gfSubstitutionLogDetailedPrintf
#define LOGM_CONFIGURATION           gfSubstitutionLogPrintf
#define LOGM_FLOW            gfSubstitutionLogDetailedPrintf
#define LOGM_PTS             gfSubstitutionLogDetailedPrintf
#define LOGM_RESOURCE        gfSubstitutionLogDetailedPrintf
#define LOGM_TIMING          gfSubstitutionLogDetailedPrintf
#define LOGM_BINARY          gfSubstitutionLogDetailedPrintf

#define LOG_STATE           gfSubstitutionLogDetailedPrintf
#define LOG_CONFIGURATION           gfSubstitutionLogPrintf
#define LOG_FLOW            gfSubstitutionLogDetailedPrintf
#define LOG_PTS             gfSubstitutionLogDetailedPrintf
#define LOG_RESOURCE        gfSubstitutionLogDetailedPrintf
#define LOG_TIMING          gfSubstitutionLogDetailedPrintf
#define LOG_BINARY          gfSubstitutionLogDetailedPrintf

#define LOGM_PRINTF     gfSubstitutionLogPrintf
#define LOG_PRINTF     gfSubstitutionLogPrintf

#define LOGM_ALWAYS     gfSubstitutionLogPrintf
#define LOG_ALWAYS     gfSubstitutionLogPrintf

#define DASSERT(expr) do { \
    if (DUnlikely(!(expr))) { \
      gfSubstitutionAssertFail(); \
    } \
  } while (0)

#define DASSERT_OK(code) do { \
    if (DUnlikely(EECode_OK != code)) { \
      gfSubstitutionAssertFail(); \
    } \
  } while (0)

#endif

//module index, for log.config
typedef enum {
  //global settings
  ELogGlobal = 0,

  ELogModuleListStart,

  //engines
  ELogModuleGenericEngine = ELogModuleListStart, //first line must not be modified
  ELogModuleStreamingServerManager,
  ELogModuleCloudServerManager,
  ELogModuleIMServerManager,

  //filters
  ELogModuleDemuxerFilter,
  ELogModuleVideoCapturerFilter,
  ELogModuleAudioCapturerFilter,
  ELogModuleVideoDecoderFilter,
  ELogModuleAudioDecoderFilter,
  ELogModuleVideoRendererFilter,
  ELogModuleAudioRendererFilter,
  ELogModuleVideoPostPFilter,
  ELogModuleAudioPostPFilter,
  ELogModuleVideoOutputFilter,
  ELogModuleAudioOutputFilter,
  ELogModuleMuxerFilter,
  ELogModuleStreamingTransmiterFilter,
  ELogModuleVideoEncoderFilter,
  ELogModuleAudioEncoderFilter,
  ELogModuleConnecterPinmuxer,
  ELogModuleCloudConnecterServerAgent,
  ELogModuleCloudConnecterClientAgent,
  ELogModuleFlowController,
  ELogModuleExternalSourceVideoEs,

  //modules
  ELogModulePrivateRTSPDemuxer,
  ELogModulePrivateRTSPScheduledDemuxer,
  ELogModuleUploadingReceiver,
  ELogModuleAmbaDecoder,
  ELogModuleAmbaVideoPostProcessor,
  ELogModuleAmbaVideoRenderer,
  ELogModuleAmbaEncoder,
  ELogModuleStreamingTransmiter,
  ELogModuleFFMpegDemuxer,
  ELogModuleFFMpegMuxer,
  ELogModuleFFMpegAudioDecoder,
  ELogModuleFFMpegVideoDecoder,
  ELogModuleAudioRenderer,
  ELogModuleVideoInput,
  ELogModuleAudioInput,
  ELogModuleAACAudioDecoder,
  ELogModuleAACAudioEncoder,
  ELogModuleCustomizedAudioDecoder,
  ELogModuleCustomizedAudioEncoder,
  ELogModuleNativeTSMuxer,
  ELogModuleNativeTSDemuxer,
  ELogModuleNativeMP4Muxer,
  ELogModuleNativeMP4Demuxer,
  ELogModuleRTSPStreamingServer,
  ELogModuleCloudServer,
  ELogModuleIMServer,
  ELogModuleDirectSharingServer,

  ELogModuleOPTCVideoEncoder,
  ELogModuleOPTCAudioEncoder,
  ELogModuleOPTCVideoDecoder,
  ELogModuleOPTCAudioDecoder,

  //dsp platform
  ELogModuleDSPPlatform,
  ELogModuleDSPDec,
  ELogModuleDSPEnc,

  //scheduler
  ELogModuleRunRobinScheduler,
  ELogModulePreemptiveScheduler,

  //thread pool
  ELogModuleThreadPool,

  //data base
  ELogModuleLightWeightDataBase,

  //account manager
  ELogModuleAccountManager,

  //protocol agent
  ELogModuleSACPAdminAgent,
  ELogModuleSACPIMAgent,

  //app framework
  ELogAPPFrameworkSceneDirector,
  ELogAPPFrameworkSoundComposer,

  //platform al
  ELogPlatformALSoundOutput,

  ELogModuleListEnd,
} ELogModule;

typedef struct {
  TUint log_level, log_option, log_output;
} SLogConfig;

extern SLogConfig gmModuleLogConfig[ELogModuleListEnd];

#define DSET_MODULE_LOG_CONFIG(x) do { \
    SetLogLevel(gmModuleLogConfig[x].log_level); \
    SetLogOption(gmModuleLogConfig[x].log_option); \
    SetLogOutput(gmModuleLogConfig[x].log_output); \
  } while (0)

#define DLOG_MASK_TO_SNAPSHOT (1 << 4)
#define DLOG_MASK_TO_REMOTE (1 << 5)

extern void gfPrintMemory(TU8 *p, TU32 size);

extern EECode gfGetLogLevelString(ELogLevel level, const TChar *&loglevel_string);
extern EECode gfGetLogModuleString(ELogModule module, const TChar *&module_string);
extern EECode gfGetLogModuleIndex(const TChar *module_name, TUint &index);

extern void gfOpenSnapshotFile(TChar *name);
extern void gfCloseSnapshotFile();

#if 0
#define DDBG_MALLOC(size, tag) gfDbgMalloc(size, tag)
#define DDBG_FREE(p, tag) gfDbgFree(p, tag)
#define DDBG_CALLOC(n, size, tag) gfDbgCalloc(n, size, tag)
#define DDBG_REMALLOC(p, size, tag) gfDbgRemalloc(p, size, tag)
#else
#define DDBG_MALLOC(size, tag) malloc(size)
#define DDBG_FREE(p, tag) free(p)
#define DDBG_CALLOC(n, size, tag) calloc(n, size)
#define DDBG_REMALLOC(p, size, tag) realloc(p, size)
#endif

extern void *gfDbgMalloc(TU32 size, const TChar *tag);
extern void gfDbgFree(void *p, const TChar *tag);
extern void *gfDbgCalloc(TU32 n, TU32 size, const TChar *tag);
extern void *gfDbgRemalloc(void *p, TU32 size, const TChar *tag);
#endif

