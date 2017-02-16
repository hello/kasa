// ------------------------------------------------------------------
// Copyright (c) 2004-2007 Atheros Corporation.  All rights reserved.
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
// 	Wifi driver for AR6002
// </summary>
//
// ------------------------------------------------------------------
//===================================================================
// Author(s): ="Atheros"
//===================================================================


#ifndef _APB_ATHR_WLAN_MAP_H_
#define _APB_ATHR_WLAN_MAP_H_

#define RTC_SOC_BASE_ADDRESS                     0x00004000
#define RTC_WMAC_BASE_ADDRESS                    0x00005000
#define MAC_COEX_BASE_ADDRESS                    0x00006000
#define BT_COEX_BASE_ADDRESS                     0x00007000
#define MIT_BASE_ADDRESS                         0x00008000
#define WLAN_UART_BASE_ADDRESS                   0x0000c000
#define WLAN_DBG_UART_BASE_ADDRESS               0x0000d000
#define WLAN_UMBOX_BASE_ADDRESS                  0x0000e000
#define WLAN_SI_BASE_ADDRESS                     0x00010000
#define WLAN_GPIO_BASE_ADDRESS                   0x00014000
#define WLAN_MBOX_BASE_ADDRESS                   0x00018000
#define WLAN_ANALOG_INTF_BASE_ADDRESS            0x0001c000
#define WLAN_MAC_BASE_ADDRESS                    0x00020000
#define EFUSE_BASE_ADDRESS                       0x00030000
#define STEREO_BASE_ADDRESS                      0x00034000
#define CKSUM_ACC_BASE_ADDRESS                   0x00035000
#define STEREO_1_BASE_ADDRESS                    0x00036000
#define MMAC_BASE_ADDRESS                        0x00038000
#define FPGA_REG_BASE_ADDRESS                    0x00039000
#define GMAC_INTR_BASE_ADDRESS                   0x00040000
#define GMAC_RGMII_BASE_ADDRESS                  0x00040100
#define GMAC_MDIO_BASE_ADDRESS                   0x00040200
#define GMAC_CHAIN_0_RX_BASE_ADDRESS             0x00040800
#define GMAC_CHAIN_0_TX_BASE_ADDRESS             0x00040c00
#define SFMBOX_SDIO2_BASE_ADDRESS                0x00044000
#define SFMBOX_SPI2_BASE_ADDRESS                 0x00048000
#define SVD_BASE_ADDRESS                         0x00050000
#define USB_INTR_BASE_ADDRESS                    0x00054000
#define USB_CHAIN_0_RX_BASE_ADDRESS              0x00054100
#define USB_CHAIN_1_RX_BASE_ADDRESS              0x00054200
#define USB_CHAIN_2_RX_BASE_ADDRESS              0x00054300
#define USB_CHAIN_3_RX_BASE_ADDRESS              0x00054400
#define USB_CHAIN_4_RX_BASE_ADDRESS              0x00054500
#define USB_CHAIN_5_RX_BASE_ADDRESS              0x00054600
#define USB_CHAIN_0_TX_BASE_ADDRESS              0x00054700
#define USB_CHAIN_1_TX_BASE_ADDRESS              0x00054800
#define USB_CHAIN_2_TX_BASE_ADDRESS              0x00054900
#define USB_CHAIN_3_TX_BASE_ADDRESS              0x00054a00
#define USB_CHAIN_4_TX_BASE_ADDRESS              0x00054b00
#define WLAN_UART2_BASE_ADDRESS                  0x00054c00
#define WLAN_RDMA_BASE_ADDRESS                   0x00054d00
#define I2C_SLAVE_BASE_ADDRESS                   0x00054e00
#define MBOX_I2S_BASE_ADDRESS                    0x00055000
#define MBOX_I2S_1_BASE_ADDRESS                  0x00056000

#endif /* _APB_ATHR_WLAN_MAP_REG_H_ */
