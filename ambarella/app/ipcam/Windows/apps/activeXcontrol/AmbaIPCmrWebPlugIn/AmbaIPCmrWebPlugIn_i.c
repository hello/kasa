

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Fri Jun 24 15:18:31 2011
 */
/* Compiler settings for .\AmbaIPCmrWebPlugIn.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, LIBID_AmbaIPCmrWebPlugInLib,0x41F4ECD8,0x8B5B,0x4E84,0x8F,0xAE,0x8D,0xDF,0x7E,0x8A,0x15,0xA8);


MIDL_DEFINE_GUID(IID, DIID__DAmbaIPCmrWebPlugIn,0x20F8B004,0xC094,0x4BF9,0x81,0xB1,0xFB,0x77,0x11,0xB5,0x27,0x99);


MIDL_DEFINE_GUID(IID, DIID__DAmbaIPCmrWebPlugInEvents,0xB1419B58,0x2ACE,0x413C,0x98,0xC3,0xA4,0x77,0xF4,0x60,0xC5,0xA4);


MIDL_DEFINE_GUID(CLSID, CLSID_AmbaIPCmrWebPlugIn,0x3BCDAA6A,0x7306,0x42FF,0xB8,0xCF,0xBE,0x5D,0x35,0x34,0xC1,0xE4);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



