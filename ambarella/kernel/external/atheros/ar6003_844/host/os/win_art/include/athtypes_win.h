/*
 * Copyright ?2000-2002 Atheros Communications, Inc.,  All Rights Reserved.
 *
 * wlantype.h - Basic datatypes for each platform
 * $Id: //depot/sw/releases/olca3.1-RC/host/os/win_art/include/athtypes_win.h#3 $
 */

#ifndef _ATHTYPES_WIN_H_
#define _ATHTYPES_WIN_H_

#define MULTI_RATE_RETRY_ENABLE 1
#define UPSD 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>

/* No OS's have this define */
typedef const unsigned char     cu8;
typedef union wlanMACAddr WLAN_MACADDR;

/*
 * Following conditional compiles need to be correct. This is just a sample.
 * It also needs to be corrected for bit ordering for the CPU.
 */
#ifdef WIN32
#undef ARCH_BIG_ENDIAN

typedef char                    A_CHAR;
typedef unsigned char           A_UCHAR;
typedef A_CHAR                  A_INT8;
typedef A_UCHAR                 A_UINT8;
typedef short                   A_INT16;
typedef unsigned short          A_UINT16;
typedef long int                A_INT32;
typedef unsigned long int       A_UINT32;
typedef unsigned long int       A_UINT;

typedef unsigned char           u8;
typedef unsigned short          u16;
typedef unsigned int            u32;

typedef A_UCHAR                 A_BOOL;
typedef void *                  OS_DEVICE_ID;

typedef __int64                 A_INT64;
typedef unsigned __int64        A_UINT64;
#define A_LL                    "I64"

typedef A_UINT64                A_LONGSTATS;

typedef unsigned __int64        u64;
typedef __int64                 s64;
typedef void*                   A_ATH_TIMER;
#endif /* WIN32 */

typedef A_INT8                  A_RSSI;
typedef A_INT32                 A_RSSI32;

#define RSSI_DUMMY_MARKER       0x127

/* Endianness byte swapping functions */

#define A_swab16(x) \
        ((A_UINT16)( \
                (((A_UINT16)(x) & (A_UINT16)0x00ffU) << 8) | \
                (((A_UINT16)(x) & (A_UINT16)0xff00U) >> 8) ))
#define A_swab32(x) \
        ((A_UINT32)( \
                (((A_UINT32)(x) & (A_UINT32)0x000000ffUL) << 24) | \
                (((A_UINT32)(x) & (A_UINT32)0x0000ff00UL) <<  8) | \
                (((A_UINT32)(x) & (A_UINT32)0x00ff0000UL) >>  8) | \
                (((A_UINT32)(x) & (A_UINT32)0xff000000UL) >> 24) ))

#define A_swab64(x) \
        ((A_UINT64)( \
                (A_UINT64)(((A_UINT64)(x) & (A_UINT64)0x00000000000000ff) << 56) | \
                (A_UINT64)(((A_UINT64)(x) & (A_UINT64)0x000000000000ff00) << 40) | \
                (A_UINT64)(((A_UINT64)(x) & (A_UINT64)0x0000000000ff0000) << 24) | \
                (A_UINT64)(((A_UINT64)(x) & (A_UINT64)0x00000000ff000000) <<  8) | \
                (A_UINT64)(((A_UINT64)(x) & (A_UINT64)0x000000ff00000000) >>  8) | \
                (A_UINT64)(((A_UINT64)(x) & (A_UINT64)0x0000ff0000000000) >> 24) | \
                (A_UINT64)(((A_UINT64)(x) & (A_UINT64)0x00ff000000000000) >> 40) | \
                (A_UINT64)(((A_UINT64)(x) & (A_UINT64)0xff00000000000000) >> 56) ))

#ifdef ARCH_BIG_ENDIAN

#define cpu2le64(x) A_swab64((x))
#define le2cpu64(x) A_swab64((x))
#define cpu2le32(x) A_swab32((x))
#define le2cpu32(x) A_swab32((x))
#define cpu2le16(x) A_swab16((x))
#define le2cpu16(x) A_swab16((x))
#define cpu2be64(x) ((A_UINT64)(x))
#define be2cpu64(x) ((A_UINT64)(x))
#define cpu2be32(x) ((A_UINT32)(x))
#define be2cpu32(x) ((A_UINT32)(x))
#define cpu2be16(x) ((A_UINT16)(x))
#define be2cpu16(x) ((A_UINT16)(x))

#else /* Little_Endian */

