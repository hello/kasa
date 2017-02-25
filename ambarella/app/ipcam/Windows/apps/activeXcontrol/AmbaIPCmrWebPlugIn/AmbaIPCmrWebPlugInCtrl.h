#pragma once
#include "afxwin.h"
#include <winsock2.h>
#include "afxcmn.h"


enum { CTRL_STAT,CTRL_DPTZ };

class CAmbaSocket
{
public:
	enum REQUSET_ID{
		REQ_SET_FORCEIDR = 17,
		REQ_GET_VIDEO_PORT,

		REQ_STREAM_START,
		REQ_STREAM_STOP,

		REQ_CHANGE_BR,
		REQ_CHANGE_FR,

		REQ_GET_PARAM = 100,
		REQ_SET_PARAM,
	};
	struct  Request{
		int id;
		int	info;
		int	dataSize;
	};
	struct ACK{
		int result;
		int info;
	};
	struct CONFIG{
		char* item;
		union
		{
			int value;
			double fValue;
		};
	};

	CAmbaSocket(char* hostname, int port);
	~CAmbaSocket();
	bool SendPacket(char *pbuffer, int size);
	bool ReceivePacket(char *pbuffer, int size);
	bool ParsePacket(char *pbuffer, int size, CONFIG* ppConfig);
	bool BuildPacket(char *pbuffer, int* size, CONFIG* pConfig);

private:
	short	m_port;
	SOCKET	m_socket;
	bool	m_connect;
};

// CPropertiesDialog dialog

class CPropertiesDialog : public CDialog
{
	DECLARE_DYNAMIC(CPropertiesDialog)

public:
	CPropertiesDialog(CAmbaCamGraphMngr* p,CWnd* pParent /*=NULL*/);
	virtual ~CPropertiesDialog();

// Dialog Data
	enum { IDD = IDD_PROPPAGE_AMBAIPCMRWEBPLUGIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	CAmbaCamGraphMngr*  _pMngr;
public:
	afx_msg void OnBnClickedButton1();

protected:
//	virtual void OnOK();
public:
	afx_msg void OnBnClickedButton2();
	CComboBox _transferType;
	CComboBox _decoder;
	virtual BOOL OnInitDialog();
	afx_msg void OnCbnSelchangeComboDecoder();
	afx_msg void OnCbnSelchangeComboTransfer();
public:
	afx_msg void OnBnClickedButton3();
};


// CAboutusDialog dialog

class CAboutusDialog : public CDialog
{
	DECLARE_DYNAMIC(CAboutusDialog)

public:
	CAboutusDialog(CWnd* pParent /*=NULL*/);   // standard constructor
	virtual ~CAboutusDialog();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX_AMBAIPCMRWEBPLUGIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
};
// dptz button
class CDptzButton : public CButton
{
	DECLARE_DYNAMIC(CDptzButton)
	DECLARE_MESSAGE_MAP()
public:
	CDptzButton();
	virtual ~CDptzButton();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	virtual void PreSubclassWindow();
	void SetToolTipText(LPCTSTR lpszToolTipText);
protected:
	CToolTipCtrl m_ToolTip;
};

// CVideoPageDlg dialog

class CVideoPageDlg : public CDialog
{
	DECLARE_DYNAMIC(CVideoPageDlg)
	struct DPTZ_DATA {
		int dptz_enable;
		int dptz_source;
		int dptz_zoom;
		int dptz_x;
		int dptz_y;
	};
	struct IMG_DATA {
		float shutter;
		float agc;
	};
public:
	CVideoPageDlg( CAmbaCamGraphMngr* p, CWnd* pp);
	virtual ~CVideoPageDlg();
	static HHOOK g_hKeyBoard;
	static HWND g_hWnd;
	static LRESULT CALLBACK KeyBoardProc(int nCode,WPARAM wParam,LPARAM lParam);

// Dialog Data
	enum { IDD = IDD_VIDEODIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
	CAmbaCamGraphMngr* _pMgr;
	CWnd	*_pParent;
	bool	_bFullScreen;

