/*
 * iav_netlink.c
 *
 * History:
 *	2014/07/25 - [Zhaoyang Chen] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#include <config.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <iav_utils.h>
#include <iav_ioctl.h>
#include <dsp_api.h>
#include <dsp_format.h>
#include <vin_api.h>
#include <vout_api.h>
#include "iav.h"
#include "iav_dsp_cmd.h"
#include "iav_vin.h"
#include "iav_vout.h"
#include "iav_enc_api.h"
#include "iav_enc_utils.h"

static struct ambarella_iav *g_iav = NULL;

/* Need to add "mutex_lock / mutex_unlock" in the upper layer before
 * calling this function and its upper function. */
static int nl_send_msg(struct iav_nl_obj *nl_obj, struct nl_msg_data *msg)
{
#define RETRY_TIMES			(10)
#define TIMEOUT_JIFFY		msecs_to_jiffies(1000)
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlhdr = NULL;
	struct ambarella_iav *iav = nl_obj->iav;
	struct iav_nl_request *nl_req = NULL;
	void *data;
	int retry = RETRY_TIMES;
	int rval = 0;

	if (msg->type == NL_MSG_TYPE_SESSION) {
		if (msg->dir != NL_MSG_DIR_STATUS) {
			iav_error("NETLINK ERR: IAV can only send session status to "
				"app %d!\n", nl_obj->nl_user_pid);
			return -1;
		}
	} else if (msg->type == NL_MSG_TYPE_REQUEST) {
		if (msg->dir != NL_MSG_DIR_CMD) {
			iav_error("NETLINK ERR: IAV can only send request cmd to "
				"app %d!\n", nl_obj->nl_user_pid);
			return -1;
		}
	} else {
		iav_error("NETLINK ERR: Unrecognized IAV msg type to "
			"app %d!\n", nl_obj->nl_user_pid);
		return -1;
	}

	skb = nlmsg_new(sizeof(*msg), GFP_KERNEL);
	if (!skb) {
		iav_error("NETLINK ERR: Function nlmsg_new failed!\n");
		return -1;
	}
	nlhdr = __nlmsg_put(skb, nl_obj->nl_user_pid, 0, NLMSG_NOOP,
		sizeof(*msg), 0);

	data = NLMSG_DATA(nlhdr);
	memcpy(data, (void *)msg, sizeof(*msg));

	netlink_unicast(nl_obj->nl_sock, skb, nl_obj->nl_user_pid, 0);

	if (msg->type == NL_MSG_TYPE_REQUEST) {
		nl_req = &nl_obj->nl_requests[msg->cmd];

		mutex_unlock(&iav->iav_mutex);
		iav_debug("NETLINK DBG: Send request cmd %u to app %d.\n",
			msg->cmd, nl_obj->nl_user_pid);
		// wait for the response of the command
		while (retry > 0) {
			rval = wait_event_interruptible_timeout(nl_req->wq_request,
				nl_req->responded, TIMEOUT_JIFFY) ;
			if (rval > 0) {
				break;
			}
			iav_debug("NETLINK DBG: receive ACK of request cmd %u from app %d "
				"error, retry:%d\n", msg->cmd, nl_obj->nl_user_pid, retry);
			--retry;
		}
		mutex_lock(&iav->iav_mutex);

		if (retry <= 0) {
			iav_error("NETLINK ERR: send request cmd %u to app %d timeout\n",
				msg->cmd, nl_obj->nl_user_pid);
			return -1;
		}
	} else if (msg->type == NL_MSG_TYPE_SESSION) {
		iav_debug("NETLINK DBG: Send session status %u to session cmd %u of "
			"app %d.\n", msg->status, msg->cmd, nl_obj->nl_user_pid);
	}

	return 0;
}

