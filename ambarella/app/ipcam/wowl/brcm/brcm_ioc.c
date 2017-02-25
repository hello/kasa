/*******************************************************************************
 * brcm_ioc.c
 *
 * History:
 *    2015/8/8 - [Tao Wu] Create
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/ether.h>

#include <brcm_ioc.h>

static char G_iface[IFNAMSIZ] = "wlan0";
static wifi_chip_t G_brcm_chip = WIFI_BCM43340;
static int G_sys_exec = 0;

static int wl_pattern_atoh(char *src, char *dst)
{
	int i = 0;

	if (strncmp(src, "0x", 2) != 0 && strncmp(src, "0X", 2) != 0) {
		loge("Data invalid format. Needs to start with 0x\n");
		return -1;
	}
	src = src + 2; /* Skip past 0x */
	if (strlen(src) % 2 != 0) {
		loge("Data invalid format. Needs to be of even length\n");
		return -1;
	}
	for (i = 0; *src != '\0'; i++) {
		char num[3];
		strncpy(num, src, 2);
		num[2] = '\0';
		dst[i] = (uint8)strtoul(num, NULL, 16);
		src += 2;
	}

	return i;
}

static uint wl_iovar_mkbuf(const char *name, char *data, uint datalen,
	char *iovar_buf, uint buflen, int *perr)
{
	uint iovar_len = 0;

	iovar_len = strlen(name) + 1;

	/* check for overflow */
	if ((iovar_len + datalen) > buflen) {
		*perr = -1;
		loge("Buffer overflow, iovar_len[%u] + datalen[%u] > buflen[%u]\n",
			iovar_len, datalen, buflen);
		return 0;
	}

	/* copy data to the buffer past the end of the iovar name string */
	if (datalen > 0 && data) {
		memmove(&iovar_buf[iovar_len], data, datalen);
		iovar_len += datalen;
	}

	/* copy the name to the beginning of the buffer */
	strcpy(iovar_buf, name);

	*perr = 0;
	return iovar_len;
}

static int wl_ioctl(int cmd, void *buf, uint len, unsigned char set)
{
	struct ifreq ifr;
	wl_ioctl_t ioc;
	int ret = -1;
	int s = -1;

	strncpy(ifr.ifr_name, G_iface, IFNAMSIZ);

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		loge("cannot open socket: %s\n", strerror(errno));
		return -1;
	}

	ioc.cmd = cmd;
	ioc.buf = buf;
	ioc.len = len;
	ioc.set = set;
	ifr.ifr_data = (caddr_t) &ioc;

	ret = ioctl(s, SIOCDEVPRIVATE, &ifr);
	if (ret < 0) {
		loge("IOCTL SIOCDEVPRIVATE:%s. if[%s], fd[%d], cmd[%d], buf[%s], len[%d], set[%d]\n",
			strerror(errno), ifr.ifr_name, s, cmd, (char *)buf, len, set);
	}

	close(s);
	return ret;
}

/*  TCP Keep alive */
int wl_tcpka_conn_add(tcpka_conn_t *p_tcpka)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_MEDLEN];
	char buf_src_addr[IPV4_ADDR_STR_LEN];
	char buf_dst_addr[IPV4_ADDR_STR_LEN];
	char CMD[CMD_MINSIZE];
	int ret = -1;
	int i = 0;

	memset(buf_src_addr, 0, sizeof(buf_src_addr));
	memset(buf_dst_addr, 0, sizeof(buf_dst_addr));
	strncpy(buf_src_addr, iptoa(&p_tcpka->src_ip), sizeof(buf_src_addr));
	strncpy(buf_dst_addr, iptoa(&p_tcpka->dst_ip), sizeof(buf_dst_addr));
	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD),
		"tcpka_conn_add %s %s %s %d %d %d %u %u %d %u %u %u %u \"%s\"",
		ether_ntoa(&p_tcpka->dst_mac), buf_src_addr, buf_dst_addr,
		p_tcpka->ipid, p_tcpka->srcport, p_tcpka->dstport, p_tcpka->seq, p_tcpka->ack,
		p_tcpka->tcpwin, p_tcpka->tsval, p_tcpka->tsecr, p_tcpka->len,
		p_tcpka->ka_payload_len, p_tcpka->ka_payload);

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	buf_len = wl_iovar_mkbuf("tcpka_conn_add", (char *)p_tcpka,
		(sizeof(tcpka_conn_t) + p_tcpka->ka_payload_len - 1),
		bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(WLC_GET_VAR, bufdata, buf_len, 0);
	if (!ret) {
		logi("TCP KeepAlive Add: ID [%d], %s\n", p_tcpka->sess_id, CMD);
	} else {
		loge("TCP KeepAlive Add Failed: ID [%d], %s, %s\n",
			p_tcpka->sess_id, CMD, strerror(errno));
		logw("TCP KeepAlive Add: CMD DUMP=");
		for ( i = 0; i < buf_len; i++ ) {
			logw("%02x", bufdata[i]);
		}
		logw("\n");
	}
	return ret;
}

