/*
 * Copyright (c) 2004-2006 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef _FWIMG_H_
#define _FWIMG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PREPACK
#ifdef __GNUC__
#define PREPACK
#else
#define PREPACK
#endif
#endif

#ifndef POSTPACK
#ifdef __GNUC__
#define POSTPACK __attribute__ ((packed))
#else
#define POSTPACK
#endif
#endif

typedef PREPACK  struct 
{
	/* All values are in network byte order (big endian) */
	A_UINT32 flags;	
#define FW_FLAG_BMI_OS			0x00000001	/* BMI OS                  */
#define FW_FLAG_BMI_FLASHER		0x00000002	/* BMI Flash application   */
#define FW_FLAG_BMI_FLASHLIB	0x00000004	/* BMI Flash part library  */
#define FW_FLAG_OS				0x00000008	/* OS                      */
#define FW_FLAG_APP				0x00000010	/* WLAN application        */
#define FW_FLAG_DSET			0x00000020	/* Data Set                */
#define FW_FLAG_DSETINDEX		0x00000040	/* Data Set Index          */
#define FW_FLAG_TCMD_APP        0x00000080  /* WLAN TCMD application   */
#define FW_FLAG_DSETFILE        0x00000100  /* Data Set as a single file */
#define FW_FLAG_END				0x80000000	/* End                     */
    A_UINT32 address;
    A_UINT32 data;
    A_UINT32 length;
} POSTPACK a_imghdr_t;

#ifdef __cplusplus
}
#endif

#endif
