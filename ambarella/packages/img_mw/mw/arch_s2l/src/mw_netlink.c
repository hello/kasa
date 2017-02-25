
/*******************************************************************************
 * mw_netlink.c
 *
 * Histroy:
 *  2014/09/02 2014 - [jingyang qiu] created file
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>

#include <getopt.h>
#include <sched.h>
#include <basetypes.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/netlink.h>

#include <pthread.h>
#include <semaphore.h>

#include "iav_common.h"
#include "iav_ioctl.h"
#include "iav_vin_ioctl.h"
#include "iav_vout_ioctl.h"
#include "ambas_imgproc_arch.h"

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "mw_aaa_params.h"
#include "mw_api.h"
#include "mw_pri_struct.h"
#include "mw_image_priv.h"
#include "iav_netlink.h"


typedef struct nl_image_config_s {
	s32 fd_nl;
	s32 image_init;
	s32 nl_connected;
	struct nl_msg_data msg;
	char nl_send_buf[MAX_NL_MSG_LEN];
	char nl_recv_buf[MAX_NL_MSG_LEN];
	pthread_t nl_thread;
} nl_image_config_t;

static nl_image_config_t nl_image_config;

static sem_t g_sem;
int init_sem()
{
	int res = 0;
	if (0 != sem_init(&g_sem, 0, 0)) {
		perror("sem_init");
		res = -1;
	}
	return res;
}

int deinit_sem()
{
	int res = 0;
	if (0 != sem_destroy(&g_sem)) {
		perror("sem_close");
		res = -1;
	}
	return res;
}

int wait_sem(int ms)
{
	int res = 0;
	if (ms == 0) {
		if (0 != sem_wait(&g_sem)) {
			MW_ERROR("sem_wait error: %d\n", errno);
			perror("sem_wait");
			res = -1;
		}
	} else {
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += ms / 1000;
		ts.tv_nsec += (ms % 1000) * 1000000;
		if (0 != sem_timedwait(&g_sem, &ts)) {
			MW_ERROR("sem_timedwait error: %d\n", errno);
			perror("sem_timedwait");
			res = -1;
		}
	}
	return res;
}

int signal_sem()
{
	int res = 0;
	if (0 != sem_post(&g_sem)) {
		perror("sem_post");
		res = -1;
	}
	return res;
}

static int send_image_msg_to_kernel(struct nl_msg_data nl_msg)
{
	struct sockaddr_nl daddr;
	struct msghdr msg;
	struct nlmsghdr *nlhdr = NULL;
	struct iovec iov;

	memset(&daddr, 0, sizeof(daddr));
	daddr.nl_family = AF_NETLINK;
	daddr.nl_pid = 0;
	daddr.nl_groups = 0;
	daddr.nl_pad = 0;

	nlhdr = (struct nlmsghdr *)nl_image_config.nl_send_buf;
	nlhdr->nlmsg_pid = getpid();
	nlhdr->nlmsg_len = NLMSG_LENGTH(sizeof(nl_msg));
	nlhdr->nlmsg_flags = 0;
	memcpy(NLMSG_DATA(nlhdr), &nl_msg, sizeof(nl_msg));

	memset(&msg, 0, sizeof(struct msghdr));
	iov.iov_base = (void *)nlhdr;
	iov.iov_len = nlhdr->nlmsg_len;
	msg.msg_name = (void *)&daddr;
	msg.msg_namelen = sizeof(daddr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sendmsg(nl_image_config.fd_nl, &msg, 0);

	return 0;
}


static ssize_t recvmsg_reliable(int fd, struct msghdr *msg, int flags)
{
	ssize_t ret = 0;
	int run = 1;
	int retry = 3;
	while (run) {
		ret = recvmsg(fd, msg, flags);
		if (ret > 0) {
			break;
		} else if (ret == 0) {
			printf("Netlink connect is shutdown!\n");
			break;
		} else {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				printf("system interrupt occur, try %d times again\n", retry);
				--retry;
				if (retry == 0) {
					break;
				}
				continue;
			} else {
				perror("recvmsg");
				break;
			}
		}
	}
	return ret;
}

static int recv_image_msg_from_kernel()
{
	struct sockaddr_nl sa;
	struct nlmsghdr *nlhdr = NULL;
	struct msghdr msg;
	struct iovec iov;

	int ret = 0;

	nlhdr = (struct nlmsghdr *)nl_image_config.nl_recv_buf;
	iov.iov_base = (void *)nlhdr;
	iov.iov_len = MAX_NL_MSG_LEN;

	memset(&sa, 0, sizeof(sa));
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&(sa);
	msg.msg_namelen = sizeof(sa);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if (nl_image_config.fd_nl > 0) {
		ret = recvmsg_reliable(nl_image_config.fd_nl, &msg, 0);
	} else {
		printf("Netlink socket is not opened for receive message!\n");
		ret = -1;
	}

	return ret;
}

static int process_image_cmd(int image_cmd)
{
	int ret = 0;
	char err_msg[32];

	switch (image_cmd) {
		case NL_REQ_IMG_START_AAA:
			ret = do_start_aaa();
			sprintf(err_msg,  "%s AAA", "Start");
			break;
		case NL_REQ_IMG_STOP_AAA:
			ret = do_stop_aaa();
			sprintf(err_msg,  "%s AAA", "Stop");
			break;
		case NL_REQ_IMG_PREPARE_AAA:
			ret = do_prepare_aaa();
			sprintf(err_msg,  "%s AAA", "Prepare");
			break;
		default:
			printf("Unrecognized kernel message!\n");
			ret = -1;
			return ret;
	}

	if (ret < 0) {
		printf("%s failed \n", err_msg);
		nl_image_config.msg.status = NL_CMD_STATUS_FAIL;
	} else {
		nl_image_config.msg.status = NL_CMD_STATUS_SUCCESS;
	}

	nl_image_config.msg.pid = getpid();
	nl_image_config.msg.port = NL_PORT_IMAGE;
	nl_image_config.msg.type = NL_MSG_TYPE_REQUEST;
	nl_image_config.msg.dir = NL_MSG_DIR_STATUS;
	nl_image_config.msg.cmd = image_cmd;

	send_image_msg_to_kernel(nl_image_config.msg);

	return ret;
}

static int process_image_session_status(struct nl_msg_data *kernel_msg)
{
	int ret = 0;

	if (kernel_msg->type != NL_MSG_TYPE_SESSION ||
		kernel_msg->dir != NL_MSG_DIR_STATUS) {
		return -1;
	}

	switch (kernel_msg->cmd) {
	case NL_SESS_CMD_CONNECT:
		if (kernel_msg->status == NL_CMD_STATUS_SUCCESS) {
			nl_image_config.nl_connected = 1;
			printf("Connection established with kernel.\n");
		} else {
			nl_image_config.nl_connected = 0;
			printf("Failed to establish connection with kernel!\n");
		}
		break;
	case NL_SESS_CMD_DISCONNECT:
		nl_image_config.nl_connected = 0;
		if (kernel_msg->status == NL_CMD_STATUS_SUCCESS) {
			printf("Connection removed with kernel.\n");
		} else {
			printf("Failed to remove connection with kernel!\n");
		}
		break;
	default:
		printf("Unrecognized session cmd from kernel!\n");
		ret = -1;
		break;
	}

	return ret;
}

static int check_recv_image_msg()
{
	struct nlmsghdr *nlhdr = NULL;
	int msg_len;

	nlhdr = (struct nlmsghdr *)nl_image_config.nl_recv_buf;
	if (nlhdr->nlmsg_len <  sizeof(struct nlmsghdr)) {
		printf("Corruptted kernel message!\n");
		return -1;
	}
	msg_len = nlhdr->nlmsg_len - NLMSG_LENGTH(0);
	if (msg_len < sizeof(struct nl_msg_data)) {
		printf("Unknown kernel message!!\n");
		return -1;
	}

	return 0;
}

static int process_image_msg()
{
	struct nlmsghdr *nlhdr = NULL;
	struct nl_msg_data *kernel_msg;
	int ret = 0;

	if (check_recv_image_msg() < 0) {
		return -1;
	}

	nlhdr = (struct nlmsghdr *)nl_image_config.nl_recv_buf;
	kernel_msg = (struct nl_msg_data *)NLMSG_DATA(nlhdr);

	if ((kernel_msg->type == NL_MSG_TYPE_REQUEST) &&
		(kernel_msg->dir == NL_MSG_DIR_CMD)) {
		if (process_image_cmd(kernel_msg->cmd) < 0) {
			ret = -1;
		}
	} else if ((kernel_msg->type == NL_MSG_TYPE_SESSION) &&
		(kernel_msg->dir == NL_MSG_DIR_STATUS)) {
		if (process_image_session_status(kernel_msg) < 0) {
			ret = -1;
		}
	} else {
		printf("Incorrect message from kernel!\n");
		ret = -1;
	}

	return ret;
}

static int nl_send_image_session_cmd(int cmd)
{
	int ret = 0;

	nl_image_config.msg.pid = getpid();
	nl_image_config.msg.port = NL_PORT_IMAGE;
	nl_image_config.msg.type = NL_MSG_TYPE_SESSION;
	nl_image_config.msg.dir = NL_MSG_DIR_CMD;
	nl_image_config.msg.cmd = cmd;
	nl_image_config.msg.status = 0;

	send_image_msg_to_kernel(nl_image_config.msg);

	if (cmd == NL_SESS_CMD_CONNECT) {
		ret = recv_image_msg_from_kernel();

		if (ret > 0) {
			ret = process_image_msg();
			if (ret < 0) {
				printf("Failed to process session status!\n");
			}
		} else {
			printf("Error for getting session status!\n");
		}
	}

	return ret;
}

void * netlink_loop(void * data)
{
	int ret;

	if (nl_send_image_session_cmd(NL_SESS_CMD_CONNECT) < 0) {
		printf("Failed to establish connection with kernel!\n");
	}

	if (!nl_image_config.nl_connected) {
		return NULL;
	}
	ret = check_iav_work();
	if (ret > 0) {
		if (do_prepare_aaa() < 0) {
			MW_ERROR("do_prepare_aaa\n");
			return NULL;
		}
		wait_irq_count(1);
		if (do_start_aaa() < 0) {
			MW_ERROR("do_start_aaa\n");
			return NULL;
		}
	} else if (ret < 0) {
		MW_ERROR("IAV abnormal, check_iav_work failed\n");
		return NULL;
	}

	while (nl_image_config.nl_connected) {
		ret = recv_image_msg_from_kernel();
		if (ret > 0) {
			ret = process_image_msg();
			if (ret < 0) {
				printf("Failed to process the msg from kernel!\n");
			}
		}
		else {
			printf("Error for getting msg from kernel!\n");
		}
	}

	return NULL;
}

int init_netlink(void)
{
	u32 pid;
	struct sockaddr_nl saddr;

	nl_image_config.fd_nl = socket(AF_NETLINK, SOCK_RAW, NL_PORT_IMAGE);
	memset(&saddr, 0, sizeof(saddr));
	pid = getpid();
	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = pid;
	saddr.nl_groups = 0;
	saddr.nl_pad = 0;
	bind(nl_image_config.fd_nl, (struct sockaddr *)&saddr, sizeof(saddr));

	nl_image_config.nl_connected = 0;
	nl_image_config.image_init = 0;
	nl_image_config.nl_thread = -1;

	if (pthread_create(&nl_image_config.nl_thread, NULL, (void *)netlink_loop,
		(void *)NULL) != 0) {
		MW_ERROR("Failed. Can't create thread <%s> !\n",
			MW_THREAD_NAME_NETLINK);
		return -1;
	}
	pthread_setname_np(nl_image_config.nl_thread, MW_THREAD_NAME_NETLINK);
	if (nl_image_config.nl_thread < 0) {
		return -1;
	}
	init_sem();
	return 0;
}

int deinit_netlink(void)
{
	if(nl_image_config.fd_nl > 0) {
		close(nl_image_config.fd_nl);
		nl_image_config.fd_nl = -1;
	}
	deinit_sem();

	return 0;
}

int send_image_msg_stop_aaa(void)
{
	if (nl_image_config.nl_connected) {
		process_image_cmd(NL_REQ_IMG_STOP_AAA);
		nl_send_image_session_cmd(NL_SESS_CMD_DISCONNECT);
		pthread_join(nl_image_config.nl_thread, NULL);
		nl_image_config.nl_thread = -1;
	} else {
		if(do_stop_aaa() < 0) {
			MW_ERROR("stop_aaa_task\n");
			return -1;
		}
	}

	return 0;
}

