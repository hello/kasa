//------------------------------------------------------------------------------
// Copyright (c) 2004-2011 Atheros Corporation.  All rights reserved.
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

/*
 * This file contains the definitions of the host_proxy interface.  
 */

#ifndef _HOST_PROXY_IFACE_H_
#define _HOST_PROXY_IFACE_H_

/* Host proxy initializes shared memory with HOST_PROXY_INIT to 
 * indicate that it is ready to receive instruction */
#define HOST_PROXY_INIT         (1)
/* Host writes HOST_PROXY_NORMAL_BOOT to shared memory to 
 * indicate to host proxy that it should proceed to boot 
 * normally (bypassing BMI).
 */
#define HOST_PROXY_NORMAL_BOOT  (2)
/* Host writes HOST_PROXY_BMI_BOOT to shared memory to
 * indicate to host proxy that is should enable BMI and 
 * exit.  This allows a host to reprogram the on board
 * flash. 
 */
#define HOST_PROXY_BMI_BOOT     (3)

#endif /* _HOST_PROXY_IFACE_H_ */
