/*******************************************************************************
 * data_source_amba_dap.c
 *
 * History:
 *  2016/03/31 - [Zhi He] create file
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
#include "data_source_amba_dsp.h"

static int __query_dimension_datasource_ambadsp(void *context, type_dimension_t *width, type_dimension_t *height)
{
    data_source_amba_dsp_t *thiz = (data_source_amba_dsp_t *) context;
    *width = thiz->width;
    *height = thiz->height;
    return 0;
}

static int __read_data_datasource_ambadsp(void *context, data_buffer_t *buf)
{
    data_source_amba_dsp_t *thiz = (data_source_amba_dsp_t *) context;
    rawbuf_desc_t rawbuf_desc;
    int ret = 0;

    ret = thiz->dsp_al.f_query_raw_buffer(thiz->iav_fd, &rawbuf_desc);
    if (ret) {
        DLOG_DEBUG("[error]: rawbuf_desc fail\n");
        return ret;
    }

    memcpy(buf->p_buf, ((unsigned char *) thiz->dsp_map.base) + rawbuf_desc.raw_addr_offset, thiz->width * thiz->height * sizeof(type_pixel_t));

    return 0;
}

static void __destroy_datasource_ambadsp(void *context)
{
    data_source_amba_dsp_t *thiz = (data_source_amba_dsp_t *) context;

    if (thiz) {
        if (0 < thiz->iav_fd) {
            close_iav_handle(thiz->iav_fd);
            thiz->iav_fd = -1;
        }
        free(thiz);
    }
}

data_source_base_t *create_data_source_amba_dsp()
{
    rawbuf_desc_t desc;
    int ret = 0;
    data_source_amba_dsp_t *thiz = (data_source_amba_dsp_t *) malloc(sizeof(data_source_amba_dsp_t));

    if (thiz) {

        setup_amba_dsp(&thiz->dsp_al);
        if ((!thiz->dsp_al.f_map_dsp) || (!thiz->dsp_al.f_query_raw_buffer)) {
            DLOG_ERROR("[error]: f_map_dsp or f_query_raw_buffer not avaiable\n");
            free(thiz);
            return NULL;
        }

        DLOG_DEBUG("[flow]: open iav handle\n");
        thiz->iav_fd = open_iav_handle();
        if (0 > thiz->iav_fd) {
            DLOG_ERROR("[error]: open iav fd fail\n");
            free(thiz);
            return NULL;
        }

        DLOG_DEBUG("[flow]: map dsp\n");
        ret = thiz->dsp_al.f_map_dsp(thiz->iav_fd, &thiz->dsp_map);
        if (ret) {
            DLOG_ERROR("[error]: f_map_dsp fail\n");
            free(thiz);
            return NULL;
        }

        DLOG_DEBUG("[flow]: query raw buffer\n");
        ret = thiz->dsp_al.f_query_raw_buffer(thiz->iav_fd, &desc);
        if (ret) {
            DLOG_ERROR("[error]: rawbuf_desc fail\n");
            free(thiz);
            return NULL;
        }

        thiz->width = desc.width;
        thiz->height = desc.height;

        DLOG_DEBUG("[flow]: dsp base 0x%08x, size 0x%08x, raw width %d, height %d\n",
            (unsigned int) thiz->dsp_map.base, thiz->dsp_map.size, thiz->width, thiz->height);

        thiz->base.f_query_dimension = __query_dimension_datasource_ambadsp;
        thiz->base.f_read_raw_data = __read_data_datasource_ambadsp;
        thiz->base.f_destroy = __destroy_datasource_ambadsp;

    } else {
        DLOG_ERROR("[error]: malloc conext fail\n");
        return NULL;
    }

    return &thiz->base;
}

