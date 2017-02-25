/*******************************************************************************
 * internal_include.h
 *
 * History:
 *	2016/03/28 - [Zhi He] create file
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

#ifndef __INTERNAL_INCLUDE_H__
#define __INTERNAL_INCLUDE_H__

#define DMAX_INPUT_BUFFER_NUMBER 16

#define DUNIQUEID_MARKER_BYTE 0xAB

#define DUNIQUEID_DIGEST_SHA256_LENGTH 32
#define DUNIQUEID_DIGEST_MD5_LENGTH 16
#define DUNIQUEID_PARAM_PART_LENGTH 16

#define DUNIQUEID_GENERATE_ADDITIONAL_TIMES 6

#define DMAX_PIXEL_VALUE (1 << 14)

#define DLOG_DEBUG(format, args...) (void)0
//#define DLOG_DEBUG printf

#define DLOG_ALWAYS __DLOG_ALWAYS
#define DLOG_ERROR __DLOG_ERROR

#define __DLOG_ERROR(format, args ...) do { \
    printf("\033[40;31m\033[1m" format "\033[0m", ##args); \
    printf("\t\t\[trace] file: %s.\n\t\t\t\tfunction: %s, line %d\n", __FILE__, __FUNCTION__, __LINE__); \
    fflush(stdout); \
} while (0)

#define __DLOG_ALWAYS(format, args ...) do { \
    printf("\033[40;35m\033[1m" format "\033[0m", ##args); \
    fflush(stdout); \
} while (0)

typedef unsigned short type_pixel_t;
typedef unsigned int type_accumulate_pixel_t;
typedef unsigned short type_dimension_t;
typedef unsigned long long type_caluculation_t;

typedef struct {
    unsigned char digest_sha256[DUNIQUEID_DIGEST_SHA256_LENGTH];
    unsigned char digest_md5[DUNIQUEID_DIGEST_MD5_LENGTH];

    unsigned char param_sensor_name[DUNIQUEID_MAX_NAME_LENGTH];
    unsigned char param_resolution_width[2];
    unsigned char param_resolution_height[2];
    unsigned char param_hot_pixel_number[2];
    unsigned char param_input_frame_number;
    unsigned char param_repeat_times;
} uniqueid_sensor_bpp_internal_t;

typedef struct {
    type_dimension_t width, height;
    type_pixel_t *p_buf;
} data_buffer_t;

typedef struct {
    type_dimension_t width, height;
    type_accumulate_pixel_t *p_buf;
} accumulate_buffer_t;

typedef struct {
    type_dimension_t width, height;
    type_accumulate_pixel_t *p_buf;
} statstics_accumulate_t;

typedef struct {
    type_dimension_t x, y;
    type_pixel_t value;
    type_pixel_t delta;

    unsigned int acc_value;
    unsigned int acc_delta;
    unsigned int acc_count;

    type_pixel_t delta_max;
    type_pixel_t delta_min;
    type_pixel_t delta_deviation;
} hot_pixel_t;

//data source
typedef int (*tf_query_dimension)(void *context, type_dimension_t *width, type_dimension_t *height);
typedef int (*tf_read_data)(void *context, data_buffer_t *buf);
typedef void (*tf_destroy)(void *context);

typedef struct {
    tf_query_dimension f_query_dimension;
    tf_read_data f_read_raw_data;
    tf_destroy f_destroy;
} data_source_base_t;

typedef enum {
    E_DATA_SOURCE_INVALID = 0x0,
    E_DATA_SOURCE_SENSOR_RAW = 0x01,
} E_DATA_SOURCE;

data_source_base_t *create_data_source(E_DATA_SOURCE data_source);

//storage
typedef struct {
    type_dimension_t x, y;
    unsigned short delta;
    unsigned short delta_deviation;
} hot_pixel_storage_t;

typedef struct {
    unsigned char param_sensor_name[DUNIQUEID_MAX_NAME_LENGTH];
    unsigned short param_resolution_width;
    unsigned short param_resolution_height;
    unsigned short param_hot_pixel_number;
    unsigned char param_input_frame_number;
    unsigned char param_repeat_times;

    unsigned short number_of_pixel;
    unsigned char reserved0;
    unsigned char marker_byte; // should be DUNIQUEID_MARKER_BYTE

    type_pixel_t pixel_avg;
    type_pixel_t pixel_standard_deviation;

    hot_pixel_storage_t *pixel_list;
} hot_pixel_list_storage_t;

//data statistics
int calculate_buffer_statistics(data_buffer_t *buf, double *average, double *standard_deviation);
int calculate_buffer_pixel_distribution(data_buffer_t *buf, unsigned int *distribution_end);
int estimate_threshold(unsigned int *distribution, unsigned int range,
    type_pixel_t *high_threshold, unsigned int *high_pixel_number);
int estimate_threshold_high_low(unsigned int *distribution, unsigned int range,
    type_pixel_t *high_threshold, type_pixel_t *low_threshold,
    unsigned int *high_pixel_number, unsigned int *low_pixel_number);

//buffer related
int add_buffer(accumulate_buffer_t *acc_buf, data_buffer_t *buf1, data_buffer_t *buf2);
int accumulate_buffer(accumulate_buffer_t *acc_buf, data_buffer_t *buf);
int average_buffer(accumulate_buffer_t *acc_buf, data_buffer_t *buf, unsigned int buffer_number);

//io related
int write_hot_pixel_list_to_file(char *filename, hot_pixel_list_storage_t *storage);
int read_hot_pixel_list_from_file(char *filename, hot_pixel_list_storage_t *storage);

#endif