int nl_send_request(struct iav_nl_obj *nl_obj, int cmd)
{
	int ret = 0;
	struct nl_msg_data msg;

	msg.port = nl_obj->nl_port;
	msg.pid = 0;
	msg.dir = NL_MSG_DIR_CMD;
	msg.type = NL_MSG_TYPE_REQUEST;
	msg.cmd = cmd;
	msg.status = 0;

	nl_obj->nl_requests[cmd].responded = 0;
	ret = nl_send_msg(nl_obj, &msg);

	return ret;
}

int is_nl_request_responded(struct iav_nl_obj *nl_obj, int cmd)
{
	return nl_obj->nl_requests[cmd].responded;
}

static int nl_process_session_cmd(struct iav_nl_obj *nl_obj,
	struct nl_msg_data *msg)
{
	int ret = 0;
	struct nl_msg_data nl_resp_msg;

	switch (msg->cmd) {
	case NL_SESS_CMD_CONNECT:
		nl_obj->nl_connected = 1;
		nl_obj->nl_user_pid = msg->pid;
		iav_debug("NETLINK DBG: Correct connect cmd from app %u.\n",
			msg->pid);
		nl_resp_msg.pid = 0;
		nl_resp_msg.port = nl_obj->nl_port;
		nl_resp_msg.type = NL_MSG_TYPE_SESSION;
		nl_resp_msg.dir = NL_MSG_DIR_STATUS;
		nl_resp_msg.cmd = msg->cmd;
		nl_resp_msg.status = NL_CMD_STATUS_SUCCESS;
		nl_send_msg(nl_obj, &nl_resp_msg);
		break;
	case NL_SESS_CMD_DISCONNECT:
		nl_obj->nl_connected = 0;
		iav_debug("NETLINK DBG: Correct disconnect cmd from app %u.\n",
			msg->pid);
		nl_resp_msg.pid = 0;
		nl_resp_msg.port = nl_obj->nl_port;
		nl_resp_msg.type = NL_MSG_TYPE_SESSION;
		nl_resp_msg.dir = NL_MSG_DIR_STATUS;
		nl_resp_msg.cmd = msg->cmd;
		nl_resp_msg.status = NL_CMD_STATUS_SUCCESS;
		nl_send_msg(nl_obj, &nl_resp_msg);
		nl_obj->nl_user_pid = -1;
		break;
	default:
		iav_debug("Unknown session cmd from app %u!\n", msg->pid);
		ret = -1;
		break;
	}

	return ret;
}

static struct iav_nl_obj *find_nl_obj(int port)
{
	struct iav_nl_obj *nl_obj;
	int i;

	for(i = 0; i < NL_OBJ_MAX_NUM; ++i) {
		nl_obj = &g_iav->nl_obj[i];
		if (nl_obj->nl_init && nl_obj->nl_port == port) {
			return nl_obj;
		}
	}

	return NULL;
}

static int nl_process_request_status(struct iav_nl_obj *nl_obj,
	struct nl_msg_data *msg)
{
	struct iav_nl_request *nl_req;
	int ret = 0;

	if (msg->status == NL_CMD_STATUS_SUCCESS) {
		iav_debug("NETLINK DBG: Request %u succeeded from app %u.\n",
			msg->cmd, msg->pid);
	} else if (msg->status == NL_CMD_STATUS_FAIL) {
		iav_debug("NETLINK DBG: Request %u failed from app %u!\n",
			msg->cmd, msg->pid);
	} else {
		iav_debug("NETLINK DBG: Incorrect status %u from app %u!\n",
			msg->status, msg->pid);
		ret = -1;
	}

	if (msg->cmd > nl_obj->nl_request_count) {
		iav_debug("NETLINK DBG: Incorrect request cmd %u from app %u!\n",
			msg->cmd, msg->pid);
		ret = -1;
	}

	if (!ret) {
		nl_req = &nl_obj->nl_requests[msg->cmd];
		nl_req->responded = 1;
		wake_up_interruptible(&nl_req->wq_request);
	}

	return ret;
}