int wl_tcpka_conn_enable(uint32 sess_id, uint32 is_enable , uint16 itrvl,
	uint16 retry_itrvl, uint16 rety_cnt)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];
	tcpka_conn_sess_t tcpka_conn;
	int ret = -1;

	tcpka_conn.sess_id = sess_id;
	tcpka_conn.flag = is_enable;
	if (tcpka_conn.flag) {
		tcpka_conn.tcp_keepalive_timers.interval = itrvl;
		tcpka_conn.tcp_keepalive_timers.retry_interval = retry_itrvl;
		tcpka_conn.tcp_keepalive_timers.retry_count = rety_cnt;
	} else {
		tcpka_conn.tcp_keepalive_timers.interval = 0;
		tcpka_conn.tcp_keepalive_timers.retry_interval = 0;
		tcpka_conn.tcp_keepalive_timers.retry_count = 0;
	}

	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "tcpka_conn_enable %u %u %u %u %u",
		tcpka_conn.sess_id, tcpka_conn.flag,
		tcpka_conn.tcp_keepalive_timers.interval,
		tcpka_conn.tcp_keepalive_timers.retry_interval,
		tcpka_conn.tcp_keepalive_timers.retry_count);

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	buf_len = wl_iovar_mkbuf("tcpka_conn_enable", (char *)&tcpka_conn,
		sizeof(tcpka_conn_sess_t), bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len, 1);
	if (!ret) {
		logi("TCP KeepAlive Enable: %s\n", CMD);
	} else {
		loge("TCP KeepAlive Enable Failed: %s, %s\n", CMD, strerror(errno));
	}
	return ret;
}

int wl_tcpka_conn_sess_info(uint sess_id, tcpka_conn_sess_info_t *tcpka_sess_info)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	tcpka_conn_sess_info_t *info = NULL;
	uint id = 0;
	int ret = -1;

	id = sess_id;
	buf_len = wl_iovar_mkbuf("tcpka_conn_sess_info", (char *)&id, sizeof(uint32),
		bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(WLC_GET_VAR, bufdata, buf_len, 0);
	if (!ret) {
		info = (tcpka_conn_sess_info_t *) bufdata;
		tcpka_sess_info->tcpka_sess_ipid = info->tcpka_sess_ipid;
		tcpka_sess_info->tcpka_sess_seq = info->tcpka_sess_seq;
		tcpka_sess_info->tcpka_sess_ack = info->tcpka_sess_ack;
		logi("TCP KeepAlive Get Session: Id [%d] info: ipid[%u], seq[%u], ack[%u]\n", id,
			tcpka_sess_info->tcpka_sess_ipid,
			tcpka_sess_info->tcpka_sess_seq,
			tcpka_sess_info->tcpka_sess_ack);
	} else {
		loge("TCP KeepAlive Get Session Failed: Id [%d], %s\n", id, strerror(errno));
	}
	return ret;
}

int wl_tcpka_conn_del(uint sess_id)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];
	uint id = 0;
	int ret = -1;

	id = sess_id;
	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "tcpka_conn_del %u", id);

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	buf_len = wl_iovar_mkbuf("tcpka_conn_del", (char *)&id, sizeof(int32),
		bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len, 1);
	if (!ret) {
		logi("TCP KeepAlive Del: %s\n", CMD);
	} else {
		loge("TCP KeepAlive Del Failed: %s, %s\n", CMD, strerror(errno));
	}
	return ret;
}

