/*******************************************************************************
 * ctest.cpp
 *
 * History:
 *    2013/12/03 - [Zhi He] create file
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
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>

#include <pthread.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "cloud_lib_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_BEGIN
DCODE_DELIMITER;

//unit test self log
#if 1
//log related
enum {
    e_log_level_always = 0,

    e_log_level_error = 1,
    e_log_level_warning,
    e_log_level_notice,
    e_log_level_debug,

    e_log_level_all,

    e_log_level_total_count,
};
static TInt g_log_level = e_log_level_notice;
static TChar g_log_tags[e_log_level_total_count][32] = {
    {"      [Info]: "},
    {"  [Error]: "},
    {"    [Warnning]: "},
    {"      [Notice]: "},
    {"        [Debug]: "},
    {"          [Verbose]: "},
};

#define test_log_level_trace(level, format, args...)  do { \
        if (level <= g_log_level) { \
            fprintf(stdout, "%s", g_log_tags[level]); \
            fprintf(stdout, format,##args); \
            fprintf(stdout,"[log location]: file %s, function %s, line %d.\n", __FILE__, __FUNCTION__, __LINE__); \
        } \
    } while (0)

#define test_log_level(level, format, args...)  do { \
        if (level <= g_log_level) { \
            fprintf(stdout, "%s", g_log_tags[level]); \
            fprintf(stdout, format,##args); \
        } \
    } while (0)

#define test_logv(format, args...) test_log_level(e_log_level_all, format, ##args)
#define test_logd(format, args...) test_log_level(e_log_level_debug, format, ##args)
#define test_logn(format, args...) test_log_level_trace(e_log_level_notice, format, ##args)
#define test_logw(format, args...) test_log_level_trace(e_log_level_warning, format, ##args)
#define test_loge(format, args...) test_log_level_trace(e_log_level_error, format, ##args)

#define test_log(format, args...) test_log_level(e_log_level_always, format, ##args)
#else
#define test_logv LOG_VERBOSE
#define test_logd LOG_DEBUG
#define test_logn LOG_NOTICE
#define test_logw LOG_WARN
#define test_loge LOG_ERROR

#define test_log LOG_PRINTF
#endif

#define d_max_server_url_length  128
static TChar nvr_server_url[d_max_server_url_length] = "10.0.0.2";
static TU16 nvr_server_port = DDefaultSACPServerPort;
static TU16 local_port = DDefaultClientPort;

TInt ctest_init_params(TInt argc, TChar **argv)
{
    TInt i = 0;
    TInt ret = 0;

    //parse options
    for (i = 1; i < argc; i++) {
        if (!strcmp("--connect", argv[i])) {
            test_log("'--connect': server_url\n");
            if ((i + 1) < argc) {
                if ((d_max_server_url_length - 1) <= strlen(argv[i])) {
                    test_loge("[input argument error]: server url too long(%s).\n", argv[i]);
                } else {
                    snprintf(nvr_server_url, d_max_server_url_length - 1, "%s", argv[i + 1]);
                    nvr_server_url[d_max_server_url_length - 1] = 0x0;
                    test_log("[input argument]: server url (%s).\n", argv[i + 1]);
                }
            } else {
                test_loge("[input argument error]: '--connect', should follow with server url(%%s), argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("--port", argv[i])) {
            test_log("'--port': server port\n");
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                nvr_server_port = ret;
                test_log("server port %hu\n", nvr_server_port);
            } else {
                test_loge("[input argument error]: '--port', should follow with integer(port), argc %d, i %d.\n", argc, i);
                return (-2);
            }
            i ++;
        }
    }

    return 0;
}

static TInt __replace_enter(TChar *p_buffer, TUint size)
{
    while (size) {
        if (('\n') == (*p_buffer)) {
            *p_buffer = 0x0;
            return 0;
        }
        p_buffer ++;
        size --;
    }

    test_loge("no enter found, should be error\n");

    return 1;
}

void ctest_simple_loop(ICloudClientAgentV2 *p_client_agent)
{
    TInt ret = 0;
    EECode err = EECode_OK;
    TInt running = 1;

    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;
    TU8 pcm_buffer[64] = {0x01, 0x5a, 0x5b, 0x5c, 0x5d, 0x6a, 0x6b, 0x6c};

    while (running) {

        if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
            test_loge("read STDIN_FILENO fail\n");
            continue;
        }

        test_log("[user cmd debug]: read input '%s'\n", buffer);

        if (buffer[0] == '\n') {
            p_buffer = buffer_old;
            test_log("repeat last cmd:\n\t\t%s\n", buffer_old);
        } else if (buffer[0] == 'l') {
            test_log("show last cmd:\n\t\t%s\n", buffer_old);
            continue;
        } else {

            ret = __replace_enter(buffer, (128 - 1));
            DASSERT(0 == ret);
            if (ret) {
                test_loge("no enter found\n");
                continue;
            }
            p_buffer = buffer;

            //record last cmd
            strncpy(buffer_old, buffer, sizeof(buffer_old) - 1);
            buffer_old[sizeof(buffer_old) - 1] = 0x0;
        }

        test_log("[user cmd debug]: '%s'\n", p_buffer);

        switch (p_buffer[0]) {

            case 'q':   // exit
                test_log("[user cmd]: 'q', Quit\n");
                running = 0;
                break;

            default:
                break;
        }
    }

    return;
}

TInt main(TInt argc, TChar **argv)
{
    TInt ret;
    EECode err = EECode_OK;
    ICloudClientAgentV2 *p_client_agent = NULL;

    test_log("[ut flow]: ctest start\n");

    if ((ret = ctest_init_params(argc, argv)) < 0) {
        test_loge("mtest: mtest_init_params fail, return %d.\n", ret);
        goto ctest_main_exit;
    }

    p_client_agent = gfCreateCloudClientAgentV2(EProtocolType_SACP);

    if (DLikely(p_client_agent)) {
        TChar username[32] = "xman_0";
        TChar password[32] = "123";
        err = p_client_agent->ConnectToServer(nvr_server_url, nvr_server_port, (TChar *)username, (TChar *)password);
        if (DLikely(EECode_OK == err)) {
            ctest_simple_loop(p_client_agent);
            p_client_agent->Delete();
        } else {
            test_loge("ConnectToServer(%s) fail\n", nvr_server_url);
            goto ctest_main_exit;
        }
    } else {
        test_loge("gfCreateCloudClientAgent() fail\n");
        goto ctest_main_exit;
    }

    test_log("[ut flow]: ctest end\n");

ctest_main_exit:

    return 0;
}

