/*******************************************************************************
 * test_directstream.c
 * the program can read bitstream from BSB buffer and streaming to local network
 *
 * History:
 *  2015/03/11 - [Zhi He] create file for direct streaming
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <signal.h>
#include <basetypes.h>
#include <iav_ioctl.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "directsharing_if.h"

using namespace mw_cg;

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg) \
    do {                        \
        if (ioctl(_filp, _cmd, _arg) < 0) { \
            perror(#_cmd);      \
            return -1;          \
        }                       \
    } while (0)
#endif

// the device file handle
static int fd_iav;

// the bitstream buffer
static u8 *bsb_mem;
static u32 bsb_size;

static int running = 1;

static unsigned int current_stream = -1;
static unsigned int current_resolution = 1; // 1:4k, 0: 1080p
static unsigned int current_format = 1; // 1: h265, 0: h264

static char local_file_name[256] = {0};
static FILE *local_file_handle = NULL;

// options and usage
#define NO_ARG      0
#define HAS_ARG     1

static struct option long_options[] = {
    {"filename",    HAS_ARG,    0,  'f'},
    {"streamA", NO_ARG,     0,  'A'},
    {"streamB", NO_ARG,     0,  'B'},
    {"streamC", NO_ARG,     0,  'C'},
    {"streamD", NO_ARG,     0,  'D'},
    {"4k",  NO_ARG,     0,  '4'},
    {"2k",  NO_ARG,     0,  '2'},
    {"h264",    NO_ARG,     0,  'h'},
    {"h265",    NO_ARG,     0,  'H'},
    {0, 0, 0, 0}
};

static const char *short_options = "f:ABCD42hH";

struct hint_s {
    const char *arg;
    const char *str;
};

static const struct hint_s hint[] = {
    {"filename", "\t\tlocal filename"},
    {"", "\t\tstream A"},
    {"", "\t\tstream B"},
    {"", "\t\tstream C"},
    {"", "\t\tstream D"},
    {"", "\t\t4k"},
    {"", "\t\t2k"},
    {"", "\t\th264"},
    {"", "\t\th265"},
};

void usage(void)
{
    unsigned int i;
    printf("test_stream usage:\n");
    for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
        if (isalpha(long_options[i].val))
        { printf("-%c ", long_options[i].val); }
        else
        { printf("   "); }
        printf("--%s", long_options[i].name);
        if (hint[i].arg[0] != 0)
        { printf(" [%s]", hint[i].arg); }
        printf("\t%s\n", hint[i].str);
    }
    printf("\n");
}

int init_param(int argc, char **argv)
{
    int ch;
    int option_index = 0;

    opterr = 0;
    while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (ch) {
            case 'A':
                current_stream = 0;
                break;
            case 'B':
                current_stream = 1;
                break;
            case 'C':
                current_stream = 2;
                break;
            case 'D':
                current_stream = 3;
                break;
            case '4':
                current_resolution = 1;
                break;
            case '2':
                current_resolution = 0;
                break;
            case 'h':
                current_format = 0;
                break;
            case 'H':
                current_format = 1;
                break;
            case 'f':
                strcpy(local_file_name, optarg);
                break;
            default:
                printf("unknown command %s \n", optarg);
                return -1;
                break;
        }
    }

    return 0;
}

static u8 *__parse_next_start_code_hevc(u8 *p, u32 len, u32 &prefix_len, u8 &nal_type, u8 &first_byte)
{
    u32 state = 0;
    first_byte = 0;

    while (len) {
        switch (state) {
            case 0:
                if (!(*p)) {
                    state = 1;
                }
                break;

            case 1: //0
                if (!(*p)) {
                    state = 2;
                } else {
                    state = 0;
                }
                break;

            case 2: //0 0
                if (!(*p)) {
                    state = 3;
                } else if (1 == (*p)) {
                    prefix_len = 3;
                    nal_type = (p[1] >> 1) & 0x3f;
                    first_byte = p[3];
                    return (p - 2);
                } else {
                    state = 0;
                }
                break;

            case 3: //0 0 0
                if (!(*p)) {
                    state = 3;
                } else if (1 == (*p)) {
                    prefix_len = 4;
                    nal_type = (p[1] >> 1) & 0x3f;
                    first_byte = p[3];
                    return (p - 3);
                } else {
                    state = 0;
                }
                break;

            default:
                printf("error: impossible to comes here\n");
                break;

        }
        p++;
        len --;
    }

    return NULL;
}

static void capture_and_share_video()
{
    EECode ret;
    struct iav_querydesc query_desc;
    struct iav_framedesc *frame_desc;
    TU8 flag = 0, flag_previous = 0;
    u8 *p_video_buf = (u8 *)malloc(2 * 1024 * 1024);
    u32 data_len = 0;
    u32 nalu_num = 0;//hard code here
    u32 wait_first_key_frame = 1;
    u32 send_current = 0;

    SSharedResource resource;
    memset(&resource, 0x0, sizeof(resource));
    resource.type = ESharedResourceType_LiveStreamVideo;
    resource.index = 0;
    if (1 == current_format) {
        resource.property.video.format = (TU8)ESharedResourceFormat_H265;
    } else if (0 == current_format) {
        resource.property.video.format = (TU8)ESharedResourceFormat_H264;
        printf("[error]: not supported h264 now\n");
        return;
    } else {
        printf("[error]: not supported format %d\n", current_format);
        return;
    }
    resource.property.video.m = 0;
    resource.property.video.n = 30;
    if (1 == current_resolution) {
        resource.property.video.width = 3840;
        resource.property.video.height = 2160;
    } else if (0 == current_resolution) {
        resource.property.video.width = 1920;
        resource.property.video.height = 1080;
    } else {
        printf("[error]: not supported resolution %d\n", current_resolution);
        return;
    }

    IDirectSharingServer *server = gfCreateDSServer(DDefaultDirectSharingServerPort);
    IDirectSharingDispatcher *dispatcher = gfCreateDSDispatcher(ESharedResourceType_LiveStreamVideo);

    resource.property.video.framerate_num = 90000;
    resource.property.video.framerate_den = 3003;
    resource.property.video.bitrate = 4 * 1024 * 1024;
    strcpy(resource.description, "Ambarella 4K HEVC live streaming");
    dispatcher->SetResource(&resource);

    ret = server->AddDispatcher(dispatcher);
    ret = server->StartServer();

    printf("begin loop\n");

    if (local_file_name[0]) {
        local_file_handle = fopen(local_file_name, "wb+");
    }

    while (running) {
        memset(&query_desc, 0, sizeof(query_desc));
        frame_desc = &query_desc.arg.frame;
        query_desc.qid = IAV_DESC_FRAME;
        frame_desc->id = -1;

        if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
            perror("IAV_IOC_QUERY_DESC");
            continue;
        }

        if (frame_desc->id == current_stream) {
            u8 naltype = 0, firstbyte = 0;
            u32 prefix_len = 0;
            if (1) {
                u8 *ptmp = bsb_mem + frame_desc->data_addr_offset;
                if ((0 != ptmp[0]) || (0 != ptmp[1])) {
                    printf("error bits(non-nalu prefix?):\n");
                    gfPrintMemory(ptmp, 32);
                } else if ((0 != ptmp[2]) && (1 != ptmp[2])) {
                    printf("error bits(non-nalu prefix?):\n");
                    gfPrintMemory(ptmp, 32);
                } else if ((0 == ptmp[2]) && (1 != ptmp[3])) {
                    printf("error bits(non-nalu prefix?):\n");
                    gfPrintMemory(ptmp, 32);
                }
            }

            __parse_next_start_code_hevc(bsb_mem + frame_desc->data_addr_offset, 32, prefix_len, naltype, firstbyte);

            if ((naltype == EHEVCNalType_VPS)) {
                if (wait_first_key_frame) {
                    wait_first_key_frame = 0;
                    nalu_num = 0;
                    data_len = 0;
                }
                flag = DDirectSharingPayloadFlagKeyFrameIndicator;
                printf("key frame comes\n");
            } else if (wait_first_key_frame) {
                printf("skip before first key frame\n");
                continue;
            }

            send_current = firstbyte & 0x80;
            if ((send_current || (DDirectSharingPayloadFlagKeyFrameIndicator == flag)) && data_len) {
                //printf("send data, nalu_num %d\n", nalu_num);
                ret = dispatcher->SendData(p_video_buf, data_len, flag_previous);
                if (local_file_handle) {
                    fwrite(p_video_buf, data_len, 1, local_file_handle);
                    fflush(local_file_handle);
                }
                flag_previous = flag;
                flag = 0;
                nalu_num = 0;
                data_len = 0;
                if (EECode_OK != ret) {
                    break;
                }
            }

            memcpy(p_video_buf + data_len, bsb_mem + frame_desc->data_addr_offset, frame_desc->size);
            //printf("copy data, num = %d\n\t\t", nalu_num);
            //gfPrintMemory(p_video_buf + data_len, 8);
            data_len += frame_desc->size;
            nalu_num ++;
        }
    }

    printf("end loop\n");
    ret = server->StartServer();
    ret = server->RemoveDispatcher(dispatcher);

    dispatcher->Destroy();

    server->Destroy();

    if (local_file_handle) {
        fclose(local_file_handle);
        local_file_handle = NULL;
    }

    free(p_video_buf);
}

static void sigstop()
{
    running = 0;
}

int map_bsb(void)
{
    struct iav_querybuf querybuf;

    querybuf.buf = IAV_BUFFER_BSB;
    if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
        perror("IAV_IOC_QUERY_BUF");
        return -1;
    }

    bsb_size = querybuf.length;
    bsb_mem = (u8 *) mmap(NULL, bsb_size * 2, PROT_READ, MAP_SHARED, fd_iav, querybuf.offset);
    if (bsb_mem == MAP_FAILED) {
        perror("mmap (%d) failed: %s\n");
        return -1;
    }

    printf("bsb_mem = 0x%x, size = 0x%x\n", (u32)bsb_mem, bsb_size);
    return 0;
}

int main(int argc, char **argv)
{
    //register signal handler for Ctrl+C,  Ctrl+'\'  ,  and "kill" sys cmd
    signal(SIGINT, (__sighandler_t)sigstop);
    signal(SIGQUIT, (__sighandler_t)sigstop);
    signal(SIGTERM, (__sighandler_t)sigstop);

    if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
        perror("/dev/iav");
        return -1;
    }

    if (argc < 2) {
        usage();
        return -1;
    }

    if (init_param(argc, argv) < 0) {
        printf("init param failed \n");
        return -1;
    }

    if (map_bsb() < 0) {
        printf("map bsb failed\n");
        return -1;
    }

    capture_and_share_video();

    close(fd_iav);
    return 0;
}


