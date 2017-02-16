/*******************************************************************************
 * am_video_trans.h
 *
 * Histroy:
 *   Sep 1, 2014 - [Shupeng Ren] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_VIDEO_TRANS_H_
#define AM_VIDEO_TRANS_H_


std::string video_flip_enum_to_str(AM_VIDEO_FLIP flip);
std::string video_fps_enum_to_str(AM_VIDEO_FPS  fps);
std::string stream_format_enum_to_str(AM_ENCODE_STREAM_TYPE type);
std::string stream_fps_to_str(AMStreamFps *fps);
std::string stream_bitrate_control_enum_to_str(AM_ENCODE_H264_RATE_CONTROL rate_control);
std::string stream_profile_level_enum_to_str(AM_ENCODE_H264_PROFILE profile_level);
std::string stream_chroma_to_str(AM_ENCODE_CHROMA_FORMAT chroma_format);
std::string source_buffer_type_to_str(AM_ENCODE_SOURCE_BUFFER_TYPE buf_type);

AMStreamFps stream_frame_factor_str_to_fps(const char *frame_factor_str);
AM_VIDEO_FPS video_fps_str_to_enum(const char *fps_str);
AM_VIDEO_FLIP video_flip_str_to_enum(const char *flip_str);
AM_VIDEO_ROTATE video_rotate_str_to_enum(const char *rotate_str);
AM_ENCODE_H264_PROFILE stream_profile_str_to_enum(const char *profile_str);
AM_ENCODE_CHROMA_FORMAT stream_chroma_str_to_enum(const char *chroma_str);
AM_ENCODE_H264_RATE_CONTROL stream_bitrate_control_str_to_enum(const char *bc_str);
AM_ENCODE_SOURCE_BUFFER_TYPE buffer_type_str_to_enum(const char *type_str);
AMVideoFpsFormatQ9 get_amba_video_fps(AM_VIDEO_FPS fps);

AM_ENCODE_MODE camera_param_mode_str_to_enum(const char *mode_str);
AM_HDR_EXPOSURE_TYPE camera_param_hdr_str_to_enum(const char *hdr_str);
AM_IMAGE_ISO_TYPE camera_param_iso_str_to_enum(const char *iso_str);
AM_DEWARP_TYPE camera_param_dewarp_str_to_enum(const char *dewarp_str);
std::string camera_param_mode_enum_to_str(AM_ENCODE_MODE mode);
std::string camera_param_hdr_enum_to_str(AM_HDR_EXPOSURE_TYPE hdr);
std::string camera_param_iso_enum_to_str(AM_IMAGE_ISO_TYPE hdr);
std::string camera_param_dewarp_enum_to_str(AM_DEWARP_TYPE hdr);
#endif /* AM_VIDEO_TRANS_H_ */
