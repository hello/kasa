#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <getopt.h>
#include <sched.h>
#include <signal.h>
#include "basetypes.h"


#define CUSTOMER_MAINLOOP
#include "iav_ioctl.h"
#include "ambas_imgproc_arch.h"
#include "AmbaDSP_Img3aStatistics.h"
#include "AmbaDSP_ImgUtility.h"

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "img_customer_interface_arch.h"
#include <pthread.h>
#include "idsp_netlink.h"
#include "mn34220pl_aliso_adj_param.c"
#include "mn34220pl_mliso_adj_param.c"
#include "mn34220pl_liso_adj_param.c"
#include "mn34220pl_aeb_param.c"

#define	IMGPROC_PARAM_PATH	"/etc/idsp"
#ifndef AWB_UNIT_GAIN
#define AWB_UNIT_GAIN	1024
#endif
#ifndef ABS
#define ABS(a)	   (((a) < 0) ? -(a) : (a))
#endif
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
static pthread_t nl_thread;
static int fd_iav =-1;
static amba_img_dsp_mode_cfg_t ik_mode;
static amba_dsp_aaa_statistic_data_t g_stat;
static img_aaa_stat_t aaa_stat_info[MAX_HDR_EXPOSURE_NUM];
static awb_gain_t awb_gain[MAX_HDR_EXPOSURE_NUM];
static ae_output_t ae_output[MAX_HDR_EXPOSURE_NUM];
static img_awb_param_t is_awb_param;

static struct rgb_aaa_stat rgb_stat[4];
static struct cfa_aaa_stat cfa_stat[4];
static struct cfa_pre_hdr_stat hdr_cfa_pre_stat[MAX_PRE_HDR_STAT_BLK_NUM];
static u8 rgb_stat_valid;
static u8 cfa_stat_valid;
static u8 hdr_cfa_pre_valid;

static aaa_tile_report_t act_tile;
amba_img_dsp_ae_stat_info_t ae_tile_config;
amba_img_dsp_awb_stat_info_t awb_tile_config;
amba_img_dsp_af_stat_info_t af_tile_config;
static amba_img_dsp_hdr_stat_info_t hdr_stat_config;
static amba_img_dsp_hist_stat_info_t hist_stat_config;

static pthread_t id;
static img_config_info_t cus_work_info;
static raw_offset_config_t cus_offset_cfg;
static hdr_proc_data_t cus_hdr_proc_pipe;
static u8 exit_flag =0;
static amba_img_dsp_tone_curve_t cus_dyn_tone;
extern int coll_transmit_filter(void* filter, void* cmd, void* usr1, void* usr2);
void cus_main_loop(void* arg)
{
	int fd_iav;
	fd_iav = (int)arg;
	hdr_pipeline_t hdr_pipeline = cus_work_info.hdr_config.pipeline;
	isp_pipeline_t isp_pipeline = cus_work_info.isp_pipeline;

	int adj_index =0;

	/* enable dynamic tone curve control or not */
	/*dynamic_tone_curve_enable(1);
	dynamic_tone_curve_set_alpha(0.7);*/

	awb_flow_control_init(&is_awb_param);

	while(!exit_flag){
		if(amba_img_dsp_3a_get_aaa_stat(fd_iav,&ik_mode,&g_stat)<0){
			printf("amba_img_dsp_3a_get_aaa_stat fail\n");
			continue;
		}
		if(parse_aaa_data(&g_stat, hdr_pipeline, aaa_stat_info, &act_tile)<0){
			printf("parse_aaa_data fail\n");
			continue;
		}

		//*******awb algo********//
		awb_flow_control(aaa_stat_info,awb_gain); //awb algo
		static amba_img_dsp_wb_gain_t wb_gain={WB_DGAIN_UNIT,WB_DGAIN_UNIT,WB_DGAIN_UNIT,WB_DGAIN_UNIT,WB_DGAIN_UNIT};
		if(awb_gain[0].video_gain_update ==1){
			if(hdr_pipeline == HDR_PIPELINE_OFF){
				wb_gain.GainR =awb_gain[0].video_wb_gain.r_gain<<2; //dsp wb unit =4096, while wb algo is 1024
				wb_gain.GainG =awb_gain[0].video_wb_gain.g_gain<<2;
				wb_gain.GainB =awb_gain[0].video_wb_gain.b_gain<<2;
				amba_img_dsp_set_wb_gain(fd_iav,&ik_mode,&wb_gain);
				awb_gain->video_gain_update=0;
			}else{
				// in HDR pipeline, wb gain are left to HDR block
				wb_gain.GainR = WB_DGAIN_UNIT;
				wb_gain.GainG = WB_DGAIN_UNIT;
				wb_gain.GainB = WB_DGAIN_UNIT;
				amba_img_dsp_set_wb_gain(fd_iav,&ik_mode,&wb_gain);
			}
		}

		//*****ae_algo********//
		//ae algo output gain_db and shutter_time
		//adj_index = gain_db*128/6;//sensor gain table to 128 unit.

	/*	int expo_id = 0;
		if(hdr_pipeline == HDR_PIPELINE_OFF){
			ae_output[0].shutter = ???;
			ae_output[0].agc = ???;
			ae_output[0].dgain = ???;
		}else{
			for(expo_id = 0; expo_id < expo_num; ++ expo_id){
				ae_output[expo_id].shutter = ???;
				ae_output[expo_id].agc = ???;
				ae_output[expo_id].dgain = ???;
			}
		}	*/

		img_runtime_adj(fd_iav, &ik_mode, adj_index, &awb_gain[0].video_wb_gain); //include all ISP filter but set_wb_gain
		u32 g_dgain =wb_gain.AeGain*wb_gain.GlobalDGain/WB_DGAIN_UNIT;
		config_parser_dgain(g_dgain);

		if(dynamic_tone_curve_get_status()){
			u8 tone_update = 0;
			tone_update = dynamic_tone_curve_control(&cus_dyn_tone, &aaa_stat_info[0]);
			if(tone_update){
				filter_header_t f_header;
				f_header.filter_id = ADJ_TONE;
				coll_transmit_filter((void*)&f_header, (void*)&cus_dyn_tone, (void *)fd_iav, (void*)&ik_mode);
			}
		}
		if(hdr_pipeline != HDR_PIPELINE_OFF){
			hdr_proc_control(&cus_hdr_proc_pipe, awb_gain, ae_output);
			img_set_hdr_batch(fd_iav, &ik_mode, &cus_work_info, &cus_hdr_proc_pipe);
		}

		if((isp_pipeline== ISP_PIPELINE_B_LISO ||isp_pipeline ==ISP_PIPELINE_ADV_LISO)) {
			amba_img_dsp_post_exe_cfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_PARTIALCOPY, 0);
			ik_mode.ConfigId ^= 1;
			amba_img_dsp_post_exe_cfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_FULLCOPY, 0);
		}
	}
	awb_flow_deinit();
}

