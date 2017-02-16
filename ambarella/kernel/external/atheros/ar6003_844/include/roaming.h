//------------------------------------------------------------------------------
// <copyright file="roaming.h" company="Atheros">
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
// 	Wifi driver for AR6003
// </summary>
//
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================

#ifndef _ROAMING_H_
#define _ROAMING_H_

/* 
 * The signal quality could be in terms of either snr or rssi. We should 
 * have an enum for both of them. For the time being, we are going to move 
 * it to wmi.h that is shared by both host and the target, since we are 
 * repartitioning the code to the host 
 */
#define SIGNAL_QUALITY_NOISE_FLOOR        -96
#define SIGNAL_QUALITY_METRICS_NUM_MAX    2
typedef enum {
    SIGNAL_QUALITY_METRICS_SNR = 0,
    SIGNAL_QUALITY_METRICS_RSSI,
    SIGNAL_QUALITY_METRICS_ALL,
} SIGNAL_QUALITY_METRICS_TYPE;

#endif  /* _ROAMING_H_ */