static void nl_recv_msg_handler(struct sk_buff * skb)
{
	struct nlmsghdr * nlhdr = NULL;
	int len;
	int msg_len;
	struct iav_nl_obj *nl_obj;
	struct nl_msg_data msg;

	nlhdr = nlmsg_hdr(skb);
	len = skb->len;

	for(; NLMSG_OK(nlhdr, len); nlhdr = NLMSG_NEXT(nlhdr, len)) {
		if (nlhdr->nlmsg_len < sizeof(struct nlmsghdr)) {
			iav_error("NETLINK ERR: Corruptted msg!\n");
			continue;
		}
		msg_len = nlhdr->nlmsg_len - NLMSG_LENGTH(0);
		if (msg_len < sizeof(msg)) {
			iav_error("NETLINK ERR: Incorrect msg!\n");
			continue;
		}
		memcpy(&msg, NLMSG_DATA(nlhdr), sizeof(msg));
		if (NETLINK_CB(skb).portid == msg.pid) {
			nl_obj = find_nl_obj(msg.port);
			if (!nl_obj) {
				iav_error("NETLINK ERR: Unknown msg, port %u, app %u!\n",
					msg.port, msg.pid);
				continue;
			}

			if (msg.type == NL_MSG_TYPE_SESSION &&
				msg.dir == NL_MSG_DIR_CMD) {
				nl_process_session_cmd(nl_obj, &msg);
			}
			if (msg.type == NL_MSG_TYPE_REQUEST &&
				msg.dir == NL_MSG_DIR_STATUS) {
				nl_process_request_status(nl_obj, &msg);
			}
		}
	}
}

static int init_nl_obj_image(struct ambarella_iav *iav)
{
	struct iav_nl_obj *nl_obj = &iav->nl_obj[NL_OBJ_IMAGE];
	struct netlink_kernel_cfg cfg;
	int ret = 0;
	int i;

	nl_obj->iav = iav;
	nl_obj->nl_connected = 0;
	nl_obj->nl_init = 0;
	nl_obj->nl_port = NL_PORT_IMAGE;
	nl_obj->nl_user_pid = -1;
	nl_obj->nl_session_count = NL_SESS_CMD_NUM;
	nl_obj->nl_request_count = NL_REQ_IMG_NUM;

	for (i = NL_REQ_IMG_FIRST; i < NL_REQ_IMG_LAST; ++i) {
		nl_obj->nl_requests[i].request_id = i;
		init_waitqueue_head(&nl_obj->nl_requests[i].wq_request);
	}

	cfg.groups = 0;
	cfg.input = nl_recv_msg_handler;
	nl_obj->nl_sock = netlink_kernel_create(&init_net,
		nl_obj->nl_port, &cfg);

	nl_obj->nl_init = 1;

	return ret;
}

static int init_nl_obj_vsync(struct ambarella_iav *iav)
{
	struct iav_nl_obj *nl_obj = &iav->nl_obj[NL_OBJ_VSYNC];
	struct netlink_kernel_cfg cfg;
	int ret = 0;
	int i;

	nl_obj->iav = iav;
	nl_obj->nl_connected = 0;
	nl_obj->nl_init = 0;
	nl_obj->nl_port = NL_PORT_VSYNC;
	nl_obj->nl_user_pid = -1;
	nl_obj->nl_session_count = NL_SESS_CMD_NUM;
	nl_obj->nl_request_count = NL_REQ_VSYNC_NUM;

	for (i = NL_REQ_VSYNC_FIRST; i < NL_REQ_VSYNC_LAST; ++i) {
		nl_obj->nl_requests[i].request_id = i;
		init_waitqueue_head(&nl_obj->nl_requests[i].wq_request);
	}

	cfg.groups = 0;
	cfg.input = nl_recv_msg_handler;
	nl_obj->nl_sock = netlink_kernel_create(&init_net,
		nl_obj->nl_port, &cfg);

	nl_obj->nl_init = 1;

	return ret;
}

int init_netlink(struct ambarella_iav *iav)
{
	g_iav = iav;

	init_nl_obj_image(iav);
	init_nl_obj_vsync(iav);

	return 0;
}