int load_containers(char* sensor_name)
{
	fc_collection_t fcc;
	memset(&fcc, 0, sizeof(fcc));
	isp_pipeline_t isp_pipeline = cus_work_info.isp_pipeline;
	hdr_pipeline_t hdr_pipeline = cus_work_info.hdr_config.pipeline;
	u8 expo_num = cus_work_info.hdr_config.expo_num;
	printf("mn34220pl\n");
	if(isp_pipeline ==ISP_PIPELINE_LISO){
		fcc.fc_ae_target = &mn34220pl_liso_fc_ae_target;
		fcc.fc_wb_ratio = &mn34220pl_liso_fc_wb_ratio;
		fcc.fc_blc = &mn34220pl_liso_fc_blc;
		fcc.fc_antialiasing = &mn34220pl_liso_fc_antialiasing;
		fcc.fc_grgbmismatch = &mn34220pl_liso_fc_grgbmismatch;
		fcc.fc_dpc = &mn34220pl_liso_fc_dpc;
		fcc.fc_cfanf_low = &mn34220pl_liso_fc_cfanf;
		fcc.fc_cfanf_high = NULL;
		fcc.fc_le = (hdr_pipeline == HDR_PIPELINE_OFF)? NULL : \
			((expo_num == HDR_2X)? &mn34220pl_liso_fc_2x_hdr_le : &mn34220pl_liso_fc_3x_hdr_le);
		fcc.fc_demosaic = &mn34220pl_liso_fc_demosaic;
		fcc.fc_cc = &mn34220pl_liso_fc_cc;
		fcc.fc_tone = (hdr_pipeline == HDR_PIPELINE_OFF)? &mn34220pl_liso_fc_tone : \
			((expo_num == HDR_2X)? &mn34220pl_liso_fc_2x_hdr_tone : &mn34220pl_liso_fc_3x_hdr_tone);
		fcc.fc_rgb2yuv = &mn34220pl_liso_fc_rgb2yuv;
		fcc.fc_chroma_scale = &mn34220pl_liso_fc_chroma_scale;
		fcc.fc_chroma_median = &mn34220pl_liso_fc_chroma_median;
		fcc.fc_chroma_nf = &mn34220pl_liso_fc_chroma_nf;
		fcc.fc_cdnr = &mn34220pl_liso_fc_cdnr;
		fcc.fc_1stmode_sel = &mn34220pl_liso_fc_1stmode_sel;
		fcc.fc_asf = &mn34220pl_liso_fc_asf;
		fcc.fc_1st_shpboth = &mn34220pl_liso_fc_1st_shpboth;
		fcc.fc_1st_shpnoise = &mn34220pl_liso_fc_1st_shpnoise;
		fcc.fc_1st_shpfir = &mn34220pl_liso_fc_1st_shpfir;
		fcc.fc_1st_shpcoring = &mn34220pl_liso_fc_1st_shpcoring;
		fcc.fc_1st_shpcoring_idx_scale = &mn34220pl_liso_fc_1st_shpcoring_idx_scale;
		fcc.fc_1st_shpcoring_min = &mn34220pl_liso_fc_1st_shpcoring_min;
		fcc.fc_1st_shpcoring_scale_coring = &mn34220pl_liso_fc_1st_shpcoring_scale_coring;
		fcc.fc_video_mctf = &mn34220pl_liso_fc_video_mctf;
		fcc.fc_hdr_alpha = \
			(hdr_pipeline == HDR_PIPELINE_OFF)? NULL : &mn34220pl_liso_fc_hdr_alpha;
		fcc.fc_hdr_threshold = \
			(hdr_pipeline == HDR_PIPELINE_OFF)? NULL : &mn34220pl_liso_fc_hdr_threshold;
		fcc.fc_hdr_ce = \
			(hdr_pipeline == HDR_PIPELINE_OFF)? NULL : &mn34220pl_liso_fc_hdr_ce;
	}
	else if(isp_pipeline ==ISP_PIPELINE_ADV_LISO){
		fcc.fc_ae_target = &mn34220pl_aliso_fc_ae_target;
		fcc.fc_wb_ratio = &mn34220pl_aliso_fc_wb_ratio;
		fcc.fc_blc = &mn34220pl_aliso_fc_blc;
		fcc.fc_antialiasing = &mn34220pl_aliso_fc_antialiasing;
		fcc.fc_grgbmismatch = &mn34220pl_aliso_fc_grgbmismatch;
		fcc.fc_dpc = &mn34220pl_aliso_fc_dpc;
		fcc.fc_cfanf_low = &mn34220pl_aliso_fc_cfanf;
		fcc.fc_cfanf_high = NULL;
		fcc.fc_le = (hdr_pipeline == HDR_PIPELINE_OFF)? NULL : \
			((expo_num == HDR_2X)? &mn34220pl_aliso_fc_2x_hdr_le : &mn34220pl_aliso_fc_3x_hdr_le);
		fcc.fc_demosaic = &mn34220pl_aliso_fc_demosaic;
		fcc.fc_cc = &mn34220pl_aliso_fc_cc;
		fcc.fc_tone = (hdr_pipeline == HDR_PIPELINE_OFF)? &mn34220pl_aliso_fc_tone : \
			((expo_num == HDR_2X)? &mn34220pl_aliso_fc_2x_hdr_tone : &mn34220pl_aliso_fc_3x_hdr_tone);
		fcc.fc_rgb2yuv = &mn34220pl_aliso_fc_rgb2yuv;
		fcc.fc_chroma_scale = &mn34220pl_aliso_fc_chroma_scale;
		fcc.fc_chroma_median = &mn34220pl_aliso_fc_chroma_median;
		fcc.fc_chroma_nf = &mn34220pl_aliso_fc_chroma_nf;
		fcc.fc_cdnr = &mn34220pl_aliso_fc_cdnr;
		fcc.fc_1stmode_sel = &mn34220pl_aliso_fc_1stmode_sel;
		fcc.fc_asf = &mn34220pl_aliso_fc_asf;
		fcc.fc_1st_shpboth = &mn34220pl_aliso_fc_1st_shpboth;
		fcc.fc_1st_shpnoise = &mn34220pl_aliso_fc_1st_shpnoise;
		fcc.fc_1st_shpfir = &mn34220pl_aliso_fc_1st_shpfir;
		fcc.fc_1st_shpcoring = &mn34220pl_aliso_fc_1st_shpcoring;
		fcc.fc_1st_shpcoring_idx_scale = &mn34220pl_liso_fc_1st_shpcoring_idx_scale;
		fcc.fc_1st_shpcoring_min = &mn34220pl_aliso_fc_1st_shpcoring_min;
		fcc.fc_1st_shpcoring_scale_coring = &mn34220pl_aliso_fc_1st_shpcoring_scale_coring;
		fcc.fc_final_shpboth = &mn34220pl_aliso_fc_final_shpboth;
		fcc.fc_final_shpnoise = &mn34220pl_aliso_fc_final_shpnoise;
		fcc.fc_final_shpfir = &mn34220pl_aliso_fc_final_shpfir;
		fcc.fc_final_shpcoring = &mn34220pl_aliso_fc_final_shpcoring;
		fcc.fc_final_shpcoring_idx_scale = &mn34220pl_aliso_fc_final_shpcoring_idx_scale;
		fcc.fc_final_shpcoring_min = &mn34220pl_aliso_fc_final_shpcoring_min;
		fcc.fc_final_shpcoring_scale_coring = &mn34220pl_aliso_fc_final_shpcoring_scale_coring;
		fcc.fc_wide_chroma_filter =&mn34220pl_aliso_fc_wide_chroma_filter;
		fcc.fc_wide_chroma_filter_combine =&mn34220pl_aliso_fc_wide_chroma_filter_combine;
		fcc.fc_video_mctf = &mn34220pl_aliso_fc_video_mctf;
		fcc.fc_video_mctf_temporal_adjust =&mn34220pl_aliso_fc_video_mctf_temporal_adjust;
		fcc.fc_hdr_alpha = \
			(hdr_pipeline == HDR_PIPELINE_OFF)? NULL : &mn34220pl_aliso_fc_hdr_alpha;
		fcc.fc_hdr_threshold = \
			(hdr_pipeline == HDR_PIPELINE_OFF)? NULL : &mn34220pl_aliso_fc_hdr_threshold;
		fcc.fc_hdr_ce = \
			(hdr_pipeline == HDR_PIPELINE_OFF)? NULL : &mn34220pl_aliso_fc_hdr_ce;
	}
	else if(isp_pipeline ==ISP_PIPELINE_MID_LISO){
		fcc.fc_ae_target = &mn34220pl_mliso_fc_ae_target;
		fcc.fc_wb_ratio = &mn34220pl_mliso_fc_wb_ratio;
		fcc.fc_blc = &mn34220pl_mliso_fc_blc;
		fcc.fc_antialiasing = &mn34220pl_mliso_fc_antialiasing;
		fcc.fc_grgbmismatch = &mn34220pl_mliso_fc_grgbmismatch;
		fcc.fc_dpc = &mn34220pl_mliso_fc_dpc;
		fcc.fc_cfanf_low = &mn34220pl_mliso_fc_cfanf;
		fcc.fc_cfanf_high = NULL;
		fcc.fc_le = (hdr_pipeline == HDR_PIPELINE_OFF)? NULL : \
			((expo_num == HDR_2X)? &mn34220pl_mliso_fc_2x_hdr_le : &mn34220pl_mliso_fc_3x_hdr_le);
		fcc.fc_demosaic = &mn34220pl_mliso_fc_demosaic;
		fcc.fc_cc = &mn34220pl_mliso_fc_cc;
		fcc.fc_tone = (hdr_pipeline == HDR_PIPELINE_OFF)? &mn34220pl_mliso_fc_tone : \
			((expo_num == HDR_2X)? &mn34220pl_mliso_fc_2x_hdr_tone : &mn34220pl_mliso_fc_3x_hdr_tone);
		fcc.fc_rgb2yuv = &mn34220pl_mliso_fc_rgb2yuv;
		fcc.fc_chroma_scale = &mn34220pl_mliso_fc_chroma_scale;
		fcc.fc_chroma_median = &mn34220pl_mliso_fc_chroma_median;
		fcc.fc_chroma_nf = &mn34220pl_mliso_fc_chroma_nf;
		fcc.fc_cdnr = &mn34220pl_mliso_fc_cdnr;
		fcc.fc_1stmode_sel = &mn34220pl_mliso_fc_1stmode_sel;
		fcc.fc_asf = &mn34220pl_mliso_fc_asf;
		fcc.fc_1st_shpboth = &mn34220pl_mliso_fc_1st_shpboth;
		fcc.fc_1st_shpnoise = &mn34220pl_mliso_fc_1st_shpnoise;
		fcc.fc_1st_shpfir = &mn34220pl_mliso_fc_1st_shpfir;
		fcc.fc_1st_shpcoring = &mn34220pl_mliso_fc_1st_shpcoring;
		fcc.fc_1st_shpcoring_idx_scale = &mn34220pl_mliso_fc_1st_shpcoring_idx_scale;
		fcc.fc_1st_shpcoring_min = &mn34220pl_mliso_fc_1st_shpcoring_min;
		fcc.fc_1st_shpcoring_scale_coring = &mn34220pl_mliso_fc_1st_shpcoring_scale_coring;
		fcc.fc_video_mctf = &mn34220pl_mliso_fc_video_mctf;
		fcc.fc_video_mctf_temporal_adjust =&mn34220pl_mliso_fc_video_mctf_temporal_adjust;
		fcc.fc_hdr_alpha = \
			(hdr_pipeline == HDR_PIPELINE_OFF)? NULL : &mn34220pl_mliso_fc_hdr_alpha;
		fcc.fc_hdr_threshold = \
			(hdr_pipeline == HDR_PIPELINE_OFF)? NULL : &mn34220pl_mliso_fc_hdr_threshold;
		fcc.fc_hdr_ce = \
			(hdr_pipeline == HDR_PIPELINE_OFF)? NULL : &mn34220pl_mliso_fc_hdr_ce;
	}
	sprintf(sensor_name, "mn34220pl");
	img_adj_init_containers_liso(&fcc);
	return 0;
}
int enable_cc(int fd_iav, char* sensor_name, amba_img_dsp_mode_cfg_t* ik_mode)
{
	amba_img_dsp_color_correction_reg_t cc_reg;
	amba_img_dsp_color_correction_t cc_3d;

	u8* reg, *matrix;
	char filename[128];
	int file, count;

	reg = malloc(AMBA_DSP_IMG_CC_REG_SIZE);
	matrix = malloc(AMBA_DSP_IMG_CC_3D_SIZE);

	sprintf(filename,"%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("%s cannot be opened\n", filename);
		return -1;
	}
	if((count = read(file, reg, AMBA_DSP_IMG_CC_REG_SIZE)) != AMBA_DSP_IMG_CC_REG_SIZE) {
		printf("read %s error\n", filename);
		return -1;
	}
	close(file);

	sprintf(filename,"%s/sensors/%s_0%d_3D.bin", IMGPROC_PARAM_PATH, sensor_name, 2);
	if((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("%s cannot be opened\n", filename);
		return -1;
	}
	if((count = read(file, matrix, AMBA_DSP_IMG_CC_3D_SIZE)) != AMBA_DSP_IMG_CC_3D_SIZE) {
		printf("read %s error\n", filename);
		return -1;
	}
	close(file);

	cc_reg.RegSettingAddr = (u32)reg;
	amba_img_dsp_set_color_correction_reg(ik_mode, &cc_reg);

	cc_3d.MatrixThreeDTableAddr = (u32)matrix;
	amba_img_dsp_set_color_correction(fd_iav, ik_mode,&cc_3d);

	free(reg);
	free(matrix);
	return 0;
}

int load_cc_bin(char * sensor_name)
{
	int file, count;
	char filename[128];
	u8 *matrix[4];
	u8 i;

	for (i = 0; i < 4; i++) {
		matrix[i] = malloc(AMBA_DSP_IMG_CC_3D_SIZE);
		sprintf(filename,"%s/sensors/%s_0%d_3D.bin", IMGPROC_PARAM_PATH, sensor_name, (i+1));
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			printf("%s cannot be opened\n", filename);
			return -1;
		}
		if((count = read(file, matrix[i], AMBA_DSP_IMG_CC_3D_SIZE)) != AMBA_DSP_IMG_CC_3D_SIZE) {
			printf("read %s error\n",filename);
			return -1;
		}
		close(file);
	}
	cc_binary_addr_t cc_addr;
	cc_addr.cc_0 = matrix[0];
	cc_addr.cc_1 = matrix[1];
	cc_addr.cc_2 = matrix[2];
	cc_addr.cc_3 = matrix[3];
	img_adj_load_cc_binary(&cc_addr);

	for(i=0; i<4; i++)
		free(matrix[i]);
	return 0;
}
void config_stat_tiles(statistics_config_t* tile_cfg)
{
	hdr_pipeline_t hdr_pipeline = cus_work_info.hdr_config.pipeline;
	u8 expo_num = cus_work_info.hdr_config.expo_num;
	int i = 0;

	ae_tile_config.AeTileNumCol   = tile_cfg->ae_tile_num_col;
	ae_tile_config.AeTileNumRow   = tile_cfg->ae_tile_num_row;
	ae_tile_config.AeTileColStart = tile_cfg->ae_tile_col_start;
	ae_tile_config.AeTileRowStart = tile_cfg->ae_tile_row_start;
	ae_tile_config.AeTileWidth    = tile_cfg->ae_tile_width;
	ae_tile_config.AeTileHeight   = tile_cfg->ae_tile_height;
	ae_tile_config.AePixMinValue = tile_cfg->ae_pix_min_value;
	ae_tile_config.AePixMaxValue = tile_cfg->ae_pix_max_value;

	awb_tile_config.AwbTileNumCol = tile_cfg->awb_tile_num_col;
	awb_tile_config.AwbTileNumRow = tile_cfg->awb_tile_num_row;
	awb_tile_config.AwbTileColStart = tile_cfg->awb_tile_col_start;
	awb_tile_config.AwbTileRowStart = tile_cfg->awb_tile_row_start;
	awb_tile_config.AwbTileWidth = tile_cfg->awb_tile_width;
	awb_tile_config.AwbTileHeight = tile_cfg->awb_tile_height;
	awb_tile_config.AwbTileActiveWidth = tile_cfg->awb_tile_active_width;
	awb_tile_config.AwbTileActiveHeight = tile_cfg->awb_tile_active_height;
	awb_tile_config.AwbPixMinValue = tile_cfg->awb_pix_min_value;
	awb_tile_config.AwbPixMaxValue = tile_cfg->awb_pix_max_value;

	af_tile_config.AfTileNumCol = tile_cfg->af_tile_num_col;
	af_tile_config.AfTileNumRow = tile_cfg->af_tile_num_row;
	af_tile_config.AfTileColStart = tile_cfg->af_tile_col_start;
	af_tile_config.AfTileRowStart = tile_cfg->af_tile_row_start;
	af_tile_config.AfTileWidth = tile_cfg->af_tile_width;
	af_tile_config.AfTileHeight = tile_cfg->af_tile_height;
	af_tile_config.AfTileActiveWidth = tile_cfg->af_tile_active_width;
	af_tile_config.AfTileActiveHeight = tile_cfg->af_tile_active_height;
	config_parser_stat_tiles(tile_cfg);

	if(hdr_pipeline != HDR_PIPELINE_OFF){
		hdr_stat_config.VinStatsMainOn =1;
		hdr_stat_config.VinStatsHdrOn = 1;
		hdr_stat_config.TotalExposures = expo_num;
		hdr_stat_config.TotalSliceInX = 1;
	}

	hist_stat_config.HistMode = 2;
	for (i = 0; i < 8; ++i) {
		hist_stat_config.TileMask[i] = 0xFFF;
	}
}

