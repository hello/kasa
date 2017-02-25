/*******************************************************************************
 * datatx_lib.c
 *
 * History:
 *	2012/05/23 - [Jian Tang] created file
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
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <basetypes.h>
#include "amba_usb.h"
#include "datatx_lib.h"

//#define	DEBUG_DATATX

#define	PORT_START_ADDRESS		(2000)
#define	PORT_END_ADDRESS			(2032 - 1)
#define	FILENAME_LENGTH			(256)

static const char *default_host_ip = "10.0.0.1";
static pthread_mutex_t G_usb_mutex;

static int fd_usb = 0;
static int usb_pkt_size;
static struct amb_usb_buf *usb_buf = NULL;
static int usb_users = 0;

static void inline lock_usb(void)
{
	pthread_mutex_lock(&G_usb_mutex);
}


static void inline unlock_usb(void)
{
	pthread_mutex_unlock(&G_usb_mutex);
}

static int get_usb_command(struct amb_usb_cmd *cmd)
{
	int rval = 0;

	rval = ioctl(fd_usb, AMB_DATA_STREAM_RD_CMD, cmd);
	if (rval < 0) {
		perror("read usb command error");
		return rval;
	}
	if (cmd->signature != AMB_COMMAND_TOKEN) {
		printf("Wrong signature: %08x\n", cmd->signature);
		return -EINVAL;
	}

	return 0;
}

static int set_response(int response)
{
	struct amb_usb_rsp rsp;
	rsp.signature = AMB_STATUS_TOKEN;
	rsp.response = response;

	if (ioctl(fd_usb, AMB_DATA_STREAM_WR_RSP, &rsp) < 0) {
		perror("ioctl AMB_DATA_STREAM_WR_RSP error");
		return -1;
	}
	return 0;
}

static int init_usb(void)
{
	int rval = 0;
	struct amb_usb_cmd cmd;
	static int init_flag = 0;

	if (init_flag) {
		return 0;
	}

	pthread_mutex_init(&G_usb_mutex, NULL);
	fd_usb = open("/dev/amb_gadget", O_RDWR);
	if (fd_usb < 0) {
		perror("Cannot open device");
		return -1;
	}

	/* Read usb command */
	get_usb_command(&cmd);

	/* Process usb command */
	switch (cmd.command) {
		case USB_CMD_SET_MODE:
			usb_pkt_size = cmd.parameter[1];
			usb_buf = malloc(usb_pkt_size);
			/* response to the host */
			set_response(AMB_RSP_SUCCESS);
			break;

		default:
			printf("Unknow usb command: %08x\n", cmd.command);
			rval = -1;
			break;
	}

	init_flag = 1;
	return rval;
}

static int usb_write(int port, const void *buf, int length)
{
	int rval = 0, len1,ret_len = 0;
	int actual_len = usb_pkt_size - USB_HEAD_SIZE;

	lock_usb();
	while (length > 0) {
		len1 = length < actual_len ? length : actual_len;
		usb_buf->head.port_id = port;
		usb_buf->head.size = len1;
		usb_buf->head.flag1 = USB_PORT_OPEN;

		rval = write(fd_usb, usb_buf, USB_HEAD_SIZE);
		if (rval < 0) {
			unlock_usb();
			perror("usb_write head");
			return rval;
		}

		rval = write(fd_usb, buf + ret_len, len1);
		if (rval < 0) {
			unlock_usb();
			perror("usb_write buf");
			return rval;
		}

		ret_len += rval;
		length -= rval;
	}

	++usb_users;
	unlock_usb();
	return ret_len;
}

static int close_usb_port(int port)
{
	int rval = 0;
	if (usb_users == 0)
		return 0;

	lock_usb();
	usb_buf->head.port_id = port;
	usb_buf->head.size = 0;
	usb_buf->head.flag1 = USB_PORT_CLOSED;

	rval =  write(fd_usb, usb_buf, USB_HEAD_SIZE);
	if (rval < 0) {
		unlock_usb();
		perror("close_usb_port");
		return rval;
	}

	--usb_users;
	unlock_usb();
	return rval;
}

static int create_file_usb(const char*name, int port)
{
	char fname[FILENAME_LENGTH];

	memset(fname, 0, sizeof(fname));
	strncpy(fname, name, FILENAME_LENGTH-1);
	if (usb_write(port, fname, sizeof(fname)) < 0)
		return -1;

	return port;
}

static int create_file_nfs(const char *name)
{
	int fd;
	if ((fd = open(name, O_CREAT | O_TRUNC | O_WRONLY, 0777)) < 0) {
		perror(name);
		return -1;
	}

	return fd;
}

