/*******************************************************************************
 * sd_benchmark.c
 *
 * History:
 *   Jan 19, 2016 - [zfgong] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <memory.h>
#include <fcntl.h>
#include <ctype.h>
#include <getopt.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>

#define	NO_ARG		0
#define	HAS_ARG		1


#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define CLASS2			(2)
#define CLASS4			(4)
#define CLASS6			(6)
#define CLASS10			(10)

//The empiric value is from QA test result,
//SONY microSDHC I 16GB class 10 is 2.51MB/s
#define TOLERANCE_RATIO		(0.25)

#define VERSION 	"1.0.0"


#define MAX_FD_NUM	(5)
#define MAX_RETRY_CNT	(3)
#define DFT_MOUNT_POINT	"/sdcard"
static char mount_point[128];
static int len_1MB = 1*1024*1024;
static void *buf_1MB = NULL;

static int mmcblk_id = 0;
static int fixed_step_flag = 0;
static float speed_inst = 0.0;
static float speed_inst_min = 0.0;
static float speed_inst_max = 0.0;

static float speed_avg = 0.0;
static float speed_avg_min = 0.0;
static float speed_avg_max = 0.0;

struct file_desc {
	union {
		int fd;
		FILE *fp;
	};
};

struct task_info_t {
	struct file_desc *fd;
	int total;
	int wait;
	char path[32];
	pthread_cond_t cond;
	pthread_mutex_t mutex;
};

struct hint_s {
	const char *arg;
	const char *str;
};

struct sdcard_desc {
	char dev[32];
	char desc[64];
};

struct sdcard_desc sdcard_info[] = {
	{"name", ""},
	{"cid", ""},
	{"csd", ""},
	{"scr", ""},
	{"serial", ""},
	{"manfid", ""},
	{"oemid", ""},
	{"date", ""},
};

typedef struct file_ops {
	struct file_desc * (*open)(const char *path);
	ssize_t (*write)(struct file_desc *fd, const void *buf, size_t count);
	ssize_t (*read)(struct file_desc *fd, void *buf, size_t count);
	off_t (*seek)(struct file_desc *fd, off_t offset, int whence);
	int (*sync)(struct file_desc *fd);
	void (*close)(struct file_desc *fd);
} file_ops_t;


static struct file_ops *file_handle = NULL;

static int range_random(int min, int max)
{
	int val;
	uint64_t tick;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	tick = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

	srandom((unsigned int)tick);
	if (min < max) {
		val = random() % (max - min + 1) + min;
	} else {
		val = random() % (min - max + 1) + max;
	}
	return val;
}

static void get_sdcard_info()
{
	int i = 0;
	int ret = 0;
	int fd = 0;
	char path[128];
	char prefix[128];
	snprintf(prefix, sizeof(prefix), "/sys/block/mmcblk%d/device", mmcblk_id);
	printf("prefix = %s\n", prefix);
	for (i = 0; i < ARRAY_SIZE(sdcard_info); i++) {
		snprintf(path, sizeof(path), "%s/%s",
				prefix, sdcard_info[i].dev);
		fd = open(path, O_RDONLY, 0666);
		if (fd == -1) {
			printf("open %s failed:%d %s\n",
					path, errno, strerror(errno));
			break;
		}
		ret = read(fd, sdcard_info[i].desc,
				sizeof(sdcard_info[i].desc));
		if (ret == -1) {
			printf("read failed:%d %s\n", errno, strerror(errno));
			return;
		}
		close(fd);
	}
	printf("sdcard info:\n");
	for (i = 0; i < ARRAY_SIZE(sdcard_info); i++) {
		printf("%10s: %s", sdcard_info[i].dev, sdcard_info[i].desc);
	}
	printf("\n");
}

static float get_speed(struct timeval *pre, struct timeval *post, int bytes)
{
	float cost_sec = ((1000000 * (post->tv_sec - pre->tv_sec))
			+ (post->tv_usec - pre->tv_usec)) / 1000000.0;
	float mega_bytes = bytes / (1024 * 1024.0);
	float speed = mega_bytes / cost_sec;
	if (speed_inst_max == 0 || speed_inst_min == 0) {
		speed_inst_min = speed;
		speed_inst_max = speed;
	} else {
		if (speed_inst_max < speed) {
			speed_inst_max = speed;
		}
		if (speed_inst_min > speed) {
			speed_inst_min = speed;
		}
	}
	return speed;
}

static void print_speed(struct timeval *pre, struct timeval *post,
		int bytes, char *prefix)
{
	float cost_sec = ((1000000 * (post->tv_sec - pre->tv_sec))
			+ (post->tv_usec - pre->tv_usec)) / 1000000.0;
	float mega_bytes = bytes / (1024 * 1024.0);
	speed_avg = mega_bytes / cost_sec;
	if (speed_avg_max == 0 || speed_avg_min == 0) {
		speed_avg_min = speed_avg;
		speed_avg_max = speed_avg;
	} else {
		if (speed_avg_max < speed_avg) {
			speed_avg_max = speed_avg;
		}
		if (speed_avg_min > speed_avg) {
			speed_avg_min = speed_avg;
		}
	}
	fprintf(stderr, "\r%8s\t %6.2fMB\t %6.2fs\t "
			"%4.2fMB/s\t %4.2fMB/s\t %4.2fMB/s\t "
			"%4.2fMB/s\t %4.2fMB/s\t %4.2fMB/s\t ",
			prefix, mega_bytes, cost_sec,
			speed_inst, speed_inst_min, speed_inst_max,
			speed_avg, speed_avg_min, speed_avg_max);
}

static int invalidate_cache(int fd, off_t len)
{
	int ret = 0;
	int page_size;
	int offset = lseek(fd, 0, SEEK_CUR);
	page_size = getpagesize();
	offset = offset % page_size;
	ret = posix_fadvise (fd, offset, len, POSIX_FADV_DONTNEED);
	if (ret != 0) {
		return -1;
	}
	return 0;
}

static struct file_desc *fio_open(const char *path)
{
	struct file_desc *file = (struct file_desc *)calloc(1,
			sizeof(struct file_desc));
	if (!file) {
		printf("malloc failed:%d %s\n", errno, strerror(errno));
		return NULL;
	}
	file->fp = fopen(path, "w");
	if (!file->fp) {
		printf("fopen %s failed:%d %s\n",
				path, errno, strerror(errno));
		free(file);
		return NULL;
	}
	return file;
}

static ssize_t fio_read(struct file_desc *file, void *buf, size_t len)
{
	int n;
	void *p = buf;
	size_t left = len;
	size_t step = 1024*1024;
	int cnt = 0;
	if (file == NULL || buf == NULL || len == 0) {
		printf("%s paraments invalid!\n", __func__);
		return -1;
	}
	FILE *fp = file->fp;
	while (left > 0) {
		if (left < step)
			step = left;
		n = fread(p, 1, step, fp);
		if (n > 0) {
			p += n;
			left -= n;
			continue;
		} else {
			if (0 == feof(fp)) {
				break;
			} else {
				if (errno == EINTR || errno == EAGAIN) {
					if (++cnt > MAX_RETRY_CNT)
						break;
					continue;
				}
			}
		}
	}
	return (len - left);
}

static ssize_t fio_write(struct file_desc *file, const void *buf, size_t count)
{
	ssize_t ret = 0;
	ssize_t written;
	void *cursor = (void *)buf;
	size_t step;
	size_t remain = count;
	int retry = 0;
	struct timeval pre = {0, 0}, post = {0, 0};
	FILE *fp = NULL;

	do {
		if (file == NULL || buf == NULL || count == 0) {
			printf("%s paraments invalid, buf=%p, "
#if __WORDSIZE == 64
			       "count=%lu!\n",
#else
			       "count=%u!\n",
#endif
			       __func__, buf, count);
			ret = -1;
			break;
		}
		fp = file->fp;
		gettimeofday(&pre, NULL);
		while (remain > 0) {
			if (fixed_step_flag) {
				step = 1024 * 1024;
			} else {
				step = range_random(128*1024, 1024* 1024);
			}
			if (remain < step)
				step = remain;
			written = fwrite(cursor, 1, step, fp);
			if (written > 0) {
				cursor += written;
				remain -= written;
				gettimeofday(&post, NULL);
				speed_inst = get_speed(&pre, &post,
						count-remain);
				if (fflush(fp) < 0) {
					printf("fsync failed:%d %s\n",
						errno, strerror(errno));

				}
				continue;
			} else {
				if (errno == EINTR || errno == EAGAIN) {
					if (++retry > MAX_RETRY_CNT) {
						printf("reach max retry\n");
						break;
					}
					continue;
				} else {
					printf("write failed:%d %s\n",
						errno, strerror(errno));
					ret = -1;
					break;
				}
			}
		}
		ret = count - remain;
	} while (0);
	return ret;
}

static off_t fio_seek(struct file_desc *file, off_t offset, int whence)
{
	if (file) {
		return fseek(file->fp, offset, whence);
	}
	return -1;
}

static int fio_sync(struct file_desc *file)
{
	if (file) {
		return fsync(file->fd);
	}
	return -1;
}

static void fio_close(struct file_desc *file)
{
	if (file) {
		fclose(file->fp);
		free(file);
	}
}

static struct file_desc *io_open(const char *path)
{
	struct file_desc *file = (struct file_desc *)calloc(1,
			sizeof(struct file_desc));
	if (!file) {
		printf("malloc failed:%d %s\n", errno, strerror(errno));
		return NULL;
	}
	file->fd = open(path, O_CREAT | O_TRUNC | O_RDWR | O_SYNC, 0666);
	if (file->fd == -1) {
		printf("open %s failed:%d %s\n", path, errno, strerror(errno));
		free(file);
		return NULL;
	}
	return file;
}

static ssize_t io_read(struct file_desc *file, void *buf, size_t len)
{
	int n;
	void *p = buf;
	size_t left = len;
	size_t step = 1024*1024;
	int cnt = 0;
	if (file == NULL || buf == NULL || len == 0) {
		printf("%s paraments invalid!\n", __func__);
		return -1;
	}
	int fd = file->fd;
	while (left > 0) {
		if (left < step)
			step = left;
		n = read(fd, p, step);
		if (n > 0) {
			p += n;
			left -= n;
			continue;
		} else {
			if (n == 0) {
				break;
			} else {
				if (errno == EINTR || errno == EAGAIN) {
					if (++cnt > MAX_RETRY_CNT)
						break;
					continue;
				}
			}
		}
	}
	return (len - left);
}

static ssize_t io_write(struct file_desc *file, const void *buf, size_t count)
{
	ssize_t ret = 0;
	ssize_t written;
	void *cursor = (void *)buf;
	size_t step;
	size_t remain = count;
	int retry = 0;
	struct timeval pre = {0, 0}, post = {0, 0};
	int fd = -1;

	do {
		if (file == NULL || buf == NULL || count == 0) {
			printf("%s paraments invalid, fd=%d, buf=%p, "
#if __WORDSIZE == 64
				"count=%lu!\n",
#else
				"count=%u!\n",
#endif
			       __func__, fd, buf, count);
			ret = -1;
			break;
		}
		fd = file->fd;
		ret = invalidate_cache(fd, count);
		if (ret != 0) {
			printf("invalidate_cache failed!\n");
		}
		gettimeofday(&pre, NULL);
		while (remain > 0) {
			if (fixed_step_flag) {
				step = 1024 * 1024;
			} else {
				step = range_random(128*1024, 1024* 1024);
			}
			if (remain < step)
				step = remain;
			written = write(fd, cursor, step);
			if (written > 0) {
				cursor += written;
				remain -= written;
				gettimeofday(&post, NULL);
				speed_inst = get_speed(&pre, &post,
						count-remain);
				if (fsync(fd) < 0) {
					printf("fsync failed:%d %s\n",
						errno, strerror(errno));

				}
				continue;
			} else {
				if (errno == EINTR || errno == EAGAIN) {
					if (++retry > MAX_RETRY_CNT) {
						printf("reach max retry\n");
						break;
					}
					continue;
				} else {
					printf("write failed:%d %s\n",
						errno, strerror(errno));
					ret = -1;
					break;
				}
			}
		}
		ret = count - remain;
	} while (0);
	return ret;
}

static off_t io_seek(struct file_desc *file, off_t offset, int whence)
{
	if (file) {
		return lseek(file->fd, offset, whence);
	}
	return -1;
}

static int io_sync(struct file_desc *file)
{
	if (file) {
		return fflush(file->fp);
	}
	return -1;
}

static void io_close(struct file_desc *file)
{
	if (file) {
		close(file->fd);
		free(file);
	}
}

static struct file_ops fio_ops = {
	.open  = fio_open,
	.write = fio_write,
	.read  = fio_read,
	.seek  = fio_seek,
	.sync  = fio_sync,
	.close = fio_close,
};

static struct file_ops io_ops = {
	.open  = io_open,
	.write = io_write,
	.read  = io_read,
	.seek  = io_seek,
	.sync  = io_sync,
	.close = io_close,
};

static void *wait_task(void *arg)
{
	struct task_info_t *tp = (struct task_info_t *)arg;
	pthread_mutex_lock(&tp->mutex);
	pthread_cond_wait(&tp->cond, &tp->mutex);
	pthread_mutex_unlock(&tp->mutex);
	pthread_mutex_destroy(&tp->mutex);
	pthread_cond_destroy(&tp->cond);
	if (tp->fd) {
		file_handle->close(tp->fd);
		tp->fd = NULL;
	}
	return NULL;
}

static void *write_task(void *arg)
{
	struct task_info_t *tp = (struct task_info_t *)arg;
	struct timeval pre = {0, 0}, post = {0, 0};
	char prefix[32];

	int i, ret = 0;
	gettimeofday(&pre, NULL);
	for (i = 0; i < tp->total/len_1MB; i++) {
		ret = file_handle->write(tp->fd, buf_1MB, len_1MB);
		if (ret < 0) {
			printf("write fd[%d] failed!\n", i);
			break;
		}
		gettimeofday(&post, NULL);
		snprintf(prefix, sizeof(prefix), "%s", tp->path);
		print_speed(&pre, &post, (i+1)*len_1MB, prefix);
	}
	pthread_mutex_lock(&tp->mutex);
	pthread_cond_signal(&tp->cond);
	pthread_mutex_unlock(&tp->mutex);

	return NULL;
}

static int thread_write(int num, int total)
{
	pthread_t tid;
	int i;
	int ret = 0;
	int wait = 0;
	struct task_info_t *tp = NULL;
	struct task_info_t *tlist = NULL;

	if (num > MAX_FD_NUM) {
		num = MAX_FD_NUM;
	}
	printf("thread_write num = %d, %d MB bytes:\n", num, total/len_1MB);
	printf("%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\n",
		"path", "written", "cost",
		"*inst_speed", "inst_min", "inst_max",
		"*avg_speed", "avg_min", "avg_max");
	do {
		tlist = (struct task_info_t *)calloc(num,
				sizeof(struct task_info_t));
		if (!tlist) {
			printf("%s: malloc failed!\n", __func__);
			ret = -1;
			break;
		}
		for (i = 0; i < num; i++) {
			tp = tlist+i;
			snprintf(tp->path, sizeof(tp->path), "%s/%s%d",
					mount_point, "tfile", i);
			tp->fd = file_handle->open(tp->path);
			if (!tp->fd) {
				ret = -1;
				continue;
			}
			tp->total = total;
			pthread_mutex_init(&tp->mutex, NULL);
			pthread_cond_init(&tp->cond, NULL);
		}
		for (i = 0; i < num; i++) {
			tp = tlist+i;
			if (!tp->fd) {
				continue;
			}
			pthread_create(&tid, NULL, write_task, tp);
			pthread_create(&tid, NULL, wait_task, tp);
		}
	} while (0);

	if (tlist) {
		while (1) {
			for (i = 0; i < num; i++) {
				tp = tlist+i;
				if (tp->fd) {
					tp->wait = 1;
				} else {
					tp->wait = 0;
				}
			}
			for (i = 0, wait = 0; i < num; i++) {
				tp = tlist+i;
				wait += tp->wait;
			}
			if (!wait) {
				break;
			}
			sleep(1);
		}
		free(tlist);
	}
	return ret;
}

static int infinite_write()
{
	int loop = 0;
	int ret = 0;
	int block, remain;
	void *ptr;
	char path[32];
	struct timeval pre = {0, 0}, post = {0, 0};
	struct file_desc *fd = NULL;
	int total = 1*1024*1024*1024;
	printf("infinite_write %d MB bytes:\n", total/len_1MB);
	printf("%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\n",
		"path", "written", "cost",
		"*inst_speed", "inst_min", "inst_max",
		"*avg_speed", "avg_min", "avg_max");
	do {
		snprintf(path, sizeof(path), "%s/%s", mount_point, "ifile");
		fd = file_handle->open(path);
		if (!fd) {
			ret = -1;
			break;
		}
		gettimeofday(&pre, NULL);
		for (loop = 0; ; loop++) {
			remain = total;
			block = len_1MB;
			ptr = buf_1MB;
			while (remain > 0) {
				if (remain < block) {
					block = remain;
				}
				if (ptr - buf_1MB >= len_1MB) {
					ptr = buf_1MB;
				}
				ret = file_handle->write(fd, ptr, block);
				if (ret > 0) {
					ptr += ret;
					remain -= ret;
					gettimeofday(&post, NULL);
					print_speed(&pre, &post,
						(total-remain)*(loop+1), path);
				} else {
					printf("write failed!\n");
					break;
				}
			}
			file_handle->seek(fd, 0, SEEK_SET);
		}
	} while (0);
	if (fd) {
		file_handle->close(fd);
	}
	return ret;
}

static int cross_write(int num, int total)
{
	int i, j;
	int total_mb;
	int ret = 0;
	struct file_desc *fd[MAX_FD_NUM];
	char path[MAX_FD_NUM][32];
	char prefix[32];
	struct timeval pre = {0, 0}, post = {0, 0};

	printf("cross_write num = %d, %d MB bytes:\n", num, total/len_1MB);
	printf("%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\n",
		"path", "written", "cost",
		"*inst_speed", "inst_min", "inst_max",
		"*avg_speed", "avg_min", "avg_max");
	if (num > MAX_FD_NUM) {
		num = MAX_FD_NUM;
	}
	total_mb = total / len_1MB;
	do {
		for (i = 0; i < num; i++) {
			snprintf(path[i], sizeof(path[i]), "%s/%s%d",
					mount_point, "cfile", i);
			fd[i] = file_handle->open(path[i]);
			if (!fd[i]) {
				ret = -1;
				break;
			}
		}
		gettimeofday(&pre, NULL);
		for (j = 0; j < total_mb; j++) {
			for (i = 0; i < num; i++) {
				ret = file_handle->write(fd[i], buf_1MB,
						len_1MB);
				if (ret < 0) {
					printf("write fd[%d] failed!\n", i);
					break;
				}
				gettimeofday(&post, NULL);
				snprintf(prefix, sizeof(prefix), "%s", path[i]);
				print_speed(&pre, &post, ((j*num)+i+1)*len_1MB,
						prefix);
			}
		}
		printf("\n");
	} while (0);

	for (i = 0; i < num; i++) {
		if (fd[i]) {
			file_handle->close(fd[i]);
		}
	}
	return ret;
}

static int single_write(int total)
{
	int ret = 0;
	int block, remain;
	void *ptr;
	char path[32];
	struct timeval pre = {0, 0}, post = {0, 0};
	struct file_desc *fd = NULL;

	printf("single_write %d MB bytes:\n", total/len_1MB);
	printf("%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\n",
		"path", "written", "cost",
		"*inst_speed", "inst_min", "inst_max",
		"*avg_speed", "avg_min", "avg_max");
	do {
		snprintf(path, sizeof(path), "%s/%s", mount_point, "sfile");
		fd = file_handle->open(path);
		if (!fd) {
			ret = -1;
			break;
		}
		remain = total;
		block = len_1MB;
		ptr = buf_1MB;
		gettimeofday(&pre, NULL);
		while (remain > 0) {
			if (remain < block) {
				block = remain;
			}
			if (ptr - buf_1MB >= len_1MB) {
				ptr = buf_1MB;
			}
			ret = file_handle->write(fd, ptr, block);
			if (ret > 0) {
				ptr += ret;
				remain -= ret;
				gettimeofday(&post, NULL);
				print_speed(&pre, &post, total-remain, path);
			} else {
				printf("write failed!\n");
				break;
			}
		}
		printf("\n");
	} while (0);
	if (fd) {
		file_handle->close(fd);
	}
	return ret;
}

static int analysis(int target, float factor)
{
	int ratio = (int)(factor*100);
	if (speed_avg < (float)(target * factor)) {
		printf("average speed is %d%% slower than Class %d speed.\n",
				ratio, target);
		return -1;
	}
	printf("average speed reach %d%% Class %d speed.\n", ratio, target);
	return 0;
}

static int init_buffer()
{
	int i = 0;
	buf_1MB = calloc(1, len_1MB);
	if (!buf_1MB) {
		printf("calloc failed:%d %s\n", errno, strerror(errno));
		return -1;
	}
	for (i = 0; i < len_1MB; i++) {
		*((char *)buf_1MB + i) = i;
	}
	return 0;
}

static void deinit_buffer()
{
	if (buf_1MB) {
		free(buf_1MB);
		buf_1MB = NULL;
	}
	file_handle = NULL;
}

static void sig_handle()
{
	deinit_buffer();
	exit(0);
}

static const char *short_options = "bB:c:C:Ff:im:p:r:s:t:vh";

static const struct hint_s hint[] = {
	{"",       "\t\t\t" "using buffer-io(fopen/fwrite),"
		            " default non-buffer(open/write)"},
	{"0~1",    "\t\t"   "slot id of mmcblk"},
	{"1~1024", "\t"     "MB bytes to cross write in one thread"},
	{"1~10",   "\t"     "cross file number"},
	{"2,4,6,10","\t"    "specify the CLASS level"},
	{"",       "\t\t"   "fixed step write 1MB"},
	{"",       "\t\t"   "infinite write 1024MB"},
	{"1~1024", "\t"     "MB bytes to multi thread write"},
	{"",       "\t\t"   "mount point of /dev/mmcblkn"},
	{"0.1~1.0","\t\t"   "tolerance ratio of class level"},
	{"1~1024", "\t"     "MB bytes to single write"},
	{"1~5",    "\t"     "multi thread number"},
	{"",       "\t"     "version"},
	{"help",   "\t\t"   "show usage\n" "e.g.\n"
		"./sd_benchmark -s 10 (single file write 10 MB)\n"
		"./sd_benchmark -i 10 (infinite file write 10 MB)\n"
		"./sd_benchmark -c 10 -f 2 (cross write 10MB to 2 files)\n"
		"./sd_benchmark -m 10 -t 3 (3 thread write 10MB to 3 files)\n"
	},
	{"", ""},
};

static struct option long_options[] = {
	{"buffer-io",      NO_ARG,  0, 'b'},
	{"mmcblk",         HAS_ARG, 0, 'B'},
	{"cross-write",    HAS_ARG, 0, 'c'},
	{"CLASS level",    HAS_ARG, 0, 'C'},
	{"file-number",    HAS_ARG, 0, 'f'},
	{"fixed-step",     NO_ARG,  0, 'F'},
	{"infinite-write", NO_ARG,  0, 'i'},
	{"thread-write",   HAS_ARG, 0, 'm'},
	{"mount-point",    HAS_ARG, 0, 'p'},
	{"tolerance-ratio",HAS_ARG, 0, 'r'},
	{"single-write",   HAS_ARG, 0, 's'},
	{"thread-number",  HAS_ARG, 0, 't'},
	{"version",        NO_ARG,  0, 'v'},
	{"help",           NO_ARG,  0, 'h'},
	{"", NO_ARG, 0, 0},
};

void usage(int argc, char **argv)
{
	int i;
	printf("%s usage:\n", argv[0]);
	for (i = 0; i < sizeof(long_options)/sizeof(long_options[0])-1; i++) {
		if (isalpha(long_options[i].val))
			printf("-%c, ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
}

int parse_param(int argc, char **argv)
{
	int buffer_io_flag = 0;
	int infinite_write_flag = 0;
	int single_write_flag = 0;
	int thread_write_flag = 0;
	int cross_write_flag = 0;
	int class_check_flag = 0;
	int class_lvl = CLASS10;
	float tolerance = TOLERANCE_RATIO;
	int file_num = 0;
	int thread_num = 0;
	int mega = 0;
	int ch = 0;
	int ret = 0;
	int opt_index = 0;
	snprintf(mount_point, sizeof(mount_point), "%s", DFT_MOUNT_POINT);
	while ((ch = getopt_long(argc, argv, short_options,
					long_options, &opt_index)) != -1) {
		switch (ch) {
		case 'b':
			buffer_io_flag =1;
			break;
		case 'B':
			mmcblk_id = strtoul(optarg, NULL, 10);
			printf("mmcblk_id = %d\n", mmcblk_id);
			break;
		case 'c':
			mega = strtoul(optarg, NULL, 10);
			if (mega > 1024) {
				printf("write data too large, max is 1024MB\n");
				mega = 1024;
			}
			cross_write_flag = 1;
			break;
		case 'C':
			class_lvl = strtoul(optarg, NULL, 10);
			if (!(class_lvl == CLASS2 || class_lvl == CLASS4
				|| class_lvl == CLASS6 || class_lvl == CLASS10)) {
				printf("invalid CLASS level, must be 2|4|6|10,"
						"default using CLASS2\n");
				class_lvl = CLASS2;
			}
			class_check_flag = 1;
			break;
		case 'f':
			file_num = strtoul(optarg, NULL, 10);
			break;
		case 'F':
			fixed_step_flag = 1;
			break;
		case 'i':
			infinite_write_flag = 1;
			break;
		case 'm':
			mega = strtoul(optarg, NULL, 10);
			if (mega > 1024) {
				printf("write data too large, max is 1024MB\n");
				mega = 1024;
			}
			thread_write_flag = 1;
			break;
		case 'p':
			snprintf(mount_point, sizeof(mount_point), "%s", optarg);
			printf("mount_point is %s\n", mount_point);
			break;
		case 'r':
			tolerance = atof(optarg);
			if (tolerance > 1 || tolerance < 0) {
				printf("tolerance ratio must be (0, 1],"
					"default is %0.2f\n", TOLERANCE_RATIO);
				tolerance = TOLERANCE_RATIO;
			}
			break;
		case 's':
			mega = strtoul(optarg, NULL, 10);
			if (mega > 1024) {
				printf("write data too large, max is 1024MB\n");
				mega = 1024;
			}
			single_write_flag = 1;
			break;
		case 't':
			thread_num = strtoul(optarg, NULL, 10);
			printf("thread number %d\n", thread_num);
			break;
		case 'v':
			printf("version: %s\n", VERSION);
			break;
		default:
			usage(argc, argv);
			break;
		}
	}

	if (infinite_write_flag || single_write_flag
		|| cross_write_flag || thread_write_flag || class_check_flag) {
		get_sdcard_info();
	}
	if (buffer_io_flag) {
		file_handle = &fio_ops;
	} else {
		file_handle = &io_ops;
	}
	if (infinite_write_flag) {
		infinite_write();
	}
	if (single_write_flag) {
		single_write(mega*1024*1024);
	}
	if (cross_write_flag) {
		cross_write(file_num, mega*1024*1024);
	}
	if (thread_write_flag) {
		thread_write(thread_num, mega*1024*1024);
	}
	if (class_check_flag) {
		ret = analysis(class_lvl, tolerance);
		if (ret == -1) {
			printf("FAILED.\n");
		} else {
			printf("PASSED.\n");
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	signal(SIGINT, sig_handle);
	signal(SIGQUIT, sig_handle);
	signal(SIGTERM, sig_handle);

	if (argc < 2) {
		usage(argc, argv);
		return -1;
	}
	init_buffer();
	parse_param(argc, argv);
	deinit_buffer();
	return 0;
}
