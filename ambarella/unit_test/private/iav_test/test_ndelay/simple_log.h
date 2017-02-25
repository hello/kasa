/**
 * simple_log.h
 *
 * History:
 *	2015/08/31 - [Zhi He] create file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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
 *
 */

#ifndef __SIMPLE_LOG_H__
#define __SIMPLE_LOG_H__

//#define BUILD_OS_WINDOWS

//-----------------------------------------------------------------------
//
//  log system
//
//-----------------------------------------------------------------------
#define DCONFIG_USE_NATIVE_LOG_SYSTEM
#define DCONFIG_ENALDE_DEBUG_LOG
#define DMAX_LOGTAG_NAME_LEN 64

#define DCAL_BITMASK(x) (1 << x)

typedef enum {
    ELogLevel_None = 0,
    ELogLevel_Fatal,
    ELogLevel_Error,
    ELogLevel_Warn,
    ELogLevel_Notice,
    ELogLevel_Info,
    ELogLevel_Debug,
    ELogLevel_Verbose,

    //last element
    ELogLevel_TotalCount,
} ELogLevel;

typedef enum {
    ELogOutput_Console = 0,
    ELogOutput_File,

    //last element
    ELogOutput_TotalCount,
} ELogOutput;

extern volatile unsigned int* const pgConfigLogLevel;
extern volatile unsigned int* const pgConfigLogOutput;

extern const char gcConfigLogLevelTag[ELogLevel_TotalCount][DMAX_LOGTAG_NAME_LEN];
extern const char gcConfigLogOutputTag[ELogOutput_TotalCount][DMAX_LOGTAG_NAME_LEN];

extern FILE* gpLogOutputFile;

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

#define _NOLOG(format, ...)	(void)0

#define _LOG_LEVEL(level, format, ...)  do { \
    if ((*pgConfigLogLevel) >= level) { \
        _DLOG_GLOBAL_MUTEX_NAME; \
        if ((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_Console)) { \
            printf("%s\t", (const char*)gcConfigLogLevelTag[level]); \
            printf(format, __VA_ARGS__);  \
        } \
        if ((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_File)) { \
            if (gpLogOutputFile) { \
                fprintf(gpLogOutputFile, "%s\t", (const char*)gcConfigLogLevelTag[level]); \
                fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
                fflush(gpLogOutputFile); \
            } \
        } \
    } \
} while (0)

#define _LOG_LEVEL_TRACE(level, format, ...)  do { \
    if ((*pgConfigLogLevel) >= level) { \
        _DLOG_GLOBAL_MUTEX_NAME; \
        if ((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_Console)) { \
            printf("%s\t", (const char*)gcConfigLogLevelTag[level]); \
            printf(format, __VA_ARGS__);  \
            printf("\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
        } \
        if ((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_File)) { \
            if (gpLogOutputFile) { \
                fprintf(gpLogOutputFile, "%s\t", (const char*)gcConfigLogLevelTag[level]); \
                fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
                fprintf(gpLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                fflush(gpLogOutputFile); \
            } \
        } \
    } \
} while (0)

#define __CLEAN_PRINT(format, ...)  do { \
    printf(format, __VA_ARGS__);  \
    if (gpLogOutputFile) { \
        fprintf(gpLogOutputFile, format, __VA_ARGS__);  \
    } \
} while (0)

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
#define LOG_PRINTF(format, ...)          _LOG_LEVEL(ELogLevel_None, format, __VA_ARGS__)

#define DASSERT(expr) do { \
    if (!(expr)) { \
        printf("assertion failed: %s\n\tAt file: %s\n\tfunction: %s, line %d\n", #expr, __FILE__, __FUNCTION__, __LINE__); \
        if (gpLogOutputFile) { \
            fprintf(gpLogOutputFile,"assertion failed: %s\n\tAt file: %s\n\tfunction: %s: line %d\n", #expr, __FILE__, __FUNCTION__, __LINE__); \
            fflush(gpLogOutputFile); \
        } \
    } \
} while (0)

#define DASSERT_OK(code) do { \
    if (EECode_OK != code) { \
        printf("ERROR code %s\n\tAt file: %s\n\tfunction: %s, line %d\n", gfGetErrorCodeString(code), __FILE__, __FUNCTION__, __LINE__); \
        if (gpLogOutputFile) { \
            fprintf(gpLogOutputFile, "ERROR code %s\n\tAt file: %s\n\tfunction: %s, line %d\n", gfGetErrorCodeString(code), __FILE__, __FUNCTION__, __LINE__); \
            fflush(gpLogOutputFile); \
        } \
    } \
} while (0)

