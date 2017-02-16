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

#ifndef _TX99_WCMD_H
#define _TX99_WCMD_H

enum tx99_wcmd_type_t {
	TX99_WCMD_ENABLE,
	TX99_WCMD_DISABLE,
	TX99_WCMD_SET_FREQ,
	TX99_WCMD_SET_RATE,
	TX99_WCMD_SET_POWER,
	TX99_WCMD_SET_TXMODE,
	TX99_WCMD_SET_CHANMASK,
	TX99_WCMD_SET_TYPE,
	TX99_WCMD_GET
};

struct tx99_wcmd_data_t {
	u_int32_t freq;	/* tx frequency (MHz) */
	u_int32_t htmode; /* tx bandwidth (HT20/HT40) */
	u_int32_t htext; /* extension channel offset (0(none),
				1(plus) and 2(minus)) */
	u_int32_t rate;	/* Kbits/s */
	u_int32_t power; /* (dBm) */
	u_int32_t txmode; /* wireless mode, 11NG(8), auto-select(0) */
	u_int32_t chanmask; /* tx chain mask */
	u_int32_t type;
};

struct tx99_wcmd_t {
	char                if_name[IFNAMSIZ];/**< Interface name */
	enum tx99_wcmd_type_t    type;             /**< Type of wcmd */
	struct tx99_wcmd_data_t    data;             /**< Data */
};

int ath6kl_wmi_tx99_cmd(struct net_device *netdev, void *data);
#endif /* _TX99_WCMD_H */
