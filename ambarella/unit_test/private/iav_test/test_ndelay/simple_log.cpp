/**
 * simple_log.cpp
 *
 * History:
 *	2015/08/31 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
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

