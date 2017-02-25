#ifndef __AMBA_COMM_H_
#define __AMBA_COMM_H_

#define H264_BUF_SIZE (2*1024*1024)
#define MJPEG_BUF_SIZE		(2*1024*1024)
#define BUF_SIZE (MJPEG_BUF_SIZE > H264_BUF_SIZE ? MJPEG_BUF_SIZE:H264_BUF_SIZE)
#define DEFAULT_HOST "10.0.0.2"
#define RECORD_FILENAME_LENGTH 256
#define DEFAULT_FILE_NAME "c:\\media\\test"
#define DEFAULT_STAT_WINDOW_SIZE 60

// must keep same with driver
enum ENC_TYPE
{
	ENC_NONE,
	ENC_H264,
	ENC_MJPEG,
	ENC_OTHERS,
};

enum REC_TYPE
{
	REC_ORG,
	REC_MP4,
	REC_F4V,
};

enum TRANS_TYPE
{
	TRANS_UDP,
	TRANS_TCP,
	TRANS_MULTI,
};

struct ENC_STAT
{
	float avg_fps;
	int avg_bitrate;
	int avg_pts;
	unsigned int window_size;
};

struct SLICE_INFO
{
	long long pts;
	int data_size;
	short nal_unit_type;
	bool pts_valid;
};

class CParseH264
{
public:

	typedef struct h264_param_s
	{
		unsigned short pic_width;
		unsigned short pic_height;
		float frame_rate; // reversed
	}h264_param_t;

	#define parse_ue(x,y,z)		parse_exp_codes((x),(y),(z),0)
	#define parse_se(x,y,z)		parse_exp_codes((x),(y),(z),1)
	#define u8 unsigned char

	static int read_bit(unsigned char* pBuffer,  int* value, u8* bit_pos=0, int num = 1)
	{
		if (num <0)
		{
			return -1;
		}
		*value = 0;
		int i=0;
		for (int j =0 ; j<num; j++)
		{
			if (*bit_pos == 8)
			{
				*bit_pos = 0;
				i++;
			}
			*value  <<= 1;
			*value  += pBuffer[i] >> (7 -(*bit_pos)++) & 0x1;
		}
		return i;
	};

	static int read_bit(unsigned char* pBuffer,  u8* value, u8* bit_pos=0, int num = 1)
	{
		if (num > 8 || num < 1)
		{
			return -1;
		}
		*value = 0;
		int i=0;
		for (int j =0 ; j<num; j++)
		{
			if (*bit_pos == 8)
			{
				*bit_pos = 0;
				i++;
			}
			*value <<= 1;
			*value += pBuffer[i] >> (7 -(*bit_pos)++) & 0x1;
		}
		return i;
	};

	static int parse_exp_codes(unsigned char* pBuffer, int* value,  u8* bit_pos=0,u8 type=0)
	{
		int leadingZeroBits = -1;
		int i=0;
		u8 j=*bit_pos;
		for (bool b = 0; !b; leadingZeroBits++,j++)
		{
			if(j == 8)
			{
				i++;
				j=0;
			}
			b = pBuffer[i] >> (7 -j) & 0x1;
		}
		int codeNum = 0;
		i += read_bit(pBuffer+i,  &codeNum, &j, leadingZeroBits);
		codeNum += (1 << leadingZeroBits) -1;
		if (type == 0) //ue(v)
		{
			*value = codeNum;
		}
		else if (type == 1) //se(v)
		{
			*value = (codeNum/2+1);
			if (codeNum %2 == 0)
			{
				*value *= -1;
			}
		}
		*bit_pos = j;
		return i;
	}

	static short  get_nal_unit_type(unsigned char* pBuffer , int offset = 0)
	{
		short  nal_unit_type = pBuffer[offset] & 0x1F;
		return nal_unit_type;
	};

	static int get_nal_unit_type_length(short* nal_unit_type, unsigned char* pBuffer , int size)
	{
		if (size < 5) 
		{
			return 0;
		}
		int code, tmp, pos;
		for (code=0xffffffff, pos = 0; pos <4; pos++)
		{
			tmp = pBuffer[pos];
			code = (code<<8)|tmp;
		}
		if (code != 0x00000001) // check start code 0x00000001
		{
			return 0;
		}
		
		*nal_unit_type = pBuffer[pos++] & 0x1F;
		for (code=0xffffffff; pos < size; pos++)
		{
			tmp = pBuffer[pos];
			if ((code=(code<<8)|tmp) == 0x00000001)
			{
				break;//found next start code
			}
		}
		if (pos == size )
		{
			return size;
		}
		else
		{
			return pos-4+1;
		}
	};

