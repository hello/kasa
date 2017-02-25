/**
 * app/ipcam/fastboot_smart3a/smart3a_adc.c
 *
 * Author: Caizhang Lin <czlin@ambarella.com>
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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
 */


#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include "adc_io.h"

#define NO_ARG		0
#define HAS_ARG		1

struct hint_s {
    const char *arg;
    const char *str;
};

static int select_read_3a = 1;
static int dump_from_mem = 0;
static int verbose = 0;

static const char *short_options = "rwvm";

static struct option long_options[] = {
    {"read",    NO_ARG,        0,    'r'},
    {"write",    NO_ARG,        0,    'w'},
    {"verbose",    NO_ARG,        0,    'v'},
    {"memory",    NO_ARG,        0,    'm'},
    {0, 0, 0, 0}
};

static const struct hint_s hint[] = {
    {"", "Read out the 3A config from ADC (At the begin of Linux)"},
    {"", "Write 3A config into ADC which from 3A process (In the end of Linux)"},
    {"", "print debug info"},
    {"", "copy parmas info from memory, only when fastboot has copied data to specific memory"},
};

static void usage(void)
{
    unsigned int i;
    char *itself = "smart_3a";

    printf("This program used to read/write 3A config from/into ADC partition\n");
    printf("\n");
    for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
        if (isalpha(long_options[i].val))
            printf("-%c ", long_options[i].val);
        else
            printf("   ");
        printf("--%s", long_options[i].name);
        if (hint[i].arg[0] != 0)
            printf(" [%s]", hint[i].arg);
        printf("\t%s\n", hint[i].str);
    }

    printf("Example:\n\tRead # %s -r\n", itself);
    printf("\tWrite # %s -w -v\n", itself);
}

static int init_param(int argc, char **argv)
{
    int ch;
    int option_index = 0;

    while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (ch) {
            case 'r':
                select_read_3a = 1;
                break;
            case 'w':
                select_read_3a = 0;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'm':
                dump_from_mem = 1;
                break;
            default:
                printf("unknown option found: %c\n", ch);
                return -1;
        }
    }

    return 0;
}
/* ==========================================================================*/
int main(int argc, char **argv)
{
    int rval = 0;

    if (argc < 2) {
        usage();
        return -1;
    }

    if (init_param(argc, argv) < 0) {
        usage();
        return -1;
    }

    do {
        if (select_read_3a) {
            if (dump_from_mem) {
                rval = adc_io_mem_dump();
            } else {
                rval = adc_io_read(verbose,  SMART3A_PARAMETERS | AMBOOT_PARAMETERS);
            }
        } else {
            rval = adc_io_write(verbose, SMART3A_PARAMETERS, NULL);
        }

        if (rval < 0) {
            printf("smart3a_adc_read/write failed\n");
            break;
        }
    } while (0);

    return rval;
}

