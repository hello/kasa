/**
 * common_substitution_log.cpp
 *
 * History:
 *  2014/09/14 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

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


