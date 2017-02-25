/*******************************************************************************
 * lbr_param_priv.h
 *
 * History:
 *	2014/05/02 - [Jian Tang] created file
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

#ifndef __LBR_PARAM_PRIV_H__
#define  __LBR_PARAM_PRIV_H__

static lbr_control_t g_lbr_control_param_IPPP_GOP[LBR_PROFILE_NUM] =
{
	{	//PROFILE 0 : LBR_PROFILE_STATIC
		.bitrate_target =
		{
			.bitrate_1080p = 100* 1024,
			.bitrate_720p = 80 * 1024,
			.bitrate_per_MB = 23,
		},
		.zmv_threshold = 32,
		.I_qp_reduce = 4,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
	{	//PROFILE 1 : LBR_PROFILE_SMALL_MOTION
		.bitrate_target =
		{
			.bitrate_1080p = 600* 1024,
			.bitrate_720p = 400 * 1024,
			.bitrate_per_MB = 114,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
	{	//PROFILE 2 : LBR_PROFILE_BIG_MOTION
		.bitrate_target =
		{
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
	{	//PROFILE 3 : LBR_PROFILE_LOW_LIGHT
		.bitrate_target =
		{
			.bitrate_1080p = 800* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 32,
		.I_qp_reduce = 1,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
	{	//PROFILE 4 : LBR_PROFILE_BIG_MOTION_WITH_FRAME_DROP
		.bitrate_target =
		{
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 4,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 36,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 36,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 36,
		.skip_frame_mode = 6,
		.adapt_qp = 4,
	},
	{	//PROFILE 5 : LBR_PROFILE_SECURIY_IPCAM_CBR
		.bitrate_target =
		{
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 0,
		.I_qp_reduce = 1,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
	{	//PROFILE 6 : LBR_PROFILE_RESERVE2
		.bitrate_target =
		{
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
	{	//PROFILE 7 : LBR_PROFILE_RESERVE3
		.bitrate_target =
		{
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
};

static lbr_control_t g_lbr_control_param_IBBP_GOP[LBR_PROFILE_NUM] =
{
	{	//PROFILE 0 : LBR_PROFILE_STATIC
		.bitrate_target =
		{
			.bitrate_1080p = 100* 1024,
			.bitrate_720p = 80 * 1024,
			.bitrate_per_MB = 23,
		},
		.zmv_threshold = 16,
		.I_qp_reduce = 8,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
	{	//PROFILE 1 : LBR_PROFILE_SMALL_MOTION
		.bitrate_target =
		{
			.bitrate_1080p = 500* 1024,
			.bitrate_720p = 300 * 1024,
			.bitrate_per_MB = 85,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
	{	//PROFILE 2 : LBR_PROFILE_BIG_MOTION
		.bitrate_target =
		{
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
	{	//PROFILE 3 : LBR_PROFILE_LOW_LIGHT
		.bitrate_target =
		{
			.bitrate_1080p = 600* 1024,
			.bitrate_720p = 400 * 1024,
			.bitrate_per_MB = 114,
		},
		.zmv_threshold = 32,
		.I_qp_reduce = 1,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
	{	//PROFILE 4 : LBR_PROFILE_BIG_MOTION_WITH_FRAME_DROP
		.bitrate_target =
		{
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 36,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 36,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 36,
		.skip_frame_mode = 6,
		.adapt_qp = 4,
	},
	{	//PROFILE 5 : LBR_PROFILE_SECURIY_IPCAM_CBR
		.bitrate_target =
		{
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 0,
		.I_qp_reduce = 1,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
	{	//PROFILE 6 : LBR_PROFILE_RESERVE2
		.bitrate_target =
		{
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
	{	//PROFILE 7 : LBR_PROFILE_RESERVE3
		.bitrate_target =
		{
			.bitrate_1080p = 1024* 1024,
			.bitrate_720p = 500 * 1024,
			.bitrate_per_MB = 142,
		},
		.zmv_threshold = 8,
		.I_qp_reduce = 1,
		.I_qp_limit_min = 10,
		.I_qp_limit_max = 51,
		.P_qp_limit_min = 14,
		.P_qp_limit_max = 51,
		.B_qp_limit_min = 14,
		.B_qp_limit_max = 51,
		.skip_frame_mode = 0,
		.adapt_qp = 4,
	},
};

#endif
