//dict for vin

//[0] is main key, followed by sub keys
char* pipeline_dict[] = 	{"pipeline", 	"enc_mode", "raw2enc", "raw_cap", "raw_max_width", "raw_max_height", "exposure_num", "duration", "next_cfg_file"};
char* vin_dict[] = 		{"vin", 	"sensor_id", "video_mode", "hdr_mode", "width", "height", "fps", "bit_resolution", "mirror", "bayer_pattern", "sensor_name"};
char* vout0_dict[] = 		{"vout0", 	"enable", "input", "mode", "width", "height", "device", "lcd_model_index"};
char* vout1_dict[] = 		{"vout1", 	"enable", "input", "mode", "width", "height", "device", "lcd_model_index"};
char* osd_dict[] =		{"osd", 	"from_mixer_a", "from_mixer_b"};
//char* dummy_win_dict[] = 	{"dummy_win", 	"width", "height"};
//char* active_win_dict[] = 		{"active_win", 	"width", "height"};
char* main_buf_dict[] = 	{"main_buf", 	"input_width", "input_height", "input_offset_x", "input_offset_y", "max_width", "max_height", "width", "height", "type"};
char* second_buf_dict[] = 	{"second_buf", 	"input_width", "input_height", "input_offset_x", "input_offset_y", "max_width", "max_height", "width", "height", "type"};
char* third_buf_dict[] = 	{"third_buf", 	"input_width", "input_height", "input_offset_x", "input_offset_y", "max_width", "max_height", "width", "height", "type"};
char* fourth_buf_dict[] = 	{"fourth_buf", 	"input_width", "input_height", "input_offset_x", "input_offset_y", "max_width", "max_height", "width", "height", "type"};

char* stream_a_dict[] = 	{"stream_a", 	"width", "height", "max_width", "max_height", "m", "n", "bit_rate", "quality", "h_flip", "v_flip", "rotate", "fps_ratio", "source", "type"};
char* stream_b_dict[] = 	{"stream_b", 	"width", "height", "max_width", "max_height", "m", "n", "bit_rate", "quality", "h_flip", "v_flip", "rotate", "fps_ratio", "source", "type"};
char* stream_c_dict[] = 	{"stream_c", 	"width", "height", "max_width", "max_height", "m", "n", "bit_rate", "quality", "h_flip", "v_flip", "rotate", "fps_ratio", "source", "type"};
char* stream_d_dict[] = 	{"stream_d", 	"width", "height", "max_width", "max_height", "m", "n", "bit_rate", "quality", "h_flip", "v_flip", "rotate", "fps_ratio", "source", "type"};
