#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"
#include "common_io.h"

#include "cloud_lib_if.h"
#include "sacp_types.h"
#include "lw_database_if.h"
#include "im_mw_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_BEGIN
DCODE_DELIMITER;

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

TInt account_test()
{
    EECode err = EECode_OK;
    SPersistCommonConfig config;
    //init config
    config.database_config.level1_higher_bit = 63;
    config.database_config.level1_lower_bit = 50;

    config.database_config.level2_higher_bit = 49;
    config.database_config.level2_lower_bit = 41;

    config.database_config.host_tag_higher_bit = 31;
    config.database_config.host_tag_lower_bit = 28;

    IAccountManager *p_account_manager = gfCreateAccountManager(EAccountManagerType_generic, (const volatile SPersistCommonConfig *)&config, NULL, 0);
    if (DUnlikely(!p_account_manager)) {
        LOG_ERROR("gfCreateAccountManager() fail\n");
        return (-3);
    }

    err = p_account_manager->LoadFromDataBase((TChar *)"./database/sourcedevice_account.db", (TChar *)"./database/sourcedevice_account_ext.db", (TChar *)"./database/user_account.db", (TChar *)"./database/user_account_ext.db", (TChar *)"./database/datanode_account.db", (TChar *)"./database/datanode_account_ext.db", (TChar *)"./database/datanode_list.db", (TChar *)"./database/controlnode_account.db", (TChar *)"./database/controlnode_account_ext.db");
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("LoadFromDataBase fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;
    TU8 running = 1;

    //todo
    TUniqueID id[20] = {0};
    TU8 type[20] = {0};
    TU32 idnum = 0;

    while (running) {
        usleep(500000);
        memset(buffer, 0x0, 128);
        if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
            TInt errcode = errno;
            if (errcode == EAGAIN) {
                continue;
            }
            test_log("read STDIN_FILENO fail, errno %d\n", errcode);
            continue;
        }

        TUint length = strlen(buffer);
        if (buffer[length - 1] == '\n') {
            buffer[length - 1] = '\0';
        }

        test_log("[user cmd debug]: read input '%s'\n", buffer);

        switch (buffer[0]) {

            case 'q':
                test_log("[user cmd]: Quit!\n");
                running = 0;
                break;

            case 'a': {
                    EAccountGroup group;
                    if (strncmp(buffer, "adduser", 7) == 0) {
                        //add new account
                        group = EAccountGroup_UserAccount;
                        type[idnum] = 0;
                    } else if (strncmp(buffer, "adddevice", 9) == 0) {
                        group = EAccountGroup_SourceDevice;
                        type[idnum] = 1;
                    }

                    TChar *p_account = NULL;
                    TChar *p_password = NULL;
                    TChar *p_del = strchr(buffer, ' ');
                    if (DUnlikely(!p_del)) {
                        test_loge("[cmd add new account]: usage: addxxx account_name password\n");
                        break;
                    }
                    p_account = p_del + 1;
                    p_del = strchr(p_account, ' ');
                    if (DUnlikely(!p_del)) {
                        test_loge("[cmd add new account]: usage: addxxx account_name password\n");
                        break;
                    }
                    *p_del = '\0';
                    p_password = p_del + 1;

                    test_log("add account: account: %s, password: %s\n", p_account, p_password);
                    err = p_account_manager->NewAccount(p_account, p_password, group, id[idnum], EAccountCompany_Ambarella);
                    if (DUnlikely(EECode_OK != err)) {
                        test_loge("NewAccount faill, name %s, password %s\n", p_account, p_password);
                    } else {
                        test_log("NewAccount done, id: %llu\n", id[idnum]);
                        idnum++;
                    }
                }
                break;

            case 'd': {
                    TChar *p_del = strchr(buffer, ' ');
                    if (DUnlikely(!p_del)) {
                        test_loge("[cmd] delete account: Usage: delebyid id_num or delebyname account_name\n");
                        break;
                    }
                    *p_del = '\0';
                    p_del++;
                    if (strncmp(buffer, "delebyid", 8) == 0) {
                        TU32 num = 0;
                        if (DUnlikely(1 != sscanf(p_del, "%u", &num))) {
                            test_loge("[cmd delete account]: delebyid id_num\n");
                            break;
                        }
                        if (num > idnum) {
                            test_loge("user input num(%u) > idnum(%u)\n", num, idnum);
                            break;
                        }
                        err = p_account_manager->DeleteAccount(id[num]);
                        if (DUnlikely(EECode_OK != err)) {
                            test_loge("[cmd delete account] fail, id num %u\n", num);
                        } else {
                            test_log("[cmd delete account] done! %u\n", num);
                            id[num] = id[idnum - 1];
                            id[idnum - 1] = 0;
                            type[num] = type[idnum - 1];
                            type[idnum - 1] = 0;
                            idnum--;
                        }
                    } else if (strncmp(buffer, "delebyname", 10) == 0) {
                        EAccountGroup group;
                        TChar *p_account = p_del;
                        TChar *p_type = strchr(p_account, ' ');
                        if (DUnlikely(!p_type)) {
                            test_loge("[cmd] delete account: Usage: delebyid id_num or delebyname account_name\n");
                            break;
                        }
                        *p_type = '\0';
                        p_type++;
                        TU32 type = 0;
                        if (DUnlikely(1 != sscanf(p_type, "%u", &type))) {
                            test_loge("[cmd delete account]: delebyname account_name type\n");
                            break;
                        }
                        group = (type == 0) ? EAccountGroup_UserAccount : EAccountGroup_SourceDevice;
                        err = p_account_manager->DeleteAccount(p_account, group);
                        if (DUnlikely(EECode_OK != err)) {
                            test_loge("[cmd delete account] fail, account %s\n", p_account);
                        }
                    }
                }
                break;

            case 'Q': {
                    TChar *p_del = strchr(buffer, ' ');
                    if (DUnlikely(!p_del)) {
                        test_loge("[cmd] query account: Usage: query id_num or queryuser account_name or querydevice account_name\n");
                        break;
                    }
                    *p_del = '\0';
                    p_del++;

                    if (strncmp(buffer, "Querybyid", 9) == 0) {
                        TU32 num = 0;
                        if (DUnlikely(1 != sscanf(p_del, "%u", &num))) {
                            test_loge("[cmd query account]: delebyid id_num\n");
                            break;
                        }
                        if (num > idnum) {
                            test_loge("user input num(%u) > idnum(%u)\n", num, idnum);
                            break;
                        }
                        SAccountInfoRoot *p_info;
                        err = p_account_manager->QueryAccount(id[num], p_info);
                        if (DUnlikely(EECode_OK != err)) {
                            test_loge("QueryAccount fail, id num %u\n", num);
                            break;
                        } else {
                            test_log("Query done!:\n");
                            test_log("account: %s\n", p_info->base.name);
                            test_log("password: %s\n", p_info->base.password);
                            test_log("id: %llu, %llu\n", id[num], p_info->header.id);
                        }
                    } else if (strncmp(buffer, "Queryuser", 9) == 0) {
                        TChar *p_account = p_del;
                        SAccountInfoUser *p_info;

                        err = p_account_manager->QueryUserAccountByName(p_account, p_info);
                        if (DUnlikely(EECode_OK != err)) {
                            test_loge("QueryAccount fail, account name %s\n", p_account);
                            break;
                        } else {
                            test_log("Query done!:\n");
                            test_log("account: %s, %s\n", p_account, p_info->root.base.name);
                            test_log("password: %s\n", p_info->root.base.password);
                        }
                    } else if (strncmp(buffer, "Querydevice", 11) == 0) {
                        TChar *p_account = p_del;
                        SAccountInfoSourceDevice *p_info;

                        err = p_account_manager->QuerySourceDeviceAccountByName(p_account, p_info);
                        if (DUnlikely(EECode_OK != err)) {
                            test_loge("QueryAccount fail, account name %s\n", p_account);
                            break;
                        } else {
                            test_log("Query done!:\n");
                            test_log("account: %s, %s\n", p_account, p_info->root.base.name);
                            test_log("password: %s\n", p_info->root.base.password);
                        }
                    }
                }
                break;

            case 'p':
                test_log("total account: %u\n", idnum);
                for (TInt i = 0; i < idnum; i++) {
                    test_log("account: %d, id: 0x%llx, type: %u\n", i, id[i], type[i]);
                }
                break;

            default:
                break;
        }
    }

    err = p_account_manager->StoreToDataBase((TChar *)"./database/sourcedevice_account.db", (TChar *)"./database/sourcedevice_account_ext.db", (TChar *)"./database/user_account.db", (TChar *)"./database/user_account_ext.db", (TChar *)"./database/datanode_account.db", (TChar *)"./database/datanode_account_ext.db", (TChar *)"./database/datanode_list.db", (TChar *)"./database/controlnode_account.db", (TChar *)"./database/controlnode_account_ext.db");
    if (DUnlikely(EECode_OK != err)) {
        test_loge("StoreToDataBase fail, rer %d, %s\n", err, gfGetErrorCodeString(err));
    }

    return 0;
}

TInt main(TInt argc, TChar **argv)
{
    account_test();
    return 0;
}
