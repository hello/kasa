/*++
Copyright 2007 Atheros Communications

Portions: Copyright (c) 1990-2000    Microsoft Corporation All Rights Reserved

Module Name:

    ntddsd_raw.h

Abstract:

    absed on sdionmars sample.

Environment:

    user and kernel
Notes:


Revision History:


--*/

#ifndef __NTDDSD_RAW_H
#define __NTDDSD_RAW_H
 
//
// Define an Interface Guid for the mars device class.
//
// {804AC2AD-0EF6-43b8-859D-F10C1FA7E1A0}
DEFINE_GUID(GUID_DEVINTERFACE_SDRAW, 
    0x804ac2ad, 0xef6, 0x43b8, 0x85, 0x9d, 0xf1, 0xc, 0x1f, 0xa7, 0xe1, 0xa0);


DEFINE_GUID(GUID_DEVINTERFACE_SDRAW2, 
    0x301DF991, 0xAFE0, 0x45f8, 0xAB, 0x82, 0xC9, 0xD, 0xFE, 0xCA, 0x8A, 0xBB);


//
// GUID definition are required to be outside of header inclusion pragma to avoid
// error during precompiled headers.
//



#define FILE_DEVICE_SDRAW       32768

#ifndef CTL_CODE
    #define CTL_CODE(DeviceType, Function, Method, Access) ( \
        ((DeviceType)<<16) | ((Access)<<14) | ((Function)<<2) | (Method) \
    )

    #define METHOD_BUFFERED   0
    #define METHOD_IN_DIRECT  1
    #define METHOD_OUT_DIRECT 2
    #define METHOD_NEITHER    3
    #define FILE_ANY_ACCESS   0
    #define FILE_READ_ACCESS  1    // file & pipe
    #define FILE_WRITE_ACCESS 2    // file & pipe
#endif //CTL_CODE

#define IOCTL_SDRAW_GET_DRIVER_VERSION \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x800, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_SDRAW_GET_FUNCTION_NUMBER \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x801, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_SDRAW_GET_FUNCTION_FOCUS \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x802, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_SDRAW_SET_FUNCTION_FOCUS \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x803, METHOD_BUFFERED, FILE_WRITE_ACCESS)



#define IOCTL_SDRAW_GET_BUS_WIDTH \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x804, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_SDRAW_SET_BUS_WIDTH \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x805, METHOD_BUFFERED, FILE_WRITE_ACCESS)


#define IOCTL_SDRAW_GET_BUS_CLOCK \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x806, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_SDRAW_SET_BUS_CLOCK \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x807, METHOD_BUFFERED, FILE_WRITE_ACCESS)


#define IOCTL_SDRAW_GET_BLOCKLEN \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x808, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_SDRAW_SET_BLOCKLEN \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x809, METHOD_BUFFERED, FILE_WRITE_ACCESS)


#define IOCTL_SDRAW_GET_FN0_BLOCKLEN \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x80a, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_SDRAW_SET_FN0_BLOCKLEN \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x80b, METHOD_BUFFERED, FILE_WRITE_ACCESS)


#define IOCTL_SDRAW_GET_BUS_INTERFACE_CONTROL \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x80c, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_SDRAW_SET_BUS_INTERFACE_CONTROL \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x80d, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_SDRAW_GET_INT_ENABLE \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x80e, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_SDRAW_SET_INT_ENABLE \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x80f, METHOD_BUFFERED, FILE_WRITE_ACCESS)




//input:  PULONG address
//output: PUCHAR data
#define IOCTL_SDRAW_CMD52_READ \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x810, METHOD_BUFFERED, FILE_WRITE_ACCESS|FILE_READ_ACCESS)
typedef struct _SDRAW_CMD52 {
    ULONG Address;          //address to read/write
    UCHAR Data;             //Data to write
}SDRAW_CMD52, *PSDRAW_CMD52;
//input PSDRAW_CMD52
#define IOCTL_SDRAW_CMD52_WRITE \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x811, METHOD_BUFFERED, FILE_WRITE_ACCESS|FILE_READ_ACCESS)

#define IOCTL_SDRAW_SET_TRANSFER_MODE \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x812, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_SDRAW_TOGGLE_MODE \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x813, METHOD_BUFFERED, FILE_WRITE_ACCESS)

typedef struct _SDRAW_CMD53 {
    ULONG Address;          //address to read/write
    UCHAR Opcode;           // 0 - fixed address 1 - incremetal
    UCHAR Mode;             // 0 - byte mode 1 - block mode
    ULONG BlockCount;       // number of blocks to transfer
    ULONG BlockLength;      // block length of transfer
}SDRAW_CMD53, *PSDRAW_CMD53;
typedef struct _SDRAW_CMD53_WRITE {
    SDRAW_CMD53   Params;
    UCHAR Data[0];          //Data to write
}SDRAW_CMD53_WRITE, *PSDRAW_CMD53_WRITE;
//input  PSDRAW_CMD52
//output PUCHAR
#define IOCTL_SDRAW_CMD53_READ \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x815, METHOD_BUFFERED, FILE_WRITE_ACCESS|FILE_READ_ACCESS)
//input  PSDRAW_CMD52_WRITE
#define IOCTL_SDRAW_CMD53_WRITE \
            CTL_CODE( FILE_DEVICE_SDRAW, 0x816, METHOD_BUFFERED, FILE_WRITE_ACCESS|FILE_READ_ACCESS)

#endif //__NTDDSD_RAW_H
