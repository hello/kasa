/*******************************************************************************
 * platform_amba_dsp.h
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

#ifndef __PLATFORM_AMBA_DSP_H__
#define __PLATFORM_AMBA_DSP_H__

typedef struct {
    void *base;
    unsigned int size;
} dsp_map_t;

typedef struct {
    unsigned int raw_addr_offset;
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned long long mono_pts;
} rawbuf_desc_t;

typedef int (*tf_map_dsp)(int iav_fd, dsp_map_t *map);
typedef int (*tf_query_raw_buffer)(int iav_fd, rawbuf_desc_t *desc);

typedef struct {
    tf_map_dsp f_map_dsp;
    tf_query_raw_buffer f_query_raw_buffer;
} amba_dsp_t;

void setup_amba_dsp(amba_dsp_t *al);

int open_iav_handle();
void close_iav_handle(int fd);

#endif

