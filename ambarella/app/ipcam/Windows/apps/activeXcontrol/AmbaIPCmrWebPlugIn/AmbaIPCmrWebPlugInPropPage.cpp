// AmbaIPCmrWebPlugInPropPage.cpp : Implementation of the CAmbaIPCmrWebPlugInPropPage property page class.

#include "stdafx.h"
#include "AmbaIPCmrWebPlugIn.h"
#include "AmbaIPCmrWebPlugInPropPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(CAmbaIPCmrWebPlugInPropPage, COlePropertyPage)



// Message map

BEGIN_MESSAGE_MAP(CAmbaIPCmrWebPlugInPropPage, COlePropertyPage)
END_MESSAGE_MAP()



// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CAmbaIPCmrWebPlugInPropPage, "AMBAIPCMRWEBPL.AmbaIPCmrWebPlPropPage.1",
	0x82e2a79e, 0x4791, 0x41c4, 0x96, 0xe7, 0xbf, 0x30, 0x22, 0x4c, 0x40, 0x72)



// CAmbaIPCmrWebPlugInPropPage::CAmbaIPCmrWebPlugInPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CAmbaIPCmrWebPlugInPropPage

BOOL CAmbaIPCmrWebPlugInPropPage::CAmbaIPCmrWebPlugInPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_AMBAIPCMRWEBPLUGIN_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}



// CAmbaIPCmrWebPlugInPropPage::CAmbaIPCmrWebPlugInPropPage - Constructor

CAmbaIPCmrWebPlugInPropPage::CAmbaIPCmrWebPlugInPropPage() :
	COlePropertyPage(IDD, IDS_AMBAIPCMRWEBPLUGIN_PPG_CAPTION)
{
}



// CAmbaIPCmrWebPlugInPropPage::DoDataExchange - Moves data between page and properties

void CAmbaIPCmrWebPlugInPropPage::DoDataExchange(CDataExchange* pDX)
{
	DDP_PostProcessing(pDX);
}



// CAmbaIPCmrWebPlugInPropPage message handlers
