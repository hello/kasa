#ifndef CFG_SRV_H
#define CFG_SRV_H

//str field must be the last one, and only 1 str field is allowed, since "get" uses int_ptr++
typedef struct{
	int sensor_id;
	int video_mode;
	int hdr_mode;
	int width;
	int height;
	int fps;
	int bit_resolution;
	int mirror;
	int bayer_pattern;
	char sensor_name[64];
}vin_t;

typedef struct{
	int enable;
	int input;
	int mode;
	int width;
	int height;
	int device;
	int lcd_model_index;
}vout_t;

typedef struct{
	int from_mixer_a;
	int from_mixer_b;
}osd_t;

typedef struct{
	int enc_mode;
	int raw2enc;
	int raw_cap;
	int raw_max_width;
	int raw_max_height;
	int exposure_num;
	int duration;
	char next_cfg_file[64];
}pipeline_t;

typedef struct{
	int input_width;
	int input_height;
	int input_offset_x;
	int input_offset_y;
	int max_width;
	int max_height;
	int width;
	int height;
	int type;
}buffer_t;

typedef struct{
	int width;
	int height;
	int max_width;
	int max_height;
	int m;
	int n;
	int bit_rate;
	int quality;
	int h_flip;
	int v_flip;
	int rotate;
	int fps_ratio;
	int source;
	int type;
}stream_t;

#endif
