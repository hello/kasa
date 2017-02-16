/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */
#ifndef _REX_H
#define _REX_H

typedef void * rex_tcb_type;
typedef unsigned int rex_sigs_type;
void rex_clr_sigs(rex_tcb_type tcb, rex_sigs_type sigs);
rex_sigs_type rex_wait(rex_sigs_type sigs);
void rex_set_sigs(rex_tcb_type tcb, rex_sigs_type sigs);
rex_sigs_type rex_get_sigs(rex_tcb_type tcb);

#endif