static int create_file_tcp(const char *name, int port)
{
	int sock;
	struct sockaddr_in sa;
	char fname[FILENAME_LENGTH];
	const char *host_ip_addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("sock");
		return -1;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	host_ip_addr = getenv("HOST_IP_ADDR");
	if (host_ip_addr == NULL)
		host_ip_addr = default_host_ip;

	sa.sin_addr.s_addr = inet_addr(host_ip_addr);

	if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("connect");
		close(sock);
		return -1;
	}
	memset(fname, 0, sizeof(fname));
	strncpy(fname, name, FILENAME_LENGTH-1);
	if (send(sock, fname, sizeof(fname), MSG_NOSIGNAL) < 0) {
		perror("send");
		return -1;
	}

	return sock;
}

int amba_transfer_init(int transfer_method)
{
	switch (transfer_method) {
	case TRANS_METHOD_NONE:
	case TRANS_METHOD_TCP:
	case TRANS_METHOD_NFS:
	case TRANS_METHOD_STDOUT:
		break;
	case TRANS_METHOD_USB:
		if (init_usb() < 0) {
			perror("usb init");
			return -1;
		}
		break;
	default:
		printf("Invalid transfer method [%d] !\n", transfer_method);
		assert(0);
		break;
	}
	return 0;
}

int amba_transfer_deinit(int transfer_method)
{
	switch (transfer_method) {
	case TRANS_METHOD_NONE:
	case TRANS_METHOD_TCP:
	case TRANS_METHOD_NFS:
	case TRANS_METHOD_STDOUT:
		break;
	case TRANS_METHOD_USB:
		if ((usb_users == 0) && (usb_buf != NULL)) {
			free(usb_buf);
			usb_buf = NULL;
			close(fd_usb);
			pthread_mutex_destroy(&G_usb_mutex);
#ifdef DEBUG_DATATX
			printf("free usb_buf\n");
#endif
		}
		break;
	default:
		printf("Invalid transfer method [%d] !\n", transfer_method);
		assert(0);
		break;
	}
	return 0;
}


int amba_transfer_open(const char *name, int transfer_method, int port)
{
	int fd;

	if (port < PORT_START_ADDRESS || port > PORT_END_ADDRESS) {
		printf("Port number [%d] must be in the range of [%d, %d].\n",
			port, PORT_START_ADDRESS, PORT_END_ADDRESS);
		return -1;
	}

	if (name == NULL) {
		printf("File name is a NULL pointer!\n");
		return -1;
	}

	switch (transfer_method) {
		case TRANS_METHOD_NONE:
			fd = 99;
			break;
		case TRANS_METHOD_TCP:
			fd = create_file_tcp(name, port);
#ifdef DEBUG_DATATX
			printf("tcp create port %d\n", port);
#endif
			break;
		case TRANS_METHOD_USB:
			fd = create_file_usb(name, port);
#ifdef DEBUG_DATATX
			printf("usb create port %d\n", port);
#endif
			break;
		case TRANS_METHOD_NFS:
			fd = create_file_nfs(name);
			break;
		case TRANS_METHOD_STDOUT:
			fd = 1;
			break;
		default:
			printf("Invalid transfer method [%d] !\n", transfer_method);
			assert(0);
			break;
	}

	return fd;
}

int amba_transfer_close(int fd, int transfer_method)
{
	switch(transfer_method) {
		case TRANS_METHOD_NONE:
		case TRANS_METHOD_TCP:
		case TRANS_METHOD_NFS:
			close(fd);
			break;
		case TRANS_METHOD_USB:
			close_usb_port(fd);
#ifdef DEBUG_DATATX
			printf("close usb port %d\n", fd);
#endif
			break;
		case TRANS_METHOD_STDOUT:
			break;
		default:
			assert(0);
			break;
	}

	return 0;
}

int amba_transfer_write(int fd, void *data, int bytes, int transfer_method)
{
	int ret = 0;

	switch(transfer_method) {
		case TRANS_METHOD_NONE:
			break;
		case TRANS_METHOD_STDOUT:
		case TRANS_METHOD_TCP:
		case TRANS_METHOD_NFS:
			if ((ret = write(fd, data, bytes)) < 0) {
				perror("write");
				return -1;
			}
			break;
		case TRANS_METHOD_USB:
			if (usb_users <= 0) {
				printf("usb file not create!\n");
				return -1;
			}
			if ((ret = usb_write(fd, data, bytes)) < 0) {
				perror("usb_write");
				return -1;
			}
			break;
		default:
			assert(0);
			break;
	}

	return ret;
}


