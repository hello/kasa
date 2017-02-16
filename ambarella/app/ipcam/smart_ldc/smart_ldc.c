#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/sendfile.h>

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
#include <iav_ioctl.h>
#include <jpeglib.h>

#define READ_INTERVAL (30)
#define WRITE_INTERVAL (150)
#define LISTEN_PORT (8080)

const char* filename = "/tmp/snap0.jpg";
const char* filename_tmp = "/tmp/snap0.jpg.tmp";
char *block_header = "----AmbarellaBoundaryS0JIa4uHF7yHd8xJ\r\nContent-Type: image/jpeg\r\n\r\n";
int block_header_len = 0;
int connfd;

int fd_iav;
u8 *dsp_mem = NULL;
u32 dsp_size = 0;
static pthread_mutex_t streamer_mutex = PTHREAD_MUTEX_INITIALIZER;

#define MAX_YUV_BUFFER_SIZE (4096*3000)		// 4096x3000
#define MAX_JPEG_SIZE (1024*1024*5)

typedef struct {
	struct jpeg_destination_mgr pub; /* public fields */
	JOCTET * buffer;    /* start of buffer */
	unsigned char *outbuffer;
	int outbuffer_size;
	unsigned char *outbuffer_cursor;
	int *written;
} my_destmgr;

struct yuv_neon_arg {
	u8 *in;
	u8 *u;
	u8 *v;
	int row;
	int col;
	int pitch;
};

static int outputfile()
{
	unsigned long fSize = 0;
	FILE* file = NULL;
	struct stat filestat;
	pthread_mutex_lock(&streamer_mutex);

	while (!file) {
		file = fopen(filename, "r");
	}

	stat(filename, &filestat);
	fSize = filestat.st_size;

	if (write(connfd, block_header, block_header_len) < 0) {
		pthread_mutex_unlock(&streamer_mutex);
		return -1;
	}

	// use sendfile to avoid mem copy.
	// It's Zero copy now.
	if (sendfile(connfd, fileno(file), NULL, fSize) < 0) {
		pthread_mutex_unlock(&streamer_mutex);
		return -1;
	}
	pthread_mutex_unlock(&streamer_mutex);

	if (write(connfd, "\r\n",2) < 0) {
		return -1;
	}
	fsync(connfd);
	if(file){
	  fclose(file);
	}

	return 0;
}

