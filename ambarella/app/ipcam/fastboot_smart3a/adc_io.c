/**
 * app/ipcam/fastboot_smart3a/adc_iol.c
 *
 * Author: Caizhang Lin <czlin@ambarella.com>
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <iav_ioctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include "adc_io.h"
#include "adc_util.h"
#include "iav_fastboot.h"

/* external interfaces of adc_util.lib */
#define PRELOAD_PARAMS_IN_AMBOOT_FILE    "/tmp/params_in_amboot.conf"
static int print_amboot_params(struct params_info* params)
{
    FILE *fp = fopen(PRELOAD_PARAMS_IN_AMBOOT_FILE, "wb");
    if (fp) {
        fprintf(fp, "enable_params=%u\n"
                "    stream0_recording_mode=%u\n"
                "    stream0_recording_bitrate=%u\n"
                "    enable_stream0_recording_smartavc=%u\n"
                "    enable_dual_stream=%u\n"
                "    stream1_recording_mode=%u\n"
                "    stream1_recording_bitrate=%u\n"
                "    enable_stream1_recording_smartavc=%u\n"
                "    stream0_streaming_resolution=%u\n"
                "    stream0_streaming_bitrate=%u\n"
                "    enable_stream0_streaming_smartavc=%u\n"
                "    enable_stream0_streaming_svct=%u\n"
                "    rotation_mode=%d\n"
                "### end of enable_params control ####\n"
                "enable_fastosd=%u\n"
                "fastosd_string=%s\n"
                "timezone=%d\n"
                "smart3a_strategy=%d\n"
                "enable_ldc=%d\n"
                "vca_mode=%d\n"
                "vca_frame_num=%d\n",
                params->enable_params,
                params->stream0_recording_mode,
                params->stream0_recording_bitrate,
                params->enable_stream0_recording_smartavc,
                params->enable_dual_stream,
                params->stream1_recording_mode,
                params->stream1_recording_bitrate,
                params->enable_stream1_recording_smartavc,
                params->stream0_streaming_resolution,
                params->stream0_streaming_bitrate,
                params->enable_stream0_streaming_smartavc,
                params->enable_stream0_streaming_svct,
                params->rotation_mode,
                params->enable_fastosd,
                params->fastosd_string,
                params->timezone,
                params->smart3a_strategy,
                params->enable_ldc,
                params->vca_mode,
                params->vca_frame_num);
        fclose(fp);
    } else {
        printf("open %s failed!\n", PRELOAD_PARAMS_IN_AMBOOT_FILE);
        return -1;
    }
    return 0;
}

int adc_io_mem_dump()
{
    char aaa_content[32];
    int fd_iav = open("/dev/iav", O_RDWR, 0);
    if(fd_iav < 0) {
        return -1;
    }
    u8* smart3A_mem;
    struct iav_querybuf querybuf;
    querybuf.buf = IAV_BUFFER_FB_DATA;
check_again:
    if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
        if(errno == EINTR) goto check_again;
        perror("IAV_IOC_QUERY_BUF");
        return -1;
    }
    smart3A_mem = (u8*)mmap(NULL,  querybuf.length, PROT_READ, MAP_SHARED, fd_iav, querybuf.offset);
    if (smart3A_mem == MAP_FAILED) {
        perror("AE AWB mmap failed\n");
        return -1;
    }
    close(fd_iav);

    struct params_info* amboot_params = (struct params_info*) (smart3A_mem + FASTBOOT_USER_DATA_OFFSET);
    print_amboot_params(amboot_params);
    // TODO:why 4 bytes offset
    struct  smart3a_file_info* aaa_param = (struct smart3a_file_info*) (smart3A_mem + FASTBOOT_USER_DATA_OFFSET + sizeof(struct params_info)-4);

    memset(aaa_content, 0, sizeof(aaa_content));
    snprintf(aaa_content, sizeof(aaa_content), "%d,%d\n",
             aaa_param->r_gain,
             aaa_param->b_gain);
    save_content_file(PRELOAD_AWB_FILE, aaa_content);

    memset(aaa_content, 0, sizeof(aaa_content));
    snprintf(aaa_content, sizeof(aaa_content), "%d,%d,%d\n",
             aaa_param->d_gain,
             aaa_param->shutter,
             aaa_param->agc);
    save_content_file(PRELOAD_AE_FILE, aaa_content);

    return 0;
}

int adc_io_read(int verbose, int mode)
{
    int  rval = 0;
    struct params_info amboot_params = {0};
    do {
        rval = adc_util_init(verbose);
        if (rval < 0) {
            printf("adc_util_init failed\n");
            break;
        }
        if (mode & AMBOOT_PARAMETERS) {
            rval = adc_util_get_amboot_params(&amboot_params);
            if (rval < 0) {
                printf("adc_util_get_amboot_params failed\n");
                break;
            }
            if (print_amboot_params(&amboot_params)<0) {
                break;
            }

        }
        if (mode & SMART3A_PARAMETERS) {
            rval = adc_util_read_3a_params(verbose);
            if (rval < 0) {
                break;
            }
        }
    } while (0);

    adc_util_finish();
    return rval;
}

int adc_io_write(int verbose, int mode, struct params_info* params)
{
    int rval = 0;
    do {
        rval = adc_util_init(verbose);
        if (rval < 0) {
            printf("adc_util_init failed\n");
            break;
        }

        if ((mode & AMBOOT_PARAMETERS) && (params != NULL)) {
            //udpate params_in_amboot
            rval = adc_util_set_amboot_params(params);
            if (rval < 0) {
                printf("adc_util_set_amboot_params failed\n");
                break;
            }
        }
        switch(mode) {
            case SMART3A_PARAMETERS:
                rval = adc_util_write(verbose, 0);
                break;
            case AMBOOT_PARAMETERS:
                rval = adc_util_write(verbose, 1);        //skip updating 3a parameters, only update amboot_arams
                break;
            case AMBOOT_PARAMETERS | SMART3A_PARAMETERS:
                rval = adc_util_write(verbose, 0);
                break;
            default:
                printf("Unknown adc write mode.\n");
                rval = -1;
                break;
        }
        if (rval < 0) {
            break;
        }
    } while (0);
    adc_util_finish();
    return rval;
}
