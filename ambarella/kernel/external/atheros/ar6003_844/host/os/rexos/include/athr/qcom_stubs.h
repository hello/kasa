/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef _QCOM_STUBS_H
#define _QCOM_STUBS_H

typedef unsigned char byte;
typedef unsigned char boolean;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef short int16;
typedef unsigned int uint32;
typedef unsigned int dword;
typedef unsigned long long uint64;
typedef int sint31;

#if 0
#define PACKED __attribute__ ((packed))
#else
#define PACKED 
#endif

typedef unsigned char * dsm_item_type;
typedef int net_wlan_8021x_info_u_type;

typedef int q_link_type;

int dsm_pullup(dsm_item_type **pkt, char *data, int len);
void dsm_free_packet(dsm_item_type **pkt);
void wlan_trp_adp_cmd_complete_ind(void);

void set_max_tx_packets(unsigned int max);

#endif
