//------------------------------------------------------------------------------
// Copyright (c) 2007-2010 Atheros Corporation.  All rights reserved.
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

#ifndef __WLAN_DSET_H__
#define __WLAN_DSET_H__

typedef PREPACK struct wow_config_dset {

    A_UINT8 valid_dset;
    A_UINT8 gpio_enable;
    A_UINT16 gpio_pin;
} POSTPACK WOW_CONFIG_DSET;

#endif
