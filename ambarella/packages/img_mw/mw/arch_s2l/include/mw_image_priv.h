

#ifndef __MW_IMAGE_PRIV_H__
#define __MW_IMAGE_PRIV_H__

#define	MW_EXPOSURE_LEVEL_MIN		(25)
#define	MW_EXPOSURE_LEVEL_MAX		(400)

#define	MW_SATURATION_MIN			(0)
#define	MW_SATURATION_MAX			(255)
#define	MW_BRIGHTNESS_MIN			(-255)
#define	MW_BRIGHTNESS_MAX			(255)
#define	MW_CONTRAST_MIN				(0)
#define	MW_CONTRAST_MAX				(128)
#define	MW_SHARPNESS_MIN			(0)
#define	MW_SHARPNESS_MAX			(11)

#define	MW_MCTF_STRENGTH_MIN		(0)
#define	MW_MCTF_STRENGTH_MAX		(11)

#define	MW_DC_IRIS_BALANCE_DUTY_MAX	(800)
#define	MW_DC_IRIS_BALANCE_DUTY_MIN	(400)

#define	MW_LE_STRENGTH_MIN		(0)
#define	MW_LE_STRENGTH_MAX		(128)

#define	FOCUS_INFINITY		(50000)
#define	FOCUS_MACRO			(50)

#define	PIXEL_IN_MB			(16)

#define	IMGPROC_PARAM_PATH	"/etc/idsp"

#define	SHT_TIME(SHT_Q9)		DIV_ROUND(512000000, (SHT_Q9))

#define	HDR_2X_MAX_SHUTTER(Q9)	((Q9) * 4 / 5)		// 1 + 4 = 5
#define	HDR_3X_MAX_SHUTTER(Q9)	((Q9) * 16 / 21)		// 1 + 4 + 16 = 21

#define	MW_THREAD_NAME_NETLINK	"img_mw.nl"
#define	MW_THREAD_NAME_AE_CTRL		"img_mw.ae_ctrl"

typedef enum {
	MW_CALIB_BPC = 1,
	MW_CALIB_WB,
	MW_CALIB_LENS_SHADING,
} MW_CALIBRATION_TYPE;


int get_sensor_aaa_params(_mw_global_config *pMw_info);
int config_sensor_lens_info(_mw_global_config *pMw_info);
int load_dsp_cc_table(int fd, char *sensor_name, amba_img_dsp_mode_cfg_t *ik_mode);
inline int get_vin_mode(u32 *vin_mode);
inline int get_vin_frame_rate(u32 *pFrame_time);
inline int get_shutter_time(u32 *pShutter_time);
int load_ae_exp_lines(mw_ae_param *ae);
int get_ae_exposure_lines(mw_ae_line *lines, u32 num);
int set_ae_exposure_lines(mw_ae_line *lines, u32 num);
int set_ae_switch_points(mw_ae_point *points, u32 num);
int set_day_night_mode(u32 dn_mode);
int create_ae_control_task(void);
int destroy_ae_control_task(void);
int load_calibration_data(mw_cali_file *file, MW_CALIBRATION_TYPE type);
int load_default_params(void);
int check_af_params(mw_af_param *pAF);
int reload_previous_params(void);
inline int check_state(void);
int set_chroma_noise_filter_max(int fd);
int get_sensor_agc_info(int fd, struct vindev_agc *agc_info);
int get_dsp_mode_cfg(amba_img_dsp_mode_cfg_t *dsp_mode_cfg);

int load_mctf_bin(void);

int get_sensor_aaa_params_from_bin(_mw_global_config *pMw_info);
int load_adj_file(_mw_global_config *pMw_info);
int save_adj_file(_mw_global_config *pMw_info);

int check_aaa_state(int fd);
int load_binary_file(void);
u32 shutter_index_to_q9(int index);
int shutter_q9_to_index(u32 shutter_time);

int check_iav_work(void);
int do_prepare_aaa(void);
int do_start_aaa(void);
int do_stop_aaa(void);

int init_netlink(void);
int deinit_netlink(void);
void * netlink_loop(void * data);
int send_image_msg_stop_aaa(void);
int init_sem(void);
int deinit_sem(void);
int wait_sem(int ms);
int signal_sem(void);

#endif // __MW_IMAGE_PRIV_H__

