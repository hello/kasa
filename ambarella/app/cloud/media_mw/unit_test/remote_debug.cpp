/*******************************************************************************
 * remote_debug.cpp
 *
 * History:
 *    2014/03/16 - [Zhi He] create file
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

#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "cloud_lib_if.h"

#include "media_mw_if.h"

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

#define LOCKFILE "/var/run/remote_debug.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

static int rd_lockfile(int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK; /* write lock */
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0; //lock the whole file
    return(fcntl(fd, F_SETLK, &fl));
}

static int rd_already_running(const char *filename)
{
    int fd;
    char buf[16];

    fd = open(filename, O_RDWR | O_CREAT, LOCKMODE);
    if (fd < 0) {
        test_loge("can't open %s\n", filename);
        perror("open");
        return (-1);
    }

    if (rd_lockfile(fd) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            test_loge("file: %s already locked\n", filename);
            close(fd);
            return (-3);
        }
        test_loge("can't lock %s.\n", filename);
        return (-2);
    }

    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);

    return fd;
}

#define DRemoteDebugStringMaxLength 256

typedef struct {
    TChar cloud_server_addr[DRemoteDebugStringMaxLength];
    //TChar cloud_tag[DRemoteDebugStringMaxLength];

    TU16 server_port;
    TU8 preset_loglevel;
    TU8 loglevel;

    TU8 b_monitor_mode;
    TU8 monitor_thread_state;
    TU8 b_force_running;
    TU8 reserved2;

    ISimpleQueue *p_cmd_queue;
} SRemoteDebugContext;

void _remote_debug_print_version()
{
    TU32 major, minor, patch;
    TU32 year;
    TU32 month, day;

    printf("\ncurrent version:\n");
    gfGetCommonVersion(major, minor, patch, year, month, day);
    printf("\tlibmwcg_common version: %d.%d.%d, date: %04d-%02d-%02d\n", major, minor, patch, year, month, day);
    gfGetCloudLibVersion(major, minor, patch, year, month, day);
    printf("\tlibmwcg_cloud_lib version: %d.%d.%d, date: %04d-%02d-%02d\n", major, minor, patch, year, month, day);
    gfGetMediaMWVersion(major, minor, patch, year, month, day);
    printf("\tlibmwcg_media_mw version: %d.%d.%d, date: %04d-%02d-%02d\n", major, minor, patch, year, month, day);
}

void _remote_debug_print_usage()
{
    printf("\nremote_debug options:\n");
    printf("\t'--connect [server_addr]': specify cloud server addr\n");
    printf("\t'--port [server_port]': specify cloud server port\n");
    //printf("\t'--cloudtag [cloud_tag]': specify cloud tag name\n");
    printf("\t'--loglevel [%%d]': set log level\n");
    printf("\t'--force': force mode\n");
}

void _remote_debug_print_runtime_usage()
{
    printf("\nremote_debug runtime cmds: press cmd + Enter\n");
    printf("\t'q': quit unit test\n");
}

