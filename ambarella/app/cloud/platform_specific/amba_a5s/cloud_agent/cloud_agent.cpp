/*******************************************************************************
 * cloud_agent.cpp
 *
 * History:
 *  2014/09/11 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
#include "am_include.h"
#include "am_utility.h"
#include "transfer/am_data_transfer.h"
#include "transfer/am_transfer_client.h"

#include <sys/time.h>
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/times.h>
#include <sys/select.h>

#include <pthread.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "cloud_lib_if.h"
#include "sacp_types.h"
#include "amba_cloud_agent.h"

DCONFIG_COMPILE_OPTION_CPPFILE_BEGIN
DCODE_DELIMITER;

static const TChar g_account_conf_file[] = "/etc/camera/cloud_account.conf";
static const TChar g_owner_file[] = "/etc/camera/owner.conf";

TInt g_running = 1;

typedef struct {
    TU8 daemon;
    TU8 stream_index;
    TU8 audio_disable;
    TU8 M;

    TSocketPort gd_port;
    TU16 reserved1;

    TSocketHandler gd;
} SCloudAgentContext;

static void cloud_agent_print_usage()
{
    printf("usage:\n");
    printf("--help [print this help info]\n");

    printf("\t\t'--gdport %%d': enable guardian\n");
    printf("\t\t'--stream %%d': upload stream index\n");
    printf("\t\t'--M %%d': specify M\n");
    printf("\t\t'--enableaudio': enable audio path\n");
    printf("\t\t'--disableaudio': disable audio path\n");
}

static EECode cloud_agent_test_init_params(TInt argc, TChar **argv, SCloudAgentContext *p_context)
{
    TInt i = 0;
    TInt param = 0;

    //parse options
    for (i = 1; i < argc; i++) {
        if (!strcmp("--help", argv[i])) {
            cloud_agent_print_usage();
        } else if (!strcmp("--daemon", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &param))) {
                if (!param) {
                    p_context->daemon = 0;
                    LOG_NOTICE("non daemon mode\n");
                } else {
                    p_context->daemon = 1;
                    LOG_NOTICE("daemon mode\n");
                }
            } else {
                LOG_ERROR("[input argument error]: '--daemon', should follow with integer(enable or disable daemon mode), argc %d, i %d.\n", argc, i);
                return EECode_BadParam;
            }
            i ++;
        } else if (!strcmp("--gdport", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &param))) {
                p_context->gd_port = param;
                LOG_NOTICE("guardian port %d\n", p_context->gd_port);
            } else {
                LOG_ERROR("[input argument error]: '--gdport', should follow with integer(guardian port), argc %d, i %d.\n", argc, i);
                return EECode_BadParam;
            }
            i ++;
        } else if (!strcmp("--stream", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &param))) {
                p_context->stream_index = param;
                LOG_NOTICE("stream index %d\n", p_context->stream_index);
            } else {
                LOG_ERROR("[input argument error]: '--stream', should follow with integer(stream index), argc %d, i %d.\n", argc, i);
                return EECode_BadParam;
            }
            i ++;
        } else if (!strcmp("--M", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &param))) {
                p_context->M = param;
                LOG_NOTICE("M %d\n", p_context->M);
            } else {
                LOG_ERROR("[input argument error]: '--M', should follow with integer(M), argc %d, i %d.\n", argc, i);
                return EECode_BadParam;
            }
            i ++;
        } else if (!strcmp("--enableaudio", argv[i])) {
            p_context->audio_disable = 0;
            LOG_NOTICE("enable audio\n");
        } else if (!strcmp("--disableaudio", argv[i])) {
            p_context->audio_disable = 1;
            LOG_NOTICE("disable audio\n");
        }
    }

    return EECode_OK;
}

static void default_setting(SCloudAgentContext *context)
{
    context->daemon = 0;
    context->stream_index = 1;
    context->M = 1;
    context->audio_disable = 0;
    context->gd = -1;
    context->gd_port = 0;
}

static void sigstop(int sig_num)
{
    g_running = 0;
}

int main(int argc, char **argv)
{
    EECode err = EECode_OK;
    SCloudAgentContext context;
    default_setting(&context);

    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);

    err = cloud_agent_test_init_params(argc, argv, &context);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("cloud_agent_test_init_params, return %d, %s\n", err, gfGetErrorCodeString(err));
        return (-6);
    }

    if (context.gd_port) {
        context.gd = gfCreateGuardianAgent(EGuardianType_socket, context.gd_port);
    }

    if (context.daemon) {
        daemon(1, 0);
    }

    CAmbaCloudAgent *p_agent = CAmbaCloudAgent::Create(g_account_conf_file, g_owner_file);

    if (DUnlikely(NULL == p_agent)) {
        LOG_ERROR("CAmbaCloudAgent::Create fail\n");
        return (-1);
    }

    err = p_agent->Start(context.stream_index, context.audio_disable, context.M);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("p_agent->Start() faile return %d, %s\n", err, gfGetErrorCodeString(err));
        return (-2);
    }

    while (g_running) {
        usleep(500000);
    }

    p_agent->Stop();
    p_agent->Destroy();

    return 0;
}

