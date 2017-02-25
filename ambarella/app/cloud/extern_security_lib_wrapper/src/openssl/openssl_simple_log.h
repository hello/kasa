/*******************************************************************************
 * openssl_simple_log.h
 *
 * History:
 *  2015/05/18 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
 ******************************************************************************/

#ifndef __OPENSSL_SIMPLE_LOG_H__
#define __OPENSSL_SIMPLE_LOG_H__

//-----------------------------------------------------------------------
//
//  log system
//
//-----------------------------------------------------------------------

#define DOPENSSL_MAX_LOGTAG_NAME_LEN 64

enum EOpenSSLSimpleLogLevel {
    EOpenSSLSimpleLogLevel_None = 0,
    EOpenSSLSimpleLogLevel_Fatal,
    EOpenSSLSimpleLogLevel_Error,
    EOpenSSLSimpleLogLevel_Warn,
    EOpenSSLSimpleLogLevel_Notice,
    EOpenSSLSimpleLogLevel_Info,
    EOpenSSLSimpleLogLevel_Debug,
    EOpenSSLSimpleLogLevel_Verbose,

    //last element
    EOpenSSLSimpleLogLevel_TotalCount,
};

#define DOPENSSLLOG_OUTPUT_FILE 0x1
#define DOPENSSLLOG_OUTPUT_CONSOLE 0x2

extern volatile unsigned long gOpenSSLConfigLogLevel;
extern volatile unsigned long gOpenSSLConfigLogOutput;

extern const char gOpenSSLConfigLogLevelTag[EOpenSSLSimpleLogLevel_TotalCount][DOPENSSL_MAX_LOGTAG_NAME_LEN];
extern FILE *gpOpenSSLSimpleLogOutputFile;
//-----------------------------------------------------------------------
//
//  log macros
//
//-----------------------------------------------------------------------

#define _DOPENSSL_SIMPLE_NOLOG(format, ...) (void)0

#ifdef BUILD_OS_WINDOWS
#define _DOPENSSL_SIMPLE_LOG_LEVEL(level, format, ...)  do { \
        if (gOpenSSLConfigLogLevel >= level) { \
            if (gOpenSSLConfigLogOutput & DOPENSSLLOG_OUTPUT_CONSOLE) { \
                if (g_log_output) { \
                    BIO_printf(g_log_output, "%s\t", (const char*)gOpenSSLConfigLogLevelTag[level]); \
                    BIO_printf(g_log_output, format, __VA_ARGS__);  \
                    ERR_print_errors(g_log_output); \
                } else { \
                    printf("%s\t", (const char*)gOpenSSLConfigLogLevelTag[level]); \
                    printf(format, __VA_ARGS__);  \
                } \
            } \
            if (gOpenSSLConfigLogOutput & DOPENSSLLOG_OUTPUT_FILE) { \
                if (gpOpenSSLSimpleLogOutputFile) { \
                    fprintf(gpOpenSSLSimpleLogOutputFile, "%s\t", (const char*)gOpenSSLConfigLogLevelTag[level]); \
                    fprintf(gpOpenSSLSimpleLogOutputFile, format, __VA_ARGS__);  \
                    fflush(gpOpenSSLSimpleLogOutputFile); \
                } \
            } \
        } \
    } while (0)

