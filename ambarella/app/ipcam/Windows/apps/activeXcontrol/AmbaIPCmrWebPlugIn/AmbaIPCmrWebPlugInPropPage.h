#pragma once

// AmbaIPCmrWebPlugInPropPage.h : Declaration of the CAmbaIPCmrWebPlugInPropPage property page class.


// CAmbaIPCmrWebPlugInPropPage : See AmbaIPCmrWebPlugInPropPage.cpp for implementation.

class CAmbaIPCmrWebPlugInPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CAmbaIPCmrWebPlugInPropPage)
	DECLARE_OLECREATE_EX(CAmbaIPCmrWebPlugInPropPage)

// Constructor
public:
	CAmbaIPCmrWebPlugInPropPage();

// Dialog Data
	enum { IDD = IDD_PROPPAGE_AMBAIPCMRWEBPLUGIN };

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	DECLARE_MESSAGE_MAP()
};

