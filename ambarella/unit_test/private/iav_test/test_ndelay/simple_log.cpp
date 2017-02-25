/**
 * simple_log.cpp
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

#include <stdlib.h>
#include <stdio.h>
#include "simple_log.h"

static volatile unsigned int gspLogLevel = ELogLevel_Info;
static volatile unsigned int gspLogOutput = ELogOutput_File | ELogOutput_Console;

volatile unsigned int* const pgConfigLogLevel = (volatile unsigned int* const) &gspLogLevel;
volatile unsigned int* const pgConfigLogOutput = (volatile unsigned int* const) &gspLogOutput;

const char gcConfigLogLevelTag[ELogLevel_TotalCount][DMAX_LOGTAG_NAME_LEN] =
{
    {"\t"},
    {"[Fatal ]"},
    {"[Error ]"},
    {"[Warn  ]"},
    {"[Notice]"},
    {"[Info  ]"},
    {"[Debug ]"},
    {"[Verbose]"},
};

FILE* gpLogOutputFile = NULL;

unsigned int isLogFileOpened()
{
    if (gpLogOutputFile) {
        return 1;
    }

    return 0;
}

void gfOpenLogFile(char* name)
{
    if (name) {
        if (gpLogOutputFile) {
            fclose(gpLogOutputFile);
            gpLogOutputFile = NULL;
        }

        gpLogOutputFile = fopen(name, "wt");
        if (gpLogOutputFile) {
            LOG_PRINTF("open log file(%s) success\n", name);
        } else {
            LOG_ERROR("open log file(%s) fail\n", name);
        }
    }
}

void gfCloseLogFile()
{
    if (gpLogOutputFile) {
        fclose(gpLogOutputFile);
        gpLogOutputFile = NULL;
    }
}

void gfSetLogLevel(ELogLevel level)
{
    gspLogLevel = level;
}

void gfPrintMemory(unsigned char* p, unsigned int size)
{
    if (p) {
        while (size > 7) {
            __CLEAN_PRINT("%02x %02x %02x %02x %02x %02x %02x %02x\n", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
            p += 8;
            size -= 8;
        }

        if (size) {
            while (size) {
                __CLEAN_PRINT("%02x ", p[0]);
                p ++;
                size --;
            }
            __CLEAN_PRINT("\n");
        }
    }
}

