//------------------------------------------------------------------------------
// <copyright file="miscdrv.h" company="Atheros">
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
// Author(s): ="Atheros"
//==============================================================================
#ifndef _MISCDRV_H
#define _MISCDRV_H

A_UINT32 ar6kRev2Array[][128]   = {
                                    {0xFFFF, 0xFFFF},      // No Patches
                               };

#define CFG_REV2_ITEMS                0     // no patches so far
#define AR6K_RESET_ADDR               0x4000
#define AR6K_RESET_VAL                0x100

#define EEPROM_SZ                     768
#define EEPROM_WAIT_LIMIT             4

#endif

