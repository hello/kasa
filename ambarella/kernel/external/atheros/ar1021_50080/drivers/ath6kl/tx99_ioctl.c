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

#include "core.h"
#include "hif-ops.h"
#include "cfg80211.h"
#include "target.h"
#include "debug.h"
#include "ce_athtst.h"
#include "hif-ops.h"
#include "tx99_wcmd.h"
#include <linux/vmalloc.h>
#ifdef TX99_SUPPORT
#define ATH_MAC_LEN 6
#define MPATTERN                (10*4)
struct TCMD_CONT_TX {
	uint32_t                testCmdId;
	uint32_t                mode;
	uint32_t                freq;
	uint32_t                dataRate;
	int32_t                 txPwr;
	uint32_t                antenna;
	uint32_t                enANI;
	uint32_t                scramblerOff;
	uint32_t                aifsn;
	uint16_t                pktSz;
	uint16_t                txPattern;
	uint32_t                shortGuard;
	uint32_t                numPackets;
	uint32_t                wlanMode;
	uint32_t                lpreamble;
	uint32_t	            txChain;
	uint32_t                miscFlags;

	uint32_t                broadcast;
	uint8_t                 bssid[ATH_MAC_LEN];
	uint16_t                bandwidth;
	uint8_t                 txStation[ATH_MAC_LEN];
	uint16_t                unUsed2;
	uint8_t                 rxStation[ATH_MAC_LEN];
	uint16_t                unUsed3;
	uint32_t                tpcm;
	uint32_t                retries;
	uint32_t                agg;
	uint32_t                nPattern;
	uint8_t                 dataPattern[MPATTERN]; /* bytes to be written */
} __packed;

union TEST_CMD {
	struct TCMD_CONT_TX         contTx;
} __packed;

enum TCMD_ID {
	TCMD_CONT_TX_ID,
	TCMD_CONT_RX_ID,
	TCMD_PM_ID,
	TC_CMDS_ID,
	TCMD_SET_REG_ID,
	TC_CMD_TLV_ID,
	TCMD_CMDS_PSAT_CAL,
	TCMD_CMDS_PSAT_CAL_RESULT,
	TCMD_CMDS_CHAR_PSAT,
	TCMD_CMDS_CHAR_PSAT_RESULT,

	INVALID_CMD_ID = 255,
};

enum TCMD_WLAN_MODE {
	TCMD_WLAN_MODE_NOHT = 0,
	TCMD_WLAN_MODE_HT20 = 1,
	TCMD_WLAN_MODE_HT40PLUS = 2,
	TCMD_WLAN_MODE_HT40MINUS = 3,
};

enum TPC_TYPE {
	TPC_TX_PWR = 0,
	TPC_FORCED_GAIN,
	TPC_TGT_PWR,
	TPC_TX_FORCED_GAIN
};
static uint8_t BSSID_DEFAULT[ATH_MAC_LEN] = {
	0x00, 0x03, 0x7f, 0x03, 0x40, 0x33}; /* arbitary macAddr*/
enum TCMD_CONT_TX_MODE {
	TCMD_CONT_TX_OFF = 0,
	TCMD_CONT_TX_SINE,
	TCMD_CONT_TX_FRAME,
	TCMD_CONT_TX_TX99,
	TCMD_CONT_TX_TX100
};