int wl_wowl_pattern_bcm43340(int offset, char *mask, char *wowl_pattern)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];

	const char *str;
	wl_wowl_pattern_t *wl_pattern;
	char *buf, *mask_and_pattern;
	char *pattern;
	uint str_len = 0;
	int ret = -1;
	int i = 0;

	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "wowl_pattern add %d %s %s",
		offset, mask, wowl_pattern);

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	/* wl wowl_pattern add 0 0x0f 0x77616b65 : Receive "wake" tcp data */
	str = "wowl_pattern";
	str_len = strlen(str);
	strncpy(bufdata, str, str_len);
	bufdata[str_len] = '\0';
	buf = bufdata + strlen(str) + 1;
	buf_len = str_len + 1;

	str = "add";
	strncpy(buf, str, strlen(str));
	buf_len += strlen(str) + 1;

	wl_pattern = (wl_wowl_pattern_t *)(buf + strlen(str) + 1);
	mask_and_pattern = (char*)wl_pattern + sizeof(wl_wowl_pattern_t);
	wl_pattern->offset = offset;

	/* Parse the mask */
	wl_pattern->masksize = (strlen(mask) -2)/2;
	wl_pattern_atoh(mask, mask_and_pattern);
	mask_and_pattern += wl_pattern->masksize;
	wl_pattern->patternoffset = sizeof(wl_wowl_pattern_t) +	wl_pattern->masksize;

	/* Parse the pattern */
	pattern = wowl_pattern;
	wl_pattern->patternsize = (strlen(pattern)-2)/2;
	wl_pattern_atoh(pattern, mask_and_pattern);
	buf_len += sizeof(wl_wowl_pattern_t) + wl_pattern->patternsize + wl_pattern->masksize;

	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len, 1);
	if (!ret) {
		logi("WOWL Pattern: %s\n", CMD);
	} else {
		loge("WOWL Pattern Failed: %s, %s\n", CMD, strerror(errno));
		logw("WOWL Pattern: wowl_pattern[%d]=%s\n", strlen(pattern), pattern);
		logw("WOWL Pattern: CMD DUMP=");
		for ( i = 0; i < buf_len; i++ ) {
			logw("%02x", bufdata[i]);
		}
		logw("\n");
	}
	return ret;
}

int wl_wowl_pattern_bcm43438(int offset, char *mask, char *wowl_pattern)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];

	const char *str;
	wl_wowl_pattern_t_bcm43438 *wl_pattern;
	char *buf, *mask_and_pattern;
	char *pattern;
	uint str_len = 0;
	int ret = -1;
	int i = 0;

	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "wowl_pattern add %d %s %s",
		offset, mask, wowl_pattern);

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	/* wl wowl_pattern add 0 0x0f 0x77616b65 : Receive "wake" tcp data */
	str = "wowl_pattern";
	str_len = strlen(str);
	strncpy(bufdata, str, str_len);
	bufdata[str_len] = '\0';
	buf = bufdata + strlen(str) + 1;
	buf_len = str_len + 1;

	str = "add";
	strncpy(buf, str, strlen(str));
	buf_len += strlen(str) + 1;
	wl_pattern = (wl_wowl_pattern_t_bcm43438 *)(buf + strlen(str) + 1);
	mask_and_pattern = (char*)wl_pattern + sizeof(wl_wowl_pattern_t_bcm43438);
	wl_pattern->type = 0;
	wl_pattern->offset = offset;

	/* Parse the mask */
	wl_pattern->masksize = (strlen(mask) -2)/2;
	wl_pattern_atoh(mask, mask_and_pattern);

	mask_and_pattern += wl_pattern->masksize;
	wl_pattern->patternoffset = sizeof(wl_wowl_pattern_t_bcm43438) + wl_pattern->masksize;

	/* Parse the pattern */
	pattern = wowl_pattern;
	wl_pattern->patternsize = (strlen(pattern)-2)/2;
	wl_pattern_atoh(pattern, mask_and_pattern);
	buf_len += sizeof(wl_wowl_pattern_t_bcm43438) + wl_pattern->patternsize +
		wl_pattern->masksize;

	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len, 1);
	if (!ret) {
		logi("WOWL Pattern: %s\n", CMD);
	} else {
		loge("WOWL Pattern Failed: %s, %s\n", CMD, strerror(errno));
		logw("WOWL Pattern: wowl_pattern[%d]=%s\n", strlen(pattern), pattern);
		logw("WOWL Pattern: CMD DUMP=");
		for ( i = 0; i < buf_len; i++ ) {
			logw("%02x", bufdata[i]);
		}
		logw("\n");
	}
	return ret;
}

