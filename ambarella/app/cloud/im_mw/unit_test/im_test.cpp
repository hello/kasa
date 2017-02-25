/*******************************************************************************
 * im_test.cpp
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
#include "sacp_types.h"

#include "lw_database_if.h"
#include "im_mw_if.h"

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

typedef struct {
    IAccountManager *p_account_manager;

    CICommunicationServerPort *p_admin_server_port;

    IIMServer *p_server;

    TU8 *p_responce_buffer;
    TU32 responce_buffer_size;
} SAdminCallbackContext;

typedef struct {
    TU8 daemon;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;

    TSocketPort server_port;
    TSocketPort admin_port;

    SPersistCommonConfig config;

    SAdminCallbackContext callback_context;
} SIMTestContxt;

typedef struct {
    TChar source[DMAX_ACCOUNT_NAME_LENGTH_EX];
    TChar password[DMAX_ACCOUNT_NAME_LENGTH_EX];
    TChar id_string[DDynamicInputStringLength + 4];
} SAdminPortInfo;

static TInt g_running = 1;
#define user_name_password_len (27)

void _imtest_print_version()
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

void _imtest_print_usage()
{
    printf("\nimtest options:\n");
}

void _imtest_print_runtime_usage()
{
    printf("\nimtest runtime cmds: press cmd + Enter\n");
    printf("\t'q': quit unit test\n");
    printf("\t'pall': will print all components' status\n");
}

static void sacp_fill_header(SSACPHeader *p_header, TU32 type, TU32 size, TU8 ext_type, TU16 ext_size, TU32 seq_count, TU8 flag)
{
    p_header->type_1 = (type >> 24) & 0xff;
    p_header->type_2 = (type >> 16) & 0xff;
    p_header->type_3 = (type >> 8) & 0xff;
    p_header->type_4 = type & 0xff;

    p_header->size_0 = (size >> 16) & 0xff;
    p_header->size_1 = (size >> 8) & 0xff;
    p_header->size_2 = size & 0xff;

    p_header->seq_count_0 = (seq_count >> 16) & 0xff;
    p_header->seq_count_1 = (seq_count >> 8) & 0xff;
    p_header->seq_count_2 = seq_count & 0xff;

    p_header->encryption_type_1 = 0;
    p_header->encryption_type_2 = 0;

    p_header->flags = 0;
    p_header->header_ext_type = ext_type;
    p_header->header_ext_size_1 = (ext_size >> 8) & 0xff;
    p_header->header_ext_size_2 = ext_size & 0xff;
}

static EECode __im_process_admin_callback(void *owner, TChar *source, void *p_source, void *p_port, TU8 *p_data, TU32 datasize, TU32 type, TU32 cat, TU32 subtype)
{
    LOG_PRINTF("datasize [%u]\n", datasize);
    SAdminCallbackContext *context = (SAdminCallbackContext *) owner;
    EECode err = EECode_OK;
    TU32 cur_size = 0;
    TU8 *p_cur = NULL;
    STLV16HeaderBigEndian *p_tlv = NULL;
    TU32 length = 0;

    if (DUnlikely(!owner)) {
        LOG_ERROR("NULL pointer\n");
        return EECode_BadCommand;
    }

    if (DUnlikely(ESACPTypeCategory_AdminChannel != cat)) {
        LOG_ERROR("BAD cmd, type %08x, cat %08x, subtype %08x\n", type, cat, subtype);
        return EECode_BadCommand;
    }

    switch (subtype) {

        case ESACPAdminSubType_QuerySourceViaAccountName:
            LOG_PRINTF("ESACPAdminSubType_QuerySourceViaAccountName start\n");
            if (DLikely(context->p_account_manager)) {
                SAccountInfoSourceDevice *p_info = NULL;
                p_tlv = (STLV16HeaderBigEndian *) p_data;
                TU16 tlvtype = (p_tlv->type_high << 8) | (p_tlv->type_low);
                length = (p_tlv->length_high << 8) | (p_tlv->length_low);

                if ((ETLV16Type_AccountName != tlvtype) || (length >= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
                    LOG_PRINTF("ESACPAdminSubType_QuerySourceViaAccountName bad protocol format\n");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, 0, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader));
                    DASSERT_OK(err);
                    return EECode_BadFormat;
                }

                TChar account_name[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
                memcpy(account_name, p_data + sizeof(STLV16HeaderBigEndian), length);

                err = context->p_account_manager->QuerySourceDeviceAccountByName(account_name, p_info);
                if ((EECode_OK == err) && p_info) {
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    //wirte ID, username, password, id string here
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
                    p_tlv->length_high = 0;
                    p_tlv->length_low = sizeof(TUniqueID);
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);

                    p_cur[0] = (p_info->root.header.id >> 56) & 0xff;
                    p_cur[1] = (p_info->root.header.id >> 48) & 0xff;
                    p_cur[2] = (p_info->root.header.id >> 40) & 0xff;
                    p_cur[3] = (p_info->root.header.id >> 32) & 0xff;
                    p_cur[4] = (p_info->root.header.id >> 24) & 0xff;
                    p_cur[5] = (p_info->root.header.id >> 16) & 0xff;
                    p_cur[6] = (p_info->root.header.id >> 8) & 0xff;
                    p_cur[7] = (p_info->root.header.id) & 0xff;
                    p_cur += sizeof(TUniqueID);
                    cur_size += sizeof(TUniqueID);

                    length = strlen(p_info->root.base.name);
                    DASSERT(length < DMAX_ACCOUNT_NAME_LENGTH_EX);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountName >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountName) & 0xff;
                    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
                    p_tlv->length_low = (length + 1) & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, p_info->root.base.name, length);
                    p_cur += length;
                    *p_cur++ = 0x0;
                    cur_size += length + 1;

                    length = strlen(p_info->root.base.password);
                    DASSERT(length < DMAX_ACCOUNT_NAME_LENGTH);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountPassword >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountPassword) & 0xff;
                    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
                    p_tlv->length_low = (length + 1) & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, p_info->root.base.password, length);
                    p_cur += length;
                    *p_cur++ = 0x0;
                    cur_size += length + 1;

                    length = strlen(p_info->ext.production_code);
                    //LOG_WARN("[debug]: production length %d, p_info %p\n", length, p_info);
                    DASSERT(length < DMAX_PRODUCTION_CODE_LENGTH);
                    //length = DMAX_PRODUCTION_CODE_LENGTH;
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_SourceDeviceProductCode >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_SourceDeviceProductCode) & 0xff;
                    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
                    p_tlv->length_low = (length + 1) & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, p_info->ext.production_code, length);
                    p_cur += length;
                    *p_cur++ = 0x0;
                    cur_size += length + 1;

                    length = sizeof(TUniqueID);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_SourceDeviceOwner >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_SourceDeviceOwner) & 0xff;
                    p_tlv->length_high = (length >> 8) & 0xff;
                    p_tlv->length_low = length & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    p_cur[0] = (p_info->ext.ownerid >> 56) & 0xff;
                    p_cur[1] = (p_info->ext.ownerid >> 48) & 0xff;
                    p_cur[2] = (p_info->ext.ownerid >> 40) & 0xff;
                    p_cur[3] = (p_info->ext.ownerid >> 32) & 0xff;
                    p_cur[4] = (p_info->ext.ownerid >> 24) & 0xff;
                    p_cur[5] = (p_info->ext.ownerid >> 16) & 0xff;
                    p_cur[6] = (p_info->ext.ownerid >> 8) & 0xff;
                    p_cur[7] = (p_info->ext.ownerid) & 0xff;
                    p_cur += sizeof(TUniqueID);
                    cur_size += sizeof(TUniqueID);

                    length = p_info->friendsnum * sizeof(TUniqueID);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_SourceDeviceShareList >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_SourceDeviceShareList) & 0xff;
                    p_tlv->length_high = (length >> 8) & 0xff;
                    p_tlv->length_low = length & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    for (TU32 i = 0; i < p_info->friendsnum; i++) {
                        p_cur[0] = (p_info->p_sharedfriendsid[i] >> 56) & 0xff;
                        p_cur[1] = (p_info->p_sharedfriendsid[i] >> 48) & 0xff;
                        p_cur[2] = (p_info->p_sharedfriendsid[i] >> 40) & 0xff;
                        p_cur[3] = (p_info->p_sharedfriendsid[i] >> 32) & 0xff;
                        p_cur[4] = (p_info->p_sharedfriendsid[i] >> 24) & 0xff;
                        p_cur[5] = (p_info->p_sharedfriendsid[i] >> 16) & 0xff;
                        p_cur[6] = (p_info->p_sharedfriendsid[i] >> 8) & 0xff;
                        p_cur[7] = (p_info->p_sharedfriendsid[i]) & 0xff;
                        p_cur += sizeof(TUniqueID);
                        cur_size += sizeof(TUniqueID);
                    }

                    length = p_info->friendsnum * sizeof(TU32);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_SourceDeviceSharePrivilegeMaskList >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_SourceDeviceSharePrivilegeMaskList) & 0xff;
                    p_tlv->length_high = (length >> 8) & 0xff;
                    p_tlv->length_low = length & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    for (TU32 i = 0; i < p_info->friendsnum; i++) {
                        p_cur[0] = (p_info->p_sharedfriendsid[i] >> 24) & 0xff;
                        p_cur[1] = (p_info->p_sharedfriendsid[i] >> 16) & 0xff;
                        p_cur[2] = (p_info->p_sharedfriendsid[i] >> 8) & 0xff;
                        p_cur[3] = (p_info->p_sharedfriendsid[i]) & 0xff;
                        p_cur += sizeof(TU32);
                        cur_size += sizeof(TU32);
                    }

                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

                    DASSERT((cur_size + sizeof(SSACPHeader)) < DSACP_MAX_PAYLOAD_LENGTH);
                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, cur_size + sizeof(SSACPHeader));
                    DASSERT_OK(err);
                } else {
                    LOG_PRINTF("ESACPAdminSubType_QuerySourceViaAccountName not found, name %s\n", (TChar *)p_data);
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, 0, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_NotExist);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader));
                    DASSERT_OK(err);
                }
            } else {
                LOG_ERROR("NULL context->p_account_manager\n");
                sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, 0, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_ServerLogicalBug);

                err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader));
                DASSERT_OK(err);
            }
            break;

        case ESACPAdminSubType_QuerySourceViaAccountID:
            LOG_PRINTF("ESACPAdminSubType_QuerySourceViaAccountID start\n");
            if (DLikely(context->p_account_manager)) {
                SAccountInfoSourceDevice *p_info = NULL;
                p_tlv = (STLV16HeaderBigEndian *) p_data;
                TU16 tlvtype = (p_tlv->type_high << 8) | (p_tlv->type_low);
                length = (p_tlv->length_high << 8) | (p_tlv->length_low);

                if ((ETLV16Type_AccountID != tlvtype) || (length != sizeof(TUniqueID))) {
                    LOG_PRINTF("ESACPAdminSubType_QuerySourceViaAccountID bad protocol format\n");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, 0, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader));
                    DASSERT_OK(err);
                    return EECode_BadFormat;
                }

                TUniqueID id = 0;
                p_cur = p_data + sizeof(STLV16HeaderBigEndian);
                id = ((TUniqueID)p_cur[0] << 56) | ((TUniqueID)p_cur[1] << 48) | ((TUniqueID)p_cur[2] << 40) | ((TUniqueID)p_cur[3] << 32) | ((TUniqueID)p_cur[4] << 24) | ((TUniqueID)p_cur[5] << 16) | ((TUniqueID)p_cur[6] << 8) | (TUniqueID)p_cur[7];
                SAccountInfoRoot *p_root_info = NULL;

                err = context->p_account_manager->QueryAccount(id, p_root_info);
                p_info = (SAccountInfoSourceDevice *) p_root_info;
                if ((EECode_OK == err) && p_info) {
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    //wirte ID, username, password, id string here
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
                    p_tlv->length_high = 0;
                    p_tlv->length_low = sizeof(TUniqueID);
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);

                    p_cur[0] = (p_info->root.header.id >> 56) & 0xff;
                    p_cur[1] = (p_info->root.header.id >> 48) & 0xff;
                    p_cur[2] = (p_info->root.header.id >> 40) & 0xff;
                    p_cur[3] = (p_info->root.header.id >> 32) & 0xff;
                    p_cur[4] = (p_info->root.header.id >> 24) & 0xff;
                    p_cur[5] = (p_info->root.header.id >> 16) & 0xff;
                    p_cur[6] = (p_info->root.header.id >> 8) & 0xff;
                    p_cur[7] = (p_info->root.header.id) & 0xff;
                    p_cur += sizeof(TUniqueID);
                    cur_size += sizeof(TUniqueID);

                    length = strlen(p_info->root.base.name);
                    DASSERT(length < DMAX_ACCOUNT_NAME_LENGTH_EX);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountName >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountName) & 0xff;
                    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
                    p_tlv->length_low = (length + 1) & 0xff;;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, p_info->root.base.name, length);
                    p_cur += length;
                    *p_cur++ = 0x0;
                    cur_size += length + 1;

                    length = strlen(p_info->root.base.password);
                    DASSERT(length < DMAX_ACCOUNT_NAME_LENGTH);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountPassword >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountPassword) & 0xff;
                    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
                    p_tlv->length_low = (length + 1) & 0xff;;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, p_info->root.base.password, length);
                    p_cur += length;
                    *p_cur++ = 0x0;
                    cur_size += length + 1;

                    length = DMAX_PRODUCTION_CODE_LENGTH;
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_SourceDeviceProductCode >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_SourceDeviceProductCode) & 0xff;
                    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
                    p_tlv->length_low = (length + 1) & 0xff;;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, p_info->ext.production_code, length);
                    p_cur += length;
                    cur_size += length;

                    length = sizeof(TUniqueID);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_SourceDeviceOwner >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_SourceDeviceOwner) & 0xff;
                    p_tlv->length_high = (length >> 8) & 0xff;
                    p_tlv->length_low = length & 0xff;;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    p_cur[0] = (p_info->ext.ownerid >> 56) & 0xff;
                    p_cur[1] = (p_info->ext.ownerid >> 48) & 0xff;
                    p_cur[2] = (p_info->ext.ownerid >> 40) & 0xff;
                    p_cur[3] = (p_info->ext.ownerid >> 32) & 0xff;
                    p_cur[4] = (p_info->ext.ownerid >> 24) & 0xff;
                    p_cur[5] = (p_info->ext.ownerid >> 16) & 0xff;
                    p_cur[6] = (p_info->ext.ownerid >> 8) & 0xff;
                    p_cur[7] = (p_info->ext.ownerid) & 0xff;
                    p_cur += sizeof(TUniqueID);
                    cur_size += sizeof(TUniqueID);

                    length = p_info->friendsnum * sizeof(TUniqueID);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_SourceDeviceShareList >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_SourceDeviceShareList) & 0xff;
                    p_tlv->length_high = (length >> 8) & 0xff;
                    p_tlv->length_low = length & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    for (TU32 i = 0; i < p_info->friendsnum; i++) {
                        p_cur[0] = (p_info->p_sharedfriendsid[i] >> 56) & 0xff;
                        p_cur[1] = (p_info->p_sharedfriendsid[i] >> 48) & 0xff;
                        p_cur[2] = (p_info->p_sharedfriendsid[i] >> 40) & 0xff;
                        p_cur[3] = (p_info->p_sharedfriendsid[i] >> 32) & 0xff;
                        p_cur[4] = (p_info->p_sharedfriendsid[i] >> 24) & 0xff;
                        p_cur[5] = (p_info->p_sharedfriendsid[i] >> 16) & 0xff;
                        p_cur[6] = (p_info->p_sharedfriendsid[i] >> 8) & 0xff;
                        p_cur[7] = (p_info->p_sharedfriendsid[i]) & 0xff;
                        p_cur += sizeof(TUniqueID);
                        cur_size += sizeof(TUniqueID);
                    }

                    length = p_info->friendsnum * sizeof(TU32);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_SourceDeviceSharePrivilegeMaskList >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_SourceDeviceSharePrivilegeMaskList) & 0xff;
                    p_tlv->length_high = (length >> 8) & 0xff;
                    p_tlv->length_low = length & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    for (TU32 i = 0; i < p_info->friendsnum; i++) {
                        p_cur[0] = (p_info->p_sharedfriendsid[i] >> 24) & 0xff;
                        p_cur[1] = (p_info->p_sharedfriendsid[i] >> 16) & 0xff;
                        p_cur[2] = (p_info->p_sharedfriendsid[i] >> 8) & 0xff;
                        p_cur[3] = (p_info->p_sharedfriendsid[i]) & 0xff;
                        p_cur += sizeof(TU32);
                        cur_size += sizeof(TU32);
                    }

                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

                    DASSERT((cur_size + sizeof(SSACPHeader)) < DSACP_MAX_PAYLOAD_LENGTH);
                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, cur_size + sizeof(SSACPHeader));
                    DASSERT_OK(err);
                } else {
                    LOG_PRINTF("ESACPAdminSubType_QuerySourceViaAccountName not found, name %s\n", (TChar *)p_data);
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, 0, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_NotExist);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader));
                    DASSERT_OK(err);
                }
            } else {
                LOG_ERROR("NULL context->p_account_manager\n");
                sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, 0, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_ServerLogicalBug);

                err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader));
                DASSERT_OK(err);
            }
            break;

        case ESACPAdminSubType_QueryDataNodeViaName:
            LOG_PRINTF("ESACPAdminSubType_QueryDataNodeViaName start\n");
            if (DLikely(context->p_account_manager)) {
                SAccountInfoCloudDataNode *p_info = NULL;
                p_tlv = (STLV16HeaderBigEndian *) p_data;
                TU16 tlvtype = (p_tlv->type_high << 8) | (p_tlv->type_low);
                length = (p_tlv->length_high << 8) | (p_tlv->length_low);

                if ((ETLV16Type_AccountName != tlvtype) || (length >= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
                    LOG_PRINTF("ESACPAdminSubType_QueryDataNodeViaName bad protocol format\n");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, 0, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader));
                    DASSERT_OK(err);
                    return EECode_BadFormat;
                }

                TChar account_name[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
                memcpy(account_name, p_data + sizeof(STLV16HeaderBigEndian), length);

                err = context->p_account_manager->QueryDataNodeByName(account_name, p_info);
                if ((EECode_OK == err) && p_info) {
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    //wirte ID, username, password, id string here
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
                    p_tlv->length_high = 0;
                    p_tlv->length_low = sizeof(TUniqueID);
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);

                    p_cur[0] = (p_info->root.header.id >> 56) & 0xff;
                    p_cur[1] = (p_info->root.header.id >> 48) & 0xff;
                    p_cur[2] = (p_info->root.header.id >> 40) & 0xff;
                    p_cur[3] = (p_info->root.header.id >> 32) & 0xff;
                    p_cur[4] = (p_info->root.header.id >> 24) & 0xff;
                    p_cur[5] = (p_info->root.header.id >> 16) & 0xff;
                    p_cur[6] = (p_info->root.header.id >> 8) & 0xff;
                    p_cur[7] = (p_info->root.header.id) & 0xff;
                    p_cur += sizeof(TUniqueID);
                    cur_size += sizeof(TUniqueID);

                    length = strlen(p_info->root.base.name);
                    DASSERT(length < DMAX_ACCOUNT_NAME_LENGTH_EX);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountName >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountName) & 0xff;
                    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
                    p_tlv->length_low = (length + 1) & 0xff;;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, p_info->root.base.name, length);
                    p_cur += length;
                    *p_cur++ = 0x0;
                    cur_size += length + 1;

                    length = strlen(p_info->root.base.password);
                    DASSERT(length < DMAX_ACCOUNT_NAME_LENGTH);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountPassword >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountPassword) & 0xff;
                    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
                    p_tlv->length_low = (length + 1) & 0xff;;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, p_info->root.base.password, length);
                    p_cur += length;
                    *p_cur++ = 0x0;
                    cur_size += length + 1;

                    length = p_info->current_channel_number * sizeof(TUniqueID);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_DataNodeChannelIDList >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_DataNodeChannelIDList) & 0xff;
                    p_tlv->length_high = (length >> 8) & 0xff;
                    p_tlv->length_low = length & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    for (TU32 i = 0; i < p_info->current_channel_number; i++) {
                        p_cur[0] = (p_info->p_channel_id[i] >> 56) & 0xff;
                        p_cur[1] = (p_info->p_channel_id[i] >> 48) & 0xff;
                        p_cur[2] = (p_info->p_channel_id[i] >> 40) & 0xff;
                        p_cur[3] = (p_info->p_channel_id[i] >> 32) & 0xff;
                        p_cur[4] = (p_info->p_channel_id[i] >> 24) & 0xff;
                        p_cur[5] = (p_info->p_channel_id[i] >> 16) & 0xff;
                        p_cur[6] = (p_info->p_channel_id[i] >> 8) & 0xff;
                        p_cur[7] = (p_info->p_channel_id[i]) & 0xff;
                        p_cur += sizeof(TUniqueID);
                        cur_size += sizeof(TUniqueID);
                    }

                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

                    DASSERT((cur_size + sizeof(SSACPHeader)) < DSACP_MAX_PAYLOAD_LENGTH);
                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, cur_size + sizeof(SSACPHeader));
                    DASSERT_OK(err);
                } else {
                    LOG_PRINTF("ESACPAdminSubType_QuerySourceViaAccountName not found, name %s\n", (TChar *)p_data);
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, 0, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_NotExist);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader));
                    DASSERT_OK(err);
                }
            } else {
                LOG_ERROR("NULL context->p_account_manager\n");
                sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, 0, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_ServerLogicalBug);

                err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader));
                DASSERT_OK(err);
            }
            break;

        case ESACPAdminSubType_QueryDataNodeViaID:
            LOG_PRINTF("ESACPAdminSubType_QueryDataNodeViaID start\n");
            if (DLikely(context->p_account_manager)) {
                SAccountInfoCloudDataNode *p_info = NULL;
                p_tlv = (STLV16HeaderBigEndian *) p_data;
                TU16 tlvtype = (p_tlv->type_high << 8) | (p_tlv->type_low);
                length = (p_tlv->length_high << 8) | (p_tlv->length_low);

                if ((ETLV16Type_AccountID != tlvtype) || (length != sizeof(TUniqueID))) {
                    LOG_PRINTF("ESACPAdminSubType_QueryDataNodeViaID bad protocol format\n");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, 0, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader));
                    DASSERT_OK(err);
                    return EECode_BadFormat;
                }

                TUniqueID id = 0;
                p_cur = p_data + sizeof(STLV16HeaderBigEndian);
                id = ((TUniqueID)p_cur[0] << 56) | ((TUniqueID)p_cur[1] << 48) | ((TUniqueID)p_cur[2] << 40) | ((TUniqueID)p_cur[3] << 32) | ((TUniqueID)p_cur[4] << 24) | ((TUniqueID)p_cur[5] << 16) | ((TUniqueID)p_cur[6] << 8) | (TUniqueID)p_cur[7];
                SAccountInfoRoot *p_root_info = NULL;

                err = context->p_account_manager->QueryAccount(id, p_root_info);
                p_info = (SAccountInfoCloudDataNode *) p_root_info;
                if ((EECode_OK == err) && p_info) {
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    //wirte ID, username, password, id string here
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
                    p_tlv->length_high = 0;
                    p_tlv->length_low = sizeof(TUniqueID);
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);

                    p_cur[0] = (p_info->root.header.id >> 56) & 0xff;
                    p_cur[1] = (p_info->root.header.id >> 48) & 0xff;
                    p_cur[2] = (p_info->root.header.id >> 40) & 0xff;
                    p_cur[3] = (p_info->root.header.id >> 32) & 0xff;
                    p_cur[4] = (p_info->root.header.id >> 24) & 0xff;
                    p_cur[5] = (p_info->root.header.id >> 16) & 0xff;
                    p_cur[6] = (p_info->root.header.id >> 8) & 0xff;
                    p_cur[7] = (p_info->root.header.id) & 0xff;
                    p_cur += sizeof(TUniqueID);
                    cur_size += sizeof(TUniqueID);

                    length = strlen(p_info->root.base.name);
                    DASSERT(length < DMAX_ACCOUNT_NAME_LENGTH_EX);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountName >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountName) & 0xff;
                    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
                    p_tlv->length_low = (length + 1) & 0xff;;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, p_info->root.base.name, length);
                    p_cur += length;
                    *p_cur++ = 0x0;
                    cur_size += length + 1;

                    length = strlen(p_info->root.base.password);
                    DASSERT(length < DMAX_ACCOUNT_NAME_LENGTH);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountPassword >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountPassword) & 0xff;
                    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
                    p_tlv->length_low = (length + 1) & 0xff;;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, p_info->root.base.password, length);
                    p_cur += length;
                    *p_cur++ = 0x0;
                    cur_size += length + 1;

                    length = p_info->current_channel_number * sizeof(TUniqueID);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_DataNodeChannelIDList >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_DataNodeChannelIDList) & 0xff;
                    p_tlv->length_high = (length >> 8) & 0xff;
                    p_tlv->length_low = length & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    for (TU32 i = 0; i < p_info->current_channel_number; i++) {
                        p_cur[0] = (p_info->p_channel_id[i] >> 56) & 0xff;
                        p_cur[1] = (p_info->p_channel_id[i] >> 48) & 0xff;
                        p_cur[2] = (p_info->p_channel_id[i] >> 40) & 0xff;
                        p_cur[3] = (p_info->p_channel_id[i] >> 32) & 0xff;
                        p_cur[4] = (p_info->p_channel_id[i] >> 24) & 0xff;
                        p_cur[5] = (p_info->p_channel_id[i] >> 16) & 0xff;
                        p_cur[6] = (p_info->p_channel_id[i] >> 8) & 0xff;
                        p_cur[7] = (p_info->p_channel_id[i]) & 0xff;
                        p_cur += sizeof(TUniqueID);
                        cur_size += sizeof(TUniqueID);
                    }

                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

                    DASSERT((cur_size + sizeof(SSACPHeader)) < DSACP_MAX_PAYLOAD_LENGTH);
                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, cur_size + sizeof(SSACPHeader));
                    DASSERT_OK(err);
                } else {
                    LOG_PRINTF("ESACPAdminSubType_QuerySourceViaAccountName not found, name %s\n", (TChar *)p_data);
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, 0, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_NotExist);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader));
                    DASSERT_OK(err);
                }
            } else {
                LOG_ERROR("NULL context->p_account_manager\n");
                sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, 0, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_ServerLogicalBug);

                err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader));
                DASSERT_OK(err);
            }
            break;

        case ESACPAdminSubType_NewSourceAccount:
            LOG_PRINTF("ESACPAdminSubType_NewSourceAccount start\n");
            if (DLikely(context->p_account_manager)) {
                SAccountInfoSourceDevice *p_info = NULL;
                TU16 tlv_type = 0;
                TU16 tlv_len = 0;
                TChar account_name[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
                TChar account_password[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
                //TChar account_idstring[DMAX_ID_AUTHENTICATION_STRING_LENGTH] = {0};
                TChar account_production_code[DMAX_PRODUCTION_CODE_LENGTH] = {0};

                p_cur = (TU8 *) p_data;
                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);
                if (DUnlikely(ETLV16Type_AccountName != tlv_type)) {
                    LOG_ERROR("No Username\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("No Username");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "No Username");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                } else {
                    if (DUnlikely(tlv_len >= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
                        LOG_ERROR("Username too long\n");
                        p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                        p_tlv = (STLV16HeaderBigEndian *) p_cur;
                        cur_size = strlen("Username too long");
                        p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                        p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                        p_tlv->length_high = (cur_size >> 8) & 0xff;
                        p_tlv->length_low = cur_size & 0xff;
                        p_cur += sizeof(STLV16HeaderBigEndian);
                        cur_size += sizeof(STLV16HeaderBigEndian);
                        strcpy((TChar *)p_cur, "Username too long");

                        sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                        err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                        DASSERT_OK(err);
                        break;
                    }
                    memcpy(account_name, p_cur, tlv_len);
                    LOG_PRINTF("[account_name: %s], tlv_len %d\n", account_name, tlv_len);
                    p_cur += tlv_len;
                }

                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);
                if (DUnlikely(ETLV16Type_AccountPassword != tlv_type)) {
                    LOG_ERROR("No Password\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("No Password");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "No Password");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                } else {
                    if (DUnlikely(tlv_len >= DMAX_ACCOUNT_NAME_LENGTH)) {
                        LOG_ERROR("Password too long\n");
                        p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                        p_tlv = (STLV16HeaderBigEndian *) p_cur;
                        cur_size = strlen("Password too long");
                        p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                        p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                        p_tlv->length_high = (cur_size >> 8) & 0xff;
                        p_tlv->length_low = cur_size & 0xff;
                        p_cur += sizeof(STLV16HeaderBigEndian);
                        cur_size += sizeof(STLV16HeaderBigEndian);
                        strcpy((TChar *)p_cur, "Password too long");
                        sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                        err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                        DASSERT_OK(err);
                        break;
                    }
                    memcpy(account_password, p_cur, tlv_len);
                    LOG_PRINTF("[account_password: %s], tlv_len %d\n", account_password, tlv_len);
                    p_cur += tlv_len;
                }

#if 0
                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);
                if (DUnlikely(ETLV16Type_AccountIDString != tlv_type)) {
                    LOG_ERROR("No ID String\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("No ID String");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "No ID String");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                } else {
                    if (DUnlikely(tlv_len >= DMAX_ID_AUTHENTICATION_STRING_LENGTH)) {
                        LOG_ERROR("ID String too long\n");
                        p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                        p_tlv = (STLV16HeaderBigEndian *) p_cur;
                        cur_size = strlen("ID String too long");
                        p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                        p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                        p_tlv->length_high = (cur_size >> 8) & 0xff;
                        p_tlv->length_low = cur_size & 0xff;
                        p_cur += sizeof(STLV16HeaderBigEndian);
                        cur_size += sizeof(STLV16HeaderBigEndian);
                        strcpy((TChar *)p_cur, "ID String too long");
                        sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                        err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                        DASSERT_OK(err);
                        break;
                    }
                    memcpy(account_idstring, p_cur, tlv_len);
                    LOG_PRINTF("[account_idstring: %s], tlv_len %d\n", account_idstring, tlv_len);
                    p_cur += tlv_len;
                }
#endif

                TUniqueID newid = 0x0;
                cur_size = 0;

                err = context->p_account_manager->NewAccount(account_name, account_password, EAccountGroup_SourceDevice, newid, EAccountCompany_Ambarella, account_production_code, DMAX_PRODUCTION_CODE_LENGTH);
                if (EECode_OK == err) {
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    //wirte ID
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
                    p_tlv->length_high = 0;
                    p_tlv->length_low = sizeof(TUniqueID);
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);

                    LOG_PRINTF("id %llx\n", newid);
                    p_cur[0] = (newid >> 56) & 0xff;
                    p_cur[1] = (newid >> 48) & 0xff;
                    p_cur[2] = (newid >> 40) & 0xff;
                    p_cur[3] = (newid >> 32) & 0xff;
                    p_cur[4] = (newid >> 24) & 0xff;
                    p_cur[5] = (newid >> 16) & 0xff;
                    p_cur[6] = (newid >> 8) & 0xff;
                    p_cur[7] = (newid) & 0xff;
                    LOG_PRINTF("%02x %02x %02x %02x %02x %02x %02x %02x\n", p_cur[0], p_cur[1], p_cur[2], p_cur[3], p_cur[4], p_cur[5], p_cur[6], p_cur[7]);
                    p_cur += sizeof(TUniqueID);
                    cur_size += sizeof(TUniqueID);

                    length = DMAX_PRODUCTION_CODE_LENGTH;
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_SourceDeviceProductCode >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_SourceDeviceProductCode) & 0xff;
                    p_tlv->length_high = ((length) >> 8) & 0xff;
                    p_tlv->length_low = (length) & 0xff;;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    //err = context->p_account_manager->QuerySourceDeviceAccountByName(account_name, p_info);
                    //DASSERT(p_info);
                    //DASSERT(EECode_OK == err);
                    //SAccountInfoRoot* p_root = NULL;
                    //err = context->p_account_manager->QueryAccount(newid, p_root);
                    //p_info = (SAccountInfoSourceDevice *)p_root;
                    //memcpy(p_cur, p_info->ext.production_code, length);
                    memcpy(p_cur, account_production_code, length);
                    LOG_NOTICE("production code %s\n", account_production_code);
                    p_cur += length;
                    cur_size += length;

                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

                    DASSERT((cur_size + sizeof(SSACPHeader)) < DSACP_MAX_PAYLOAD_LENGTH);
                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, cur_size + sizeof(SSACPHeader));
                    LOG_NOTICE("reply size %zu\n", cur_size + sizeof(SSACPHeader));
                    DASSERT_OK(err);
                } else {
                    LOG_ERROR("new fail\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("new fail");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "new fail");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                }
            } else {
                LOG_ERROR("NULL context->p_account_manager\n");
                p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                cur_size = strlen("NULL p_account_manager");
                p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                p_tlv->length_high = (cur_size >> 8) & 0xff;
                p_tlv->length_low = cur_size & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                strcpy((TChar *)p_cur, "NULL p_account_manager");
                sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_ServerLogicalBug);

                err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                DASSERT_OK(err);
                break;
            }
            break;

        case ESACPAdminSubType_NewSinkAccount:
            LOG_PRINTF("ESACPAdminSubType_NewSinkAccount[debug only] start\n");
            if (DLikely(context->p_account_manager)) {
                SAccountInfoUser *p_info = NULL;
                TU16 tlv_type = 0;
                TU16 tlv_len = 0;
                TChar account_name[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
                TChar account_password[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
                //TChar account_idstring[DMAX_ID_AUTHENTICATION_STRING_LENGTH] = {0};

                p_cur = (TU8 *) p_data;
                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);
                if (DUnlikely(ETLV16Type_AccountName != tlv_type)) {
                    LOG_ERROR("No Username\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("No Username");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "No Username");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                } else {
                    if (DUnlikely(tlv_len >= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
                        LOG_ERROR("Username too long\n");
                        p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                        p_tlv = (STLV16HeaderBigEndian *) p_cur;
                        cur_size = strlen("Username too long");
                        p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                        p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                        p_tlv->length_high = (cur_size >> 8) & 0xff;
                        p_tlv->length_low = cur_size & 0xff;
                        p_cur += sizeof(STLV16HeaderBigEndian);
                        cur_size += sizeof(STLV16HeaderBigEndian);
                        strcpy((TChar *)p_cur, "Username too long");
                        sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                        err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                        DASSERT_OK(err);
                        break;
                    }
                    memcpy(account_name, p_cur, tlv_len);
                    LOG_PRINTF("[account_name: %s], tlv_len %d\n", account_name, tlv_len);
                    p_cur += tlv_len;
                }

                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);
                if (DUnlikely(ETLV16Type_AccountPassword != tlv_type)) {
                    LOG_ERROR("No Password\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("No Password");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "No Password");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                } else {
                    if (DUnlikely(tlv_len >= DMAX_ACCOUNT_NAME_LENGTH)) {
                        LOG_ERROR("Password too long\n");
                        p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                        p_tlv = (STLV16HeaderBigEndian *) p_cur;
                        cur_size = strlen("Password too long");
                        p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                        p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                        p_tlv->length_high = (cur_size >> 8) & 0xff;
                        p_tlv->length_low = cur_size & 0xff;
                        p_cur += sizeof(STLV16HeaderBigEndian);
                        cur_size += sizeof(STLV16HeaderBigEndian);
                        strcpy((TChar *)p_cur, "Password too long");
                        sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                        err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                        DASSERT_OK(err);
                        break;
                    }
                    memcpy(account_password, p_cur, tlv_len);
                    LOG_PRINTF("[account_password: %s], tlv_len %d\n", account_password, tlv_len);
                    p_cur += tlv_len;
                }

                TUniqueID newid = 0x0;
                cur_size = 0;
                //todo
                err = context->p_account_manager->NewAccount(account_name, account_password, EAccountGroup_UserAccount, newid, EAccountCompany_Ambarella);
                if (EECode_OK == err) {
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    //wirte ID
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
                    p_tlv->length_high = 0;
                    p_tlv->length_low = sizeof(TUniqueID);
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);

                    LOG_PRINTF("id %llx\n", newid);
                    p_cur[0] = (newid >> 56) & 0xff;
                    p_cur[1] = (newid >> 48) & 0xff;
                    p_cur[2] = (newid >> 40) & 0xff;
                    p_cur[3] = (newid >> 32) & 0xff;
                    p_cur[4] = (newid >> 24) & 0xff;
                    p_cur[5] = (newid >> 16) & 0xff;
                    p_cur[6] = (newid >> 8) & 0xff;
                    p_cur[7] = (newid) & 0xff;
                    LOG_PRINTF("%02x %02x %02x %02x %02x %02x %02x %02x\n", p_cur[0], p_cur[1], p_cur[2], p_cur[3], p_cur[4], p_cur[5], p_cur[6], p_cur[7]);
                    p_cur += sizeof(TUniqueID);
                    cur_size += sizeof(TUniqueID);

                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

                    DASSERT((cur_size + sizeof(SSACPHeader)) < DSACP_MAX_PAYLOAD_LENGTH);
                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, cur_size + sizeof(SSACPHeader));
                    LOG_NOTICE("reply size %zu\n", cur_size + sizeof(SSACPHeader));
                    DASSERT_OK(err);
                } else {
                    LOG_ERROR("NewAccount fail\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("NewAccount fail");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "NewAccount fail");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                }
            } else {
                LOG_ERROR("NULL context->p_account_manager\n");
                p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                cur_size = strlen("NULL p_account_manager");
                p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                p_tlv->length_high = (cur_size >> 8) & 0xff;
                p_tlv->length_low = cur_size & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                strcpy((TChar *)p_cur, "NULL p_account_manager");
                sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_ServerLogicalBug);

                err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                DASSERT_OK(err);
                break;
            }
            break;

        case ESACPAdminSubType_QuerySinkViaAccountName:
            LOG_PRINTF("ESACPAdminSubType_QuerySinkViaAccountName start\n");
            if (DLikely(context->p_account_manager)) {
                SAccountInfoUser *p_info = NULL;
                TChar account_name[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
                p_tlv = (STLV16HeaderBigEndian *) p_data;
                TU16 tlvtype = (p_tlv->type_high << 8) | (p_tlv->type_low);
                length = (p_tlv->length_high << 8) | (p_tlv->length_low);
                memcpy(account_name, p_data + sizeof(STLV16HeaderBigEndian), length);
                err = context->p_account_manager->QueryUserAccountByName((TChar *)account_name, p_info);
                if ((EECode_OK == err) && p_info) {
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    cur_size = 0;
                    //wirte ID, username, password, id string here
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
                    p_tlv->length_high = 0;
                    p_tlv->length_low = sizeof(TUniqueID);
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    LOG_PRINTF("id %llx\n", p_info->root.header.id);
                    p_cur[0] = (p_info->root.header.id >> 56) & 0xff;
                    p_cur[1] = (p_info->root.header.id >> 48) & 0xff;
                    p_cur[2] = (p_info->root.header.id >> 40) & 0xff;
                    p_cur[3] = (p_info->root.header.id >> 32) & 0xff;
                    p_cur[4] = (p_info->root.header.id >> 24) & 0xff;
                    p_cur[5] = (p_info->root.header.id >> 16) & 0xff;
                    p_cur[6] = (p_info->root.header.id >> 8) & 0xff;
                    p_cur[7] = (p_info->root.header.id) & 0xff;
                    LOG_PRINTF("%02x %02x %02x %02x %02x %02x %02x %02x\n", p_cur[0], p_cur[1], p_cur[2], p_cur[3], p_cur[4], p_cur[5], p_cur[6], p_cur[7]);
                    p_cur += sizeof(TUniqueID);
                    cur_size += sizeof(TUniqueID);

                    length = strlen(p_info->root.base.name);
                    DASSERT(length < DMAX_ACCOUNT_NAME_LENGTH_EX);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountName >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountName) & 0xff;
                    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
                    p_tlv->length_low = (length + 1) & 0xff;;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, p_info->root.base.name, length);
                    p_cur += length;
                    *p_cur ++ = 0x0;
                    cur_size += length + 1;

                    length = strlen(p_info->root.base.password);
                    DASSERT(length < DMAX_ACCOUNT_NAME_LENGTH);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountPassword >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountPassword) & 0xff;
                    p_tlv->length_high = ((length + 1) >> 8) & 0xff;
                    p_tlv->length_low = (length + 1) & 0xff;;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    memcpy(p_cur, p_info->root.base.password, length);
                    p_cur += length;
                    *p_cur ++ = 0x0;
                    cur_size += length + 1;

                    length = p_info->admindevicenum * sizeof(TUniqueID);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_UserOwnedDeviceList >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_UserOwnedDeviceList) & 0xff;
                    p_tlv->length_high = (length >> 8) & 0xff;
                    p_tlv->length_low = length & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    for (TU32 i = 0; i < p_info->admindevicenum; i++) {
                        p_cur[0] = (p_info->p_admindeviceid[i] >> 56) & 0xff;
                        p_cur[1] = (p_info->p_admindeviceid[i] >> 48) & 0xff;
                        p_cur[2] = (p_info->p_admindeviceid[i] >> 40) & 0xff;
                        p_cur[3] = (p_info->p_admindeviceid[i] >> 32) & 0xff;
                        p_cur[4] = (p_info->p_admindeviceid[i] >> 24) & 0xff;
                        p_cur[5] = (p_info->p_admindeviceid[i] >> 16) & 0xff;
                        p_cur[6] = (p_info->p_admindeviceid[i] >> 8) & 0xff;
                        p_cur[7] = (p_info->p_admindeviceid[i]) & 0xff;
                        LOG_PRINTF("p_info->p_admindeviceid[%d] %llx\n", i, p_info->p_admindeviceid[i]);
                        p_cur += sizeof(TUniqueID);
                        cur_size += sizeof(TUniqueID);
                    }

                    length = p_info->shareddevicenum * sizeof(TUniqueID);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_UserSharedDeviceList >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_UserSharedDeviceList) & 0xff;
                    p_tlv->length_high = (length >> 8) & 0xff;
                    p_tlv->length_low = length & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    for (TU32 i = 0; i < p_info->shareddevicenum; i++) {
                        p_cur[0] = (p_info->p_shareddeviceid[i] >> 56) & 0xff;
                        p_cur[1] = (p_info->p_shareddeviceid[i] >> 48) & 0xff;
                        p_cur[2] = (p_info->p_shareddeviceid[i] >> 40) & 0xff;
                        p_cur[3] = (p_info->p_shareddeviceid[i] >> 32) & 0xff;
                        p_cur[4] = (p_info->p_shareddeviceid[i] >> 24) & 0xff;
                        p_cur[5] = (p_info->p_shareddeviceid[i] >> 16) & 0xff;
                        p_cur[6] = (p_info->p_shareddeviceid[i] >> 8) & 0xff;
                        p_cur[7] = (p_info->p_shareddeviceid[i]) & 0xff;
                        LOG_PRINTF("p_info->p_shareddeviceid[%d] %llx\n", i, p_info->p_shareddeviceid[i]);
                        p_cur += sizeof(TUniqueID);
                        cur_size += sizeof(TUniqueID);
                    }

                    length = p_info->friendsnum * sizeof(TUniqueID);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_UserFriendList >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_UserFriendList) & 0xff;
                    p_tlv->length_high = (length >> 8) & 0xff;
                    p_tlv->length_low = length & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    for (TU32 i = 0; i < p_info->friendsnum; i++) {
                        p_cur[0] = (p_info->p_friendsid[i] >> 56) & 0xff;
                        p_cur[1] = (p_info->p_friendsid[i] >> 48) & 0xff;
                        p_cur[2] = (p_info->p_friendsid[i] >> 40) & 0xff;
                        p_cur[3] = (p_info->p_friendsid[i] >> 32) & 0xff;
                        p_cur[4] = (p_info->p_friendsid[i] >> 24) & 0xff;
                        p_cur[5] = (p_info->p_friendsid[i] >> 16) & 0xff;
                        p_cur[6] = (p_info->p_friendsid[i] >> 8) & 0xff;
                        p_cur[7] = (p_info->p_friendsid[i]) & 0xff;
                        LOG_PRINTF("p_info->p_friendsid[%d] %llx\n", i, p_info->p_friendsid[i]);
                        p_cur += sizeof(TUniqueID);
                        cur_size += sizeof(TUniqueID);
                    }

                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

                    DASSERT((cur_size + sizeof(SSACPHeader)) < DSACP_MAX_PAYLOAD_LENGTH);
                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, cur_size + sizeof(SSACPHeader));
                    DASSERT_OK(err);
                } else {
                    LOG_PRINTF("ESACPAdminSubType_QuerySinkViaAccountName not found, %s\n", account_name);
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("not found");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "not found");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_NotExist);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                }
            } else {
                LOG_ERROR("NULL context->p_account_manager\n");
                p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                cur_size = strlen("NULL p_account_manager");
                p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                p_tlv->length_high = (cur_size >> 8) & 0xff;
                p_tlv->length_low = cur_size & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                strcpy((TChar *)p_cur, "NULL p_account_manager");
                sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_ServerLogicalBug);

                err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                DASSERT_OK(err);
            }
            break;

        case ESACPAdminSubType_NewDataNode:
            LOG_PRINTF("ESACPAdminSubType_NewDataNode start\n");
            if (DLikely(context->p_account_manager)) {
                SAccountInfoCloudDataNode *p_info = NULL;
                TU16 tlv_type = 0;
                TU16 tlv_len = 0;
                TChar account_name[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
                TChar account_password[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
                //TChar account_idstring[DMAX_ID_AUTHENTICATION_STRING_LENGTH] = {0};

                p_cur = (TU8 *) p_data;
                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);
                if (DUnlikely(ETLV16Type_AccountName != tlv_type)) {
                    LOG_ERROR("No Username\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("No Username");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "No Username");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                } else {
                    if (DUnlikely(tlv_len >= DMAX_ACCOUNT_NAME_LENGTH_EX)) {
                        LOG_ERROR("Username too long\n");
                        p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                        p_tlv = (STLV16HeaderBigEndian *) p_cur;
                        cur_size = strlen("Username too long");
                        p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                        p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                        p_tlv->length_high = (cur_size >> 8) & 0xff;
                        p_tlv->length_low = cur_size & 0xff;
                        p_cur += sizeof(STLV16HeaderBigEndian);
                        cur_size += sizeof(STLV16HeaderBigEndian);
                        strcpy((TChar *)p_cur, "Username too long");
                        sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                        err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                        DASSERT_OK(err);
                        break;
                    }
                    memcpy(account_name, p_cur, tlv_len);
                    LOG_PRINTF("[account_name: %s], tlv_len %d\n", account_name, tlv_len);
                    p_cur += tlv_len;
                }

                TSocketPort admin_port = 0, cloud_port = 0, rtsp_port = 0;
                TU32 max_channel = 0, current_channel = 0;
                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);
                if (DUnlikely(ETLV16Type_DataNodeAdminPort != tlv_type)) {
                    LOG_ERROR("No Admin Port\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("No Admin Port");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "No Admin Port");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                }
                admin_port = ((TSocketPort)p_cur[0] << 8) | ((TSocketPort)p_cur[1]);
                DASSERT(2 == tlv_len);
                p_cur += tlv_len;

                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);
                if (DUnlikely(ETLV16Type_DataNodeCloudPort != tlv_type)) {
                    LOG_ERROR("No Cloud Port\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("No Cloud Port");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "No Cloud Port");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                }
                cloud_port = ((TSocketPort)p_cur[0] << 8) | ((TSocketPort)p_cur[1]);
                DASSERT(2 == tlv_len);
                p_cur += tlv_len;

                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);
                if (DUnlikely(ETLV16Type_DataNodeRTSPPort != tlv_type)) {
                    LOG_ERROR("No RTSP Port\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("No RTSP Port");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "No RTSP Port");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                }
                rtsp_port = ((TSocketPort)p_cur[0] << 8) | ((TSocketPort)p_cur[1]);
                DASSERT(2 == tlv_len);
                p_cur += tlv_len;

                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);
                if (DUnlikely(ETLV16Type_DataNodeMaxChannelNumber != tlv_type)) {
                    LOG_ERROR("No max channel\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("No max channel");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "No max channel");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                }
                max_channel = ((TU32)p_cur[0] << 24) | ((TU32)p_cur[1] << 16) | ((TU32)p_cur[2] << 8) | ((TU32)p_cur[3]);
                DASSERT(4 == tlv_len);
                p_cur += tlv_len;

                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                tlv_type = (p_tlv->type_high << 8) | (p_tlv->type_low);
                tlv_len = (p_tlv->length_high << 8) | (p_tlv->length_low);
                p_cur += sizeof(STLV16HeaderBigEndian);
                if (DUnlikely(ETLV16Type_DataNodeCurrentChannelNumber != tlv_type)) {
                    LOG_ERROR("No current channel\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("No current channel");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "No current channel");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                }
                current_channel = ((TU32)p_cur[0] << 24) | ((TU32)p_cur[1] << 16) | ((TU32)p_cur[2] << 8) | ((TU32)p_cur[3]);
                DASSERT(4 == tlv_len);
                p_cur += tlv_len;

                //TODO
                if (current_channel) { current_channel = 0; }

                TUniqueID newid = 0;
                err = context->p_account_manager->NewDataNode(account_name, account_password, newid, admin_port, cloud_port, rtsp_port, max_channel, current_channel);
                if (EECode_OK == err) {
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    //wirte ID
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    p_tlv->type_high = (ETLV16Type_AccountID >> 8) & 0xff;
                    p_tlv->type_low = (ETLV16Type_AccountID) & 0xff;
                    p_tlv->length_high = 0;
                    p_tlv->length_low = sizeof(TUniqueID);
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);

                    LOG_PRINTF("id %llx\n", newid);
                    p_cur[0] = (newid >> 56) & 0xff;
                    p_cur[1] = (newid >> 48) & 0xff;
                    p_cur[2] = (newid >> 40) & 0xff;
                    p_cur[3] = (newid >> 32) & 0xff;
                    p_cur[4] = (newid >> 24) & 0xff;
                    p_cur[5] = (newid >> 16) & 0xff;
                    p_cur[6] = (newid >> 8) & 0xff;
                    p_cur[7] = (newid) & 0xff;
                    LOG_PRINTF("%02x %02x %02x %02x %02x %02x %02x %02x\n", p_cur[0], p_cur[1], p_cur[2], p_cur[3], p_cur[4], p_cur[5], p_cur[6], p_cur[7]);
                    p_cur += sizeof(TUniqueID);
                    cur_size += sizeof(TUniqueID);

                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, DSACPHeaderFlagBit_PacketStartIndicator | DSACPHeaderFlagBit_PacketEndIndicator);

                    DASSERT((cur_size + sizeof(SSACPHeader)) < DSACP_MAX_PAYLOAD_LENGTH);
                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, cur_size + sizeof(SSACPHeader));
                    LOG_NOTICE("reply size %zu\n", cur_size + sizeof(SSACPHeader));
                    DASSERT_OK(err);
                } else {
                    LOG_ERROR("NewDataNode fail\n");
                    p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                    p_tlv = (STLV16HeaderBigEndian *) p_cur;
                    cur_size = strlen("NewDataNode fail");
                    p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                    p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                    p_tlv->length_high = (cur_size >> 8) & 0xff;
                    p_tlv->length_low = cur_size & 0xff;
                    p_cur += sizeof(STLV16HeaderBigEndian);
                    cur_size += sizeof(STLV16HeaderBigEndian);
                    strcpy((TChar *)p_cur, "NewDataNode fail");
                    sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_BadParam);

                    err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                    DASSERT_OK(err);
                    break;
                }
            } else {
                LOG_ERROR("NULL context->p_account_manager\n");
                p_cur = context->p_responce_buffer + sizeof(SSACPHeader);
                p_tlv = (STLV16HeaderBigEndian *) p_cur;
                cur_size = strlen("NULL p_account_manager");
                p_tlv->type_high = (ETLV16Type_ErrorDescription >> 8) & 0xff;
                p_tlv->type_low = ETLV16Type_ErrorDescription & 0xff;
                p_tlv->length_high = (cur_size >> 8) & 0xff;
                p_tlv->length_low = cur_size & 0xff;
                p_cur += sizeof(STLV16HeaderBigEndian);
                cur_size += sizeof(STLV16HeaderBigEndian);
                strcpy((TChar *)p_cur, "NULL p_account_manager");
                sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, cur_size, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_ServerLogicalBug);

                err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader) + cur_size);
                DASSERT_OK(err);
                break;
            }
            break;

        case ESACPAdminSubType_UpdateDataNode:
            LOG_ERROR("ESACPAdminSubType_UpdateDataNode: TO DO!\n");
            break;

        case ESACPAdminSubType_QueryAllDataNode:
            break;

        default:
            LOG_ERROR("Bad subtype %d\n", subtype);
            sacp_fill_header((SSACPHeader *) context->p_responce_buffer, type, 0, EProtocolHeaderExtentionType_SACP_ADMIN, 0, 0, ESACPRequestResult_ServerNotSupport);

            err = context->p_admin_server_port->Reply((SCommunicationPortSource *) p_port, context->p_responce_buffer, sizeof(SSACPHeader));
            DASSERT_OK(err);
            break;

    }

    return EECode_OK;
}

TInt imtest_config_test_user_info(IAccountManager *p_account_manager, SIMTestContxt *p_content, const TChar *configfile)
{
    test_loge("not implemented.\n");
    return -1;
}

TInt imtest_config_test_device_info(IAccountManager *p_account_manager, SIMTestContxt *p_content, const TChar *configfile)
{
    if (DUnlikely(!p_account_manager || !p_content || !configfile)) {
        test_loge("[input argument error]: imtest_config_test_device_info NULL params %p, %p, %p.\n", p_account_manager, p_content, configfile);
        return (-1);
    }

    IRunTimeConfigAPI *api = gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI, 128);
    if (DUnlikely(!api)) {
        test_loge("gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI) fail.\n");
        return (-2);
    }
    IRunTimeConfigAPI *api_write = gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI, 128);
    if (DUnlikely(!api_write)) {
        test_loge("gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI) api_write fail.\n");
        return (-2);
    }

    EECode err = api->OpenConfigFile((const TChar *)configfile, 0, 1, 1);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("OpenConfigFile(%s) fail, ret %d, %s.\n", configfile, err, gfGetErrorCodeString(err));
        return (-3);
    }

    TChar read_string[256] = {0};
    TChar read_string1[256] = {0};
    TChar device_username[user_name_password_len + 1];
    TChar device_password[user_name_password_len + 1];
    TChar device_identity_string[DIdentificationStringLength + 1];
    TChar device_product_code[DMAX_PRODUCTION_CODE_LENGTH + 1];
    TInt device_username_len = 0, device_password_len = 0, device_identitystring_len = 0, device_product_code_length = 0;
    TUint devicenumber = 0;
    TU32 val = 0;
    TUniqueID device_unique_id = 0;

    err = api->GetContentValue("devicenumber", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(devicenumber) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        err = api->CloseConfigFile();
        DASSERT_OK(err);
        return -3;
    } else {
        sscanf(read_string, "%d", &val);
        devicenumber = val;
        //test_log("devicenumber:%s, %d\n", read_string, devicenumber);
    }

    for (TUint index = 0; index < devicenumber; index++) {
        sprintf(read_string, "device_%d_info", index);
        memset(read_string1, 0x0, sizeof(read_string1));
        err = api->GetContentValue((const TChar *)read_string, read_string1);
        if (DUnlikely(EECode_OK != err)) {
            test_loge("GetContentValue(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
            err = api->CloseConfigFile();
            DASSERT_OK(err);
            return -3;
        } else {
            //test_log("%s: %s\n", read_string, read_string1);
            memset(device_username, 0x0, sizeof(device_username));
            memset(device_password, 0x0, sizeof(device_password));
            memset(device_identity_string, 0x0, sizeof(device_identity_string));
            memset(device_product_code, 0x0, sizeof(device_product_code));
            sscanf(read_string1, "%s %s %s", device_username, device_password, device_identity_string);
            /*test_log("device_username: %s\n", device_username);
            test_log("device_password: %s\n", device_password);
            test_log("device_identity_string: %s\n", device_identity_string);*/
            device_username_len = strnlen(device_username, user_name_password_len);
            device_password_len = strnlen(device_password, user_name_password_len);
            device_identitystring_len = strnlen(device_identity_string, DIdentificationStringLength);
            device_product_code_length = DMAX_PRODUCTION_CODE_LENGTH;
            if ((user_name_password_len < device_username_len)
                    || (user_name_password_len < device_password_len)
                    || (DIdentificationStringLength < device_identitystring_len)) {
                test_loge("GetContentValue(%s) fail, device_username_len=%d(should be %d), device_password_len=%d(should be %d), device_identitystring_len=%d(should<=%d).\n",
                          read_string, device_username_len, user_name_password_len, device_password_len, user_name_password_len, device_identitystring_len, DIdentificationStringLength);
                return -3;
            }
            err = p_account_manager->NewAccount(device_username, device_password, EAccountGroup_SourceDevice, device_unique_id, EAccountCompany_Ambarella, device_product_code, device_product_code_length);
            if (DUnlikely(EECode_OK != err)) {
                test_loge("NewAccount(%s %s %s) fail, ret %d, %s.\n", device_username, device_password, device_identity_string, err, gfGetErrorCodeString(err));
                return -3;
            }

            sprintf(read_string, "device_%s_info.ini", device_username);
            EECode err = api_write->OpenConfigFile((const TChar *)read_string, 1, 0, 1);
            if (DUnlikely(EECode_OK != err)) {
                test_loge("OpenConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                return (-3);
            }
            err = api_write->SetContentValue("username", device_username);
            if (DUnlikely(EECode_OK != err)) {
                test_loge("SetContentValue(username) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                err = api_write->CloseConfigFile();
                DASSERT_OK(err);
                return -3;
            }
            err = api_write->SetContentValue("password", device_password);
            if (DUnlikely(EECode_OK != err)) {
                test_loge("SetContentValue(password) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                err = api_write->CloseConfigFile();
                DASSERT_OK(err);
                return -3;
            }
            err = api_write->SetContentValue("productcode", device_product_code);
            if (DUnlikely(EECode_OK != err)) {
                test_loge("SetContentValue(productcode) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                err = api_write->CloseConfigFile();
                DASSERT_OK(err);
                return -3;
            }
            memset(read_string1, 0x0, sizeof(read_string1));
            sprintf(read_string1, "%llx", device_unique_id);
            err = api_write->SetContentValue("uniqueid", read_string1);
            if (DUnlikely(EECode_OK != err)) {
                test_loge("SetContentValue(uniqueid) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                err = api_write->CloseConfigFile();
                DASSERT_OK(err);
                return -3;
            }
            err = api_write->SaveConfigFile(NULL);
            DASSERT_OK(err);
            err = api_write->CloseConfigFile();
            DASSERT_OK(err);

            sprintf(read_string, "device_%s_identity.ini", device_username);
            err = api_write->OpenConfigFile((const TChar *)read_string, 1, 0, 1);
            if (DUnlikely(EECode_OK != err)) {
                test_loge("OpenConfigFile(%s) fail, ret %d, %s.\n", read_string, err, gfGetErrorCodeString(err));
                return (-3);
            }
            err = api_write->SetContentValue("identity", device_identity_string);
            if (DUnlikely(EECode_OK != err)) {
                test_loge("SetContentValue(identity) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                err = api_write->CloseConfigFile();
                DASSERT_OK(err);
                return -3;
            }
            err = api_write->SaveConfigFile(NULL);
            DASSERT_OK(err);
            err = api_write->CloseConfigFile();
            DASSERT_OK(err);
        }
    }

    api_write->Destroy();

    err = api->CloseConfigFile();
    DASSERT_OK(err);
    api->Destroy();

    return 0;
}

TInt imtest_load_config(SIMTestContxt *p_content, const TChar *configfile)
{
    if (DUnlikely(!p_content || !configfile)) {
        test_loge("[input argument error]: mtest2_pushmode_load_config NULL params %p, %p.\n", p_content, configfile);
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

    err = api->GetContentValue("import", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(import) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
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

    err = api->GetContentValue("adminport", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(adminport) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
    } else {
        sscanf(read_string, "%d", &val);
        p_content->admin_port = val;
        test_log("admin port:%s, %d\n", read_string, p_content->admin_port);
    }

    err = api->CloseConfigFile();
    DASSERT_OK(err);

    api->Destroy();

    return 0;
}

TInt imtest_load_default_device_account(SIMTestContxt *p_content, const TChar *configfile)
{
    if (DUnlikely(!p_content || !configfile)) {
        test_loge("[input argument error]: mtest2_pushmode_load_config NULL params %p, %p.\n", p_content, configfile);
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

    err = api->GetContentValue("import", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        test_loge("GetContentValue(import) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
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

    err = api->CloseConfigFile();
    DASSERT_OK(err);

    api->Destroy();

    return 0;
}

TInt imtest_init_params(SIMTestContxt *p_content, TInt argc, TChar **argv)
{
    TInt i = 0;
    TInt ret = 0;

    TULong cur_group = 0;
    TULong cur_source_is_hd = 0;
    TU8 total_group_number = 0;

    if (argc < 2) {
        //_imtest_print_version();
        _imtest_print_usage();
        _imtest_print_runtime_usage();
        return 1;
    }

    //parse options
    for (i = 1; i < argc; i++) {
        if (!strcmp("--serverport", argv[i])) {
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                p_content->server_port = ret;
                test_log("[input argument]: '--serverport': (%d).\n", p_content->server_port);
            } else {
                test_loge("[input argument error]: '--serverport', should follow with integer(port), argc %d, i %d.\n", argc, i);
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

TInt im_test(SIMTestContxt *p_context)
{
    TInt ret = 0;
    EECode err = EECode_OK;
    SAdminPortInfo test_admin;

    IIMServerManager *p_server_manager = gfCreateIMServerManager((const volatile SPersistCommonConfig *)&p_context->config, NULL, NULL);
    if (DUnlikely(!p_server_manager)) {
        LOG_ERROR("gfCreateIMServerManager() fail\n");
        return (-1);
    }

    err = p_server_manager->StartServerManager();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("StartServerManager fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return (-2);
    }

    IAccountManager *p_account_manager = gfCreateAccountManager(EAccountManagerType_generic, (const volatile SPersistCommonConfig *)&p_context->config, NULL, 0);
    if (DUnlikely(!p_account_manager)) {
        LOG_ERROR("gfCreateAccountManager() fail\n");
        return (-3);
    }

    err = p_account_manager->LoadFromDataBase((TChar *)"./database/sourcedevice_account.db", (TChar *)"./database/sourcedevice_account_ext.db", (TChar *)"./database/user_account.db", (TChar *)"./database/user_account_ext.db", (TChar *)"./database/datanode_account.db", (TChar *)"./database/datanode_account_ext.db", (TChar *)"./database/datanode_list.db", (TChar *)"./database/controlnode_account.db", (TChar *)"./database/controlnode_account_ext.db");
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("LoadFromDataBase fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    if (-1 != access("im_device_add_account_list.ini", F_OK)) {
        ret = imtest_config_test_device_info(p_account_manager, p_context, "im_device_add_account_list.ini");
        if (0 != ret) {
            LOG_ERROR("imtest_config_test_device_info fail\n");
            return ret;
        }
    } else {
        LOG_NOTICE("im_device_add_account_list.ini not exist.\n");
    }

    IIMServer *p_server = gfCreateIMServer(EProtocolType_SACP, &p_context->config, NULL, p_account_manager, p_context->server_port);
    if (DUnlikely(!p_server)) {
        LOG_ERROR("gfCreateIMServer(EProtocolType_SACP) fail\n");
        return (-4);
    }

    err = p_server_manager->AddServer(p_server);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("AddServer fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return (-5);
    }

    if (!p_context->admin_port) {
        p_context->admin_port = DDefaultSACPAdminPort;
    }
    LOG_NOTICE("admin port %hd\n", p_context->admin_port);

    CICommunicationServerPort *p_admin_server_port = CICommunicationServerPort::Create(&p_context->callback_context, __im_process_admin_callback, p_context->admin_port);
    if (DUnlikely(!p_admin_server_port)) {
        LOG_ERROR("CICommunicationServerPort::Create fail\n");
        return (-6);
    }

    p_context->callback_context.p_account_manager = p_account_manager;
    p_context->callback_context.p_admin_server_port = p_admin_server_port;
    p_context->callback_context.p_server = p_server;

    memset(&test_admin, 0x0, sizeof(test_admin));
    strcpy(test_admin.source, "testadmin");
    strcpy(test_admin.password, "123456");
    strcpy(test_admin.id_string, "21436587");

    SCommunicationPortSource *source_port = NULL;
    err = p_admin_server_port->AddSource(test_admin.source, test_admin.password, NULL, test_admin.id_string, (void *)&test_admin, source_port);
    if (DUnlikely((EECode_OK != err) || (!source_port))) {
        LOG_ERROR("AddSource fail, ret %d, %s, source_port %p\n", err, gfGetErrorCodeString(err), source_port);
        return (-7);
    }

    err = p_admin_server_port->Start(DDefaultSACPAdminPort);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("Start fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return (-8);
    }

    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;

    TInt flags;
    flags = fcntl(STDIN_FILENO, F_GETFL);
    flags |= O_NONBLOCK;
    if (fcntl(STDIN_FILENO, F_SETFL, flags) == -1) {
        perror("fcntl");
        test_loge("set STDIN_FILENO O_NONBLOCK failed.\n");
        return -1;
    }

    if (!p_context->daemon) {

        //process user cmd line
        while (g_running) {
            //add sleep to avoid affecting the performance
            usleep(500000);
            //memset(buffer, 0x0, sizeof(buffer));
            if (read(STDIN_FILENO, buffer, sizeof(buffer)) < 0) {
                TInt errcode = errno;
                if (errcode == EAGAIN) {
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
                    g_running = 0;
                    break;

                case 'p': {
                        if (!strncmp("paccount", p_buffer, strlen("paccount"))) {
                            test_log("print account\n");
                            p_account_manager->PrintStatus0();
                        } else if (!strncmp("pclient", p_buffer, strlen("pclient"))) {
                            test_log("print client\n");
                            p_server->PrintStatus();
                        }
                    }
                    break;

                case 'h':   // help
                    _imtest_print_version();
                    _imtest_print_usage();
                    _imtest_print_runtime_usage();
                    break;

                default:
                    break;
            }
        }

    } else {
        while (g_running) {
            usleep(5000000);
        }
    }

    p_admin_server_port->Stop();
    p_admin_server_port->Destroy();
    p_admin_server_port = NULL;

    err = p_server_manager->StopServerManager();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("StopServerManager fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    err = p_account_manager->StoreToDataBase((TChar *)"./database/sourcedevice_account.db", (TChar *)"./database/sourcedevice_account_ext.db", (TChar *)"./database/user_account.db", (TChar *)"./database/user_account_ext.db", (TChar *)"./database/datanode_account.db", (TChar *)"./database/datanode_account_ext.db", (TChar *)"./database/datanode_list.db", (TChar *)"./database/controlnode_account.db", (TChar *)"./database/controlnode_account_ext.db");
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("StoreToDataBase fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
    }

    p_account_manager->Destroy();
    p_account_manager = NULL;

    //p_server->Destroy();
    //p_server = NULL;

    p_server_manager->Destroy();
    p_server_manager = NULL;

    return 0;
}

static void __im_test_sig_pipe(int a)
{
    test_logw("sig pipe\n");
}

static void __im_test_sigstop(int sig_num)
{
    test_logw("Sig Quit\n");
    g_running = 0;
}

static void _imtest_default_initial_setting(SIMTestContxt *p_content)
{
    memset(p_content, 0x0, sizeof(SIMTestContxt));

    p_content->daemon = 0;
    p_content->server_port = DDefaultSACPIMServerPort;

    p_content->config.database_config.level1_higher_bit = 63;
    p_content->config.database_config.level1_lower_bit = 50;

    p_content->config.database_config.level2_higher_bit = 49;
    p_content->config.database_config.level2_lower_bit = 41;

    p_content->config.database_config.host_tag_higher_bit = 31;
    p_content->config.database_config.host_tag_lower_bit = 28;
}

TInt main(TInt argc, TChar **argv)
{
    TInt ret = 0;
    SIMTestContxt content;

    signal(SIGPIPE,  __im_test_sig_pipe);
    signal(SIGINT, __im_test_sigstop);
    signal(SIGQUIT, __im_test_sigstop);
    signal(SIGTERM, __im_test_sigstop);

    _imtest_default_initial_setting(&content);

    if (1 == argc) {
        ret = imtest_load_config(&content, "im_conf.ini");
        if (0 > ret) {
            goto imtest_main_exit;
        }
    } else {
        if ((ret = imtest_init_params(&content, argc, argv)) < 0) {
            test_loge("imtest_init_params fail, return %d.\n", ret);
            goto imtest_main_exit;
        } else if (ret > 0) {
            goto imtest_main_exit;
        }
    }

    if (content.daemon) {
        daemon(1, 0);
    }

    content.callback_context.responce_buffer_size = DSACP_MAX_PAYLOAD_LENGTH;
    content.callback_context.p_responce_buffer = (TU8 *)malloc(content.callback_context.responce_buffer_size);
    if (!content.callback_context.p_responce_buffer) {
        LOG_FATAL("no memory\n");
        goto imtest_main_exit;
    }

    im_test(&content);

    test_log("[ut flow]: test end\n");

imtest_main_exit:

    return 0;
}