int ath6kl_wmi_tx99_cmd(struct net_device *netdev, void *data)
{
	struct ath6kl_vif *vif = netdev_priv(netdev);
	struct tx99_wcmd_t     *req = NULL;
	unsigned long      req_len = sizeof(struct tx99_wcmd_t);
	unsigned long      status = EIO;
	static struct tx99_wcmd_data_t tx99_cmd = {
		2412,
		0,
		0,
		11,
		0,
		0,
		3,
		0,
	};
	union TEST_CMD test_tcmd;
	void *buf = &test_tcmd;
	int ret;
	int buf_len = sizeof(union TEST_CMD);
	enum  tx99_wcmd_type_t    type;             /**< Type of wcmd */
	req = vmalloc(req_len);

	if (!req)
		return -ENOMEM;

	memset(req, 0, req_len);

	/* XXX: Optimize only copy the amount thats valid */
	if (copy_from_user(req, data, req_len))
		goto done;

	/*use ath6kl_wmi_test_cmd format*/
	type = req->type;
	if ((type == TX99_WCMD_ENABLE) || (type == TX99_WCMD_DISABLE)) {
		memset(&test_tcmd, 0x00, sizeof(test_tcmd));
		test_tcmd.contTx.testCmdId = TCMD_CONT_TX_ID;
		test_tcmd.contTx.numPackets = 0;
		test_tcmd.contTx.wlanMode = TCMD_WLAN_MODE_NOHT;
		test_tcmd.contTx.tpcm = TPC_TX_PWR;
		test_tcmd.contTx.txChain = 1;/* default to use chain 0 */
		test_tcmd.contTx.broadcast = 1;/* default to broadcast */
		test_tcmd.contTx.agg = 1;
		test_tcmd.contTx.miscFlags = 0;
		test_tcmd.contTx.bandwidth = 0;
		memcpy(test_tcmd.contTx.bssid, BSSID_DEFAULT, ATH_MAC_LEN);
		test_tcmd.contTx.pktSz = 999;
		test_tcmd.contTx.freq = 2412;
		test_tcmd.contTx.dataRate = 11;/*54Mb*/
		test_tcmd.contTx.enANI = true;
		test_tcmd.contTx.scramblerOff = 1;
		test_tcmd.contTx.aifsn = 1;

		if (type == TX99_WCMD_ENABLE) {
			test_tcmd.contTx.mode = TCMD_CONT_TX_TX99;
			if (tx99_cmd.type == 1)
				test_tcmd.contTx.mode = TCMD_CONT_TX_SINE;
		} else if (type == TX99_WCMD_DISABLE)
			test_tcmd.contTx.mode = TCMD_CONT_TX_OFF;

		switch (tx99_cmd.htmode) {
		case 0:/* HT20 */
			test_tcmd.contTx.wlanMode = TCMD_WLAN_MODE_HT20;
			break;
		case 2:/* HT40 */
			switch (tx99_cmd.htext) {
			case 0:/*default*/
				test_tcmd.contTx.wlanMode =
				TCMD_WLAN_MODE_HT40PLUS;
				break;
			case 1:/*plus*/
				test_tcmd.contTx.wlanMode =
				TCMD_WLAN_MODE_HT40PLUS;
				break;
			case 2:/*minus*/
				test_tcmd.contTx.wlanMode =
				TCMD_WLAN_MODE_HT40MINUS;
				break;
			}
			break;
		default:
			test_tcmd.contTx.wlanMode = TCMD_WLAN_MODE_NOHT;
			break;
		}

		test_tcmd.contTx.antenna = tx99_cmd.chanmask;
		if (test_tcmd.contTx.mode != TCMD_CONT_TX_SINE)
			test_tcmd.contTx.txPwr = tx99_cmd.power*2;
		else
			test_tcmd.contTx.txPwr = tx99_cmd.power;
		test_tcmd.contTx.freq = tx99_cmd.freq;

		test_tcmd.contTx.dataRate = tx99_cmd.rate;
		ret = ath6kl_wmi_test_cmd(vif->ar->wmi, buf, buf_len);
	} else {/*record parameter in driver side;*/
		switch (type) {
		case TX99_WCMD_SET_FREQ:/* Mhz */
			tx99_cmd.freq = req->data.freq;
			tx99_cmd.htmode = req->data.htmode;/*HT20(0), or HT40
								mode(2)*/
			tx99_cmd.htext = req->data.htext;/* 0(none), 1(plus)
							and 2(minus)*/
			break;
		case TX99_WCMD_SET_RATE:
			tx99_cmd.rate = req->data.rate;
			break;
		case TX99_WCMD_SET_POWER:/*unit is 0.5dbm*/
			tx99_cmd.power = req->data.power;
			break;
		case TX99_WCMD_SET_TXMODE:/*no use*/
			tx99_cmd.txmode = req->data.txmode;
			break;
		case TX99_WCMD_SET_CHANMASK: /*1, 2, 3 (for 2*2)*/
			tx99_cmd.chanmask = req->data.chanmask;
			break;
		case TX99_WCMD_SET_TYPE:/*0: data modulated, 1:single carrier*/
			tx99_cmd.type = req->data.type;
			break;
		default:
			break;
		}
	}

	up(&vif->ar->sem);

	status = 0;

	/* XXX: Optimize only copy the amount thats valid */
	if (copy_to_user(data, req, req_len))
		status = -EIO;

done:
	vfree(req);

	return status;
}
#endif
