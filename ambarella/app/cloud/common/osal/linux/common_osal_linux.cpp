/*******************************************************************************
 * common_osal_linux.cpp
 *
 * History:
 *  2013/09/12 - [Zhi He] create file
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

#include <unistd.h>
#include <poll.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "common_osal_linux.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CIClockSourceGenericLinux
//
//-----------------------------------------------------------------------
IClockSource *gfCreateGenericClockSource()
{
    return CIClockSourceGenericLinux::Create();
}

IClockSource *CIClockSourceGenericLinux::Create()
{
    CIClockSourceGenericLinux *result = new CIClockSourceGenericLinux();
    if (result && (EECode_OK != result->Construct())) {
        delete result;
        result = NULL;
    }
    DASSERT(result);
    return result;
}

EECode CIClockSourceGenericLinux::Construct()
{
    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

void CIClockSourceGenericLinux::Delete()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    inherited::Delete();

    return;
}

CObject *CIClockSourceGenericLinux::GetObject0() const
{
    return (CObject *) this;
}

CIClockSourceGenericLinux::CIClockSourceGenericLinux()
    : inherited("CIClockSourceGenericLinux")
    , mbUseNativeUSecond(1)
    , msState(EClockSourceState_stopped)
    , mbGetBaseTime(0)
    , mTimeUintDen(1000000)
    , mTimeUintNum(1)
    , mSourceCurrentTimeTick(0)
    , mSourceBaseTimeTick(0)
    , mCurrentTimeTick(0)
    , mpMutex(NULL)
{

}

CIClockSourceGenericLinux::~CIClockSourceGenericLinux()
{

}


TTime CIClockSourceGenericLinux::GetClockTime(TUint relative) const
{
    AUTO_LOCK(mpMutex);
    if (DLikely(relative)) {
        if (DLikely(mbUseNativeUSecond)) {
            return mCurrentTimeTick;
        } else {
            return (mCurrentTimeTick) * 10000000 / mTimeUintDen * mTimeUintNum;
        }
    } else {
        if (DLikely(mbUseNativeUSecond)) {
            return mSourceCurrentTimeTick;
        } else {
            return mSourceCurrentTimeTick / mTimeUintDen * 1000000 * mTimeUintNum;
        }
    }
}

TTime CIClockSourceGenericLinux::GetClockBase() const
{
    AUTO_LOCK(mpMutex);
    if (DLikely(mbUseNativeUSecond)) {
        return mSourceBaseTimeTick;
    } else {
        return mSourceBaseTimeTick * 10000000 / mTimeUintDen * mTimeUintNum;
    }
}

void CIClockSourceGenericLinux::SetClockFrequency(TUint num, TUint den)
{
    AUTO_LOCK(mpMutex);

    DASSERT(num);
    DASSERT(den);

    if (num && den) {
        mTimeUintDen = num;
        mTimeUintNum = den;

        if ((1000000 != mTimeUintDen) || (1 != mTimeUintNum)) {
            LOGM_WARN("not u second!\n");
            mbUseNativeUSecond = 0;
        }
    }
}

void CIClockSourceGenericLinux::GetClockFrequency(TUint &num, TUint &den) const
{
    AUTO_LOCK(mpMutex);
    num = mTimeUintDen;
    den = mTimeUintNum;
}

void CIClockSourceGenericLinux::SetClockState(EClockSourceState state)
{
    AUTO_LOCK(mpMutex);
    msState = (TU8)state;
}

EClockSourceState CIClockSourceGenericLinux::GetClockState() const
{
    AUTO_LOCK(mpMutex);
    return (EClockSourceState)msState;
}

void CIClockSourceGenericLinux::UpdateTime()
{
    AUTO_LOCK(mpMutex);
    if (msState != EClockSourceState_running) {
        return;
    }

    struct timeval time;

    gettimeofday(&time, NULL);
    TTime tmp = (TTime)time.tv_sec * 1000000 + (TTime)time.tv_usec;

    if (DUnlikely(!mbGetBaseTime)) {
        if (DLikely(mbUseNativeUSecond)) {
            mSourceBaseTimeTick = mSourceCurrentTimeTick = tmp;
        } else {
            mSourceBaseTimeTick = mSourceCurrentTimeTick = tmp / mTimeUintDen * 1000000 * mTimeUintNum;
        }
        mbGetBaseTime = 1;
        mCurrentTimeTick = 0;
    } else {
        if (DLikely(mbUseNativeUSecond)) {
            mSourceCurrentTimeTick = tmp;
        } else {
            mSourceCurrentTimeTick = tmp / mTimeUintDen * 1000000 * mTimeUintNum;
        }
        mCurrentTimeTick = mSourceCurrentTimeTick - mSourceBaseTimeTick;
    }

}

void CIClockSourceGenericLinux::PrintStatus()
{
    AUTO_LOCK(mpMutex);
    LOGM_ALWAYS("mbGetBaseTime %d, mbUseNativeUSecond %d, msState %d\n", mbGetBaseTime, mbUseNativeUSecond, msState);
    LOGM_ALWAYS("mSourceBaseTimeTick %lld, mSourceCurrentTimeTick %lld, mCurrentTimeTick %lld\n", mSourceBaseTimeTick, mSourceCurrentTimeTick, mCurrentTimeTick);
}


EECode gfGetCurrentDateTime(SDateTime *datetime)
{
    time_t timer;
    struct tm *tblock = NULL;

    if (DUnlikely(!datetime)) {
        LOG_ERROR("NULL params\n");
        return EECode_BadParam;
    }

    timer = time(NULL);
    tblock = localtime(&timer);

    DASSERT(tblock);
    datetime->year = tblock->tm_year + 1900;
    datetime->month = tblock->tm_mon + 1;
    datetime->day = tblock->tm_mday;
    datetime->hour = tblock->tm_hour;
    datetime->minute = tblock->tm_min;
    datetime->seconds = tblock->tm_sec;

    datetime->weekday = tblock->tm_wday;

    return EECode_OK;
}

TTime gfGetNTPTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    TTime systime = (TTime)tv.tv_sec * 1000000 + tv.tv_usec;

#define NTP_OFFSET 2208988800ULL
#define NTP_OFFSET_US (NTP_OFFSET * 1000000ULL)
    TTime ntp_time = (systime / 1000 * 1000) + NTP_OFFSET_US;

    return ntp_time;
}

TTime gfGetRalativeTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((TTime)tv.tv_sec * 1000000 + tv.tv_usec);
}

void gfGetDateString(TChar *pbuffer, TUint size)
{
    time_t tt = time(NULL);
    strftime(pbuffer, size, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
}

EECode gfOSWritePipe(TSocketHandler fd, const TChar byte)
{
    size_t ret = 0;

    ret = write(fd, &byte, 1);
    if (DLikely(1 == ret)) {
        return EECode_OK;
    }

    return EECode_OSError;
}

EECode gfOSReadPipe(TSocketHandler fd, TChar &byte)
{
    size_t ret = 0;

    ret = read(fd, &byte, 1);
    if (DLikely(1 == ret)) {
        return EECode_OK;
    }

    return EECode_OSError;
}

EECode gfOSCreatePipe(TSocketHandler fd[2])
{
    TInt ret = pipe(fd);
    if (DLikely(0 == ret)) {
        return EECode_OK;
    }

    return EECode_OSError;
}

void gfOSClosePipeFd(TSocketHandler fd)
{
    close(fd);
}

EECode gfOSGetHostName(TChar *name, TU32 name_buffer_length)
{
    gethostname(name, name_buffer_length);
    return EECode_OK;
}

EECode gfOSPollFdReadable(THandler fd)
{
    struct pollfd fds;
    fds.fd = fd;
    fds.events = POLLIN;
    fds.revents = 0;

    if (poll(&fds, 1, 0) <= 0) {
        return EECode_TryAgain;
    }

    return EECode_OK;
}

void gfOSmsleep(TU32 ms)
{
    usleep(1000 * ms);
}

void gfOSusleep(TU32 us)
{
    usleep(us);
}

void gfOSsleep(TU32 s)
{
    sleep(s);
}

void gfGetHostName(TChar *name, TU32 name_buffer_size)
{
    gethostname(name, name_buffer_size);
}

void gfGetSTRFTimeString(TChar *time_string, TU32 time_string_buffer_length)
{
    time_t cur_time = time(NULL);
    strftime(time_string, time_string_buffer_length, "%Z%Y%m%d_%H%M%S", gmtime(&cur_time));
}

EECode gfGetSystemSettings(SSystemSettings *p_settings)
{
    LOG_FATAL("gfGetSystemSettings need implement\n");
    return EECode_NotSupported;
}

#if 0
TInt gfOsIsExist(const TChar *name)
{
    struct stat st;
    if (!name) {
        LOG_FATAL("NULL name\n");
        return 0;
    }

    if (stat(name, &st)) {
        return(0);
    }

    return(st.st_mode);
}

TInt gfOsIsDir(const TChar *file)
{
    TInt exist = gfOsIsExist(file);
    if (!exist) {
        return 0;
    }

    if ((exist & _S_IFMT) == _S_IFDIR) {
        return 1;
    }

    return 0;
}

TInt gfOsIsFile(const TChar *path)
{
    TInt exist = gfOsIsExist(path);
    if (!exist) {
        return 0;
    }

    if ((exist & _S_IFMT) == _S_IFDIR) {
        return 0;
    }

    return 1;
}
#endif

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

