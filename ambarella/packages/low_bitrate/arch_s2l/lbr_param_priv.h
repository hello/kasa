/*  lbr_param_priv.h */
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
