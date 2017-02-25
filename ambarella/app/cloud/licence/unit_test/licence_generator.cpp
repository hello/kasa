/*******************************************************************************
 * licence_generator.cpp
 *
 * History:
 *    2014/03/09 - [Zhi He] create file
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
    //licence content
    TChar company_name[64];
    TChar project_name[64];
    TChar server_name[64];
    TChar server_index[64];

    SDateTime expire_time;

    TU8 generate_device_id;
    TU8 generate_licence_file;
    TU8 reserved0;
    TU8 reserved1;

    TU32 device_id_index;
    TU32 licence_index;
    TU32 licence_index_ext;
    TU32 licence_max_channel;

    TChar device_id_binary_file_name[128];
    TChar device_id_text_file_name[128];

    TChar licence_binary_file_name[128];
    TChar licence_text_file_name[128];
} SLicenceGenerator;

TInt licence_generator_init_params(SLicenceGenerator *p_content, TInt argc, TChar **argv)
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
            p_content->generate_device_id = 1;
        } else if (!strcmp("--licensefile", argv[i])) {
#if 0
            if ((i + 1) < argc) {
                snprintf(p_content->licence_text_file_name, 128, "%s.txt", argv[i + 1]);
                snprintf(p_content->licence_binary_file_name, 128, "%s.bin", argv[i + 1]);
            } else {
                test_loge("[input argument error]: '--licencefile', should follow with licencefile name(%%s), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
#endif
            p_content->generate_licence_file = 1;
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
            p_content->generate_device_id = 1;
        } else if (!strcmp("--licenseindex", argv[i])) {
            if ((i + 1) < argc) {
                sscanf(argv[i + 1], "%d", &p_content->licence_index);
                test_log("[input argument]: licence_index %d\n", p_content->licence_index);
            } else {
                test_loge("[input argument error]: '--licenceindex', should follow with licence index (%%d), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
            p_content->generate_licence_file = 1;
        } else if (!strcmp("--company", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(p_content->company_name, 64, "%s", argv[i + 1]);
                test_log("[input argument]: company_name %s\n", p_content->company_name);
            } else {
                test_loge("[input argument error]: '--company', should follow with company name(%%s), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
        } else if (!strcmp("--project", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(p_content->project_name, 64, "%s", argv[i + 1]);
                test_log("[input argument]: project_name %s\n", p_content->project_name);
            } else {
                test_loge("[input argument error]: '--project', should follow with project name(%%s), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
        } else if (!strcmp("--server", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(p_content->server_name, 64, "%s", argv[i + 1]);
                test_log("[input argument]: server_name %s\n", p_content->server_name);
            } else {
                test_loge("[input argument error]: '--server', should follow with server name(%%s), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
        } else if (!strcmp("--serverindex", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(p_content->server_index, 64, "%s", argv[i + 1]);
                test_log("[input argument]: server_index %s\n", p_content->server_index);
            } else {
                test_loge("[input argument error]: '--serverindex', should follow with serverindex(%%s), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
        } else if (!strcmp("--maxchannel", argv[i])) {
            if ((i + 1) < argc) {
                sscanf(argv[i + 1], "%d", &p_content->licence_max_channel);
                test_log("[input argument]: licence_max_channel %d\n", p_content->licence_max_channel);
            } else {
                test_loge("[input argument error]: '--maxchannel', should follow with maxchannel(%%d), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
        } else if (!strcmp("--expire", argv[i])) {
            if ((i + 1) < argc) {
                TU32 year, month, day;
                sscanf(argv[i + 1], "%d-%d-%d", &year, &month, &day);
                if (month > 12) {
                    test_loge("month(%d) exceed 12\n", month);
                    month = 12;
                }
                if (day > 31) {
                    test_loge("day(%d) exceed 31\n", day);
                    month = 31;
                }
                test_log("[input argument]: expire date %d year, month %d, day %d\n", year, month, day);
                p_content->expire_time.year = year;
                p_content->expire_time.month = month;
                p_content->expire_time.day = day;
            } else {
                test_loge("[input argument error]: '--maxchannel', should follow with maxchannel(%%d), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
        } else if (!strcmp("--month", argv[i])) {
            if ((i + 1) < argc) {
                TU32 monthes = 0, years = 0;
                sscanf(argv[i + 1], "%d", &monthes);
                if (monthes) {
                    EECode err = gfGetCurrentDateTime(&p_content->expire_time);
                    years = monthes / 12;
                    monthes = monthes % 12;
                    p_content->expire_time.year += years;
                    p_content->expire_time.month += monthes;
                    test_log("[input argument]: licence length %d year, month %d, target %d year, %d month\n", years, monthes, p_content->expire_time.year, p_content->expire_time.month);
                } else {
                    p_content->expire_time.year = 0;
                    p_content->expire_time.month = 0;
                    p_content->expire_time.day = 0;
                }
            } else {
                test_loge("[input argument error]: '--maxchannel', should follow with maxchannel(%%d), argc %d, i %d.\n", argc, i);
                return (-4);
            }
            i ++;
        } else {
            test_loge("[input argument error]: not recognized argument %s, argc %d, i %d.\n", argv[i], argc, i);
        }

    }

    return 0;
}

static EECode generate_device_id(SLicenceGenerator *p_content)
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
    test_log("device string: %s\n", device_string);

    TU8 encrypted_string[256 + 32] = {0};
    TMemSize encryption_size = 256 + 32;
    TChar readable_string[512 + 64] = {0};

    encryptor = gfCreateLicenceEncryptor(ELicenceEncryptionType_CustomizedV1, datetime.minute, datetime.seconds);
    if (DUnlikely(!encryptor)) {
        LOG_ERROR("gfCreateLicenceEncryptor return NULL\n");
        return EECode_NoMemory;
    }

    err = encryptor->Encryption((TU8 *)encrypted_string, encryption_size, (TU8 *)device_string, (TMemSize)strlen((TChar *)device_string));
    test_log("[ut flow]: encryption_size %ld\n", encryption_size);

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

    LOG_NOTICE("%02x %02x %02x %02x, %02x %02x %02x %02x\n", encrypted_string[0], encrypted_string[1], encrypted_string[2], encrypted_string[3], encrypted_string[4], encrypted_string[5], encrypted_string[6], encrypted_string[7]);
    gfEncodingBase16(readable_string, (const TU8 *)encrypted_string, (TInt)encryption_size);

    test_log("[ut flow]: readable_string %s, encryption_size %ld\n", readable_string, encryption_size);

    //snprintf(device_filename, 127, "device_id_%04d.txt", index);
    device_file = fopen(p_content->device_id_text_file_name, "wb");
    if (DLikely(device_file)) {
        fwrite(readable_string, 2, encryption_size, device_file);
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
        LOG_NOTICE("Encryption Decryption result, decryped_string %s\n", decryped_string);
    }

    test_log("[ut flow]: generate_device_id() end\n");

    return EECode_OK;
}

static EECode generate_licencefile(SLicenceGenerator *p_content)
{
    ILicenceEncryptor *encryptor = NULL;
    EECode err;

    test_log("[ut flow]: generate_licencefile() begin\n");

    if (DUnlikely(!p_content)) {
        LOG_ERROR("NULL p_content\n");
        return EECode_BadParam;
    }

    TU8 device_string[256] = {0};
    TU8 device_encrypted_string[256 + 32] = {0};
    TU8 device_readable_string[512 + 64] = {0};
    TUint read_size = 0;

    FILE *device_file = NULL;
    device_file = fopen(p_content->device_id_text_file_name, "rb");
    if (DLikely(device_file)) {
        read_size = fread(device_readable_string, 1, 512 + 64, device_file);
        LOG_NOTICE("device_readable_string %s, read_size %ld\n", device_readable_string, read_size);
        if (DLikely(read_size)) {
            gfDecodingBase16(device_encrypted_string, (const TU8 *)device_readable_string, (TInt)read_size);
            //LOG_NOTICE("after gfDecodingBase16: %02x %02x %02x %02x, %02x %02x %02x %02x\n", device_encrypted_string[0], device_encrypted_string[1], device_encrypted_string[2], device_encrypted_string[3], device_encrypted_string[4], device_encrypted_string[5], device_encrypted_string[6], device_encrypted_string[7]);
            //LOG_NOTICE("%02x %02x %02x %02x, %02x %02x %02x %02x\n", device_encrypted_string[8], device_encrypted_string[9], device_encrypted_string[10], device_encrypted_string[11], device_encrypted_string[12], device_encrypted_string[13], device_encrypted_string[14], device_encrypted_string[15]);
        } else {
            LOG_FATAL("fread fail\n");
            return EECode_BadParam;
        }
        read_size = read_size / 2;
    } else {
#if 0
        device_file = fopen(p_content->device_id_binary_file_name, "rb");
        if (DLikely(device_file)) {
            read_size = fread(device_encrypted_string, 1, 256 + 32, device_file);
            LOG_NOTICE("%02x %02x %02x %02x, %02x %02x %02x %02x\n", device_encrypted_string[0], device_encrypted_string[1], device_encrypted_string[2], device_encrypted_string[3], device_encrypted_string[4], device_encrypted_string[5], device_encrypted_string[6], device_encrypted_string[7]);
            if (DUnlikely(!read_size)) {
                LOG_FATAL("fread fail\n");
            }
        } else {
            LOG_FATAL("open binary or txt file fail\n");
        }
#else
        LOG_FATAL("open file fail\n");
        return EECode_BadParam;
#endif
    }

    encryptor = gfCreateLicenceEncryptor(ELicenceEncryptionType_CustomizedV1, 17, 29);
    if (DUnlikely(!encryptor)) {
        LOG_ERROR("gfCreateLicenceEncryptor fail\n");
        return err;
    }

    TMemSize decryption_size = 256;
    //LOG_NOTICE("before Decryption, read_size %ld, %02x %02x %02x %02x, %02x %02x %02x %02x\n", read_size, device_encrypted_string[0], device_encrypted_string[1], device_encrypted_string[2], device_encrypted_string[3], device_encrypted_string[4], device_encrypted_string[5], device_encrypted_string[6], device_encrypted_string[7]);
    err = encryptor->Decryption(device_encrypted_string, (TMemSize)read_size, device_string, decryption_size);
    //LOG_NOTICE("after Decryption, decryption_size %ld, %02x %02x %02x %02x, %02x %02x %02x %02x\n", decryption_size, device_string[0], device_string[1], device_string[2], device_string[3], device_string[4], device_string[5], device_string[6], device_string[7]);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("Decryption return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    } else {
        LOG_NOTICE("device decrypted size %ld, string %s\n", decryption_size, device_string);
    }

    TChar *licence_string[512] = {0};

    snprintf((TChar *)licence_string, 512, "licenceid=%08x, idext=%08x, maxchannel=%08x, company=%s, project=%s, server=%s, index=%s, expire=%04d-%02d-%02d;[[%s]]", p_content->licence_index, p_content->licence_index_ext, p_content->licence_max_channel, p_content->company_name, p_content->project_name, p_content->server_name, p_content->server_index, p_content->expire_time.year, p_content->expire_time.month, p_content->expire_time.day, device_string);
    test_log("licence_string: %s\n", licence_string);

    TU8 licence_encrypted_string[512 + 32] = {0};
    TMemSize licence_encryption_size = 512 + 32;
    TChar readable_licence_string[1024 + 64] = {0};

    err = encryptor->Encryption((TU8 *)licence_encrypted_string, licence_encryption_size, (TU8 *)licence_string, (TMemSize)strlen((TChar *)licence_string));
    //test_log("[ut flow]: licence_encryption_size %ld\n", licence_encryption_size);

    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("Encryption return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    FILE *licence_file = NULL;

#if 0
    licence_file = fopen(p_content->licence_binary_file_name, "wb");
    if (DLikely(licence_file)) {
        fwrite(licence_encrypted_string, 1, licence_encryption_size, licence_file);
        fclose(licence_file);
        licence_file = NULL;
    }
#endif

    gfEncodingBase16(readable_licence_string, (const TU8 *)licence_encrypted_string, (TInt)licence_encryption_size);
    test_log("[ut flow]: readable_licence_string %s, licence_encryption_size %ld\n", readable_licence_string, licence_encryption_size);
    licence_file = fopen(p_content->licence_text_file_name, "wb");
    if (DLikely(licence_file)) {
        fwrite(readable_licence_string, 2, licence_encryption_size, licence_file);
        fclose(licence_file);
        licence_file = NULL;
    }

    TU8 licence_check_string[512 + 64] = {0};
    TMemSize licence_check_string_size = 512 + 64;

    //LOG_NOTICE("!!!before encryp check string, size %ld, %02x %02x %02x %02x %02x %02x %02x %02x\n", licence_encryption_size, licence_encrypted_string[0], licence_encrypted_string[1], licence_encrypted_string[2], licence_encrypted_string[3], licence_encrypted_string[4], licence_encrypted_string[5], licence_encrypted_string[6], licence_encrypted_string[7]);
    err = encryptor->Encryption((TU8 *)licence_check_string, licence_check_string_size, (TU8 *)licence_encrypted_string, licence_encryption_size);
    //LOG_NOTICE("!!!after encryp check string, size %ld, %02x %02x %02x %02x %02x %02x %02x %02x\n", licence_check_string_size, licence_check_string[0], licence_check_string[1], licence_check_string[2], licence_check_string[3], licence_check_string[4], licence_check_string[5], licence_check_string[6], licence_check_string[7]);
    //test_log("[ut flow]: licence_check_string_size %ld\n", licence_check_string_size);

    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("Encryption return %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    TU8 licence_check_readable_string[1024 + 128] = {0};
    //LOG_NOTICE("!!!gfEncodingBase16, size %ld, %02x %02x %02x %02x %02x %02x %02x %02x\n", licence_check_string_size, licence_check_string[0], licence_check_string[1], licence_check_string[2], licence_check_string[3], licence_check_string[4], licence_check_string[5], licence_check_string[6], licence_check_string[7]);
    gfEncodingBase16((TChar *)licence_check_readable_string, (const TU8 *)licence_check_string, (TInt)licence_check_string_size);
    //LOG_NOTICE("!!!gfEncodingBase16, size %ld, %02x %02x %02x %02x %02x %02x %02x %02x\n", licence_check_string_size, licence_check_readable_string[0], licence_check_readable_string[1], licence_check_readable_string[2], licence_check_readable_string[3], licence_check_readable_string[4], licence_check_readable_string[5], licence_check_readable_string[6], licence_check_readable_string[7]);
    test_log("[ut flow]: licence_check_readable_string %s, licence_check_string_size %ld\n", licence_check_readable_string, licence_check_string_size);
    licence_file = fopen("licence.check", "wb");
    if (DLikely(licence_file)) {
        fwrite("static const TChar* gLicenceCheckString = \"", 1, strlen("static const TChar* gLicenceCheckString = \""), licence_file);
        fwrite(licence_check_readable_string, 2, licence_check_string_size, licence_file);
        fwrite("\";", 1, strlen("\";"), licence_file);
        fclose(licence_file);
        licence_file = NULL;
    }

    licence_file = fopen("licence.check.txt", "wb");
    if (DLikely(licence_file)) {
        fwrite(licence_check_readable_string, 2, licence_check_string_size, licence_file);
        fclose(licence_file);
        licence_file = NULL;
    }

    test_log("[ut flow]: generate_licencefile() end\n");

    return EECode_OK;
}

TInt main(TInt argc, TChar **argv)
{
    TInt ret;
    TUint i = 0;
    EECode err;

    TChar password[64] = {0};

    printf("password:\n");
    scanf("%s", password);
    if ((password[0] == '1') && (password[1] == '2') && (password[2] == '3') && (password[3] == 'q') && (password[4] == 'w') && (password[5] == 'e') && (password[6] == '1') && (password[7] == '2')) {
        if (password[8] == 0x0) {

        } else {
            printf("wrong password\n");
            return 1;
        }
    } else {
        printf("wrong password\n");
        return 1;
    }

    SLicenceGenerator *p_content = (SLicenceGenerator *) malloc(sizeof(SLicenceGenerator));
    if (DUnlikely(!p_content)) {
        test_loge("can not malloc mem(SLicenceGenerator), request size %d\n", sizeof(SLicenceGenerator));
        goto licence_generator_main_exit;
    }
    memset(p_content, 0x0, sizeof(SLicenceGenerator));

    snprintf(p_content->device_id_text_file_name, 128, "deviceid.txt");
    snprintf(p_content->licence_text_file_name, 128, "licence.txt");

    if ((ret = licence_generator_init_params(p_content, argc, argv)) < 0) {
        test_loge("licence_generator: licence_generator_init_params fail, return %d.\n", ret);
        goto licence_generator_main_exit;
    } else if (ret > 0) {
        goto licence_generator_main_exit;
    }

    //p_content->generate_device_id = 1;
    if (p_content->generate_device_id) {
        err = generate_device_id(p_content);
    }

    if (p_content->generate_licence_file) {
        err = generate_licencefile(p_content);
    }

    test_log("[ut flow]: test end\n");

licence_generator_main_exit:

    if (p_content) {
        free(p_content);
        p_content = NULL;
    }

    return 0;
}

