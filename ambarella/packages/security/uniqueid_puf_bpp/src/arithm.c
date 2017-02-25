/*******************************************************************************
 * arithm.c
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
#include <math.h>

#include "uniqueid_puf_bpp_if.h"

#include "internal_include.h"

int calculate_buffer_statistics(data_buffer_t *buf, double *average, double *standard_deviation)
{
    unsigned int tot_cnt = (unsigned int) buf->width * (unsigned int) buf->height, i = 0;
    type_pixel_t *p = buf->p_buf;
    double sum = 0, avg = 0, square_sum = 0;
    double t = 0;

    i = 0;
    p = buf->p_buf;
    while (i < tot_cnt) {
        sum += (double) p[0];
        p ++;
        i ++;
    }

    t = sum / (double) tot_cnt;
    avg = t;
    if (average) {
        *average = avg;
    }

    if (standard_deviation) {
        i = 0;
        p = buf->p_buf;
        while (i < tot_cnt) {
            square_sum += (double) (p[0] - avg) * (double) (p[0] - avg);
            p ++;
            i ++;
        }

        t = square_sum / (double) tot_cnt;
        *standard_deviation = sqrt(t);
    }

    return 0;
}

int calculate_buffer_pixel_distribution(data_buffer_t *buf, unsigned int *distribution)
{
    unsigned int tot_cnt = (unsigned int) buf->width * (unsigned int) buf->height, i = 0;
    type_pixel_t *p = buf->p_buf;

    i = 0;
    while (i < tot_cnt) {
        if (DMAX_PIXEL_VALUE > p[0]) {
            distribution[p[0]] ++;
            p ++;
            i ++;
        } else {
            DLOG_ERROR("pixel value 0x%08x exceed max value 0x%08x\n", p[0], DMAX_PIXEL_VALUE);
        }
    }

    return 0;
}

int estimate_threshold(unsigned int *distribution, unsigned int range,
    type_pixel_t *high_threshold, unsigned int *high_pixel_number)
{
    unsigned int *p = distribution + range - 1;
    type_pixel_t current_index = range - 1;
    unsigned int current_pixels = 0;

    while (current_index > 0) {
        if (p[0]) {
            if ((p[0] + current_pixels) > high_pixel_number[0]) {
                if (current_pixels) {
                    high_threshold[0] = current_index;
                    high_pixel_number[0] = current_pixels;
                    DLOG_DEBUG("get threshold, current value 0x%08x, pixel number %d\n", current_index, current_pixels);
                } else {
                    DLOG_ERROR("[error]: do not get threshold, current value 0x%08x, pixel number %d\n", current_index, p[0]);
                    return (-1);
                }
                return 0;
            }
        }
        current_pixels += p[0];
        current_index --;
        p --;
    }

    return 0;
}

int estimate_threshold_high_low(unsigned int *distribution, unsigned int range,
    type_pixel_t *high_threshold, type_pixel_t *low_threshold,
    unsigned int *high_pixel_number, unsigned int *low_pixel_number)
{
    unsigned int *p = distribution + range;
    type_pixel_t current_index = range;
    unsigned int current_pixels = 0;
    unsigned int get_high_threshold = 0;

    while (current_index > 1) {
        if (p[0]) {
            if (!get_high_threshold) {
                if ((p[0] + current_pixels) > high_pixel_number[0]) {
                    get_high_threshold = 1;
                    if (current_pixels) {
                        high_threshold[0] = current_index;
                        high_pixel_number[0] = current_pixels;
                        DLOG_DEBUG("get high threshold, current value 0x%08x, pixel number %d\n", current_index, current_pixels);
                    } else {
                        DLOG_DEBUG("do not get high threshold, current value 0x%08x, pixel number %d\n", current_index, p[0]);
                        high_threshold[0] = current_index - 1;
                        high_pixel_number[0] = current_pixels + p[0];
                    }
                }
            }
            if ((p[0] + current_pixels) > low_pixel_number[0]) {
                if (current_pixels) {
                    low_threshold[0] = current_index;
                    low_pixel_number[0] = current_pixels;
                    DLOG_DEBUG("get low threshold, current value %d, pixel number %d\n", current_index, current_pixels);
                } else {
                    DLOG_DEBUG("do not get low threshold, current value %d, pixel number %d\n", current_index, p[0]);
                    low_threshold[0] = current_index - 1;
                    low_pixel_number[0] = current_pixels + p[0];
                }
                return 0;
            }
        }
        current_pixels += p[0];
        current_index --;
        p --;
    }

    return 0;
}

int add_buffer(accumulate_buffer_t *acc_buf, data_buffer_t *buf1, data_buffer_t *buf2)
{
    unsigned int tot_cnt = (unsigned int) acc_buf->width * (unsigned int) acc_buf->height;
    type_pixel_t *p1 = buf1->p_buf, *p2 = buf2->p_buf;
    type_accumulate_pixel_t *pa = acc_buf->p_buf;

    while (tot_cnt) {
        pa[0] = p1[0] + p2[0];
        pa ++;
        p1 ++;
        p2 ++;
        tot_cnt --;
    }

    return 0;
}

int accumulate_buffer(accumulate_buffer_t *acc_buf, data_buffer_t *buf)
{
    unsigned int tot_cnt = (unsigned int) acc_buf->width * (unsigned int) acc_buf->height;
    type_pixel_t *p1 = buf->p_buf;
    type_accumulate_pixel_t *pa = acc_buf->p_buf;

    while (tot_cnt) {
        pa[0] += p1[0];
        pa ++;
        p1 ++;
        tot_cnt --;
    }

    return 0;
}

int average_buffer(accumulate_buffer_t *acc_buf, data_buffer_t *buf, unsigned int buffer_number)
{
    unsigned int tot_cnt = (unsigned int) acc_buf->width * (unsigned int) acc_buf->height;
    type_pixel_t *p1 = buf->p_buf;
    type_accumulate_pixel_t *pa = acc_buf->p_buf;

    while (tot_cnt) {
        p1[0] = (type_pixel_t) pa[0] / buffer_number;
        pa ++;
        p1 ++;
        tot_cnt --;
    }

    return 0;
}

