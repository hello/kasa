//------------------------------------------------------------------------------
// <copyright file="dset_internal.h" company="Atheros">
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


#ifndef __DSET_INTERNAL_H__
#define __DSET_INTERNAL_H__

#ifndef ATH_TARGET
#include "athstartpack.h"
#endif

/*
 * Internal dset definitions, common for DataSet layer.
 */

#define DSET_TYPE_STANDARD      0
#define DSET_TYPE_BPATCHED      1
#define DSET_TYPE_COMPRESSED    2

/* Dataset descriptor */

typedef PREPACK struct dset_descriptor_s {
  struct dset_descriptor_s  *next;         /* List link. NULL only at the last
                                              descriptor */
  A_UINT16                   id;           /* Dset ID */
  A_UINT16                   size;         /* Dset size. */
  void                      *DataPtr;      /* Pointer to raw data for standard
                                              DataSet or pointer to original
                                              dset_descriptor for patched
                                              DataSet */
  A_UINT32                   data_type;    /* DSET_TYPE_*, above */

  void                      *AuxPtr;       /* Additional data that might
                                              needed for data_type. For
                                              example, pointer to patch
                                              Dataset descriptor for BPatch. */
} POSTPACK dset_descriptor_t;

#ifndef ATH_TARGET
#include "athendpack.h"
#endif

#endif /* __DSET_INTERNAL_H__ */
