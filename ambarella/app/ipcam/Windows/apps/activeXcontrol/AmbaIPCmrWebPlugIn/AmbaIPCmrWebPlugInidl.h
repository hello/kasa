

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


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


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __AmbaIPCmrWebPlugInidl_h__
#define __AmbaIPCmrWebPlugInidl_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef ___DAmbaIPCmrWebPlugIn_FWD_DEFINED__
#define ___DAmbaIPCmrWebPlugIn_FWD_DEFINED__
typedef interface _DAmbaIPCmrWebPlugIn _DAmbaIPCmrWebPlugIn;
#endif 	/* ___DAmbaIPCmrWebPlugIn_FWD_DEFINED__ */


#ifndef ___DAmbaIPCmrWebPlugInEvents_FWD_DEFINED__
#define ___DAmbaIPCmrWebPlugInEvents_FWD_DEFINED__
typedef interface _DAmbaIPCmrWebPlugInEvents _DAmbaIPCmrWebPlugInEvents;
#endif 	/* ___DAmbaIPCmrWebPlugInEvents_FWD_DEFINED__ */


#ifndef __AmbaIPCmrWebPlugIn_FWD_DEFINED__
#define __AmbaIPCmrWebPlugIn_FWD_DEFINED__

#ifdef __cplusplus
typedef class AmbaIPCmrWebPlugIn AmbaIPCmrWebPlugIn;
#else
typedef struct AmbaIPCmrWebPlugIn AmbaIPCmrWebPlugIn;
#endif /* __cplusplus */

#endif 	/* __AmbaIPCmrWebPlugIn_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __AmbaIPCmrWebPlugInLib_LIBRARY_DEFINED__
#define __AmbaIPCmrWebPlugInLib_LIBRARY_DEFINED__

/* library AmbaIPCmrWebPlugInLib */
/* [control][helpstring][helpfile][version][uuid] */ 


EXTERN_C const IID LIBID_AmbaIPCmrWebPlugInLib;

#ifndef ___DAmbaIPCmrWebPlugIn_DISPINTERFACE_DEFINED__
#define ___DAmbaIPCmrWebPlugIn_DISPINTERFACE_DEFINED__

/* dispinterface _DAmbaIPCmrWebPlugIn */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__DAmbaIPCmrWebPlugIn;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("20F8B004-C094-4BF9-81B1-FB7711B52799")
    _DAmbaIPCmrWebPlugIn : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _DAmbaIPCmrWebPlugInVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _DAmbaIPCmrWebPlugIn * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _DAmbaIPCmrWebPlugIn * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _DAmbaIPCmrWebPlugIn * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _DAmbaIPCmrWebPlugIn * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _DAmbaIPCmrWebPlugIn * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _DAmbaIPCmrWebPlugIn * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _DAmbaIPCmrWebPlugIn * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _DAmbaIPCmrWebPlugInVtbl;

    interface _DAmbaIPCmrWebPlugIn
    {
        CONST_VTBL struct _DAmbaIPCmrWebPlugInVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _DAmbaIPCmrWebPlugIn_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _DAmbaIPCmrWebPlugIn_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _DAmbaIPCmrWebPlugIn_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _DAmbaIPCmrWebPlugIn_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _DAmbaIPCmrWebPlugIn_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _DAmbaIPCmrWebPlugIn_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _DAmbaIPCmrWebPlugIn_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___DAmbaIPCmrWebPlugIn_DISPINTERFACE_DEFINED__ */


#ifndef ___DAmbaIPCmrWebPlugInEvents_DISPINTERFACE_DEFINED__
#define ___DAmbaIPCmrWebPlugInEvents_DISPINTERFACE_DEFINED__

/* dispinterface _DAmbaIPCmrWebPlugInEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__DAmbaIPCmrWebPlugInEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("B1419B58-2ACE-413C-98C3-A477F460C5A4")
    _DAmbaIPCmrWebPlugInEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _DAmbaIPCmrWebPlugInEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _DAmbaIPCmrWebPlugInEvents * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _DAmbaIPCmrWebPlugInEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _DAmbaIPCmrWebPlugInEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _DAmbaIPCmrWebPlugInEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _DAmbaIPCmrWebPlugInEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _DAmbaIPCmrWebPlugInEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _DAmbaIPCmrWebPlugInEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _DAmbaIPCmrWebPlugInEventsVtbl;

    interface _DAmbaIPCmrWebPlugInEvents
    {
        CONST_VTBL struct _DAmbaIPCmrWebPlugInEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _DAmbaIPCmrWebPlugInEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _DAmbaIPCmrWebPlugInEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _DAmbaIPCmrWebPlugInEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _DAmbaIPCmrWebPlugInEvents_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _DAmbaIPCmrWebPlugInEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _DAmbaIPCmrWebPlugInEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _DAmbaIPCmrWebPlugInEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___DAmbaIPCmrWebPlugInEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_AmbaIPCmrWebPlugIn;

#ifdef __cplusplus

class DECLSPEC_UUID("3BCDAA6A-7306-42FF-B8CF-BE5D3534C1E4")
AmbaIPCmrWebPlugIn;
#endif
#endif /* __AmbaIPCmrWebPlugInLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


