/*******************************************************************************
 * mtest2.cpp
 *
 * History:
 *    2014/02/19 - [Zhi He] create file
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

#define DAudioAdpcmTestStringMaxLength 256

typedef struct {
    TChar input_filename[DAudioAdpcmTestStringMaxLength];
    TChar output_filename[DAudioAdpcmTestStringMaxLength];

    TChar compare_filename_1[DAudioAdpcmTestStringMaxLength];
    TChar compare_filename_2[DAudioAdpcmTestStringMaxLength];

    TU8 b_printraw;
    TU8 b_printad;
    TU8 b_readfile;
    TU8 b_writefile;

    TU8 b_printdiff;
    TU8 encoding_mode;
    TU8 decoding_mode;
    TU8 reserved2;
} SAudioAdpcmTestContext;

void _audio_adpcm_test_print_version()
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

void _audio_adpcm_test_print_usage()
{
    printf("\naudio_adpcm_test options:\n");
    printf("\t'-i [input_filename]': specify input filename\n");
    printf("\t'-o [output_filename]': specify output filename\n");
    printf("\t'-c1 [compare_filename 1]': specify compare filename 1\n");
    printf("\t'-c2 [compare_filename 2]': specify compare filename 2\n");
    printf("\t'--encoding': test encoding\n");
    printf("\t'--decoding': test decoding\n");
    printf("\t'--printraw': print raw sample\n");
    printf("\t'--notprintraw': not print raw sample\n");
    printf("\t'--printad': print ad sample\n");
    printf("\t'--notprintad': not print ad sample\n");
    printf("\t'--binarydiff': print diff\n");
    printf("\t'--notbinarydiff': not print diff\n");
    printf("\t'--save': save output\n");
    printf("\t'--notsave': not save output\n");
}

void _audio_adpcm_test_print_runtime_usage()
{
}


TInt audio_adpcm_test_init_params(SAudioAdpcmTestContext *p_content, TInt argc, TChar **argv)
{
    TInt i = 0;
    TInt ret = 0;

    if (argc < 2) {
        _audio_adpcm_test_print_version();
        _audio_adpcm_test_print_usage();
        _audio_adpcm_test_print_runtime_usage();
        return 1;
    }

    DASSERT(p_content);

    //parse options
    for (i = 1; i < argc; i++) {
        if (!strcmp("-f", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(&p_content->input_filename[0], DAudioAdpcmTestStringMaxLength, "%s", argv[i + 1]);
            } else {
                test_loge("[input argument error]: '-f', should follow with input file name (%%s), argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("-o", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(&p_content->output_filename[0], DAudioAdpcmTestStringMaxLength, "%s", argv[i + 1]);
            } else {
                test_loge("[input argument error]: '-o', should follow with output file name (%%s), argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("-c1", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(&p_content->compare_filename_1[0], DAudioAdpcmTestStringMaxLength, "%s", argv[i + 1]);
            } else {
                test_loge("[input argument error]: '-c1', should follow with compare(1) file name (%%s), argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("-c2", argv[i])) {
            if ((i + 1) < argc) {
                snprintf(&p_content->compare_filename_2[0], DAudioAdpcmTestStringMaxLength, "%s", argv[i + 1]);
            } else {
                test_loge("[input argument error]: '-c2', should follow with compare(2) file name (%%s), argc %d, i %d.\n", argc, i);
                return (-1);
            }
            i ++;
        } else if (!strcmp("--encoding", argv[i])) {
            p_content->encoding_mode = 1;
        } else if (!strcmp("--decoding", argv[i])) {
            p_content->decoding_mode = 1;
        } else if (!strcmp("--printraw", argv[i])) {
            p_content->b_printraw = 1;
        } else if (!strcmp("--notprintraw", argv[i])) {
            p_content->b_printraw = 0;
        } else if (!strcmp("--printad", argv[i])) {
            p_content->b_printad = 1;
        } else if (!strcmp("--notprintad", argv[i])) {
            p_content->b_printad = 0;
        } else if (!strcmp("--printdiff", argv[i])) {
            p_content->b_printdiff = 1;
        } else if (!strcmp("--notprintdiff", argv[i])) {
            p_content->b_printdiff = 0;
        } else if (!strcmp("--save", argv[i])) {
            p_content->b_writefile = 1;
        } else if (!strcmp("--notsave", argv[i])) {
            p_content->b_writefile = 0;
        } else {
            test_loge("unknown option[%d]: %s\n", i, argv[i]);
            return (-255);
        }
    }

    return 0;
}

void calculate_diff(TU16 *raw, TU16 *diff, TU32 number)
{
    TU32 i = 0;

    DASSERT(raw);
    DASSERT(diff);

    diff[0] = raw[0];
    for (i = 1; i < number; i ++) {
        if (raw[i] > raw[i - 1]) {
            diff[i] = raw[i] - raw[i - 1];
        } else {
            diff[i] = raw[i - 1] - raw[i];
        }
    }

}

TInt audio_adpcm_test(SAudioAdpcmTestContext *p_context)
{
    TU32 index = 0, max_loop_number = 0;
    TU32 i = 0;

    TU16 read_buf[1024] = {0};
    TU16 write_buf[1024] = {0};

    TU16 *p_raw = NULL, *p_ad = NULL;

    TU16 diff_buf[1024] = {0};

    FILE *input_file = NULL;
    FILE *output_file = NULL;

    TU32 total_sample_number = 0;
    TU32 read_index = 0;
    TU32 write_index = 0;

    TU64 total_noise = 0;

    test_log("[ut flow]: audio_adpcm_test start\n");

    input_file = fopen(p_context->input_filename, "rb");
    if (!input_file) {
        test_loge("not valid input_filename %s\n", p_context->input_filename);
        return (-1);
    }
    fseek(input_file, 0L, SEEK_END);
    total_sample_number = ftell(input_file);
    fseek(input_file, 0L, SEEK_SET);
    total_sample_number = total_sample_number / 2;

    if (p_context->b_writefile) {
        output_file = fopen(p_context->output_filename, "wb");
        if (!output_file) {
            test_loge("not valid output_filename %s\n", p_context->output_filename);
            return (-2);
        }
    }

    max_loop_number = total_sample_number / 1024;

    test_log("[ut flow]: audio_adpcm_test before loop, total sample number %d, loop number %d\n", total_sample_number, max_loop_number);
    for (index = 0; index < max_loop_number; index ++) {
        fread(read_buf, 2, 1024, input_file);

        calculate_diff(read_buf, diff_buf, 1024);

        p_raw = read_buf;
        p_ad = diff_buf;
        for (i = 0; i < (1024 / 16); i ++, p_raw += 16, p_ad += 16) {
            test_log("[ut flow]: audio_adpcm_test in loop, i %d, index %d, max_loop_number %d\n", i, index, max_loop_number);
            if (p_context->b_printraw) {
                test_log("\traw:\t%04x %04x %04x %04x, %04x %04x %04x %04x, %04x %04x %04x %04x, %04x %04x %04x %04x\n",
                         p_raw[0], p_raw[1], p_raw[2], p_raw[3], \
                         p_raw[4], p_raw[5], p_raw[6], p_raw[7], \
                         p_raw[8], p_raw[9], p_raw[10], p_raw[11], \
                         p_raw[12], p_raw[13], p_raw[14], p_raw[15]);
            }

            if (p_context->b_printad) {
                test_log("\tad:\t%04x %04x %04x %04x, %04x %04x %04x %04x, %04x %04x %04x %04x, %04x %04x %04x %04x\n",
                         p_ad[0], p_ad[1], p_ad[2], p_ad[3], \
                         p_ad[4], p_ad[5], p_ad[6], p_ad[7], \
                         p_ad[8], p_ad[9], p_ad[10], p_ad[11], \
                         p_ad[12], p_ad[13], p_ad[14], p_ad[15]);
            }
        }
    }

    return 0;
}

TInt audio_adpcm_test_encoding(SAudioAdpcmTestContext *p_context)
{
    TU32 index = 0, max_loop_number = 0;
    TU32 i = 0;

    TU16 read_buf[1024] = {0};
    TU16 write_buf[1024] = {0};

    TU16 *p_raw = NULL, *p_ad = NULL;

    TU16 diff_buf[1024] = {0};

    FILE *input_file = NULL;
    FILE *output_file = NULL;

    TU32 total_sample_number = 0;
    TU32 read_index = 0;
    TU32 write_index = 0;

    TMemSize out_size = 0;

    ICustomizedCodec *codec = gfCustomizedCodecFactory((TChar *)"customized_adpcm", 0);
    if (DUnlikely(NULL == codec)) {
        test_loge("gfCustomizedCodecFactory fail\n");
        return (-8);
    }

    test_log("[ut flow]: audio_adpcm_test_encoding start\n");

    input_file = fopen(p_context->input_filename, "rb");
    if (!input_file) {
        test_loge("not valid input_filename %s\n", p_context->input_filename);
        return (-1);
    }
    fseek(input_file, 0L, SEEK_END);
    total_sample_number = ftell(input_file);
    fseek(input_file, 0L, SEEK_SET);
    total_sample_number = total_sample_number / 2;

    if (p_context->b_writefile) {
        output_file = fopen(p_context->output_filename, "wb");
        if (!output_file) {
            test_loge("not valid output_filename %s\n", p_context->output_filename);
            fclose(input_file);
            return (-2);
        }
    }

    max_loop_number = total_sample_number / 1024;

    test_log("[ut flow]: audio_adpcm_test_encoding before loop, total sample number %d, loop number %d\n", total_sample_number, max_loop_number);
    for (index = 0; index < max_loop_number; index ++) {
        fread(read_buf, 2, 1024, input_file);

        codec->Encoding((void *)read_buf, (void *)write_buf, 2048, out_size);
        DASSERT(516 == out_size);
        if (output_file) {
            fwrite(write_buf, 1, out_size, output_file);
        }
    }

    fclose(input_file);
    if (output_file) {
        fclose(output_file);
    }

    codec->Destroy();
    return 0;
}

TInt audio_adpcm_test_decoding(SAudioAdpcmTestContext *p_context)
{
    TU32 index = 0, max_loop_number = 0;
    TU32 i = 0;

    TU16 read_buf[1024] = {0};
    TU16 write_buf[1024] = {0};

    TU16 *p_raw = NULL, *p_ad = NULL;

    TU16 diff_buf[1024] = {0};

    FILE *input_file = NULL;
    FILE *output_file = NULL;

    TU32 total_sample_number = 0;
    TU32 read_index = 0;
    TU32 write_index = 0;

    TMemSize out_size = 0;

    ICustomizedCodec *codec = gfCustomizedCodecFactory((TChar *)"customized_adpcm", 0);
    if (DUnlikely(NULL == codec)) {
        test_loge("gfCustomizedCodecFactory fail\n");
        return (-8);
    }

    test_log("[ut flow]: audio_adpcm_test_decoding start\n");

    input_file = fopen(p_context->input_filename, "rb");
    if (!input_file) {
        test_loge("not valid input_filename %s\n", p_context->input_filename);
        return (-1);
    }
    fseek(input_file, 0L, SEEK_END);
    total_sample_number = ftell(input_file);
    fseek(input_file, 0L, SEEK_SET);

    if (p_context->b_writefile) {
        output_file = fopen(p_context->output_filename, "wb");
        if (!output_file) {
            test_loge("not valid output_filename %s\n", p_context->output_filename);
            fclose(input_file);
            return (-2);
        }
    }

    max_loop_number = total_sample_number / 516;

    test_log("[ut flow]: audio_adpcm_test_decoding before loop, total sample number %d, loop number %d\n", total_sample_number, max_loop_number);
    for (index = 0; index < max_loop_number; index ++) {
        fread(read_buf, 1, 516, input_file);

        codec->Decoding((void *)read_buf, (void *)write_buf, 516, out_size);
        DASSERT(2048 == out_size);
        if (output_file) {
            fwrite(write_buf, 1, out_size, output_file);
        }
    }

    fclose(input_file);
    if (output_file) {
        fclose(output_file);
    }

    codec->Destroy();
    return 0;
}

TInt main(TInt argc, TChar **argv)
{
    TInt ret = 0;
    SAudioAdpcmTestContext *p_content = (SAudioAdpcmTestContext *) malloc(sizeof(SAudioAdpcmTestContext));
    memset(p_content, 0x0, sizeof(SAudioAdpcmTestContext));

    test_log("[ut flow]: test start\n");

    if ((ret = audio_adpcm_test_init_params(p_content, argc, argv)) < 0) {
        test_loge("[ut flow]: audio_adpcm_test_init_params fail, return %d.\n", ret);
        goto audio_adpcm_test_main_exit;
    } else if (ret > 0) {
        goto audio_adpcm_test_main_exit;
    }

    if (p_content->encoding_mode) {
        audio_adpcm_test_encoding(p_content);
    } else if (p_content->decoding_mode) {
        audio_adpcm_test_decoding(p_content);
    } else {
        audio_adpcm_test(p_content);
    }

    test_log("[ut flow]: test end\n");

audio_adpcm_test_main_exit:

    if (p_content) {
        free(p_content);
        p_content = NULL;
    }

    return 0;
}

