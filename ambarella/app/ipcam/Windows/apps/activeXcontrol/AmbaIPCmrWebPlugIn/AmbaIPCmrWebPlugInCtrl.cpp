// AmbaIPCmrWebPlugInCtrl.cpp : Implementation of the CAmbaIPCmrWebPlugInCtrl ActiveX Control class.


#include "stdafx.h"
#include "uuids.h"
#include "streams.h"
#include <initguid.h>
#include "Ambacamgraphmngr.h"

#include "AmbaIPCmrWebPlugIn.h"
#include "AmbaIPCmrWebPlugInCtrl.h"
#include "AmbaIPCmrWebPlugInPropPage.h"
#include "Resource.h"
#include "winuser.h"

#define CONFIG_FILE "IPCmrWebPlugIn.ini"
#define VERSION 0x0100000E
#define ENCODER_SERVER_PORT	20000
#define IMG_SERVER_PORT	20002
#define STAT_REFRESH_TIME 1000
#define _CRTDBG_MAP_ALLOC
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(CAmbaIPCmrWebPlugInCtrl, COleControl)



// Message map

BEGIN_MESSAGE_MAP(CAmbaIPCmrWebPlugInCtrl, COleControl)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()



// Dispatch map

BEGIN_DISPATCH_MAP(CAmbaIPCmrWebPlugInCtrl, COleControl)
	DISP_FUNCTION_ID(CAmbaIPCmrWebPlugInCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION_ID(CAmbaIPCmrWebPlugInCtrl, "Play", 0, Play, VT_I1, VTS_NONE)
	DISP_FUNCTION_ID(CAmbaIPCmrWebPlugInCtrl, "Stop", 1, Stop, VT_I1, VTS_NONE)
	DISP_FUNCTION_ID(CAmbaIPCmrWebPlugInCtrl, "Record", 2, Record, VT_EMPTY, VTS_I1)
	DISP_FUNCTION_ID(CAmbaIPCmrWebPlugInCtrl, "GetRecordStatus", 3, GetRecordStatus, VT_I1, VTS_NONE)
	DISP_FUNCTION_ID(CAmbaIPCmrWebPlugInCtrl, "SetStreamId",4, SetStreamId, VT_EMPTY,VTS_I2)
	DISP_FUNCTION_ID(CAmbaIPCmrWebPlugInCtrl, "SetHostname",5, SetHostname, VT_EMPTY,VTS_BSTR)
	DISP_FUNCTION_ID(CAmbaIPCmrWebPlugInCtrl, "SetRecvType",6, SetRecvType, VT_I1,VTS_I2)
	DISP_FUNCTION_ID(CAmbaIPCmrWebPlugInCtrl, "ShowStat",7, ShowStat, VT_I1, VTS_I1)
	DISP_FUNCTION_ID(CAmbaIPCmrWebPlugInCtrl, "ShowDPTZ",8, ShowDPTZ, VT_I1, VTS_I1)
	DISP_FUNCTION_ID(CAmbaIPCmrWebPlugInCtrl, "EnableDPTZ",9, EnableDPTZ, VT_I1, VTS_NONE)
	DISP_FUNCTION_ID(CAmbaIPCmrWebPlugInCtrl, "SetStatWindowSize",10, SetStatWindowSize, VT_EMPTY,VTS_UI2)
END_DISPATCH_MAP()



// Event map

BEGIN_EVENT_MAP(CAmbaIPCmrWebPlugInCtrl, COleControl)
END_EVENT_MAP()



// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CAmbaIPCmrWebPlugInCtrl, 1)
	PROPPAGEID(CAmbaIPCmrWebPlugInPropPage::guid)
END_PROPPAGEIDS(CAmbaIPCmrWebPlugInCtrl)



// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CAmbaIPCmrWebPlugInCtrl, "AMBAIPCMRWEBPLUG.AmbaIPCmrWebPlugCtrl.1",
	0x3bcdaa6a, 0x7306, 0x42ff, 0xb8, 0xcf, 0xbe, 0x5d, 0x35, 0x34, 0xc1, 0xe4)



// Type library ID and version

IMPLEMENT_OLETYPELIB(CAmbaIPCmrWebPlugInCtrl, _tlid, _wVerMajor, _wVerMinor)



// Interface IDs

const IID BASED_CODE IID_DAmbaIPCmrWebPlugIn =
		{ 0x20F8B004, 0xC094, 0x4BF9, { 0x81, 0xB1, 0xFB, 0x77, 0x11, 0xB5, 0x27, 0x99 } };
const IID BASED_CODE IID_DAmbaIPCmrWebPlugInEvents =
		{ 0xB1419B58, 0x2ACE, 0x413C, { 0x98, 0xC3, 0xA4, 0x77, 0xF4, 0x60, 0xC5, 0xA4 } };



// Control type information

static const DWORD BASED_CODE _dwAmbaIPCmrWebPlugInOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CAmbaIPCmrWebPlugInCtrl, IDS_AMBAIPCMRWEBPLUGIN, _dwAmbaIPCmrWebPlugInOleMisc)

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
 AFX_MANAGE_STATE(AfxGetStaticModuleState());
 return AfxDllGetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllCanUnloadNow

//extern "C"
STDAPI DllCanUnloadNow(void)
{
 AFX_MANAGE_STATE(AfxGetStaticModuleState());
 return AfxDllCanUnloadNow();
}

// CAmbaIPCmrWebPlugInCtrl::CAmbaIPCmrWebPlugInCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CAmbaIPCmrWebPlugInCtrl

BOOL CAmbaIPCmrWebPlugInCtrl::CAmbaIPCmrWebPlugInCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_AMBAIPCMRWEBPLUGIN,
			IDB_AMBAIPCMRWEBPLUGIN,
			afxRegApartmentThreading,
			_dwAmbaIPCmrWebPlugInOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}



// CAmbaIPCmrWebPlugInCtrl::CAmbaIPCmrWebPlugInCtrl - Constructor

CAmbaIPCmrWebPlugInCtrl::CAmbaIPCmrWebPlugInCtrl():_bRun(false),m_ref(0)
{
	InitializeIIDs(&IID_DAmbaIPCmrWebPlugIn, &IID_DAmbaIPCmrWebPlugInEvents);
	// TODO: Initialize your control's instance data here.
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	_pMngr = new CAmbaCamGraphMngr();
	_pDialog = new CVideoPageDlg(_pMngr, this);
	SetConfItems();
	LoadConfig();
}



// CAmbaIPCmrWebPlugInCtrl::~CAmbaIPCmrWebPlugInCtrl - Destructor

CAmbaIPCmrWebPlugInCtrl::~CAmbaIPCmrWebPlugInCtrl()
{
	// TODO: Cleanup your control's instance data here.
	//_pMngr->stop();
	if (_pMngr->_defconf)
	{
		char del_filename[64];
		char* config_dir = getenv("ProgramFiles");
		if (config_dir) {
			sprintf_s(del_filename, "%s\\Ambarella\\%s",config_dir,CONFIG_FILE);
			remove(del_filename);
		}
	}
	else
	{
		SaveConfig();
	}
	delete m_groups[0];
	delete _pDialog;
	delete _pMngr;

}