#else

#define _NOLOG(format, args...)	(void)0

#define _LOG_LEVEL(level, header, tail, format, args...)  do { \
    if ((*pgConfigLogLevel) >= level) { \
        _DLOG_GLOBAL_MUTEX_NAME; \
        if ((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_Console)) { \
            printf(header "%s\t", (const char*)gcConfigLogLevelTag[level]); \
            printf(format, ##args);  \
            printf(tail); \
        } \
        if ((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_File)) { \
            if (gpLogOutputFile) { \
                fprintf(gpLogOutputFile, "%s\t", (const char*)gcConfigLogLevelTag[level]); \
                fprintf(gpLogOutputFile, format,##args);  \
                fflush(gpLogOutputFile); \
            } \
        } \
    } \
} while (0)

#define _LOG_LEVEL_TRACE(level, header, tail, format, args...)  do { \
    if ((*pgConfigLogLevel) >= level) { \
        _DLOG_GLOBAL_MUTEX_NAME; \
        if ((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_Console)) { \
            printf(header "%s\t", (const char*)gcConfigLogLevelTag[level]); \
            printf(format, ##args);  \
            printf(tail); \
            printf("\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
        } \
        if ((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_File)) { \
            if (gpLogOutputFile) { \
                fprintf(gpLogOutputFile, "%s\t", (const char*)gcConfigLogLevelTag[level]); \
                fprintf(gpLogOutputFile, format,##args);  \
                fprintf(gpLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                fflush(gpLogOutputFile); \
            } \
        } \
    } \
} while (0)

#define __CLEAN_PRINT(format, args...)  do { \
    printf(format, ##args);  \
    if (gpLogOutputFile) { \
        fprintf(gpLogOutputFile, format, ##args);  \
    } \
} while (0)

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

#define LOG_PRINTF(format, args...)          _LOG_LEVEL(ELogLevel_None, _DLOGCOLOR_HEADER_INFO, _DLOGCOLOR_TAIL_INFO, format, ##args)

#define DASSERT(expr) do { \
    if (!(expr)) { \
        printf(_DLOGCOLOR_HEADER_FATAL "assertion failed: %s\n\tAt file: %s.\n\tfunction: %s: line %d\n" _DLOGCOLOR_TAIL_FATAL, #expr, __FILE__, __FUNCTION__, __LINE__); \
        if (gpLogOutputFile) { \
            fprintf(gpLogOutputFile,"assertion failed: %s\n\tAt file: %s.\n\tfunction: %s, line %d\n", #expr, __FILE__, __FUNCTION__, __LINE__); \
            fflush(gpLogOutputFile); \
        } \
    } \
} while (0)

#define DASSERT_OK(code) do { \
    if (EECode_OK != code) { \
        printf(_DLOGCOLOR_HEADER_FATAL "ERROR code %s\n\tAt file: %s.\n\tfunction: %s, line%d\n" _DLOGCOLOR_TAIL_FATAL, gfGetErrorCodeString(code), __FILE__, __FUNCTION__, __LINE__); \
        if (gpLogOutputFile) { \
            fprintf(gpLogOutputFile, "ERROR code %s\n\tAt file %s.\n\tfunction: %s, line %d\n", gfGetErrorCodeString(code), __FILE__, __FUNCTION__, __LINE__); \
            fflush(gpLogOutputFile); \
        } \
    } \
} while (0)

#endif

extern unsigned int isLogFileOpened();
extern void gfOpenLogFile(char* name);
extern void gfCloseLogFile();
extern void gfSetLogLevel(ELogLevel level);

#endif


