/*******************************************************************************
 * deviceid_generator.cpp
 *
 * History:
 *    2014/03/13 - [Zhi He] create file
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

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "licence_lib_if.h"
#include "licence_encryption_if.h"

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
    TU32 device_id_index;

    TChar device_id_binary_file_name[128];
    TChar device_id_text_file_name[128];
} SDeviceIDGenerator;

TInt deviceid_generator_init_params(SDeviceIDGenerator *p_content, TInt argc, TChar **argv)
{
    TInt i = 0;
    TInt ret = 0;

    TULong cur_group = 0;
    TULong cur_source_is_hd = 0;
    TU8 total_group_number = 0;

    //parse options
    for (i = 1; i < argc; i++) {
        if (!strcmp("--deviceid", argv[i])) {
#if 0
            if ((i + 1) < argc) {
                snprintf(p_content->device_id_text_file_name, 128, "%s.txt", argv[i + 1]);
                snprintf(p_content->device_id_binary_file_name, 128, "%s.bin", argv[i + 1]);
            } else {
                test_loge("[input argument error]: '--deviceid', should follow with deviceid filename(%%s), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
#endif
        } else if (!strcmp("--deviceindex", argv[i])) {
            if ((i + 1) < argc) {
                sscanf(argv[i + 1], "%d", &p_content->device_id_index);
                test_log("[input argument]: device_id_index %d\n", p_content->device_id_index);
            } else {
                test_loge("[input argument error]: '--deviceindex', should follow with device index (%%d), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
            if (p_content->device_id_index) {
                snprintf(p_content->device_id_text_file_name, 128, "deviceid_%04d.txt", p_content->device_id_index);
            }
        } else {
            test_loge("[input argument error]: not recognized argument %s, argc %d, i %d.\n", argv[i], argc, i);
        }

    }

    return 0;
}

static EECode generate_device_id(SDeviceIDGenerator *p_content)
{
    SNetworkDevices devices;
    SDateTime datetime;

    ILicenceEncryptor *encryptor = NULL;

    test_log("[ut flow]: generate_device_id() begin\n");
    memset(&devices, 0x0, sizeof(SNetworkDevices));

    EECode err = gfGetNetworkDeviceInfo(&devices);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("gfGetNetworkDeviceInfo return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    memset(&datetime, 0x0, sizeof(datetime));
    err = gfGetCurrentDateTime(&datetime);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("gfGetCurrentDateTime return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    TU8 device_string[256] = {0};

    snprintf((TChar *)device_string, 255, "devindex=%d, hwaddr=%s, day=%04d-%02d-%02d, weekday=%02d, time=%02d:%02d:%02d", p_content->device_id_index, devices.device[0].mac_address, datetime.year, datetime.month, datetime.day, datetime.weekday, datetime.hour, datetime.minute, datetime.seconds);
    //test_log("device string: %s\n", device_string);

    TU8 encrypted_string[256 + 32] = {0};
    TMemSize encryption_size = 256 + 32;
    TChar readable_string[512 + 64] = {0};

    encryptor = gfCreateLicenceEncryptor(ELicenceEncryptionType_CustomizedV1, datetime.minute, datetime.seconds);
    if (DUnlikely(!encryptor)) {
        LOG_ERROR("gfCreateLicenceEncryptor return NULL\n");
        return EECode_NoMemory;
    }

    //LOG_NOTICE("before Encryption, input size %d, %02x %02x %02x %02x, %02x %02x %02x %02x\n", strlen((TChar*)device_string), device_string[0], device_string[1], device_string[2], device_string[3], device_string[4], device_string[5], device_string[6], device_string[7]);
    err = encryptor->Encryption((TU8 *)encrypted_string, encryption_size, (TU8 *)device_string, (TMemSize)strlen((TChar *)device_string));
    //LOG_NOTICE("after Encryption, encryption_size %ld, %02x %02x %02x %02x, %02x %02x %02x %02x\n", encryption_size, encrypted_string[0], encrypted_string[1], encrypted_string[2], encrypted_string[3], encrypted_string[4], encrypted_string[5], encrypted_string[6], encrypted_string[7]);

    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("Encryption return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    FILE *device_file = NULL;
    //TChar device_filename[128] = {0};
    //snprintf(device_filename, 127, "device_id_%04d.bin", index);
    device_file = fopen(p_content->device_id_binary_file_name, "wb");
    if (DLikely(device_file)) {
        fwrite(encrypted_string, 1, encryption_size, device_file);
        fclose(device_file);
        device_file = NULL;
    }

    //LOG_NOTICE("before gfEncodingBase16, %02x %02x %02x %02x, %02x %02x %02x %02x\n", encrypted_string[0], encrypted_string[1], encrypted_string[2], encrypted_string[3], encrypted_string[4], encrypted_string[5], encrypted_string[6], encrypted_string[7]);
    gfEncodingBase16(readable_string, (const TU8 *)encrypted_string, (TInt)encryption_size);
    //LOG_NOTICE("after gfEncodingBase16: %02x %02x %02x %02x, %02x %02x %02x %02x\n", readable_string[0], readable_string[1], readable_string[2], readable_string[3], readable_string[4], readable_string[5], readable_string[6], readable_string[7]);
    test_log("[ut flow]: readable_string %s, encryption_size %ld\n", readable_string, encryption_size);

    //snprintf(device_filename, 127, "device_id_%04d.txt", index);
    device_file = fopen(p_content->device_id_text_file_name, "wb");
    if (DLikely(device_file)) {
        fwrite(readable_string, 1, encryption_size * 2, device_file);
        fclose(device_file);
        device_file = NULL;
    }

    TU8 decryped_string[256] = {0};
    TMemSize decryption_size = 256;

    err = encryptor->Decryption(encrypted_string, encryption_size, decryped_string, decryption_size);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("Decryption return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    DASSERT(decryption_size == strlen((TChar *)device_string));
    if (DUnlikely(memcmp((void *)decryped_string, (void *)device_string, decryption_size))) {
        LOG_ERROR("Encryption Decryption check fail, decryped_string %s, device_string %s\n", decryped_string, device_string);
        return EECode_InternalLogicalBug;
    } else {
        //LOG_NOTICE("Encryption Decryption result, decryped_string %s\n", decryped_string);
    }

    test_log("[ut flow]: generate_device_id() end\n");

    return EECode_OK;
}

TInt main(TInt argc, TChar **argv)
{
    TInt ret;
    TUint i = 0;
    EECode err;

    SDeviceIDGenerator *p_content = (SDeviceIDGenerator *) DDBG_MALLOC(sizeof(SDeviceIDGenerator), "U0DG");
    if (DUnlikely(!p_content)) {
        test_loge("can not malloc mem(SDeviceIDGenerator), request size %d\n", sizeof(SDeviceIDGenerator));
        goto deviceid_generator_main_exit;
    }
    memset(p_content, 0x0, sizeof(SDeviceIDGenerator));

    snprintf(p_content->device_id_text_file_name, 128, "deviceid.txt");

    if ((ret = deviceid_generator_init_params(p_content, argc, argv)) < 0) {
        test_loge("deviceid_generator: deviceid_generator_init_params fail, return %d.\n", ret);
        goto deviceid_generator_main_exit;
    } else if (ret > 0) {
        goto deviceid_generator_main_exit;
    }

    err = generate_device_id(p_content);

    test_log("[ut flow]: test end\n");

deviceid_generator_main_exit:

    if (p_content) {
        DDBG_FREE(p_content, "U0DG");
        p_content = NULL;
    }

    return 0;
}

