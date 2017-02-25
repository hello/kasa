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
#include "user_protocol_default_if.h"

#include "lw_database_if.h"
#include "im_mw_if.h"
#include "sacp_types.h"

DCONFIG_COMPILE_OPTION_CPPFILE_BEGIN
DCODE_DELIMITER;

//unit test self log
#if 0
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
    TU8 version;
    TU8 reserved0;

    TSocketPort server_port;

    TChar server_url[DMaxServerUrlLength];

    IIMClientAgent *p_agent;
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
    printf("\t '--connect [server url]'\n");
    printf("\t '--port [server port]'\n");
}

void _imclienttest_print_runtime_usage()
{
    printf("\nimclienttest runtime cmds: press cmd + Enter\n");
    printf("\t'q': quit unit test\n");

    printf("\tuser related:\n");
    printf("\t\t 'ureg username password': register\n");
    printf("\t\t 'ulogin username password': user login\n");
    printf("\t\t 'uadmindevice': list user owned device list\n");
    printf("\t\t 'ushareddevice': list user shared device list\n");
    printf("\t\t 'ufriend': list user friend list\n");
    printf("\t\t 'ushareddevice': list user shared device list\n");
    printf("\t\t 'uowndevice production_code': user own device\n");
    printf("\t\t 'uaddfriend friend_name': user add friend\n");
    printf("\t\t 'urmfriend friend_id': user remove friend\n");
    printf("\t\t 'ushare device_id friend_id': user share device to friend\n");
    printf("\t\t 'ulogout': user logout\n");
    printf("\t\t 'uchangepassword oripassword newpassword': user change password\n");
    printf("\t\t 'uchangenickname nickname': user change nickname\n");
    printf("\t\t 'uchangedevicenickname nickname': user change device nickname\n");
    printf("\t\t 'uunbind deviceid': user unbind device\n");
    printf("\t\t 'udonate target_user deviceid': user donate device\n");

    printf("\tdevice related:\n");
    printf("\t\t 'dlogin device_id password': device login\n");
    printf("\t\t 'dsetowner ownername': set owner\n");
    printf("\t\t 'dlogout': device logout\n");

    printf("\tsecurity related:\n");
    printf("\t\t 'sresetpassword account_name new_password answer1 answer2': resetpassword\n");
    printf("\t\t 'sshowsecurityquestion account_name': show security questions\n");

    printf("\tmessage relay:\n");
    printf("\t\t 'mpost target_id message': post message to target\n");
    printf("\t\t 'mtlv8zoom target_id zoom_factor zoom_offset_x zoom_offset_y': video zoom: zoom_factor[0, 10], top-left: zoom_offset_x[0, 10], zoom_offset_y[0, 10]\n");
    printf("\t\t 'mtlv8br target_id bitrate': video set bitrate\n");
    printf("\t\t 'mtlv8fr target_id framerate': video set framerate\n");
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
        if ((strlen(read_string) >= DMaxServerUrlLength)) {
            test_loge("server url too long\n");
            return (-4);
        }
        memcpy(p_content->server_url, read_string, strlen(read_string));
        test_log("server url: %s\n", p_content->server_url);
    }

    err = api->GetContentValue("serverport", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(serverport) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        return (-5);
    } else {
        sscanf(read_string, "%d", &val);
        p_content->server_port = val;
        test_log("server port:%s, %d\n", read_string, p_content->server_port);
    }

    err = api->CloseConfigFile();
    DASSERT_OK(err);

    api->Destroy();

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
                if ((ret >= DMaxServerUrlLength)) {
                    test_loge("server url too long\n");
                    return (-4);
                }
                memcpy(p_content->server_url, argv[i + 1], ret);
                test_log("[input argument]: '--connect': (%s).\n", p_content->server_url);
            } else {
                test_loge("[input argument error]: '--connect', should follow with server url, argc %d, i %d.\n", argc, i);
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

void _imclient_receive_message_callback(void *owner, TUniqueID id, TU8 *p_message, TU32 message_length, TU32 need_reply)
{
    test_log("message from [%llx], need reply %d\n", id, need_reply);
    gfPrintMemory(p_message, message_length);
}

void _imclient_system_notify_callback(void *owner, ESystemNotify notify, void *context)
{
    switch (notify) {

        case ESystemNotify_IM_FriendInvitation: {
                SSystemNotifyIMFriendInvitation *c = (SSystemNotifyIMFriendInvitation *)context;
                test_log("\tESystemNotify_IM_FriendInvitation, id %llx, name %s\n", c->invitor_id, c->invitor_name);
            }
            break;

        case ESystemNotify_IM_DeviceDonation:
            test_log("\tESystemNotify_IM_DeviceDonation\n");
            break;

        case ESystemNotify_IM_DeviceSharing:
            test_log("\tESystemNotify_IM_DeviceSharing\n");
            break;

        case ESystemNotify_IM_UpdateOwnedDeviceList:
            test_log("\tESystemNotify_IM_UpdateOwnedDeviceList\n");
            break;

        default:
            test_loge("unknown notify %x\n", notify);
            break;
    }
}

void _imclient_error_callback(void *owner, EECode err)
{
    test_loge("error callback %d, %s\n", err, gfGetErrorCodeString(err));
}

TInt imclient_test(SIMClientTestContxt *p_context)
{
    TInt ret = 0;
    EECode err = EECode_OK;

    IIMClientAgent *p_agent = gfCreateIMClientAgent(EProtocolType_SACP, 2);
    p_context->p_agent = p_agent;
    if (DUnlikely(!p_agent)) {
        test_loge("gfCreateIMClientAgent() fail\n");
        return (-1);
    }

    p_agent->SetCallBack((void *) p_context, _imclient_receive_message_callback, _imclient_system_notify_callback, _imclient_error_callback);

    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;

    TChar *p_del = NULL;

    TU32 user_login = 0;
    TU32 device_login = 0;

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

        //test_log("[user cmd debug]: read input '%s'\n", buffer);
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

        switch (p_buffer[0]) {

            case 'q':
                test_log("[user cmd]: 'q', Quit\n");
                imclient_running = 0;
                break;

                //user
            case 'u': {
                    if (strncmp(p_buffer + 1, "login", 5) == 0) {
                        TChar *p_account = NULL;
                        TChar *p_password = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[ulogin]: no username, cmd should be: 'ulogin account_name passeord' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_account = p_del;

                        p_del = strchr(p_account, ' ');
                        if (!p_del) {
                            test_loge("[ulogin]: no password, cmd should be: 'ulogin account_name passeord' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_password = p_del;

                        TUniqueID ret_id = DInvalidUniqueID;
                        err = p_agent->LoginUserAccount(p_account, p_password, p_context->server_url, p_context->server_port, ret_id);
                        if (EECode_OK != err) {
                            test_loge("[ulogin]: account: %s, password: %s fail, ret %d, %s\n", p_account, p_password, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[ulogin]: account: %s, password: %s, [id %llx] done\n", p_account, p_password, ret_id);
                            user_login = 1;
                        }

                    } else if (strncmp(p_buffer + 1, "logout", 6) == 0) {
                        if (!user_login) {
                            test_loge("[ulogout]: not login yet\n");
                            break;
                        }
                        err = p_agent->Logout();
                        if (EECode_OK != err) {
                            test_loge("[ulogout] fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                        } else {
                            test_log("[ulogout]: done\n");
                        }
                        user_login = 0;
                    } else if (strncmp(p_buffer + 1, "reg", 3) == 0) {
                        if (user_login || device_login) {
                            test_loge("[ureg] fail, already login(user_login %d, device_login %d), should logout first\n", user_login, device_login);
                            break;
                        }
                        TChar *p_account = NULL;
                        TChar *p_password = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[ureg]: no username, cmd should be: 'ureg account_name passeord' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_account = p_del;

                        p_del = strchr(p_account, ' ');
                        if (!p_del) {
                            test_loge("[ureg]: no password, cmd should be: 'ureg account_name passeord' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_password = p_del;

                        TUniqueID ret_id = DInvalidUniqueID;
                        err = p_agent->Register(p_account, p_password, p_context->server_url, p_context->server_port, ret_id);
                        if (EECode_OK != err) {
                            test_loge("[ureg]: account: %s, password: %s fail, ret %d, %s\n", p_account, p_password, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[ureg]: account: %s, password: %s, [id %llx] done\n", p_account, p_password, ret_id);
                            user_login = 1;
                        }

                    } else if (strncmp(p_buffer + 1, "admindevice", 11) == 0) {
                        if (!user_login) {
                            test_loge("[uadmindevice] fail, not login(user_login %d), should login first\n", user_login);
                            break;
                        }
                        SAgentIDListBuffer *p_list = NULL;
                        err = p_agent->RetrieveAdminDeviceList(p_list);
                        if (EECode_OK != err) {
                            test_loge("[uadmindevice]: RetrieveAdminDeviceList(): fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                            break;
                        } else if (!p_list) {
                            test_log("[uadmindevice]: done, no admin device\n");
                        } else {
                            test_log("[uadmindevice]: done, admin device:\n");
                            for (TU32 i = 0; i < p_list->id_count; i ++) {
                                test_log("\tid[%d]: %llx\n", i, p_list->p_id_list[i]);
                            }
                            p_agent->FreeIDList(p_list);
                            p_list = NULL;
                        }
                    } else if (strncmp(p_buffer + 1, "shareddevice", 12) == 0) {
                        if (!user_login) {
                            test_loge("[ushareddevice] fail, not login(user_login %d), should login first\n", user_login);
                            break;
                        }
                        SAgentIDListBuffer *p_list = NULL;
                        err = p_agent->RetrieveSharedDeviceList(p_list);
                        if (EECode_OK != err) {
                            test_loge("[ushareddevice]: RetrieveSharedDeviceList(): fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                            break;
                        } else if (!p_list) {
                            test_log("[ushareddevice]: done, no shared device\n");
                        } else {
                            test_log("[ushareddevice]: done, shared device:\n");
                            for (TU32 i = 0; i < p_list->id_count; i ++) {
                                test_log("\tid[%d]: %llx\n", i, p_list->p_id_list[i]);
                            }
                            p_agent->FreeIDList(p_list);
                            p_list = NULL;
                        }
                    } else if (strncmp(p_buffer + 1, "friend", 6) == 0) {
                        if (!user_login) {
                            test_loge("[ufriend] fail, not login(user_login %d), should login first\n", user_login);
                            break;
                        }
                        SAgentIDListBuffer *p_list = NULL;
                        err = p_agent->RetrieveFriendList(p_list);
                        if (EECode_OK != err) {
                            test_loge("[ufriend]: RetrieveFriendList() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                            break;
                        } else if (!p_list) {
                            test_log("[ufriend]: done, no friend\n");
                        } else {
                            test_log("[ufriend]: done, friend list:\n");
                            for (TU32 i = 0; i < p_list->id_count; i ++) {
                                test_log("\tid[%d]: %llx\n", i, p_list->p_id_list[i]);
                            }
                            p_agent->FreeIDList(p_list);
                            p_list = NULL;
                        }
                    } else if (strncmp(p_buffer + 1, "addfriend", 9) == 0) {
                        if (!user_login) {
                            test_loge("[uaddfriend] fail, not login(user_login %d), should login first\n", user_login);
                            break;
                        }

                        TChar *p_account = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[addfriend]: no username, cmd should be: 'uaddfriend account_name' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_account = p_del;

                        TUniqueID ret_id = DInvalidUniqueID;
                        err = p_agent->AddFriend(p_account, ret_id);
                        if (EECode_OK != err) {
                            test_loge("[addfriend]: AddFriend(%s): fail, ret %d, %s\n", p_account, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[addfriend]: AddFriend(%s): done, id %llx\n", p_account, ret_id);
                        }
                    } else if (strncmp(p_buffer + 1, "rmfriend", 8) == 0) {
                        if (!user_login) {
                            test_loge("[urmfriend] fail, not login(user_login %d), should login first\n", user_login);
                            break;
                        }

                        TChar *p_account = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[urmfriend]: no username, cmd should be: 'urmfriend friend_id'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_account = p_del;

                        TUniqueID id = DInvalidUniqueID;
                        sscanf(p_account, "%llx", &id);
                        err = p_agent->RemoveFriend(id);
                        if (EECode_OK != err) {
                            test_loge("[addfriend]: RemoveFriend(%llx): fail, ret %d, %s\n", id, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[addfriend]: RemoveFriend(%llx): done\n", id);
                        }
                    } else if (strncmp(p_buffer + 1, "owndevice", 9) == 0) {
                        if (!user_login) {
                            test_loge("[uowndevice] fail, not login(user_login %d), should login first\n", user_login);
                            break;
                        }

                        TChar *p_device = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[uowndevice]: no production_code, cmd should be: 'uowndevice production_code'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_device = p_del;

                        TUniqueID device_id = DInvalidUniqueID;

                        err = p_agent->AcquireOwnership(p_device, device_id);
                        if (EECode_OK != err) {
                            test_loge("[uowndevice]: AcquireOwnership(%s): fail, ret %d, %s\n", p_device, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[uowndevice]: AcquireOwnership(%s): done\n", p_device);
                        }
                    } else if (strncmp(p_buffer + 1, "share", 5) == 0) {
                        if (!user_login) {
                            test_loge("[ushare] fail, not login(user_login %d), should login first\n", user_login);
                            break;
                        }

                        TChar *p_device = NULL;
                        TChar *p_user = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[ushare]: no device id, cmd should be: 'ushare deviceid userid'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_device = p_del;

                        p_del = strchr(p_device, ' ');
                        if (!p_del) {
                            test_loge("[ushare]: no username, cmd should be: 'ushare deviceid userid'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_user = p_del;

                        TUniqueID device_id = DInvalidUniqueID, user_id = DInvalidUniqueID;
                        sscanf(p_device, "%llx", &device_id);
                        sscanf(p_user, "%llx", &user_id);

                        err = p_agent->GrantPrivilegeSingleDevice(device_id, user_id, 1);
                        if (EECode_OK != err) {
                            test_loge("[ushare]: GrantPrivilegeSingleDevice(%llx, %llx): fail, ret %d, %s\n", device_id, user_id, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[ushare]: GrantPrivilegeSingleDevice(%llx, %llx): done\n", device_id, user_id);
                        }
                    } else if (strncmp(p_buffer + 1, "querydevice", 11) == 0) {
                        if (!user_login) {
                            test_loge("[uquerydevice] fail, not login(user_login %d), should login first\n", user_login);
                            break;
                        }

                        TChar *p_device = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[uquerydevice]: no device id, cmd should be: 'uquerydevice deviceid'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_device = p_del;

                        TUniqueID device_id = DInvalidUniqueID;
                        sscanf(p_device, "%llx", &device_id);

                        SDeviceProfile profile;
                        err = p_agent->QueryDeivceProfile(device_id, &profile);
                        if (EECode_OK != err) {
                            test_loge("[uquerydevice]: QueryDeivceProfile(%llx): fail, ret %d, %s\n", device_id, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[uquerydevice]: QueryDeivceProfile(%llx): done\n", device_id);
                            test_log("\t\t name:\t%s\n", profile.name);
                            test_log("\t\t nickname:\t%s\n", profile.nickname);
                            test_log("\t\t url:\t%s\n", profile.dataserver_address);
                            test_log("\t\t port:\t%hu\n", profile.rtsp_streaming_port);
                            test_log("\t\t status:\t%hu\n", profile.status);
                        }
                    } else if (strncmp(p_buffer + 1, "changepassword", 14) == 0) {
                        if (!user_login) {
                            test_loge("[uchangepassword] fail, not login(user_login %d), should login first\n", user_login);
                            break;
                        }

                        TChar *p_newpassword = NULL;
                        TChar *p_oripassword = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[uchangepassword]: no new password, cmd should be: 'uchangepassword oripassword newpassword'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_oripassword = p_del;

                        p_del = strchr(p_oripassword, ' ');
                        if (!p_del) {
                            test_loge("[uchangepassword]: no ori password, cmd should be: 'uchangepassword oripassword newpassword'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_newpassword = p_del;

                        err = p_agent->UpdatePassword(p_oripassword, p_newpassword);
                        if (EECode_OK != err) {
                            test_loge("[uchangepassword]: UpdatePassword(ori [%s], new [%s]): fail, ret %d, %s\n", p_oripassword, p_newpassword, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[uchangepassword]: UpdatePassword(ori [%s], new [%s]): done\n", p_oripassword, p_newpassword);
                        }
                    } else if (strncmp(p_buffer + 1, "changenickname", 14) == 0) {
                        if (!user_login) {
                            test_loge("[uchangenickname] fail, not login(user_login %d), should login first\n", user_login);
                            break;
                        }

                        TChar *p_nickname = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[uchangenickname]: no nickname, cmd should be: 'uchangenickname nickname'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_nickname = p_del;

                        err = p_agent->UpdateNickname(p_nickname);
                        if (EECode_OK != err) {
                            test_loge("[uchangenickname]: UpdateNickname(%s): fail, ret %d, %s\n", p_nickname, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[uchangenickname]: UpdateNickname(%s): done\n", p_nickname);
                        }
                    } else if (strncmp(p_buffer + 1, "unbind", 6) == 0) {
                        if (!user_login) {
                            test_loge("[uunbind] fail, not login(user_login %d), should login first\n", user_login);
                            break;
                        }

                        TChar *p_device = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[uunbind]: no device id, cmd should be: 'uunbind deviceid'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_device = p_del;

                        TUniqueID device_id = DInvalidUniqueID;
                        sscanf(p_device, "%llx", &device_id);

                        err = p_agent->DonateOwnership(NULL, device_id);
                        if (EECode_OK != err) {
                            test_loge("[uunbind]: DonateOwnership(NULL, %llx): fail, ret %d, %s\n", device_id, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[uunbind]: DonateOwnership(NULL, %llx): done\n", device_id);
                        }
                    } else if (strncmp(p_buffer + 1, "donate", 6) == 0) {
                        if (!user_login) {
                            test_loge("[udonate] fail, not login(user_login %d), should login first\n", user_login);
                            break;
                        }

                        TChar *p_target = NULL;
                        TChar *p_device = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[udonate]: target name, cmd should be: 'udonate target_user deviceid'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_target = p_del;

                        p_del = strchr(p_target, ' ');
                        if (!p_del) {
                            test_loge("[udonate]: no device id, cmd should be: 'udonate target_user deviceid'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_device = p_del;

                        TUniqueID device_id = DInvalidUniqueID;
                        sscanf(p_device, "%llx", &device_id);

                        err = p_agent->DonateOwnership(p_target, device_id);
                        if (EECode_OK != err) {
                            test_loge("[udonate]: DonateOwnership(%s, %llx): fail, ret %d, %s\n", p_target, device_id, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[udonate]: DonateOwnership(%s, %llx): done\n", p_target, device_id);
                        }
                    } else if (strncmp(p_buffer + 1, "changedevicenickname", 20) == 0) {
                        if (!user_login) {
                            test_loge("[uchangedevicenickname] fail, not login(user_login %d), should login first\n", user_login);
                            break;
                        }

                        TChar *nickname = NULL;
                        TChar *p_device = NULL;

                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[uchangedevicenickname]: no device id, cmd should be: 'uchangedevicenickname deviceid nickname'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_device = p_del;

                        p_del = strchr(p_device, ' ');
                        if (!p_del) {
                            test_loge("[uchangedevicenickname]: no nick name, cmd should be: 'uchangedevicenickname deviceid nickname'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        nickname = p_del;

                        TUniqueID device_id = DInvalidUniqueID;
                        sscanf(p_device, "%llx", &device_id);

                        err = p_agent->UpdateDeviceNickname(device_id, nickname);
                        if (EECode_OK != err) {
                            test_loge("[uchangedevicenickname]: UpdateDeviceNickname(%llx %s): fail, ret %d, %s\n", device_id, nickname, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[uchangedevicenickname]: UpdateDeviceNickname(%llx, %s): done\n", device_id, nickname);
                        }
                    } else if (strncmp(p_buffer + 1, "setsecurityqa", 13) == 0) {
                        if (!user_login) {
                            test_loge("[usetsecurityqa] fail, not login(device_login %d), should login first\n", user_login);
                            break;
                        }

                        SSecureQuestions questions;

                        TChar *pq1 = NULL;
                        TChar *pq2 = NULL;
                        TChar *pa1 = NULL;
                        TChar *pa2 = NULL;
                        TChar *p_password = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[usetsecurityqa]: no password, cmd should be: 'usetsecurityqa password question1 answer1 question2 answer2' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_password = p_del;

                        p_del = strchr(p_password, ' ');
                        if (!p_del) {
                            test_loge("[usetsecurityqa]: no question1, cmd should be: 'usetsecurityqa password question1 answer1 question2 answer2'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        pq1 = p_del;

                        p_del = strchr(pq1, ' ');
                        if (!p_del) {
                            test_loge("[usetsecurityqa]: no answer1, cmd should be: 'usetsecurityqa password question1 answer1 question2 answer2'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        pa1 = p_del;

                        p_del = strchr(pa1, ' ');
                        if (!p_del) {
                            test_loge("[usetsecurityqa]: no question2, cmd should be: 'usetsecurityqa password question1 answer1 question2 answer2'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        pq2 = p_del;

                        p_del = strchr(pq2, ' ');
                        if (!p_del) {
                            test_loge("[usetsecurityqa]: no answer2, cmd should be: 'usetsecurityqa password question1 answer1 question2 answer2'\n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        pa1 = p_del;

                        if (strlen(pq1) >= DMAX_SECURE_QUESTION_LENGTH) {
                            test_loge("[usetsecurityqa]: q1(%s) too long %ld\n", pq1, (TULong)strlen(pq1));
                            break;
                        }

                        if (strlen(pq2) >= DMAX_SECURE_QUESTION_LENGTH) {
                            test_loge("[usetsecurityqa]: q2(%s) too long %ld\n", pq2, (TULong)strlen(pq2));
                            break;
                        }

                        if (strlen(pa1) >= DMAX_SECURE_ANSWER_LENGTH) {
                            test_loge("[usetsecurityqa]: a1(%s) too long %ld\n", pa1, (TULong)strlen(pa1));
                            break;
                        }

                        if (strlen(pa2) >= DMAX_SECURE_ANSWER_LENGTH) {
                            test_loge("[usetsecurityqa]: a2(%s) too long %ld\n", pa2, (TULong)strlen(pa2));
                            break;
                        }

                        if (strlen(p_password) >= DIdentificationStringLength) {
                            test_loge("[usetsecurityqa]: password(%s) too long %ld\n", p_password, (TULong)strlen(p_password));
                            break;
                        }

                        memset(&questions, 0x0, sizeof(SSecureQuestions));
                        strcpy(questions.question1, pq1);
                        strcpy(questions.question2, pq2);
                        strcpy(questions.answer1, pa1);
                        strcpy(questions.answer2, pa2);

                        err = p_agent->UpdateSecurityQuestion(&questions, p_password);
                        if (EECode_OK != err) {
                            test_loge("[usetsecurityqa]: fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[usetsecurityqa]: done\n");
                        }
                    }
                }
                break;

                //device
            case 'd': {
                    if (strncmp(p_buffer + 1, "login", 5) == 0) {
                        TChar *p_account = NULL;
                        TChar *p_password = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[dlogin]: no device id, cmd should be: 'ulogin device_id passeord' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_account = p_del;

                        p_del = strchr(p_account, ' ');
                        if (!p_del) {
                            test_loge("[dlogin]: no password, cmd should be: 'ulogin device_id passeord' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_password = p_del;

                        TChar *dataserver_url;
                        TSocketPort dataserver_port;
                        TU64 dynamiccode_input;
                        TUniqueID device_id = DInvalidUniqueID;
                        sscanf(p_account, "%llx", &device_id);
                        err = p_agent->Login(device_id, p_password, p_context->server_url, p_context->server_port, dataserver_url, dataserver_port, dynamiccode_input);
                        if (EECode_OK == err) {
                            test_log("[dlogin]: account: %llx, password: %s done\n", device_id, p_password);
                            device_login = 1;
                        } else if (EECode_OK_NeedSetOwner == err) {
                            test_log("[dlogin]: account: %llx, password: %s done, need set owner\n", device_id, p_password);
                            device_login = 1;
                        } else {
                            test_loge("[dlogin]: account: %llx, password: %s fail, ret %d, %s\n", device_id, p_password, err, gfGetErrorCodeString(err));
                            break;
                        }
                        break;
                    } else if (strncmp(p_buffer + 1, "logout", 6) == 0) {
                        if (!device_login) {
                            test_loge("[dlogout]: not login yet\n");
                            break;
                        }
                        err = p_agent->Logout();
                        if (EECode_OK != err) {
                            test_loge("[dlogout] fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                        } else {
                            test_log("[dlogout]: done\n");
                        }
                        device_login = 0;
                    } else if (strncmp(p_buffer + 1, "setowner", 8) == 0) {
                        if (!device_login) {
                            test_loge("[dsetowner] fail, not login(device_login %d), should login first\n", device_login);
                            break;
                        }
                        TChar *p_account = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[dsetowner]: no username, cmd should be: 'dsetowner account_name' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_account = p_del;

                        err = p_agent->SetOwner(p_account);
                        if (EECode_OK != err) {
                            test_loge("[dsetowner]: account: %s, fail, ret %d, %s\n", p_account, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[dsetowner]: account: %s, done\n", p_account);
                        }
                    }
                }
                break;

                //relay message
            case 'm': {
                    if (strncmp(p_buffer + 1, "post", 4) == 0) {
                        TChar *p_account = NULL;
                        TChar *p_message = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[mpost]: no account, cmd should be: 'mpost device_id message' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_account = p_del;

                        p_del = strchr(p_account, ' ');
                        if (!p_del) {
                            test_loge("[mpost]: no message, cmd should be: 'mpost device_id message' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_message = p_del;
                        TU32 message_length = strlen(p_message);//hard code here

                        TUniqueID target_id = DInvalidUniqueID;
                        sscanf(p_account, "%llx", & target_id);

                        err = p_agent->PostMessage(target_id, (TU8 *)p_message, message_length, ESACPCmdSubType_UserCustomizedDefaultTLV8);
                        if (EECode_OK != err) {
                            test_loge("[mpost]: account: %llx, message %s fail, ret %d, %s\n", target_id, p_message, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_log("[mpost]: account: %llx, message %s done\n", target_id, p_message);
                        }

                    } else if (strncmp(p_buffer + 1, "tlv8zoom", 8) == 0) {
                        TChar *p_account = NULL;
                        TChar *p_zoom_factor = NULL;
                        TChar *p_zoom_offset_x = NULL;
                        TChar *p_zoom_offset_y = NULL;
                        TU8 *p_message = (TU8 *)malloc(DUPDefaultCameraZoomV2CmdLength);
                        do {
                            p_del = strchr(p_buffer, ' ');
                            if (!p_del) {
                                test_loge("[mtlv8zoom]: no account, cmd should be: 'mtlv8zoom device_id zoom_factor zoom_offset_x zoom_offset_y' \n");
                                break;
                            }
                            p_del[0] = 0x0;
                            p_del ++;
                            p_account = p_del;

                            p_del = strchr(p_account, ' ');
                            if (!p_del) {
                                test_loge("[mtlv8zoom]: no zoom_factor, cmd should be: 'mtlv8zoom device_id zoom_factor zoom_offset_x zoom_offset_y' \n");
                                break;
                            }
                            p_del[0] = 0x0;
                            p_del ++;
                            p_zoom_factor = p_del;

                            p_del = strchr(p_zoom_factor, ' ');
                            if (!p_del) {
                                test_loge("[mtlv8zoom]: no zoom_offset_x, cmd should be: 'mtlv8zoom device_id zoom_factor zoom_offset_x zoom_offset_y' \n");
                                break;
                            }
                            p_del[0] = 0x0;
                            p_del ++;
                            p_zoom_offset_x = p_del;

                            p_del = strchr(p_zoom_offset_x, ' ');
                            if (!p_del) {
                                test_loge("[mtlv8zoom]: no zoom_offset_y, cmd should be: 'mtlv8zoom device_id zoom_factor zoom_offset_x zoom_offset_y' \n");
                                break;
                            }
                            p_del[0] = 0x0;
                            p_del ++;
                            p_zoom_offset_y = p_del;

                            TUniqueID target_id = DInvalidUniqueID;
                            TU32 zoom_factor = 0, zoom_offset_x = 0, zoom_offset_y = 0;
                            sscanf(p_account, "%llx", & target_id);
                            sscanf(p_zoom_factor, "%u", & zoom_factor);
                            sscanf(p_zoom_offset_x, "%u", & zoom_offset_x);
                            sscanf(p_zoom_offset_y, "%u", & zoom_offset_y);
                            if (zoom_factor > 10 || ((0 != zoom_factor) && (zoom_offset_x > 10 || zoom_offset_y > 10))) {
                                test_loge("[mtlv8zoom]: zoom_factor=%u(should be [0, 10]), zoom_offset_x=%u(should be [0, 10]), zoom_offset_y=%u(should be [0, 10]) invalid, please try again.\n", zoom_factor, zoom_offset_x, zoom_offset_y);
                                break;
                            }
                            if (DUPDefaultCameraZoomV2CmdLength != gfUPDefaultCamera_BuildZoomV2Cmd(p_message, DUPDefaultCameraZoomV2CmdLength, zoom_factor, zoom_offset_x, zoom_offset_y)) {
                                test_loge("[mtlv8zoom]: gfUPDefaultCamera_BuildZoomV2Cmd failed\n");
                                break;
                            }

                            err = p_agent->PostMessage(target_id, (TU8 *)p_message, DUPDefaultCameraZoomV2CmdLength, ESACPCmdSubType_UserCustomizedDefaultTLV8);
                            if (EECode_OK != err) {
                                test_loge("[mtlv8zoom]: account: %llx, ZoomV2Cmd fail, ret %d, %s\n", target_id, err, gfGetErrorCodeString(err));
                                break;
                            } else {
                                test_log("[mtlv8zoom]: account: %llx, ZoomV2Cmd done\n", target_id);
                            }
                        } while (0);
                        if (p_message) {
                            free(p_message);
                            p_message = NULL;
                        }
                    } else if (strncmp(p_buffer + 1, "tlv8br", 6) == 0) {
                        TChar *p_account = NULL;
                        TChar *p_bitrate = NULL;
                        TU8 *p_message = (TU8 *)malloc(DUPDefaultCameraBitrateCmdLength);
                        do {
                            p_del = strchr(p_buffer, ' ');
                            if (!p_del) {
                                test_loge("[mtlv8br]: no account, cmd should be: 'mtlv8br device_id bitrate' \n");
                                break;
                            }
                            p_del[0] = 0x0;
                            p_del ++;
                            p_account = p_del;

                            p_del = strchr(p_account, ' ');
                            if (!p_del) {
                                test_loge("[mtlv8br]: no bitrate, cmd should be: 'mtlv8br device_id bitrate' \n");
                                break;
                            }
                            p_del[0] = 0x0;
                            p_del ++;
                            p_bitrate = p_del;

                            TUniqueID target_id = DInvalidUniqueID;
                            TU32 bitrateKbps = 0;
                            sscanf(p_account, "%llx", & target_id);
                            sscanf(p_bitrate, "%u", & bitrateKbps);

                            if (DUPDefaultCameraBitrateCmdLength != gfUPDefaultCamera_BuildBitrateCmd(p_message, DUPDefaultCameraBitrateCmdLength, bitrateKbps)) {
                                test_loge("[mtlv8br]: gfUPDefaultCamera_BuildBitrateCmd failed\n");
                                break;
                            }

                            err = p_agent->PostMessage(target_id, (TU8 *)p_message, DUPDefaultCameraBitrateCmdLength, ESACPCmdSubType_UserCustomizedDefaultTLV8);
                            if (EECode_OK != err) {
                                test_loge("[mtlv8br]: account: %llx, BitrateCmd fail, ret %d, %s\n", target_id, err, gfGetErrorCodeString(err));
                                break;
                            } else {
                                test_log("[mtlv8br]: account: %llx, BitrateCmd done\n", target_id);
                            }
                        } while (0);
                        if (p_message) {
                            free(p_message);
                            p_message = NULL;
                        }
                    } else if (strncmp(p_buffer + 1, "tlv8fr", 6) == 0) {
                        TChar *p_account = NULL;
                        TChar *p_framerate = NULL;
                        TU8 *p_message = (TU8 *)malloc(DUPDefaultCameraFramerateCmdLength);
                        do {
                            p_del = strchr(p_buffer, ' ');
                            if (!p_del) {
                                test_loge("[tlv8fr]: no account, cmd should be: 'tlv8fr device_id framerate' \n");
                                break;
                            }
                            p_del[0] = 0x0;
                            p_del ++;
                            p_account = p_del;

                            p_del = strchr(p_account, ' ');
                            if (!p_del) {
                                test_loge("[tlv8fr]: no bitrate, cmd should be: 'tlv8fr device_id framerate' \n");
                                break;
                            }
                            p_del[0] = 0x0;
                            p_del ++;
                            p_framerate = p_del;

                            TUniqueID target_id = DInvalidUniqueID;
                            TU32 framerate = 0;
                            sscanf(p_account, "%llx", & target_id);
                            sscanf(p_framerate, "%u", & framerate);

                            if (DUPDefaultCameraFramerateCmdLength != gfUPDefaultCamera_BuildFramerateCmd(p_message, DUPDefaultCameraFramerateCmdLength, framerate)) {
                                test_loge("[tlv8fr]: gfUPDefaultCamera_BuildFramerateCmd failed\n");
                                break;
                            }

                            err = p_agent->PostMessage(target_id, (TU8 *)p_message, DUPDefaultCameraFramerateCmdLength, ESACPCmdSubType_UserCustomizedDefaultTLV8);
                            if (EECode_OK != err) {
                                test_loge("[tlv8fr]: account: %llx, FramerateCmd fail, ret %d, %s\n", target_id, err, gfGetErrorCodeString(err));
                                break;
                            } else {
                                test_log("[tlv8fr]: account: %llx, FramerateCmd done\n", target_id);
                            }
                        } while (0);
                        if (p_message) {
                            free(p_message);
                            p_message = NULL;
                        }
                    }
                }
                break;

                //security
            case 's': {
                    if (strncmp(p_buffer + 1, "showsecurityquestion", 20) == 0) {
                        TChar *p_account = NULL;
                        SSecureQuestions questions;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[sshowsecurityquestion]: no account, cmd should be: 'sshowsecurityquestion account_name' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_account = p_del;

                        err = p_agent->GetSecurityQuestion(p_account, &questions, p_context->server_url, p_context->server_port);
                        if (EECode_OK != err) {
                            test_loge("[sshowsecurityquestion]: account: %s, fail, ret %d, %s\n", p_account, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_loge("[sshowsecurityquestion]: account: %s, fail, q1: %s, q2: %s\n", p_account, questions.question1, questions.question2);
                        }

                    } else if (strncmp(p_buffer + 1, "resetpassword", 13) == 0) {
                        TChar *p_account = NULL;
                        SSecureQuestions questions;

                        TChar *pq1 = NULL;
                        TChar *pq2 = NULL;
                        TChar *p_new_password = NULL;
                        p_del = strchr(p_buffer, ' ');
                        if (!p_del) {
                            test_loge("[sresetpassword]: no account, cmd should be: 'sresetpassword account_name new_password answer1 answer2' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_account = p_del;

                        p_del = strchr(p_account, ' ');
                        if (!p_del) {
                            test_loge("[sresetpassword]: no new password, cmd should be: 'sresetpassword account_name new_password answer1 answer2' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        p_new_password = p_del;

                        p_del = strchr(p_new_password, ' ');
                        if (!p_del) {
                            test_loge("[sresetpassword]: no answer1, cmd should be: 'sresetpassword account_name new_password answer1 answer2' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        pq1 = p_del;

                        p_del = strchr(pq1, ' ');
                        if (!p_del) {
                            test_loge("[sresetpassword]: no answer2, cmd should be: 'sresetpassword account_name new_password answer1 answer2' \n");
                            break;
                        }
                        p_del[0] = 0x0;
                        p_del ++;
                        pq2 = p_del;

                        if (strlen(p_account) >= DMAX_ACCOUNT_NAME_LENGTH_EX) {
                            test_loge("[sresetpassword]: account(%s) too long %ld\n", p_account, (TULong)strlen(p_account));
                            break;
                        }

                        if (strlen(pq1) >= DMAX_SECURE_ANSWER_LENGTH) {
                            test_loge("[sresetpassword]: q1(%s) too long %ld\n", pq1, (TULong)strlen(pq1));
                            break;
                        }

                        if (strlen(pq2) >= DMAX_SECURE_ANSWER_LENGTH) {
                            test_loge("[sresetpassword]: q2(%s) too long %ld\n", pq2, (TULong)strlen(pq2));
                            break;
                        }

                        if (strlen(p_new_password) >= DIdentificationStringLength) {
                            test_loge("[sresetpassword]: password(%s) too long %ld\n", p_new_password, (TULong)strlen(p_new_password));
                            break;
                        }

                        memset(&questions, 0x0, sizeof(SSecureQuestions));
                        strcpy(questions.answer1, pq1);
                        strcpy(questions.answer2, pq2);

                        TUniqueID ret_id = DInvalidUniqueID;
                        err = p_agent->ResetPasswordBySecurityAnswer(p_account, &questions, p_new_password, ret_id, p_context->server_url, p_context->server_port);
                        if (EECode_OK != err) {
                            test_loge("[sresetpassword]: account: %s, fail, ret %d, %s\n", p_account, err, gfGetErrorCodeString(err));
                            break;
                        } else {
                            test_loge("[sresetpassword]: account: %s, fail, q1: %s, q2: %s\n", p_account, questions.question1, questions.question2);
                        }

                    }
                }
                break;

            case 'h':   // help
                _imclienttest_print_version();
                _imclienttest_print_usage();
                _imclienttest_print_runtime_usage();
                break;

            default:
                test_loge("not-processed cmd [%s]\n", p_buffer);
                break;
        }
    }

    p_agent->Logout();
    p_agent->Destroy();

    return 0;
}


static void __imclient_test_sig_pipe(int a)
{
    test_logw("sig pipe\n");
}

static void _imclienttest_default_initial_setting(SIMClientTestContxt *p_content)
{
    memset(p_content, 0x0, sizeof(SIMClientTestContxt));

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

    imclient_test(&content);

    test_log("[ut flow]: test end\n");

imclienttest_main_exit:

    return 0;
}


