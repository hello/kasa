#ifndef __AMBA_FDET_H__
#define __AMBA_FDET_H__

enum fdet_ioctl_id
{
	FDET_IOCTL_START		= 0,
	FDET_IOCTL_STOP,
	FDET_IOCTL_GET_RESULT		= 3,
	FDET_IOCTL_SET_MMAP_TYPE,
	FDET_IOCTL_SET_CONFIGURATION,
	FDET_IOCTL_TRACK_FACE,
};

enum fdet_mmap_type {
	FDET_MMAP_TYPE_ORIG_BUFFER	= 0,
	FDET_MMAP_TYPE_TMP_BUFFER,
	FDET_MMAP_TYPE_CLASSIFIER_BINARY,
};

enum fdet_mode {
	FDET_MODE_VIDEO			= 0,
	FDET_MODE_STILL,
};

enum fdet_filtering_policy {
	FDET_POLICY_WAIT_FS_COMPLETE		= (1 << 0),
	FDET_POLICY_DISABLE_TS			= (1 << 1),
	FDET_POLICY_DEBUG			= (1 << 2),
	FDET_POLICY_MEASURE_TIME		= (1 << 3),
};

enum fdet_result_type {
	FDET_RESULT_TYPE_FS		= 0,
	FDET_RESULT_TYPE_TS,
};

struct fdet_configuration {
	int				input_width;
	int				input_height;
	int				input_pitch;
	int				input_source;
	unsigned int			input_offset;
	enum fdet_mode			input_mode;

	enum fdet_filtering_policy	policy;
};

struct fdet_face {
	unsigned short			x;
	unsigned short			y;
	unsigned short			size;
	enum fdet_result_type		type;
};

#endif
