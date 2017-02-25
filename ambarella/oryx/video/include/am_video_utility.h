/*******************************************************************************
 * am_video_utility.h
 *
 * History:
 *   2015-7-17 - [ypchang] created file
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
#ifndef AM_VIDEO_UTILITY_H_
#define AM_VIDEO_UTILITY_H_

#include "am_encode_types.h"

#include <string>

using std::string;

/* VIN related helper functions */
namespace AMVinTrans {
string type_enum_to_str(AM_SENSOR_TYPE type);
AM_SENSOR_TYPE type_str_to_enum(const string &type_str);

string mode_enum_to_str(AM_VIN_MODE mode);
AM_VIN_MODE mode_str_to_enum(const string &mode_str);

string hdr_enum_to_str(AM_HDR_TYPE hdr_type);
AM_HDR_TYPE hdr_str_to_enum(const string &hdr_str);

AMResolution mode_to_resolution(AM_VIN_MODE mode);
AM_VIN_MODE resolution_to_mode(const AMResolution &size);

int bits_mw_to_iav(AM_VIN_BITS bits);
AM_VIN_BITS bits_iav_to_mw(int bits);
} /* namepace AMVinTrans */

/* VOUT related helper functions */
namespace AMVoutTrans {
string sink_type_enum_to_str(AM_VOUT_TYPE type);
AM_VOUT_TYPE sink_type_str_to_enum(const string &type_str);

string video_type_enum_to_str(AM_VOUT_VIDEO_TYPE type);
AM_VOUT_VIDEO_TYPE video_type_str_to_enum(const string &type_str);

string mode_enum_to_str(AM_VOUT_MODE mode);
AM_VOUT_MODE mode_str_to_enum(const string &mode_str);

AM_VOUT_MODE resolution_to_mode(const AMResolution &size);
AMResolution mode_to_resolution(AM_VOUT_MODE mode);
uint32_t mode_to_fps(AM_VOUT_MODE mode);
} /* namespace AMVoutTrans */

/* Video related helper functions */
namespace AMVideoTrans {
string stream_type_enum_to_str(AM_STREAM_TYPE type);
AM_STREAM_TYPE stream_type_str_to_enum(string type);

string fps_enum_to_str(AM_VIDEO_FPS fps);
AM_VIDEO_FPS fps_str_to_enum(const string &fps_str);

string flip_enum_to_str(AM_VIDEO_FLIP flip);
AM_VIDEO_FLIP flip_str_to_enum(const string &flip_str);

string rotate_enum_to_str(AM_VIDEO_ROTATE rotate);
AM_VIDEO_ROTATE rotate_str_to_enum(const string &rotate_str);

string stream_profile_enum_to_str(AM_PROFILE profile_level);
AM_PROFILE stream_profile_str_to_enum(const string &profile_str);

string stream_chroma_to_str(AM_CHROMA_FORMAT chroma_format);
AM_CHROMA_FORMAT stream_chroma_str_to_enum(const string &chroma_str);

string stream_bitrate_control_enum_to_str(AM_RATE_CONTROL rate_control);
AM_RATE_CONTROL stream_bitrate_control_str_to_enum(const string &bc_str);

string buffer_type_to_str(AM_SOURCE_BUFFER_TYPE buf_type);
AM_SOURCE_BUFFER_TYPE buffer_type_str_to_enum(const string &type_str);

int get_hdr_expose_num(AM_HDR_TYPE hdr_type);

string iav_state_to_str(AM_IAV_STATE state);

string stream_state_to_str(AM_STREAM_STATE state);

AMVideoFpsFormatQ9 fps_to_q9fps(AM_VIDEO_FPS fps);
AM_VIDEO_FPS q9fps_to_fps(AMVideoFpsFormatQ9 q9fps);
void fps_to_factor(AM_VIDEO_FPS vin_fps, int32_t encode_fps,
                   int32_t &mul, int32_t &div);
int32_t factor_to_encfps(AM_VIDEO_FPS vin_fps, int32_t mul, int32_t div);
} /* namespace AMVideoTrans */

#endif /* AM_VIDEO_UTILITY_H_ */