// CAmbaIPCmrWebPlugInCtrl::OnDraw - Drawing function

void CAmbaIPCmrWebPlugInCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	if (!pdc)
		return;

	// TODO: Replace the following code with your own drawing code.
	pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
	//pdc->Ellipse(rcBounds);
}

// CAmbaIPCmrWebPlugInCtrl::DoPropExchange - Persistence support

void CAmbaIPCmrWebPlugInCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.
}



// CAmbaIPCmrWebPlugInCtrl::OnResetState - Reset control to default state

void CAmbaIPCmrWebPlugInCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}



// CAmbaIPCmrWebPlugInCtrl::AboutBox - Display an "About" box to the user

void CAmbaIPCmrWebPlugInCtrl::AboutBox()
{
	CAboutusDialog dlgAbout(this);
	dlgAbout.DoModal();
}

int CAmbaIPCmrWebPlugInCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	//if (COleControl::OnCreate(lpCreateStruct) == -1)
	//	return -1;

	// TODO:  Add your specialized creation code here
	_pDialog->Create(IDD_VIDEODIALOG, this);
	_pDialog->ShowWindow(SW_SHOW);
	return 0;

}

// CAmbaIPCmrWebPlugInCtrl message handlers

void CAmbaIPCmrWebPlugInCtrl::OnSize(UINT nType, int cx, int cy)
{
	COleControl::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	RECT rc;
	GetClientRect(&rc);
	_pDialog->MoveWindow(&rc, 0);
}

bool CAmbaIPCmrWebPlugInCtrl::Play()
{
		OAFilterState state;
		if ( _pMngr->getstate(&state) && (state == State_Running))
		{
			return true;
		}

		if (_pMngr->prepare() &&
			_pMngr->connect() &&
			_pMngr->SetVideoWnd(_pDialog->GetSafeHwnd(),_pDialog->_bShowDPTZ) &&
			_pMngr->run())
		{
			_pDialog->UpdateData(false);
			if (_pDialog->_bShowStat)
			{
				::SetTimer(_pDialog->GetSafeHwnd(),1,STAT_REFRESH_TIME,NULL);
				::SetTimer(_pDialog->GetSafeHwnd(),3,STAT_REFRESH_TIME,NULL);
			}
			return true;
		}
		else
		{
			return false;
		}
}


bool CAmbaIPCmrWebPlugInCtrl::Stop()
{
	OAFilterState state;
	if(_pMngr->getstate(&state) && state == State_Stopped)
	{
		return true;
	}
	if(_pMngr->stop())
	{
		if (_pDialog->_bShowStat)
		{
			::KillTimer(_pDialog->GetSafeHwnd(),1);
			::KillTimer(_pDialog->GetSafeHwnd(),3);
		}
		return true;
	}
	else
	{
		return false;
	}
}

void CAmbaIPCmrWebPlugInCtrl::Record(bool bOp)
{
	_pMngr->record(bOp);
}

bool CAmbaIPCmrWebPlugInCtrl::GetRecordStatus()
{
	return  _pMngr->getrecord();
}

void CAmbaIPCmrWebPlugInCtrl::SetStreamId(short id)
{
	_pMngr->_streamId = id;
}

bool CAmbaIPCmrWebPlugInCtrl::SetRecvType(short recv_type) //0-tcp ,1-rtsp
{
	return _pMngr->setrecvType(recv_type);
}

void  CAmbaIPCmrWebPlugInCtrl::SetHostname(const char* hostname)
{
	strncpy(_pMngr->_hostname, hostname, strlen(hostname)+1);
}

bool CAmbaIPCmrWebPlugInCtrl::ShowStat(bool bOp)
{
	return _pDialog->ShowCtrls(CTRL_STAT,bOp);
}

bool CAmbaIPCmrWebPlugInCtrl::ShowDPTZ(bool bOp)
{
	return _pDialog->ShowCtrls(CTRL_DPTZ,bOp);
}

bool CAmbaIPCmrWebPlugInCtrl::EnableDPTZ()
{
	bool enableDPTZ = false;//enableDPTZ = true;
	if (_pDialog->GetDPTZ(&enableDPTZ) && enableDPTZ == true)
	{
		ShowDPTZ(true);
		return true;
	} else
	{
		return false;
	}
}

void CAmbaIPCmrWebPlugInCtrl::SetStatWindowSize(unsigned short size)
{
	_pMngr->_statWindowSize = size;
}

STDMETHODIMP CAmbaIPCmrWebPlugInCtrl::GetVersion (int* version)
{
	*version = VERSION;
	return NO_ERROR;
}

STDMETHODIMP CAmbaIPCmrWebPlugInCtrl::SetConfItems()
{
	m_groups[0] = new items_s("Setup");
	m_groups[0]->add_item("decode_type");
	return NO_ERROR;
}

STDMETHODIMP CAmbaIPCmrWebPlugInCtrl::LoadConfig()
{
	char config_file[64];
	char* config_dir = getenv("ProgramFiles");
	if (config_dir) {
		sprintf_s(config_file, "%s\\Ambarella\\%s",config_dir,CONFIG_FILE);
		FILE *fd = fopen(config_file, "r");
		if (fd) {
			char line[128];
			char group[32]={0};
			char item[32];
			char value[64];
			items_s* one_item;
			while (fgets(line, 128, fd)) {
				if (1 == sscanf(line,"\[%[a-zA-Z]\]",group)){
					continue;
				}
				if (2 == sscanf(line, "%s%*[ \t=]%s", item, value)){
					if (0 == strcmp(group, m_groups[0]->m_item)){
						one_item = m_groups[0];
						if ( (one_item = one_item->next_item) &&
							(0 == strcmp(item,one_item->m_item)) ){
								switch (atoi(value)) {
									case 0:
										_pMngr->SetCameraDecorderType(CDT_AVC);
										break;
									default:
										_pMngr->SetCameraDecorderType(CDT_FFDSHOW);
										break;
								}
						}
					}
				}
			}
			fclose(fd);
		}
	}
	return NO_ERROR;
}

