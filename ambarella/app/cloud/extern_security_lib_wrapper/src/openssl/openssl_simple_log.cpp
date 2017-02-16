/*
 * openssl_simple_log.cpp
 *
 * History:
 *  2015/05/18 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include <stdio.h>
#include <stdlib.h>

#include "openssl_simple_log.h"

volatile unsigned long gOpenSSLConfigLogLevel = EOpenSSLSimpleLogLevel_Notice;
volatile unsigned long gOpenSSLConfigLogOutput = DOPENSSLLOG_OUTPUT_FILE;

const char gOpenSSLConfigLogLevelTag[EOpenSSLSimpleLogLevel_TotalCount][DOPENSSL_MAX_LOGTAG_NAME_LEN] = {
    {"\t"},
    {"[Fatal ]"},
    {"[Error ]"},
    {"[Warn  ]"},
    {"[Notice]"},
    {"[Info  ]"},
    {"[Debug ]"},
    {"[Verbose]"},
};

FILE *gpOpenSSLSimpleLogOutputFile = NULL;

unsigned int isOpenSSLSimpleLogFileOpened()
{
    if (gpOpenSSLSimpleLogOutputFile) {
        return 1;
    }

    return 0;
}

void gfOpenSSOpenSimpleLogFile(char *name)
{
    if (name) {
        if (gpOpenSSLSimpleLogOutputFile) {
            fclose(gpOpenSSLSimpleLogOutputFile);
            gpOpenSSLSimpleLogOutputFile = NULL;
        }

        //fopen_s(&gpOpenSSLSimpleLogOutputFile,name, "wt");
        gpOpenSSLSimpleLogOutputFile = fopen(name, "wt");
        if (gpOpenSSLSimpleLogOutputFile) {
            //DOPENSSL_LOG_PRINTF("open log file(%s) success\n", name);
        } else {
            //DOPENSSL_LOG_ERROR("open log file(%s) fail\n", name);
        }
    }
}

void gfOpenSSCloseSimpleLogFile()
{
    if (gpOpenSSLSimpleLogOutputFile) {
        fclose(gpOpenSSLSimpleLogOutputFile);
        gpOpenSSLSimpleLogOutputFile = NULL;
    }
}

void gfOpenSSSetSimpleLogLevel(EOpenSSLSimpleLogLevel level)
{
    gOpenSSLConfigLogLevel = level;
}


