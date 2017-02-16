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

#ifndef _WLAN_LOCATION_DEFS_H
#define _WLAN_LOCATION_DEFS_H

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif
#define NUM_OFDM_TONES_ACK_FRAME 52
#define RESOLUTION_IN_BITS       10
#define RTTM_CDUMP_SIZE(rxchain, bw40)			\
	((NUM_OFDM_TONES_ACK_FRAME * 2 * (bw40 + 1) *	\
	 (rxchain) * RESOLUTION_IN_BITS) / 8)
#define MAX_RTTREQ_MEAS 10
#define NUM_WLAN_BANDS 2
#define MAX_CHAINS 2
#define NUM_CLKOFFSETCAL_SAMPLES 5
#define CLKOFFSET_GPIO_PIN 26

#define NSP_MRQST_MEASTYPEMASK 0x3

#define MRQST_MODE_RTT 0x0
#define MRQST_MODE_CIR 0x1
#define MRQST_MODE_DBG 0x2

#define NSMP_MRQST_MEASTYPE(mode) (mode & 0x3)
#define NSP_MRQST_FRAMETYPE(mode) ((mode>>2) & 0x7)
#define NSP_MRQST_TXCHAIN(mode) ((mode>>5) & 0x3)
#define NSP_MRQST_RXCHAIN(mode) ((mode>>7) & 0x3)
#define NSP_MRQST_REQMETHOD(mode) ((mode>>9) & 0x3)
#define NSP_MRQST_MOREREQ(mode) ((mode>>11) & 0x1)

struct nsp_header {
	u8 version;
	u8 frame_type;
};
#define NSP_HDR_LEN sizeof(struct nsp_header)

struct nsp_mrqst {
	u32 sta_info;
	/* A bit field used to provide information about the STA
	 * The bit fields include the station type (WP Capable/Non Capable)
	 * and other fields to be defined in the future.
	 * STAInfo[0]: Station Type: 0=STA is not WP Capable,1=STA is WP Capable
	 * STAInfo[31:1]: TBD */

	u32 transmit_rate;
	/* Rate requested for transmission. If value is all zeros
	 * the AP will choose its own rate. If not the AP will honor this
	 * 31:0: IEEE Physical Layer Transmission Rate of the Probe frame */

	u32 timeout;
	/* Time out for collecting all measurements. This includes the
	 * time taken to send out all sounding frames and retry attempts.
	 * Measured in units of milliseconds.For the Immediate Measurement mode,
	 * the WLAN AP system must return a Measurement Response after the
	 * lapse of an interval equal to “timeout” milliseconds after
	 * reception of the Measurement Request */
	u32 reserved;
	u16 mode;
#define FRAME_TYPE_HASH 0x001c
#define NULL_FRAME 0
#define QOS_NULL_FRAME 4
#define RTS_CTS 8
	/* Bits 1:0: Type of measurement:
	   00: RTT, 01: CIR
	 * Bits 4:2: 802.11 Frame Type to use as Probe
	   000: NULL, 001: Qos NULL, 010: RTS/CTS
	 * Bits 6:5 Transmit chainmask to use for transmission
	   01: 1, 10: 2, 11:3
	 * Bits 8:7: Receive chainmask to use for reception
	   01: 1, 10: 2, 11:3
	 * Bits 10:9: The method by which the request should be serviced
	   00 = Immediate: The request must be serviced as soon as possible
	   01 = Delayed: The WPC can defer the request to when
		it deems appropriate
	   10 = Cached: The WPC should service the request from cached
		results only */

	u8 request_id;
	/* A unique ID which identifies the request. It is the responsibility
	 * of the application to ensure that this is unique.
	 * The library will use this Id in its response. */

	u8 sta_mac_addr[ETH_ALEN];
	/* The MAC Address of the STA for which a measurement is requested.*/

	u8 spoof_mac_addr[ETH_ALEN];
	/* The MAC Address which the AP SW should use as the source
	 * address when sending out the sounding frames */

	u8 channel;
	/* The channel on which the STA is currently listening.
	 * The channel is specified in the notation (1-11 for 2.4 GHz
	 * and 36 – 169 for 5 GHz.
	 * If a STA is in HT40 mode, then the channel will indicate the
	 * control channel. Probe frames will always be sent at HT20 */

	u8 no_of_measurements;
	/* The number of results requested i.e the WLAN AP can stop measuring
	 * after it successfully completes a number of measurements equal
	 * to this number. For RTT based measurement this will always = 1 */
	u8 reserved2[3];
};

/* NSP Capability Request */
struct nsp_crqst {
	u8 request_id;
	/* A unique ID which identifies the request. It is the responsibility
	 * of the application to ensure that this is unique.
	 * The library will use this Id in its response. */
};

/*NSP Status Request*/
struct nsp_srqst {
	u8 request_id;
	/* A unique ID which identifies the request. It is the responsibility
	 * of the application to ensure that this is unique.
	 * The library will use this Id in its response. */
};

#define MRESP_RTT 0x0
#define MRESP_CIR 0x1
#define MRESP_DBG 0x
#define MRESP_CLKOFFSETCAL_START 0x3
#define MRESP_CLKOFFSETCAL_END   0x4