	bool SetDPTZ();
	bool SetPT(POINT pt);
	bool SetZoom(int factor);
	DPTZ_DATA m_dptz;
	long _pt_x;
	long _pt_y;
	bool _mouse_move;
	bool _slider_move;
	int _slider_pos;
	int _onControl;

public:
	afx_msg void OnPaint();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetup();
	afx_msg void OnDecoder();
	afx_msg void OnRecordSetting();
	afx_msg void OnRender();
	afx_msg void OnAboutus();
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg LRESULT OnGraphNotify(WPARAM inWParam, LPARAM inLParam);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	afx_msg void OnBnClickedButtonUp();
	afx_msg void OnBnClickedButtonLeft();
	afx_msg void OnBnClickedButtonRight();
	afx_msg void OnBnClickedButtonDown();
	afx_msg void OnBnClickedButtonBack();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult);
	bool ShowCtrls(int type, bool bShow);
	bool GetDPTZ(bool* enableDPTZ);
	bool GetImgData(IMG_DATA* pImgData);

	bool	_bShowStat;
	bool	_bShowDPTZ;

	CSliderCtrl _zoomSlider;
	CToolTipCtrl _tooltips;
	CDptzButton _btn_up;
	CDptzButton _btn_down;
	CDptzButton _btn_left;
	CDptzButton _btn_right;
	CDptzButton _btn_back;

	CRITICAL_SECTION m_stat_cs;
};


// AmbaIPCmrWebPlugInCtrl.h : Declaration of the CAmbaIPCmrWebPlugInCtrl ActiveX Control class.


// CAmbaIPCmrWebPlugInCtrl : See AmbaIPCmrWebPlugInCtrl.cpp for implementation.

class CAmbaIPCmrWebPlugInCtrl : public COleControl, public IAmbaCom
{
	DECLARE_DYNCREATE(CAmbaIPCmrWebPlugInCtrl)

// Constructor
public:
	CAmbaIPCmrWebPlugInCtrl();

// Overrides
public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();

	//IAmbaCom
	ULONG STDMETHODCALLTYPE AddRef()
	{
		m_ref++;
		return (ULONG)m_ref;
	};
	ULONG STDMETHODCALLTYPE Release()
	{
		m_ref--;
		if (m_ref == 0)
		{
			delete this;
			return 0;
		}
		return (ULONG)m_ref;
	};
	HRESULT STDMETHODCALLTYPE QueryInterface(const IID& iid, void **ppv) {
		if (iid == IID_IUnknown || iid == IID_IAmbaCom)
		{
			*ppv = (IAmbaCom *)this;
			((IAmbaCom *)(*ppv))->AddRef();
		}
		else
		{
			*ppv = NULL;
			return E_NOINTERFACE;
		}
		return S_OK;
	};

	STDMETHODIMP GetVersion (int* version);
	STDMETHODIMP SetConfItems();
	STDMETHODIMP LoadConfig();
	STDMETHODIMP SaveConfig();

// Implementation
protected:
	virtual ~CAmbaIPCmrWebPlugInCtrl();

	DECLARE_OLECREATE_EX(CAmbaIPCmrWebPlugInCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CAmbaIPCmrWebPlugInCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CAmbaIPCmrWebPlugInCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CAmbaIPCmrWebPlugInCtrl)		// Type name and misc status

// Message maps
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	DECLARE_DISPATCH_MAP()

// Event maps
	DECLARE_EVENT_MAP()

	afx_msg void AboutBox();
	afx_msg bool Play();
	afx_msg bool Stop();
	afx_msg void Record(bool bOp);
	afx_msg bool GetRecordStatus();
	afx_msg void SetStreamId(short id);
	afx_msg void SetHostname(const char*);
	afx_msg bool SetRecvType(short id);
	afx_msg bool ShowStat(bool bOp);
	afx_msg bool ShowDPTZ(bool bOp);
	afx_msg bool EnableDPTZ();
	afx_msg void SetStatWindowSize(unsigned short size);

	LRESULT OnGraphNotify(WPARAM inWParam, LPARAM inLParam);

private:
	CVideoPageDlg*		 	_pDialog;
	CAmbaCamGraphMngr* 		_pMngr;
	bool	_bRun;
	items_s* m_groups[1]; //for IAmbaCom
	int m_ref;

public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

class CPropertiesMenu : public CMenu
{
public:
	CPropertiesMenu(CAmbaCamGraphMngr *p, UINT resourceID);
	//DECLARE_MESSAGE_MAP()
private:
	CAmbaCamGraphMngr* _pMngr;
};





#pragma once


