/*******************************************************************************
 * io.c
 *
 * History:
 *  2016/04/07 - [Zhi He] create file
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

int write_hot_pixel_list_to_file(char *filename, hot_pixel_list_storage_t *storage)
{
    int ret = 0;

    if (filename && storage) {
        FILE *file = fopen(filename, "wb+");
        if (file) {
            do {
                ret = fwrite(storage->param_sensor_name, 1, sizeof(storage->param_sensor_name), file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: write param_sensor_name fail\n");
                    break;
                }

                ret = fwrite(&storage->param_resolution_width, sizeof(storage->param_resolution_width), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: write param_resolution_width fail\n");
                    break;
                }

                ret = fwrite(&storage->param_resolution_height, sizeof(storage->param_resolution_height), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: write param_resolution_height fail\n");
                    break;
                }

                ret = fwrite(&storage->param_hot_pixel_number, sizeof(storage->param_hot_pixel_number), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: write param_hot_pixel_number fail\n");
                    break;
                }

                ret = fwrite(&storage->param_input_frame_number, sizeof(storage->param_input_frame_number), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: write param_input_frame_number fail\n");
                    break;
                }

                ret = fwrite(&storage->param_repeat_times, sizeof(storage->param_repeat_times), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: write param_repeat_times fail\n");
                    break;
                }

                ret = fwrite(&storage->number_of_pixel, sizeof(storage->number_of_pixel), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: write number_of_pixel fail\n");
                    break;
                }

                ret = fwrite(&storage->reserved0, sizeof(storage->reserved0), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: write delta_error fail\n");
                    break;
                }

                ret = fwrite(&storage->marker_byte, sizeof(storage->marker_byte), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: write marker_byte fail\n");
                    break;
                }

                ret = fwrite(&storage->pixel_avg, sizeof(storage->pixel_avg), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: write pixel_avg fail\n");
                    break;
                }

                ret = fwrite(&storage->pixel_standard_deviation, sizeof(storage->pixel_standard_deviation), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: write pixel_standard_deviation fail\n");
                    break;
                }

                ret = fwrite(storage->pixel_list, sizeof(hot_pixel_storage_t), storage->number_of_pixel, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: write pixel_list fail\n");
                    break;
                }
            } while (0);

            fclose(file);
        } else {
            DLOG_ERROR("[error]: open file (%s) fail\n", filename);
            return (-1);
        }
    } else {
        DLOG_ERROR("[error]: bad params, filename %p, storage %p\n", filename, storage);
        return (-2);
    }

    return ret;
}

int read_hot_pixel_list_from_file(char *filename, hot_pixel_list_storage_t *storage)
{
    int ret = 0;

    if (filename && storage) {
        FILE *file = fopen(filename, "rb");
        if (file) {
            do {
                ret = fread(storage->param_sensor_name, 1, sizeof(storage->param_sensor_name), file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: read param_sensor_name fail\n");
                    break;
                }

                ret = fread(&storage->param_resolution_width, sizeof(storage->param_resolution_width), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: read param_resolution_width fail\n");
                    break;
                }

                ret = fread(&storage->param_resolution_height, sizeof(storage->param_resolution_height), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: read param_resolution_height fail\n");
                    break;
                }

                ret = fread(&storage->param_hot_pixel_number, sizeof(storage->param_hot_pixel_number), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: read param_hot_pixel_number fail\n");
                    break;
                }

                ret = fread(&storage->param_input_frame_number, sizeof(storage->param_input_frame_number), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: read param_input_frame_number fail\n");
                    break;
                }

                ret = fread(&storage->param_repeat_times, sizeof(storage->param_repeat_times), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: read param_repeat_times fail\n");
                    break;
                }

                ret = fread(&storage->number_of_pixel, sizeof(storage->number_of_pixel), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: read number_of_pixel fail\n");
                    break;
                }

                ret = fread(&storage->reserved0, sizeof(storage->reserved0), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: read delta_error fail\n");
                    break;
                }

                ret = fread(&storage->marker_byte, sizeof(storage->marker_byte), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: read marker_byte fail\n");
                    break;
                }

                ret = fread(&storage->pixel_avg, sizeof(storage->pixel_avg), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: read pixel_avg fail\n");
                    break;
                }

                ret = fread(&storage->pixel_standard_deviation, sizeof(storage->pixel_standard_deviation), 1, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: read pixel_standard_deviation fail\n");
                    break;
                }

                if (storage->pixel_list) {
                    free(storage->pixel_list);
                    storage->pixel_list = NULL;
                }
                storage->pixel_list = (hot_pixel_storage_t *) malloc(storage->number_of_pixel * sizeof(hot_pixel_storage_t));
                if (!storage->pixel_list) {
                    DLOG_ERROR("[error]: malloc fail, request size %d\n", storage->number_of_pixel * sizeof(hot_pixel_storage_t));
                    break;
                }

                ret = fread(storage->pixel_list, sizeof(hot_pixel_storage_t), storage->number_of_pixel, file);
                if (0 > ret) {
                    DLOG_ERROR("[error]: read pixel_list fail\n");
                    break;
                }

                if (ret != (storage->number_of_pixel)) {
                    DLOG_ERROR("[error]: read pixel_list, size %d, %d not match\n", ret, storage->number_of_pixel);
                    ret = (-10);
                    break;
                }

                fclose(file);
            } while (0);
        } else {
            DLOG_ERROR("[error]: open file (%s) fail\n", filename);
            return (-1);
        }
    } else {
        DLOG_ERROR("[error]: bad params, filename %p, storage %p\n", filename, storage);
        return (-2);
    }

    return 0;
}