TInt remote_debug_init_params(SRemoteDebugContext *p_content, TInt argc, TChar **argv)
{
    TInt i = 0;
    TInt ret = 0;

    TULong cur_group = 0;
    TULong cur_source_is_hd = 0;

    if (argc < 2) {
        _remote_debug_print_version();
        _remote_debug_print_usage();
        _remote_debug_print_runtime_usage();
        return 1;
    }

    //default settings
    p_content->server_port = DDefaultSACPServerPort;

    //parse options
    for (i = 1; i < argc; i++) {
        if (!strcmp("--connect", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(&p_content->cloud_server_addr[0], DRemoteDebugStringMaxLength, "%s", argv[i + 1]);
            } else {
                test_loge("[input argument error]: '--connect', should follow with server url(%%s), argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        }
#if 0
        else if (!strcmp("--cloudtag", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(&p_content->cloud_tag[0], DRemoteDebugStringMaxLength, "%s", argv[i + 1]);
            } else {
                test_loge("[input argument error]: '--cloudtag', should follow with cloudtag(%%s), argc %d, i %d.\n", argc, i);
                return (-2);
            }
            i ++;
        }
#endif
        else if (!strcmp("--port", argv[i])) {
            if ((i + 1) < argc) {
                if (1 == sscanf(argv[i + 1], "%d", &ret)) {
                    p_content->server_port = ret;
                }
            } else {
                test_loge("[input argument error]: '--port', should follow with server_port(%%d), argc %d, i %d.\n", argc, i);
                return (-3);
            }
            i ++;
        } else if (!strcmp("--force", argv[i])) {
            p_content->b_force_running = 1;
        } else if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                if ((ret > ELogLevel_Verbose) || (ret < ELogLevel_Fatal)) {
                    test_loge("[input argument error]: '--loglevel', ret(%d) should between %d to %d.\n", ret, ELogLevel_Fatal, ELogLevel_Verbose);
                    return (-9);
                } else {
                    p_content->preset_loglevel = 1;
                    p_content->loglevel = ret;
                    const TChar *p_log_level_string = NULL;
                    gfGetLogLevelString((ELogLevel)p_content->loglevel, p_log_level_string);
                    test_log("[input argument]: '--loglevel': (%d) (%s).\n", p_content->loglevel, p_log_level_string);
                }
            } else {
                test_loge("[input argument error]: '--loglevel', should follow with integer(loglevel), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else {
            test_loge("unknown option[%d]: %s\n", i, argv[i]);
            return (-255);
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

static EECode read_callback(void *owner, TU8 *&p_data, TInt &datasize, TU32 &remainning)
{
    TChar *pstring = (TChar *)p_data;
    static TU32 print_section_number = 0;

    //printf("read_callback: %p, %d, remaining %d\n", p_data, datasize, remainning);

    //printf("\033[2J");
    printf("\ncount %d, size %d\n", print_section_number++, datasize);
    printf("%s\n", pstring);
    //DASSERT(!remainning);

    return EECode_OK;
}

TInt remote_debug_test(SRemoteDebugContext *p_context)
{
    TInt ret = 0;
    EECode err = EECode_OK;

    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;
    TInt running = 1;
    TU32 param_0 = 0;
    TU32 b_monitor = 0;
    TU32 b_connected = 0;

    TU32 loop_count_max = 300;
    TU32 loop_current_count = 0;
    TU32 loop_cmd_type = 0;// 1: all, 2: push channel
    TU32 loop_param1 = 0;
    TU32 loop_param2 = 0;

    ICloudClientAgentV2 *p_agent = gfCreateCloudClientAgentV2(EProtocolType_SACP);
    if (DUnlikely(!p_agent)) {
        test_loge("gfCreateCloudClientAgentV2() fail\n");
        return (-1);
    }

    TU64 hardware_verification_input = 0;
    err = p_agent->ConnectToServer(p_context->cloud_server_addr, p_context->server_port, (TChar *)"remote_debug", (TChar *)"");

    if (DLikely(EECode_OK != err)) {
        test_loge("ConnectToServer(%s:%hu) fail\n", p_context->cloud_server_addr, p_context->server_port);
        return (-2);
    } else {
        test_log("ConnectToServer(%s:%hu)\n", p_context->cloud_server_addr, p_context->server_port);
    }

    IIPCAgent *p_remote_agent = gfIPCAgentFactory(EIPCAgentType_UnixDomainSocket);
    if (DUnlikely(!p_remote_agent)) {
        test_loge("gfIPCAgentFactory() fail\n");
        return (-3);
    } else {
        err = p_remote_agent->CreateContext((TChar *)"/tmp/remote_bridge", 0, NULL, read_callback);
    }

    TInt flag_stdin = fcntl(STDIN_FILENO, F_GETFL);
    if (fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK) == -1) {
        test_loge("[error]: stdin_fileno set error.\n");
    }

    //process user cmd line
    while (running) {
        //add sleep to avoid affecting the performance
        //test_log("before sleep\n");
        usleep(20000);
        //test_log("after sleep\n");
        //memset(buffer, 0x0, sizeof(buffer));
        if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
            TInt errcode = errno;
            if (errcode == EAGAIN) {
                //test_log("EAGAIN, loop_cmd_type %d, loop_current_count %d\n", loop_cmd_type, loop_current_count);
                if ((loop_cmd_type) && (loop_current_count > loop_count_max)) {
                    loop_current_count = 0;
                    if (1 == loop_cmd_type) {
                        err = p_agent->DebugPrintAllChannels(loop_param2);
                    } else if (2 == loop_cmd_type) {
                        err = p_agent->DebugPrintChannel(loop_param1, loop_param2);
                    } else if (3 == loop_cmd_type) {
                        err = p_agent->DebugPrintCloudPipeline(loop_param1, loop_param2);
                    } else if (4 == loop_cmd_type) {
                        err = p_agent->DebugPrintStreamingPipeline(loop_param1, loop_param2);
                    } else if (5 == loop_cmd_type) {
                        err = p_agent->DebugPrintRecordingPipeline(loop_param1, loop_param2);
                    } else {
                        test_loge("bad %d\n", loop_cmd_type);
                        running = 0;
                        continue;
                    }
                    loop_current_count = 0;
                    if (DUnlikely(EECode_OK != err)) {
                        running = 0;
                        LOG_ERROR("return err %d, %s\n", err, gfGetErrorCodeString(err));
                        continue;
                    }

                } else if (loop_cmd_type) {
                    loop_current_count ++;
                }
                continue;
            }
            test_loge("read STDIN_FILENO fail, errcode %d\n", errcode);
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

            case 'p': {
                    if (!strncmp("pall", p_buffer, strlen("pall"))) {
                        test_log("print all\n");
                        err = p_agent->DebugPrintAllChannels(param_0);
                        if (DUnlikely(EECode_OK != err)) {
                            running = 0;
                            LOG_ERROR("return err %d, %s\n", err, gfGetErrorCodeString(err));
                            continue;
                        }
                        loop_cmd_type = 1;
                        loop_current_count = 0;
                        loop_param2 = param_0;
                    } else if (!strncmp("pchannel", p_buffer, strlen("pchannel"))) {
                        TU32 index = 0;
                        if (1 != sscanf(p_buffer, "pchannel %d", &index)) {
                            test_loge("pchannel should follow by a integer(channel index)\n");
                            break;
                        }
                        test_log("print channel %d\n", index);
                        err = p_agent->DebugPrintChannel(index, param_0);
                        if (DUnlikely(EECode_OK != err)) {
                            running = 0;
                            LOG_ERROR("return err %d, %s\n", err, gfGetErrorCodeString(err));
                            continue;
                        }
                        loop_cmd_type = 1;
                        loop_current_count = 0;
                        loop_param2 = param_0;
                    } else if (p_buffer[1] == 'p') {
                        TU32 index = 0;
                        if (p_buffer[2] == 's') {
                            if (1 != sscanf(p_buffer, "pps %d", &index)) {
                                test_loge("pps should follow by a integer(streaming pipeline index)\n");
                                break;
                            }
                            test_log("print streaming pipeline %d\n", index);
                            err = p_agent->DebugPrintStreamingPipeline(index, param_0);
                            if (DUnlikely(EECode_OK != err)) {
                                running = 0;
                                LOG_ERROR("return err %d, %s\n", err, gfGetErrorCodeString(err));
                                continue;
                            }
                            loop_cmd_type = 5;
                            loop_current_count = 0;
                            loop_param1 = index;
                            loop_param2 = param_0;
                        } else if (p_buffer[2] == 'c') {
                            if (1 != sscanf(p_buffer, "ppc %d", &index)) {
                                test_loge("ppc should follow by a integer(cloud pipeline index)\n");
                                break;
                            }
                            test_log("print cloud pipeline %d\n", index);
                            err = p_agent->DebugPrintCloudPipeline(index, param_0);
                            if (DUnlikely(EECode_OK != err)) {
                                running = 0;
                                LOG_ERROR("return err %d, %s\n", err, gfGetErrorCodeString(err));
                                continue;
                            }
                            loop_cmd_type = 3;
                            loop_current_count = 0;
                            loop_param1 = index;
                            loop_param2 = param_0;
                        } else if (p_buffer[2] == 'r') {
                            if (1 != sscanf(p_buffer, "ppr %d", &index)) {
                                test_loge("ppr should follow by a integer(recording pipeline index)\n");
                                break;
                            }
                            test_log("print recording pipeline %d\n", index);
                            err = p_agent->DebugPrintRecordingPipeline(index, param_0);
                            if (DUnlikely(EECode_OK != err)) {
                                running = 0;
                                LOG_ERROR("return err %d, %s\n", err, gfGetErrorCodeString(err));
                                continue;
                            }
                            loop_cmd_type = 4;
                            loop_current_count = 0;
                            loop_param1 = index;
                            loop_param2 = param_0;
                        }
                    }
                }
                break;

            case 'c': {
                    if (0 == strncmp(p_buffer, "cmonitor", strlen("cmonitor"))) {
                        test_log("cmonitor, switch to monitor mode\n");
                        param_0 = DLOG_MASK_TO_REMOTE;
                        if (!b_connected) {
                            err = p_remote_agent->Start();
                            if (DLikely(EECode_OK == err)) {
                                b_connected = 1;
                            } else {
                                test_log("connect fail\n");
                            }
                        }
                    } else if (0 == strncmp(p_buffer, "cdebug", strlen("cdebug"))) {
                        test_log("cdebug, switch to debug mode\n");
                        param_0 = DLOG_MASK_TO_SNAPSHOT;
                        if (b_connected) {
                            err = p_remote_agent->Stop();
                            if (DLikely(EECode_OK == err)) {
                                b_connected = 0;
                            } else {
                                test_log("dis connect fail\n");
                            }
                        }
                    }
                }
                break;

            case 'h':   // help
                _remote_debug_print_version();
                _remote_debug_print_usage();
                _remote_debug_print_runtime_usage();
                break;

            default:
                break;
        }
    }

    if (fcntl(STDIN_FILENO, F_SETFL, flag_stdin) == -1) {
        test_loge("[error]: stdin_fileno set error");
    }

    if (p_remote_agent) {
        if (b_connected) {
            p_remote_agent->Stop();
        }
        p_remote_agent->Destroy();
        p_remote_agent = NULL;
    }

    p_agent->Delete();

    return 0;
}

static void __remote_debug_sig_pipe(int a)
{
    test_logw("sig pipe\n");
}

static TInt _remote_debug_default_initial_setting(SRemoteDebugContext *p_content)
{
    DASSERT(p_content);

    FILE *file = fopen("/etc/remote_debug.conf", "rt");

    if (!file) {
        test_log("no [/etc/remote_debug.conf] file\n");
        return 0;
    }

    //default settings
    TUint length = 0;
    fgets(&p_content->cloud_server_addr[0], DRemoteDebugStringMaxLength, file);
    length = strlen(p_content->cloud_server_addr);
    DASSERT((length + 1) < DRemoteDebugStringMaxLength);
    if (p_content->cloud_server_addr[length - 1] == '\n') {
        p_content->cloud_server_addr[length - 1] = 0;
    } else {
        test_log("%02x %02x", '\n', p_content->cloud_server_addr[length - 1]);
    }

    TChar port_string[32] = {0};
    TU32 port = DDefaultSACPServerPort;
    fgets(&port_string[0], 32, file);
    sscanf(port_string, "%d\n", &port);
    p_content->server_port = port;

#if 0
    fgets(&p_content->cloud_tag[0], DRemoteDebugStringMaxLength, file);
    length = strlen(p_content->cloud_tag);
    DASSERT((length + 1) < DRemoteDebugStringMaxLength);
    if (p_content->cloud_tag[length - 1] == '\n') {
        p_content->cloud_tag[length - 1] = 0;
    } else {
        test_log("%02x %02x", '\n', p_content->cloud_tag[length - 1]);
    }
#endif
    //p_content->server_port = DDefaultSACPServerPort;

    return 1;
}

static TInt _remote_debug_save_setting(SRemoteDebugContext *p_content)
{
    DASSERT(p_content);
    TChar byte = '\n';

    FILE *file = fopen("/etc/remote_debug.conf", "wt");

    if (!file) {
        test_loge("can not open [/etc/remote_debug.conf] file\n");
        return 0;
    }
    test_log("write cloud_server_addr: %s\n", p_content->cloud_server_addr);
    fputs(&p_content->cloud_server_addr[0], file);
    fwrite(&byte, 1, 1, file);

    TChar port_string[32] = {0};
    snprintf(port_string, 32, "%d\n", p_content->server_port);
    test_log("write port: %s", port_string);
    fputs(&port_string[0], file);
#if 0
    test_log("write cloud_tag: %s\n", p_content->cloud_tag);
    fputs(&p_content->cloud_tag[0], file);
    fwrite(&byte, 1, 1, file);
#endif
    fclose(file);

    return 1;
}

TInt main(TInt argc, TChar **argv)
{
    TInt ret = 0;
    TUint i = 0;
    TInt exfd = 0;
    SRemoteDebugContext *p_content = (SRemoteDebugContext *) malloc(sizeof(SRemoteDebugContext));

    test_log("[ut flow]: test start\n");

    if (!p_content) {
        test_loge("no memory,request size %ld\n", sizeof(SRemoteDebugContext));
        goto remote_debug_main_exit;
    }
    memset(p_content, 0x0, sizeof(SRemoteDebugContext));
    p_content->p_cmd_queue = gfCreateSimpleQueue(10);
    DASSERT(p_content->p_cmd_queue);

    signal(SIGPIPE, __remote_debug_sig_pipe);

    //AutoDiscoveryExample exam;

    ret = _remote_debug_default_initial_setting(p_content);

    if ((ret) && (argc < 2)) {
        test_log("[ut flow]: load from file\n");
        test_log("      cloud_server_addr: %s\n", p_content->cloud_server_addr);
        //test_log("      cloud_tag: %s\n", p_content->cloud_server_addr);
    } else {
        if ((ret = remote_debug_init_params(p_content, argc, argv)) < 0) {
            test_loge("[ut flow]: camera_test_init_params fail, return %d.\n", ret);
            goto remote_debug_main_exit;
        } else if (ret > 0) {
            goto remote_debug_main_exit;
        }
    }

    if (!p_content->b_force_running) {
        exfd = rd_already_running(LOCKFILE);
        if (0 > exfd) {
            test_loge("there's another instance, ret %d\n", exfd);
            goto remote_debug_main_exit;
        }
    }

    ret = remote_debug_test(p_content);

    test_log("[ut flow]: test end\n");

    ret = _remote_debug_save_setting(p_content);
    if (0 == ret) {
        test_loge("write fail\n");
    }

remote_debug_main_exit:

    if (p_content) {
        if (p_content->p_cmd_queue) {
            p_content->p_cmd_queue->Destroy();
            p_content->p_cmd_queue = NULL;
        }
        free(p_content);
        p_content = NULL;
    }

    close(exfd);

    return 0;
}

