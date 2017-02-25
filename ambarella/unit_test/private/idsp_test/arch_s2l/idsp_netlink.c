/*
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

#include "iav_netlink.h"
#include "idsp_netlink.h"

#define MAX_ENCODE_STREAM_NUM	(IAV_STREAM_MAX_NUM_IMPL)
#define ALL_ENCODE_STREAMS		((1<<MAX_ENCODE_STREAM_NUM) - 1)

typedef struct nl_image_config_s {
	s32 fd_nl;
	s32 image_init;
	s32 nl_connected;
	struct nl_msg_data msg;
	char nl_send_buf[MAX_NL_MSG_LEN];
	char nl_recv_buf[MAX_NL_MSG_LEN];
} nl_image_config_t;

nl_image_config_t nl_image_config;

int init_netlink(void)
{
	u32 pid;
	struct sockaddr_nl saddr;

	nl_image_config.fd_nl = socket(AF_NETLINK,
		SOCK_RAW, NL_PORT_IMAGE);
	memset(&saddr, 0, sizeof(saddr));
	pid = getpid();
	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = pid;
	saddr.nl_groups = 0;
	saddr.nl_pad = 0;
	bind(nl_image_config.fd_nl, (struct sockaddr *)&saddr, sizeof(saddr));

	nl_image_config.nl_connected = 0;
	nl_image_config.image_init = 0;

	return 0;
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
		ret = recvmsg(nl_image_config.fd_nl, &msg, 0);
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

	if (check_iav_work() > 0) {
		if (do_prepare_aaa() < 0) {
			printf("do_prepare_aaa\n");
			return NULL;
		}
		if (do_start_aaa() < 0) {
			printf("do_start_aaa\n");
			return NULL;
		}
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

int send_image_msg_exit(void)
{
	if (nl_image_config.nl_connected) {
		if (check_iav_work() == 1) {
			if (process_image_cmd(NL_REQ_IMG_STOP_AAA) < 0) {
				printf("process_image_cmd error!\n");
				return -1;
			}
		}
		if (nl_send_image_session_cmd(NL_SESS_CMD_DISCONNECT) < 0) {
			printf("Failed to establish disconnect with kernel!\n");
			return -1;
		}
	}

	return 0;
}