STDMETHODIMP CAmbaIPCmrWebPlugInCtrl::SaveConfig()
{
	char config_file[64];
	char* config_dir = getenv("ProgramFiles");
	if (config_dir) {
		sprintf_s(config_file, "%s\\Ambarella\\%s",config_dir,CONFIG_FILE);
		//lock
		HANDLE hMutex = CreateMutex(
							NULL,                        // default security descriptor
							FALSE,                       // mutex not owned
							TEXT("IPCmrWebPlugInMutex"));  // object name

		if (GetLastError() == ERROR_ALREADY_EXISTS)
			hMutex =  OpenMutex(
				MUTEX_ALL_ACCESS,            // request full access
				FALSE,                       // handle not inheritable
				TEXT("IPCmrWebPlugInMutex"));
		if (hMutex) {
			DWORD dwWaitResult = WaitForSingleObject(
				hMutex,    // handle to mutex
				1000);
			 switch (dwWaitResult) {
				// The thread got ownership of the mutex
				case WAIT_OBJECT_0:
					break;
				default://not save config
					CloseHandle(hMutex);
					return NO_ERROR;
			}
		}

		FILE *fd = fopen(config_file, "w+");
		if (fd) {
			char line[64];
			items_s* one_item;

			sprintf(line,"[%s]\n",m_groups[0]->m_item);
			fputs(line,fd);
			one_item = m_groups[0];
			if (NULL != (one_item = one_item->next_item)) {
				sprintf(line,"%s = %d\n",one_item->m_item,_pMngr->GetCameraDecorderType());
				fputs(line,fd);
			}
			fputs("\n",fd);
			fclose(fd);
		}
		if (hMutex) {
			ReleaseMutex(hMutex);
			CloseHandle(hMutex);
		}
	}
	return NO_ERROR;
}

//CAmbaSocket: communicate to encoder_server for Digital PTZ
CAmbaSocket::CAmbaSocket(char* hostname, int port)
			:m_connect(false)
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET) {
		int errCode = WSAGetLastError();
		return;
	}
	sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = inet_addr(hostname);
	client.sin_port = htons(port);

	if (0 == connect(m_socket, (SOCKADDR*)&client, sizeof(client))) {
		m_connect = true;
	}
}
CAmbaSocket::~CAmbaSocket()
{
	if (m_connect)
	{
		closesocket(m_socket);
		m_connect = false;
	}
}
bool CAmbaSocket::SendPacket(char *pbuffer, int size)
{
	if (!m_connect || size < 0)
	{
		return false;
	}
	int iResult = send(m_socket, pbuffer, size, 0);
	if (iResult != size)
	{
		return false;
	}
	return true;
}

bool CAmbaSocket::ReceivePacket(char *pbuffer, int size)
{
	if (!m_connect || size < 0)
	{
		return false;
	}
	int iResult = recv(m_socket, pbuffer, size, 0);
	if (iResult != size)
	{
		return false;
	}
	return true;
}

bool CAmbaSocket::ParsePacket(char *pbuffer, int size, CONFIG* ppConfig)
{
	char *p = pbuffer;
	char *line = pbuffer;
	char *buffer_end = pbuffer + size;
	CONFIG* pConfig = NULL;
	int parse_ret;
	while (p < buffer_end)
	{
		if (*p++ == '\n')
		{
			char item[32] = {0};
			char value[32] = {0};
			parse_ret = sscanf(line,"%[a-zA-Z_]%*[ \t=]%[0-9.]",item, value);
			if (parse_ret != 2)
			{
				return false;
			}

			pConfig = ppConfig;
			while (pConfig->item != NULL)
			{
				if ( 0 == strcmp(pConfig->item,item) )
				{
					if (pConfig->value == 0)
					{
						pConfig->fValue = atof(value);
					}
					else
					{
						pConfig->value = atoi(value);
					}
					break;
				}
				pConfig++;
			}
			if (pConfig->item == NULL)
			{
				return false; //not found item
			}
			line = p;
		}
	}
	return true;
}

bool CAmbaSocket::BuildPacket(char *pbuffer, int* size, CONFIG* pConfig)
{
	memset(pbuffer,0,*size);
	size_t ret_size = 0;
	char *buf_end = pbuffer + *size;
	char *line = pbuffer;
	while (pConfig->item != NULL)
	{
		sprintf(line,"%s = %d\n",pConfig->item,pConfig->value);
		ret_size = strlen(line);
		line += ret_size;
		if (line >= buf_end)
		{
			return false;
		}
		pConfig++;
	}
	*size = (int)(line - pbuffer);
	return true;
}
// CDptzButton
IMPLEMENT_DYNAMIC(CDptzButton, CButton)

BEGIN_MESSAGE_MAP(CDptzButton, CButton)
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

//CToolTipCtrl CDptzButton::m_ToolTip;

CDptzButton::CDptzButton() : CButton()
{

}

CDptzButton::~CDptzButton()
{
}

void CDptzButton::OnMouseMove(UINT nFlags, CPoint point)
{
	//   TODO: Add your message handler code here and/or call default
	if (m_ToolTip.m_hWnd != NULL)
	{
		MSG msg;
		 msg.hwnd = m_hWnd;
		 msg.message = WM_MOUSEMOVE;
		 msg.wParam = LOWORD(point.x);
		 msg.lParam = LOWORD(point.y);
		 msg.time = 0;
		 msg.pt.x = LOWORD(point.y);
		 msg.pt.y = HIWORD(point.y);
         m_ToolTip.RelayEvent(&msg);
	}
	CButton::OnMouseMove(nFlags, point);
}

void CDptzButton::PreSubclassWindow()
{
	if(!m_ToolTip.GetSafeHwnd())
		m_ToolTip.Create(this);
	m_ToolTip.AddTool(this);
	CButton::PreSubclassWindow();
}

void CDptzButton::SetToolTipText(LPCTSTR lpszToolTipText)
{
	m_ToolTip.UpdateTipText(lpszToolTipText,this);
}

// CVideoPageDlg dialog
HHOOK CVideoPageDlg::g_hKeyBoard = NULL;
HWND CVideoPageDlg::g_hWnd = 0;
LRESULT CALLBACK CVideoPageDlg::KeyBoardProc(
	int nCode,
	WPARAM wParam,
	LPARAM lParam
)
{
	if(lParam & 0x80000000)
	{
		switch (wParam)
		{
		case VK_UP:
			::SendMessage(g_hWnd,WM_COMMAND,IDC_BUTTON_UP,NULL);
			return 1;
		case VK_DOWN:
			::SendMessage(g_hWnd,WM_COMMAND,IDC_BUTTON_DOWN,NULL);
			return 1;
		case VK_LEFT:
			::SendMessage(g_hWnd,WM_COMMAND,IDC_BUTTON_LEFT,NULL);
			return 1;
		case VK_RIGHT:
			::SendMessage(g_hWnd,WM_COMMAND,IDC_BUTTON_RIGHT,NULL);
			return 1;
		case VK_NEXT:
			::SendMessage(g_hWnd,WM_MOUSEWHEEL, (1+~WHEEL_DELTA)<<16,0xFFFFFFFF);
			return 1;
		case VK_PRIOR:
			::SendMessage(g_hWnd,WM_MOUSEWHEEL, WHEEL_DELTA<<16,0xFFFFFFFF);
			return 1;
		default:
			break;
		}
	}
	return CallNextHookEx(g_hKeyBoard,nCode,wParam,lParam);
}