int wl_wowl_pattern(int offset, char *mask, char *wowl_pattern)
{
	if (G_brcm_chip == WIFI_BCM43438) {
		return wl_wowl_pattern_bcm43438(offset, mask, wowl_pattern);
	} else {
		return wl_wowl_pattern_bcm43340(offset, mask, wowl_pattern);
	}
}

int wl_wowl_pattern_clr(void)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];
	int ret = -1;
	char str[] = "clr";

	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "wowl_pattern %s", str);

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	buf_len = wl_iovar_mkbuf("wowl_pattern", str, sizeof(str),
		bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len, 1);
	if (!ret) {
		logi("WOWL Pattern Clear: %s\n", CMD);
	} else {
		loge("WOWL Pattern Clear Failed: %s, %s\n", CMD, strerror(errno));
	}
	return ret;
}

int wl_wowl_wakeind(wl_wowl_wakeind_t *wakeind)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];
	wl_wowl_wakeind_t *wake = NULL;
	int ret = -1;

	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "wowl_wakeind");

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	buf_len = wl_iovar_mkbuf("wowl_wakeind", NULL, 0, bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(WLC_GET_VAR, bufdata, buf_len, 0);
	if (!ret) {
		wake = (wl_wowl_wakeind_t *) bufdata;
		memcpy(wakeind, wake, sizeof(wl_wowl_wakeind_t));
		logi("WOWL Wakeind: %s. [0x%08x] Reason:\n", CMD, wake->ucode_wakeind);
		if (wake->ucode_wakeind != 0) {
			if ((wake->ucode_wakeind & WL_WOWL_MAGIC) == WL_WOWL_MAGIC)
				logi("\tMAGIC packet received\n");
			if ((wake->ucode_wakeind & WL_WOWL_NET) == WL_WOWL_NET)
				logi("\tPacket received with Netpattern\n");
			if ((wake->ucode_wakeind & WL_WOWL_DIS) == WL_WOWL_DIS)
				logi("\tDisassociation/Deauth received\n");
			if ((wake->ucode_wakeind & WL_WOWL_RETR) == WL_WOWL_RETR)
				logi("\tRetrograde TSF detected\n");
			if ((wake->ucode_wakeind & WL_WOWL_BCN) == WL_WOWL_BCN)
				logi("\tBeacons Lost\n");
			if ((wake->ucode_wakeind & WL_WOWL_TCPKEEP_DATA) == WL_WOWL_TCPKEEP_DATA)
				logi("\tWake on TCP Keepalive Data\n");
			if ((wake->ucode_wakeind & WL_WOWL_TCPKEEP_TIME) == WL_WOWL_TCPKEEP_TIME)
				logi("\tWake on TCP Keepalive Timeout\n");
			if ((wake->ucode_wakeind & WL_WOWL_TCPFIN) == WL_WOWL_TCPFIN)
				logi("\tWake on TCP FIN\n");
			if ((wake->ucode_wakeind & (WL_WOWL_NET | WL_WOWL_MAGIC))) {
				if ((wake->ucode_wakeind & WL_WOWL_BCAST) == WL_WOWL_BCAST)
					logi("\t\tBroadcast/Mcast frame received\n");
				else
					logi("\t\tUnicast frame received\n");
			}
		} else {
			logi("\tWake on other event\n");
		}
	} else {
		loge("WOWL Wakeind Failed: %s\n", strerror(errno));
	}
	return ret;
}

