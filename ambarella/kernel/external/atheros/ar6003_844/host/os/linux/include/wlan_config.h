//------------------------------------------------------------------------------
// Copyright (c) 2004-2010 Atheros Corporation.  All rights reserved.
// 
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
//
//------------------------------------------------------------------------------
//==============================================================================
// This file contains the tunable configuration items for the WLAN module
//
// Author(s): ="Atheros"
//==============================================================================
#ifndef _HOST_WLAN_CONFIG_H_
#define _HOST_WLAN_CONFIG_H_

/* Include definitions here that can be used to tune the WLAN module behavior.
 * Different customers can tune the behavior as per their needs, here. 
 */

/* This configuration item when defined will consider the barker preamble 
 * mentioned in the ERP IE of the beacons from the AP to determine the short 
 * preamble support sent in the (Re)Assoc request frames.
 */
#define WLAN_CONFIG_DONOT_IGNORE_BARKER_IN_ERP 0

/* This config item when defined will not send the power module state transition
 * failure events that happen during scan, to the host. 
 */
#define WLAN_CONFIG_IGNORE_POWER_SAVE_FAIL_EVENT_DURING_SCAN 0

/* This config item disables sending a DisAssoc frame prior to sending a 
 * DeAuth frame.
 */
#define WLAN_CONFIG_NO_DISASSOC_UPON_DEAUTH 0

/*
 * This configuration item enable/disable keepalive support.
 * Keepalive support: In the absence of any data traffic to AP, null 
 * frames will be sent to the AP at periodic interval, to keep the association
 * active. This configuration item defines the periodic interval.
 * Use value of zero to disable keepalive support
 * Default: 60 seconds
 */
#define WLAN_CONFIG_KEEP_ALIVE_INTERVAL 60 

/*
 * This configuration item sets the value of disconnect timeout
 * Firmware delays sending the disconnec event to the host for this
 * timeout after is gets disconnected from the current AP.
 * If the firmware successly roams within the disconnect timeout
 * it sends a new connect event
 */
#ifdef ANDROID_ENV
#define WLAN_CONFIG_DISCONNECT_TIMEOUT 3
#else
#define WLAN_CONFIG_DISCONNECT_TIMEOUT 10
#endif /* ANDROID_ENV */ 

/*
 * This configuration item enable BT clock sharing support
 * 1 - Enable
 * 0 - Disable (Default)
 */
#define WLAN_CONFIG_BT_SHARING          0

/*
 * This configuration item sets WIFI OFF policy
 * 0 - CUT_POWER
 * 1 - DEEP_SLEEP (Default)
 */
#define WLAN_CONFIG_WLAN_OFF                1

/*
 * This configuration item disables 11n support. 
 * 0 - Enable
 * 1 - Disable
 */
#define WLAN_CONFIG_DISABLE_11N         0

/*
 * This configuration item sets suspend policy
 * 0 - CUT_POWER (Default)
 * 1 - DEEP_SLEEP
 * 2 - WoW
 * 3 - CUT_POWER if BT OFF (clock sharing designs only)
 */
#define WLAN_CONFIG_PM_SUSPEND              0

/*
 * This configuration item sets suspend policy to use if PM_SUSPEND is
 * set to WoW and device is not connected at the time of suspend
 * 0 - CUT_POWER (Default)
 * 1 - DEEP_SLEEP
 * 2 - WoW
 * 3 - CUT_POWER if BT OFF (clock sharing designs only)
 */
#define WLAN_CONFIG_PM_WOW2                 1

/* 
 * Define GPIO number for WoW in your platform other than zero 
 * Wake lock will be called when GPIO asserted. 
 */
#define PLAT_WOW_GPIO_PIN                  0

/*
 * This configuration item for the WOW patterns in AP mode
 * 0 - Wake up host only if any unicast IP, EAPOL-like and ARP, broadcast dhcp and ARP packets. (default)
 * 1 - Wake up host if any unicast/broadcast/multicast packets
 */
#define WLAN_CONFIG_SIMPLE_WOW_AP_MODE      0

/*
 * set to 1 to let STA do 2.4GHz band scan on 1st scan after 
 * either insmod or wmiconfig --wlan enable
 *
 * set to 0 to disable this feature
 */
#define WLAN_CONFIG_FIRST_SCAN_2G_ONLY      0

/* 
 *   1: to disable sending ps-poll in TIM interrupt
 *   0: to send one ps-poll (the default)
 *    
 */
#define WLAN_CONFIG_PSPOLL_NUM              0

/*
 *  0: to use the default (NORMAL_DTIM)
 *  1: to use IGNORE_DTIM
 *  2: to use NORMAL_DTIM
 *  3: to use STICK_DTIM
 *  4: to use AUTO_DTIM
 */
#define WLAN_CONFIG_DTIM_POLICY             0


/* 
 * Define the GPIO number for WLAN CHIP_PWD PIN other than zero
 * Only use when you define plat_setup_power as plat_setup_power_stub
 */

#define PLAT_WLAN_CHIP_PWD_PIN              1

/*
 * Platform specific function to power ON/OFF AR6000 with CHIP_PWD PIN
 * and enable/disable SDIO card detection
 *
 * Either implement wlan CHIP_PWD PIN and power GPIO control into 
 * plat_setup_power_stub(..) place holder in ar6000_pm.c 
 * or redefine plat_setup_power(..) into your export funciton. 
 */

#if PLAT_WLAN_CHIP_PWD_PIN
extern void plat_setup_power_stub(struct ar6_softc *ar, int on, int detect); 
#define plat_setup_power(ar, on, detect) plat_setup_power_stub(ar, on, detect)
#elif defined(CONFIG_MMC_MSM) && defined(CONFIG_ARCH_MSM7X27) && defined(CONFIG_MSM_SOC_REV_A) 
extern void msm7x27a_wifi_power(bool on);
#define plat_setup_power(ar, on, detect) msm7x27a_wifi_power(on) 
#elif defined(CONFIG_MMC_MSM) && defined(CONFIG_ARCH_MSM9615)
#define MSM9615_WLAN_CHIP_PWD_PIN               21
#define MSM9615_WLAN_PM_ENABLE_PIN              2
extern int msm9615_wifi_power(AR_SOFTC_T *ar, int on);
#define plat_setup_power(ar, on, detect) msm9615_wifi_power(ar, on)
#else
#define plat_setup_power(ar, on, detect) /* define as your function */
#endif 

#endif /* _HOST_WLAN_CONFIG_H_ */
