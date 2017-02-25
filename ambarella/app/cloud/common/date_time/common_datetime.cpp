/*******************************************************************************
 * common_datetime.cpp
 *
 * History:
 *  2014/01/11 - [Zhi He] create file
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

#include "common_log.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//static const SDateTime g_initialDate = {.year = 2014; .month = 1, .day = 11, .weekday = 6};
//static SDateTime g_currentInitialDate;
//static TUint g_bLoadCurrentInitialDate = 0;

static const TU8 g_dayOfMonth[12] = {
    31,
    28,
    31,
    30,
    31,
    30,
    31,
    31,
    30,
    31,
    30,
    31
};

static const TU8 g_dayOfMonthLeapYear[12] = {
    31,
    29,
    31,
    30,
    31,
    30,
    31,
    31,
    30,
    31,
    30,
    31
};

static TUint isLeapYear(TU32 year)
{
    if (DLikely(year & 0x3)) {
        return 0;
    }

    TU32 mode_400 = year % 400;
    if (DUnlikely((100 == mode_400) || (200 == mode_400) || (300 == mode_400))) {
        return 0;
    }

    return 1;
}

static TU8 getDayOfMonth(TU8 month, TU32 year)
{
    if (DUnlikely(2 == month)) {
        if (DUnlikely(1 == isLeapYear(year))) {
            return 29;
        }
        return 28;
    } else if (DUnlikely(12 < month)) {
        LOG_FATAL("month %d > 12\n", month);
        return 30;
    }

    return g_dayOfMonth[month - 1];
}

static TU8 getDayOfMonthInLeapYear(TU8 month)
{
    if (DUnlikely(12 < month)) {
        LOG_FATAL("month %d > 12\n", month);
        return 30;
    }

    return g_dayOfMonthLeapYear[month - 1];
}

static TU8 getDayOfMonthInNormalYear(TU8 month)
{
    if (DUnlikely(12 < month)) {
        LOG_FATAL("month %d > 12\n", month);
        return 30;
    }

    return g_dayOfMonth[month - 1];
}

static TU32 getDayOfYear(TU32 year)
{
    if (DUnlikely(1 == isLeapYear(year))) {
        return 366;
    }
    return 365;
}

#if 0
static TU32 getDayOfLeapYear()
{
    return 366;
}

static TU32 getDayOfNormalYear()
{
    return 365;
}
#endif

// 1: later, 0 : earlier, 2 : equal
TU32 gfDateIsLater(SDateTime &current_date, SDateTime &start_date)
{
    if (DLikely(current_date.year == start_date.year)) {
        if (DLikely(current_date.month == start_date.month)) {
            if (DLikely(current_date.day > start_date.day)) {
                return 1;
            } else if (current_date.day == start_date.day) {
                if (DLikely(current_date.hour > start_date.hour)) {
                    return 1;
                } else if (current_date.hour == start_date.hour) {
                    if (DLikely(current_date.minute > start_date.minute)) {
                        return 1;
                    } else if (current_date.minute == start_date.minute) {
                        if (DLikely(current_date.seconds > start_date.seconds)) {
                            return 1;
                        } else if (current_date.seconds == start_date.seconds) {
                            return 2;
                        }
                    }
                }
            }
        } else if (DLikely(current_date.month > start_date.month)) {
            return 1;
        }
    } else if (DLikely(current_date.year > start_date.year)) {
        return 1;
    }

    return 0;
}

TS32 dayOfMonthes(TU8 cur_month, TU8 start_month, TU32 year)
{
    TS32 days = 0;
    TU8 i = 0;

    if (DLikely(isLeapYear(year))) {
        if (DLikely(cur_month > start_month)) {
            for (i = start_month; i < cur_month; i ++) {
                days += getDayOfMonthInLeapYear(i);
            }
        } else if (cur_month == start_month) {
            return 0;
        } else {
            LOG_FATAL("negative\n");
            return 0;
        }
    } else {
        if (DLikely(cur_month > start_month)) {
            for (i = start_month; i < cur_month; i ++) {
                days += getDayOfMonthInNormalYear(i);
            }
        } else if (cur_month == start_month) {
            return 0;
        } else {
            LOG_FATAL("negative\n");
            return 0;
        }
    }

    return days;
}

TS32 dayOfMonthesInLeapYear(TU8 cur_month, TU8 start_month)
{
    if (DLikely(cur_month > start_month)) {
        TS32 days = 0;
        TU8 i = 0;
        for (i = start_month; i < cur_month; i ++) {
            days += getDayOfMonthInLeapYear(i);
        }
        return days;
    } else if (cur_month == start_month) {

    } else {
        LOG_FATAL("negative\n");
    }

    return 0;
}

TS32 dayOfMonthesInNormalYear(TU8 cur_month, TU8 start_month)
{
    if (DLikely(cur_month > start_month)) {
        TS32 days = 0;
        TU8 i = 0;
        for (i = start_month; i < cur_month; i ++) {
            days += getDayOfMonthInNormalYear(i);
        }
        return days;
    } else if (cur_month == start_month) {

    } else {
        LOG_FATAL("negative\n");
    }

    return 0;
}

TS32 dayOfYears(TU32 cur_year, TU32 start_year)
{
    TS32 days = 0;
    TU32 i = 0;

    if (cur_year > start_year) {
        for (i = start_year; i < cur_year; i ++) {
            days += getDayOfYear(i);
        }
    } else if (cur_year == start_year) {
        return 0;
    } else {
        LOG_FATAL("negative\n");
        return 0;
    }

    return days;
}

TU64 secondsToEndOfDay(SDateTime &current_date)
{
    TU64 v = (TU64)(24 - current_date.hour) * (TU64)3600;

    v -= (TU64)current_date.minute * 60;
    v -= (TU64)current_date.seconds;

    return v;
}

TU64 secondsToEndOfMonth(SDateTime &current_date)
{
    TU64 v = getDayOfMonth(current_date.month, current_date.year);

    v -= current_date.day;
    v *= 3600 * 24;
    v += secondsToEndOfDay(current_date);

    return v;
}

TU64 secondsToEndOfYear(SDateTime &current_date)
{
    TU64 v = dayOfMonthes(12, current_date.month, current_date.year);

    v -= current_date.day;
    v *= 3600 * 24;
    v += secondsToEndOfDay(current_date);

    return v;
}

void gfDateCalculateGap(SDateTime &current_date, SDateTime &start_date, SDateTimeGap &gap)
{
    TU64 vd = 0, vs = 0;

    if (DLikely(current_date.year == start_date.year)) {
        vd = dayOfMonthes(current_date.month, start_date.month, current_date.year);
        vd += current_date.day;
        vd -= start_date.day;
    } else {
        vd = dayOfYears(current_date.year, start_date.year);
        vd += dayOfMonthes(current_date.month, 0, current_date.year);
        vd += current_date.day;
        vd -= dayOfMonthes(start_date.month, 0, start_date.year);
        vd -= start_date.day;
    }

    vs = ((TU64)current_date.hour * 3600) + ((TU64)current_date.minute * 60) + current_date.seconds;
    vs += (3600 * 24) * vd;
    vs -= ((TU64)start_date.hour * 3600) + ((TU64)start_date.minute * 60) + start_date.seconds;

    gap.days = vd;
    gap.overall_seconds = vs;

    gap.total_hours = vs / 3600;
    vs -= gap.total_hours * 3600;
    gap.total_minutes = vs / 60;
    gap.total_seconds = vs - (gap.total_minutes * 60);

}

void gfDateCalculateNextTime(SDateTime &next_date, SDateTime &start_date, TU64 seconds)
{
    TU64 v = secondsToEndOfDay(start_date);
    if (DLikely(seconds < v)) {
        next_date = start_date;
        if (seconds < ((TU64)60 - start_date.seconds)) {
            start_date.seconds += seconds;
            return;
        } else {
            next_date.minute ++;
            seconds -= ((TU64)60 - next_date.seconds);
            next_date.seconds = 0;
        }

        TU32 v0 = seconds / 60;
        TU32 v1 = next_date.minute + v0;
        TU32 v2 = v1 / 60;
        next_date.hour += v2;
        next_date.minute = v1 - (v2 * 60);
        next_date.seconds = seconds - (v0 * 60);
    } else {
        LOG_FATAL("gfDateCalculateNextTime: TO DO\n");
        return;
    }

}

void gfDateNextDay(SDateTime &date, TU32 days)
{
    LOG_FATAL("gfDateNextDay TO DO\n");
    return;
}

void gfDatePreviousDay(SDateTime &date, TU32 days)
{
    LOG_FATAL("gfDatePreviousDay TO DO\n");
    return;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