int wl_wowl_wakeind_clear(void)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];
	int ret = -1;
	char str[]="clear";

	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "wowl_wakeind %s", str);

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	buf_len = wl_iovar_mkbuf("wowl_wakeind", str, sizeof(str) + sizeof(wl_wowl_wakeind_t),
		bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len, 1);
	if (!ret) {
		logi("WOWL Wakeind Clear: %s\n", CMD);
	} else {
		loge("WOWL Wakeind Clear Failed: %s, %s\n", CMD, strerror(errno));
	}

	return 1;
}

int wl_wowl(uint32 flag)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];
	int ret = -1;
	uint32 wowl = 0;

	wowl = flag;
	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "wowl 0x%x", wowl);

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	buf_len = wl_iovar_mkbuf("wowl", (char *)&wowl, sizeof(uint32),
		bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len, 1);
	if (!ret) {
		logi("WOWL Flag: %s\n", CMD);
	} else {
		loge("WOWL Flag Failed: %s, %s\n", CMD, strerror(errno));
	}
	return ret;
}

int wl_wowl_activate(uint32 is_enable)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];
	int ret = -1;

	uint32 wowl_activate = is_enable;
	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "wowl_activate %u", wowl_activate);

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	buf_len = wl_iovar_mkbuf("wowl_activate", (char *)&wowl_activate, sizeof(uint32),
		bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len, 1);
	if (!ret) {
		logi("WOWL Active: %s\n", CMD);
	} else {
		loge("WOWL Active Failed: %s, %s\n", CMD, strerror(errno));
	}
	return ret;
}

int wl_wowl_clear(void)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];
	int ret = -1;

	memset(CMD, 0, CMD_MINSIZE);
	snprintf(CMD, sizeof(CMD), "wowl_clear");

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	buf_len = wl_iovar_mkbuf("wowl_clear", NULL, 0, bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	/* FIXME: why need add 4 byte more */
	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len + 4, 1);
	if (!ret) {
		logi("WOWL Clear: %s\n", CMD);
	} else {
		loge("WOWL Clear Failed: %s, %s\n", CMD, strerror(errno));
	}
	return ret;
}

/*  UDP Keep alive */
int wl_mkeep_alive(uint8 sess_id, uint32 interval, uint32 length, const u_char *packet)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_MEDLEN];
	char CMD[CMD_MAXSIZE];
	char CMD_BAK[CMD_MAXSIZE];

	const char *str;
	wl_mkeep_alive_pkt_t  mkeep_alive_pkt;
	wl_mkeep_alive_pkt_t  *p_mkeep_alive_pkt;
	uint str_len = 0;
	int ret = -1;
	int i = 0;

	memset(CMD, 0, sizeof(CMD));
	memset(CMD_BAK, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "mkeep_alive %u %u", sess_id, interval * 1000);
	strcat(CMD, " 0x");
	for (i = 0; i < length; i++) {
		strcpy(CMD_BAK, CMD);
		snprintf(CMD, sizeof(CMD), "%s%02x", CMD_BAK, packet[i]);
	}

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	str = "mkeep_alive";
	str_len = strlen(str);
	strncpy(bufdata, str, str_len);
	bufdata[ str_len ] = '\0';
	buf_len = str_len + 1;
	p_mkeep_alive_pkt = (wl_mkeep_alive_pkt_t *) (bufdata + str_len + 1);

	memset(&mkeep_alive_pkt, 0, sizeof(wl_mkeep_alive_pkt_t));
	mkeep_alive_pkt.period_msec = interval * 1000; // milliseconds
	mkeep_alive_pkt.version = WL_MKEEP_ALIVE_VERSION;
	mkeep_alive_pkt.length = WL_MKEEP_ALIVE_FIXED_LEN;
	mkeep_alive_pkt.keep_alive_id = sess_id;
	mkeep_alive_pkt.len_bytes = length;
	memcpy(p_mkeep_alive_pkt->data, packet, length);
	buf_len += WL_MKEEP_ALIVE_FIXED_LEN + mkeep_alive_pkt.len_bytes;
	memcpy((char *)p_mkeep_alive_pkt, &mkeep_alive_pkt, WL_MKEEP_ALIVE_FIXED_LEN);

	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len, 0);
	if (!ret) {
		logi("UDP KeepAlive: %s\n", CMD);
	} else {
		loge("UDP KeepAlive Failed: %s, %s\n", CMD, strerror(errno));
	}
	return ret;
}

