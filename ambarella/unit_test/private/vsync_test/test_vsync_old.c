/*
 * test_soft_vsync.c
 *
 * History:
 *	2010/01/08 - [Qiao Wang] create
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
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>

#include <signal.h>

#ifdef __UCLIBC__
#include <bits/getopt.h>
#else
#include <getopt.h>
#include <sched.h>
#endif

#include <pthread.h>
#include <semaphore.h>

/* ========================================================================== */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

#define NO_ARG 0
#define HAS_ARG 1
/* ========================================================================== */
static char irq_proc_name[256] = "/proc/ambarella/vin0_idsp";
static char irq_proc_name2[256];
static int gpio_id = -1;
static int diff_mode = 0;
static char sync_wo_name[256] = "/sys/class/backlight/pwm-backlight.0/brightness";
static int sync_wo_id = -1;

static struct option long_options[] = {
	{"proc",	HAS_ARG,	0,	'p'},
	{"gpio",	HAS_ARG,	0,	'g'},
	{"proc2",	HAS_ARG,	0,	'P'},
	{"pwm",		HAS_ARG,	0,	'b'},

	{0,		0,		0,	0},
};
struct hint_s {
	const char *arg;
	const char *str;
};
static const struct hint_s hint[] = {
	{"file name",		"\tIrq proc file name"},
	{"0 ~ max",		"\tgpio id"},
	{"file name",		"\tSencond name to show the diff"},
	{"file name",		"\tWrite into the file on evey sync"},
};
static const char *short_options = "p:g:P:b:";

void usage(int argc, char *argv[])
{
	int i;

	printf("%s usage:\n", argv[0]);
	for (i = 0; i < (ARRAY_SIZE(long_options) - 1); i++) {
		if (isalpha(long_options[i].val))
			printf("-%c ", long_options[i].val);
		else
			printf("   ");
		printf("--%s", long_options[i].name);
		if (hint[i].arg[0] != 0)
			printf(" [%s]", hint[i].arg);
		printf("\t%s\n", hint[i].str);
	}
	printf("\n");
}

int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options,
		long_options, &option_index)) != -1) {
		switch (ch) {
		case 'p':
			strncpy(irq_proc_name, optarg, sizeof(irq_proc_name));
			break;

		case 'g':
			gpio_id = atoi(optarg);
			break;

		case 'P':
			diff_mode = 1;
			strncpy(irq_proc_name2, optarg, sizeof(irq_proc_name2));
			break;

		case 'b':
			strncpy(sync_wo_name, optarg, sizeof(irq_proc_name));
			sync_wo_id = 1;
			break;

		default:
			printf("unknown option found: %c\n", ch);
			return -1;
		}
	}

	return 0;
}

/* ========================================================================== */
static inline int gpio_open(u8 gpio_id, char *buf)
{
	int _export, direction;

	_export = open("/sys/class/gpio/export", O_WRONLY);
	if (_export < 0) {
		printf("%s: Can't open export sys file!\n", __func__);
		direction = -1;
		goto gpio_open_exit;
	}
	sprintf(buf, "%d", gpio_id);
	write(_export, buf, strlen(buf));
	close(_export);

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
	direction = open(buf, O_WRONLY);
	if (direction < 0) {
		close(_export);
		printf("%s: Can't open direction sys file!\n", __func__);
		goto gpio_open_exit;
	}
	sprintf(buf, "low");
	write(direction, buf, strlen(buf));

gpio_open_exit:
	return direction;
}

static inline void gpio_close(u8 gpio_id, int direction, char *buf)
{
	int unexport;

	close(direction);

	unexport = open("/sys/class/gpio/unexport", O_WRONLY);
	if (unexport < 0) {
		printf("%s: Can't open unexport sys file!\n", __func__);
		goto gpio_close_exit;
	}
	sprintf(buf, "%d", gpio_id);
	write(unexport, buf, strlen(buf));
	close(unexport);

gpio_close_exit:
	return;
}

static inline void gpio_set(int direction, char *buf)
{
	sprintf(buf, "high");
	write(direction, buf, strlen(buf));
}

static inline void gpio_clr(int direction, char *buf)
{
	sprintf(buf, "low");
	write(direction, buf, strlen(buf));
}

/* ========================================================================== */
static int irq_fd = -1;
static int irq_fd2 = -1;
static int gpio_fd = -1;
static int pwm_fd = -1;
static char gpio_buf[256];
static char pwm_buf[256];
struct timeval base_time;
struct timeval diff_time;
static sem_t *base_sem = NULL;
static sem_t *diff_sem = NULL;
static u32 irq_counter = 0;
static u32 irq_counter2 = 0;
pthread_mutex_t diff_start = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t base_sync = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t diff_sync = PTHREAD_MUTEX_INITIALIZER;

static inline void soft_vsync(int irq_fd)
{
	char vin_int_array[32];

	vin_int_array[8] = 0;
	read(irq_fd, vin_int_array, 8);
	lseek(irq_fd, 0 , 0);

	sscanf(vin_int_array, "%x", &irq_counter);
}

static void *soft_irq_thread_base()
{
	char vin_int_array[32];

	pthread_mutex_lock(&diff_start);
	pthread_mutex_unlock(&diff_start);
	while (1) {
		read(irq_fd, vin_int_array, 8);
		lseek(irq_fd, 0 , 0);
		sscanf(vin_int_array, "%x", &irq_counter);
		gettimeofday(&base_time, NULL);
		sem_post(base_sem);
		if (gpio_fd >= 0)
			gpio_set(gpio_fd, gpio_buf);
		pthread_mutex_lock(&base_sync);
		pthread_mutex_unlock(&base_sync);
	}

	return NULL;
}

