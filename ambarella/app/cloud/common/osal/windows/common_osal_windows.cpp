/*******************************************************************************
 * common_osal_windows.cpp
 *
 * History:
 *  2014/09/25 - [Zhi He] create file
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
#include "common_utils.h"

#include <time.h>
#include <direct.h>
#include <io.h>

#include "common_log.h"

#include "common_base.h"

#include "common_osal_windows.h"

#include <DSound.h>

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static TInt _gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;

    GetLocalTime(&wtm);

#if 0
    static TInt lastsecond = 0;
    if (lastsecond != wtm.wSecond) {
        LOG_PRINTF("[print time]: %d-%d-%d %d-%d-%d, %d\n", wtm.wYear, wtm.wMonth, wtm.wDay, wtm.wHour, wtm.wMinute, wtm.wSecond, wtm.wMilliseconds);
    } else {
        LOG_PRINTF("[print time]: wtm.wMilliseconds %d\n", wtm.wMilliseconds);
    }
    lastsecond = wtm.wSecond;
#endif

    tm.tm_year     = wtm.wYear - 1900;
    tm.tm_mon     = wtm.wMonth - 1;
    tm.tm_mday     = wtm.wDay;
    tm.tm_hour     = wtm.wHour;
    tm.tm_min     = wtm.wMinute;
    tm.tm_sec     = wtm.wSecond;
    tm.tm_isdst    = -1;
    clock = mktime(&tm);
    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;

    return (0);
}

//-----------------------------------------------------------------------
//
// CIClockSourceGenericWindows
//
//-----------------------------------------------------------------------
IClockSource *gfCreateGenericClockSource()
{
    return CIClockSourceGenericWindows::Create();
}

IClockSource *CIClockSourceGenericWindows::Create()
{
    CIClockSourceGenericWindows *result = new CIClockSourceGenericWindows();
    if (result && (EECode_OK != result->Construct())) {
        delete result;
        result = NULL;
    }
    DASSERT(result);
    return result;
}

EECode CIClockSourceGenericWindows::Construct()
{
    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("gfCreateMutex() fail\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

void CIClockSourceGenericWindows::Delete()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    inherited::Delete();

    return;
}

CObject *CIClockSourceGenericWindows::GetObject0() const
{
    return (CObject *) this;
}

CIClockSourceGenericWindows::CIClockSourceGenericWindows()
    : inherited("CIClockSourceGenericWindows")
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

CIClockSourceGenericWindows::~CIClockSourceGenericWindows()
{

}


TTime CIClockSourceGenericWindows::GetClockTime(TUint relative) const
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

TTime CIClockSourceGenericWindows::GetClockBase() const
{
    AUTO_LOCK(mpMutex);
    if (DLikely(mbUseNativeUSecond)) {
        return mSourceBaseTimeTick;
    } else {
        return mSourceBaseTimeTick * 10000000 / mTimeUintDen * mTimeUintNum;
    }
}

void CIClockSourceGenericWindows::SetClockFrequency(TUint num, TUint den)
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

void CIClockSourceGenericWindows::GetClockFrequency(TUint &num, TUint &den) const
{
    AUTO_LOCK(mpMutex);
    num = mTimeUintDen;
    den = mTimeUintNum;
}

void CIClockSourceGenericWindows::SetClockState(EClockSourceState state)
{
    AUTO_LOCK(mpMutex);
    msState = (TU8)state;
}

EClockSourceState CIClockSourceGenericWindows::GetClockState() const
{
    AUTO_LOCK(mpMutex);
    return (EClockSourceState)msState;
}

void CIClockSourceGenericWindows::UpdateTime()
{
    AUTO_LOCK(mpMutex);
    if (msState != EClockSourceState_running) {
        return;
    }

    struct timeval time;

    _gettimeofday(&time, NULL);
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

void CIClockSourceGenericWindows::PrintStatus()
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
    _gettimeofday(&tv, NULL);
    TTime systime = (TTime)tv.tv_sec * 1000000 + tv.tv_usec;

#define NTP_OFFSET 2208988800ULL
#define NTP_OFFSET_US (NTP_OFFSET * 1000000ULL)
    TTime ntp_time = (systime / 1000 * 1000) + NTP_OFFSET_US;

    return ntp_time;
}

TTime gfGetRalativeTime()
{
    struct timeval tv;
    _gettimeofday(&tv, NULL);

    return ((TTime)tv.tv_sec * 1000000 + tv.tv_usec);
}

void gfGetDateString(TChar *pbuffer, TUint size)
{
    time_t tt = time(NULL);
    strftime(pbuffer, size, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
}

static TInt _pipe_by_socket(TSocketHandler fildes[2])
{
    TSocketHandler tcp1, tcp2;
    sockaddr_in name;
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    TInt namelen = sizeof(name);
    tcp1 = tcp2 = -1;
    TInt tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp == -1) {
        goto clean;
    }
    if (bind(tcp, (sockaddr *)&name, namelen) == -1) {
        goto clean;
    }
    if (listen(tcp, 5) == -1) {
        goto clean;
    }
    if (getsockname(tcp, (sockaddr *)&name, &namelen) == -1) {
        goto clean;
    }
    tcp1 = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp1 == -1) {
        goto clean;
    }
    if (-1 == connect(tcp1, (sockaddr *)&name, namelen)) {
        goto clean;
    }
    tcp2 = accept(tcp, (sockaddr *)&name, &namelen);
    if (tcp2 == -1) {
        goto clean;
    }
    if (closesocket(tcp) == -1) {
        goto clean;
    }
    fildes[0] = tcp1;
    fildes[1] = tcp2;
    return 0;
clean:
    if (tcp != -1) {
        closesocket(tcp);
    }
    if (tcp2 != -1) {
        closesocket(tcp2);
    }
    if (tcp1 != -1) {
        closesocket(tcp1);
    }
    return -1;
}

EECode gfOSWritePipe(TSocketHandler fd, const TChar byte)
{
    size_t ret = 0;

    ret = send(fd, &byte, 1, 0);
    if (DLikely(1 == ret)) {
        return EECode_OK;
    }

    return EECode_OSError;
}

EECode gfOSReadPipe(TSocketHandler fd, TChar &byte)
{
    size_t ret = 0;

    ret = recv(fd, &byte, 1, 0);
    if (DLikely(1 == ret)) {
        return EECode_OK;
    }

    return EECode_OSError;
}

EECode gfOSCreatePipe(TSocketHandler fd[2])
{
    TInt ret = _pipe_by_socket(fd);
    if (DLikely(0 == ret)) {
        return EECode_OK;
    }

    return EECode_OSError;
}

void gfOSClosePipeFd(TSocketHandler fd)
{
    closesocket(fd);
}

EECode gfOSGetHostName(TChar *name, TU32 name_buffer_length)
{
    gethostname(name, name_buffer_length);
    return EECode_OK;
}

EECode gfOSPollFdReadable(THandler fd)
{
    // to do
#if 0
    struct pollfd fds;
    fds.fd = fd;
    fds.events = POLLIN;
    fds.revents = 0;

    if (poll(&fds, 1, 0) <= 0) {
        return EECode_TryAgain;
    }

    return EECode_OK;
#endif

    return EECode_TryAgain;
}

void gfOSmsleep(TU32 ms)
{
    Sleep(ms);
}

void gfOSusleep(TU32 us)
{
    Sleep(us / 1000);
}

void gfOSsleep(TU32 s)
{
    Sleep(1000 * s);
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
    if (DUnlikely(!p_settings)) {
        LOG_FATAL("gfGetSystemSettings NULL input params\n");
        return EECode_BadParam;
    }

    //hard code here
    p_settings->screen_number = 1;
    p_settings->screens[0].screen_width = GetSystemMetrics(SM_CXSCREEN);
    p_settings->screens[0].screen_height = GetSystemMetrics(SM_CYSCREEN);
    p_settings->screens[0].pixel_format = EPixelFMT_rgba32;
    p_settings->screens[0].refresh_rate = 30;

    return EECode_OK;
}

#ifdef BUILD_MODULE_WDS
static BOOL CALLBACK __DSEnumProc(LPGUID lpGUID, LPCTSTR lpszDesc, LPCTSTR lpszDrvName, LPVOID lpContext)
{
    SSystemSoundInputDevices *p_devices = (SSystemSoundInputDevices *)lpContext;
    if (DLikely(p_devices)) {
        if (DLikely(p_devices->sound_input_devices_number < DMAX_SOUND_INPUT_DEVICE_NUMBER)) {
            if (lpGUID) {
                GUID *p_guid = (GUID *) DDBG_MALLOC(sizeof(GUID), "GUID");
                p_devices->devices[p_devices->sound_input_devices_number].p_devname = lpszDrvName;
                p_devices->devices[p_devices->sound_input_devices_number].p_desc = lpszDesc;
                *p_guid = *lpGUID;
                p_devices->devices[p_devices->sound_input_devices_number].p_context = (void *) p_guid;
            } else {
                p_devices->devices[p_devices->sound_input_devices_number].p_devname = strdup(lpszDrvName);
                p_devices->devices[p_devices->sound_input_devices_number].p_desc = strdup(lpszDesc);
                p_devices->devices[p_devices->sound_input_devices_number].p_context = NULL;
            }
            p_devices->sound_input_devices_number ++;
        }
    }

    return(TRUE);
}
#endif

EECode gfGetSystemSoundInputDevices(SSystemSoundInputDevices *p_devices)
{
    p_devices->sound_input_devices_number = 0;
#ifdef BUILD_MODULE_WDS
    HRESULT hr = DirectSoundCaptureEnumerate((LPDSENUMCALLBACK)__DSEnumProc, (void *)p_devices);
    if (FAILED(hr)) {
        LOG_ERROR("DirectSoundCaptureEnumerate() fail, ret 0x%08x\n", hr);
        return EECode_OSError;
    }

    LOG_NOTICE("system sound device number %d\n", p_devices->sound_input_devices_number);
#endif
    return EECode_OK;
}

#if 0
TInt gfOsIsExist(const TChar *name)
{
    _stat st;
    wchar_t *tmp_16 = alloc_utf16_from_8(name, 1);
    TInt len, res;
    TUint old_error_mode;

    len = wcslen(tmp_16);

    if (len > 3 && (tmp_16[len - 1] == L'\\' || tmp_16[len - 1] == L'/')) {
        tmp_16[len - 1] = '\0';
    }

    if ((tmp_16[1] ==  L':') && (tmp_16[2] ==  L'\0')) {
        tmp_16[2] = L'\\';
        tmp_16[3] = L'\0';
    }

    /* change error mode so user does not get a "no disk in drive" popup
     * when looking for a file on an empty CD/DVD drive */
    old_error_mode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    res = _wstat(tmp_16, &st);

    SetErrorMode(old_error_mode);

    free(tmp_16);

    if (res == -1) { return(0); }

    return(st.st_mode);
}

TInt gfOsIsDir(const TChar *file)
{
    TInt exist = gfOsIsExist(file);

    if (!exist) {
        return 0;
    }

    if ((exist & _S_IFDIR) == _S_IFDIR) {
        return 1;
    }

    return 0;
}

TInt gfOsIsFile(const TChar *path)
{
    TInt exist = gfOsIsExist(file);

    if (!exist) {
        return 0;
    }

    if ((exist & _S_IFDIR) == _S_IFDIR) {
        return 0;
    }

    return 1;
}
#endif


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