IMPLEMENT_DYNAMIC(CVideoPageDlg, CDialog)
CVideoPageDlg::CVideoPageDlg
	(
		CAmbaCamGraphMngr* p,
		CWnd* pp
	) : CDialog(CVideoPageDlg::IDD, NULL)
	,_pMgr(p)
	,_pParent(pp)
	,_bFullScreen(false)
	,_bShowStat(true)
	,_bShowDPTZ(false)
	,_pt_x(-1)
	,_pt_y(-1)
	,_mouse_move(false)
	,_slider_move(false)
	,_slider_pos(0)
	,_onControl(0)
{
	m_dptz.dptz_enable = 0;
	m_dptz.dptz_source = 0;
	m_dptz.dptz_zoom = 1;
	m_dptz.dptz_x = 0;
	m_dptz.dptz_y = 0;
	InitializeCriticalSection(&m_stat_cs);
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR) {
		return;
	}
}
CVideoPageDlg::~CVideoPageDlg()
{
	if (g_hKeyBoard)
	{
		UnhookWindowsHookEx(g_hKeyBoard);
	}
	DeleteCriticalSection(&m_stat_cs);
}

void CVideoPageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER1, _zoomSlider);
	DDX_Control(pDX, IDC_BUTTON_UP, _btn_up);
	DDX_Control(pDX, IDC_BUTTON_DOWN, _btn_down);
	DDX_Control(pDX, IDC_BUTTON_LEFT, _btn_left);
	DDX_Control(pDX, IDC_BUTTON_RIGHT, _btn_right);
	DDX_Control(pDX, IDC_BUTTON_BACK, _btn_back);
}


BEGIN_MESSAGE_MAP(CVideoPageDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_CTLCOLOR()
	ON_WM_TIMER()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_SETTING_SETTING,   OnSetup)
	ON_COMMAND(ID_SETTING_RECORDSETTING,   OnRecordSetting)
	ON_COMMAND(ID_SETTING_DECODER,  OnDecoder)
	ON_COMMAND(ID_SETTING_RENDERPROPERTY, OnRender)
	ON_COMMAND(ID_SETTING_ABOUTUS, OnAboutus)
	ON_WM_LBUTTONDBLCLK()
	ON_MESSAGE(WM_GRAPHNOTIFY, OnGraphNotify)
	ON_WM_KEYUP()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_BUTTON_UP, &CVideoPageDlg::OnBnClickedButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_LEFT, &CVideoPageDlg::OnBnClickedButtonLeft)
	ON_BN_CLICKED(IDC_BUTTON_RIGHT, &CVideoPageDlg::OnBnClickedButtonRight)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, &CVideoPageDlg::OnBnClickedButtonDown)
	ON_BN_CLICKED(IDC_BUTTON_BACK, &CVideoPageDlg::OnBnClickedButtonBack)
	ON_WM_VSCROLL()
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
END_MESSAGE_MAP()


// CVideoPageDlg message handlers

void CVideoPageDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CDialog::OnPaint() for painting messages
	CRect   rect;
	GetClientRect(rect);
	dc.FillSolidRect(rect,RGB(0xfa,0xfc,0xff));
	CDialog::OnPaint();
}

HBRUSH CVideoPageDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	int nCtlID = pWnd->GetDlgCtrlID();
	if ( nCtlID == IDC_STATIC1 || nCtlID == IDC_STATIC2 || nCtlID == IDC_STATIC3
		|| nCtlID == IDC_STATIC4 || nCtlID == IDC_STATIC5
		|| nCtlID == IDC_SLIDER1)
	{
		pDC->SetBkColor(RGB(0xfa,0xfc,0xff));
		hbr = ::CreateSolidBrush(RGB(0xfa,0xfc,0xff));
	}
	return hbr;
}

void CVideoPageDlg::OnTimer(UINT nIDEvent)
{
	if (nIDEvent == 1)
	{
		LPTSTR strFramerate = new TCHAR[16];
		LPTSTR strBitrate = new TCHAR[16];
		LPTSTR strPTS = new TCHAR[16];
		ENC_STAT stat = {0.0,0,0};
		if (_pMgr->getstat(&stat))
		{
			sprintf(strFramerate, TEXT("%.2f"),stat.avg_fps);
			sprintf(strBitrate, TEXT("%d"),stat.avg_bitrate);
			sprintf(strPTS,TEXT("%d"),stat.avg_pts);
			::SetDlgItemText(m_hWnd,IDC_EDIT1,strBitrate);
			::SetDlgItemText(m_hWnd,IDC_EDIT2,strFramerate);
			::SetDlgItemText(m_hWnd,IDC_EDIT3,strPTS);
		}
		delete[] strFramerate;
		delete[] strBitrate;
		delete[] strPTS;
	}
	else if (nIDEvent == 2)
	{
		if (_mouse_move)
		{
			POINT pt;
			RECT rect;
			GetCursorPos(&pt);
			GetWindowRect(&rect);
			if (!PtInRect(&rect,pt))
			{
				::KillTimer(this->GetSafeHwnd(),2);
				_mouse_move = false;
			}
			else
			{
				if (!SetPT(pt))
				{
					::KillTimer(this->GetSafeHwnd(),2);
					_mouse_move = false;
					MessageBox("Set digital PAN/TILT failed!", "Error");
				}
			}
		}
	}
	else if (nIDEvent == 3)
	{
		LPTSTR strAgc = new TCHAR[16];
		LPTSTR strShutter = new TCHAR[16];

		IMG_DATA stat = {0,0};
		if (!GetImgData(&stat))
		{
			//MessageBox("Get shutter and agc failed", "Error");
			::KillTimer(this->GetSafeHwnd(),3);
			MessageBox("Get shutter and agc failed", "Error");
		}
		else
		{
			sprintf(strAgc, TEXT("%.2f"),stat.agc);
			sprintf(strShutter, TEXT("%.2f"),stat.shutter);
			::SetDlgItemText(m_hWnd,IDC_EDIT4,strAgc);
			::SetDlgItemText(m_hWnd,IDC_EDIT5,strShutter);
		}
		delete[] strAgc;
		delete[] strShutter;
	}
}

bool CVideoPageDlg::ShowCtrls(int type,bool bShow)
{
	if (type == CTRL_STAT) // statistic controls
	{
		if (bShow == _bShowStat)
		{
			return false;
		}
		if (bShow)
		{
			GetDlgItem(IDC_STATIC1)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_STATIC2)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_STATIC3)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_STATIC4)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_STATIC5)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_EDIT1)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_EDIT2)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_EDIT3)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_EDIT4)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_EDIT5)->ShowWindow(SW_SHOW);
			_bShowStat = true;
			::SetTimer(this->GetSafeHwnd(),1,STAT_REFRESH_TIME,NULL);
			::SetTimer(this->GetSafeHwnd(),3,STAT_REFRESH_TIME,NULL);
		}
		else
		{
			GetDlgItem(IDC_STATIC1)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_STATIC2)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_STATIC3)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_STATIC4)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_STATIC5)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_EDIT1)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_EDIT2)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_EDIT3)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_EDIT4)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_EDIT5)->ShowWindow(SW_HIDE);
			_bShowStat = false;
			::KillTimer(this->GetSafeHwnd(),1);
			::KillTimer(this->GetSafeHwnd(),3);
		}
	}
	else if (type == CTRL_DPTZ) //Digital PTZ controls
	{
		if (bShow == _bShowDPTZ)
		{
			return false;
		}
		if (bShow)
		{
			GetDlgItem(IDC_BUTTON_LEFT)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_BUTTON_RIGHT)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_BUTTON_UP)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_BUTTON_DOWN)->ShowWindow(SW_SHOW);
			GetDlgItem(IDC_BUTTON_BACK)->ShowWindow(SW_SHOW);
			_zoomSlider.SetPos(110 - m_dptz.dptz_zoom * 10);
			GetDlgItem(IDC_SLIDER1)->ShowWindow(SW_SHOW);
			_slider_pos = _zoomSlider.GetPos();
			_bShowDPTZ = true;
		}
		else
		{
			GetDlgItem(IDC_BUTTON_LEFT)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_BUTTON_RIGHT)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_BUTTON_UP)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_BUTTON_DOWN)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_BUTTON_BACK)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_SLIDER1)->ShowWindow(SW_HIDE);
			_bShowDPTZ = false;
		}
	}
	if (_bShowDPTZ)
	{
		GetDlgItem(IDC_STATIC_FOCUS)->SetFocus();
	}
	return true;
}

