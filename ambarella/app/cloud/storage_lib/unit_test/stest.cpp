/*******************************************************************************
 * stest.cpp
 *
 * History:
 *    2014/05/04 - [Zhi He] create file
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
#include "common_private_data.h"

#include "storage_lib_if.h"

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

TInt g_storage_manage_type = EStorageMnagementType_Simple;
TU32 g_channel_number = 4;

TInt stest_init_params(TInt argc, TChar **argv)
{
    TInt i = 0;
    TInt ret = 0;

    //parse options
    for (i = 1; i < argc; i++) {
        if (!strcmp("--type", argv[i])) {
            test_log("'--type': storage manage type\n");
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                g_storage_manage_type = ret;
                test_log("storage manage type %d\n", g_storage_manage_type);
            } else {
                test_loge("[input argument error]: '--type', should follow with storage manage type(%%d, 1:simple, 2:sqlite), argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("--channel", argv[i])) {
            test_log("'--channel': storage channel number\n");
            if (((i + 1) < argc) && (1 == sscanf(argv[i + 1], "%d", &ret))) {
                g_channel_number = (TU32)ret;
                test_log("storage channel number %u\n", g_channel_number);
            } else {
                test_loge("[input argument error]: '--channel', should follow with storage channel number(%%d), argc %d, i %d.\n", argc, i);
                return (-1);
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

static TU8 *_find_pridata_pesheader(TU8 *p_data, TInt size)
{
    TU8 *ptr = p_data;
    if (NULL == ptr) //check for safety
    { return NULL; }

    while (ptr < p_data + size - 4) {
        if (*ptr == 0x00) {
            if (*(ptr + 1) == 0x00) {
                if (*(ptr + 2) == 0x01) {
                    if (*(ptr + 3) == 0xBF) {
                        return ptr;
                    }
                }
            }
        }
        ++ptr;
    }
    //test_loge("p_data=%p, cannot find pridata pes header.\n");
    return NULL;
}

void stest_simple_loop(IStorageManagement *p_store_manage)
{
    TInt ret = 0;
    EECode err = EECode_OK;
    TInt running = 1;

    TChar buffer_old[128] = {0};
    TChar buffer[128] = {0};
    TChar *p_buffer = buffer;
    TChar read_string[256] = {0};
    TChar read_string1[256] = {0};

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

            case 's': {
                    if (!strncmp("ssavedb", p_buffer, strlen("ssavedb"))) {
                        test_log("'ssavedb' start\n");
                        err = p_store_manage->SaveDataBase(g_channel_number);
                        test_log("'ssavedb' end, err=%d\n", err);
                    } else if (!strncmp("sinitdb:", p_buffer, strlen("sinitdb:"))) {
                        if (1 == sscanf(p_buffer, "sinitdb:%s", read_string)) {
                            test_log("'sinitdb:%s' start\n", read_string);
                            err = p_store_manage->LoadDataBase(read_string, g_channel_number);
                            test_log("'sinitdb:%s' end, err=%d\n", read_string, err);
                        } else {
                            test_loge("'sinitdb:%%s' should specify root path\n");
                        }
                    } else if (!strncmp("scleardb:", p_buffer, strlen("scleardb:"))) {
                        if (1 == sscanf(p_buffer, "scleardb:%s", read_string)) {
                            test_log("'scleardb:%s' start\n", read_string);
                            err = p_store_manage->ClearDataBase(read_string, g_channel_number);
                            test_log("'scleardb:%s' end, err=%d\n", read_string, err);
                        } else {
                            test_loge("'scleardb:%%s' should specify root path\n");
                        }
                    } else if (!strncmp("saddchannel:", p_buffer, strlen("saddchannel:"))) {
                        TU32 max_storage_time = 0, redudant_storage_time = 0, single_file_duration_minutes = 0;
                        if (4 == sscanf(p_buffer, "saddchannel:%s %d %d %d", read_string, &max_storage_time, &redudant_storage_time,  &single_file_duration_minutes)) {
                            test_log("'saddchannel:'%s start\n", read_string);
                            err = p_store_manage->AddChannel(read_string, max_storage_time, redudant_storage_time, single_file_duration_minutes);
                            test_log("'saddchannel:'%s end, err=%d\n", read_string, err);
                        } else {
                            test_loge("'saddchannel:%%s %%d %%d %%d' should specify channel name, max_storage_time, redudant_storage_time and single_file_duration_minutes\n");
                        }
                    } else if (!strncmp("srmchannel:", p_buffer, strlen("srmchannel:"))) {
                        if (1 == sscanf(p_buffer, "srmchannel:%s", read_string)) {
                            test_log("'srmchannel:'%s start\n", read_string);
                            err = p_store_manage->RemoveChannel(read_string);
                            test_log("'srmchannel:'%s end, err=%d\n", read_string, err);
                        } else {
                            test_loge("'srmchannel:%%s' should specify channel name\n");
                        }
                    } else if (!strncmp("sclearchannel:", p_buffer, strlen("sclearchannel:"))) {
                        if (1 == sscanf(p_buffer, "sclearchannel:%s", read_string)) {
                            test_log("'sclearchannel:%s' start\n", read_string);
                            err = p_store_manage->ClearChannelStorage(read_string);
                            test_log("'sclearchannel:%s' end, err=%d\n", read_string, err);
                        } else {
                            test_loge("'sclearchannel:%%s' should specify channel name\n");
                        }
                    } else if (!strncmp("squerychannel:", p_buffer, strlen("squerychannel:"))) {
                        if (1 == sscanf(p_buffer, "squerychannel:%s", read_string)) {
                            test_log("'squerychannel:%s' start\n", read_string);
                            void *p_out = NULL;
                            err = p_store_manage->QueryChannelStorage(read_string, p_out);
                            test_log("'squerychannel:%s' end, err=%d\n", read_string, err);
                        } else {
                            test_loge("'squerychannel:%%s' should specify channel name\n");
                        }
                    } else if (!strncmp("srequnit:", p_buffer, strlen("srequnit:"))) {
                        SDateTime time = {0}, filestarttime = {0};
                        TChar *file_name = NULL;
                        TU16 fileduration = 0;
                        TU32 tmp1, tmp2, tmp3, tmp4, tmp5;//, tmp6;
                        TInt ret_sscanf = 0;
                        //                        ret_sscanf = sscanf(p_buffer, "srequnit:%s %d-%d-%d-%d-%d-%d", read_string, &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6);
                        //                        if (7 == ret_sscanf) {
                        ret_sscanf = sscanf(p_buffer, "srequnit:%s %d-%d-%d-%d-%d", read_string, &tmp1, &tmp2, &tmp3, &tmp4, &tmp5);
                        if (6 == ret_sscanf) {
                            time.year = tmp1;
                            time.month = tmp2;
                            time.day = tmp3;
                            time.hour = tmp4;
                            time.minute = tmp5;
                            time.seconds = 0;//tmp6;
                            test_log("'srequnit:%s' start\n", read_string);
                            err = p_store_manage->RequestExistUint(read_string, &time, file_name, &filestarttime, fileduration);
                            //                            test_log("'srequnit:%s' end, err=%d, file_name=%s, starttime=%d-%d-%d-%d-%d-%d, fileduration=%d\n", read_string, err, file_name, filestarttime.year, filestarttime.month, filestarttime.day, filestarttime.hour, filestarttime.minute, filestarttime.seconds, fileduration);
                            test_log("'srequnit:%s' end, err=%d, file_name=%s, starttime=%04d-%02d-%02d-%02d-%02d, fileduration=%d\n", read_string, err, file_name, filestarttime.year, filestarttime.month, filestarttime.day, filestarttime.hour, filestarttime.minute, fileduration);
                        } else {
                            test_loge("'srequnit:%%s %%d-%%d-%%d-%%d-%%d-%%d' should specify channel name and file start time\n");
                        }
                    } else if (!strncmp("srequnit1by1:", p_buffer, strlen("srequnit1by1:"))) {//direction: 0-newer file, 1-older file
                        TChar *file_name = NULL;
                        SDateTime filestarttime = {0};
                        TU16 fileduration = 0;
                        TU32 tmp1;
                        TInt ret_sscanf = 0;
                        ret_sscanf = sscanf(p_buffer, "srequnit1by1:%s %s %d", read_string, read_string1, &tmp1);
                        if (3 == ret_sscanf) {
                            test_log("'srequnit1by1:%s %s %d' start\n", read_string, read_string1, tmp1);
                            err = p_store_manage->RequestExistUintSequentially(read_string, read_string1, tmp1, file_name, &filestarttime, fileduration);
                            test_log("'srequnit1by1:%s %s %d' end, err=%d, file_name=%s, starttime=%04d-%02d-%02d-%02d-%02d, fileduration=%d\n", read_string, read_string1, tmp1, err, file_name, filestarttime.year, filestarttime.month, filestarttime.day, filestarttime.hour, filestarttime.minute, fileduration);
                        } else {
                            test_loge("'srequnit1by1:%%s %%s %%d' should specify channel name, old file name and request direction(1-backward, 0-forward)\n");
                        }
                    } else if (!strncmp("srelunit:", p_buffer, strlen("srelunit:"))) {
                        TChar *file_name = NULL;
                        if (2 == sscanf(p_buffer, "srelunit:%s %s", read_string, read_string1)) {
                            test_log("'srelunit:%s, %s' start\n", read_string, read_string1);
                            err = p_store_manage->ReleaseExistUint(read_string, read_string1);
                            test_log("'srelunit:%s %s' end, err=%d\n", read_string, read_string1, err);
                        } else {
                            test_loge("'srelunit:%%s %%s' should specify channel name and file name\n");
                        }
                    } else if (!strncmp("sacqunit:", p_buffer, strlen("sacqunit:"))) {
                        TChar *file_name = NULL;
                        if (1 == sscanf(p_buffer, "sacqunit:%s", read_string)) {
                            test_log("'sacqunit:%s' start\n", read_string);
                            err = p_store_manage->AcquireNewUint(read_string, file_name);
                            test_log("'sacqunit:%s' end, err=%d, file_name=%s\n", read_string, err, file_name);
                            TChar cmd[256] = "";
                            sprintf(cmd, "touch %s", file_name);
                            LOG_INFO("%s\n", cmd);
                            system(cmd);
                        } else {
                            test_loge("'sacqunit:%%s' should specify channel name\n");
                        }
                    } else if (!strncmp("sfinunit:", p_buffer, strlen("sfinunit:"))) {
                        if (2 == sscanf(p_buffer, "sfinunit:%s %s", read_string, read_string1)) {
                            test_log("'sfinunit:%s, %s' start\n", read_string, read_string1);
                            err = p_store_manage->FinalizeNewUint(read_string, read_string1);
                            test_log("'sfinunit:%s %s' end, err=%d\n", read_string, read_string1, err);
                        } else {
                            test_loge("'sfinunit:%%s %%s' should specify channel name and file name\n");
                        }
                    }
                }
                break;
            case 'r': {
                    FILE *pf = NULL;
                    TU8 *p_data = NULL;
                    TU8 *p_data_tmp = NULL;
                    TU8 *p_cur = NULL;
                    TU8 *p_idx = NULL;
                    TU32 *p_idx32 = NULL;
                    TU32 file_size = 0;
                    TU16 src_len = 0, read_len = 0, ts_pak_len = 188, check_pak_num = 0;
                    EECode err = EECode_OK;
                    TInt ret = 0;
                    if (!strncmp("rh", p_buffer, strlen("rh"))) {//retrieve head private data
                        do {
                            if (1 == sscanf(p_buffer, "rhst:%s", read_string)) {
                                test_log("'rhst:%s' start\n", read_string);
                            } else if (1 == sscanf(p_buffer, "rhdu:%s", read_string)) {
                                test_log("'rhdu:%s' start\n", read_string);
                            } else if (1 == sscanf(p_buffer, "rhcn:%s", read_string)) {
                                test_log("'rhcn:%s' start\n", read_string);
                            } else {
                                test_loge("command invalid or ts file path not specified.\n");
                                break;
                            }
                            //fopen ts file
                            pf = fopen(read_string, "rb");
                            if (!pf) {
                                test_loge("ts private data retrieve test, can not open file %s\n", read_string);
                                break;
                            }
                            //seek to 188*n, retrieve data
                            ret = fseek(pf, 2 * ts_pak_len, SEEK_SET);
                            if (0 != ret) {
                                test_loge("ts private data retrieve test, fseek to 2*188 from head failed.\n");
                                break;
                            }
                            check_pak_num = 1;
                            p_data = (TU8 *)malloc(ts_pak_len * check_pak_num);
                            if (!p_data) {
                                test_loge("ts private data retrieve test, cannot malloc 1*188 mem to retrieve data.\n");
                                break;
                            }
                            read_len = fread(p_data, 1, ts_pak_len * check_pak_num, pf);
                            if (read_len != ts_pak_len * check_pak_num) {
                                test_loge("ts private data retrieve test, cannot read data correctly, read_len=%u(should==%u)\n", read_len, ts_pak_len * check_pak_num);
                                break;
                            }
                            if (!(p_cur = _find_pridata_pesheader(p_data, ts_pak_len * check_pak_num))) {
                                test_loge("ts private data retrieve test, can not find pridata pesheader.\n");
                                break;
                            }
                            //now p_cur pointer to the beginning of pridata PES Header, 0x00 00 01 BF
                            p_cur += 4;
                            src_len = *((TU16 *)p_cur);
                            if (src_len < sizeof(STSPrivateDataHeader)) {
                                test_loge("ts private data retrieve test, data src_len=%u(should>=%zu), please check the binary.\n",
                                          src_len, sizeof(STSPrivateDataHeader));
                                break;
                            }
                            p_cur += 2;
                            //now p_cur pointer to the beginning of STSPrivateDataHeader
                            if (!strncmp("rhst:", p_buffer, strlen("rhst:"))) {
                                SDateTime data_time = {0};
                                err = gfRetrieveTSPriData((void *)&data_time, sizeof(data_time), p_cur, src_len, ETSPrivateDataType_StartTime, 0);
                                if (EECode_OK != err) {
                                    test_loge("ts private data retrieve test, gfRetrieveTSPriData ETSPrivateDataType_StartTime failed, err=%d\n", err);
                                    break;
                                }
                                test_log("'rhst:%s' ok, start time = %04d-%02d-%02d-%02d-%02d-%02d\n", read_string, data_time.year, data_time.month, data_time.day, data_time.hour, data_time.minute, data_time.seconds);
                            } else if (!strncmp("rhdu:", p_buffer, strlen("rhdu:"))) {
                                TU16 duration = 0;
                                err = gfRetrieveTSPriData((void *)&duration, sizeof(duration), p_cur, src_len, ETSPrivateDataType_Duration, 0);
                                if (EECode_OK != err) {
                                    test_loge("ts private data retrieve test, gfRetrieveTSPriData ETSPrivateDataType_Duration failed, err=%d\n", err);
                                    break;
                                }
                                test_log("'rhdu:%s' ok, duration = %u seconds\n", read_string, duration);
                            } else if (!strncmp("rhcn:", p_buffer, strlen("rhcn:"))) {
                                TU8 channel_name[DMaxChannelNameLength];
                                memset(channel_name, 0x0, DMaxChannelNameLength);
                                err = gfRetrieveTSPriData((void *)channel_name, DMaxChannelNameLength, p_cur, src_len, ETSPrivateDataType_ChannelName, 0);
                                if (EECode_OK != err) {
                                    test_loge("ts private data retrieve test, gfRetrieveTSPriData ETSPrivateDataType_ChannelName failed, err=%d\n", err);
                                    break;
                                }
                                test_log("'rhcn:%s' ok, channel_name = %s\n", read_string, channel_name);
                            } else {
                                test_loge("'unknown command for retrieve private data in the head of file.\n");
                            }
                        } while (0);
                        if (p_data) {
                            free(p_data);
                            p_data = NULL;
                        }
                        if (pf) {
                            fclose(pf);
                            pf = NULL;
                        }
                    } else if (!strncmp("rt", p_buffer, strlen("rt"))) {//retrieve tail private data
                        if (!strncmp("rtpi:", p_buffer, strlen("rtpi:"))) {
                            if (1 == sscanf(p_buffer, "rtpi:%s", read_string)) {
                                test_log("'rtpi:%s' start\n", read_string);
                                do {
                                    //fopen ts file
                                    pf = fopen(read_string, "rb");
                                    if (!pf) {
                                        test_loge("ts private data retrieve test, can not open file %s\n", read_string);
                                        break;
                                    }
                                    //get file total size
                                    fseek(pf, 0, SEEK_END);
                                    file_size = ftell(pf);
                                    rewind(pf);
                                    //seek to 188*n, retrieve data
                                    check_pak_num = 200;
                                    ret = fseek(pf, file_size - (check_pak_num * ts_pak_len), SEEK_SET);
                                    if (0 != ret) {
                                        test_loge("ts private data retrieve test, fseek to 200*188 from tail failed. file size %u\n", file_size);
                                        break;
                                    }
                                    p_data = (TU8 *)malloc(check_pak_num * ts_pak_len);
                                    if (!p_data) {
                                        test_loge("ts private data retrieve test, cannot malloc 200*188 mem to retrieve data.\n");
                                        break;
                                    }
                                    read_len = fread(p_data, 1, check_pak_num * ts_pak_len, pf);
                                    if (read_len != check_pak_num * ts_pak_len) {
                                        test_loge("ts private data retrieve test, cannot read data correctly, read_len=%u(should==%u)\n", read_len, check_pak_num * ts_pak_len);
                                        break;
                                    }
                                    p_data_tmp = p_data;
                                    while (check_pak_num) {
                                        if (p_cur = _find_pridata_pesheader(p_data_tmp, ts_pak_len)) {
                                            //now p_cur pointer to the beginning of pridata PES Header, 0x00 00 01 BF
                                            p_cur += 4;
                                            src_len = ((TU16)p_cur[0] << 8) | (TU16)p_cur[1]; //LITTLE ENDIAN TO BIG ENDIAN
                                            if (src_len < sizeof(STSPrivateDataHeader)) {
                                                test_loge("ts private data retrieve test, data src_len=%u(should>=%zu), please check the binary.\n",
                                                          src_len, sizeof(STSPrivateDataHeader));
                                                break;
                                            }
                                            p_cur += 2;
                                            //now p_cur pointer to the beginning of STSPrivateDataHeader

                                            if (p_idx) {
                                                free(p_idx);
                                                p_idx = NULL;
                                            }
                                            p_idx = (TU8 *)malloc(src_len - sizeof(STSPrivateDataHeader));
                                            if (!p_idx) {
                                                test_loge("ts private data retrieve test, cannot malloc mem to retrieve data. p_idx=%p\n", p_idx);
                                                break;
                                            }
                                            err = gfRetrieveTSPriData((void *)p_idx, (src_len - sizeof(STSPrivateDataHeader)), p_cur, src_len, ETSPrivateDataType_TSPakIdx4GopStart, 0);
                                            if (EECode_OK != err) {
                                                test_loge("ts private data retrieve test, gfRetrieveTSPriData ETSPrivateDataType_TSPakIdx4GopStart failed, err=%d\n", err);
                                                break;
                                            }
                                            test_log("'rtpi:%s' ok, TS Packet Index of Gop begin <\n", read_string);
                                            p_idx32 = (TU32 *)p_idx;
                                            for (TU16 i = 0; i < (src_len - sizeof(STSPrivateDataHeader)) / 4; i++) {
                                                test_log("%u\n", *p_idx32);
                                                p_idx32++;
                                            }
                                            test_log("'TS Packet Index of Gop end -->, %s\n", read_string);
                                        }
                                        p_data_tmp += ts_pak_len;
                                        check_pak_num--;
                                    }
                                } while (0);
                                if (p_data) {
                                    free(p_data);
                                    p_data = NULL;
                                }
                                if (p_idx) {
                                    free(p_idx);
                                    p_idx = NULL;
                                }
                                if (pf) {
                                    fclose(pf);
                                    pf = NULL;
                                }
                            } else {
                                test_loge("'rtpi:%%s' should specify ts file path\n");
                            }
                        } else {
                            test_loge("'unknown command for retrieve private data in the tail of file.\n");
                        }
                    } else {
                        test_loge("'unknown command for retrieve private data.\n");
                    }
                }
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
    IStorageManagement *p_storage_manage = NULL;

    test_log("[ut flow]: stest start\n");

    if ((ret = stest_init_params(argc, argv)) < 0) {
        test_loge("stest: stest_init_params fail, return %d.\n", ret);
        goto stest_main_exit;
    }

    p_storage_manage = gfCreateStorageManagement((EStorageMnagementType)g_storage_manage_type);

    if (DLikely(p_storage_manage)) {
        stest_simple_loop(p_storage_manage);
        p_storage_manage->Delete();
    } else {
        test_loge("gfCreateStorageManagement() fail\n");
        goto stest_main_exit;
    }

    test_log("[ut flow]: stest end\n");

stest_main_exit:

    return 0;
}

