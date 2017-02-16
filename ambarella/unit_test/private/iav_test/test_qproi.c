/*
 * test_qproi.c
 *
 * History:
 * 2014/11/28  - [Zhaoyang Chen] created for S2L
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <basetypes.h>
#include <iav_ioctl.h>


#define MAX_ENCODE_STREAM_NUM		(IAV_STREAM_MAX_NUM_IMPL)
#define QP_MATRIX_SINGLE_SIZE		(96 << 10)

#define MAX_QP_DELTA_NUM            (3)
#define MAX_QP_VALUE				(51)


#ifndef ROUND_UP
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))
#endif
#ifndef ROUND_DOWN
#define ROUND_DOWN(size, align) ((size) & ~((align) - 1))
#endif

#define VERIFY_STREAMID(x)   do {		\
		if (((x) < 0) || ((x) >= MAX_ENCODE_STREAM_NUM)) {	\
			printf ("stream id wrong %d \n", (x));			\
			return -1; 	\
		}	\
	} while (0)


typedef struct qp_roi_s {
	int quality;		// 0 means the ROI is in the default QP RC.
	int qp_frame_type;
	int qp_offset;
	int start_x;
	int start_y;
	int width;
	int height;
} qp_roi_t;

typedef struct stream_roi_s {
	int enable;
	int qp_type;
	int encode_width;
	int encode_height;
	s8 qp_delta[MAX_QP_DELTA_NUM];
} stream_roi_t;

static int exit_flag = 0;

static int fd_iav;
static u8 *qp_matrix_addr = NULL;
static int stream_qp_matrix_size = 0;
static int single_matrix_size = 0;
static int stream_matrix_num = 0;

static stream_roi_t stream_roi[MAX_ENCODE_STREAM_NUM];
static qp_roi_t qp_roi[MAX_ENCODE_STREAM_NUM];

static void usage(void)
{
	printf("test_qproi usage:\n");
	printf("\t test_qproi\n"
		"\t Select a stream to set ROI.\n"
		"\t Select a method to configure qp roi, either quality or qp offset.\n"
		"\t Up to 3 levels are supported for configuring quality.\n"
		"\t QP offset is supported in MB level.\n"
		"\t Shall be used in ENCODE state.");
	printf("\n");
}

static void show_main_menu(void)
{
	printf("\n|-------------------------------|\n");
	printf("| Main Menu              \t|\n");
	printf("| a - Config Stream A ROI\t|\n");
	printf("| b - Config Stream B ROI\t|\n");
	printf("| c - Config Stream C ROI\t|\n");
	printf("| d - Config Stream D ROI\t|\n");
	printf("| q - Quit               \t|\n");
	printf("|-------------------------------|\n");
}

static int show_stream_menu(int stream_id)
{
	VERIFY_STREAMID(stream_id);
	printf("\n|-------------------------------|\n");
	printf("| Stream %c                \t|\n", 'A' + stream_id);
	printf("| 1 - Config QP ROI        \t|\n");
	printf("| 2 - Add an ROI         \t|\n");
	printf("| 3 - Remove an ROI      \t|\n");
	printf("| 4 - Clear all ROIs     \t|\n");
	printf("| 5 - View ROI           \t|\n");
	printf("| 6 - Check ROI pattern	 \t|\n");
	printf("| q - Back to Main menu  \t|\n");
	printf("|-------------------------------|\n");

	return 0;
}

static int map_qp_matrix(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_QPMATRIX;
	if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	qp_matrix_addr = mmap(NULL, querybuf.length, PROT_READ | PROT_WRITE,
		MAP_SHARED, fd_iav, querybuf.offset);
	stream_qp_matrix_size = querybuf.length / MAX_ENCODE_STREAM_NUM;
	stream_matrix_num = stream_qp_matrix_size / QP_MATRIX_SINGLE_SIZE;
	single_matrix_size = stream_qp_matrix_size / stream_matrix_num;

	printf("stream_matrix_num=%d, single_matrix_size=0x%x!\n",
		stream_matrix_num, single_matrix_size);

	return 0;
}

static int check_for_qp_roi(int stream_id)
{
	struct iav_stream_info stream_info;
	struct iav_stream_format stream_format;

	VERIFY_STREAMID(stream_id);

	memset(&stream_info, 0, sizeof(stream_info));
	stream_info.id = stream_id;
	if (ioctl(fd_iav, IAV_IOC_GET_STREAM_INFO, &stream_info) < 0) {
		perror("IAV_IOC_GET_STREAM_INFO");
		return -1;
	}
	if (stream_info.state != IAV_STREAM_STATE_ENCODING) {
		printf("Stream %c shall be in ENCODE state.\n", 'A' + stream_id);
		return -1;
	}

	memset(&stream_format, 0, sizeof(stream_format));
	stream_format.id = stream_id;
	if (ioctl(fd_iav, IAV_IOC_GET_STREAM_FORMAT, &stream_format) < 0) {
		perror("IAV_IOC_GET_STREAM_FORMAT");
		return -1;
	}
	if (stream_format.type != IAV_STREAM_TYPE_H264) {
		printf("Stream %c encode format shall be H.264.\n", 'A' + stream_id);
		return -1;
	}
	stream_roi[stream_id].encode_width = stream_format.enc_win.width;
	stream_roi[stream_id].encode_height = stream_format.enc_win.height;

	return 0;
}

static int get_qp_roi(int stream_id, struct iav_qpmatrix *matrix)
{
	struct iav_stream_cfg stream_cfg;

	VERIFY_STREAMID(stream_id);
	memset(&stream_cfg, 0, sizeof(stream_cfg));
	stream_cfg.id = stream_id;
	stream_cfg.cid = IAV_H264_CFG_QP_ROI;
	stream_cfg.arg.h264_roi.id = stream_id;
	stream_cfg.arg.h264_roi.qpm_no_update = 1;
	if (ioctl(fd_iav, IAV_IOC_GET_STREAM_CONFIG, &stream_cfg) < 0) {
		perror("IAV_IOC_GET_STREAM_CONFIG");
		return -1;
	}
	*matrix = stream_cfg.arg.h264_roi;

	return 0;
}

static int set_qp_roi(int stream_id, int qp_type)
{
	struct iav_stream_cfg stream_cfg;
	struct iav_qpmatrix *qp_matrix;
	int i, j;
	u8 *addr;
	struct iav_qproi_data *roi_addr;
	u32 buf_width, buf_pitch, buf_height;
	u32 start_x, start_y, end_x, end_y;

	VERIFY_STREAMID(stream_id);
	qp_matrix = &stream_cfg.arg.h264_roi;
	if (get_qp_roi(stream_id, qp_matrix) < 0)
		return -1;

	qp_matrix->id = stream_id;
	qp_matrix->enable = 1;
	qp_matrix->qpm_no_update = 0;
	qp_matrix->qpm_no_check = 0;

	// QP matrix is MB level. One MB is 16x16 pixels.
	buf_width = ROUND_UP(stream_roi[stream_id].encode_width, 16) / 16;
	buf_pitch = ROUND_UP(buf_width, 8);
	buf_height = ROUND_UP(stream_roi[stream_id].encode_height, 16) / 16;

	start_x = ROUND_DOWN(qp_roi[stream_id].start_x, 16) / 16;
	start_y = ROUND_DOWN(qp_roi[stream_id].start_y, 16) / 16;
	end_x = ROUND_UP(qp_roi[stream_id].width, 16) / 16 + start_x;
	end_y = ROUND_UP(qp_roi[stream_id].height, 16) / 16 + start_y;

	if (qp_type == QPROI_TYPE_QP_QUALITY) {
		addr = qp_matrix_addr + stream_qp_matrix_size * stream_id;
	} else {
		addr = qp_matrix_addr + stream_qp_matrix_size * stream_id +
			qp_roi[stream_id].qp_frame_type * single_matrix_size;
	}
	roi_addr = (struct iav_qproi_data *)addr;
	if (qp_type == QPROI_TYPE_QP_QUALITY) {
		for (i = start_y; i < end_y && i < buf_height; i++) {
			for (j = start_x; j < end_x && j < buf_width; j++)
				roi_addr[i * buf_pitch + j].qp_quality = qp_roi[stream_id].quality;
		}
	} else {
		for (i = start_y; i < end_y && i < buf_height; i++) {
			for (j = start_x; j < end_x && j < buf_width; j++)
				roi_addr[i * buf_pitch + j].qp_offset = qp_roi[stream_id].qp_offset;
		}
	}

	stream_cfg.id = stream_id;
	stream_cfg.cid = IAV_H264_CFG_QP_ROI;
	qp_matrix->type = qp_type;
	qp_matrix->size = buf_pitch * buf_height * sizeof(struct iav_qproi_data);
	if (ioctl(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg) < 0) {
		perror("IAV_IOC_SET_STREAM_CONFIG");
		return -1;
	}
	stream_roi[stream_id].enable = 1;

	return 0;
}

static int clear_qp_roi(int stream_id)
{
	struct iav_stream_cfg stream_cfg;
	struct iav_qpmatrix *qp_matrix;
	u32 *addr = (u32 *)(qp_matrix_addr + stream_qp_matrix_size * stream_id);

	VERIFY_STREAMID(stream_id);
	if (check_for_qp_roi(stream_id) < 0) {
		perror("check_for_qp_roi\n");
		return -1;
	}

	qp_matrix = &stream_cfg.arg.h264_roi;
	if (get_qp_roi(stream_id, qp_matrix) < 0)
		return -1;

	qp_matrix->id = stream_id;
	qp_matrix->enable = 1;
	qp_matrix->qpm_no_update = 0;
	qp_matrix->qpm_no_check = 1;

	memset(addr, 0, stream_qp_matrix_size);
	stream_cfg.id = stream_id;
	stream_cfg.cid = IAV_H264_CFG_QP_ROI;
	if (ioctl(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg) < 0) {
		perror("IAV_IOC_SET_STREAM_CONFIG");
		return -1;
	}
	stream_roi[stream_id].enable = 0;
	printf("\nClear all qp matrix for stream %c.\n", 'A' + stream_id);

	return 0;
}

static int display_qp_roi(int stream_id)
{
	int i, j, k;
	struct iav_qpmatrix qp_matrix;
	u8 *addr;
	struct iav_qproi_data *roi_addr;
	u32 buf_width, buf_pitch, buf_height;

	VERIFY_STREAMID(stream_id);
	buf_width = ROUND_UP(stream_roi[stream_id].encode_width, 16) / 16;
	buf_pitch = ROUND_UP(buf_width, 8);
	buf_height = ROUND_UP(stream_roi[stream_id].encode_height, 16) / 16;
	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;

	if (qp_matrix.enable) {
		if (qp_matrix.type == QPROI_TYPE_QP_QUALITY) {
			addr = qp_matrix_addr + stream_qp_matrix_size * stream_id;
			roi_addr = (struct iav_qproi_data *)addr;
			for (i = 0; i < QP_FRAME_TYPE_NUM; i++) {
				printf("\n\n=============================================\n");
				printf("QP ROI TYPE: QUALITY\n");
				printf("Quality level: 0-[%d], 1-[%d], 2-[%d], 3-[%d]\n",
				       qp_matrix.qp_delta[i][0], qp_matrix.qp_delta[i][1],
				       qp_matrix.qp_delta[i][2],qp_matrix.qp_delta[i][3]);
				printf("=============================================\n");
			}
			for (i = 0; i < buf_height; i++) {
				printf("\n");
				for (j = 0; j < buf_width; j++)
					printf("%-2d", roi_addr[i * buf_pitch + j].qp_quality);
			}
			printf("\n");
		} else if (qp_matrix.type == QPROI_TYPE_QP_OFFSET) {
			printf("\n\n=============================================\n");
			printf("QP ROI TYPE: QP offset\n");
			for (i = 0; i < stream_matrix_num; i++) {
				addr = qp_matrix_addr + stream_qp_matrix_size * stream_id +
					i * single_matrix_size;
				roi_addr = (struct iav_qproi_data *)addr;
				switch (i) {
				case QP_FRAME_I:
					if (stream_matrix_num == 1) {
						printf("QP ROI FOR I/P/B frames\n");
					} else {
						printf("QP ROI FOR I frame\n");
					}
					break;
				case QP_FRAME_P:
					printf("QP ROI FOR P frame\n");
					break;
				case QP_FRAME_B:
					printf("QP ROI FOR B frame\n");
					break;
				default:
					printf("Invalid QP frame type\n");
					return -1;
				}
				printf("=============================================\n");
				for (j = 0; j < buf_height; j++) {
					printf("\n");
					for (k = 0; k < buf_width; k++)
						printf("%2d", roi_addr[j * buf_pitch + k].qp_offset);
				}
				printf("\n");
			}
		} else {
			printf("Invalid qp roi type!\n");
			return -1;
		}
	}

	return 0;
}

static int get_qp_roi_type(int stream_id)
{
	struct iav_qpmatrix qp_matrix;

	VERIFY_STREAMID(stream_id);
	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;

	stream_roi[stream_id].qp_type = qp_matrix.type;

	return 0;
}


static int get_quality_level(int stream_id)
{
	struct iav_qpmatrix qp_matrix;

	VERIFY_STREAMID(stream_id);
	if (get_qp_roi(stream_id, &qp_matrix) < 0)
		return -1;

	stream_roi[stream_id].qp_delta[0] = qp_matrix.qp_delta[0][1];
	stream_roi[stream_id].qp_delta[1] = qp_matrix.qp_delta[0][2];
	stream_roi[stream_id].qp_delta[2] = qp_matrix.qp_delta[0][3];

	return 0;
}

static int set_quality_level(int stream_id)
{
	struct iav_stream_cfg stream_cfg;
	struct iav_qpmatrix *qp_matrix;
	int i;

	VERIFY_STREAMID(stream_id);
	qp_matrix = &stream_cfg.arg.h264_roi;
	if (get_qp_roi(stream_id, qp_matrix) < 0)
		return -1;
	qp_matrix->id = stream_id;
	qp_matrix->enable = 1;
	qp_matrix->qpm_no_update = 1;
	qp_matrix->qpm_no_check = 1;

	for (i = 0; i < QP_FRAME_TYPE_NUM; i++) {
		qp_matrix->qp_delta[i][0] = 0;
		qp_matrix->qp_delta[i][1] = stream_roi[stream_id].qp_delta[0];
		qp_matrix->qp_delta[i][2] = stream_roi[stream_id].qp_delta[1];
		qp_matrix->qp_delta[i][3] = stream_roi[stream_id].qp_delta[2];
	}
	stream_cfg.id = stream_id;
	stream_cfg.cid = IAV_H264_CFG_QP_ROI;
	if (ioctl(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg) < 0) {
		perror("IAV_IOC_SET_STREAM_CONFIG");
		return -1;
	}

	return 0;
}

static int fill_qp_offset(int stream_id)
{
	struct iav_stream_cfg stream_cfg;
	struct iav_qpmatrix *qp_matrix;
	int i, j, k;
	u8 *addr;
	struct iav_qproi_data *roi_addr;
	u32 buf_width, buf_pitch, buf_height;

	VERIFY_STREAMID(stream_id);
	qp_matrix = &stream_cfg.arg.h264_roi;
	if (get_qp_roi(stream_id, qp_matrix) < 0)
		return -1;

	qp_matrix->id = stream_id;
	qp_matrix->enable = 1;
	qp_matrix->qpm_no_update = 0;
	qp_matrix->qpm_no_check = 1;

	// QP matrix is MB level. One MB is 16x16 pixels.
	buf_width = ROUND_UP(stream_roi[stream_id].encode_width, 16) / 16;
	buf_pitch = ROUND_UP(buf_width, 8);
	buf_height = ROUND_UP(stream_roi[stream_id].encode_height, 16) / 16;

	for (i = 0; i < stream_matrix_num; i++) {
		addr = qp_matrix_addr + stream_qp_matrix_size * stream_id +
			i * single_matrix_size;
		roi_addr = (struct iav_qproi_data *)addr;

		for (j = 0; j < buf_height; j++)
			for (k = 0; k < buf_width; k++) {
				roi_addr[j * buf_pitch + k].qp_offset = k % 103 - 51;
		}
	}

	stream_cfg.id = stream_id;
	stream_cfg.cid = IAV_H264_CFG_QP_ROI;
	qp_matrix->type = stream_roi[stream_id].qp_type;
	qp_matrix->size = buf_pitch * buf_height * sizeof(struct iav_qproi_data);
	if (ioctl(fd_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg) < 0) {
		perror("IAV_IOC_SET_STREAM_CONFIG");
		return -1;
	}
	stream_roi[stream_id].enable = 1;
	printf("\nFilled qp matrix with qp offset for stream %c.\n", 'A' + stream_id);

	return 0;
}

static int input_value(int min, int max)
{
	int retry, i, input = 0;
#define MAX_LENGTH	16
	char tmp[MAX_LENGTH];

	do {
		retry = 0;
		scanf("%s", tmp);
		for (i = 0; i < MAX_LENGTH; i++) {
			if ((i == 0) && (tmp[i] == '-')) {
				continue;
			}
			if (tmp[i] == 0x0) {
				if (i == 0) {
					retry = 1;
				}
				break;
			}
			if ((tmp[i]  < '0') || (tmp[i] > '9')) {
				printf("error input:%s\n", tmp);
				retry = 1;
				break;
			}
		}

		input = atoi(tmp);
		if (input > max || input < min) {
			printf("\nInvalid. Enter a number (%d~%d): ", min, max);
			retry = 1;
		}

		if (retry) {
			printf("\nInput again: ");
			continue;
		}

	} while (retry);

	return input;
}


static int config_stream_roi(int stream_id)
{
	int back2main = 0, i;
	char opt, input[16];
	char roi_type[2][32] = {"Quality Level", "QP offset"};
	while (back2main == 0) {
		show_stream_menu(stream_id);
		printf("Your choice: ");
		scanf("%s", input);
		opt = tolower(input[0]);
		switch(opt) {
		case '1':
			if (check_for_qp_roi(stream_id) < 0)
				break;
			get_qp_roi_type(stream_id);
			printf("\nCurrent QP ROI type is: %s.\n",
				roi_type[stream_roi[stream_id].qp_type]);
			printf("Input QP ROI type (0 for Quality Level; 1 for QP offset): ");
			stream_roi[stream_id].qp_type = input_value(0, 1);
			if (stream_roi[stream_id].qp_type == QPROI_TYPE_QP_QUALITY) {
				if (get_quality_level(stream_id) < 0)
					break;
				printf("\nCurrent quality level is 1:[%d], 2:[%d], 3:[%d].\n",
						stream_roi[stream_id].qp_delta[0],
						stream_roi[stream_id].qp_delta[1],
						stream_roi[stream_id].qp_delta[2]);
				i = 0;
				do {
					printf("Input QP delta (-51~51) for level %d: ", i+1);
					stream_roi[stream_id].qp_delta[i] = input_value(-51, 51);
				} while (++i < MAX_QP_DELTA_NUM);
				set_quality_level(stream_id);
			}
			break;
		case '2':
		case '3':
			if (check_for_qp_roi(stream_id) < 0)
				break;
			printf("\nInput ROI offset x (0~%d): ", stream_roi[stream_id].encode_width - 1);
			qp_roi[stream_id].start_x = input_value(0, stream_roi[stream_id].encode_width - 1);
			printf("Input ROI offset y (0~%d): ", stream_roi[stream_id].encode_height - 1);
			qp_roi[stream_id].start_y = input_value(0, stream_roi[stream_id].encode_height -1);
			printf("Input ROI width (1~%d): ", stream_roi[stream_id].encode_width
		        - qp_roi[stream_id].start_x);
			qp_roi[stream_id].width = input_value(1, stream_roi[stream_id].encode_width
				- qp_roi[stream_id].start_x);
			printf("Input ROI height (1~%d): ", stream_roi[stream_id].encode_height
			    - qp_roi[stream_id].start_y);
			qp_roi[stream_id].height = input_value(1, stream_roi[stream_id].encode_height
				- qp_roi[stream_id].start_y);
			if (stream_roi[stream_id].qp_type == QPROI_TYPE_QP_QUALITY) {
				if (opt == '2') {
					printf("Input ROI quality level (1~%d): ", MAX_QP_DELTA_NUM);
					qp_roi[stream_id].quality = input_value(1, MAX_QP_DELTA_NUM);
				} else {
					qp_roi[stream_id].quality = 0;
				}
			} else {
				if (stream_matrix_num == 3) {
					printf("Input Frame type (0 for I frame; 1 for P frame; 2 for B frame): ");
					qp_roi[stream_id].qp_frame_type = input_value(0, 2);
				}
				if (opt == '2') {
					printf("Input ROI qp offset (-51~51): ");
					qp_roi[stream_id].qp_offset = input_value(-51, 51);
				} else {
					qp_roi[stream_id].qp_offset = 0;
				}
			}
			set_qp_roi(stream_id, stream_roi[stream_id].qp_type);
			break;
		case '4':
			if (check_for_qp_roi(stream_id) < 0)
				break;
			clear_qp_roi(stream_id);
			break;
		case '5':
			if (check_for_qp_roi(stream_id) < 0)
				break;
			display_qp_roi(stream_id);
			break;
		case '6':
			if (check_for_qp_roi(stream_id) < 0)
				break;
			if (stream_roi[stream_id].qp_type != QPROI_TYPE_QP_OFFSET) {
				printf("Invalid QP ROI type: %d!\n",
					stream_roi[stream_id].qp_type);
			} else {
				fill_qp_offset(stream_id);
			}
			break;
		case 'q':
			back2main = 1;
			break;
		default:
			printf("Unknown option %d.", opt);
			break;
		}
	}
	return 0;
}

static void quit_qp_roi()
{
	int i;
	exit_flag = 1;
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; i++)
		if (stream_roi[i].enable)
			clear_qp_roi(i);
	exit(0);
}

int main(int argc, char **argv)
{
	char opt;
	char input[16];
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}
	if (argc > 1) {
		usage();
		return 0;
	}

	if (map_qp_matrix() < 0)
		return -1;

	signal(SIGINT, quit_qp_roi);
	signal(SIGTERM, quit_qp_roi);
	signal(SIGQUIT, quit_qp_roi);

	while (exit_flag == 0) {
		show_main_menu();
		printf("Your choice: ");
		fflush(stdin);
		scanf("%s", input);
		opt = tolower(input[0]);
		fflush(stdin);
		if ( opt >= 'a' && opt <= 'h') {
			config_stream_roi(opt - 'a');
		}
		else if (opt == 'q')
			exit_flag = 1;
		else
			printf("Unknown option %d.", opt);
	}
	return 0;
}