int wl_pkt_filter_add(int id, int offset, char *mask, char *filter_pattern)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];

	const char *str;
	char *pattern;
	wl_pkt_filter_t	 pkt_filter;
	wl_pkt_filter_t	 *pkt_filterp;
	uint32 mask_size = 0;
	uint32 pattern_size = 0;
	int ret = -1;
	int str_len = 0;

	/* wl pkt_filter_add 200 0 0 36 0xffff 0x1EC5 : Receive UDP from port 7877 */
	str = "pkt_filter_add";
	str_len = strlen(str);
	strncpy(bufdata, str, str_len);
	bufdata[ str_len ] = '\0';
	buf_len = str_len + 1;
	pkt_filterp = (wl_pkt_filter_t *) (bufdata + str_len + 1);

	pkt_filter.id = id;
	pkt_filter.negate_match = 0;
	pkt_filter.type = 0;
	pkt_filter.u.pattern.offset = offset;

	mask_size = (strlen(mask) -2)/2;
	wl_pattern_atoh(mask, (char*) pkt_filterp->u.pattern.mask_and_pattern);
	pattern = filter_pattern;
	pattern_size = (strlen(pattern)-2)/2;
	wl_pattern_atoh(pattern, (char*) &pkt_filterp->u.pattern.mask_and_pattern[mask_size]);

	pkt_filter.u.pattern.size_bytes = pattern_size;
	buf_len += WL_PKT_FILTER_FIXED_LEN;
	buf_len += (WL_PKT_FILTER_PATTERN_FIXED_LEN + 2 * mask_size);

	memcpy((char *)pkt_filterp, &pkt_filter,
		WL_PKT_FILTER_FIXED_LEN + WL_PKT_FILTER_PATTERN_FIXED_LEN);

	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "pkt_filter_add %u %u %u %u %s %s", pkt_filter.id,
		 pkt_filter.negate_match, pkt_filter.type, pkt_filter.u.pattern.offset, mask, filter_pattern);

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len, 1);
	if (!ret) {
		logi("Pkt Filter Add: %s\n", CMD);
	} else {
		loge("Pkt Filter Add Failed: %s, %s\n", CMD, strerror(errno));
		logw("Pkt Filter Add: filter_pattern[%d]=%s\n", strlen(pattern), pattern);
	}
	return ret;
}

int wl_pkt_filter_enable(uint32 id, uint32 is_enable)
{
	int ret = -1;
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];
	wl_pkt_filter_enable_t enable_parm;

	/* Init pkt_filter data */
	enable_parm.id = id;
	enable_parm.enable = is_enable;
	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "pkt_filter_enable %u %u", enable_parm.id, enable_parm.enable);

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	buf_len = wl_iovar_mkbuf("pkt_filter_enable", (char *)&enable_parm,
		sizeof(wl_pkt_filter_enable_t), bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}
	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len, 1);
	if (!ret){
		logi("Pkt Filter Enable: %s\n", CMD);
	} else {
		loge("Pkt Filter Enable Failed: %s, %s\n", CMD, strerror(errno));
	}
	return ret;
}

int wl_pkt_filter_delete(uint32 id)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];
	int ret = -1;

	uint32 filter_id = id;
	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "pkt_filter_delete %u", filter_id);

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	buf_len = wl_iovar_mkbuf("pkt_filter_delete", (char *)&filter_id, sizeof(uint32),
		bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len, 1);
	if (!ret) {
		logi("Pkt Filter Del: %s\n", CMD);
	} else {
		loge("Pkt Filter Del Failed: %s, %s\n", CMD, strerror(errno));
	}
	return ret;
}

