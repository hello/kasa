/*
 * Copyright (c) 2004-2011 Atheros Communications Inc.
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

#include <linux/moduleparam.h>
#include <linux/inetdevice.h>

#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#endif

#include "core.h"
#include "cfg80211.h"
#include "debug.h"
#include "hif-ops.h"
#include "testmode.h"
#include "cfg80211_btcoex.h"

bool __ath6kl_btcoex_cfg80211_ready(struct ath6kl *ar)
{
	if (!test_bit(WMI_READY, &ar->flag)) {
		ath6kl_err("wmi is not ready\n");
		return false;
	}

	return true;
}

bool ath6kl_btcoex_cfg80211_ready(struct ath6kl_vif *vif)
{
	if (!__ath6kl_btcoex_cfg80211_ready(vif->ar))
		return false;

	if (!test_bit(WLAN_ENABLED, &vif->flags))
		return false;

	return true;
}

#ifdef NL80211_CMD_BTCOEX_QCA
int ath6kl_notify_btcoex(struct wiphy *wiphy, u8 *buf,
					int len)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct ath6kl_vif *vif;
	int ret;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "BT coex wmi command:%p\n", buf);

	if (!ath6kl_btcoex_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	ret = ath6kl_wmi_send_btcoex_cmd(ar, buf, len);

	up(&ar->sem);

	return ret;
}
#endif