bool CVideoPageDlg::SetDPTZ()
{
	if (!_bShowDPTZ)
	{
		return false;
	}
	bool ret = false;
	CAmbaSocket my_socket(_pMgr->_hostname, ENCODER_SERVER_PORT);
	char send_buf[128];
	int buf_size = 128;
	CAmbaSocket::CONFIG ptz_config[5] = {
					{"dptz_source",m_dptz.dptz_source},
					{"dptz_zoom",m_dptz.dptz_zoom},
					{"dptz_x", m_dptz.dptz_x},
					{"dptz_y", m_dptz.dptz_y},
					{NULL,0}
					};
	if (!my_socket.BuildPacket(send_buf,&buf_size,ptz_config))
	{
		return false;
	}
	const char* section_name = "DPTZ";
	CAmbaSocket::Request request = {CAmbaSocket::REQ_SET_PARAM,buf_size,(int)strlen(section_name)};
	CAmbaSocket::ACK ack = {0,0};

	if (my_socket.SendPacket((char *)&request, sizeof(request)) &&
		my_socket.SendPacket((char *)section_name, request.dataSize))
	{
		my_socket.ReceivePacket((char *)&ack, sizeof(ack));
		if (ack.result == 0 && ack.info == 0)
		{
			if (my_socket.SendPacket(send_buf,buf_size))
			{
				if(my_socket.ReceivePacket((char *)&ack, sizeof(ack)))
				{
					if (ack.result == 0 && ack.info == 0)
					{
						ret = true;
					}
				}
			}
		}
	}
	return ret;
}
bool CVideoPageDlg::GetDPTZ(bool* enableDPTZ)
{
	bool ret = false;
	CAmbaSocket my_socket(_pMgr->_hostname, ENCODER_SERVER_PORT);
	const char* section_name = "DPTZ";
	CAmbaSocket::Request request = {CAmbaSocket::REQ_GET_PARAM,_pMgr->_streamId,(int)strlen(section_name)};
	CAmbaSocket::ACK ack = {0,0};
	if (my_socket.SendPacket((char *)&request, sizeof(request)) &&
		my_socket.SendPacket((char *)section_name, request.dataSize))
	{
		my_socket.ReceivePacket((char *)&ack, sizeof(ack));
		if (ack.result == 0 && ack.info > 0)
		{
			char* recv_buf = new char[ack.info];
			if (my_socket.ReceivePacket(recv_buf, ack.info))
			{
				CAmbaSocket::CONFIG ptz_config[6] = {
					{"dptz_enable",-1},
					{"dptz_source", -1},
					{"dptz_zoom",-1},
					{"dptz_x", -1},
					{"dptz_y", -1},
					{NULL,0}
				};
				if (my_socket.ParsePacket(recv_buf, ack.info,ptz_config))
				{
					m_dptz.dptz_enable = ptz_config[0].value;
					m_dptz.dptz_source = ptz_config[1].value;
					m_dptz.dptz_zoom = ptz_config[2].value;
					m_dptz.dptz_x = ptz_config[3].value;
					m_dptz.dptz_y = ptz_config[4].value;
					if (ptz_config[0].value == 1)
					{
						*enableDPTZ = true;
					}
					ret = true;
				}
			}
			delete[] recv_buf;
		}
	}

	return ret;
}

bool CVideoPageDlg::SetPT(POINT pt)
{
	long move_x = _pt_x - pt.x;
	long move_y = _pt_y - pt.y;
	long width = 0;
	long height = 0;
	bool ret = false;
	if (_pMgr->getvideoSize(&width, &height))
	{
		if (move_x != 0 || move_y != 0 )
		{
			m_dptz.dptz_x = move_x * 1000 / width;
			m_dptz.dptz_y = move_y * 1000 / height;
			_pt_x = pt.x;
			_pt_y = pt.y;
			if (SetDPTZ())
			{
				ret = true;
			}
		}
		else
		{
			ret = true;
		}
	}
	return ret;
}

bool CVideoPageDlg::SetZoom(int factor)
{
	m_dptz.dptz_x = 0;
	m_dptz.dptz_y = 0;
	int _old_dptz_zoom = m_dptz.dptz_zoom;
	m_dptz.dptz_zoom = factor;
	if (SetDPTZ())
	{
		if ( 0 == m_dptz.dptz_zoom )
		{
			m_dptz.dptz_zoom = 1;
			_zoomSlider.SetPos(100);
		}
		return true;
	} else
	{
		m_dptz.dptz_zoom = _old_dptz_zoom;
		_zoomSlider.SetPos( 110 - m_dptz.dptz_zoom * 10 );
		return false;
	}
}

bool CVideoPageDlg::GetImgData(IMG_DATA* pImgData)
{
	bool ret = false;
	CAmbaSocket my_socket(_pMgr->_hostname, IMG_SERVER_PORT);
	const char* section_name = "STAT";
	CAmbaSocket::Request request = {CAmbaSocket::REQ_GET_PARAM,0,(int)strlen(section_name)};
	CAmbaSocket::ACK ack = {0,0};
	if (my_socket.SendPacket((char *)&request, sizeof(request)) &&
		my_socket.SendPacket((char *)section_name, request.dataSize))
	{
		my_socket.ReceivePacket((char *)&ack, sizeof(ack));
		if (ack.result == 0 && ack.info > 0)
		{
			char* recv_buf = new char[ack.info];
			if (my_socket.ReceivePacket(recv_buf, ack.info))
			{
				CAmbaSocket::CONFIG stat[3] = {
					{"agc",-1},
					{"shutter", -1},
					{NULL,0}
				};
				if (my_socket.ParsePacket(recv_buf, ack.info,stat))
				{
					EnterCriticalSection(&m_stat_cs);
					pImgData->agc = 1.0f * stat[0].value / 1024;
					if ( stat[1].value > 0 && stat[1].value <= 512000000)
					{
						pImgData->shutter = 512.0f * 1000000 / stat[1].value;
						ret = true;
					}
					LeaveCriticalSection(&m_stat_cs);
				}
			}
			delete[] recv_buf;
		}
	}

	return ret;
}

void CVideoPageDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// TODO: Add your message handler code here
	CPropertiesMenu   pMenu1(_pMgr, IDR_MENU1);

	CMenu*   pMenu   =   pMenu1.GetSubMenu(0);
	pMenu->TrackPopupMenu(TPM_RIGHTBUTTON,point.x,point.y,this);
}

void CVideoPageDlg::OnSetup()
{
	CPropertiesDialog dlgProp(_pMgr,this);
	dlgProp.DoModal();
}

void CVideoPageDlg::OnRecordSetting()
{
	_pMgr->PopSourceFilterPropPage(this->GetSafeHwnd());
}

void CVideoPageDlg::OnDecoder()
{
	_pMgr->PopDecorderFilterPropPage(this->GetParent()->GetParent()->GetSafeHwnd());
}

void CVideoPageDlg::OnRender()
{
	_pMgr->PopRenderFilterPropPage(this->GetSafeHwnd());
}

void CVideoPageDlg::OnAboutus()
{
	CAboutusDialog dlgAbout(this);
	dlgAbout.DoModal();
}

BOOL CVideoPageDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	RECT rc;
	_pParent->GetClientRect(&rc);
	MoveWindow(&rc, 0);
	_pMgr->SetWnd((OAHWND)GetSafeHwnd(), rc);
	_pMgr->SetNotifyWindow(this->GetSafeHwnd());
	_zoomSlider.SetRange(0,100);
	_zoomSlider.SetTicFreq(10);

	_tooltips.Create(GetDlgItem(IDC_SLIDER1));

	CBitmap Bitmap;
	Bitmap.LoadBitmap(IDB_BITMAP_UP);
	HBITMAP hBitmap=(HBITMAP)Bitmap.Detach();
	_btn_up.SetBitmap(hBitmap);
	_btn_up.SetToolTipText("move up 10%");

	Bitmap.LoadBitmap(IDB_BITMAP_DOWN);
	hBitmap=(HBITMAP)Bitmap.Detach();
	_btn_down.SetBitmap(hBitmap);
	_btn_down.SetToolTipText("move down 10%");

	Bitmap.LoadBitmap(IDB_BITMAP_LEFT);
	hBitmap=(HBITMAP)Bitmap.Detach();
	_btn_left.SetBitmap(hBitmap);
	_btn_left.SetToolTipText("mvoe left 10%");

	Bitmap.LoadBitmap(IDB_BITMAP_RIGHT);
	hBitmap=(HBITMAP)Bitmap.Detach();
	_btn_right.SetBitmap(hBitmap);
	_btn_right.SetToolTipText("move right 10%");

	Bitmap.LoadBitmap(IDB_BITMAP_BACK);
	hBitmap=(HBITMAP)Bitmap.Detach();
	_btn_back.SetBitmap(hBitmap);
	_btn_back.SetToolTipText("back to center");

	g_hKeyBoard=SetWindowsHookEx(WH_KEYBOARD,KeyBoardProc,NULL,GetCurrentThreadId());
	g_hWnd = GetSafeHwnd();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CVideoPageDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	//CDialog::OnLButtonDblClk(nFlags, point);
	if (!_bFullScreen)
	{
		POINT pt;
		GetCursorPos(&pt);
		long video_left, video_top, video_right, video_bottom;
		if (_pMgr->getvideoPos(&video_left, &video_top,
							&video_right, &video_bottom))
		{
			if (pt.x >= video_left && pt.x <= video_right &&
				pt.y >= video_top && pt.y <= video_bottom)
			{
				TRACE("enter full screen!!!!\n");
				_pMgr->SetVideoWnd(NULL);
				CWnd::ShowWindow(SW_HIDE);
				_bFullScreen = true;
			}
		}
	}
	else
	{
		TRACE("return to original size!!!!\n");
		CWnd::ShowWindow(SW_RESTORE);
		_pMgr->SetVideoWnd(this->GetSafeHwnd(),_bShowDPTZ);
		_bFullScreen = false;
	}
}

// Deal with the graph events
LRESULT CVideoPageDlg::OnGraphNotify(WPARAM inWParam, LPARAM inLParam)
{
	IMediaEventEx * pEvent = NULL;
	if (_pMgr->_pGraph && (pEvent = _pMgr->GetEventHandle()))
	{
		LONG   eventCode = 0, eventParam1 = 0, eventParam2 = 0;
		while (SUCCEEDED(pEvent->GetEvent(&eventCode, &eventParam1, &eventParam2, 0)))
		{
			// Spin through the events
			pEvent->FreeEventParams(eventCode, eventParam1, eventParam2);
			CString strMsg;
			switch (eventCode)
			{
			case EC_COMPLETE:
				//strMsg.Format("EC_COMPLETE:%d.%d\n",eventParam1,eventParam2);
				//MessageBox(strMsg,"Notice");
				//break;
			case EC_USERABORT:
				//MessageBox("EC_USERABORT","Notice");
				//break;
			case EC_ERRORABORT:
				if(!_pMgr->stop())
				{
					return -1;
				}
				MessageBox("Encoding is stopped!","Notice");
				//strMsg.Format("EC_ERRORABORT:%d.%d\n",eventParam1,eventParam2);
				//MessageBox(strMsg,"Notice");
				break;
			case EC_VIDEO_SIZE_CHANGED:
				_pMgr->SetVideoWnd(this->GetSafeHwnd(),_bShowDPTZ);
				break;
			default:
				break;
			}
		}
	}
	return 0;
}

void CVideoPageDlg::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_ESCAPE)
	{
		if (_bFullScreen)
		{
			TRACE("return to original size!!!!\n");
			CWnd::ShowWindow(SW_RESTORE);
			_pMgr->SetVideoWnd(this->GetSafeHwnd(),_bShowDPTZ);
			_bFullScreen = FALSE;
		}
		return;
	}
}


void CVideoPageDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ( nFlags == 0 && _bShowDPTZ && !_bFullScreen)
	{
		POINT pt;
		GetCursorPos(&pt);
		if (_mouse_move)
		{
			::KillTimer(GetSafeHwnd(),2);
			_mouse_move = false;
			if (!SetPT(pt))
			{
				MessageBox("Set digital PAN/TILT failed!", "Error");
			}
		}
		else if (_slider_move)
		{
			//::KillTimer(GetSafeHwnd(),3);
			_slider_move = false;
			int curPos = _zoomSlider.GetPos();
			if (curPos != _slider_pos)
			{
				_slider_pos = curPos;
				if (!SetZoom( (110 - curPos)/10 ))
				{
					MessageBox("Set digital zoom failed","Error");
				}
			}
		}
		else
		{
			do {
				RECT rect;
				GetDlgItem(IDC_BUTTON_LEFT)->GetWindowRect(&rect);
				if (_onControl ==IDC_BUTTON_LEFT && PtInRect(&rect,pt))
				{
					OnBnClickedButtonLeft();
					break;
				}
				GetDlgItem(IDC_BUTTON_UP)->GetWindowRect(&rect);
				if (_onControl ==IDC_BUTTON_UP && PtInRect(&rect,pt))
				{
					OnBnClickedButtonUp();
					break;
				}
				GetDlgItem(IDC_BUTTON_RIGHT)->GetWindowRect(&rect);
				if (_onControl ==IDC_BUTTON_RIGHT && PtInRect(&rect,pt))
				{
					OnBnClickedButtonRight();
					break;
				}
				GetDlgItem(IDC_BUTTON_DOWN)->GetWindowRect(&rect);
				if (_onControl ==IDC_BUTTON_DOWN && PtInRect(&rect,pt))
				{
					OnBnClickedButtonDown();
					break;
				}
			} while (0);
			_onControl = 0;
		}

	}
	CDialog::OnLButtonUp(nFlags, point);
}

void CVideoPageDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	if ((nFlags & MK_LBUTTON) && _bShowDPTZ && !_bFullScreen)
	{
		GetDlgItem(IDC_STATIC_FOCUS)->SetFocus();
		_pt_x = -1;
		_pt_y = -1;
		_onControl = 0;
		POINT pt;
		GetCursorPos(&pt);
		RECT rect;
		GetDlgItem(IDC_BUTTON_LEFT)->GetWindowRect(&rect);
		if (PtInRect(&rect,pt))
		{
			_onControl = IDC_BUTTON_LEFT;
			return;
		}
		GetDlgItem(IDC_BUTTON_UP)->GetWindowRect(&rect);
		if (PtInRect(&rect,pt))
		{
			_onControl = IDC_BUTTON_UP;
			return;
		}
		GetDlgItem(IDC_BUTTON_RIGHT)->GetWindowRect(&rect);
		if (PtInRect(&rect,pt))
		{
			_onControl = IDC_BUTTON_RIGHT;
			return;
		}
		GetDlgItem(IDC_BUTTON_DOWN)->GetWindowRect(&rect);
		if (PtInRect(&rect,pt))
		{
			_onControl = IDC_BUTTON_DOWN;
			return;
		}
		RECT rect1;
		RECT rect2;
		_zoomSlider.GetWindowRect(&rect1);
		_zoomSlider.GetThumbRect(&rect2);

		rect.top = rect1.top + rect2.top;
		rect.left = rect1.left + rect2.left;
		rect.bottom = rect1.top + rect2.bottom;
		rect.right =  rect1.left + rect2.right;
		if (PtInRect(&rect,pt))
		{
			_onControl = IDC_SLIDER1;
			return;
		}
		else if (PtInRect(&rect1,pt))
		{
			return;
		}

		long video_left, video_top, video_right, video_bottom;
		if (_pMgr->getvideoPos(&video_left, &video_top,
							&video_right, &video_bottom))
		{
			if (pt.x >= video_left && pt.x <= video_right &&
				pt.y >= video_top && pt.y <= video_bottom)
			{
				_pt_x = pt.x;
				_pt_y = pt.y;
			}
		}
	}
	else
	{
		CDialog::OnLButtonDown(nFlags, point);
	}
}

BOOL CVideoPageDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (nFlags == 0 && _bShowDPTZ && !_bFullScreen)
	{
		long video_left , video_top, video_right, video_bottom;
		if (!_pMgr->getvideoPos(&video_left, &video_top,
							&video_right, &video_bottom))
		{
			return false;
		}
		if ( (pt.x == -1 && pt.y == -1)
			|| (pt.x >= video_left && pt.x <= video_right &&
			pt.y >= video_top && pt.y <= video_bottom)
			)
		{
			int zoom_factor = m_dptz.dptz_zoom;
			if (zDelta > 0)
			{
				zoom_factor++;
				if (zoom_factor > 11)
				{
					zoom_factor = 11;
				}
			}
			else if (zDelta < 0)
			{
				zoom_factor--;
				if (zoom_factor < 1)
				{
					zoom_factor = 1;
				}
			}
			if(zDelta != 0 && !SetZoom(zoom_factor))
			{
				MessageBox("Set digital zoom failed!","Error");
			}
			else
			{
				_zoomSlider.SetPos(110 - m_dptz.dptz_zoom * 10);
				return true;
			}
		}
	}
	return CDialog::OnMouseWheel(nFlags, zDelta, pt);
}

void CVideoPageDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if ((nFlags & MK_LBUTTON)  && _bShowDPTZ && !_bFullScreen)
	{
		if (_pt_x != -1 && _pt_y != -1)
		{
			if (!_mouse_move)
			{
				_mouse_move = true;
				::SetTimer(GetSafeHwnd(),2,55,NULL);
			}
		}
		else if (_onControl == IDC_SLIDER1)
		{
				POINT pt;
				GetCursorPos(&pt);
				RECT rect1, rect2, rect3;
				_zoomSlider.GetThumbRect(&rect1);
				_zoomSlider.GetChannelRect(&rect2);
				_zoomSlider.GetWindowRect(&rect3);
				RECT rect = {rect3.left+rect1.left, rect3.top + rect2.left,
					rect3.left + rect1.right, rect3.top + rect2.right};
				if (!PtInRect(&rect,pt))
				{
					_onControl = 0;
				}
				else
				{
					int nPos = (pt.y - rect.top) * 100 / (rect.bottom - rect.top);
					_zoomSlider.SetPos(nPos);
					OnVScroll(SB_THUMBTRACK, nPos, (CScrollBar *)&_zoomSlider);
				}
		}
	}
	if (!(nFlags & MK_LBUTTON) && _mouse_move)
	{
		_mouse_move = false;
		::KillTimer(GetSafeHwnd(),2);
	}
	CDialog::OnMouseMove(nFlags, point);
}

void CVideoPageDlg::OnBnClickedButtonUp()
{
	if (_bShowDPTZ && !_bFullScreen)
	{
		m_dptz.dptz_x = 0;
		m_dptz.dptz_y = -100;
		if (!SetDPTZ())
		{
			MessageBox("Set digital PAN/TILT failed!", "Error");
		}
	}

}

void CVideoPageDlg::OnBnClickedButtonLeft()
{
	if (_bShowDPTZ && !_bFullScreen)
	{
		m_dptz.dptz_x = -100;
		m_dptz.dptz_y = 0;
		if (!SetDPTZ())
		{
			MessageBox("Set digital PAN/TILT failed!", "Error");
		}
	}
}

void CVideoPageDlg::OnBnClickedButtonRight()
{
	if (_bShowDPTZ && !_bFullScreen)
	{
		m_dptz.dptz_x = 100;
		m_dptz.dptz_y = 0;
		if (!SetDPTZ())
		{
			MessageBox("Set digital PAN/TILT failed!", "Error");
		}
	}
}

void CVideoPageDlg::OnBnClickedButtonDown()
{
	if (_bShowDPTZ && !_bFullScreen)
	{
		m_dptz.dptz_x = 0;
		m_dptz.dptz_y = 100;
		if (!SetDPTZ())
		{
			MessageBox("Set digital PAN/TILT failed!", "Error");
		}
	}
}

