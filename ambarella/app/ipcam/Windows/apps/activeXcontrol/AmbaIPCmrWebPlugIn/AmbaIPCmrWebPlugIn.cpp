// AmbaIPCmrWebPlugIn.cpp : Implementation of CAmbaIPCmrWebPlugInApp and DLL registration.

#include "stdafx.h"
#include "AmbaIPCmrWebPlugIn.h"
#include "comcat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CAmbaIPCmrWebPlugInApp NEAR theApp;

const CATID CATID_SafeForScripting = 
 {0x7dd95801,0x9882,0x11cf,{0x9f,0xa9,0x00,0xaa,0x00,0x6c,0x42,0xc4}};

const CATID CATID_SafeForInitializing = 
 {0x7dd95802,0x9882,0x11cf,{0x9f,0xa9,0x00,0xaa,0x00,0x6c,0x42,0xc4}};

const GUID CLSID_SafeItem =
{0x3BCDAA6A,0x7306, 0x42FF,{0xB8,0xCF,0xBE,0x5D,0x35,0x34,0xC1,0xE4}};

const GUID CDECL BASED_CODE _ctlid =
 { 0x9f225a87, 0xec35, 0x45a7, {0x86, 0xa4, 0xfe, 0xbf, 0x81, 0x11, 0xde, 0xae} };

const GUID CDECL BASED_CODE _tlid =
		{ 0x41F4ECD8, 0x8B5B, 0x4E84, { 0x8F, 0xAE, 0x8D, 0xDF, 0x7E, 0x8A, 0x15, 0xA8 } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;






HRESULT CreateComponentCategory(CATID catid, WCHAR* catDescription)
{
 ICatRegister* pcr = NULL ;
 HRESULT hr = S_OK ;

 hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
                       NULL,
                       CLSCTX_INPROC_SERVER,
                       IID_ICatRegister,
                       (void**)&pcr);
 if (FAILED(hr))
 {
  return hr;
 }

 // Make sure the HKCR\Component Categories\{..catid...}
 // key is registered
 CATEGORYINFO catinfo;
 catinfo.catid = catid;
 catinfo.lcid = 0x0409 ; // english

 // Make sure the provided description is not too long.
 // Only copy the first 127 characters if it is
 size_t len = wcslen(catDescription);
 if (len>127)
 {
  len = 127;
 }
 wcsncpy(catinfo.szDescription, catDescription, len);
 // Make sure the description is null terminated
 catinfo.szDescription[len] = '\0';

 hr = pcr->RegisterCategories(1, &catinfo);
 pcr->Release();

 return hr;
}

// Helper function to register a CLSID as belonging to a component
// category
HRESULT RegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
{
 // Register your component categories information.
 ICatRegister* pcr = NULL ;
 HRESULT hr = S_OK ;
 hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
                       NULL,
                       CLSCTX_INPROC_SERVER,
                       IID_ICatRegister,
                       (void**)&pcr);
 if (SUCCEEDED(hr))
 {
  // Register this category as being "implemented" by
  // the class.
  CATID rgcatid[1] ;
  rgcatid[0] = catid;
  hr = pcr->RegisterClassImplCategories(clsid, 1, rgcatid);
 }

 if (pcr != NULL)
 {
  pcr->Release();
 }

 return hr;
}

// CAmbaIPCmrWebPlugInApp::InitInstance - DLL initialization

BOOL CAmbaIPCmrWebPlugInApp::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();

	if (bInit)
	{
		// TODO: Add your own module initialization code here.
	}

	return bInit;
}



// CAmbaIPCmrWebPlugInApp::ExitInstance - DLL termination

int CAmbaIPCmrWebPlugInApp::ExitInstance()
{
	// TODO: Add your own module termination code here.

	return COleControlModule::ExitInstance();
}



// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	/*AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;*/

	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);


	////////////////////////////////////////////////////////////////////////////////
	// iobjectsafety
	
	HRESULT hr;
	hr = CreateComponentCategory(CATID_SafeForInitializing,
	L"Controls safely initializable from persistent data!");
	if(FAILED(hr))
	{
	return hr;
	}

	hr = RegisterCLSIDInCategory(CLSID_SafeItem,CATID_SafeForInitializing);
	if(FAILED(hr))
	{
	return hr;
	}

	// Mark the control as safe for scripting.
	hr = CreateComponentCategory(CATID_SafeForScripting, L"Controls safely scriptable!");
	if(FAILED(hr))
	{
	return hr;
	}

	hr = RegisterCLSIDInCategory(CLSID_SafeItem,CATID_SafeForScripting);
	if(FAILED(hr))
	{
	return hr;
	}
	
	if (FAILED( CreateComponentCategory(
		CATID_SafeForScripting,
		L"Controls that are safely scriptable") ))
	return ResultFromScode(SELFREG_E_CLASS);

	if (FAILED( CreateComponentCategory(
		CATID_SafeForInitializing,
		L"Controls safely initializable from persistent data") ))
	return ResultFromScode(SELFREG_E_CLASS);

	if (FAILED( RegisterCLSIDInCategory(
		_ctlid, CATID_SafeForScripting) ))
	return ResultFromScode(SELFREG_E_CLASS);

	if (FAILED( RegisterCLSIDInCategory(
		_ctlid, CATID_SafeForInitializing) ))
	return ResultFromScode(SELFREG_E_CLASS);

	////////////////////////////////////////////////////////////////////////////////

	return NOERROR;
}



// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}
