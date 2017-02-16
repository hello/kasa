/*
 * Copyright (c) 2011 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "linux/if.h"
#include "linux/socket.h"
#include "linux/netlink.h"
#include <net/sock.h>

#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/cache.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include "ath_netlink.h"
#include "rttapi.h"
#include "core.h"
#include "debug.h"
#define MAX_PAYLOAD 1024

static struct sock *ath_nl_sock;
static u32 gpid;

void ath_netlink_send(char *event_data, u32 event_datalen)
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh;

	skb = nlmsg_new(NLMSG_SPACE(event_datalen), GFP_ATOMIC);
	if (!skb) {
		ath6kl_err("%s: No memory,\n", __func__);
		return;
	}

	nlh = nlmsg_put(skb, gpid, 0, 0 , NLMSG_SPACE(event_datalen), 0);
	if (!nlh) {
		ath6kl_err("%s: nlmsg_put() failed\n", __func__);
		return;
	}

	memcpy(NLMSG_DATA(nlh), event_data, event_datalen);

#ifdef ATH6KL_SUPPORT_NETLINK_KERNEL3_7
	NETLINK_CB(skb).portid = 0;        /* from kernel */
#else
	NETLINK_CB(skb).pid = 0;        /* from kernel */
#endif
	NETLINK_CB(skb).dst_group = 0;  /* unicast */
	netlink_unicast(ath_nl_sock, skb, gpid, MSG_DONTWAIT);
}

/* --adhoc_netlink_data_ready--
 * Stub function that does nothing */
void ath_netlink_reply(int pid, int seq, void *payload)
{
	return;
}

/* Receive messages from netlink socket. */
#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
static void ath_netlink_receive(struct sk_buff *__skb, int len)
#else
static void ath_netlink_receive(struct sk_buff *__skb)
#endif
{
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	u_int8_t *data = NULL;
	u_int32_t uid, pid, seq;

	skb = skb_get(__skb);
	if (skb) {
		/* process netlink message pointed by skb->data */
		nlh = (struct nlmsghdr *)skb->data;
		pid = NETLINK_CREDS(skb)->pid;
		pid = nlh->nlmsg_pid;
		uid = NETLINK_CREDS(skb)->uid;
		seq = nlh->nlmsg_seq;
		data = NLMSG_DATA(nlh);

		gpid = pid;
		ath_netlink_reply(pid, seq, data);
		rttm_issue_request(data, nlmsg_len(nlh));
		kfree_skb(skb);
	}
	return ;
}

int ath_netlink_init()
{
#ifdef ATH6KL_SUPPORT_NETLINK_KERNEL3_7
	struct netlink_kernel_cfg netlink_cfg;

	if (ath_nl_sock == NULL) {
		netlink_cfg.groups = 1;
		netlink_cfg.input = ath_netlink_receive;
		netlink_cfg.cb_mutex = NULL;
		netlink_cfg.bind = NULL;
		netlink_cfg.flags = 0;

		ath_nl_sock = (struct sock *)netlink_kernel_create(
					&init_net, NETLINK_ATH_EVENT,
					&netlink_cfg);
		if (ath_nl_sock == NULL) {
			ath6kl_err("%s NetLink Create Failed\n", __func__);
			return -ENODEV;
		}
	}
#elif defined(ATH6KL_SUPPORT_NETLINK_KERNEL3_6)
	struct netlink_kernel_cfg netlink_cfg;

	if (ath_nl_sock == NULL) {
		netlink_cfg.groups = 1;
		netlink_cfg.input = ath_netlink_receive;
		netlink_cfg.cb_mutex = NULL;
		netlink_cfg.bind = NULL;

		ath_nl_sock = (struct sock *)netlink_kernel_create(
					&init_net, NETLINK_ATH_EVENT,
					THIS_MODULE,
					&netlink_cfg);
		if (ath_nl_sock == NULL) {
			ath6kl_err("%s NetLink Create Failed\n", __func__);
			return -ENODEV;
		}
	}
#else
	if (ath_nl_sock == NULL) {
#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
		ath_nl_sock = (struct sock *)netlink_kernel_create(
					NETLINK_ATH_EVENT,
					1, &ath_netlink_receive, NULL,
					THIS_MODULE);
#else
		ath_nl_sock = (struct sock *)netlink_kernel_create(
					&init_net, NETLINK_ATH_EVENT,
					1, &ath_netlink_receive, NULL,
					THIS_MODULE);
#endif
		if (ath_nl_sock == NULL) {
			ath6kl_err("%s NetLink Create Failed\n", __func__);
			return -ENODEV;
		}
	}
#endif

	return 0;
}

int ath_netlink_free(void)
{
	if (ath_nl_sock) {
#ifdef CE_OLD_KERNEL_SUPPORT_2_6_23
		sock_release(ath_nl_sock->sk_socket);
#else
		netlink_kernel_release(ath_nl_sock);
#endif
		ath_nl_sock = NULL;
	}

	return 0;
}