struct nsp_mresphdr {
	u8 request_id;
	/* A unique ID which identifies the request. It is the responsibility
	 * of the application to ensure that this is unique.
	 * The library will use this Id in its response. */
	u8 sta_mac_addr[ETH_ALEN];
	/* The MAC Address of the STA for which a measurement is requested.*/

	u8 response_type;
	/*Type Of Response 0:CIR 1:RTT*/

	u16 no_of_responses;
	/* no of responses */

	u16 result;
	/* Result Of Probe Bit-0 0:Complete  1:More Responses For this
	 * Request ID*/

	u32 begints;
	/* FW Timestamp when RTT Req Begins */

	u32 endts;
	/* FW Timestamp when RTT Req processing completes*/

	u32 reserved;
};

#define RTTM_CHANNEL_DUMP_LEN RTTM_CDUMP_SIZE(MAX_CHAINS, 1)
struct nsp_cir_resp {
	u32 tod;
	/* A timestamp indicating when the measurement probe was sent */

	u32 toa;
	/* A timestamp indicating when the probe response was received */

	u32 sendrate;
	/* IEEE Physical Layer Transmission Rate of the Send Probe frame */

	u32 recvrate;
	/*IEEE Physical Layer Transmission Rate of the response to the
	 * Probe Frame*/

	u8 channel_dump[RTTM_CHANNEL_DUMP_LEN];
	/* channel dump for max no of chains */

	u8 no_of_chains;
	/* Number of chains used for reception */

	u8 rssi[MAX_CHAINS];
	/* Received signal strength indicator */

	u8  isht40;

	s8  fwcorr;

	u8 reserved[3];
};

struct rttresp_meas_debug {
	u32 rtt;
	int rtt_cal;
	s8 corr;
	u8 rssi0;
	u8 rssi1;
	u8 reserved;
	u32 range;
	int clkoffset;
};

struct nsp_rttresp_debuginfo {
	struct rttresp_meas_debug rttmeasdbg[MAX_RTTREQ_MEAS];
	u8 nTotalMeas;
	u8 wlanband;
	u8 isht40;
	u8 reserved1;
	u32 reserved2;
};

#define RTTRESP_DEBUG_SIZE sizeof(struct nsp_rttresp_debuginfo)

struct nsp_rtt_resp {
	u32 range_average;
	/*Result Of Probe*/

	u32 range_stddev;
	/*Results Of Range*/

	u32 range_min;

	u32 range_max;

	u32 rtt_min;

	u32 rtt_max;

	u32 rtt_average;
	/*The mean of the samples of RTT calculated by WML(ns)*/

	u32 rtt_stddev;
	/*The standard deviation of the samples of RTT calculated by
	 * WML(ns)*/

	u8 rssi_average;
	/*The mean of the samples of the RSSI(dB)*/

	u8 rssi_stddev;
	/*The Standard Deviation of the samples of the RSSI(dB)*/

	u8 rssi_min;
	/*Min RSSI*/

	u8 rssi_max;
	/*Max RSSI*/

	u8 rtt_samples;
	/*The Number Of Samples used in the RTT Calculation*/

	u8 reserved1[3];

	u32 reserved2;

	u32 clock_delta;

	u8 debug[RTTRESP_DEBUG_SIZE];

};

struct nsp_rtt_config {
	u32 ClkCal[NUM_WLAN_BANDS];
	u8  FFTScale;
	u8  RangeScale;
	u8  ClkSpeed[NUM_WLAN_BANDS];
	u32 reserved[4];
};

struct stRttHostTS {
	u32 sec;
	u32 nsec;
};

struct nsp_rttd2h2_clkoffset {
	struct stRttHostTS tabs_h2[NUM_CLKOFFSETCAL_SAMPLES];
	u8 numsamples;
	u8 reserved[3];
};


struct nsp_rtt_clkoffset {
	u32 tdelta_d1d2[NUM_CLKOFFSETCAL_SAMPLES];
	struct stRttHostTS tabs_h1[NUM_CLKOFFSETCAL_SAMPLES];
	struct stRttHostTS tabs_h2[NUM_CLKOFFSETCAL_SAMPLES];
	u32 tabs_d1[NUM_CLKOFFSETCAL_SAMPLES];
	u32 tabs_d2[NUM_CLKOFFSETCAL_SAMPLES];
	u8  numsamples;
	u8  reserved[3];
};

#define MREQ_LEN sizeof(struct nsp_mrqst)
#define MRES_LEN sizeof(struct nsp_mresphdr)
#define CIR_RES_LEN sizeof(struct nsp_cir_resp)

enum NSP_FRAME_TYPE {
	NSP_MRQST = 1,
	NSP_CIRMRESP,
	NSP_RTTMRESP,
	NSP_SRQST,
	NSP_SRESP,
	NSP_CRQST,
	NSP_CRESP,
	NSP_WRQST,
	NSP_WRESP,
	NSP_SLRQST,
	NSP_SLRESP,
	NSP_RTTCONFIG,
	NSP_RTTCLKCAL_INFO,
};


#endif
