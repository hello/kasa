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

/* pci_bus.h - contains declarations of the pci bus functions */

#ifndef __DK_PCI_BUS_H_
#define __DK_PCI_BUS_H_

#include "dk.h"

INT32 bus_module_init
(
	VOID
);

VOID bus_module_exit
(
	VOID
);

INT32 bus_dev_init
(
     void *bus_dev
);


INT32 bus_dev_exit
(
     void  *bus_dev
);

INT32 bus_cfg_read
(
     void *bus_dev,
     INT32 offset,
     INT32 size,
     INT32 *ret_val
);

INT32 bus_cfg_write
(
    void *bus_dev,
    INT32 offset,
	INT32 size,
	INT32 ret_val
);
		
#endif //__PCI_BUS_H_