static int map_buffer(void)
{
	struct iav_querybuf querybuf;

	querybuf.buf = IAV_BUFFER_DSP;
	if (ioctl(fd_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
		perror("IAV_IOC_QUERY_BUF");
		return -1;
	}

	dsp_size = querybuf.length;
	dsp_mem = mmap(NULL, dsp_size, PROT_READ, MAP_SHARED, fd_iav, querybuf.offset);
	if (dsp_mem == MAP_FAILED) {
		perror("mmap (%d) failed: %s\n");
		return -1;
	}

	return 0;
}

static int check_state(void)
{
	int state;
	if (ioctl(fd_iav, IAV_IOC_GET_IAV_STATE, &state) < 0) {
		perror("IAV_IOC_GET_IAV_STATE");
		exit(-1);
	}

	if ((state != IAV_STATE_PREVIEW) && state != IAV_STATE_ENCODING) {
		printf("IAV is not in preview / encoding state !\n");
		exit(-1);
	}

	return 0;
}

static int save_yuv_luma_buffer(u8* output, struct iav_yuvbufdesc *yuv_desc)
{
	int i;
	u8 *in;
	u8 *out;

	if (yuv_desc->pitch < yuv_desc->width) {
		printf("pitch size smaller than width!\n");
		return -1;
	}

	if (yuv_desc->pitch == yuv_desc->width) {
		memcpy(output, dsp_mem + yuv_desc->y_addr_offset,
			yuv_desc->width * yuv_desc->height);
	} else {
		in = dsp_mem + yuv_desc->y_addr_offset;
		out = output;
		for (i = 0; i < yuv_desc->height; i++) {		//row
			memcpy(out, in, yuv_desc->width);
			in += yuv_desc->pitch;
			out += yuv_desc->width;
		}
	}

	return 0;
}

extern void chrome_convert(struct yuv_neon_arg *);

static int jpeg_encode_yuv_row(FILE *fp, u8 *input, int quality, int width, int height)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	int i = 0;
	int j = 0;
	int written = 0;

	u32 size = width * height;
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
	jpeg_stdio_dest(&cinfo, fp);

	cinfo.image_width = width;	/* image width and height, in pixels */
	cinfo.image_height = height;
	cinfo.input_components = 3;	/* # of color components per pixel */
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

	jpeg_set_quality(&cinfo, quality, TRUE); /* limit to baseline-JPEG values */
	jpeg_start_compress(&cinfo, TRUE);

	for (j = 0; j < height; j += 16) {
		for (i = 0; i < 16; i++) {
		        row = width * (i + j);
		        y[i] = input + row;
		        if (i % 2 == 0) {
		                cb[i/2] = input + size + row / 4;
		                cr[i/2] = input + size + quarter_size + row / 4;
		        }
		}
		jpeg_write_raw_data(&cinfo, planes, 16);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	return written;
}

void *create_jpeg_thread(void *arg)
{
	u8 * luma_chroma = NULL;
	struct iav_yuvbufdesc *yuv_desc;
	struct iav_querydesc query_desc;
	struct yuv_neon_arg yuv_arg;
	FILE *fd_tmp = NULL;

	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		exit(-1);
	}

	if (map_buffer() < 0)
		exit(-1);

	//check iav state
	if (check_state() < 0)
		exit(-1);

	luma_chroma = malloc(MAX_YUV_BUFFER_SIZE * 2);
	if (luma_chroma == NULL) {
		printf("Not enough memory for preview capture !\n");
		exit(-1);;
	}

	memset(&query_desc, 0, sizeof(query_desc));
	query_desc.qid = IAV_DESC_YUV;
	query_desc.arg.yuv.buf_id = IAV_SRCBUF_MN;
	yuv_desc = &query_desc.arg.yuv;

	//Main loop
	while (1) {
		pthread_mutex_lock(&streamer_mutex);
		fd_tmp = fopen(filename_tmp, "w+");
		if (ioctl(fd_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
			perror("IAV_IOC_QUERY_DESC");
			pthread_mutex_unlock(&streamer_mutex);
			continue;
		}

		if (((u8 *)yuv_desc->y_addr_offset == NULL) || ((u8 *)yuv_desc->uv_addr_offset == NULL)) {
			printf("YUV buffer address is NULL!\n");
			pthread_mutex_unlock(&streamer_mutex);
			continue;
		}

		if (save_yuv_luma_buffer(luma_chroma, yuv_desc) < 0) {
			perror("Failed to save luma data into buffer !\n");
			pthread_mutex_unlock(&streamer_mutex);
			continue;
		}

		yuv_arg.in = dsp_mem + yuv_desc->uv_addr_offset;
		yuv_arg.u = luma_chroma + yuv_desc->width * yuv_desc->height;
		yuv_arg.v = yuv_arg.u + yuv_desc->width * yuv_desc->height / 4;
		yuv_arg.row = yuv_desc->height / 2;
		yuv_arg.col = yuv_desc->width;
		yuv_arg.pitch = yuv_desc->pitch;

		chrome_convert(&yuv_arg);
		jpeg_encode_yuv_row(fd_tmp, luma_chroma, 50, yuv_desc->width, yuv_desc->height);

		fclose(fd_tmp);

		rename(filename_tmp, filename);
		pthread_mutex_unlock(&streamer_mutex);
		usleep(WRITE_INTERVAL * 1000);
	}

	return NULL;
}

int main()
{
	int listenfd;
	struct sockaddr_in servaddr;
	int flag=1,len=sizeof(int);
	int response_header_len;
	pthread_t create_jpeg_tid;

	signal(SIGPIPE, SIG_IGN);

	block_header_len = strlen(block_header);

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ) {
		printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
		return -1;
	}

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, len) < 0) {
		perror("setsockopt");
		return -1;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(LISTEN_PORT);

	if( bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
		printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
		return -1;
	}

	if (listen(listenfd, 10) == -1) {
		printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
		return -1;
	}

	char *response_header = "HTTP/1.1 200 OK\r\nContent-type: multipart/x-mixed-replace; boundary=----AmbarellaBoundaryS0JIa4uHF7yHd8xJ\r\n\r\n";
	response_header_len = strlen(response_header);

	pthread_create(&create_jpeg_tid, NULL, create_jpeg_thread, NULL);
	pthread_detach(create_jpeg_tid);

ACCEPT:

	if (connfd) {
		shutdown(connfd, SHUT_RDWR);
		close(connfd);
	}
	connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
	write(connfd, response_header, response_header_len);

	while (1) {
	if (outputfile() < 0) {
		goto ACCEPT;
	}
		usleep(READ_INTERVAL * 1000);
	}

	return 0;

}