static int start_aaa_init(int fd_iav)
{
	char sensor_name[64];
	#define	PIXEL_IN_MB			(16)
	hdr_blend_info_t cus_hdr_blend;
	img_config_info_t* p_img_config = &cus_work_info;
	int sensor_id = 0x0;
	struct vindev_video_info video_info;
	struct iav_enc_mode_cap mode_cap;
	struct iav_system_resource system_resource;
	struct iav_srcbuf_setup	srcbuf_setup;
	struct vindev_aaa_info vin_aaa_info;
	amba_img_dsp_variable_range_t dsp_variable_range;

	// video info
	memset(&video_info, 0, sizeof(video_info));
	video_info.vsrc_id = 0;
	video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
		perror("IAV_IOC_VIN_GET_VIDEOINFO");
		return 0;
	}
	p_img_config->raw_width = ROUND_UP(video_info.info.width, PIXEL_IN_MB);
	p_img_config->raw_height = ROUND_UP(video_info.info.height, PIXEL_IN_MB);
	p_img_config->raw_resolution =video_info.info.bits;

	// encode mode capability
	memset(&mode_cap, 0, sizeof(mode_cap));
	mode_cap.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(fd_iav, IAV_IOC_QUERY_ENC_MODE_CAP, &mode_cap)) {
		perror("IAV_IOC_QUERY_ENC_MODE_CAP");
		return -1;
	}

	// system resource
	memset(&system_resource, 0, sizeof(system_resource));
	system_resource.encode_mode = DSP_CURRENT_MODE;
	if (ioctl(fd_iav, IAV_IOC_GET_SYSTEM_RESOURCE, &system_resource) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE\n");
		return -1;
	}
	p_img_config->hdr_config.expo_num = system_resource.exposure_num;
	p_img_config->hdr_config.pipeline = system_resource.hdr_type;
	p_img_config->isp_pipeline = system_resource.iso_type;
	p_img_config->raw_pitch =system_resource.raw_pitch_in_bytes;
