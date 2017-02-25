/*******************************************************************************
 * guardian.cpp
 *
 * History:
 *    2014/07/21 - [Zhi He] create file
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
#include <unistd.h>
#include <errno.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

DCONFIG_COMPILE_OPTION_CPPFILE_BEGIN
DCODE_DELIMITER;

#define DGuardianCMDLineMaxLength 512

typedef struct {
    TChar cmdline[DGuardianCMDLineMaxLength + 32];
    TU16 len_cmdline;
    TU16 last_state;

    TU16 server_port;
    TU8 loglevel;
    TU8 b_daemon;

    TU8 log_to_file;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;

    TU32 running;
} SGuadianContext;

void _guardian_print_version()
{
    TU32 major, minor, patch;
    TU32 year;
    TU32 month, day;

    LOG_NOTICE("\ncurrent version:\n");
    gfGetCommonVersion(major, minor, patch, year, month, day);
}

void _guardian_print_usage()
{
    LOG_NOTICE("\nguardian options:\n");
    LOG_NOTICE("\t'--port [port]': specify guardian port\n");
    LOG_NOTICE("\t'--loglevel [%%d]': set log level\n");
    LOG_NOTICE("\t'--daemon': daemon mode\n");
    LOG_NOTICE("\t'--log': log to file\n");
}

void _guardian_print_runtime_usage()
{
    LOG_NOTICE("\nguardian runtime cmds: press cmd + Enter\n");
    LOG_NOTICE("\t'q': quit unit test\n");
}

TInt guardian_init_params(SGuadianContext *p_content, TInt argc, TChar **argv)
{
    TInt i = 0;
    TInt ret = 0;

    TULong cur_group = 0;
    TULong cur_source_is_hd = 0;

    if (argc < 2) {
        _guardian_print_version();
        _guardian_print_usage();
        _guardian_print_runtime_usage();
        return 1;
    }

    //default settings
    p_content->server_port = 7123;

    //parse options
    for (i = 1; i < argc; i ++) {
        if (!strcmp("--port", argv[i])) {
            if ((i + 1) < argc) {
                if (1 == sscanf(argv[i + 1], "%d", &ret)) {
                    p_content->server_port = ret;
                }
            } else {
                LOG_NOTICE("[input argument error]: '--port', should follow with port(%%d), argc %d, i %d.\n", argc, i);
                return (-3);
            }
            i ++;
        } else if (!strcmp("--loglevel", argv[i])) {
            if ((i + 1) < argc) {
                if (1 == sscanf(argv[i + 1], "%d", &ret)) {
                    p_content->loglevel = ret;
                }
            } else {
                LOG_NOTICE("[input argument error]: '--port', should follow with loglevel (%%d), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
        } else if (!strcmp("--daemon", argv[i])) {
            p_content->b_daemon = 1;
        } else if (!strcmp("--log", argv[i])) {
            p_content->log_to_file = 1;
        } else {
            break;
        }
    }

    TU16 option_len = 0;
    //append cmdline
    for (; i < argc; i ++) {
        option_len = strlen(argv[i]);
        if ((p_content->len_cmdline + option_len + 1) < DGuardianCMDLineMaxLength) {
            p_content->cmdline[p_content->len_cmdline ++] = ' ';
            memcpy(p_content->cmdline + p_content->len_cmdline, argv[i], option_len);
            p_content->len_cmdline += option_len;
        } else {
            LOG_NOTICE("cmdline too long, exit\n");
            return (-10);
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

    LOG_NOTICE("no enter found, should be error\n");

    return 1;
}

static EECode guardianCallBack(void *owner, TU32 cur_state)
{
    LOG_NOTICE("\n cur state %d\n", cur_state);
    TInt ret = 0;
    SGuadianContext *p_context = (SGuadianContext *)owner;
    if (p_context) {
        switch (cur_state) {

            case EGuardianState_idle:
                if (EGuardianState_guard == p_context->last_state) {
                    LOG_NOTICE("EGuardianState_guard to EGuardianState_idle\n");
                    ret = system(p_context->cmdline);
                    LOG_NOTICE("system ret %d\n", ret);
                } else {
                    LOG_NOTICE("EGuardianState_idle: from %d to %d?\n", p_context->last_state, cur_state);
                }
                p_context->last_state = cur_state;
                break;

            case EGuardianState_wait_paring:
                if (EGuardianState_idle == p_context->last_state) {
                    LOG_NOTICE("start listen\n");
                } else if (EGuardianState_guard == p_context->last_state) {
                    LOG_NOTICE("EGuardianState_guard to EGuardianState_wait_paring\n");
                    ret = system(p_context->cmdline);
                    LOG_NOTICE("system ret %d\n", ret);
                } else {
                    LOG_NOTICE("EGuardianState_wait_paring: from %d to %d?\n", p_context->last_state, cur_state);
                }
                p_context->last_state = cur_state;
                break;

            case EGuardianState_guard:
                if (EGuardianState_wait_paring == p_context->last_state) {
                    LOG_NOTICE("start guard\n");
                } else {
                    LOG_NOTICE("EGuardianState_guard: from %d to %d?\n", p_context->last_state, cur_state);
                }
                p_context->last_state = cur_state;
                break;

            case EGuardianState_halt:
                LOG_NOTICE("EGuardianState_halt: from %d to %d?\n", p_context->last_state, cur_state);
                break;

            case EGuardianState_error:
                LOG_NOTICE("EGuardianState_error: from %d to %d?\n", p_context->last_state, cur_state);
                break;

            default:
                LOG_ERROR("bad state %d\n", cur_state);
                break;
        }
    }

    return EECode_OK;
}

TInt guardian_test(SGuadianContext *p_context)
{
    TInt ret = 0;
    EECode err = EECode_OK;

    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;
    TInt started = 0;

    IGuardian *p_guardian = gfCreateGuardian(EGuardianType_socket);
    if (DUnlikely(!p_guardian)) {
        LOG_NOTICE("gfCreateGuardian() fail\n");
        return (-1);
    }

    LOG_NOTICE("before Start\n");
    err = p_guardian->Start(p_context->server_port, guardianCallBack, (void *) p_context);
    ret = system(p_context->cmdline);
    LOG_NOTICE("@@system ret %d\n", ret);
    started = 1;
    //LOG_NOTICE("1111111 ret %d\n", ret);

    p_context->running = 1;

    //process user cmd line
    if (p_context->b_daemon) {
        while (p_context->running) {
            usleep(5000000);
        }
    } else {
        while (p_context->running) {
            usleep(1000000);
            if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
                TInt errcode = errno;
                if (errcode == EAGAIN) {
                    continue;
                }
                LOG_NOTICE("read STDIN_FILENO fail, errcode %d\n", errcode);
                continue;
            }

            if (buffer[0] == '\n') {
                p_buffer = buffer_old;
                LOG_NOTICE("repeat last cmd:\n\t\t%s\n", buffer_old);
            } else if (buffer[0] == 'l') {
                LOG_NOTICE("show last cmd:\n\t\t%s\n", buffer_old);
                continue;
            } else {

                ret = __replace_enter(buffer, (128 - 1));
                DASSERT(0 == ret);
                if (ret) {
                    LOG_NOTICE("no enter found\n");
                    continue;
                }
                p_buffer = buffer;

                //record last cmd
                strncpy(buffer_old, buffer, sizeof(buffer_old) - 1);
                buffer_old[sizeof(buffer_old) - 1] = 0x0;
            }

            LOG_NOTICE("[user cmd debug]: '%s'\n", p_buffer);

            switch (p_buffer[0]) {

                case 'q':   // exit
                    LOG_NOTICE("[user cmd]: 'q', Quit\n");
                    p_context->running = 0;
                    break;

                case 'h':   // help
                    _guardian_print_version();
                    _guardian_print_usage();
                    _guardian_print_runtime_usage();
                    break;

                default:
                    break;
            }
        }
    }

    if (p_guardian) {
        if (started) {
            started = 0;
            p_guardian->Stop();
        }
        p_guardian->Destroy();
        p_guardian = NULL;
    }

    return 0;
}

static void __guardian_sig_pipe(int a)
{
    LOG_NOTICE("sig pipe\n");
}

static TInt _guardian_default_initial_setting(SGuadianContext *p_content)
{
    return 1;
}

TInt main(TInt argc, TChar **argv)
{
    TInt ret = 0;
    SGuadianContext *p_content = (SGuadianContext *) DDBG_MALLOC(sizeof(SGuadianContext), "U0GD");

    LOG_NOTICE("[ut flow]: test start\n");

    if (!p_content) {
        LOG_NOTICE("no memory,request size %ld\n", sizeof(SGuadianContext));
        goto guardian_main_exit;
    }
    memset(p_content, 0x0, sizeof(SGuadianContext));

    signal(SIGPIPE, __guardian_sig_pipe);

    ret = _guardian_default_initial_setting(p_content);

    if ((ret = guardian_init_params(p_content, argc, argv)) < 0) {
        LOG_NOTICE("[ut flow]: guardian_init_params fail, return %d.\n", ret);
        goto guardian_main_exit;
    }

    if (p_content->b_daemon) {
        daemon(1, 0);
    }

    if (p_content->log_to_file) {
        gfOpenLogFile((TChar *)"./guardian.log");
    }

    ret = guardian_test(p_content);

    LOG_NOTICE("[ut flow]: test end\n");

guardian_main_exit:

    if (p_content) {
        DDBG_FREE(p_content, "U0GD");
        p_content = NULL;
    }

    gfCloseLogFile();

    return 0;
}

