/*******************************************************************************
 * im_client_test.cpp
 *
 * History:
 *    2014/06/18 - [Zhi He] create file
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

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"
#include "common_io.h"

#include "cloud_lib_if.h"

#include "lw_database_if.h"
#include "im_mw_if.h"
#include "sacp_types.h"

DCONFIG_COMPILE_OPTION_CPPFILE_BEGIN
DCODE_DELIMITER;

#define SERVER_URL "192.168.0.102"
#define SERVER_PORT 8260

TUniqueID g_admin_device[20] = {0};
TUniqueID g_shared_device[20] = {0};
TUniqueID g_friend[20] = {0};
SDeviceProfile g_admin_device_profile[20] = {0};

TU32 g_admin_device_count = 0;
TU32 g_shared_device_count = 0;
TU32 g_friend_count = 0;

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

typedef struct {
    TU8 daemon;
    TU8 async_mode;
    TU8 version;
    TU8 reserved2;

    TU16 server_port;
    TU16 reserved3;

    TChar *p_server_url;

} SIMClientTestContxt;

static TInt imclient_running = 1;

void _imclienttest_print_version()
{
    TU32 major, minor, patch;
    TU32 year;
    TU32 month, day;

    printf("\ncurrent version:\n");
    gfGetCommonVersion(major, minor, patch, year, month, day);
    printf("\tlibmwcg_common version: %d.%d.%d, date: %04d-%02d-%02d\n", major, minor, patch, year, month, day);
    gfGetCloudLibVersion(major, minor, patch, year, month, day);
    printf("\tlibmwcg_cloud_lib version: %d.%d.%d, date: %04d-%02d-%02d\n", major, minor, patch, year, month, day);
}

void _imclienttest_print_usage()
{
    printf("\nimclienttest options:\n");
}

void _imclienttest_print_runtime_usage()
{
    printf("\nimclienttest runtime cmds: press cmd + Enter\n");
    printf("\t'q': quit unit test\n");
    printf("\t'pall': will print all components' status\n");
}

TInt imclienttest_load_config(SIMClientTestContxt *p_content, const TChar *configfile)
{
    if (DUnlikely(!p_content || !configfile)) {
        test_loge("[input argument error]: imclienttest_load_config NULL params %p, %p.\n", p_content, configfile);
        return (-1);
    }

    IRunTimeConfigAPI *api = gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI, 128);
    if (DUnlikely(!api)) {
        test_loge("gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI) fail.\n");
        return (-2);
    }

    EECode err = api->OpenConfigFile((const TChar *)configfile, 0, 1, 1);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("OpenConfigFile(%s) fail, ret %d, %s.\n", configfile, err, gfGetErrorCodeString(err));
        return (-3);
    }

    TChar read_string[256] = {0};
    TChar read_string1[256] = {0};
    TU32 val = 0;

    err = api->GetContentValue("serverurl", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(serverurl) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        p_content->p_server_url = (TChar *)malloc(strlen(read_string) + 1);
        if (DLikely(p_content->p_server_url)) {
            snprintf(p_content->p_server_url, strlen(read_string) + 1, "%s", read_string);
        }
        test_log("server url: %s, %s\n", read_string, p_content->p_server_url);
    }

    err = api->GetContentValue("serverport", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(serverport) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->server_port = val;
        test_log("server port:%s, %d\n", read_string, p_content->server_port);
    }

    err = api->GetContentValue("daemon", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(daemon) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->daemon = val;
        test_log("daemon:%s, %d\n", read_string, p_content->daemon);
    }

    err = api->GetContentValue("async", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(async) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->async_mode = val;
        test_log("async_mode:%s, %d\n", read_string, p_content->async_mode);
    }

    err = api->GetContentValue("version", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(version) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->version = val;
        test_log("version:%s, %d\n", read_string, p_content->version);
    }

    err = api->CloseConfigFile();
    DASSERT_OK(err);

    api->Delete();

    return 0;
}

TInt imclienttest_init_params(SIMClientTestContxt *p_content, TInt argc, TChar **argv)
{
    TInt i = 0;
    TInt ret = 0;

    TULong cur_group = 0;
    TULong cur_source_is_hd = 0;
    TU8 total_group_number = 0;

    if (argc < 2) {
        //_imclienttest_print_version();
        _imclienttest_print_usage();
        _imclienttest_print_runtime_usage();
        return 1;
    }

    //parse options
    for (i = 1; i < argc; i++) {
        if (!strcmp("--port", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->server_port = ret;
                test_log("[input argument]: '--port': (%d).\n", p_content->server_port);
            } else {
                test_loge("[input argument error]: '--port', should follow with integer(port), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--connect", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                ret = strlen(argv[i + 1]);
                p_content->p_server_url = (TChar *) malloc(ret + 1);
                if (DUnlikely(!p_content->p_server_url)) {
                    LOG_ERROR("no memory, request %d\n", ret + 1);
                    return (-11);
                }
                strcpy(p_content->p_server_url, argv[i + 1]);
                test_log("[input argument]: '--connect': (%s).\n", p_content->p_server_url);
            } else {
                test_loge("[input argument error]: '--serverport', should follow with integer(port), argc %d, i %d.\n", argc, i);
                return (-10);
            }
            i ++;
        } else if (!strcmp("--version", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                ret = strlen(argv[i + 1]);
                p_content->version = ret;
                test_log("[input argument]: '--version': (%d).\n", p_content->version);
            } else {
                test_loge("[input argument error]: '--version', should follow with integer(port), argc %d, i %d.\n", argc, i);
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

EECode handleMessageCallback(void *owner, EECode ret, TU32 sub_type, TU32 reqres_bits, ETLV16Type type, TU8 *p_data, TU32 length)
{
    test_log("handleMessageCallback In\n");
    IIMClientAgent *p_agent = (IIMClientAgent *)owner;
    if (reqres_bits == (TU32)DSACPTypeBit_Responce) {
        switch (sub_type) {
            case ESACPClientCmdSubType_SinkOwnSourceDevice: {
                    test_log("SinkOwnSourceDevice: get im server responce:\n");
                    if (DUnlikely(ret != EECode_OK)) {
                        test_loge("ret is not OK, 0x%x\n", ret);
                    } else {
                        if (g_admin_device_count > 20) {
                            test_logw("TODO: too many admin\n");
                            break;
                        }
                        g_admin_device[g_admin_device_count] = *(TUniqueID *)p_data;
                        g_admin_device_count++;
                    }
                }
                break;

            case ESACPClientCmdSubType_SinkInviteFriend: {
                    test_log("SinkInviteFriend: get im server responce:\n");
                    if (DUnlikely(ret != EECode_OK)) {
                        test_loge("ret is not OK, 0x%x\n", ret);
                    } else {
                        if (g_friend_count > 20) {
                            test_logw("TODO: too many friend\n");
                            break;
                        }
                        g_friend[g_friend_count] = *(TUniqueID *)p_data;
                        g_friend_count++;
                    }
                }
                break;

            case ESACPClientCmdSubType_SinkDonateSourceDevice:
            case ESACPClientCmdSubType_SinkRemoveFriend:
            case ESACPClientCmdSubType_SinkUpdateFriendPrivilege:
                if (ret == EECode_OK) {
                    test_log("cmd 0x%x return from server is OK\n", sub_type);
                } else {
                    test_loge("cmd 0x%x return from server is 0x%x, not OK!\n", sub_type, ret);
                }
                break;

            case ESACPClientCmdSubType_SinkRetrieveFriendsList:
                if (DLikely(EECode_OK == ret)) {
                    TUniqueID *p_id_list = (TUniqueID *)p_data;
                    TU32 count = length / sizeof(TUniqueID);
                    test_log("cmd ESACPClientCmdSubType_SinkRetrieveFriendsList, get id list from server:\n");
                    g_friend_count = count;
                    for (TU32 i = 0; i < count; i++) {
                        test_log("num: %u, id: 0x%llx\n", i, p_id_list[i]);
                        g_friend[i] = p_id_list[i];
                    }
                } else {
                    test_loge("cmd ESACPClientCmdSubType_SinkRetrieveFriendsList fail!\n");
                }
                break;
            case ESACPClientCmdSubType_SinkRetrieveAdminDeviceList:
                if (DLikely(EECode_OK == ret)) {
                    TUniqueID *p_id_list = (TUniqueID *)p_data;
                    TU32 count = length / sizeof(TUniqueID);
                    test_log("cmd ESACPClientCmdSubType_SinkRetrieveAdminDeviceList, get id list from server:\n");
                    g_admin_device_count = count;
                    for (TU32 i = 0; i < count; i++) {
                        test_log("num: %u, id: 0x%llx\n", i, p_id_list[i]);
                        g_admin_device[i] = p_id_list[i];
                    }
                } else {
                    test_loge("cmd ESACPClientCmdSubType_SinkRetrieveAdminDeviceList fail!\n");
                }
                break;
            case ESACPClientCmdSubType_SinkRetrieveSharedDeviceList:
                if (DLikely(EECode_OK == ret)) {
                    TUniqueID *p_id_list = (TUniqueID *)p_data;
                    TU32 count = length / sizeof(TUniqueID);
                    test_log("cmd ESACPClientCmdSubType_SinkRetrieveSharedDeviceList, get id list from server:\n");
                    g_shared_device_count = count;
                    for (TU32 i = 0; i < count; i++) {
                        test_log("num: %u, id: 0x%llx\n", i, p_id_list[i]);
                        g_shared_device[i] = p_id_list[i];
                    }
                } else {
                    test_loge("cmd ESACPClientCmdSubType_SinkRetrieveSharedDeviceList fail!\n");
                }
                break;

            case ESACPClientCmdSubType_SinkQuerySource: {
                    if (DLikely(ret == EECode_OK)) {
                        if (type == ETLV16Type_SourceDeviceDataServerAddress) {
                            test_log("SinkQuerySoure: data server address: %s\n", (TChar *)p_data);
                        } else if (type == ETLV16Type_SourceDeviceStreamingTag) {
                            test_log("SinkQuerySoure: live streaming tag: %s, vod streaming tag: vod_%s\n", (TChar *)p_data, (TChar *)p_data);
                        } else if (type == ETLV16Type_SourceDeviceUploadingPort) {
                            TSocketPort port = *(TSocketPort *)p_data;
                            test_log("SinkQuerySoure: data server uploading port: %hu\n", port);
                        } else if (type == ETLV16Type_SourceDeviceStreamingPort) {
                            TSocketPort port = *(TSocketPort *)p_data;
                            test_log("SinkQuerySoure: data server streaming port: %hu\n", port);
                        } else {
                            test_loge("SinkQuerySoure: unknown type: 0x%x\n", type);
                        }
                    } else {
                        test_loge("cmd 0x%x fail!\n", sub_type);
                    }
                }
                break;

            default:
                break;
        }
    } else if (reqres_bits == (TU32)DSACPTypeBit_Request) {
        test_log("Request from other clients!\n");
        switch (sub_type) {
            case ESACPClientCmdSubType_SinkInviteFriend:
                test_log("Get request: invite friend: \n");
                if (g_friend_count > 20) {
                    test_logw("TODO: too many friend\n");
                    break;
                }
                g_friend[g_friend_count] = *(TUniqueID *)p_data;
                test_log("get Add friend invite, friend: %llx\n", g_friend[g_friend_count]);
                if (p_agent->AcceptFriend(g_friend[g_friend_count]) != EECode_OK) {
                    test_loge("AcceptFriend fail\n");
                    break;
                }
                g_friend_count++;
                test_log("AcceptFriend done\n");
                break;

            default:
                break;
        }
    }
    return EECode_OK;
}

TInt imclient_test_async(SIMClientTestContxt *p_context)
{
    TInt ret = 0;
    EECode err = EECode_OK;

    IIMClientAgentAsync *p_agent = gfCreateIMClientAgentAsync(EProtocolType_SACP);
    if (DUnlikely(!p_agent)) {
        LOG_ERROR("gfCreateIMClientAgent() fail\n");
        return (-1);
    }

    //p_agent->SetReceiveMessageCallBack((void*)p_agent, handleMessageCallback);

    TChar buffer[128] = {0};
    TInt imclient_running = 1;

    TChar *p_del = NULL;

    if (!p_context->daemon) {

        //process user cmd line
        while (imclient_running) {
            //add sleep to avoid affecting the performance
            usleep(500000);
            memset(buffer, 0x0, sizeof(buffer));
            if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
                TInt errcode = errno;
                if (errcode == EAGAIN) {
                    continue;
                }
                test_loge("read STDIN_FILENO fail, errcode %d\n", errcode);
                continue;
            }

            TUint length = strlen(buffer);
            if (buffer[length - 1] == '\n') {
                buffer[length - 1] = '\0';
            }
            test_log("[user cmd debug]: read input '%s'\n", buffer);

            switch (buffer[0]) {

                case 'q':   // exit
                    if (strncmp(buffer, "querydevice", 11) == 0) {
                        TInt id_num = 0;
                        if (1 != sscanf(buffer, "querydevice %d", &id_num)) {
                            test_loge("[cmd query device info]: 'querydevice id_num' \n");
                            break;
                        }
                        if (id_num >= g_admin_device_count) {
                            break;
                        }
                        if (g_admin_device_count == 0) {
                            test_logw("[cmd query device info]: please own deivce firstly\n");
                            break;
                        }

                        if (EECode_OK != p_agent->QueryDeivceInfo(g_admin_device[id_num])) {
                            test_loge("[cmd query device info]: id num %d, id: 0x%llx fail\n", id_num, g_admin_device[id_num]);
                        } else {
                            test_log("[cmd query device info] done!id: 0x%llx\n", g_admin_device[id_num]);
                        }
                    } else if (strncmp(buffer, "q", 1) == 0) {
                        test_log("[user cmd]: 'q', Quit\n");
                        imclient_running = 0;
                    }
                    break;

                case 'p':
                    test_log("===admin device list===\n");
                    for (TU32 i = 0; i < g_admin_device_count; i++) {
                        test_log("device %u, id 0x%llx\n", i, g_admin_device[i]);
                    }
                    test_log("===friend list===\n");
                    for (TU32 i = 0; i < g_friend_count; i++) {
                        test_log("friend %u, id 0x%llx\n", i, g_friend[i]);
                    }
                    test_log("===shared device list===\n");
                    for (TU32 i = 0; i < g_shared_device_count; i++) {
                        test_log("device %u, id 0x%llx\n", i, g_shared_device[i]);
                    }
                    break;

                case 'r': {
                        if (strncmp(buffer, "reg", 3) == 0) {
                            //test_log("[cmd register]: ");
                            p_del = strchr(buffer, ' ');
                            if (!p_del) {
                                test_loge("[cmd register]: 'reg account_name passeword' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TChar *p_account = p_del;
                            p_del = strchr(p_account, ' ');
                            if (!p_del) {
                                test_loge("[cmd register]: 'reg account_name passeword' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TChar *p_password = p_del;

                            if (EECode_OK != p_agent->Register(p_account, p_password, p_context->p_server_url, p_context->server_port)) {
                                test_loge("[cmd register]: account: %s, password: %s fail\n", p_account, p_password);
                            } else {
                                test_log("[cmd register]: account: %s, password: %s done\n", p_account, p_password);
                            }
                        } else if (strncmp(buffer, "rmfriend", 7) == 0) {
                            p_del = strchr(buffer, ' ');
                            if (!p_del) {
                                test_loge("[cmd remove friend]: 'rfriend id_num' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TInt id_num = 0;
                            if (1 != sscanf(p_del, "%d", &id_num)) {
                                test_loge("[cmd remove friend]: 'rfriend id_num' \n");
                                break;
                            }
                            if (id_num >= g_friend_count) {
                                break;
                            }

                            if (EECode_OK != p_agent->RemoveFriend(g_friend[g_friend_count])) {
                                test_loge("[cmd remove friend]: id num %d, id: 0x%llx fail\n", id_num, g_friend[g_friend_count]);
                            } else {
                                test_log("[cmd remove friend]: remove friend done!\n");
                            }
                        } else if (strncmp(buffer, "rmdevice", 7) == 0) {
                            //todo
                        } else if (strncmp(buffer, "retrieve", 8) == 0) {
                            if (buffer[8] == 'f') {
                                err = p_agent->RetrieveFriendList();
                            } else if (buffer[8] == 'a') {
                                err = p_agent->RetrieveAdminDeviceList();
                            } else if (buffer[8] == 's') {
                                err = p_agent->RetrieveSharedDeviceList();
                            }

                            if (DUnlikely(err != EECode_OK)) {
                                test_loge("[cmd retrieve%c] fail\n", buffer[8]);
                            } else {
                                test_log("[cmd retrieve%c] done\n", buffer[8]);
                            }
                        }
                    }
                    break;

                case 'l': {
                        if (strncmp(buffer, "login", 5) == 0) {
                            p_del = strchr(buffer, ' ');
                            if (!p_del) {
                                test_loge("[cmd register]: 'reg account_name passeord' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TChar *p_account = p_del;
                            p_del = strchr(p_account, ' ');
                            if (!p_del) {
                                test_loge("[cmd register]: 'reg account_name passeord' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TChar *p_password = p_del;

                            if (EECode_OK != p_agent->LoginUserAccount(p_account, p_password, p_context->p_server_url, p_context->server_port)) {
                                test_loge("[cmd login]: account: %s, password: %s fail\n", p_account, p_password);
                            } else {
                                test_log("[cmd login]: account: %s, password: %s done\n", p_account, p_password);
                            }
                        } else if (strncmp(buffer, "logout", 6) == 0) {
                            if (EECode_OK != p_agent->Logout()) {
                                test_loge("[cmd logout]: fail\n");
                            } else {
                                test_log("[cmd logout]: done\n");
                            }
                        }
                    }
                    break;

                case 'a': {
                        //add friend by name
                        //afriendname
                        if (strncmp(buffer, "addfriend", 9) == 0) {
                            p_del = strchr(buffer, ' ');
                            if (!p_del) {
                                test_loge("[cmd addfriend]: 'addfriend friend_name' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TChar *p_account = p_del;
                            if (EECode_OK != p_agent->AddFriend(p_account)) {
                                test_loge("[cmd addfriend]: friend name: %s fail!\n", p_account);
                            } else {
                                test_log("[cmd addfriend]: friend name: %s done\n", p_account);
                            }
                        } else if (strncmp(buffer, "adddevice", 9) == 0) {
                            p_del = strchr(buffer, ' ');
                            if (!p_del) {
                                test_loge("[cmd adddevice]: 'adddevice device_code' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TChar *p_account = p_del;
                            if (EECode_OK != p_agent->AcquireOwnership(p_account)) {
                                test_loge("[cmd adddevice]: device name: %s fail!\n", p_account);
                            } else {
                                test_log("[cmd adddevice]: device name: %s done\n", p_account);
                            }
                        }
                    }
                    break;

                case 's': {
                        if (0 == strncmp(buffer, "share", 5)) {
                            TInt device_id_num = 0, friend_id_num = 0;
                            if (2 != sscanf(buffer, "share %d %d", &device_id_num, &friend_id_num)) {
                                test_loge("[cmd share device to friend]: 'share device_id_num friend_id_num'\n");
                                break;
                            }
                            if (device_id_num >= g_admin_device_count || friend_id_num >= g_friend_count) {
                                test_loge("[cmd share device to friend]: invalid id_num\n");
                                break;
                            }
                            if (EECode_OK != p_agent->GrantPrivilege(&g_admin_device[device_id_num], 1, g_friend[friend_id_num])) {
                                test_loge("[cmd share device to friend]: fail, deivce id: %llx, friend id: %llx\n", g_admin_device[device_id_num], g_friend[friend_id_num]);
                                break;
                            }
                            test_log("[cmd share device to friend] done\n");
                        }
                    }
                    break;

                case 'h':   // help
                    _imclienttest_print_version();
                    _imclienttest_print_usage();
                    _imclienttest_print_runtime_usage();
                    break;

                default:
                    break;
            }
        }

    } else {
        while (imclient_running) {
            usleep(5000000);
        }
    }

    LOG_NOTICE("while out\n");
    p_agent->Logout();
    p_agent->Destroy();

    if (p_context->p_server_url) {
        free(p_context->p_server_url);
    }

    return 0;
}


TInt imclient_test(SIMClientTestContxt *p_context)
{
    TInt ret = 0;
    EECode err = EECode_OK;

    TU32 version = p_context->version;
    IIMClientAgent *p_agent = gfCreateIMClientAgent(EProtocolType_SACP, 2);
    if (DUnlikely(!p_agent)) {
        LOG_ERROR("gfCreateIMClientAgent() fail\n");
        return (-1);
    }

    TChar buffer[128] = {0};
    TInt imclient_running = 1;

    TChar *p_del = NULL;
    TChar name[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
    TChar password[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};

    if (!p_context->daemon) {

        //process user cmd line
        while (imclient_running) {
            //add sleep to avoid affecting the performance
            usleep(500000);
            memset(buffer, 0x0, sizeof(buffer));
            if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
                TInt errcode = errno;
                if (errcode == EAGAIN) {
                    continue;
                }
                test_loge("read STDIN_FILENO fail, errcode %d\n", errcode);
                continue;
            }

            TUint length = strlen(buffer);
            if (buffer[length - 1] == '\n') {
                buffer[length - 1] = '\0';
            }
            test_log("[user cmd debug]: read input '%s'\n", buffer);

            switch (buffer[0]) {

                case 'q':   // exit
                    if (strncmp(buffer, "querydevice", 11) == 0) {
                        TInt id_num = 0;
                        if (1 != sscanf(buffer, "querydevice %d", &id_num)) {
                            test_loge("[cmd query device info]: 'querydevice id_num' \n");
                            break;
                        }
                        if (id_num >= g_admin_device_count) {
                            break;
                        }
                        if (g_admin_device_count == 0) {
                            test_logw("[cmd query device info]: please own deivce firstly\n");
                            break;
                        }

                        if (EECode_OK != p_agent->QueryDeivceProfile(g_admin_device[id_num], g_admin_device_profile + id_num)) {
                            test_loge("[cmd query device info]: id num %d, id: 0x%llx fail\n", id_num, g_admin_device[id_num]);
                        } else {
                            test_log("[cmd query device info] done!id: 0x%llx\n", g_admin_device[id_num]);
                            test_log("==streaming tag: %s==\n", g_admin_device_profile[id_num].name);
                            test_log("==nickname: %s==\n", g_admin_device_profile[id_num].nickname);
                            test_log("==dataserver_address: %s==\n", g_admin_device_profile[id_num].dataserver_address);
                            test_log("==on_line: %u==\n", g_admin_device_profile[id_num].status);
                        }
                    } else if (strncmp(buffer, "q", 1) == 0) {
                        test_log("[user cmd]: 'q', Quit\n");
                        imclient_running = 0;
                    }
                    break;

                case 'p':
                    test_log("===admin device list===\n");
                    for (TU32 i = 0; i < g_admin_device_count; i++) {
                        test_log("device %u, id 0x%llx\n", i, g_admin_device[i]);
                        test_log("==streaming tag: %s==\n", g_admin_device_profile[i].name);
                        test_log("==nickname: %s==\n", g_admin_device_profile[i].nickname);
                        test_log("==dataserver_address: %s==\n", g_admin_device_profile[i].dataserver_address);
                        test_log("==on_line: %u==\n", g_admin_device_profile[i].status);
                    }
                    test_log("===friend list===\n");
                    for (TU32 i = 0; i < g_friend_count; i++) {
                        test_log("friend %u, id 0x%llx\n", i, g_friend[i]);
                    }
                    test_log("===shared device list===\n");
                    for (TU32 i = 0; i < g_shared_device_count; i++) {
                        test_log("device %u, id 0x%llx\n", i, g_shared_device[i]);
                    }
                    break;

                case 'r': {
                        if (strncmp(buffer, "reg", 3) == 0) {
                            //test_log("[cmd register]: ");
                            p_del = strchr(buffer, ' ');
                            if (!p_del) {
                                test_loge("[cmd register]: 'reg account_name passeword' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TChar *p_account = p_del;
                            p_del = strchr(p_account, ' ');
                            if (!p_del) {
                                test_loge("[cmd register]: 'reg account_name passeword' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TChar *p_password = p_del;
                            TUniqueID id = 0;

                            if (EECode_OK != p_agent->Register(p_account, p_password, p_context->p_server_url, p_context->server_port, id)) {
                                test_loge("[cmd register]: account: %s, password: %s fail\n", p_account, p_password);
                            } else {
                                test_log("[cmd register]: account: %s, password: %s done\n", p_account, p_password);
                                snprintf(name, DMAX_ACCOUNT_NAME_LENGTH_EX, "%s", p_account);
                                snprintf(password, DMAX_ACCOUNT_NAME_LENGTH_EX, "%s", p_password);
                            }
                        } else if (strncmp(buffer, "rmfriend", 7) == 0) {
                            p_del = strchr(buffer, ' ');
                            if (!p_del) {
                                test_loge("[cmd remove friend]: 'rfriend id_num' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TInt id_num = 0;
                            if (1 != sscanf(p_del, "%d", &id_num)) {
                                test_loge("[cmd remove friend]: 'rfriend id_num' \n");
                                break;
                            }
                            if (id_num >= g_friend_count) {
                                break;
                            }

                            if (EECode_OK != p_agent->RemoveFriend(g_friend[g_friend_count])) {
                                test_loge("[cmd remove friend]: id num %d, id: 0x%llx fail\n", id_num, g_friend[g_friend_count]);
                            } else {
                                test_log("[cmd remove friend]: remove friend done!\n");
                            }
                        } else if (strncmp(buffer, "rmdevice", 7) == 0) {
                            //todo
                        } else if (strncmp(buffer, "retrieve", 8) == 0) {
                            SAgentIDListBuffer *p_id_list = NULL;
                            TU8 list_type = 0;
                            if (buffer[8] == 'f') {
                                err = p_agent->RetrieveFriendList(p_id_list);
                                list_type = 0;
                            } else if (buffer[8] == 'a') {
                                err = p_agent->RetrieveAdminDeviceList(p_id_list);
                                list_type = 1;
                            } else if (buffer[8] == 's') {
                                err = p_agent->RetrieveSharedDeviceList(p_id_list);
                                list_type = 2;
                            } else {
                                test_loge("invalid cmd; %s\n", buffer);
                                break;
                            }

                            if (DUnlikely(err != EECode_OK)) {
                                test_loge("[cmd retrieve%c] fail\n", buffer[8]);
                            } else {
                                if (p_id_list) {
                                    test_log("[cmd retrieve%c] done\n", buffer[8]);
                                    for (TU32 i = 0; i < p_id_list->id_count; i++) {
                                        test_log("==%d, %llu==\n", i, p_id_list->p_id_list[i]);
                                        if (list_type == 0) {
                                            g_friend[g_friend_count] = p_id_list->p_id_list[i];
                                            g_friend_count++;
                                        } else if (list_type == 1) {
                                            g_admin_device[g_admin_device_count] = p_id_list->p_id_list[i];
                                            g_admin_device_count++;
                                        } else if (list_type == 2) {
                                            g_shared_device[g_shared_device_count] = p_id_list->p_id_list[i];
                                            g_shared_device_count++;
                                        }
                                    }
                                    p_agent->FreeIDList(p_id_list);
                                } else {
                                    test_log("[cmd retrieve%c], but no one\n", buffer[8]);
                                }
                            }
                        } else if (strncmp(buffer, "releasedc", 9) == 0) {
                            TInt device_id_num = 0;
                            if (1 != sscanf(buffer, "releasedc %d", &device_id_num)) {
                                test_loge("[cmd release dynamic code]: 'releasedc device_id_num'\n");
                                break;
                            }
                            if (device_id_num >= g_admin_device_count) {
                                test_loge("[cmd release dynamic code]: invalid id_num\n");
                                break;
                            }
                            if (EECode_OK != p_agent->ReleaseDynamicCode(g_admin_device[device_id_num])) {
                                test_loge("[cmd release dynamic code]: fail, device id %llx\n", g_admin_device[device_id_num]);
                                break;
                            }
                            test_log("[cmd release dynamic code]: done\n");
                        }
                    }
                    break;

                case 'l': {
                        if (strncmp(buffer, "login", 5) == 0) {
                            p_del = strchr(buffer, ' ');
                            if (!p_del) {
                                test_loge("[cmd register]: 'reg account_name passeord' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TChar *p_account = p_del;
                            p_del = strchr(p_account, ' ');
                            if (!p_del) {
                                test_loge("[cmd register]: 'reg account_name passeord' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TChar *p_password = p_del;

                            TUniqueID ret_id = DInvalidUniqueID;
                            if (EECode_OK != p_agent->LoginUserAccount(p_account, p_password, p_context->p_server_url, p_context->server_port, ret_id)) {
                                test_loge("[cmd login]: account: %s, password: %s fail\n", p_account, p_password);
                            } else {
                                test_log("[cmd login]: account: %s, password: %s done\n", p_account, p_password);
                                snprintf(name, DMAX_ACCOUNT_NAME_LENGTH_EX, "%s", p_account);
                                snprintf(password, DMAX_ACCOUNT_NAME_LENGTH_EX, "%s", p_password);
                            }
                        } else if (strncmp(buffer, "logout", 6) == 0) {
                            if (EECode_OK != p_agent->Logout()) {
                                test_loge("[cmd logout]: fail\n");
                            } else {
                                test_log("[cmd logout]: done\n");
                            }
                        }
                    }
                    break;

                case 'a': {
                        //add friend by name
                        //afriendname
                        TUniqueID id = 0;
                        if (strncmp(buffer, "addfriend", 9) == 0) {
                            p_del = strchr(buffer, ' ');
                            if (!p_del) {
                                test_loge("[cmd addfriend]: 'addfriend friend_name' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TChar *p_account = p_del;
                            if (EECode_OK != p_agent->AddFriend(p_account, id)) {
                                test_loge("[cmd addfriend]: friend name: %s fail!\n", p_account);
                            } else {
                                test_log("[cmd addfriend]: friend name: %s done, id %llu\n", p_account, id);
                                g_friend[g_friend_count] = id;
                                g_friend_count++;
                            }
                        } else if (strncmp(buffer, "adddevice", 9) == 0) {
                            p_del = strchr(buffer, ' ');
                            if (!p_del) {
                                test_loge("[cmd adddevice]: 'adddevice device_code' \n");
                                break;
                            }
                            *p_del = '\0';
                            p_del++;
                            TChar *p_account = p_del;
                            if (EECode_OK != p_agent->AcquireOwnership(p_account, id)) {
                                test_loge("[cmd adddevice]: device name: %s fail!\n", p_account);
                            } else {
                                test_log("[cmd adddevice]: device name: %s done, id %llu\n", p_account, id);
                                g_admin_device[g_admin_device_count] = id;
                                g_admin_device_count++;
                            }
                        } else if (strncmp(buffer, "acquiredc", 9) == 0) {
                            TInt device_id_num = 0;
                            if (1 != sscanf(buffer, "acquiredc %d", &device_id_num)) {
                                test_loge("[cmd acquire dynamic code]: 'acquiredc device_id_num'\n");
                                break;
                            }
                            if (device_id_num >= g_admin_device_count) {
                                test_loge("[cmd acquire dynamic code]: invalid id_num\n");
                                break;
                            }
                            TU32 dynamic_code = 0;
                            if (EECode_OK != p_agent->AcquireDynamicCode(g_admin_device[device_id_num], password, dynamic_code)) {
                                test_loge("[cmd acquire dynamic code]: fail, deivce id: %llx\n", g_admin_device[device_id_num]);
                                break;
                            }
                            test_log("[cmd acquire dynamic code] done, dynamic code %u\n", dynamic_code);
                        }
                    }
                    break;

                case 's': {
                        if (0 == strncmp(buffer, "share", 5)) {
                            TInt device_id_num = 0, friend_id_num = 0;
                            if (2 != sscanf(buffer, "share %d %d", &device_id_num, &friend_id_num)) {
                                test_loge("[cmd share device to friend]: 'share device_id_num friend_id_num'\n");
                                break;
                            }
                            if (device_id_num >= g_admin_device_count || friend_id_num >= g_friend_count) {
                                test_loge("[cmd share device to friend]: invalid id_num\n");
                                break;
                            }
                            if (EECode_OK != p_agent->GrantPrivilege(&g_admin_device[device_id_num], 1, g_friend[friend_id_num], 0)) {
                                test_loge("[cmd share device to friend]: fail, deivce id: %llx, friend id: %llx\n", g_admin_device[device_id_num], g_friend[friend_id_num]);
                                break;
                            }
                            test_log("[cmd share device to friend] done\n");
                        }
                    }
                    break;

                case 'h':   // help
                    _imclienttest_print_version();
                    _imclienttest_print_usage();
                    _imclienttest_print_runtime_usage();
                    break;

                default:
                    break;
            }
        }

    } else {
        while (imclient_running) {
            usleep(5000000);
        }
    }

    LOG_NOTICE("while out\n");
    p_agent->Logout();
    p_agent->Destroy();

    if (p_context->p_server_url) {
        free(p_context->p_server_url);
    }

    return 0;
}


static void __imclient_test_sig_pipe(int a)
{
    test_logw("sig pipe\n");
}

static void _imclienttest_default_initial_setting(SIMClientTestContxt *p_content)
{
    memset(p_content, 0x0, sizeof(SIMClientTestContxt));

    p_content->daemon = 0;
    p_content->server_port = DDefaultSACPIMServerPort;
}

TInt main(TInt argc, TChar **argv)
{
    TInt ret = 0;
    SIMClientTestContxt content;

    signal(SIGPIPE,  __imclient_test_sig_pipe);

    _imclienttest_default_initial_setting(&content);

    if (1 == argc) {
        ret = imclienttest_load_config(&content, "imclient_conf.ini");
        if (0 > ret) {
            goto imclienttest_main_exit;
        }
    } else {
        if ((ret = imclienttest_init_params(&content, argc, argv)) < 0) {
            test_loge("imclienttest_init_params fail, return %d.\n", ret);
            goto imclienttest_main_exit;
        } else if (ret > 0) {
            goto imclienttest_main_exit;
        }
    }

    if (content.daemon) {
        daemon(1, 0);
    }

    if (1 || content.async_mode == 0) {
        test_log("==imclient_test==\n");
        imclient_test(&content);
    } else {
        test_log("==imclient_test_async==\n");
        imclient_test_async(&content);
    }

    test_log("[ut flow]: test end\n");

imclienttest_main_exit:

    return 0;
}


