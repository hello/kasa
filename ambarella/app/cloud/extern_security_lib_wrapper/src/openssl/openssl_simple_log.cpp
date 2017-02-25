/*******************************************************************************
 * openssl_simple_log.cpp
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