int wl_pkt_filter_stats(uint32 id, wl_pkt_filter_stats_t *filter_stats)
{
	int ret = -1;
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	wl_pkt_filter_stats_t *stats = NULL;
	uint32 filter_id;

	filter_id = id;
	buf_len = wl_iovar_mkbuf("pkt_filter_stats", (char *)&filter_id, sizeof(uint32),
		bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(WLC_GET_VAR, bufdata, buf_len, 0);
	if (!ret) {
		stats = (wl_pkt_filter_stats_t *) bufdata;
		filter_stats->num_pkts_matched = stats->num_pkts_matched;
		filter_stats->num_pkts_discarded = stats->num_pkts_discarded;
		filter_stats->num_pkts_forwarded = stats->num_pkts_forwarded;
		logi("Pkt Filter State Get: Id [%d]: matched[%d], discarded[%d], forwarded[%d]\n", filter_id,
			filter_stats->num_pkts_matched,
			filter_stats->num_pkts_discarded,
			filter_stats->num_pkts_forwarded);
	} else {
		loge("Pkt Filter State Get Failed: Id [%d], %s\n", filter_id, strerror(errno));
	}

	return ret;
}

int wl_set_get_pm_mode(int *mode, int is_set)
{
	int cmd_op = 0;
	int power = 0;
	int set = -1;
	int ret = -1;

	if (is_set) {
		cmd_op = WLC_SET_PM;
		set = 1;
		power = *mode;
	} else {
		cmd_op = WLC_GET_PM;
		set = 0;
	}

	ret = wl_ioctl(cmd_op, &power, sizeof(int), set);
	if (!ret) {
		if (is_set)
			logi("PM Set [%d]\n", power);
		else {
			*mode = power;
			logi("PM Get [%d]\n", *mode);
		}
	} else {
		loge("PM %d [%d] Failed: %s\n", is_set, power, strerror(errno));
	}
	return ret;
}

int wl_set_get_host_sleep(int *sleep, int is_set)
{
	int buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	int cmd_op = 0;
	int mode = 0;
	int set = 0;
	int ret = -1;
	int *p_value = NULL;

	if (is_set) {
		cmd_op = WLC_SET_VAR;
		set = 1;
		mode = *sleep;
	} else {
		cmd_op = WLC_GET_VAR;
		set = 0;
	}

	buf_len = wl_iovar_mkbuf("hostsleep", (char *)&mode, sizeof(int),
		bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(cmd_op, bufdata, buf_len, set);
	if (!ret) {
		if (is_set) {
			logi("Hostsleep set [%d]\n", mode);
		} else {
			p_value = (int *)(&bufdata[0]);
			*sleep = *p_value;
			logi("Hostsleep get [%d]\n", *sleep);
		}
	} else {
		loge("Hostsleep %d [%d] Failed: %s\n", is_set, *sleep, strerror(errno));
	}
	return ret;
}

int wl_set_get_host_cipher(uint8 *cipher_str, uint32 len, int is_set)
{
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	int cmd_op = 0;
	int ret = -1;
	int set = 0;
	int i = 0;
	wlc_host_cipher_t cipher1;

	memset(&cipher1, 0, sizeof(cipher1));
	if (is_set) {
		cmd_op = WLC_SET_VAR;
		set = 1;
		cipher1.len = len;
		memcpy(&cipher1.payload[0], cipher_str, len);
	} else {
		cmd_op = WLC_GET_VAR;
		set = 0;
	}

	buf_len = wl_iovar_mkbuf("host_cipher", (char *)&cipher1, sizeof(wlc_host_cipher_t),
		bufdata, sizeof(bufdata), &ret);
	if (ret) {
		loge("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(cmd_op, bufdata, buf_len, set);
	if (!ret) {
		if (is_set) {
			logi("Host_cipher[%d]: Set OK\n", cipher1.len);
		} else {
			memcpy(&cipher1, bufdata, sizeof(wlc_host_cipher_t));
			logi("Host_cipher[%d]: Get OK\n", cipher1.len);
			logv("Host_cipher dump:\n");
			for(i = 0; i < cipher1.len; i++) {
				logv("%02x,", cipher1.payload[i]);
				if((i + 1) % 8 == 0)
					logv("\n");
			}
		}
	} else {
		loge("Host_cipher Failed: %d, %s\n", is_set, strerror(errno));
	}
	return ret;
}

int wl_get_ap_beacon_interval(int *ap_beacon_interval)
{
	int ret = -1;

	ret = wl_ioctl(WLC_GET_BCNPRD, ap_beacon_interval, sizeof(int), 0);
	if (!ret) {
		logi("AP Beacon interval get [%d]\n", *ap_beacon_interval);
	} else {
		loge("AP Beacon interval get failed: %s\n", strerror(errno));
	}
	return ret;
}

int wl_get_ap_dtim(int *ap_dtim)
{
	int ret = -1;

	ret = wl_ioctl(WLC_GET_DTIMPRD, ap_dtim, sizeof(int), 0);
	if (!ret) {
		logi("AP DTIM get [%d]\n", *ap_dtim);
	} else {
		loge("AP DTIM get failed: %s\n", strerror(errno));
	}
	return ret;
}

int wl_set_bcn_li_dtim(int bcn_li_dtim)
{
	int ret = -1;
	uint buf_len = 0;
	char bufdata[WLC_IOCTL_SMLEN];
	char CMD[CMD_MINSIZE];

	memset(CMD, 0, sizeof(CMD));
	snprintf(CMD, sizeof(CMD), "bcn_li_dtim %d", bcn_li_dtim);

	if (G_sys_exec) {
		snprintf(CMD, sizeof(CMD), "%s %s", BRCM_TOOL, CMD);
		system(CMD);
		logi("CMD: %s\n", CMD);
		return 0;
	}

	buf_len = wl_iovar_mkbuf("bcn_li_dtim", (char *)&bcn_li_dtim, sizeof(int),
		bufdata, sizeof(bufdata), &ret);
	if (ret) {
		printf("Failed to build buffer, %s.\n", __FUNCTION__);
		return -1;
	}

	ret = wl_ioctl(WLC_SET_VAR, bufdata, buf_len, 1);
	if (!ret) {
		logi("Bcn li DTIM set: %s\n", CMD);
	} else {
		loge("Bcn li DTIM set failed: %s, %s\n", CMD, strerror(errno));
	}
	return ret;
}

int wl_set_dtim_interval(int dtim_interval)
{
	int ret = -1;
	int ap_beacon_interval = 100;
	int ap_dtim = 1;
	int sta_bcn_li_dtim = 3;

	ret = wl_get_ap_beacon_interval(&ap_beacon_interval);
	if (ret) {
		loge("Get AP bi failed\n");
	}
	ret = wl_get_ap_dtim(&ap_dtim);
	if (ret) {
		loge("Get AP dtim failed\n");
	}
	if ((ap_beacon_interval == 0) || (ap_dtim == 0)) {
		logw("Get AP beacon abnormal. ap_beacon_interval[%d], ap_dtim[%d]\n",
			ap_beacon_interval, ap_dtim);
		return -1;
	}

	sta_bcn_li_dtim = (int)(dtim_interval/( ap_beacon_interval * ap_dtim));
	if (sta_bcn_li_dtim > 0) {
		ret = wl_set_bcn_li_dtim(sta_bcn_li_dtim);
	}
	logd("ap_beacon_interval[%d], ap_dtim[%d], bcn_li_dtim[%d]. Result: DTIM Interval is [%d] ms\n",
		ap_beacon_interval, ap_dtim, sta_bcn_li_dtim, (sta_bcn_li_dtim) ?
		 (ap_beacon_interval * ap_dtim * sta_bcn_li_dtim):(ap_beacon_interval * ap_dtim));

	return ret;
}

void brcm_ioc_wowl_init(char* iface, wifi_chip_t chip, int sys_exec)
{
	logi("BRCM Ioctl Library Version: %s, Compile time: %s\n", BRCM_IOCTL_VERSION, __TIME__);

	strncpy(G_iface, iface, sizeof(G_iface));
	G_brcm_chip = chip;
	G_sys_exec = sys_exec;

	logi("BRCM Ioctl Library Config: Iface[%s], Chip[%d], Exec[%d].\n",
		G_iface, G_brcm_chip, G_sys_exec);
}

