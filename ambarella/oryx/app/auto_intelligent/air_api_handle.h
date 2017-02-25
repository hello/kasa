/*******************************************************************************
 * air_api_handle.h
 *
 * History:
 *   Nov 19, 2015 - [Huaiqing Wang] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
#ifndef ORYX_APP_AUTO_INTELLIGENT_AIR_API_HANDLE_H_
#define ORYX_APP_AUTO_INTELLIGENT_AIR_API_HANDLE_H_

#include "common.h"

int32_t api_helper_init();
int32_t add_stream_overlay(const overlay_layout_desc *position,
                           const char *bmp_file);
int32_t delete_stream_overlay(uint32_t area_id);
int32_t destroy_stream_overlay();
int32_t start_encode();
int32_t stop_encode();
int32_t set_input_size_dynamic(int32_t buf_id, const buffer_crop_desc *rect);
int32_t set_source_buffer_size(int32_t buf_id, const buffer_crop_desc *rect);
int32_t get_gray_frame(int32_t buf_id, gray_frame_desc *data);
int32_t init_bmp_overlay_area(const point_desc &offset, const size_desc &size);
int32_t add_bmp_to_area(const bmp_overlay_manipulate_desc &bmp);
int32_t render_bmp_overlay_area(int32_t area_id);
int32_t update_bmp_overlay_area(const bmp_overlay_manipulate_desc &bmp);

#endif /* ORYX_APP_AUTO_INTELLIGENT_AIR_API_HANDLE_H_ */