#define cpu2le64(x) ((A_UINT64)(x))
#define le2cpu64(x) ((A_UINT64)(x))
#define cpu2le32(x) ((A_UINT32)(x))
#define le2cpu32(x) ((A_UINT32)(x))
#define cpu2le16(x) ((A_UINT16)(x))
#define le2cpu16(x) ((A_UINT16)(x))
#define cpu2be64(x) A_swab64((x))
#define be2cpu64(x) A_swab64((x))
#define cpu2be32(x) A_swab32((x))
#define be2cpu32(x) A_swab32((x))
#define cpu2be16(x) A_swab16((x))
#define be2cpu16(x) A_swab16((x))

#endif /* Endianness */

/*
 * Define some useful macros
 */

#define A_MAX(x, y)         (((x) > (y)) ? (x) : (y))
#define A_MIN(x, y)         (((x) < (y)) ? (x) : (y))
#define A_ABS(x)            (((x) >= 0) ? (x) : (-(x)))
#define A_LPF_RSSI(x, y, len) ((x != RSSI_DUMMY_MARKER) ? (((x) * ((len) - 1) + (y)) / (len)) : (y))
#define A_LPF_RATE(x, y, len) ((x) ? (((x) * ((len) - 1) + (y)) / (len)) : (y))
#define A_ROUNDUP(x, y)     ((((x) + ((y) - 1)) / (y)) * (y))
#define A_ROUNDUP_PAD(x, y) (A_ROUNDUP(x, y) - (x))
#define MAKE_BOOL(x)        ((x) ? TRUE : FALSE)
#define A_TOLOWER(c)        (((c) >= 'A' && (c) <= 'Z') ? ((c)-'A'+'a') : (c))
#define A_TOUPPER(c)        (((c) >= 'a' && (c) <= 'z') ? ((c)-'a'+'A') : (c))
#define TU_TO_MS(x)         ((x) * 1024 / 1000)
#define TU_TO_US(x)         ((x) << 10)
#define MS_TO_TU(x)         ((x) * 1000 / 1024)
#define KHZ_TO_MHZ(x)       ((x) / 1000)
#define MHZ_TO_KHZ(x)       ((x) * 1000)

#ifdef DEBUG
#define A_UNUSED_VAR(x)
#else
#define A_UNUSED_VAR(x)     (x)=(x)
#endif

//#define A_STATUS_API        A_STATUS __cdecl

/*
 * The following macros are used for manipulating extended-precision values.
 * A_EP_RND returns the value rounded to normal precision, and A_EP_MUL
 * is used to multiply up from normal to extended precision.
 */
#define A_EP_RND(x, mul)   ((((x)%(mul)) >= ((mul)/2)) ? ((x) + ((mul) - 1)) / (mul) : (x)/(mul))
#define A_EP_MUL(x, mul)   ((x) * (mul))
/*
 * The following macros define the number of bits and masks
 */
#define NUM_BITS_NBL 4
#define NUM_BITS_BYTE 8
#define NBL_MASK   0x0f
/*
 * Enumerated types
 */
typedef enum {
    DISCONNECT_NOW,
    DISCONNECT_DELAYED
} DISCONNECT_ENUM;

typedef enum {
    REMOVE_BSS,
    DONT_REMOVE_BSS
} REMOVE_BSS_ENUM;

typedef enum {
    SEND_DEAUTH,
    DONT_SEND_DEAUTH
} SEND_DEAUTH_ENUM;

typedef enum {
    RESET_INC_CTR,
    RESET_DONT_INC_CTR
} RESET_CTR_ENUM;

typedef enum {
    DO_WAIT,
    DONT_WAIT
} DRAIN_ENUM;

typedef enum {
    DO_COMPLETE,
    DONT_COMPLETE
} COMPLETION_ENUM;

typedef enum {
    DO_CLEAR_TX_Q,
    DONT_CLEAR_TX_Q
} CLEAR_TX_Q_ENUM;

typedef enum {
    ANTENNA_CONTROLLABLE,
    ANTENNA_FIXED_A,
    ANTENNA_FIXED_B,
    ANTENNA_DUMMY_MAX
} ANTENNA_CONTROL;

typedef enum {
    ATH_DEV_TYPE_UNDEFINED,
    ATH_DEV_TYPE_CARDBUS,
    ATH_DEV_TYPE_PCI,
    ATH_DEV_TYPE_MINIPCI,
    ATH_DEV_TYPE_AP,
    ATH_DEV_TYPE_MAX
} ATH_DEV_TYPE;

#define ARRAY_NUM_ENTRIES(a) (sizeof(a)/sizeof(*(a)))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _ATHTYPES_WIN_H_ */
