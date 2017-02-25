/*******************************************************************************
 * common_substitution_log.cpp
 *
 * History:
 *  2014/09/14 - [Zhi He] create file
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

#include "common_config.h"

#include "common_types.h"
#include "common_osal.h"

#include "common_log.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#ifndef DCONFIG_USE_NATIVE_LOG_SYSTEM

#define DMAX_SUBSTITUTION_BUFFER_LENGTH 8192
static FILE *gpSubstitutionLogOutputFile = NULL;
static FILE *gpSnapshotSubstitutionOutputFile = NULL;
static IMutex *gpSubstitutionLogMutex = NULL;
static ELogLevel gSubstitutionLogLevel = ELogLevel_Notice;
static TChar gpSubstitutionLogBuffer[DMAX_SUBSTITUTION_BUFFER_LENGTH + 4] = {0};

static TU32 gnSubstitutionTrackerCount = 0;
TU32 gSubstitutionTracker()
{
    return gnSubstitutionTrackerCount ++;
}

void gfSubstitutionLogFatal(const TChar *log, ...)
{
    AUTO_LOCK(gpSubstitutionLogMutex);

    if (DLikely((gpSubstitutionLogOutputFile) && (gSubstitutionLogLevel >= ELogLevel_Fatal))) {
        va_list vlist;
        TInt length = 0;
        va_start(vlist, log);
        length = vsnprintf(gpSubstitutionLogBuffer, DMAX_SUBSTITUTION_BUFFER_LENGTH, log, vlist);
        va_end(vlist);
        gpSubstitutionLogBuffer[length] = 0x0;
        fprintf(gpSubstitutionLogOutputFile, "%s", gpSubstitutionLogBuffer);
        fflush(gpSubstitutionLogOutputFile);
    }

    gSubstitutionTracker();
}

void gfSubstitutionLogError(const TChar *log, ...)
{
    AUTO_LOCK(gpSubstitutionLogMutex);

    if (DLikely((gpSubstitutionLogOutputFile) && (gSubstitutionLogLevel >= ELogLevel_Error))) {
        va_list vlist;
        TInt length = 0;
        va_start(vlist, log);
        length = vsnprintf(gpSubstitutionLogBuffer, DMAX_SUBSTITUTION_BUFFER_LENGTH, log, vlist);
        va_end(vlist);
        gpSubstitutionLogBuffer[length] = 0x0;
        fprintf(gpSubstitutionLogOutputFile, "%s", gpSubstitutionLogBuffer);
        fflush(gpSubstitutionLogOutputFile);
    }

    gSubstitutionTracker();
}

void gfSubstitutionLogWarning(const TChar *log, ...)
{
    AUTO_LOCK(gpSubstitutionLogMutex);

    if (DLikely((gpSubstitutionLogOutputFile) && (gSubstitutionLogLevel >= ELogLevel_Warn))) {
        va_list vlist;
        TInt length = 0;
        va_start(vlist, log);
        length = vsnprintf(gpSubstitutionLogBuffer, DMAX_SUBSTITUTION_BUFFER_LENGTH, log, vlist);
        va_end(vlist);
        gpSubstitutionLogBuffer[length] = 0x0;
        fprintf(gpSubstitutionLogOutputFile, "%s", gpSubstitutionLogBuffer);
        fflush(gpSubstitutionLogOutputFile);
    }

    //gSubstitutionTracker();
}

void gfSubstitutionLogNotice(const TChar *log, ...)
{
    AUTO_LOCK(gpSubstitutionLogMutex);

    if (DLikely((gpSubstitutionLogOutputFile) && (gSubstitutionLogLevel >= ELogLevel_Notice))) {
        va_list vlist;
        TInt length = 0;
        va_start(vlist, log);
        length = vsnprintf(gpSubstitutionLogBuffer, DMAX_SUBSTITUTION_BUFFER_LENGTH, log, vlist);
        va_end(vlist);
        gpSubstitutionLogBuffer[length] = 0x0;
        fprintf(gpSubstitutionLogOutputFile, "%s", gpSubstitutionLogBuffer);
        fflush(gpSubstitutionLogOutputFile);
    }
}

void gfSubstitutionLogInfo(const TChar *log, ...)
{
    AUTO_LOCK(gpSubstitutionLogMutex);

    if (DLikely((gpSubstitutionLogOutputFile) && (gSubstitutionLogLevel >= ELogLevel_Info))) {
        va_list vlist;
        TInt length = 0;
        va_start(vlist, log);
        length = vsnprintf(gpSubstitutionLogBuffer, DMAX_SUBSTITUTION_BUFFER_LENGTH, log, vlist);
        va_end(vlist);
        gpSubstitutionLogBuffer[length] = 0x0;
        fprintf(gpSubstitutionLogOutputFile, "%s", gpSubstitutionLogBuffer);
        fflush(gpSubstitutionLogOutputFile);
    }
}

void gfSubstitutionLogDebug(const TChar *log, ...)
{
    AUTO_LOCK(gpSubstitutionLogMutex);

    if (DLikely((gpSubstitutionLogOutputFile) && (gSubstitutionLogLevel >= ELogLevel_Debug))) {
        va_list vlist;
        TInt length = 0;
        va_start(vlist, log);
        length = vsnprintf(gpSubstitutionLogBuffer, DMAX_SUBSTITUTION_BUFFER_LENGTH, log, vlist);
        va_end(vlist);
        gpSubstitutionLogBuffer[length] = 0x0;
        fprintf(gpSubstitutionLogOutputFile, "%s", gpSubstitutionLogBuffer);
        fflush(gpSubstitutionLogOutputFile);
    }
}

void gfSubstitutionLogVerbose(const TChar *log, ...)
{
    AUTO_LOCK(gpSubstitutionLogMutex);

    if (DLikely((gpSubstitutionLogOutputFile) && (gSubstitutionLogLevel >= ELogLevel_Verbose))) {
        va_list vlist;
        TInt length = 0;
        va_start(vlist, log);
        length = vsnprintf(gpSubstitutionLogBuffer, DMAX_SUBSTITUTION_BUFFER_LENGTH, log, vlist);
        va_end(vlist);
        gpSubstitutionLogBuffer[length] = 0x0;
        fprintf(gpSubstitutionLogOutputFile, "%s", gpSubstitutionLogBuffer);
        fflush(gpSubstitutionLogOutputFile);
    }
}

void gfSubstitutionLogPrintf(const TChar *log, ...)
{
    AUTO_LOCK(gpSubstitutionLogMutex);

    if (DLikely(gpSubstitutionLogOutputFile)) {
        va_list vlist;
        TInt length = 0;
        va_start(vlist, log);
        length = vsnprintf(gpSubstitutionLogBuffer, DMAX_SUBSTITUTION_BUFFER_LENGTH, log, vlist);
        va_end(vlist);
        gpSubstitutionLogBuffer[length] = 0x0;
        fprintf(gpSubstitutionLogOutputFile, "%s", gpSubstitutionLogBuffer);
        fflush(gpSubstitutionLogOutputFile);
    }
}

void gfSubstitutionLogDetailedPrintf(const TChar *log, ...)
{
    AUTO_LOCK(gpSubstitutionLogMutex);

    if (DLikely(0 && gpSubstitutionLogOutputFile)) {
        va_list vlist;
        TInt length = 0;
        va_start(vlist, log);
        length = vsnprintf(gpSubstitutionLogBuffer, DMAX_SUBSTITUTION_BUFFER_LENGTH, log, vlist);
        va_end(vlist);
        gpSubstitutionLogBuffer[length] = 0x0;
        fprintf(gpSubstitutionLogOutputFile, "%s", gpSubstitutionLogBuffer);
        fflush(gpSubstitutionLogOutputFile);
    }
}

void gfExternSubstitutionLogPrintf(void *p_unused, int i_level, const TChar *log, va_list arg)
{
    AUTO_LOCK(gpSubstitutionLogMutex);

    switch (i_level) {
        case 0: //X264_LOG_ERROR:
            break;
        case 1: //X264_LOG_WARNING:
            break;
        case 2: //X264_LOG_INFO:
            break;
        case 3: //X264_LOG_DEBUG:
            break;
        default:
            break;
    }
#if 0
    if (DLikely(0 && gpSubstitutionLogOutputFile)) {
        va_list vlist;
        TInt length = 0;
        va_start(vlist, log);
        length = vsnprintf(gpSubstitutionLogBuffer, DMAX_SUBSTITUTION_BUFFER_LENGTH, log, vlist);
        va_end(vlist);
        gpSubstitutionLogBuffer[length] = 0x0;
        fprintf(gpSubstitutionLogOutputFile, "%s", gpSubstitutionLogBuffer);
        fflush(gpSubstitutionLogOutputFile);
    }
#endif
}

void gfSubstitutionAssertFail()
{
    if (DLikely(gpSubstitutionLogMutex && gpSubstitutionLogOutputFile)) {
        AUTO_LOCK(gpSubstitutionLogMutex);
        fprintf(gpSubstitutionLogOutputFile, "assert fail\n");
    }
    gSubstitutionTracker();
}

TU32 isLogFileOpened()
{
    if (gpSubstitutionLogOutputFile) {
        return 1;
    }

    return 0;
}

void gfOpenLogFile(TChar *name)
{
    if (DLikely(name)) {
        //ASSERT(!gpSubstitutionLogOutputFile);
        if (DUnlikely(gpSubstitutionLogOutputFile)) {
            fclose(gpSubstitutionLogOutputFile);
            gpSubstitutionLogOutputFile = NULL;
        }

        gpSubstitutionLogOutputFile = fopen(name, "wt");
        if (DLikely(gpSubstitutionLogOutputFile)) {
            LOG_PRINTF("open log file(%s) success\n", name);
        } else {
            LOG_ERROR("open log file(%s) fail\n", name);
        }
    }

    if (!gpSubstitutionLogMutex) {
        gpSubstitutionLogMutex = gfCreateMutex();
    }
}

void gfCloseLogFile()
{
    if (DLikely(gpSubstitutionLogOutputFile)) {
        fclose(gpSubstitutionLogOutputFile);
        gpSubstitutionLogOutputFile = NULL;
    }
    if (gpSubstitutionLogMutex) {
        gpSubstitutionLogMutex->Delete();
        gpSubstitutionLogMutex = NULL;
    }
}

void gfSetLogLevel(ELogLevel level)
{
    gSubstitutionLogLevel = level;
}

void gfOpenSnapshotFile(TChar *name)
{
    if (DLikely(name)) {
        //ASSERT(!gpSnapshotSubstitutionOutputFile);
        if (DUnlikely(gpSnapshotSubstitutionOutputFile)) {
            fclose(gpSnapshotSubstitutionOutputFile);
            gpSnapshotSubstitutionOutputFile = NULL;
        }

        gpSnapshotSubstitutionOutputFile = fopen(name, "wt");
        if (DLikely(gpSnapshotSubstitutionOutputFile)) {
            LOG_PRINTF("open snapshot file(%s) success\n", name);
        } else {
            LOG_ERROR("open snapshot file(%s) fail\n", name);
        }
    }
}

void gfCloseSnapshotFile()
{
    if (DLikely(gpSnapshotSubstitutionOutputFile)) {
        fclose(gpSnapshotSubstitutionOutputFile);
        gpSnapshotSubstitutionOutputFile = NULL;
    }
}

#endif

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