//	printf("expo_n %d,hdr pipe %d,isp pipe %d\n",p_img_config->hdr_config.expo_num,p_img_config->hdr_config.pipeline,p_img_config->isp_pipeline);

	// source buffer setup
	memset(&srcbuf_setup, 0, sizeof(srcbuf_setup));
	if (ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_SETUP, &srcbuf_setup) < 0) {
			printf("IAV_IOC_GET_SOURCE_BUFFER_SETUP error\n");
			return -1;
	}
	p_img_config->main_width = ROUND_UP(
		srcbuf_setup.size[IAV_SRCBUF_MN].width, PIXEL_IN_MB);
	p_img_config->main_height = ROUND_UP(
		srcbuf_setup.size[IAV_SRCBUF_MN].height, PIXEL_IN_MB);

	// vin aaa info
	vin_aaa_info.vsrc_id = 0;
	if (ioctl(fd_iav, IAV_IOC_VIN_GET_AAAINFO, &vin_aaa_info) < 0) {
		perror("IAV_IOC_VIN_GET_AAAINFO error\n");
		return -1;
	}

	p_img_config->raw_bayer = vin_aaa_info.bayer_pattern;
	sensor_id = vin_aaa_info.sensor_id;
	if(vin_aaa_info.dual_gain_mode){
		p_img_config->hdr_config.method = HDR_DUAL_GAIN_METHOD;
	}else{
		switch (vin_aaa_info.hdr_mode){
			case AMBA_VIDEO_LINEAR_MODE:
				p_img_config->hdr_config.method = HDR_NONE_METHOD;
				break;
			case AMBA_VIDEO_2X_HDR_MODE:
			case AMBA_VIDEO_3X_HDR_MODE:
			case AMBA_VIDEO_4X_HDR_MODE:
				if(sensor_id == SENSOR_AR0230 ){
					p_img_config->hdr_config.method = HDR_RATIO_FIX_LINE_METHOD;
				}else{
					p_img_config->hdr_config.method = HDR_RATIO_VARY_LINE_METHOD;
				}
				break;
			case AMBA_VIDEO_INT_HDR_MODE:
				p_img_config->hdr_config.method = HDR_BUILD_IN_METHOD;
				break;
			default:
				printf("error: invalid vin HDR sensor info.\n");
				return -1;
		}
	}

	// ik mode configuration
	memset(&ik_mode, 0, sizeof(ik_mode));
	ik_mode.ConfigId = 0;
	ik_mode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
	if (p_img_config->hdr_config.pipeline !=HDR_PIPELINE_OFF) {
		ik_mode.FuncMode = AMBA_DSP_IMG_FUNC_MODE_HDR;
	}
	if(p_img_config->isp_pipeline  ==ISP_PIPELINE_B_LISO){
		ik_mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;
		ik_mode.BatchId = 0xff;
		ik_mode.Reserved1 = 0xAA;
	}
	else if(p_img_config->isp_pipeline  ==ISP_PIPELINE_LISO){
		ik_mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
	}
	else if(p_img_config->isp_pipeline  ==ISP_PIPELINE_ADV_LISO||
		p_img_config->isp_pipeline  ==ISP_PIPELINE_MID_LISO){
		ik_mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_LISO;
		ik_mode.BatchId = 0xff;
		ik_mode.Reserved1 = 0xAA;
	}
	dsp_variable_range.max_chroma_radius = (1 << (5 + mode_cap.max_chroma_radius));
	dsp_variable_range.max_wide_chroma_radius = (1 << (5 + mode_cap.max_wide_chroma_radius));
	dsp_variable_range.inside_fpn_flag = 0;
	dsp_variable_range.wide_chroma_noise_filter_enable = mode_cap.wcr_possible;
	amba_img_dsp_set_variable_range(&ik_mode, &dsp_variable_range);

	// hdr sensor offsets configuration
	cus_offset_cfg.offset_coef = mn34220pl_aeb_sensor_config.sensor_config.hdr_offset_coef;
	cus_offset_cfg.offset_tbl.offset[0] = vin_aaa_info.hdr_long_offset;
	cus_offset_cfg.offset_tbl.offset[1] = vin_aaa_info.hdr_short1_offset;
	cus_offset_cfg.offset_tbl.offset[2] = vin_aaa_info.hdr_short2_offset;
	cus_offset_cfg.offset_tbl.offset[3] = vin_aaa_info.hdr_short3_offset;
	cus_offset_cfg.mode = \
		(cus_offset_cfg.offset_coef.long_padding < 0 || cus_offset_cfg.offset_coef.short_padding < 0 )? 0 : 1;
	cus_offset_cfg.offset_delay.delay[0] = 0;
	cus_offset_cfg.offset_delay.delay[1] = 0;
	cus_offset_cfg.offset_delay.delay[2] = 0;
	cus_offset_cfg.offset_delay.delay[3] = 0;

	//  awb param
	memcpy(&is_awb_param,&mn34220pl_aeb_wb_param.wb_param,sizeof(is_awb_param));

	// init functions
	adj_config_work_info(p_img_config);		//for img_runtime_adj
	if(load_containers(sensor_name)<0)
		return -1;
	if(load_cc_bin(sensor_name)<0)
		return -1;
	if(enable_cc(fd_iav,sensor_name,&ik_mode)<0)
		return -1;
	config_stat_tiles(&mn34220pl_aeb_tile_config.tile_config);
	hdr_proc_init(p_img_config, &cus_offset_cfg, &cus_hdr_proc_pipe);

	if(p_img_config->hdr_config.pipeline !=HDR_PIPELINE_OFF){
		get_hdr_blend_config(&cus_hdr_blend);
		cus_hdr_blend.expo_ratio = 32;	// set exposure ratio at initialization, 32x is an example
		set_hdr_blend_config(&cus_hdr_blend);
	}
	return 0;
}
//////////////////////////////////////////////
//img adj usage for cus_loop mode
/////////////////////////////////////////////
#if 0
static void process_key(char key)//not work,just show how to use
{
    int payload =16; //fix me.only sample
    switch(key){
        case '1':
        adj_set_color_style(payload);
        img_adj_reset_filter(ADJ_RGB2YUV);
        break;
    case  '2':
        adj_set_hue(payload);
        img_adj_reset_filter(ADJ_RGB2YUV);
        break;
    case '3':
        adj_set_saturation(payload);
        img_adj_reset_filter(ADJ_RGB2YUV);
        break;
    case '4':
        adj_set_brightness(payload);
        img_adj_reset_filter(ADJ_RGB2YUV);
        break;
    case '5':
        adj_set_contrast(payload);
        img_adj_reset_filter(ADJ_TONE);
        break;
    case '6':
        adj_set_bw_mode(payload);
        img_adj_reset_filter(ADJ_CHROMA_SCALE);
        adj_set_cc_reset();
        break;
    case '7':
        adj_set_sharpening_level(payload);
        img_adj_reset_filter(ADJ_1ST_SHPBOTH);
        img_adj_reset_filter(ADJ_1ST_SHPCORING_SCALE_CORING);
        break;
    case '8':
        adj_set_mctf_level(payload);
        img_adj_reset_filter(ADJ_VIDEO_MCTF);
        break;
	case '9': //dynamic load containers.
		img_adj_update_idle_container((fc_collection_t*)payload);
		img_adj_flip_containers();
		img_adj_reset_filters();
		img_adj_sync_containers();
		break;
	case '0'://dynamic  load cc
		img_adj_update_idle_cc((cc_binary_addr_t* )payload);
		img_adj_flip_cc();
		adj_set_cc_reset();
		break;
	case 'A'://auto contrast enable
		u8 enable = 0;//or 1;
		s8 mode = (enable)? -1 : 0;
		dynamic_tone_curve_enable(enable);
		img_adj_set_filter_adj_argu(mode, ADJ_TONE);
		img_adj_pick_up_single_filter(&cus_dyn_tone, sizeof(cus_dyn_tone), ADJ_TONE);
		img_adj_reset_filter(ADJ_TONE);
		break;
	case 'B':// auto contrast strength config
		float dynamic_alpha =0.1;
		dynamic_tone_curve_set_alpha(dynamic_alpha);
		img_adj_reset_filter(ADJ_TONE);
		break;
        }
}
#endif
int check_iav_work(void)
{
	u32 state;
	memset(&state, 0, sizeof(state));
	if (ioctl(fd_iav, IAV_IOC_GET_IAV_STATE, &state) < 0) {
		perror("IAV_IOC_GET_IAV_STATE");
		return -1;
	}
	if (state == IAV_STATE_PREVIEW || state == IAV_STATE_ENCODING) {
		return 1;
	}

	return 0;
}

