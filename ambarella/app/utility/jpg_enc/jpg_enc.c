/*
 * jpg_enc.c
 *
 * History:
 *	2014/11/17 - [Tao Wu] created file
 *
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <basetypes.h>
#include "amba_usb.h"
#include "datatx_lib.h"
#include <signal.h>

#include <jpeglib.h>
#include <setjmp.h>

#define NO_ARG		0
#define HAS_ARG		1

#define IYUV		0
#define NV12		1

struct hint_s {
	const char *arg;
	const char *str;
};

#define AMBA_TRANSFER_PORT	(2024)
#define JPEG_QUALITY_MIN 		(0)
#define JPEG_QUALITY_MAX 		(100)
#define COLOR_COMPONENTS 		(3)
#define DEFAULT_JPEG_QUALITY	(60)
#define OUTPUT_BUF_SIZE 		(4096)

int fd_iav;
static int transfer_method = TRANS_METHOD_NFS;
static int port = AMBA_TRANSFER_PORT;
static int verbose = 0;

static char filename_yuv[256];
static char filename_jpeg[256];
static int jpeg_quality = DEFAULT_JPEG_QUALITY;

typedef struct image_data_s {
	u8 *buffer;
	int format;
	int width;	/* Number of columns in image */
	int height;	/* Number of rows in image */
}image_data_t;

static image_data_t image_data= {NULL, 0, 0, 0};
static u8* jpeg_buf = NULL;

typedef struct {
	struct jpeg_destination_mgr pub; /* public fields */
	JOCTET * buffer;    /* start of buffer */
	unsigned char *outbuffer;
	int outbuffer_size;
	unsigned char *outbuffer_cursor;
	int *written;
} my_destmgr;

typedef struct yuv_neon_arg {
	u8 *in;
	u8 *u;
	u8 *v;
	int row;
	int col;
	int pitch;
} yuv_neon_arg;

extern void chrome_convert(struct yuv_neon_arg *);

static int init_image_mem(char *file_input, int width, int height)
{
	int fd_input = -1;
	int size_input = 0;
	int u_offset = 0;
	int size_u = 0,size_v = 0;
	int read_input = 0;
	u8 *image_chrome = NULL;
	struct timeval time1, time2;
	yuv_neon_arg yuv_arg;
	if ((filename_yuv[0] == '\0') || (width <= 0) ||(height <= 0) ) {
		printf("Please set right yuv file, width, height\n");
		return -1;
	}

	size_input = width * height * 3/2;
	u_offset = width * height;
	size_u = width * height / 4;
	size_v = size_u;
	if ((fd_input = open(file_input, O_RDONLY )) < 0) {
		printf("Failed to open file [%s].\n", file_input);
		return -1;
	}

	do {
		image_data.buffer = (u8*) malloc(size_input);
		if (!image_data.buffer) {
			printf("Not enough memory for image buffer.\n");
			break;
		}
		memset(image_data.buffer, 0, size_input);
		read_input = read(fd_input, image_data.buffer, size_input);
		if (read_input != size_input) {
			printf("Read %s error\n", file_input);
			read_input = -1;
		}
		if (image_data.format == NV12) {
			image_chrome = (u8*)malloc(size_u + size_v);
			if (image_chrome == NULL) {
				printf("Not enough memory for image_u or image_v.\n");
				return -1;;
			}
			if (verbose) {
				gettimeofday(&time1, NULL);
			}
			yuv_arg.in = image_data.buffer + u_offset;
			yuv_arg.u = image_chrome;
			yuv_arg.v = image_chrome + size_u;
			yuv_arg.row = height / 2;
			yuv_arg.col = width;
			yuv_arg.pitch = width;
			chrome_convert(&yuv_arg);
			memcpy(image_data.buffer + u_offset, yuv_arg.u, size_u + size_v);
			if (verbose) {
				gettimeofday(&time2, NULL);
				printf("NV12 to IYUV takes time %ld ms\n",(time2.tv_usec - time1.tv_usec)/1000L +(time2.tv_sec - time1.tv_sec) * 1000L);
			}
		}

	} while(0);
	if (image_chrome != NULL) {
		free(image_chrome);
		image_chrome = NULL;
	}
	close(fd_input);
	fd_input = -1;
	return read_input;
}

static int init_jpeg_mem(int width, int height)
{
	jpeg_buf = (u8*) malloc(width * height * COLOR_COMPONENTS);
	if (jpeg_buf == NULL) {
		printf("Not enough memory for jpeg buffer.\n");
		return -1;
	}
	memset(jpeg_buf, 0, width * height * COLOR_COMPONENTS);

	return 0;
}

static void free_image_jpeg_buffer()
{
	if (image_data.buffer) {
		free(image_data.buffer);
		image_data.buffer = NULL;
	}
	if (jpeg_buf) {
		free(jpeg_buf);
		jpeg_buf = NULL;
	}
}

