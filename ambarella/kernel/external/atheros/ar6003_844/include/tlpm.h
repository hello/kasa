//------------------------------------------------------------------------------
// Copyright (c) 2010 Atheros Corporation.  All rights reserved.
// 
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
// </copyright>
// 
// <summary>
// 	Wifi driver for AR6003
// </summary>
//
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================

#ifndef __TLPM_H__
#define __TLPM_H__

/* idle timeout in 16-bit value as in HOST_INTEREST hi_hci_uart_pwr_mgmt_params */
#define TLPM_DEFAULT_IDLE_TIMEOUT_MS         1000
/* hex in LSB and MSB for HCI command */
#define TLPM_DEFAULT_IDLE_TIMEOUT_LSB        0xE8
#define TLPM_DEFAULT_IDLE_TIMEOUT_MSB        0x3

/* wakeup timeout in 8-bit value as in HOST_INTEREST hi_hci_uart_pwr_mgmt_params */
#define TLPM_DEFAULT_WAKEUP_TIMEOUT_MS       10

/* default UART FC polarity is low */
#define TLPM_DEFAULT_UART_FC_POLARITY        0

#endif
