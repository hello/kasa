/*******************************************************************************
 * am_platform_iav2_utility.h
 *
 * History:
 *   Aug 12, 2015 - [ypchang] created file
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
#ifndef AM_PLATFORM_IAV2_UTILITY_H_
#define AM_PLATFORM_IAV2_UTILITY_H_

#include "am_encode_types.h"
#include "iav_ioctl.h"

using std::string;

namespace AMVinTransIav2
{
amba_vindev_mirror_pattern_e flip_mw_to_iav(AM_VIDEO_FLIP flip);
AM_VIDEO_FLIP flip_iav_to_mw(amba_vindev_mirror_pattern_e mirror);

amba_video_mode mode_mw_to_iav(AM_VIN_MODE mode);
AM_VIN_MODE mode_iav_to_mw(amba_video_mode mode);

int type_mw_to_iav(AM_VIN_TYPE type);
AM_VIN_TYPE type_iav_to_mw(int type);

int sensor_type_mw_to_iav(AM_SENSOR_TYPE type);
AM_SENSOR_TYPE sensor_type_iav_to_mw(int type);
} /* namespace AMVinTransIav2*/

namespace AMVoutTransIav2
{
amba_vout_sink_type sink_type_mw_to_iav(AM_VOUT_TYPE  type);
uint32_t video_type_mw_to_iav(AM_VOUT_VIDEO_TYPE  type);
int32_t id_mw_to_iav(AM_VOUT_ID id);
amba_video_mode mode_mw_to_iav(AM_VOUT_MODE mode);
amba_vout_flip_info flip_mw_to_iav(AM_VIDEO_FLIP flip);
amba_vout_rotate_info rotate_mw_to_iav(AM_VIDEO_ROTATE rotate);

} /* namespace AMVoutTransIav2*/

namespace AMVideoTransIav2
{
string bitrate_ctrl_method_enum_to_str(AM_BITRATE_CTRL_METHOD bitrate_ctrl);
AM_BITRATE_CTRL_METHOD bitrate_ctrl_method_str_to_enum(const string &str);

string camera_param_mode_enum_to_str(AM_ENCODE_MODE mode);
AM_ENCODE_MODE camera_param_mode_str_to_enum(const string &mode_str);

string camera_param_hdr_enum_to_str(AM_HDR_TYPE type);
AM_HDR_TYPE camera_param_hdr_str_to_enum(const string &type);

string camera_param_iso_enum_to_str(AM_IMAGE_ISO_TYPE hdr);
AM_IMAGE_ISO_TYPE camera_param_iso_str_to_enum(const string &iso_str);

string camera_param_dewarp_enum_to_str(AM_DEWARP_FUNC_TYPE hdr);
AM_DEWARP_FUNC_TYPE camera_param_dewarp_str_to_enum(const string &dewarp_str);

string camera_param_dptz_enum_to_str(AM_DPTZ_TYPE dptz);
AM_DPTZ_TYPE camera_param_dptz_str_to_enum(const string &dptz_str);

string camera_param_overlay_enum_to_str(AM_OVERLAY_TYPE overlay);
AM_OVERLAY_TYPE camera_param_overlay_str_to_enum(const string &overlay_str);

string camera_param_md_enum_to_str(AM_VIDEO_MD_TYPE md);
AM_VIDEO_MD_TYPE camera_param_md_str_to_enum(const std::string &md_str);

iav_srcbuf_type buffer_type_mw_to_iav(AM_SOURCE_BUFFER_TYPE type);
AM_SOURCE_BUFFER_TYPE buffer_type_iav_to_mw(iav_srcbuf_type type);

iav_srcbuf_id buffer_id_mw_to_iav(AM_SOURCE_BUFFER_ID id);
AM_SOURCE_BUFFER_ID buffer_id_iav_to_mw(iav_srcbuf_id id);

iav_dsp_sub_buffer_id dsp_sub_buf_id_mw_to_iav(AM_DSP_SUB_BUF_ID id);
AM_DSP_SUB_BUF_ID dsp_sub_buf_id_iav_to_mw(iav_dsp_sub_buffer_id id);

iav_srcbuf_state buffer_state_mw_to_iav(AM_SRCBUF_STATE state);
AM_SRCBUF_STATE buffer_state_iav_to_mw(iav_srcbuf_state state);

iav_stream_type stream_type_mw_to_iav(AM_STREAM_TYPE type);
AM_STREAM_TYPE stream_type_iav_to_mw(iav_stream_type type);

iav_stream_state stream_state_mw_to_iav(AM_STREAM_STATE state);
AM_STREAM_STATE stream_state_iav_to_mw(iav_stream_state state);

iav_chroma_format h26x_chroma_mw_to_iav(AM_CHROMA_FORMAT format);
AM_CHROMA_FORMAT h26x_chroma_iav_to_mw(iav_chroma_format format);
iav_chroma_format mjpeg_chroma_mw_to_iav(AM_CHROMA_FORMAT format);
AM_CHROMA_FORMAT chroma_iav_to_mw(iav_chroma_format format);

iav_state iav_state_mw_to_iav(AM_IAV_STATE state);
AM_IAV_STATE iav_state_iav_to_mw(iav_state state);

int encode_mode_mw_to_iav(AM_ENCODE_MODE mode);
AM_ENCODE_MODE encode_mode_iav_to_mw(int mode);

int hdr_type_mw_to_iav(AM_HDR_TYPE hdr_type);
AM_HDR_TYPE hdr_type_iav_to_mw(int hdr_type);

uint32_t frame_type_mw_to_iav(AM_VIDEO_FRAME_TYPE type);
AM_VIDEO_FRAME_TYPE frame_type_iav_to_mw(uint32_t type);
} /* namespace AMVideoTransIav2 */


#endif /* AM_PLATFORM_IAV2_UTILITY_H_ */
