
#ifndef __MDMOTBUF_H__
#define __MDMOTBUF_H__

#include <basetypes.h>

#define MD_MAX_ROI_N	4
#define MD_SENSITIVITY_LEVEL 100
#define MD_MAX_THRESHOLD 100

/* ME SOURCE BUFFER */
typedef enum {
	ME_MAIN_BUFFER = 0,
	ME_SECOND_BUFFER,
	ME_TOTAL_BUFFER,
} md_me_buffer_type;

/* Motion Event */
typedef enum {
	MOT_NO_EVENT = 0,
	MOT_START,
	MOT_END,
	MOT_HAS_STARTED,
	MOT_HAS_ENDED,
} md_evt_type;

/* Motion Flag */
typedef enum {
	MOT_NONE = 0,
	MOT_HAPPEN,
} md_status;

typedef struct md_motbuf_roi_location_s {
	u16 x;	  // horizontal offset
	u16 y;	  // vertical offset
	u16 width;
	u16 height;
} md_motbuf_roi_location_t;

typedef struct md_motbuf_roi_s {
	md_motbuf_roi_location_t roi_location;
	u16 valid;
	u16 sensitivity;			// 0~9. A bigger value indicates more sensitive.
	u16 luma_diff_threshold;
	u32 luma_avg;
	u16 motion_flag;
	u16 roi_update_flag;
} md_motbuf_roi_t;

typedef struct md_motbuf_evt_s {
	u16 x;
	u16 y;
	u16 width;
	u16 height;
	u16 motion_flag;
	u16 valid;
	u16 roi_idx;
	u16 reserved;
	u32 diff_value;
} md_motbuf_evt_t;

typedef struct md_motbuf_init_s {
	int me_buffer;				//select source buffer, 0: main buffer,1: second buffer
	int frame_interval;			// frame interval for detect
	int pixel_line_interval;		// pixel in interval lines
	int motion_end_num;				// checking motion end count
} md_motbuf_init_t;

#define ME_FRAME_INTERVAL_MIN				1
#define ME_FRAME_INTERVAL_MAX				10
#define ME_PIXEL_LINE_INTERVAL_MIN			1
#define ME_PIXEL_LINE_INTERVAL_MAX			10
#define ME_MOTION_END_NUM_MIN				1
#define ME_MOTION_END_NUM_MAX				100

typedef void (*md_motbuf_callback_func)(const u8 *event);
typedef u32 (*md_motbuf_algo_func)(md_motbuf_roi_t *roi,u8 *cur_data, u8 *pre_data);

int md_motbuf_callback_setup(md_motbuf_callback_func handler);
int md_motbuf_algo_setup(md_motbuf_algo_func handler);
int md_motbuf_init(md_motbuf_init_t *md_buf_init);
int md_motbuf_deinit(void);
int md_motbuf_get_current_buffer_P_W_H(int *buffer_pitch, int *buffer_width, int *buffer_height);
int md_motbuf_set_roi_location(u32 roi_idx, const md_motbuf_roi_location_t *roi_location);
int md_motbuf_set_roi_sensitivity(u32 roi_idx, u16 sensitivity);
int md_motbuf_set_roi_threshold(u32 roi_idx, u32 threshold);
int md_motbuf_set_roi_validity(u32 roi_idx, u16 valid_flag);
int md_motbuf_get_roi_info(u32 roi_idx, md_motbuf_roi_t *roi_info);
int md_motbuf_status_display(void);
int md_motbuf_thread_start(void);
int md_motbuf_thread_stop(void);
int md_motbuf_get_event(u8 *md_event);

u32 algo_framediff_mse(md_motbuf_roi_t *roi, u8 *cur_data, u8 *pre_data);

int md_motbuf_yuv2graybmp(md_motbuf_roi_t roi, char *bmpname, int width, int height);

#endif
