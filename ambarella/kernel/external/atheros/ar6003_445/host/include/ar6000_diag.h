//------------------------------------------------------------------------------
// <copyright file="ar6000_diag.h" company="Atheros">
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

#ifndef AR6000_DIAG_H_
#define AR6000_DIAG_H_


A_STATUS
ar6000_ReadRegDiag(HIF_DEVICE *hifDevice, A_UINT32 *address, A_UINT32 *data);

A_STATUS
ar6000_WriteRegDiag(HIF_DEVICE *hifDevice, A_UINT32 *address, A_UINT32 *data);

A_STATUS
ar6000_ReadDataDiag(HIF_DEVICE *hifDevice, A_UINT32 address,
                    A_UCHAR *data, A_UINT32 length);

A_STATUS
ar6000_WriteDataDiag(HIF_DEVICE *hifDevice, A_UINT32 address,
                     A_UCHAR *data, A_UINT32 length);

A_STATUS
ar6k_ReadTargetRegister(HIF_DEVICE *hifDevice, int regsel, A_UINT32 *regval);

void
ar6k_FetchTargetRegs(HIF_DEVICE *hifDevice, A_UINT32 *targregs);

#endif /*AR6000_DIAG_H_*/