void CVideoPageDlg::OnBnClickedButtonBack()
{
	if (!SetZoom(0))
	{
		MessageBox("Set digital PTZ back to center failed!", "Error");
	}
}


void CVideoPageDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if( nSBCode == SB_THUMBTRACK )
	{
		_slider_move = true;
		if (nPos/10 != _slider_pos/10)
		{
			_slider_pos = nPos;
			if (!SetZoom(11 - nPos/10))
			{
				MessageBox("Set digital zoom failed","Error");
			}
		}
	}
	else if ( nSBCode == SB_ENDSCROLL )
	{
		GetDlgItem(IDC_STATIC_FOCUS)->SetFocus();
		if (!_slider_move)
		{
			nPos = _zoomSlider.GetPos();
			if (nPos != _slider_pos)
			{
				_slider_pos = nPos;
				if (!SetZoom((110 - nPos)/10))
				{
					MessageBox("Set digital zoom failed","Error");
				}
			}
		}
		_slider_move = false;
	}
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CVideoPageDlg::OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult)
{
    TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
   // UINT nID = pNMHDR->idFrom;
    if (pTTT->uFlags & TTF_IDISHWND)
    {
          // idFrom is actually the HWND of the tool
          UINT nID = ::GetDlgCtrlID((HWND)(pNMHDR->idFrom));
          if(IDC_SLIDER1)
          {
			  CString strToolTips;
			  strToolTips.Format("%dX", (11-_zoomSlider.GetPos()/10));
			  strcpy(pTTT->lpszText, strToolTips);
			  pTTT->hinst = NULL;
			  return(TRUE);
		  }
    }
    return(FALSE);
}

// CPropertiesDialog dialog
IMPLEMENT_DYNAMIC (CPropertiesDialog, CDialog)
 CPropertiesDialog::CPropertiesDialog
 (CAmbaCamGraphMngr* p,CWnd* pParent /*=NULL*/)
 : CDialog(CPropertiesDialog::IDD, pParent)
 ,_pMngr(p)
{

}

CPropertiesDialog::~CPropertiesDialog()
{
}

void CPropertiesDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_TRANSFER, _transferType);
	DDX_Control(pDX, IDC_COMBO_DECODER, _decoder);
}


BEGIN_MESSAGE_MAP(CPropertiesDialog, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_CBN_SELCHANGE(IDC_COMBO_DECODER, OnCbnSelchangeComboDecoder)
	ON_CBN_SELCHANGE(IDC_COMBO_TRANSFER, OnCbnSelchangeComboTransfer)
	ON_BN_CLICKED(IDC_BUTTON3, &CPropertiesDialog::OnBnClickedButton3)
END_MESSAGE_MAP()

// CPropertiesDialog message handlers
CPropertiesMenu::CPropertiesMenu(CAmbaCamGraphMngr *p, UINT resourceID) : _pMngr(p)
{
	CMenu::LoadMenu(resourceID);
}

BOOL CPropertiesDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	_transferType.InsertString(0,"TCP Stream Server");
	_transferType.InsertString(1,"RTSP Server");
	switch (_pMngr->GetCameraSourceType())
	{
	case CSFT_TCP:
		_transferType.SetCurSel(0);
		break;
	case CSFT_RTP:
		_transferType.SetCurSel(1);
		break;
	default:
		break;
	}

	_transferType.SetDroppedWidth(2);

	_decoder.InsertString(0, "FFDshow");
	_decoder.InsertString(1, "CoreAVC");
	switch (_pMngr->GetCameraDecorderType())
	{
	case CDT_FFDSHOW:
        _decoder.SetCurSel(0);
		break;
	case CDT_AVC:
		 _decoder.SetCurSel(1);
		 break;
	default:
		break;
	}
	_decoder.SetDroppedWidth(2);
	UpdateData();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPropertiesDialog::OnCbnSelchangeComboDecoder()
{
	// TODO: Add your control notification handler code here
	/*switch(_decoder.GetCurSel())
	{
		case 0:
			_pMngr->SetCameraDecorderType(CDT_FFDSHOW);
			break;
		case 1:
			_pMngr->SetCameraDecorderType(CDT_AVC);
			break;
	}*/
}

void CPropertiesDialog::OnCbnSelchangeComboTransfer()
{
	// TODO: Add your control notification handler code here
/*	switch(_transferType.GetCurSel())
	{
		case 0:
			_pMngr->SetCameraSourceType(CSFT_TCP);
			break;
		case 1:
			_pMngr->SetCameraSourceType(CSFT_RTP);
			break;
	}*/
}

void CPropertiesDialog::OnBnClickedButton1()
{
	switch(_decoder.GetCurSel())
	{
		case 0:
			_pMngr->SetCameraDecorderType(CDT_FFDSHOW);
			break;
		case 1:
			_pMngr->SetCameraDecorderType(CDT_AVC);
			break;
	}
	switch(_transferType.GetCurSel())
	{
		case 0:
			_pMngr->SetCameraSourceType(CSFT_TCP);
			break;
		case 1:
			_pMngr->SetCameraSourceType(CSFT_RTP);
			break;
	}
	CDialog::OnOK();
}

void CPropertiesDialog::OnBnClickedButton2()
{
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}

void CPropertiesDialog::OnBnClickedButton3()
{
	if ( IDOK ==
		MessageBox("Are you sure to recover all your configurations to default value?\n\n    If yes, please click OK and refresh your page.",
		"Notice", MB_OKCANCEL|MB_ICONASTERISK))
	{
		_pMngr->_defconf = true;
		CDialog::OnOK();
	}
	else
	{
		_pMngr->_defconf = false;
		CDialog::OnCancel();
	}
}
// CAboutusDialog dialog

IMPLEMENT_DYNAMIC(CAboutusDialog, CDialog)
CAboutusDialog::CAboutusDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CAboutusDialog::IDD, pParent)
{
}

CAboutusDialog::~CAboutusDialog()
{
}

void CAboutusDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CAboutusDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	CString strVersion;
	int version;
	version = VERSION;
	strVersion.Format("%d.%d.%d.%d",(version>>24) & 0xFF,(version>>16) & 0xFF,(version>>8) & 0xFF,version & 0xFF);
	SetDlgItemText(IDC_STATIC_VMain,strVersion.GetBuffer());

	version = TCP_CLIENT_VERSION;
	strVersion.Format("%d.%d.%d.%d",(version>>24) & 0xFF,(version>>16) & 0xFF,(version>>8) & 0xFF,version & 0xFF);
	SetDlgItemText(IDC_STATIC_Vtcp,strVersion.GetBuffer());

	version = RTSP_CLIENT_VERSION;
	strVersion.Format("%d.%d.%d.%d",(version>>24) & 0xFF,(version>>16) & 0xFF,(version>>8) & 0xFF,version & 0xFF);
	SetDlgItemText(IDC_STATIC_Vrtsp,strVersion.GetBuffer());

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
BEGIN_MESSAGE_MAP(CAboutusDialog, CDialog)
END_MESSAGE_MAP()