int do_init_netlink(void)
{
	init_netlink();
	pthread_create(&nl_thread, NULL, (void *)netlink_loop, (void *)NULL);

	while (1) {
		if (check_iav_work() > 0) {
			break;
		}
		usleep(10000);
	}

	return 0;
}
int do_start_aaa(void)
{
	printf("do_start_aaa\n");
	hdr_pipeline_t hdr_pipeline = cus_work_info.hdr_config.pipeline;
	isp_pipeline_t isp_pipeline = cus_work_info.isp_pipeline;
	//prepare for get 3A statistics
	memset(rgb_stat, 0, sizeof(struct rgb_aaa_stat)*4);
	memset(cfa_stat, 0, sizeof(struct cfa_aaa_stat)*4);
	memset(hdr_cfa_pre_stat, 0, sizeof(struct cfa_pre_hdr_stat) * MAX_PRE_HDR_STAT_BLK_NUM);
	rgb_stat_valid = 0;
	cfa_stat_valid = 0;
	hdr_cfa_pre_valid = 0;
	memset(&g_stat, 0, sizeof(amba_dsp_aaa_statistic_data_t));
	g_stat.CfaAaaDataAddr = (u32)cfa_stat;
	g_stat.RgbAaaDataAddr = (u32)rgb_stat;
	g_stat.CfaPreHdrDataAddr = (u32)hdr_cfa_pre_stat;
	g_stat.CfaDataValid = (u32)&cfa_stat_valid;
	g_stat.RgbDataValid = (u32)&rgb_stat_valid;
	g_stat.CfaPreHdrDataValid = (u32)&rgb_stat_valid;
	//end prepare
	// the operations within brace must be done after entering preview
	amba_img_dsp_3a_config_aaa_stat(fd_iav, 1, &ik_mode, &ae_tile_config, &awb_tile_config, &af_tile_config);
	amba_img_dsp_3a_config_histogram( fd_iav, &ik_mode, &hist_stat_config);
	if(hdr_pipeline != HDR_PIPELINE_OFF){
		amba_img_dsp_3a_config_hdr_stat(fd_iav, &ik_mode, &hdr_stat_config);
	}

	if(isp_pipeline == ISP_PIPELINE_B_LISO||
	isp_pipeline ==ISP_PIPELINE_ADV_LISO||
	isp_pipeline ==ISP_PIPELINE_MID_LISO){
		amba_img_dsp_post_exe_cfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_PARTIALCOPY, 0);
		ik_mode.ConfigId ^= 1;
		amba_img_dsp_post_exe_cfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_FULLCOPY, 0);
	}else{
		img_adj_init_misc(fd_iav, &ik_mode,&cus_work_info);
		img_adj_retrieve_filters(fd_iav, &ik_mode);
	}
	pthread_create(&id, NULL, (void*)cus_main_loop, (void*)fd_iav);

	return 0;
}
int do_prepare_aaa(void)
{
	isp_pipeline_t isp_pipeline;
	printf("do_prepare_aaa\n");
	img_lib_init(fd_iav,0,0);
	start_aaa_init(fd_iav);
	isp_pipeline= cus_work_info.isp_pipeline;
	ik_mode.ConfigId = 0;
	img_adj_reset_filters();

	if(isp_pipeline ==ISP_PIPELINE_LISO){
		return 0;
	}
	img_adj_init_misc(fd_iav, &ik_mode,&cus_work_info);
	img_adj_retrieve_filters(fd_iav, &ik_mode);
	if(isp_pipeline == ISP_PIPELINE_B_LISO||
		isp_pipeline ==ISP_PIPELINE_ADV_LISO||
		isp_pipeline ==ISP_PIPELINE_MID_LISO) {
		if(amba_img_dsp_post_exe_cfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_PARTIALCOPY, 1)<0)
			return -1;
		ik_mode.ConfigId ^= 1;
		if(amba_img_dsp_post_exe_cfg(fd_iav, &ik_mode, AMBA_DSP_IMG_CFG_EXE_FULLCOPY, 0)<0)
			return -1;
		ik_mode.Reserved1 = 0x0;
	}
	return 0;
}
int do_stop_aaa(void)
{
	exit_flag =1;
	pthread_join(id, NULL);
	if (img_lib_deinit() < 0) {
		printf("img_lib_deinit error!\n");
		return -1;
	}
	printf("do_stop_aaa\n");
	return 0;
}

int main(int argc, char **argv)
{
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("open /dev/iav");
		return -1;
	}
	sem_t sem;
	if (do_init_netlink() < 0) {
		return -1;
	}
#if 0
	char key =getchar();
	process_key(key);
#endif
	sem_init(&sem, 0, 0);
	sem_wait(&sem);

	return 0;
}