void init_destination(j_compress_ptr cinfo) {
	my_destmgr * dest = (my_destmgr*) cinfo->dest;

	/* Allocate the output buffer --- it will be released when done with
	* image */
	dest->buffer = (JOCTET *)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo,
		JPOOL_IMAGE, OUTPUT_BUF_SIZE * sizeof(JOCTET));

	*(dest->written) = 0;

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

boolean empty_output_buffer(j_compress_ptr cinfo)
{
	my_destmgr *dest = (my_destmgr *) cinfo->dest;

	memcpy(dest->outbuffer_cursor, dest->buffer, OUTPUT_BUF_SIZE);
	dest->outbuffer_cursor += OUTPUT_BUF_SIZE;
	*(dest->written) += OUTPUT_BUF_SIZE;

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

	return TRUE;
}

void term_destination(j_compress_ptr cinfo)
{
	my_destmgr * dest = (my_destmgr *) cinfo->dest;
	size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

	/* Write any data remaining in the buffer */
	memcpy(dest->outbuffer_cursor, dest->buffer, datacount);
	dest->outbuffer_cursor += datacount;
	*(dest->written) += datacount;
}

static void dest_buffer(j_compress_ptr cinfo, unsigned char *buffer, int size, int *written)
{
	my_destmgr * dest;

	if (cinfo->dest == NULL) {
		cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)
			((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(my_destmgr));
	}

	dest = (my_destmgr*) cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->outbuffer = buffer;
	dest->outbuffer_size = size;
	dest->outbuffer_cursor = buffer;
	dest->written = written;
}

static int jpeg_encode_yuv_row(u8* output, int quality, image_data_t input)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	int i = 0;
	int j = 0;
	int written = 0;

	u32 size = input.width * input.height;
	u32 quarter_size = size / 4;
	u32 row = 0;

	JSAMPROW y[16];
	JSAMPROW cb[16];
	JSAMPROW cr[16];
	JSAMPARRAY planes[3];

	planes[0] = y;
	planes[1] = cb;
	planes[2] = cr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	dest_buffer(&cinfo, output, input.width * input.height, &written);

	cinfo.image_width = input.width;	/* image width and height, in pixels */
	cinfo.image_height = input.height;
	cinfo.input_components = COLOR_COMPONENTS;	/* # of color components per pixel */
	cinfo.in_color_space = JCS_YCbCr;       /* colorspace of input image */

	jpeg_set_defaults(&cinfo);
	cinfo.raw_data_in = TRUE;	// supply downsampled data
	cinfo.dct_method = JDCT_IFAST;

	cinfo.comp_info[0].h_samp_factor = 2;
	cinfo.comp_info[0].v_samp_factor = 2;
	cinfo.comp_info[1].h_samp_factor = 1;
	cinfo.comp_info[1].v_samp_factor = 1;
	cinfo.comp_info[2].h_samp_factor = 1;
	cinfo.comp_info[2].v_samp_factor = 1;

	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
	jpeg_start_compress(&cinfo, TRUE);

	for (j = 0; j < input.height; j += 16) {
		for (i = 0; i < 16; i++) {
		        row = input.width * (i + j);
		        y[i] = input.buffer + row;
		        if (i % 2 == 0) {
		                cb[i/2] = input.buffer + size + row/4;
		                cr[i/2] = input.buffer + size + quarter_size + row/4;
		        }
		}
		jpeg_write_raw_data(&cinfo, planes, 16);
	}
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	return written;
}
#if 0
static int save_yuv_nv12_buffer(u8* output, iav_yuv_buffer_info_ex_t * info)
{
	u32 i = 0;
	u32 quarter_size = info->pitch * info->height /4;
	u8 *in = info->uv_addr;
	u8 *output_u = output;
	u8 *output_v = output + quarter_size;

	for (i = 0; i < quarter_size; i++) {
		*(output_u++) = *(in++);
		*(output_v++) = *(in++);
	}
	return 0;
}
#endif
static int save_data_in_file(char *filename, u8* data, int size, int method)
{
	int fd = -1;
	int ret = 0;

	if (amba_transfer_init(method) < 0) {
		return -1;
	}

	fd = amba_transfer_open(filename, method, port++);
	if (fd < 0) {
		perror("Failed to open file\n");
		return -1;
	}
	if (amba_transfer_write(fd, data, size, method) < 0) {
		perror("Failed to sava data into file\n");
		ret = -1;
	}
	amba_transfer_close(fd, method);

	if (amba_transfer_deinit(method) < 0) {
		return -1;
	}

	return ret;
}

