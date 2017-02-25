/*
 * test_cryptochip.c
 *
 * History:
 *	2015/09/22 - [Zhi He] create file for ATSHA204
 *
 * Copyright (C) 2015 Ambarella, Inc.
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>

#include "cryptochip_library_if.h"

#define u_printf printf
#define u_printf_error printf
#define u_printf_debug printf

#define DCC_SLOT_CONFIG1_READ_SLOT_MASK 0x0f
#define DCC_SLOT_CONFIG1_CHECK_ONLY_BIT 0x10
#define DCC_SLOT_CONFIG1_SINGLE_USE_BIT 0x20
#define DCC_SLOT_CONFIG1_ENCRYPT_READ_BIT 0x40
#define DCC_SLOT_CONFIG1_IS_SECRET_BIT 0x80

#define DCC_SLOT_CONFIG2_WRITE_SLOT_MASK 0x0f
#define DCC_SLOT_CONFIG2_ENCRYPT_WRITE_BIT 0x40
#define DCC_SLOT_CONFIG2_NON_WRITE_FLAG 0x80

typedef struct {
    unsigned char b_show_serial_number;
    unsigned char b_show_config;
    unsigned char b_show_otp;
    unsigned char b_initialize_config_zone;

    unsigned char b_initialize_otp_and_data_zone;
    unsigned char reserved_0;
    unsigned char reserved_1;
    unsigned char reserved_2;

    unsigned char b_test_en_write;
    unsigned char test_en_write_slot;
    unsigned char b_test_en_read;
    unsigned char test_en_read_slot;

    unsigned char test_generate_hwsignature;
    unsigned char genhwsign_storage_slot;
    unsigned char test_verify_hwsignature;
    unsigned char verifyhwsign_storage_slot;

    void * p_chip;
} test_cryptochip_context;

static void __print_textable(const char* p, int len)
{
    char* p_buf = (char*) malloc(len + 8);
    snprintf(p_buf, len, "%s", p);
    u_printf("%s\n", p_buf);
}

static void __print_memory_8align_hex(const unsigned char* p, int len)
{
    int i;

    while (len > 8) {
        u_printf(" 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
        p += 8;
        len -= 8;
    }

    for (i = 0; i < len; i++) {
        u_printf(" 0x%02X", p[i]);
    }

    u_printf("\n");
}

static void __print_memory_hex(const unsigned char* data, const int len)
{
    int i;

    for (i = 0; i < len; i++) {
        if (i > 0) {
            u_printf(" ");
        }
        u_printf("0x%02X", data[i]);
    }

    u_printf("\n");
}

static void __print_ut_options()
{
    u_printf("test_cryptochip options:\n");
    u_printf("\t'--showsn': will show serial number\n");
    u_printf("\t'--showconfig': will show config zone, 88 bytes\n");
    u_printf("\t'--showotp': will show one time programmable data, 64 bytes\n");

    u_printf("\t'--initconfig': will write and lock (initialize) config zone\n");
    u_printf("\t'--initotpdata': will write and lock (initialize) otp and data zone\n");

    u_printf("\t'--testenwrite %%d': will write a data slot. (encrypt write)\n");
    u_printf("\t'--testenread %%d': will read a data slot. (encrypt read)\n");

    u_printf("\t'--testgensign %%d': will generate a fake digest (fake_digest) and store its hw signature\n");
    u_printf("\t'--testverifysign %%d': will verify the fake digest (fake_digest) and its hw signature\n");

    u_printf("\t'--help': print help\n\n");
    u_printf("\t setup cryptochip:\n");
    u_printf("\t\t modprobe atsha204\n");
    u_printf("\t\t echo atsha204 0x64 | tee /sys/class/i2c-dev/i2c-0/device/new_device\n");
}

static int __init_test_cryptochip_params(int argc, char **argv, test_cryptochip_context* context)
{
    int i = 0;

    for (i = 1; i < argc; i++) {
        if (!strcmp("--showsn", argv[i])) {
            context->b_show_serial_number = 1;
            u_printf("[input argument] --showsn, show serial number.\n");
        } else if (!strcmp("--showconfig", argv[i])) {
            context->b_show_config = 1;
            u_printf("[input argument] --showconfig, show config zone.\n");
        } else if (!strcmp("--showotp", argv[i])) {
            context->b_show_otp = 1;
            u_printf("[input argument] --showotp, show otp zone.\n");
        } else if (!strcmp("--initconfig", argv[i])) {
            context->b_initialize_config_zone = 1;
            u_printf("[input argument] --initconfig, write and lock config zone.\n");
        } else if (!strcmp("--initotpdata", argv[i])) {
            context->b_initialize_otp_and_data_zone = 1;
            u_printf("[input argument] --initotpdata, write and lock otp and data zone.\n");
        } else if (!strcmp("--testenwrite", argv[i])) {
            int slot = 0;
            if (argc > (i + 1)) {
                if (1 == sscanf(argv[i + 1], "%d", &slot)) {
                    if (16 > slot) {
                        context->b_test_en_write = 1;
                        context->test_en_write_slot = slot;
                        u_printf("[input argument] --testenwrite, slot %d\n", context->test_en_write_slot);
                    } else {
                        u_printf_error("bad slot id %d\n", slot);
                        return (-1);
                    }
                } else {
                    u_printf_error(" --testenwrite should follow slot id \n");
                    return (-1);
                }
            } else {
                u_printf_error(" --testenwrite should follow slot id\n");
                return (-1);
            }
            i ++ ;
        } else if (!strcmp("--testenread", argv[i])) {
            int slot = 0;
            if (argc > (i + 1)) {
                if (1 == sscanf(argv[i + 1], "%d", &slot)) {
                    if (16 > slot) {
                        context->b_test_en_read = 1;
                        context->test_en_read_slot = slot;
                        u_printf("[input argument] --testenread, slot %d\n", context->test_en_read_slot);
                    } else {
                        u_printf_error("bad slot id %d\n", slot);
                        return (-1);
                    }
                } else {
                    u_printf_error(" --testenread should follow slot id\n");
                    return (-1);
                }
            } else {
                u_printf_error(" --testenread should follow slot id\n");
                return (-1);
            }
            i ++ ;
        } else if (!strcmp("--testgensign", argv[i])) {
            int slot = 0;
            if (argc > (i + 1)) {
                if (1 == sscanf(argv[i + 1], "%d", &slot)) {
                    if (16 > slot) {
                        context->test_generate_hwsignature = 1;
                        context->genhwsign_storage_slot = slot;
                        u_printf("[input argument] --testgensign, slot %d\n", context->genhwsign_storage_slot);
                    } else {
                        u_printf_error("bad slot id %d\n", slot);
                        return (-1);
                    }
                } else {
                    u_printf_error(" --testgensign should follow slot id\n");
                    return (-1);
                }
            } else {
                u_printf_error(" --testgensign should follow slot id\n");
                return (-1);
            }
            i ++ ;
        } else if (!strcmp("--testverifysign", argv[i])) {
            int slot = 0;
            if (argc > (i + 1)) {
                if (1 == sscanf(argv[i + 1], "%d", &slot)) {
                    if (16 > slot) {
                        context->test_verify_hwsignature = 1;
                        context->verifyhwsign_storage_slot = slot;
                        u_printf("[input argument] --testverifysign, slot %d\n", context->verifyhwsign_storage_slot);
                    } else {
                        u_printf_error("bad slot id %d\n", slot);
                        return (-1);
                    }
                } else {
                    u_printf_error(" --testverifysign should follow slot id\n");
                    return (-1);
                }
            } else {
                u_printf_error(" --testverifysign should follow slot id\n");
                return (-1);
            }
            i ++ ;
        } else {
            u_printf_error("error: NOT processed option(%s).\n", argv[i]);
            __print_ut_options();
            return (-1);
        }
    }

    return 0;
}


#define DCIRCULAR_SHIFT_8(x, s) ((x >> s) | (x << (8 - s)))

void __scamble_sequence(unsigned char *p, unsigned int len)
{
    unsigned int i = 0;
    unsigned char r[8] = {17, 37, 5, 11, 119, 29, 163, 101};
    unsigned char v[8] = {59, 211, 73, 7, 127, 19, 23, 73};
    unsigned char* pp = NULL;

    unsigned char t1, t2;

    if (7 < len) {
        for (i = 0; i < 8; i ++) {
            t1 = (v[i] ^ p[i]);
            t2 = (v[i] & 0x7);
            v[i] = DCIRCULAR_SHIFT_8(t1, t2) ^ r[i];
        }
    } else {
        for (i = 0; i < len; i ++) {
            t1 = (v[i] ^ p[i]);
            t2 = (v[i] & 0x7);
            v[i] = DCIRCULAR_SHIFT_8(t1, t2) ^ r[i];
        }

        for (; i < 8; i ++) {
            t1 = v[i];
            t2 = (v[i] & 0x7);
            v[i] = DCIRCULAR_SHIFT_8(t1, t2) ^ r[i];
        }
    }

    t2 = (v[7] & 0x7);
    i = 0;
    pp = p;
    while (len > 8) {
        pp[0] = (((v[0] ^ v[1]) & (r[4] + r[5])) | DCIRCULAR_SHIFT_8(pp[0], t2)) + 19;
        t2 = pp[0] & 0x7;
        pp[1] = (((v[1] & v[2]) ^ (r[5] | r[6])) ^ DCIRCULAR_SHIFT_8(pp[1], t2)) + 47;
        t2 = pp[1] & 0x7;
        pp[2] = (((v[2] | v[3]) + (r[6] & r[7])) ^ DCIRCULAR_SHIFT_8(pp[2], t2)) + 113;
        t2 = pp[2] & 0x7;
        pp[3] = (((v[3] + v[4]) | (r[7] ^ r[0])) ^ DCIRCULAR_SHIFT_8(pp[3], t2)) + 17;
        t2 = pp[3] & 0x7;
        pp[4] = (((v[4] & v[5]) ^ (r[0] | r[1])) | DCIRCULAR_SHIFT_8(pp[4], t2)) + 101;
        t2 = pp[4] & 0x7;
        pp[5] = (((v[5] ^ v[6]) | (r[1] + r[2])) ^ DCIRCULAR_SHIFT_8(pp[5], t2)) + 157;
        t2 = pp[5] & 0x7;
        pp[6] = (((v[6] + v[7]) | (r[2] ^ r[3])) + DCIRCULAR_SHIFT_8(pp[6], t2)) + 31;
        t2 = pp[6] & 0x7;
        pp[7] = (((v[7] | v[0]) + (r[3] & r[4])) ^ DCIRCULAR_SHIFT_8(pp[7], t2)) + 237;
        t2 = pp[7] & 0x7;

        v[0] = (v[1] ^ r[6]) + 197 + i + pp[7];
        r[0] = (v[6] & r[1]) + 149 + i + pp[6];
        v[1] = ((v[2] | r[5]) ^ 109) + i + pp[5];
        r[1] = ((v[5] + r[2]) & 83) + i + pp[4];

        v[2] = (v[3] ^ r[0]) + 179 + i + pp[3];
        r[2] = (v[0] & r[3]) + 67 + i + pp[2];
        v[3] = ((v[4] | r[7]) ^ 71) + i + pp[1];
        r[3] = ((v[7] + r[4]) & 37) + i + pp[0];

        v[4] = (v[2] ^ r[6]) + 59 + i + pp[7];
        r[4] = (v[6] & r[2]) + 131 + i + pp[5];
        v[5] = ((v[1] | r[7]) ^ 71) + i + pp[3];
        r[5] = ((v[7] + r[1]) & 29) + i + pp[1];

        v[6] = (v[5] ^ r[3]) + 119 + i + pp[0];
        r[6] = (v[3] & r[5]) + 151 + i + pp[2];
        v[7] = ((v[4] | r[2]) ^ 61) + i + pp[4];
        r[7] = ((v[2] + r[4]) & 91) + i + pp[6];

        len -= 8;
        pp += 8;
        i ++;
    }

    i = 0;
    for (i = 0; i < len; i ++) {
        pp[i] = pp[i] ^ (((v[i] + r[(8 - i) & 0x7]) & (v[(8 - i) & 0x7] | r[i])) + 97);
    }

    return;
}

static int __write_binary_file(const char *filename, unsigned char *pdata, int len)
{
    FILE* file = fopen(filename, "wb");
    if (file) {
        fwrite(pdata, 1, len, file);
        fclose(file);
        file = NULL;
    } else {
        u_printf_error("write file (%s) fail\n", filename);
        return (-1);
    }

    return 0;
}

static int __read_binary_file(const char *filename, unsigned char *pdata, int *len)
{
    FILE* file = fopen(filename, "rb");
    if (file) {
        int file_len = 0;

        fseek(file, 0L, SEEK_END);
        file_len = ftell(file);
        fseek(file, 0L, SEEK_SET);

        if (file_len < (*len)) {
            *len = file_len;
            u_printf("[warning]: file too short\n");
        } else if (file_len > (*len)) {
            u_printf("[warning]: file too long\n");
        }

        fread(pdata, 1, *len, file);
        fclose(file);
        file = NULL;
    } else {
        u_printf_error("read file (%s) fail\n", filename);
        return (-1);
    }

    return 0;
}

static void __show_slot_config(unsigned char slot, unsigned char config1, unsigned char config2)
{
    u_printf("\tslot %d: config 1 %02x, config 2 %02x, read key slot %d, write key slot %d\n", slot, config1, config2, (config1 & DCC_SLOT_CONFIG1_READ_SLOT_MASK), (config2 & DCC_SLOT_CONFIG2_WRITE_SLOT_MASK));

    if (config1 & DCC_SLOT_CONFIG1_CHECK_ONLY_BIT) {
        u_printf("\t\tcheck only = 1\n");
    } else {
        u_printf("\t\tcheck only = 0\n");
    }

    if (config1 & DCC_SLOT_CONFIG1_SINGLE_USE_BIT) {
        u_printf("\t\tsingle use = 1\n");
    } else {
        u_printf("\t\tsingle use = 0\n");
    }

    if (config1 & DCC_SLOT_CONFIG1_ENCRYPT_READ_BIT) {
        u_printf("\t\tencrypt read = 1\n");
    } else {
        u_printf("\t\tencrypt read = 0\n");
    }

    if (config1 & DCC_SLOT_CONFIG1_IS_SECRET_BIT) {
        u_printf("\t\tis secret = 1\n");
    } else {
        u_printf("\t\tis secret = 0\n");
    }

    if (config2 & DCC_SLOT_CONFIG2_ENCRYPT_WRITE_BIT) {
        u_printf("\t\tencrypt write = 1\n");
    } else if ((config2 & 0xc0) == DCC_SLOT_CONFIG2_NON_WRITE_FLAG) {
        u_printf("\t\tnot writable = 1\n");
    } else {
        u_printf("\t\tplaintext write = 1\n");
    }

}

static void __show_config_zone_configs(unsigned char* pp)
{
    unsigned char* p = pp + 16;
    unsigned char i;

    if (0x55 == pp[87]) {
        u_printf("config zone is not locked\n");
    } else {
        u_printf("config zone is locked\n");
    }

    if (0x55 == pp[86]) {
        u_printf("otp/data zone is not locked\n");
    } else {
        u_printf("otp/data zone is locked\n");
    }

    u_printf("i2c addr %02x, check mac source %02x, otp mode %02x, selector mode %02x\n", p[0], p[1], p[2], p[3]);

    p += 4;
    for (i = 0; i < 16; i ++) {
        __show_slot_config(i, p[2 * i], p[2 * i + 1]);
    }
}

int main(int argc, char** argv)
{
    int ret = 0;
    test_cryptochip_context context;
    sf_cryptochip_interface chip_interface;
    unsigned int b_config_zone_locked = 0, b_otp_data_zone_locked = 0;

    if (2 > argc) {
        __print_ut_options();
        return 0;
    }

    memset(&context, 0x0, sizeof(context));
    chip_interface.f_create_handle = NULL;
    chip_interface.f_destroy_handle = NULL;

    ret = __init_test_cryptochip_params(argc, argv, &context);
    if (ret) {
        if (0 > ret) {
            u_printf_error("__init_test_cryptochip_params fail\n");
            goto __fail_exit;
        }
    }

    ret = get_cryptochip_library_interface(&chip_interface, ECRYPTOCHIP_TYPE_ATMEL_ATSHA204);
    if (ret) {
        if (0 > ret) {
            u_printf_error("get_cryptochip_library_interface fail, ret %d\n", ret);
            goto __fail_exit;
        }
    }

    if (chip_interface.f_create_handle) {
        context.p_chip = chip_interface.f_create_handle();
        if (!context.p_chip) {
            u_printf_error("f_create_handle() fail\n");
            ret = (-10);
            goto __fail_exit;
        }
    } else {
        u_printf_error("NULL f_create_handle()\n");
        ret = (-11);
        goto __fail_exit;
    }

    if (context.b_show_serial_number) {
        if (chip_interface.f_query_serial_number) {
            unsigned int serial_number_len = 0;
            unsigned char * p_serial_number = chip_interface.f_query_serial_number(context.p_chip, &serial_number_len);
            if (p_serial_number && serial_number_len) {
                u_printf("show serial number, len %d\n", serial_number_len);
                __print_memory_hex(p_serial_number, 9);
            } else {
                u_printf_error("f_query_serial_number() fail\n");
                ret = (-10);
                goto __fail_exit;
            }
        } else {
            u_printf_error("NULL f_query_serial_number()\n");
            ret = (-11);
            goto __fail_exit;
        }
    }

    if (chip_interface.f_query_chip_status) {
        ret = chip_interface.f_query_chip_status(context.p_chip, &b_config_zone_locked, &b_otp_data_zone_locked);
        if (ret) {
            u_printf_error("f_query_chip_status() fail, ret %d\n", ret);
            goto __fail_exit;
        }

        if (!b_config_zone_locked) {
            u_printf("warning: chip's config zone is not initialized(locked), you should initialize config zone first\n");
        }

        if (!b_otp_data_zone_locked) {
            u_printf("warning: chip's otp and data zone is not initialized(locked), you should initialize otp and data zone first\n");
        }
    } else {
        u_printf_error("NULL f_query_chip_status()\n");
        ret = (-11);
        goto __fail_exit;
    }

    if (context.b_initialize_config_zone) {
        if (chip_interface.f_initialize_config_zone) {
            ret = chip_interface.f_initialize_config_zone(context.p_chip);
            if (!ret) {
                u_printf("initialize (write and lock) config zone done\n");
            } else {
                u_printf_error("initialize (write and lock) config zone fail, ret %d\n", ret);
                goto __fail_exit;
            }
        } else {
            u_printf_error("NULL f_initialize_config_zone()\n");
            ret = (-11);
            goto __fail_exit;
        }
    }

    if (context.b_initialize_otp_and_data_zone) {
        if (chip_interface.f_initialize_otp_data_zone) {
            ret = chip_interface.f_initialize_otp_data_zone(context.p_chip);
            if (!ret) {
                u_printf("initialize (write and lock) otp and data zone done\n");
            } else {
                u_printf_error("initialize (write and lock) otp and data zone fail, ret %d\n", ret);
                goto __fail_exit;
            }
        } else {
            u_printf_error("NULL f_initialize_otp_data_zone()\n");
            ret = (-11);
            goto __fail_exit;
        }
    }

    if (context.b_show_config) {
        if (chip_interface.f_query_config_zone) {
            unsigned char config_zone[88] = {0};
            unsigned int config_zone_len = 0;
            ret = chip_interface.f_query_config_zone(context.p_chip, config_zone, &config_zone_len);
            if (!ret) {
                u_printf("show config zone\n");
                __print_memory_8align_hex(config_zone, 88);
                __show_config_zone_configs(config_zone);
            } else {
                u_printf_error("f_query_config_zone() fail, %d\n", ret);
                goto __fail_exit;
            }
        } else {
            u_printf_error("NULL f_query_config_zone()\n");
            ret = (-11);
            goto __fail_exit;
        }
    }

    if (context.b_show_otp) {
        if (chip_interface.f_query_otp_zone) {
            unsigned char otp_zone[64] = {0};
            ret = chip_interface.f_query_otp_zone(context.p_chip, otp_zone, 64);
            if (!ret) {
                u_printf("show otp zone, hex:\n");
                __print_memory_8align_hex(otp_zone, 64);
                u_printf("show otp zone, textable:\n");
                __print_textable((const char *)otp_zone, 64);
            } else {
                u_printf_error("f_query_otp_zone() fail, %d\n", ret);
                goto __fail_exit;
            }
        } else {
            u_printf_error("NULL f_query_otp_zone()\n");
            ret = (-11);
            goto __fail_exit;
        }
    }

    if (context.b_test_en_write) {
        if (chip_interface.f_encrypt_write) {
            unsigned char ran_data[32];
            struct timeval tv;

            gettimeofday(&tv, NULL);
            memcpy(ran_data, &tv, sizeof(tv));
            __scamble_sequence(ran_data, 32);
            u_printf("encrypt write data to slot %d\n", context.test_en_write_slot);
            __print_memory_8align_hex(ran_data, 32);

            ret = chip_interface.f_encrypt_write(context.p_chip, ran_data, context.test_en_write_slot);
            if (!ret) {
                u_printf("encrypt write done\n");
            } else {
                u_printf_error("f_encrypt_write() fail, ret %d\n", ret);
                goto __fail_exit;
            }
        } else {
            u_printf_error("NULL f_encrypt_write()\n");
            ret = (-11);
            goto __fail_exit;
        }
    }

    if (context.b_test_en_read) {
        if (chip_interface.f_encrypt_read) {
            unsigned char read_data[32] = {0};
            ret = chip_interface.f_encrypt_read(context.p_chip, read_data, context.test_en_read_slot);
            if (!ret) {
		u_printf("encrypt read data from slot %d\n", context.test_en_read_slot);
		__print_memory_8align_hex(read_data, 32);
            } else {
                u_printf_error("f_encrypt_read() fail, ret %d\n", ret);
                goto __fail_exit;
            }
        } else {
            u_printf_error("NULL f_encrypt_read()\n");
            ret = (-11);
            goto __fail_exit;
        }
    }

    if (context.test_generate_hwsignature) {
        if (chip_interface.f_gen_hwsign) {
            unsigned char fake_digest[32];
            struct timeval tv;

            gettimeofday(&tv, NULL);
            memcpy(fake_digest, &tv, sizeof(tv));
            __scamble_sequence(fake_digest, 32);
            u_printf("generate fake digest\n");
            __print_memory_8align_hex(fake_digest, 32);

            ret = __write_binary_file("fake_digest", fake_digest, 32);
            if (ret) {
                u_printf_error("write binary file fail\n");
                goto __fail_exit;
            }

            ret = chip_interface.f_gen_hwsign(context.p_chip, fake_digest, context.genhwsign_storage_slot);
            if (!ret) {
                u_printf("generate hw signature done\n");
            } else {
                u_printf_error("generate hw signature fail, ret %d\n", ret);
                goto __fail_exit;
            }

        } else {
            u_printf_error("NULL f_gen_hwsign()\n");
            ret = (-11);
            goto __fail_exit;
        }
    }

    if (context.test_verify_hwsignature) {
        if (chip_interface.f_verify_hwsign) {
            unsigned char fake_digest[32];
            int len = 32;

            ret = __read_binary_file("fake_digest", fake_digest, &len);
            if (ret) {
                u_printf_error("read binary file fail\n");
                goto __fail_exit;
            }

            ret = chip_interface.f_verify_hwsign(context.p_chip, fake_digest, context.verifyhwsign_storage_slot);
            if (!ret) {
                u_printf("verify hw signature done\n");
            } else {
                u_printf_error("verify hw signature fail, ret %d\n", ret);
                goto __fail_exit;
            }

        } else {
            u_printf_error("NULL f_verify_hwsign()\n");
            ret = (-11);
            goto __fail_exit;
        }
    }

__fail_exit:
    if (context.p_chip && chip_interface.f_destroy_handle) {
        chip_interface.f_destroy_handle(context.p_chip);
        context.p_chip = NULL;
    }

    return ret;
}

