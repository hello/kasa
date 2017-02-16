/*
 * Copyright (c) 2010, Atheros Communications Inc.
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

#ifndef _ACL_WCMD_H
#define _ACL_WCMD_H
struct acl_wcmd_t {
	u32		           mode;           /* Operation mode */
	u32		           val;            /* Operation val */
};

/*
 * Used with WMI_AP_ACL_POLICY_CMDID
 */
#define AP_ACL_DISABLE          0x00
#define AP_ACL_ALLOW_MAC        0x01
#define AP_ACL_DENY_MAC         0x02
#define AP_ACL_RETAIN_LIST_MASK 0x80
struct WMI_AP_ACL_POLICY_CMD {
	u8     policy;
};


#define ADD_MAC_ADDR    1
#define DEL_MAC_ADDR    2
#define ATH_MAC_LEN             6
struct WMI_AP_ACL_MAC_CMD {
	u8     action;
	u8     index;
	u8     mac[ATH_MAC_LEN];
	u8     wildcard;
} ;

int ath6kl_wmi_set_param_cmd(struct net_device *netdev, void *data);
int ath6kl_wmi_get_param_cmd(struct net_device *netdev, void *data);
int ath6kl_wmi_addmac_cmd(struct net_device *netdev, void *data);
int ath6kl_wmi_delmac_cmd(struct net_device *netdev, void *data);
int ath6kl_wmi_getmac_cmd(struct net_device *netdev, void *data);
int ath6kl_wmi_set_mlme_cmd(struct net_device *netdev, void *data);
#endif /* _ACL_WCMD_H */
