// Copyright (c) 2010 Atheros Communications Inc.
// All rights reserved.
// 
//
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
//
//

#ifndef __ANWIIO_H
#define __ANWIIO_H

#include "anwi.h"
#include "anwiclient.h"

ULONG32 anwiRegRead(ULONG32 offset,pAnwiAddrDesc pRegMap);
VOID anwiRegWrite(ULONG32 offset,ULONG32 data,pAnwiAddrDesc pRegMap);

ULONG32 anwiCfgRead(ULONG32 offset,ULONG32 length,pAnwiClientInfo pClient);
VOID anwiCfgWrite(ULONG32 offset,ULONG32 length,ULONG32 data,pAnwiClientInfo pClient);

USHORT anwiIORead ( ULONG32 offset, ULONG32 length);
VOID anwiIOWrite ( ULONG32 offset, ULONG32 length, USHORT data);

#endif