static void *soft_irq_thread_diff()
{
	char vin_int_array[32];

	pthread_mutex_lock(&diff_start);
	pthread_mutex_unlock(&diff_start);
	while (1) {
		read(irq_fd2, vin_int_array, 8);
		lseek(irq_fd2, 0 , 0);
		sscanf(vin_int_array, "%x", &irq_counter2);
		gettimeofday(&diff_time, NULL);
		sem_post(diff_sem);
		if (gpio_fd >= 0)
			gpio_clr(gpio_fd, gpio_buf);
		pthread_mutex_lock(&diff_sync);
		pthread_mutex_unlock(&diff_sync);
	}

	return NULL;
}

static void sigstop(int reg)
{
	if (irq_fd >= 0)
		close(irq_fd);
	if (irq_fd2 >= 0)
		close(irq_fd2);
	if (gpio_fd >= 0)
		gpio_close(gpio_id, gpio_fd, gpio_buf);
	if (pwm_fd >= 0)
		close(pwm_fd);
	if (base_sem)
		sem_close(base_sem);
	if (diff_sem)
		sem_close(diff_sem);

	exit(1);
}

int main(int argc, char *argv[])
{
	int usec = 0, sec;
	pthread_t base_thread;
	pthread_t diff_thread;
	int base_thread_rc;
	int diff_thread_rc;
	int pwm_n;

	if (init_param(argc, argv) < 0) {
		usage(argc, argv);
		return -1;
	}

	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	irq_fd = open(irq_proc_name, O_RDONLY);
	if (irq_fd < 0) {
		printf("\nCan't open %s.\n", irq_proc_name);
		usage(argc, argv);
		return -1;
	}
	printf("\nOpen %s as sync source.\n", irq_proc_name);

	if (gpio_id != -1) {
		gpio_fd = gpio_open(gpio_id, gpio_buf);
	}

	if (sync_wo_id == 1) {
		if ((pwm_fd = open(sync_wo_name,
			O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
			printf("open %s failed!\n", sync_wo_name);
		}
		sprintf(pwm_buf, "10");
	}

	if (diff_mode == 0) {
		gettimeofday(&base_time, NULL);
		while (1) {
			if (gpio_fd >= 0) {
				gpio_set(gpio_fd, gpio_buf);
			}
			soft_vsync(irq_fd);
			if (gpio_fd >= 0) {
				gpio_clr(gpio_fd, gpio_buf);
			}
			gettimeofday(&diff_time, NULL);
			if ((sync_wo_id == 1) && (pwm_fd > 0)) {
				pwm_n = write(pwm_fd, pwm_buf, strlen(pwm_buf));
				if (pwm_n != strlen(pwm_buf)) {
					printf("pwm [%s] set error [%d].\n",
						pwm_buf, pwm_n);
				}
			}
			sec = (s32)diff_time.tv_sec - (s32)base_time.tv_sec;
			usec = (s32)diff_time.tv_usec - (s32)base_time.tv_usec;
			printf("IRQ[%08d]:[%08dus]\n",
				irq_counter, usec + sec * 1000000);
			memcpy(&base_time, &diff_time, sizeof(struct timeval));
		}
	} else {
		irq_fd2 = open(irq_proc_name2, O_RDONLY);
		if (irq_fd2 < 0) {
			printf("\nCan't open %s.\n", irq_proc_name2);
			usage(argc, argv);
			return -1;
		}
		printf("\nOpen %s as diff.\n", irq_proc_name2);

		base_sem = sem_open("Base", O_CREAT);
		diff_sem = sem_open("Diff", O_CREAT);
		if ((base_sem == SEM_FAILED) ||
			(diff_sem == SEM_FAILED)) {
			printf("\nopen sem fail.\n");
			usage(argc, argv);
			return -1;
		}

		pthread_mutex_lock(&diff_start);
		if ((base_thread_rc = pthread_create(&base_thread, NULL,
			&soft_irq_thread_base, NULL))) {
			printf("Base thread failed: %d\n", base_thread_rc);
		}

		if ((diff_thread_rc = pthread_create(&diff_thread, NULL,
			&soft_irq_thread_diff, NULL))) {
			printf("Diff thread failed: %d\n", diff_thread_rc);
		}

		pthread_mutex_unlock(&diff_start);
		while (1) {
			if (usec < 0) {
				pthread_mutex_lock(&base_sync);
				sem_wait(diff_sem);
				sec = (s32)diff_time.tv_sec - (s32)base_time.tv_sec;
				usec = (s32)diff_time.tv_usec - (s32)base_time.tv_usec;
				while (1) {
					if (sem_trywait(base_sem) < 0)
						break;
				}
				while (1) {
					if (sem_trywait(diff_sem) < 0)
						break;
				}
				pthread_mutex_unlock(&base_sync);
			} else {
				sem_wait(base_sem);
				sem_wait(diff_sem);
				sec = (s32)diff_time.tv_sec - (s32)base_time.tv_sec;
				usec = (s32)diff_time.tv_usec - (s32)base_time.tv_usec;
			}
			printf("IRQ[%08d:%08d]:[%08dus]\n", irq_counter,
				irq_counter2, usec + sec * 1000000);
		}
	}

	if (irq_fd >= 0)
		close(irq_fd);
	if (irq_fd2 >= 0)
		close(irq_fd2);
	if (gpio_fd >= 0)
		gpio_close(gpio_id, gpio_fd, gpio_buf);
	if (pwm_fd >= 0)
		close(pwm_fd);
	if (base_sem)
		sem_close(base_sem);
	if (diff_sem)
		sem_close(diff_sem);

	return 0;
}