	static int parse_scaling_list(u8* pBuffer,int sizeofScalingList,u8* bit_pos)
	{
		int* scalingList = new int[sizeofScalingList];
		bool useDefaultScalingMatrixFlag;
		int lastScale = 8;
		int nextScale = 8;
		int delta_scale;
		int i = 0;
		for (int j =0; j<sizeofScalingList; j++ )
		{
			if (nextScale != 0)
			{
				i += parse_se(pBuffer, &delta_scale,bit_pos);
				nextScale = (lastScale+ delta_scale + 256)%256;
				useDefaultScalingMatrixFlag = ( j == 0 && nextScale == 0 );
			}
			scalingList[j] = ( nextScale == 0 ) ? lastScale : nextScale;
			lastScale = scalingList[j];
		}
		delete[] scalingList;
		return i;
	};

	static int  get_pic_width_height(unsigned char* pBuffer, int size, h264_param_t* pH264Param)
	{
		u8* pSPS = pBuffer;
		u8 bit_pos = 0;
		u8 profile_idc; // u(8)
		u8 constraint_set; //u(8)
		u8 level_idc; //u(8)
		int seq_paramter_set_id = 0;
		int chroma_format_idc = 0;
		bool residual_colour_tramsform_flag = false;
		int bit_depth_luma_minus8 = 0;
		int bit_depth_chroma_minus8 = 0;
		bool qpprime_y_zero_transform_bypass_flag = false;
		bool seq_scaling_matrix_present_flag = false;
		bool seq_scaling_list_present_flag[8] ;
		int log2_max_frame_num_minus4;
		int pic_order_cnt_type;
		int log2_max_pic_order_cnt_lsb_minus4;
		int num_ref_frames;
		bool gaps_in_frame_num_value_allowed_flag = false;
		int pic_width_in_mbs_minus1;
		int pic_height_in_map_units_minus1;
		bool frame_mbs_only_flag = false;
		bool direct_8x8_inference_flag = false;

		pSPS += read_bit(pSPS, &profile_idc,&bit_pos,8);
		pSPS += read_bit(pSPS, &constraint_set,&bit_pos,8);
		pSPS += read_bit(pSPS, &level_idc, &bit_pos,8);
		pSPS += parse_ue(pSPS,&seq_paramter_set_id, &bit_pos);
		if (profile_idc == 100 ||profile_idc == 110 ||
			profile_idc == 122 ||profile_idc == 144 )
		{
			pSPS += parse_ue(pSPS,&chroma_format_idc,&bit_pos);
			if (chroma_format_idc == 3)
			{
				pSPS += read_bit(pSPS, (u8 *)&residual_colour_tramsform_flag, &bit_pos);
				pSPS += parse_ue(pSPS,&bit_depth_luma_minus8,&bit_pos);
				pSPS += read_bit(pSPS,(u8 *)&qpprime_y_zero_transform_bypass_flag,&bit_pos);
				pSPS += read_bit(pSPS,(u8 *)&seq_scaling_matrix_present_flag,&bit_pos);
				if (seq_scaling_matrix_present_flag )
				{
					for (int i = 0; i<8; i++)
					{
						pSPS += read_bit(pSPS,(u8 *)&seq_scaling_list_present_flag[i],&bit_pos);
						if (seq_scaling_list_present_flag[i])
						{
							if (i < 6)
							{
								pSPS += parse_scaling_list(pSPS,16,&bit_pos);
							}
							else
							{
								pSPS += parse_scaling_list(pSPS,64,&bit_pos);
							}
						}
					}
				}
			}
		}
		pSPS += parse_ue(pSPS,&log2_max_frame_num_minus4,&bit_pos);
		pSPS += parse_ue(pSPS,&pic_order_cnt_type,&bit_pos);
		if (pic_order_cnt_type == 0)
		{
			pSPS += parse_ue(pSPS,&log2_max_pic_order_cnt_lsb_minus4,&bit_pos);
		}
		else if (pic_order_cnt_type == 1)
		{
			bool delta_pic_order_always_zero_flag = false;
			pSPS += read_bit(pSPS, (u8 *)&delta_pic_order_always_zero_flag, &bit_pos);
			int offset_for_non_ref_pic;
			int offset_for_top_to_bottom_field;
			int num_ref_frames_in_pic_order_cnt_cycle;
			pSPS += parse_se(pSPS,&offset_for_non_ref_pic, &bit_pos);
			pSPS += parse_se(pSPS,&offset_for_top_to_bottom_field, &bit_pos);
			pSPS += parse_ue(pSPS,&num_ref_frames_in_pic_order_cnt_cycle, &bit_pos);
			int* offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
			for (int i =0;i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
			{
				pSPS += parse_se(pSPS,offset_for_ref_frame+i, &bit_pos);
			}
			delete[] offset_for_ref_frame;
		}
		pSPS += parse_ue(pSPS,&num_ref_frames, &bit_pos);
		pSPS += read_bit(pSPS, (u8 *)&gaps_in_frame_num_value_allowed_flag, &bit_pos);
		pSPS += parse_ue(pSPS,&pic_width_in_mbs_minus1,&bit_pos);
		pSPS += parse_ue(pSPS,&pic_height_in_map_units_minus1, &bit_pos);

		pH264Param->pic_width = (short)(pic_width_in_mbs_minus1 + 1) << 4;
		pH264Param->pic_height = (short)(pic_height_in_map_units_minus1 + 1) <<4;
		
		pSPS += read_bit(pSPS, (u8 *)&frame_mbs_only_flag, &bit_pos);
		if (!frame_mbs_only_flag)
		{
			bool mb_adaptive_frame_field_flag = false;
			pSPS += read_bit(pSPS, (u8 *)&mb_adaptive_frame_field_flag, &bit_pos);
			pH264Param->pic_height *= 2;
		}
		#if 0
		bool frame_cropping_flag;
		bool vui_parameters_present_flag;
		pSPS += read_bit(pSPS, (u8 *)&direct_8x8_inference_flag, &bit_pos);
		pSPS += read_bit(pSPS, (u8 *)&frame_cropping_flag, &bit_pos);
		if (frame_cropping_flag)
		{
			int frame_crop_left_offset;
			int frame_crop_right_offset;
			int frame_crop_top_offset;
			int frame_crop_bottom_offset;
			pSPS += parse_ue(pSPS,&frame_crop_left_offset, &bit_pos);
			pSPS += parse_ue(pSPS,&frame_crop_right_offset, &bit_pos);
			pSPS += parse_ue(pSPS,&frame_crop_top_offset, &bit_pos);
			pSPS += parse_ue(pSPS,&frame_crop_bottom_offset, &bit_pos);
		}
		pSPS += read_bit(pSPS, (u8 *)&vui_parameters_present_flag, &bit_pos);
		if (vui_parameters_present_flag)
		{
			bool aspect_ratio_info_present_flag;
			pSPS += read_bit(pSPS, (u8 *)&aspect_ratio_info_present_flag, &bit_pos);
			if (aspect_ratio_info_present_flag)
			{
				u8 aspect_ratio_idc;
				pSPS += read_bit(pSPS, &aspect_ratio_idc,&bit_pos,8);
				if (aspect_ratio_idc == 255) // Extended_SAR
				{
					int sar_width;
					int sar_height;
					pSPS += read_bit(pSPS, &sar_width, &bit_pos, 16);
					pSPS += read_bit(pSPS, &sar_height, &bit_pos, 16);
				}
				bool overscan_info_present_flag = false;
				pSPS += read_bit(pSPS, (u8 *)&overscan_info_present_flag, &bit_pos);
				if (overscan_info_present_flag)
				{
					bool overscan_appropriate_flag = false;
					pSPS += read_bit(pSPS, (u8 *)&overscan_appropriate_flag, &bit_pos);
				}
				bool video_signal_type_present_flag;
				pSPS += read_bit(pSPS, (u8 *)&video_signal_type_present_flag, &bit_pos);
				if (video_signal_type_present_flag)
				{
					u8 video_format;
					pSPS += read_bit(pSPS, (u8 *)&video_format, &bit_pos,3);
					bool video_full_range_flag;
					pSPS += read_bit(pSPS, (u8 *)&video_full_range_flag, &bit_pos);
					bool video_signal_type_present_flag;
					pSPS += read_bit(pSPS, (u8 *)&video_signal_type_present_flag, &bit_pos);
					if (video_signal_type_present_flag)
					{
						u8 colour_primaries, transfer_characteristics,matrix_coefficients;
						pSPS += read_bit(pSPS, (u8 *)&colour_primaries, &bit_pos, 8);
						pSPS += read_bit(pSPS, (u8 *)&transfer_characteristics, &bit_pos, 8);
						pSPS += read_bit(pSPS, (u8 *)&matrix_coefficients, &bit_pos, 8);
					}
				}
				bool chroma_loc_info_present_flag = false;
				pSPS += read_bit(pSPS, (u8 *)&chroma_loc_info_present_flag, &bit_pos);
				if( chroma_loc_info_present_flag )
				{
					int chroma_sample_loc_type_top_field;
					int chroma_sample_loc_type_bottom_field;
					pSPS += parse_ue(pSPS,&chroma_sample_loc_type_top_field, &bit_pos);
					pSPS += parse_ue(pSPS,&chroma_sample_loc_type_bottom_field, &bit_pos);
				}
				bool timing_info_present_flag = false;
				pSPS += read_bit(pSPS, (u8 *)&timing_info_present_flag, &bit_pos);
				if (timing_info_present_flag)
				{
					int num_units_in_tick,time_scale;
					bool fixed_frame_rate_flag;
					pSPS += read_bit(pSPS, (u8 *)&num_units_in_tick, &bit_pos, 32);
					pSPS += read_bit(pSPS, (u8 *)&time_scale, &bit_pos, 32);
					pSPS += read_bit(pSPS, (u8 *)&fixed_frame_rate_flag, &bit_pos);
					if (fixed_frame_rate_flag)
					{
						u8 divisor = 2; //when pic_struct_present_flag == 1 && pic_struct == 0
						pH264Param->frame_rate = (float)time_scale/num_units_in_tick/divisor;
					}
				}
			}
		}
		#endif
		return 0;
	};
};
#endif //__AMBA_COMM_H_