#define _DOPENSSL_SIMPLE_LOG_LEVEL_TRACE(level, format, ...)  do { \
        if (gOpenSSLConfigLogLevel >= level) { \
            if (gOpenSSLConfigLogOutput & DOPENSSLLOG_OUTPUT_CONSOLE) { \
                if (g_log_output) { \
                    BIO_printf(g_log_output, "%s\t", (const char*)gOpenSSLConfigLogLevelTag[level]); \
                    BIO_printf(g_log_output, format, __VA_ARGS__);  \
                    BIO_printf(g_log_output, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                    ERR_print_errors(g_log_output); \
                } else { \
                    printf("%s\t", (const char*)gOpenSSLConfigLogLevelTag[level]); \
                    printf(format, __VA_ARGS__);  \
                    printf("\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                } \
            } \
            if (gOpenSSLConfigLogOutput & DOPENSSLLOG_OUTPUT_FILE) { \
                if (gpOpenSSLSimpleLogOutputFile) { \
                    fprintf(gpOpenSSLSimpleLogOutputFile, "%s\t", (const char*)gOpenSSLConfigLogLevelTag[level]); \
                    fprintf(gpOpenSSLSimpleLogOutputFile, format, __VA_ARGS__);  \
                    fprintf(gpOpenSSLSimpleLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                    fflush(gpOpenSSLSimpleLogOutputFile); \
                } \
            } \
        } \
    } while (0)

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define DOPENSSL_LOG_VERBOSE(format, ...)        _DOPENSSL_SIMPLE_LOG_LEVEL(EOpenSSLSimpleLogLevel_Verbose, format, __VA_ARGS__)
#define DOPENSSL_LOG_DEBUG(format, ...)          _DOPENSSL_SIMPLE_LOG_LEVEL(EOpenSSLSimpleLogLevel_Debug, format, __VA_ARGS__)
#define DOPENSSL_LOG_INFO(format, ...)           _DOPENSSL_SIMPLE_LOG_LEVEL(EOpenSSLSimpleLogLevel_Info, format, __VA_ARGS__)
#else
#define DOPENSSL_LOG_VERBOSE         _DOPENSSL_SIMPLE_NOLOG
#define DOPENSSL_LOG_DEBUG           _DOPENSSL_SIMPLE_NOLOG
#define DOPENSSL_LOG_INFO            _DOPENSSL_SIMPLE_NOLOG
#endif

#define DOPENSSL_LOG_NOTICE(format, ...)         _DOPENSSL_SIMPLE_LOG_LEVEL_TRACE(EOpenSSLSimpleLogLevel_Notice, format, __VA_ARGS__)
#define DOPENSSL_LOG_WARN(format, ...)           _DOPENSSL_SIMPLE_LOG_LEVEL_TRACE(EOpenSSLSimpleLogLevel_Warn, format, __VA_ARGS__)
#define DOPENSSL_LOG_ERROR(format, ...)          _DOPENSSL_SIMPLE_LOG_LEVEL_TRACE(EOpenSSLSimpleLogLevel_Error, format, __VA_ARGS__)
#define DOPENSSL_LOG_FATAL(format, ...)          _DOPENSSL_SIMPLE_LOG_LEVEL_TRACE(EOpenSSLSimpleLogLevel_Fatal, format, __VA_ARGS__)

#define DOPENSSL_LOG_PRINTF(format, ...)          _DOPENSSL_SIMPLE_LOG_LEVEL(EOpenSSLSimpleLogLevel_None, format, __VA_ARGS__)

#else

#define _DOPENSSL_SIMPLE_LOG_LEVEL(level, format, args...)  do { \
        if (gOpenSSLConfigLogLevel >= level) { \
            if (gOpenSSLConfigLogOutput & DOPENSSLLOG_OUTPUT_CONSOLE) { \
                if (g_log_output) { \
                    BIO_printf(g_log_output, "%s\t", (const char*)gOpenSSLConfigLogLevelTag[level]); \
                    BIO_printf(g_log_output, format, ##args);  \
                    ERR_print_errors(g_log_output); \
                } else { \
                    printf("%s\t", (const char*)gOpenSSLConfigLogLevelTag[level]); \
                    printf(format, ##args);  \
                } \
            } \
            if (gOpenSSLConfigLogOutput & DOPENSSLLOG_OUTPUT_FILE) { \
                if (gpOpenSSLSimpleLogOutputFile) { \
                    fprintf(gpOpenSSLSimpleLogOutputFile, "%s\t", (const char*)gOpenSSLConfigLogLevelTag[level]); \
                    fprintf(gpOpenSSLSimpleLogOutputFile, format, ##args);  \
                    fflush(gpOpenSSLSimpleLogOutputFile); \
                } \
            } \
        } \
    } while (0)

#define _DOPENSSL_SIMPLE_LOG_LEVEL_TRACE(level, format, args...)  do { \
        if (gOpenSSLConfigLogLevel >= level) { \
            if (gOpenSSLConfigLogOutput & DOPENSSLLOG_OUTPUT_CONSOLE) { \
                if (g_log_output) { \
                    BIO_printf(g_log_output, "%s\t", (const char*)gOpenSSLConfigLogLevelTag[level]); \
                    BIO_printf(g_log_output, format, ##args);  \
                    BIO_printf(g_log_output, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                    ERR_print_errors(g_log_output); \
                } else { \
                    printf("%s\t", (const char*)gOpenSSLConfigLogLevelTag[level]); \
                    printf(format, ##args);  \
                    printf("\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                } \
            } \
            if (gOpenSSLConfigLogOutput & DOPENSSLLOG_OUTPUT_FILE) { \
                if (gpOpenSSLSimpleLogOutputFile) { \
                    fprintf(gpOpenSSLSimpleLogOutputFile, "%s\t", (const char*)gOpenSSLConfigLogLevelTag[level]); \
                    fprintf(gpOpenSSLSimpleLogOutputFile, format, ##args);  \
                    fprintf(gpOpenSSLSimpleLogOutputFile, "\t\t\t\t\t[trace] file: %s.\n\t\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                    fflush(gpOpenSSLSimpleLogOutputFile); \
                } \
            } \
        } \
    } while (0)

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define DOPENSSL_LOG_VERBOSE(format, args...)        _DOPENSSL_SIMPLE_LOG_LEVEL(EOpenSSLSimpleLogLevel_Verbose, format, ##args)
#define DOPENSSL_LOG_DEBUG(format, args...)          _DOPENSSL_SIMPLE_LOG_LEVEL(EOpenSSLSimpleLogLevel_Debug, format, ##args)
#define DOPENSSL_LOG_INFO(format, args...)           _DOPENSSL_SIMPLE_LOG_LEVEL(EOpenSSLSimpleLogLevel_Info, format, ##args)
#else
#define DOPENSSL_LOG_VERBOSE         _DOPENSSL_SIMPLE_NOLOG
#define DOPENSSL_LOG_DEBUG           _DOPENSSL_SIMPLE_NOLOG
#define DOPENSSL_LOG_INFO            _DOPENSSL_SIMPLE_NOLOG
#endif

#define DOPENSSL_LOG_NOTICE(format, args...)         _DOPENSSL_SIMPLE_LOG_LEVEL_TRACE(EOpenSSLSimpleLogLevel_Notice, format, ##args)
#define DOPENSSL_LOG_WARN(format, args...)           _DOPENSSL_SIMPLE_LOG_LEVEL_TRACE(EOpenSSLSimpleLogLevel_Warn, format, ##args)
#define DOPENSSL_LOG_ERROR(format, args...)          _DOPENSSL_SIMPLE_LOG_LEVEL_TRACE(EOpenSSLSimpleLogLevel_Error, format, ##args)
#define DOPENSSL_LOG_FATAL(format, args...)          _DOPENSSL_SIMPLE_LOG_LEVEL_TRACE(EOpenSSLSimpleLogLevel_Fatal, format, ##args)

#define DOPENSSL_LOG_PRINTF(format, args...)          _DOPENSSL_SIMPLE_LOG_LEVEL(EOpenSSLSimpleLogLevel_None, format, ##args)

#endif

#define DOPENSSL_ASSERT(expr) do { \
        if (!(expr)) { \
            if (g_log_output) { \
                BIO_printf(g_log_output, "assertion failed: %s\n\tAt file: %s\n\tfunction: %s, line %d\n", #expr, __FILE__, __FUNCTION__, __LINE__); \
                ERR_print_errors(g_log_output); \
            } else { \
                printf("assertion failed: %s\n\tAt file: %s\n\tfunction: %s, line %d\n", #expr, __FILE__, __FUNCTION__, __LINE__); \
            } \
            if (gpOpenSSLSimpleLogOutputFile) { \
                fprintf(gpOpenSSLSimpleLogOutputFile,"assertion failed: %s\n\tAt file: %s\n\tfunction: %s: line %d\n", #expr, __FILE__, __FUNCTION__, __LINE__); \
                fflush(gpOpenSSLSimpleLogOutputFile); \
            } \
        } \
    } while (0)

extern unsigned int isOpenSSLSimpleLogFileOpened();
extern void gfOpenSSOpenSimpleLogFile(char *name);
extern void gfOpenSSCloseSimpleLogFile();
extern void gfOpenSSSetSimpleLogLevel(EOpenSSLSimpleLogLevel level);

#endif