static int capture_main_buf_to_jpeg(void)
{
	struct timespec lasttime, curtime;
	int jpeg_size = 0;

	if (verbose) {
		clock_gettime(CLOCK_REALTIME, &lasttime);
	}

	//save_yuv_nv12_buffer(image_data.buffer, &info);
	struct timeval time1, time2;
	if (verbose) {
		gettimeofday(&time1, NULL);
	}
	jpeg_size = jpeg_encode_yuv_row(jpeg_buf, jpeg_quality, image_data);
	if (verbose) {
		gettimeofday(&time2, NULL);
		printf("1:encode to jpeg takes time %ld ms\n",(time2.tv_usec - time1.tv_usec)/1000L +(time2.tv_sec - time1.tv_sec) * 1000L);
	}
	if (filename_jpeg[0] != '\0' ) {
		save_data_in_file(filename_jpeg, jpeg_buf, jpeg_size, transfer_method);
		printf("Save JPEG file [%s] OK.\n", filename_jpeg);
	}

	if (verbose) {
		clock_gettime(CLOCK_REALTIME, &curtime);
		printf("JPEG Elapsed Time  (ms): [%05ld]\n", (curtime.tv_sec - lasttime.tv_sec) * 1000
			+ (curtime.tv_nsec - lasttime.tv_nsec) / 1000000);
			lasttime = curtime;
	}
	return 0;
}

static const char *short_options = "q:f:F:y:w:h:tuv";

static struct option long_options[] = {
	{"quailty",	HAS_ARG, 0, 'q'},		/*JPEG quality*/
	{"filename",	HAS_ARG, 0, 'f'},		/*JPEG file*/
	{"file_format", HAS_ARG, 0, 'F'},
	{"yuvfile",	HAS_ARG, 0, 'y'},		/*YUV420_NV12 input file*/
	{"width",		HAS_ARG, 0, 'w'},		/*YUV420_NV12 width*/
	{"height",	HAS_ARG, 0, 'h'},		/*YUV420_NV12 height*/
	{"tcp", 		NO_ARG, 0, 't'},
	{"usb",		NO_ARG, 0,'u'},
	{"verbose",	NO_ARG, 0, 'v'},

	{0, 0, 0, 0}
};

static const struct hint_s hint[] = {
	{"0~100", "set JPEG quaility. Default is 60"},
	{"?.jpeg", "filename to store output JPEG. Default app don't save file"},
	{"",	"specify input file's format,0 for IYUV, 1 for NV12"},
	{"",	"\tinput file name"},
	{"",	"\tYUV width"},
	{"",	"\tYUV height"},
	{"",	"\tuse tcp to send data to PC"},
	{"",	"\tuse usb to send data to PC"},
	{"",	"\tprint more messages"},
};

static void usage(void)
{
	u32 i;
	char *itself = "jpg_enc";
	printf("This program captures main buffer and compress to JPEG file, resolution is the same as main buffer\n");
	printf("\n");
	for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}

	printf("Example:\n> %s -y [NV12_in.yuv] -w 1920 -h 720 -q 70 -f out.jpeg -t\n", itself);
}

static int init_param(int argc, char **argv)
{
	int ch, value;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'q':
			value = atoi(optarg);
			if (value < JPEG_QUALITY_MIN || value > JPEG_QUALITY_MAX) {
				printf("Please set JPEG quality in %d ~ %d.\n",
					JPEG_QUALITY_MIN, JPEG_QUALITY_MAX);
				return -1;
			}
			jpeg_quality = value;
			break;
		case 'f':
			strncpy(filename_jpeg, optarg, sizeof(filename_jpeg));
			break;
		case 'y':
			strncpy(filename_yuv, optarg, sizeof(filename_yuv));
			break;
		case 'F':
			value = atoi(optarg);
			if (value != NV12 && value != IYUV) {
				printf("image format should be only NV12 or IYUV/n");
				return -1;
			}
			image_data.format = value;
			break;
		case 'w':
			image_data.width = atoi(optarg);
			break;
		case 'h':
			image_data.height = atoi(optarg);
			break;
		case 't':
			transfer_method = TRANS_METHOD_TCP;
			break;
		case 'u':
			transfer_method = TRANS_METHOD_USB;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int retv = 0;

	if (argc < 2) {
		usage();
		return -1;
	}
	if (init_param(argc, argv) < 0) {
		usage();
		return -1;
	}

	if (init_image_mem(filename_yuv, image_data.width, image_data.height) < 0) {
		perror("Failed to init image buffer !\n");
		return -1;
	}
	if (init_jpeg_mem(image_data.width, image_data.height) <0) {
		perror("Failed to init jpeg buffer !\n");
		return -1;
	}

	retv = capture_main_buf_to_jpeg();
	if (retv < 0) {
		perror("Capture main buffer to JPEG failed\n");
	}

	free_image_jpeg_buffer();
	return retv;
}

