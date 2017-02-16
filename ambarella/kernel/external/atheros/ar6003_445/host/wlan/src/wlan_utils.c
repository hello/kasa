//------------------------------------------------------------------------------
// <copyright file="wlan_utils.c" company="Atheros">
//    Copyright (c) 2004-2010 Atheros Corporation.  All rights reserved.
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
//------------------------------------------------------------------------------
//==============================================================================
// This module implements frequently used wlan utilies
//
// Author(s): ="Atheros"
//==============================================================================
#ifdef WIN_MOBILE7
#include <ntddk.h>
#endif

#include <a_config.h>
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>

/*
 * converts ieee channel number to frequency
 */
A_UINT16
wlan_ieee2freq(int chan)
{
    if (chan == 14) {
        return 2484;
    }
    if (chan < 14) {    /* 0-13 */
        return (2407 + (chan*5));
    }
    if (chan < 27) {    /* 15-26 */
        return (2512 + ((chan-15)*20));
    }
    return (5000 + (chan*5));
}

/*
 * Converts MHz frequency to IEEE channel number.
 */
A_UINT32
wlan_freq2ieee(A_UINT16 freq)
{
    if (freq == 2484)
        return 14;
    if (freq < 2484)
        return (freq - 2407) / 5;
    if (freq < 5000)
        return 15 + ((freq - 2512) / 20);
    return (freq - 5000) / 5;
}
