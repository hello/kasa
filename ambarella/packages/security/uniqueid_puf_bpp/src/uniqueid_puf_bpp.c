/*******************************************************************************
 * uniqueid_puf_bpp.c
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

#include "cryptography_if.h"

typedef struct
{
    data_source_base_t *data_source;

    accumulate_buffer_t acc_buffer;
    data_buffer_t input_buffer[DMAX_INPUT_BUFFER_NUMBER];
    data_buffer_t avg_buffer;

    unsigned int *p_distribution;

    hot_pixel_t *hot_pixel_list;

    unsigned int input_buffer_num;
    unsigned int hot_pixel_number;
    type_pixel_t hot_pixel_threshold;

    double avg_buffer_average, avg_buffer_standard_deviation;
} calculation_context;

unsigned int g_print_pixel_list = 0;

static int __allocate_buffer(data_buffer_t *buf)
{
    unsigned int mem_size = buf->width * buf->height * sizeof(type_pixel_t);
    if (mem_size) {
        buf->p_buf = (type_pixel_t *) malloc(mem_size);
        if (!buf->p_buf) {
            DLOG_DEBUG("[error]: no memory, rerquest size %d\n", mem_size);
            return (-1);
        }
    } else {
        DLOG_DEBUG("[error]: bad params, width %d, height %d\n", buf->width, buf->height);
        return (-2);
    }
    return 0;
}

static void __release_buffer(data_buffer_t *buf)
{
    if (buf->p_buf) {
        free(buf->p_buf);
        buf->p_buf = NULL;
    }
}

static int __allocate_accumulate_buffer(accumulate_buffer_t *buf)
{
    unsigned int mem_size = buf->width * buf->height * sizeof(type_accumulate_pixel_t);
    if (mem_size) {
        buf->p_buf = (type_accumulate_pixel_t *) malloc(mem_size);
        if (!buf->p_buf) {
            DLOG_DEBUG("[error]: no memory, rerquest size %d\n", mem_size);
            return (-1);
        }
    } else {
        DLOG_DEBUG("[error]: bad params, width %d, height %d\n", buf->width, buf->height);
        return (-2);
    }
    return 0;
}

static void __release_accumulate_buffer(accumulate_buffer_t *buf)
{
    if (buf->p_buf) {
        free(buf->p_buf);
        buf->p_buf = NULL;
    }
}

static int __retrieve_hot_pixel_list(data_buffer_t *buf, type_pixel_t threashold, hot_pixel_t *hot_pixel_list, double avg, double deviation)
{
    type_dimension_t x = 0, y = 0;
    unsigned int index = 0;
    double vd = 0;
    type_pixel_t *p = buf->p_buf;

    for (y = 0; y < buf->height; y ++) {
        for (x = 0; x < buf->width; x ++) {
            if (p[0] > threashold) {
                if (DMAX_PIXEL_VALUE > p[0]) {
                    vd = (double) p[0];
                    if (vd > avg) {
                        vd = ((vd - avg) + (deviation / 2)) / deviation;
                        hot_pixel_list[index].x = x;
                        hot_pixel_list[index].y = y;
                        hot_pixel_list[index].value = p[0];
                        hot_pixel_list[index].delta = (unsigned int) vd;
                        hot_pixel_list[index].delta_max = hot_pixel_list[index].delta;
                        hot_pixel_list[index].delta_min = hot_pixel_list[index].delta;
                        hot_pixel_list[index].acc_value = hot_pixel_list[index].value;
                        hot_pixel_list[index].acc_delta = hot_pixel_list[index].delta;
                        hot_pixel_list[index].acc_count = 1;
                        index ++;
                    } else {
                        DLOG_ERROR("hot pixel value(%d) less than avg(%lf)?\n", p[0], avg);
                    }
                } else {
                    DLOG_ERROR("pixel value 0x%08x exceed max value 0x%08x\n", p[0], DMAX_PIXEL_VALUE);
                }
            }
            p ++;
        }
    }

    return 0;
}

static int __process_data(calculation_context *context)
{
    unsigned int i = 0;
    int ret = 0;

    DLOG_DEBUG("[flow]: read data...\n");
    for (i = 0; i < context->input_buffer_num; i ++) {
        ret = context->data_source->f_read_raw_data(context->data_source, &context->input_buffer[i]);
        if (ret) {
            DLOG_ERROR("[error]: read_data[%d] fail\n", i);
            return ret;
        }
    }

    DLOG_DEBUG("[flow]: accumulate buffers...\n");
    add_buffer(&context->acc_buffer, &context->input_buffer[0], &context->input_buffer[1]);
    for (i = 2; i < context->input_buffer_num; i ++) {
        accumulate_buffer(&context->acc_buffer, &context->input_buffer[i]);
    }

    DLOG_DEBUG("[flow]: average buffer...\n");
    average_buffer(&context->acc_buffer, &context->avg_buffer, context->input_buffer_num);

    DLOG_DEBUG("[flow]: calculate average buffer's average and standard deviation\n");
    ret = calculate_buffer_statistics(&context->avg_buffer, &context->avg_buffer_average, &context->avg_buffer_standard_deviation);
    if (ret) {
        DLOG_ERROR("[error]: calculate_buffer_statistics[%d] fail\n", i);
        return ret;
    }
    DLOG_DEBUG("[statistics]: average buffer's average is %lf, standard deviation is %lf\n", context->avg_buffer_average, context->avg_buffer_standard_deviation);

    DLOG_DEBUG("[flow]: calculate pixel distribution\n");
    ret = calculate_buffer_pixel_distribution(&context->avg_buffer, context->p_distribution);
    if (ret) {
        DLOG_ERROR("[error]: calculate_buffer_pixel_distribution fail\n");
        return ret;
    }

    DLOG_DEBUG("[flow]: estimate threshold, pixel number %d\n", context->hot_pixel_number);
    ret = estimate_threshold(context->p_distribution, DMAX_PIXEL_VALUE, &context->hot_pixel_threshold, &context->hot_pixel_number);
    if (ret) {
        DLOG_ERROR("[error]: estimate_threshold fail\n");
        return ret;
    }
    DLOG_DEBUG("[statistics]: threshold 0x%08x, %d, number %d\n", context->hot_pixel_threshold, context->hot_pixel_threshold, context->hot_pixel_number);

    context->hot_pixel_list = (hot_pixel_t *) malloc(context->hot_pixel_number * sizeof(hot_pixel_t));
    if (!context->hot_pixel_list) {
        DLOG_ERROR("[error]: alloc hot pixel list fail, pixel number %d, request size %d\n", context->hot_pixel_number, context->hot_pixel_number * sizeof(hot_pixel_t));
        return (-2);
    }
    memset(context->hot_pixel_list, 0x0, context->hot_pixel_number * sizeof(hot_pixel_t));

    ret = __retrieve_hot_pixel_list(&context->avg_buffer, context->hot_pixel_threshold, context->hot_pixel_list, context->avg_buffer_average, context->avg_buffer_standard_deviation);
    if (ret) {
        DLOG_ERROR("[error]: __retrieve_hot_pixel_list fail\n");
        return ret;
    }

    return 0;
}

static int __pixel_in_list(hot_pixel_t *pixel, hot_pixel_t **pp_list, unsigned int *p_num)
{
    type_dimension_t x = pixel->x, y = pixel->y;
    hot_pixel_t *cur = *pp_list;
    unsigned int num = *p_num;

    while (num) {
        if ((cur->x == x) && cur->y == y) {
            *pp_list = cur;
            *p_num = num;
            return 1;
        }
        num --;
        cur ++;
    }

    return 0;
}

static unsigned int __get_common_pixels(hot_pixel_t *list1, unsigned int num_1, hot_pixel_t *list2, unsigned int num_2)
{
    hot_pixel_t *p0 = list1;
    hot_pixel_t *p1 = list1;
    int find = 0;
    unsigned int remain_num = 0;
    hot_pixel_t *tmp_list2 = list2;
    unsigned int tmp_num_2 = num_2;

    while (num_1) {
        find = __pixel_in_list(p1, &tmp_list2, &tmp_num_2);
        if (find) {
            if (p0 != p1) {
                p0->x = p1->x;
                p0->y = p1->y;
                p0->value = p1->value;
                p0->delta = p1->delta;

                p0->acc_value = p1->acc_value;
                p0->acc_count = p1->acc_count;
                p0->acc_delta = p1->acc_delta;

                p0->delta_max = p1->delta_max;
                p0->delta_min = p1->delta_min;
            }

            p0->acc_value += tmp_list2->value;
            p0->acc_delta += tmp_list2->delta;
            p0->acc_count ++;

            if (p0->delta_max < tmp_list2->delta) {
                p0->delta_max = tmp_list2->delta;
            }

            if (p0->delta_min > tmp_list2->delta) {
                p0->delta_min = tmp_list2->delta;
            }

            p0 ++;
            remain_num ++;
        }
        p1 ++;
        num_1 --;
    }

    return remain_num;
}

static int __is_pixel_match_in_list(type_dimension_t x, type_dimension_t y, unsigned short delta, unsigned short delta_deviation, hot_pixel_t **pp_list, unsigned int *p_num)
{
    hot_pixel_t *cur = *pp_list;
    unsigned int num = *p_num;

    unsigned int delta_range = 0;

    while (num) {
        if ((cur->x == x) && cur->y == y) {
            *pp_list = cur;
            *p_num = num;
            delta_range = (((delta * 3) + 5) / 10) + delta_deviation + 1;
            if (((delta + delta_range + 10) >= cur->delta ) && (delta <= (cur->delta + delta_range))) {
                return 1;
            } else {
                DLOG_ALWAYS("delta diff large, [%04d, %04d]: delta %d, delta_range %d, cur->delta %d\n", x, y, delta, delta_range, cur->delta);
            }
            return 0;
        }
        num --;
        cur ++;
    }

    return 0;
}

static int __is_pixel_match_in_list_without_delta(type_dimension_t x, type_dimension_t y, hot_pixel_t **pp_list, unsigned int *p_num)
{
    hot_pixel_t *cur = *pp_list;
    unsigned int num = *p_num;

    while (num) {
        if ((cur->x == x) && cur->y == y) {
            *pp_list = cur;
            *p_num = num;
            return 1;
        }
        num --;
        cur ++;
    }

    return 0;
}

static unsigned int __get_storage_stable_pixel_number(hot_pixel_storage_t *list, unsigned int num)
{
    unsigned int stable_pixel_num = 0;

    while (num) {
        if ((list->delta >= 5) && (list->delta >= (2 * list->delta_deviation + 1))) {
            stable_pixel_num ++;
        }
        list ++;
        num --;
    }

    return stable_pixel_num;
}

static unsigned int __get_stable_pixel_number(hot_pixel_t *list, unsigned int num)
{
    unsigned int stable_pixel_num = 0;

    while (num) {
        if ((list->delta >= 5) && (list->delta >= (2 * list->delta_deviation + 1))) {
            stable_pixel_num ++;
        }
        list ++;
        num --;
    }

    return stable_pixel_num;
}

static unsigned int __get_matched_pixel_number(hot_pixel_storage_t *list1, unsigned int num_1, hot_pixel_t *list2, unsigned int num_2)
{
    hot_pixel_storage_t *p0 = list1;
    int find = 0;
    unsigned int remain_num = 0;
    hot_pixel_t *tmp_list2 = list2;
    unsigned int tmp_num_2 = num_2;

    while (num_1) {
        find = __is_pixel_match_in_list(p0->x, p0->y, p0->delta, p0->delta_deviation, &tmp_list2, &tmp_num_2);
        if (find) {
            remain_num ++;
        }
        p0 ++;
        num_1 --;
    }

    return remain_num;
}

static unsigned int __get_matched_pixel_number_without_delta(hot_pixel_storage_t *list1, unsigned int num_1, hot_pixel_t *list2, unsigned int num_2)
{
    hot_pixel_storage_t *p0 = list1;
    int find = 0;
    unsigned int remain_num = 0;
    hot_pixel_t *tmp_list2 = list2;
    unsigned int tmp_num_2 = num_2;

    while (num_1) {
        find = __is_pixel_match_in_list_without_delta(p0->x, p0->y, &tmp_list2, &tmp_num_2);
        if (find) {
            remain_num ++;
        }
        p0 ++;
        num_1 --;
    }

    return remain_num;
}

static void __calculate_delta(hot_pixel_t *list, unsigned int num)
{
    while (num) {
        list->delta = (list->acc_delta + (list->acc_count / 2)) / list->acc_count;
        list->delta_deviation = list->delta_max - list->delta;
        if (list->delta_deviation < (list->delta - list->delta_min)) {
            list->delta_deviation = list->delta - list->delta_min;
        }
        list ++;
        num --;
    }

}

static void __print_pixel_list(hot_pixel_t *list, unsigned int num)
{
    DLOG_ALWAYS("pixel list, number %d:\n", num);

    while (num) {
        //DLOG_ALWAYS("[%04d, %04d], value 0x%08x, delta = %d, delta_deviation %d\n", list->x, list->y, list->acc_value / list->acc_count, list->delta, list->delta_deviation);
        DLOG_ALWAYS("[%04d, %04d], delta = %d, delta_deviation %d\n", list->x, list->y, list->delta, list->delta_deviation);
        list ++;
        num --;
    }

}

static void __print_storage(hot_pixel_list_storage_t *storage)
{
    unsigned int num = storage->number_of_pixel;
    hot_pixel_storage_t *list = storage->pixel_list;

    DLOG_ALWAYS("[storage]: params: resolution %dx%d, pixel number %d, input buffer %d, times %d\n",
        storage->param_resolution_width, storage->param_resolution_height,
        storage->param_hot_pixel_number, storage->param_input_frame_number, storage->param_repeat_times);
    DLOG_ALWAYS("[storage]: number of pixels %d, pixel avg %d, pixel deviation %d\n",
        storage->number_of_pixel, storage->pixel_avg, storage->pixel_standard_deviation);

    while (num) {
        DLOG_ALWAYS("[%04d, %04d], delta = %d, delta_deviation %d\n", list->x, list->y, list->delta, list->delta_deviation);
        list ++;
        num --;
    }

}

static void __print_id(uniqueid_t *id)
{
    unsigned char *p =id->id;

    DLOG_ALWAYS("unique id:\n");

    DLOG_ALWAYS("\t%02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n",
        p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    p += 16;
    DLOG_ALWAYS("\t%02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n",
        p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    p += 16;
    DLOG_ALWAYS("\t%02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n",
        p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    p += 16;
    DLOG_ALWAYS("\t%02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n",
        p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
}

static int __store_list_to_file(char *filename, hot_pixel_list_storage_t *storage, hot_pixel_t *pixel_list, unsigned int num)
{
    unsigned int i = 0;
    storage->pixel_list = (hot_pixel_storage_t *) malloc(sizeof(hot_pixel_storage_t) * num);

    for (i = 0; i < num; i ++) {
        storage->pixel_list[i].x = pixel_list[i].x;
        storage->pixel_list[i].y = pixel_list[i].y;
        storage->pixel_list[i].delta = pixel_list[i].delta;
        storage->pixel_list[i].delta_deviation = pixel_list[i].delta_deviation;
    }

    return write_hot_pixel_list_to_file(filename, storage);
}

int get_name_from_id(uniqueid_t *id, char *name)
{
    if (id && name) {
        uniqueid_sensor_bpp_internal_t *iid = (uniqueid_sensor_bpp_internal_t *) id;
        memcpy(name, iid->param_sensor_name, DUNIQUEID_MAX_NAME_LENGTH);
    } else {
        DLOG_ERROR("[error]: NULL input id %p, name %p\n", id, name);
        return (-1);
    }

    return 0;
}

int get_resolution_from_id(uniqueid_t *id, unsigned int *width, unsigned int *height)
{
    if (id && width && height) {
        uniqueid_sensor_bpp_internal_t *iid = (uniqueid_sensor_bpp_internal_t *) id;
        * width = (((unsigned int) iid->param_resolution_width[0]) << 8) | ((unsigned int) iid->param_resolution_width[1]);
        * height = (((unsigned int) iid->param_resolution_height[0]) << 8) | ((unsigned int) iid->param_resolution_height[1]);
    } else {
        DLOG_ERROR("[error]: NULL input id %p, width %p, height %p\n", id, width, height);
        return (-1);
    }

    return 0;
}

int generate_unique_id_from_puf_bpp(uniqueid_t *id, char *name, unsigned int input_buffer_num, unsigned int hot_pixel_number, unsigned int times)
{
    unsigned int i = 0;
    type_dimension_t width = 0, height = 0;
    int ret = 0;

    hot_pixel_t *current_list = NULL;
    unsigned int current_hot_list_number = 0;
    hot_pixel_list_storage_t storage;
    calculation_context context;
    uniqueid_sensor_bpp_internal_t *iid = (uniqueid_sensor_bpp_internal_t *) id;

    if ((!id) || (!name) || (!input_buffer_num) || (!hot_pixel_number) || (!times)) {
        DLOG_ERROR("[error]: bad parameters\n");
        return (-1);
    }

    if (DMAX_INPUT_BUFFER_NUMBER < input_buffer_num) {
        DLOG_ERROR("[error]: input buffer number(%d) too large, max value (%d)\n", input_buffer_num, DMAX_INPUT_BUFFER_NUMBER);
        return (-1);
    } else if (2 > input_buffer_num) {
        DLOG_ERROR("[error]: input buffer number(%d) too small, min value (2)\n", input_buffer_num);
        return (-1);
    }

    memset(&context, 0x0, sizeof(context));
    memset(&storage, 0x0, sizeof(storage));

    context.input_buffer_num = input_buffer_num;
    context.hot_pixel_number = hot_pixel_number;
    DLOG_DEBUG("buffer number %d, %d, pixel number %d, %d\n", input_buffer_num, context.input_buffer_num, hot_pixel_number, context.hot_pixel_number);

    context.data_source = create_data_source(E_DATA_SOURCE_SENSOR_RAW);
    if (!context.data_source) {
        DLOG_ERROR("[error]: create_data_source fail\n");
        return (-2);
    }

    ret = context.data_source->f_query_dimension(context.data_source, &width, &height);
    if (ret) {
        DLOG_ERROR("[error]: query dimension fail\n");
        goto __generate_unique_id_from_puf_bpp_exit;
    }
    DLOG_DEBUG("[flow]: query dimension, %dx%d\n", width, height);

    context.p_distribution = (unsigned int *) malloc(DMAX_PIXEL_VALUE * sizeof(unsigned int));
    if (!context.p_distribution) {
        DLOG_ERROR("[error]: alloc distribution memory fail\n");
        ret = -4;
        goto __generate_unique_id_from_puf_bpp_exit;
    }

    context.acc_buffer.width = width;
    context.acc_buffer.height = height;
    ret = __allocate_accumulate_buffer(&context.acc_buffer);
    if (ret) {
        DLOG_ERROR("[error]: __allocate_accumulate_buffer[%d] fail\n", i);
        goto __generate_unique_id_from_puf_bpp_exit;
    }

    context.avg_buffer.width = width;
    context.avg_buffer.height = height;
    ret = __allocate_buffer(&context.avg_buffer);
    if (ret) {
        DLOG_ERROR("[error]: __allocate_buffer[%d] fail\n", i);
        goto __generate_unique_id_from_puf_bpp_exit;
    }

    DLOG_DEBUG("[flow]: allocate buffers... (number %d, width %d, height %d)\n", input_buffer_num, width, height);
    for (i = 0; i < input_buffer_num; i ++) {
        context.input_buffer[i].width = width;
        context.input_buffer[i].height = height;
        ret = __allocate_buffer(&context.input_buffer[i]);
        if (ret) {
            DLOG_ERROR("[error]: __allocate_buffer[%d] fail\n", i);
            goto __generate_unique_id_from_puf_bpp_exit;
        }
    }

    DLOG_DEBUG("[flow]: processing... (times %d)\n", times);
    for (i = 0; i < times + DUNIQUEID_GENERATE_ADDITIONAL_TIMES; i ++) {
        memset(context.p_distribution, 0x0, DMAX_PIXEL_VALUE * sizeof(unsigned int));
        ret = __process_data(&context);
        if (ret) {
            DLOG_ERROR("[error]: __process_data[%d] fail\n", i);
            goto __generate_unique_id_from_puf_bpp_exit;
        }

        if (!current_list) {
            current_list = context.hot_pixel_list;
            current_hot_list_number = context.hot_pixel_number;
        } else {
            current_hot_list_number = __get_common_pixels(current_list, current_hot_list_number, context.hot_pixel_list, context.hot_pixel_number);
            DLOG_DEBUG("[statistics]: after process, common pixel number %d\n", current_hot_list_number);

            free(context.hot_pixel_list);
            context.hot_pixel_list = NULL;

            if (!current_hot_list_number) {
                DLOG_ERROR("[error]: do not find common hot pixels\n");
                ret = (-9);
                goto __generate_unique_id_from_puf_bpp_exit;
            }
        }
        context.hot_pixel_number = hot_pixel_number;
        context.hot_pixel_list = NULL;
    }

    if (current_list && current_hot_list_number) {

        __calculate_delta(current_list, current_hot_list_number);
        //__print_pixel_list(current_list, current_hot_list_number);

        unsigned int name_len = strlen(name);
        if (name_len > DUNIQUEID_MAX_NAME_LENGTH) {
            name_len = DUNIQUEID_MAX_NAME_LENGTH;
        }
        if (name_len) {
            memcpy(storage.param_sensor_name, name, name_len);
        }

        storage.param_resolution_width = width;
        storage.param_resolution_height = height;
        storage.param_hot_pixel_number = hot_pixel_number;
        storage.param_input_frame_number = input_buffer_num;
        storage.param_repeat_times = times;

        storage.number_of_pixel = current_hot_list_number;
        storage.marker_byte = DUNIQUEID_MARKER_BYTE;

        storage.pixel_avg = (type_pixel_t) context.avg_buffer_average;
        storage.pixel_standard_deviation = (type_pixel_t) context.avg_buffer_standard_deviation;

        __store_list_to_file((char *) "hotpixel.list", &storage, current_list, current_hot_list_number);

        if (g_print_pixel_list) {
            __print_storage(&storage);
        }

        if (storage.pixel_list) {
            void* digest_context = NULL;

            digest_context = digest_sha256_init();
            if (digest_context) {
                digest_sha256_update(digest_context, (const unsigned char *) &storage, sizeof(storage) - sizeof(storage.pixel_list));
                digest_sha256_update(digest_context, (const unsigned char *) storage.pixel_list, storage.number_of_pixel * sizeof(hot_pixel_storage_t));
                digest_sha256_final(digest_context, iid->digest_sha256);
                free(digest_context);
                digest_context = NULL;
            }

            digest_context = digest_md5_init();
            if (digest_context) {
                digest_md5_update(digest_context, (const unsigned char *) &storage, sizeof(storage) - sizeof(storage.pixel_list));
                digest_md5_update(digest_context, (const unsigned char *) storage.pixel_list, storage.number_of_pixel * sizeof(hot_pixel_storage_t));
                digest_md5_final(digest_context, iid->digest_md5);
                free(digest_context);
                digest_context = NULL;
            }

            memcpy(iid->param_sensor_name, storage.param_sensor_name, DUNIQUEID_MAX_NAME_LENGTH);
            iid->param_resolution_width[0] = (width >> 8) & 0xff;
            iid->param_resolution_width[1] = (width) & 0xff;
            iid->param_resolution_height[0] = (height >> 8) & 0xff;
            iid->param_resolution_height[1] = (height) & 0xff;
            iid->param_hot_pixel_number[0] = (hot_pixel_number >> 8) & 0xff;
            iid->param_hot_pixel_number[1] = (hot_pixel_number) & 0xff;
            iid->param_input_frame_number = input_buffer_num;
            iid->param_repeat_times = times;

            __print_id(id);
        } else {
            DLOG_ERROR("[error]: store fail, list is NULL\n");
            ret = -10;
        }

    } else {
        DLOG_ERROR("[error]: do not get common hot pixel list\n");
        ret = -11;
    }

__generate_unique_id_from_puf_bpp_exit:

    __release_accumulate_buffer(&context.acc_buffer);

    __release_buffer(&context.avg_buffer);

    for (i = 0; i < input_buffer_num; i ++) {
        __release_buffer(&context.input_buffer[i]);
    }

    if (context.p_distribution) {
        free(context.p_distribution);
        context.p_distribution = NULL;
    }

    if (context.data_source && context.data_source->f_destroy) {
        context.data_source->f_destroy(context.data_source);
        context.data_source= NULL;
    }

    if (current_list) {
        free(current_list);
        current_list = NULL;
    }

    if (context.hot_pixel_list) {
        free(context.hot_pixel_list);
        context.hot_pixel_list = NULL;
    }

    if (storage.pixel_list) {
        free(storage.pixel_list);
        storage.pixel_list = NULL;
    }

    if (context.data_source && context.data_source->f_destroy) {
        context.data_source->f_destroy(context.data_source);
        context.data_source = NULL;
    }

    return ret;
}

int verify_unique_id_from_puf_bpp(uniqueid_t *id)
{
    unsigned int i = 0;
    type_dimension_t width = 0, height = 0;
    unsigned int pixel_number = 0;
    int ret = 0;

    hot_pixel_t *current_list = NULL;
    unsigned int current_hot_list_number = 0;
    hot_pixel_list_storage_t storage;
    calculation_context context;
    uniqueid_sensor_bpp_internal_t *iid = (uniqueid_sensor_bpp_internal_t *) id;

    if (!id) {
        DLOG_ERROR("[error]: bad parameters\n");
        return (-1);
    }

    __print_id(id);

    memset(&context, 0x0, sizeof(context));
    memset(&storage, 0x0, sizeof(storage));

    ret = read_hot_pixel_list_from_file((char *) "hotpixel.list", &storage);
    if (ret) {
        DLOG_ERROR("[error]: read from file error\n");
        ret = (-1);
        goto __verify_unique_id_from_puf_bpp_exit;
    }

    if (storage.pixel_list) {
        void* digest_context = NULL;
        unsigned char digest[32] = {0};

        digest_context = digest_sha256_init();
        if (digest_context) {
            digest_sha256_update(digest_context, (const unsigned char *) &storage, sizeof(storage) - sizeof(storage.pixel_list));
            digest_sha256_update(digest_context, (const unsigned char *) storage.pixel_list, storage.number_of_pixel * sizeof(hot_pixel_storage_t));
            digest_sha256_final(digest_context, digest);
            free(digest_context);
            digest_context = NULL;
        }

        if (memcmp(iid->digest_sha256, digest, 32)) {
            DLOG_ERROR("[error]: sha256 not match\n");
            ret = (-3);
            goto __verify_unique_id_from_puf_bpp_exit;
        }

        digest_context = digest_md5_init();
        if (digest_context) {
            digest_md5_update(digest_context, (const unsigned char *) &storage, sizeof(storage) - sizeof(storage.pixel_list));
            digest_md5_update(digest_context, (const unsigned char *) storage.pixel_list, storage.number_of_pixel * sizeof(hot_pixel_storage_t));
            digest_md5_final(digest_context, digest);
            free(digest_context);
            digest_context = NULL;
        }

        if (memcmp(iid->digest_md5, digest, 16)) {
            DLOG_ERROR("[error]: md5 not match\n");
            ret = (-3);
            goto __verify_unique_id_from_puf_bpp_exit;
        }

    } else {
        DLOG_ERROR("[error]: load fail, list is NULL\n");
        ret = (-10);
        goto __verify_unique_id_from_puf_bpp_exit;
    }

    if (g_print_pixel_list) {
        __print_storage(&storage);
    }

    if (DUNIQUEID_MARKER_BYTE != storage.marker_byte) {
        DLOG_ERROR("[error]: storage's marker byte fail\n");
        ret = (-1);
        goto __verify_unique_id_from_puf_bpp_exit;
    }

    width = ((unsigned int) iid->param_resolution_width[0] << 8) | ((unsigned int) iid->param_resolution_width[1]);
    height = ((unsigned int) iid->param_resolution_height[0] << 8) | ((unsigned int) iid->param_resolution_height[1]);
    if ((storage.param_resolution_width != width) || (storage.param_resolution_height != height)) {
        DLOG_ERROR("[error]: resolution not match, from id %dx%d, from storage %dx%d\n", width, height, storage.param_resolution_width, storage.param_resolution_height);
        ret = (-2);
        goto __verify_unique_id_from_puf_bpp_exit;
    }

    pixel_number = ((unsigned int) iid->param_hot_pixel_number[0] << 8) | ((unsigned int) iid->param_hot_pixel_number[1]);
    if ((storage.param_hot_pixel_number != pixel_number) || (storage.param_input_frame_number != iid->param_input_frame_number) || (storage.param_repeat_times != iid->param_repeat_times)) {
        DLOG_ERROR("[error]: process param not match, from id (pixel number %d, input buffer num %d, repeat times %d), from storage (pixel number %d, input buffer num %d, repeat times %d)\n", pixel_number, iid->param_input_frame_number, iid->param_repeat_times, storage.param_hot_pixel_number, storage.param_input_frame_number, storage.param_repeat_times);
        ret = (-2);
        goto __verify_unique_id_from_puf_bpp_exit;
    }

    context.input_buffer_num = storage.param_input_frame_number;
    context.hot_pixel_number = storage.param_hot_pixel_number;
    DLOG_DEBUG("buffer number %d, pixel number %d\n", context.input_buffer_num, context.hot_pixel_number);

    context.data_source = create_data_source(E_DATA_SOURCE_SENSOR_RAW);
    if (!context.data_source) {
        DLOG_ERROR("[error]: create_data_source fail\n");
        ret = (-2);
        goto __verify_unique_id_from_puf_bpp_exit;
    }

    ret = context.data_source->f_query_dimension(context.data_source, &width, &height);
    if (ret) {
        DLOG_ERROR("[error]: query dimension fail\n");
        ret = (-16);
        goto __verify_unique_id_from_puf_bpp_exit;
    }
    DLOG_DEBUG("[flow]: query dimension, %dx%d\n", width, height);
    if ((width != storage.param_resolution_width) || (height != storage.param_resolution_height)) {
        DLOG_ERROR("[error]: dimension not match, query %dx%d, from storage %dx%d\n", width, height, storage.param_resolution_width, storage.param_resolution_height);
        ret = (-17);
        goto __verify_unique_id_from_puf_bpp_exit;
    }

    context.p_distribution = (unsigned int *) malloc(DMAX_PIXEL_VALUE * sizeof(unsigned int));
    if (!context.p_distribution) {
        DLOG_ERROR("[error]: alloc distribution memory fail\n");
        ret = (-4);
        goto __verify_unique_id_from_puf_bpp_exit;
    }

    context.acc_buffer.width = width;
    context.acc_buffer.height = height;
    ret = __allocate_accumulate_buffer(&context.acc_buffer);
    if (ret) {
        DLOG_ERROR("[error]: __allocate_accumulate_buffer[%d] fail\n", i);
        goto __verify_unique_id_from_puf_bpp_exit;
    }

    context.avg_buffer.width = width;
    context.avg_buffer.height = height;
    ret = __allocate_buffer(&context.avg_buffer);
    if (ret) {
        DLOG_ERROR("[error]: __allocate_buffer[%d] fail\n", i);
        goto __verify_unique_id_from_puf_bpp_exit;
    }

    DLOG_DEBUG("[flow]: allocate buffers... (number %d, width %d, height %d)\n", storage.param_input_frame_number, width, height);
    for (i = 0; i < storage.param_input_frame_number; i ++) {
        context.input_buffer[i].width = width;
        context.input_buffer[i].height = height;
        ret = __allocate_buffer(&context.input_buffer[i]);
        if (ret) {
            DLOG_ERROR("[error]: __allocate_buffer[%d] fail\n", i);
            goto __verify_unique_id_from_puf_bpp_exit;
        }
    }

    DLOG_DEBUG("[flow]: processing... (times %d)\n", storage.param_repeat_times);
    for (i = 0; i < storage.param_repeat_times; i ++) {
        memset(context.p_distribution, 0x0, DMAX_PIXEL_VALUE * sizeof(unsigned int));
        ret = __process_data(&context);
        if (ret) {
            DLOG_ERROR("[error]: __process_data[%d] fail\n", i);
            goto __verify_unique_id_from_puf_bpp_exit;
        }

        if (!current_list) {
            current_list = context.hot_pixel_list;
            current_hot_list_number = context.hot_pixel_number;
        } else {
            current_hot_list_number = __get_common_pixels(current_list, current_hot_list_number, context.hot_pixel_list, context.hot_pixel_number);
            DLOG_DEBUG("[statistics]: after process, common pixel number %d\n", current_hot_list_number);

            free(context.hot_pixel_list);
            context.hot_pixel_list = NULL;

            if (!current_hot_list_number) {
                DLOG_ERROR("[error]: do not find common hot pixels\n");
                ret = (-9);
                goto __verify_unique_id_from_puf_bpp_exit;
            }
        }
        context.hot_pixel_number = storage.param_hot_pixel_number;
        context.hot_pixel_list = NULL;
    }

    if (current_list && current_hot_list_number) {
        unsigned int matched_number = 0;
        unsigned int storage_stable_pixel_number = __get_storage_stable_pixel_number(storage.pixel_list, storage.number_of_pixel);
        unsigned int match_threshold = 0;
        unsigned int current_stable_pixel_number = 0;

        __calculate_delta(current_list, current_hot_list_number);
        if (g_print_pixel_list) {
            __print_pixel_list(current_list, current_hot_list_number);
        }

        current_stable_pixel_number = __get_stable_pixel_number(current_list, current_hot_list_number);
        if ((current_stable_pixel_number >= 5) && (storage_stable_pixel_number >= 5)) {
            if ((3 * current_stable_pixel_number) >= storage_stable_pixel_number) {
                if (current_stable_pixel_number > storage_stable_pixel_number) {
                    match_threshold = (storage_stable_pixel_number * 5) / 10;
                } else {
                    match_threshold = (current_stable_pixel_number * 5) / 10;
                }
            } else {
                match_threshold = (current_stable_pixel_number * 5) / 10;
            }
        } else {
            if (current_stable_pixel_number >= storage_stable_pixel_number) {
                if (storage_stable_pixel_number > 3) {
                    match_threshold = (storage_stable_pixel_number * 9) / 10;
                } else {
                    match_threshold = storage_stable_pixel_number;
                }
            } else {
                if (current_stable_pixel_number > 2) {
                    match_threshold = current_stable_pixel_number;
                } else {
                    DLOG_ALWAYS("delta deviation too large, calculate matched pixel without delta check\n");

                    if ((current_hot_list_number >= 5) && (storage.number_of_pixel >= 5)) {
                        if (current_hot_list_number > storage.number_of_pixel) {
                            match_threshold = storage.number_of_pixel * 5 / 10;
                        } else {
                            match_threshold = current_hot_list_number * 5 / 10;
                        }
                    } else {
                        if (current_hot_list_number >= storage.number_of_pixel) {
                            if (storage.number_of_pixel > 3) {
                                match_threshold = (storage.number_of_pixel * 9) / 10;
                            }
                        } else {
                            match_threshold = current_hot_list_number;
                        }
                    }

                    matched_number = __get_matched_pixel_number_without_delta(storage.pixel_list, storage.number_of_pixel, current_list, current_hot_list_number);

                    if (g_print_pixel_list) {
                        DLOG_ALWAYS("hot pixel number (%d, %d), matched number %d\n", current_hot_list_number, storage.number_of_pixel, matched_number);
                    }

                    if (match_threshold > matched_number) {
                        if (matched_number >= 10) {
                            DLOG_ALWAYS("result: match\n");
                            ret = 0;
                        } else {
                            DLOG_ERROR("result: not match\n");
                            ret = (-100);
                        }
                    } else {
                        DLOG_ALWAYS("result: match\n");
                        ret = 0;
                    }
                    goto __verify_unique_id_from_puf_bpp_exit;
                }
            }
        }

        matched_number = __get_matched_pixel_number(storage.pixel_list, storage.number_of_pixel, current_list, current_hot_list_number);

        if (g_print_pixel_list) {
            DLOG_ALWAYS("hot pixel number (%d, %d), stable number (%d, %d), matched number %d\n", current_hot_list_number, storage.number_of_pixel, current_stable_pixel_number, storage_stable_pixel_number, matched_number);
        }

        if (match_threshold > matched_number) {
            if (matched_number >= 10) {
                DLOG_ALWAYS("result: match\n");
                ret = 0;
                goto __verify_unique_id_from_puf_bpp_exit;
            }

            DLOG_ERROR("result: not match\n");
            ret = (-100);
        } else {
            DLOG_ALWAYS("result: match\n");
            ret = 0;
        }

    } else {
        DLOG_ERROR("[error]: do not get common hot pixel list\n");
        ret = -11;
    }

__verify_unique_id_from_puf_bpp_exit:

    __release_accumulate_buffer(&context.acc_buffer);

    __release_buffer(&context.avg_buffer);

    for (i = 0; i < storage.param_input_frame_number; i ++) {
        __release_buffer(&context.input_buffer[i]);
    }

    if (context.p_distribution) {
        free(context.p_distribution);
        context.p_distribution = NULL;
    }

    if (context.data_source && context.data_source->f_destroy) {
        context.data_source->f_destroy(context.data_source);
        context.data_source= NULL;
    }

    if (current_list) {
        free(current_list);
        current_list = NULL;
    }

    if (context.hot_pixel_list) {
        free(context.hot_pixel_list);
        context.hot_pixel_list = NULL;
    }

    if (storage.pixel_list) {
        free(storage.pixel_list);
        storage.pixel_list = NULL;
    }

    if (context.data_source && context.data_source->f_destroy) {
        context.data_source->f_destroy(context.data_source);
        context.data_source = NULL;
    }

    return ret;
}

void debug_print_pixel_list(unsigned int enable)
{
    g_print_pixel_list = 1;
}

