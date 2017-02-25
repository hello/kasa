/*
 * encode_test.c
 *	the program just test the DSP when run 4 streaming start/stop,
 *
 * History:
 *	2014/07/25 - [Jingyang Qiu] create this file base on test_encode.c
 *
 * Copyright (C) 2015 Ambarella, Inc.
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
 #include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <basetypes.h>
#include <iav_ioctl.h>

static int fd = 0;

static int streams = 0;
static pthread_cond_t streams_cond;
static pthread_mutex_t streams_mutex;

static pthread_mutex_t encode_mutex;

static pthread_t threads[4];

struct config {
	int buffer;
	int stream;
	int width;
	int height;
};

static struct config configs[4] = {
	{ .buffer = 0, .stream = 0, .width = 1920, .height = 1080 },
	{ .buffer = 1, .stream = 1, .width = 720, .height = 480 },
	{ .buffer = 1, .stream = 2, .width = 720, .height = 480 },
	{ .buffer = 1, .stream = 3, .width = 720, .height = 480 },
};

static int streams_per_buffer[4] = { 0, 0, 0, 0 };

static void setup_source_buffer_format(int buffer, int w, int h) {
	struct iav_srcbuf_format format;

	if(buffer == 0)
		return;

	memset(&format, 0, sizeof(format));
	format.buf_id = buffer;

	if (ioctl(fd, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &format) < 0) {
		printf("IAV_IOC_GET_SOURCE_BUFFER_FORMAT failed\n");
		return;
	}

	format.size.width = w;
	format.size.height = h;

	if (ioctl(fd, IAV_IOC_SET_SOURCE_BUFFER_FORMAT, &format) < 0) {
		printf("IAV_IOC_SET_SOURCE_BUFFER_FORMAT failed\n");
		return;
	}

	printf("set buffer %d to %dx%d\n", buffer, w, h);
}

static void buf_alloc(int buffer, int w, int h) {
	if (streams_per_buffer[buffer]++ > 0) {
		return;
	}

	setup_source_buffer_format(buffer, w, h);
}

static void buf_free(int buffer) {
	if (--streams_per_buffer[buffer] > 0)
		return;

	setup_source_buffer_format(buffer, 0, 0);
}

static void sl(int x) {
	printf("sleep %d\n", x/1000);
	usleep(x);
}

static void* client_thread(void *arg) {
	struct config *c = (struct config *) arg;

	printf("Thread %d started\n",c->stream);
	sl(rand() / 1000);

	while (1) {
		sl(RAND_MAX / 10000 + rand() / 10000);

		pthread_mutex_lock(&streams_mutex);
		streams++;
		pthread_mutex_unlock(&streams_mutex);

		pthread_mutex_lock(&encode_mutex);
		buf_alloc(c->buffer, c->width, c->height);
		printf("starting %d\n", c->stream);
		if (ioctl(fd, IAV_IOC_START_ENCODE, (1 << c->stream)) < 0)
			return NULL;
		printf("started stream %d\n", c->stream);
		pthread_mutex_unlock(&encode_mutex);

		pthread_mutex_lock(&streams_mutex);
		pthread_cond_signal(&streams_cond);
		pthread_mutex_unlock(&streams_mutex);

		sl(RAND_MAX / 1000 + rand() / 1000);

		pthread_mutex_lock(&streams_mutex);
		streams--;
		pthread_cond_signal(&streams_cond);
		pthread_mutex_unlock(&streams_mutex);

		pthread_mutex_lock(&encode_mutex);
		printf("stopping stream %d\n", c->stream);
		if (ioctl(fd, IAV_IOC_STOP_ENCODE, (1 << c->stream)) < 0)
			return NULL;
		printf("stopped stream %d\n", c->stream);
		buf_free(c->buffer);
		pthread_mutex_unlock(&encode_mutex);

		pthread_mutex_lock(&streams_mutex);
		if (streams == 0)
			printf("both streams stopped\n");
		pthread_mutex_unlock(&streams_mutex);
	}
}

int main(int argc, char **argv) {

	fd = open("/dev/iav", O_RDWR);

	pthread_cond_init(&streams_cond, NULL);
	pthread_mutex_init(&streams_mutex, NULL);
	pthread_mutex_init(&encode_mutex, NULL);

	int i;

	for (i = 0; i < 4; i++) {
		pthread_create(&threads[i], NULL, client_thread, (void *) &configs[i]);
	}

	while (1) {
		struct iav_querydesc query_desc;
		struct iav_framedesc *frame_desc;

		pthread_mutex_lock(&streams_mutex);
		while (streams == 0) {
			printf("waiting a stream to start\n");
			pthread_cond_wait(&streams_cond, &streams_mutex);
			printf("a stream started\n");
		}
		pthread_mutex_unlock(&streams_mutex);

		//printf("reading frame\n");
		memset(&query_desc, 0, sizeof(query_desc));
		frame_desc = &query_desc.arg.frame;
		query_desc.qid = IAV_DESC_FRAME;
		frame_desc->id = -1;
		if (ioctl(fd, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
			perror("IAV_IOC_QUERY_DESC");
			return -1;
		}
	}

	return 0;
}
