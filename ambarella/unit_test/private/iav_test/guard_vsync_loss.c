/*
 * guard_vsync_loss.c
 * this app will guard vsync loss issue automatically together with IAV
 *
 * History:
 *	2014/07/28 - [Zhaoyang Chen] create this file
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
#include <getopt.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <signal.h>
#include <basetypes.h>

#include "iav_ioctl.h"
#include "iav_netlink.h"

// vin
#include "../vin_test/vin_init.c"


#define MAX_ENCODE_STREAM_NUM	(IAV_STREAM_MAX_NUM_IMPL)
#define ALL_ENCODE_STREAMS		((1<<MAX_ENCODE_STREAM_NUM) - 1)

struct nl_vsync_config {
	s32 fd_nl;
	s32 nl_connected;
	struct nl_msg_data msg;
	char nl_send_buf[MAX_NL_MSG_LEN];
	char nl_recv_buf[MAX_NL_MSG_LEN];
};


int fd_iav;
static struct nl_vsync_config vsync_config;
static u32 encode_streams;

static int init_netlink()
{
	u32 pid;
	struct sockaddr_nl saddr;

	vsync_config.fd_nl = socket(AF_NETLINK, SOCK_RAW, NL_PORT_VSYNC);
	memset(&saddr, 0, sizeof(saddr));
	pid = getpid();
	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = pid;
	saddr.nl_groups = 0;
	saddr.nl_pad = 0;
	bind(vsync_config.fd_nl, (struct sockaddr *)&saddr, sizeof(saddr));

	vsync_config.nl_connected = 0;

	return 0;
}

static int send_vsync_msg_to_kernel(struct nl_msg_data vsync_msg)
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

	nlhdr = (struct nlmsghdr *)vsync_config.nl_send_buf;
	nlhdr->nlmsg_pid = getpid();
	nlhdr->nlmsg_len = NLMSG_LENGTH(sizeof(vsync_msg));
	nlhdr->nlmsg_flags = 0;
	memcpy(NLMSG_DATA(nlhdr), &vsync_msg, sizeof(vsync_msg));

	memset(&msg, 0, sizeof(struct msghdr));
	iov.iov_base = (void *)nlhdr;
	iov.iov_len = nlhdr->nlmsg_len;
	msg.msg_name = (void *)&daddr;
	msg.msg_namelen = sizeof(daddr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	sendmsg(vsync_config.fd_nl, &msg, 0);

	return 0;
}

static int recv_vsync_msg_from_kernel()
{
	struct sockaddr_nl sa;
	struct nlmsghdr *nlhdr = NULL;
	struct msghdr msg;
	struct iovec iov;

	int ret = 0;

	nlhdr = (struct nlmsghdr *)vsync_config.nl_recv_buf;
	iov.iov_base = (void *)nlhdr;
	iov.iov_len = MAX_NL_MSG_LEN;

	memset(&sa, 0, sizeof(sa));
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = (void *)&(sa);
	msg.msg_namelen = sizeof(sa);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	if (vsync_config.fd_nl > 0) {
		ret = recvmsg(vsync_config.fd_nl, &msg, 0);
	} else {
		printf("Netlink socket is not opened to receive message!\n");
		ret = -1;
	}

	return ret;
}

static int check_recv_vsync_msg()
{
	struct nlmsghdr *nlhdr = NULL;
	int msg_len;

	nlhdr = (struct nlmsghdr *)vsync_config.nl_recv_buf;
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

int stop_all_encode_streams(void)
{
	int i;
	u32 state;
	struct iav_stream_info stream_info;
	u32 stream_id = ALL_ENCODE_STREAMS;

	ioctl(fd_iav, IAV_IOC_GET_IAV_STATE, &state);
	if (state != IAV_STATE_ENCODING) {
		return 0;
	}
	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		stream_info.id = i;
		ioctl(fd_iav, IAV_IOC_GET_STREAM_INFO, &stream_info);
		if (stream_info.state != IAV_STREAM_STATE_ENCODING) {
			stream_id &= ~(1 << i);
		}
	}
	encode_streams = stream_id;
	if (stream_id == 0)
		return 0;
	ioctl(fd_iav, IAV_IOC_STOP_ENCODE, stream_id);
	printf("Stop encoding for stream 0x%x done.\n", stream_id);

	return 0;
}

static int goto_idle(void)
{
	int ret = 0;

	printf("Start to enter idle.\n");
	ret = ioctl(fd_iav, IAV_IOC_ENTER_IDLE, 0);
	if (!ret) {
		printf("Succeed to enter idle.\n");
	} else {
		printf("Failed to enter idle.\n");
	}

	return ret;
}

static int enter_preview(void)
{
	int ret = 0;

	printf("Start to enter preview.\n");
	ret = ioctl(fd_iav, IAV_IOC_ENABLE_PREVIEW, 0);
	if (!ret) {
		printf("Succeed to enter preview.\n");
	}
	else {
		printf("Failed to enter preview.\n");
	}

	return ret;
}

static int restart_encoded_streams(void)
{
	int i;
	struct iav_stream_info stream_info;

	int ret = 0;

	for (i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
		stream_info.id = i;
		ret = ioctl(fd_iav, IAV_IOC_GET_STREAM_INFO, &stream_info);
		if (ret) {
			printf("Failed to get stream info for stream %d.\n", i);
			return ret;
		}

		if (stream_info.state == IAV_STREAM_STATE_ENCODING) {
			encode_streams &= ~(1 << i);
		}
	}
	if (encode_streams == 0) {
		printf("No encode streams to recover.\n");
		return 0;
	}
	printf("Start recovering encoding for stream 0x%x.\n", encode_streams);
	// start encode
	ret = ioctl(fd_iav, IAV_IOC_START_ENCODE, encode_streams);
	if (!ret) {
		printf("Succeed to recover encoding for stream 0x%x.\n", encode_streams);
	} else {
		printf("Failed to recover encoding for stream 0x%x.\n", encode_streams);
	}

	return ret;
}

static int reset_vin(void)
{
	struct vindev_mode video_info;
	struct vindev_fps vsrc_fps;
	// select channel: for multi channel VIN (initialize)
	if (channel >= 0) {
		if (select_channel() < 0)
			return -1;
	}

	memset(&video_info, 0, sizeof(video_info));
	video_info.vsrc_id = 0;
	if(ioctl(fd_iav, IAV_IOC_VIN_GET_MODE, &video_info) < 0) {
		return -1;
	} else {
		printf("Start to restore vin_mode 0x%x and hdr_mode %d.\n",
			video_info.video_mode, video_info.hdr_mode);
	}

	vsrc_fps.vsrc_id = 0;
	if(ioctl(fd_iav, IAV_IOC_VIN_GET_FPS, &vsrc_fps) < 0) {
		return -1;
	} else {
		printf("Start to restore vin frame rate %d.\n", vsrc_fps.fps);
	}

	if(ioctl(fd_iav, IAV_IOC_VIN_SET_MODE, &video_info) < 0) {
		return -1;
	} else {
		printf("Succeed to restore vin_mode 0x%x and hdr_mode %d.\n",
			video_info.video_mode, video_info.hdr_mode);
	}

	if (ioctl(fd_iav, IAV_IOC_VIN_SET_FPS, &vsrc_fps) < 0) {
		perror("IAV_IOC_VIN_SET_FPS");
		return -1;
	} else {
		printf("Succeed to restore vin frame rate %d.\n", vsrc_fps.fps);
	}

	return 0;
}


static int recover_vsync_loss()
{
	int ret = 0;

	ret = stop_all_encode_streams();
	if (ret) {
		return ret;
	}
	ret = goto_idle();
	if (ret) {
		return ret;
	}
	ret = reset_vin();
	if (ret) {
		return ret;
	}
	ret = enter_preview();
	if (ret) {
		return ret;
	}
	ret = restart_encoded_streams();

	return ret;
}

static int process_vsync_req(int vsync_req)
{
	int ret = 0;

	if (vsync_req == NL_REQ_VSYNC_RESTORE) {
		ret = recover_vsync_loss();
		vsync_config.msg.pid = getpid();
		vsync_config.msg.port = NL_PORT_VSYNC;
		vsync_config.msg.type = NL_MSG_TYPE_REQUEST;
		vsync_config.msg.dir = NL_MSG_DIR_STATUS;
		vsync_config.msg.cmd = NL_REQ_VSYNC_RESTORE;
		if (ret < 0) {
			vsync_config.msg.status = NL_CMD_STATUS_FAIL;
			send_vsync_msg_to_kernel(vsync_config.msg);
		} else {
			vsync_config.msg.status = NL_CMD_STATUS_SUCCESS;
			send_vsync_msg_to_kernel(vsync_config.msg);
		}
	} else {
		printf("Unrecognized kernel message!\n");
		ret = -1;
	}

	return ret;
}

static int process_vsync_session_status(struct nl_msg_data *kernel_msg)
{
	int ret = 0;

	if (kernel_msg->type != NL_MSG_TYPE_SESSION ||
		kernel_msg->dir != NL_MSG_DIR_STATUS) {
		return -1;
	}

	switch (kernel_msg->cmd) {
	case NL_SESS_CMD_CONNECT:
		if (kernel_msg->status == NL_CMD_STATUS_SUCCESS) {
			vsync_config.nl_connected = 1;
			printf("Connection established with kernel.\n");
		} else {
			vsync_config.nl_connected = 0;
			printf("Failed to establish connection with kernel!\n");
		}
		break;
	case NL_SESS_CMD_DISCONNECT:
		vsync_config.nl_connected = 0;
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

static int process_vsync_msg()
{
	struct nlmsghdr *nlhdr = NULL;
	struct nl_msg_data *kernel_msg;
	int ret = 0;

	if (check_recv_vsync_msg() < 0) {
		return -1;
	}

	nlhdr = (struct nlmsghdr *)vsync_config.nl_recv_buf;
	kernel_msg = (struct nl_msg_data *)NLMSG_DATA(nlhdr);

	if(kernel_msg->type == NL_MSG_TYPE_REQUEST &&
		kernel_msg->dir == NL_MSG_DIR_CMD) {
		if (process_vsync_req(kernel_msg->cmd) < 0) {
			ret = -1;
		}
	} else if (kernel_msg->type == NL_MSG_TYPE_SESSION &&
				kernel_msg->dir == NL_MSG_DIR_STATUS) {
		if (process_vsync_session_status(kernel_msg) < 0) {
			ret = -1;
		}
	} else {
		printf("Incorrect message from kernel!\n");
		ret = -1;
	}

	return ret;
}

static int nl_send_vsync_session_cmd(int cmd)
{
	int ret = 0;

	vsync_config.msg.pid = getpid();
	vsync_config.msg.port = NL_PORT_VSYNC;
	vsync_config.msg.type = NL_MSG_TYPE_SESSION;
	vsync_config.msg.dir = NL_MSG_DIR_CMD;
	vsync_config.msg.cmd = cmd;
	vsync_config.msg.status = 0;
	send_vsync_msg_to_kernel(vsync_config.msg);

	ret = recv_vsync_msg_from_kernel();

	if (ret > 0) {
		ret = process_vsync_msg();
		if (ret < 0) {
			printf("Failed to process session status!\n");
		}
	} else {
		printf("Error for getting session status!\n");
	}

	return ret;
}

static void * netlink_loop(void * data)
{
	int ret;
	int count = 100;

	while (count && !vsync_config.nl_connected) {
		if (nl_send_vsync_session_cmd(NL_SESS_CMD_CONNECT) < 0) {
			printf("Failed to establish connection with kernel!\n");
		}
		sleep(1);
		count--;
	}

	if (!vsync_config.nl_connected) {
		printf("Please enable kernel vsync loss guard mechanism!!!\n");
		return NULL;
	}

	while (vsync_config.nl_connected) {
		ret = recv_vsync_msg_from_kernel();
		if (ret > 0) {
			ret = process_vsync_msg();
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

static void sigstop()
{
	nl_send_vsync_session_cmd(NL_SESS_CMD_DISCONNECT);
	exit(1);
}

int main(int argc, char **argv)
{
	//register signal handler for Ctrl+C, Ctrl+'\', and "kill" sys cmd
	signal(SIGINT, 	sigstop);
	signal(SIGQUIT,	sigstop);
	signal(SIGTERM,	sigstop);

	// open the device
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	init_netlink();
	netlink_loop(NULL);

	return 0;
}

