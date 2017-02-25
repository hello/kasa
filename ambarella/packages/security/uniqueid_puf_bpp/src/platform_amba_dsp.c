/*******************************************************************************
 * platform_amba_dsp.cpp
 *
 * History:
 *    2016/03/31 - [Zhi He] create file
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
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uniqueid_puf_bpp_if.h"
#include "internal_include.h"
#include "platform_amba_dsp.h"

#include "config.h"

#ifndef CONFIG_ARCH_A5S
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <basetypes.h>

#if defined(CONFIG_ARCH_S2) || defined (CONFIG_ARCH_S2E)
#include "ambas_common.h"
#include "iav_drv.h"

void setup_amba_dsp(amba_dsp_t *al)
{
    al->f_map_dsp = NULL;
    al->f_query_raw_buffer = NULL;
}

#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <iav_ioctl.h>

static int __map_dsp(int iav_fd, dsp_map_t *map)
{
    struct iav_querybuf querybuf;

    querybuf.buf = IAV_BUFFER_DSP;
    if (ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
        perror("IAV_IOC_QUERY_BUF");
        DLOG_ERROR("IAV_IOC_QUERY_BUF fail\n");
        return -1;
    }

    map->size = querybuf.length;
    map->base = mmap(NULL, map->size, PROT_READ, MAP_SHARED, iav_fd, querybuf.offset);
    if (MAP_FAILED == map->base) {
        perror("mmap");
        DLOG_ERROR("mmap fail\n");
        return -1;
    }

    return 0;
}

static int __query_raw_buffer(int iav_fd, rawbuf_desc_t *desc)
{
    struct iav_rawbufdesc *raw_desc;
    struct iav_querydesc query_desc;

    memset(&query_desc, 0, sizeof(query_desc));
    query_desc.qid = IAV_DESC_RAW;
    raw_desc = &query_desc.arg.raw;
    if (ioctl(iav_fd, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
        perror("IAV_IOC_QUERY_DESC");
        DLOG_ERROR("IAV_IOC_QUERY_DESC fail\n");
        return (-1);
    }

    desc->raw_addr_offset = raw_desc->raw_addr_offset;
    desc->width = raw_desc->width;
    desc->height = raw_desc->height;
    desc->pitch = raw_desc->pitch;
    desc->mono_pts = raw_desc->mono_pts;

    return 0;
}


void setup_amba_dsp(amba_dsp_t *al)
{
    al->f_map_dsp = __map_dsp;
    al->f_query_raw_buffer = __query_raw_buffer;
}

#endif

#else

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <basetypes.h>

#include "ambas_common.h"
#include "iav_drv.h"

void setup_amba_dsp(amba_dsp_t *al)
{
    al->f_map_dsp = NULL;
    al->f_query_raw_buffer = NULL;
}

#endif

int open_iav_handle()
{
    int fd = open("/dev/iav", O_RDWR, 0);
    if (0 > fd) {
        DLOG_ERROR("open iav fail, %d.\n", fd);
    }
    return fd;
}

void close_iav_handle(int fd)
{
    if (0 > fd) {
        DLOG_ERROR("bad fd %d\n", fd);
        return;
    }
    close(fd);
    return;
}

