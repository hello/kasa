 /* unit_test/private/mw_test/arch_s2/fb_image.h
 *
 * History:
 *	2013/08/06 - [Jingyang Qiu] created file
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include <errno.h>
#include <basetypes.h>

#include <sys/stat.h>
#include <sys/ioctl.h>

#ifndef __FB_IMAGE_H__
#define __FB_IMAGE_H__

#define	ERROR_LEVEL	0
#define	MSG_LEVEL	1
#define INFO_LEVEL	2
#define DEBUG_LEVEL	3
#define LOG_LEVEL_NUM	4

#define	PRINT(mylog, LOG_LEVEL, format, args...)		do {		\
							if (mylog >= LOG_LEVEL) {			\
								printf(format, ##args);			\
							}									\
						} while (0)

#define PRINT_ERROR(format, args...)		PRINT(log_level, ERROR_LEVEL, "!!! [%s: %d]" format "\n", __FILE__, __LINE__, ##args)
#define PRINT_MSG(format, args...)			PRINT(log_level, MSG_LEVEL, "" format, ##args)
#define PRINT_INFO(format, args...)		PRINT(log_level, INFO_LEVEL, "" format, ##args)
#define PRINT_DEBUG(format, args...)		PRINT(log_level, DEBUG_LEVEL, "=== " format, ##args)

typedef struct fb_vin_vout_size_s {
	u16	vin_width;
	u16	vin_height;
	u16	vout_width;
	u16	vout_height;
} fb_vin_vout_size_t;

#define	MAX_HISTOGRAM_UNIT_NUM		256

typedef struct fb_show_histogram_data_s {
	int	statist_type;
	int	hist_type;
	int	total_num;
	int	row_num;
	int	col_num;
	int	the_max;
	u32	histogram_value[MAX_HISTOGRAM_UNIT_NUM];
} fb_show_histogram_t;

int init_fb(fb_vin_vout_size_t *pFb_size);
int deinit_fb(void);
int draw_histogram_figure(fb_show_histogram_t *pHistogram_data);
int draw_histogram_plane_figure(fb_show_histogram_t *pHdata);
int show_all_fb(void);
int clear_all_fb(void);

#endif
