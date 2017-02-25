#include "StdAfx.h"

#include "uuids.h"
#include <windows.h>
#include "streams.h"
#include <initguid.h>
#include "Ambacamgraphmngr.h"

#pragma warning(disable:4995) 
#define _CRTDBG_MAP_ALLOC 

#define RTSP_CONFIG_FILE "RtspClient.ini"
#define TCP_CONFIG_FILE	"TcpClient.ini"

//AmbaTCPClient.ax
DEFINE_GUID(CLSID_AMBA_IPCAMERA_WINDRV_TCP,  
0x75437afa, 0x6a06, 0x4237, 0x9f, 0xeb, 0x77, 0x46, 0xa8, 0x62, 0x4f, 0xa);

//AmbaRTSPClient.ax
DEFINE_GUID(CLSID_AMBA_IPCAMERA_WINDRV_RTP, 
0x3a17a4e2, 0x82b1, 0x4e24, 0xad, 0x33, 0xc2, 0x7f, 0x19, 0x98, 0x72, 0xf6);

//coreAVC decoder
DEFINE_GUID(CLSID_AVC_DECODER, 
0x09571A4B, 0xF1FE, 0x4C60, 0x97, 0x60, 0xDE, 0x6D, 0x31, 0x0C, 0x7C, 0x31);

//ffdshow decoder
DEFINE_GUID(CLSID_FFDSHOW_DECODER, 
0x04FE9017, 0xF873, 0x410E, 0x87, 0x1E, 0xAB, 0x91, 0x66, 0x1A, 0x4E, 0xF7);

//render
DEFINE_GUID(CLSID_RENDERER, 
0x6BC1CFFA, 0x8FC1, 0x4261, 0xAC, 0x22, 0xCF, 0xB4, 0xCC, 0x38, 0xDB, 0x50);

DEFINE_GUID(CLSID_RENDERER_VMR9, 
0x51B4ABF3, 0x748F, 0x4E3B, 0xA2, 0x76, 0xC8, 0x28, 0x33, 0x0E, 0x92, 0x6A);



CAmbaCamGraphMngr::CAmbaCamGraphMngr(): 
	_pGraph(NULL),
	_csft(CSFT_TCP),
	_cdt(CDT_FFDSHOW),
	_bFilterChanged(false),
	_bSourceChanged(false),
	_bAudioOn(false),
	_pCamera(NULL),
	_pDecoder(NULL),
	_pRender(NULL),
	_bPrepared(false),
	_bConnected(false),
	_streamId(0),
	_pEvent(NULL),
	_videoTop(0),
	_videoBottom(0),
	_videoLeft(0),
	_videoRight(0),
	_videoWidth(0),
	_videoHeight(0),
	_statWindowSize(0),
	_defconf(false)
{
	strcpy(_hostname,DEFAULT_HOST);
	HRESULT hr  = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&_pGraph);
	if(FAILED(hr)) {
		MessageBox(NULL, "Create filter graph failed!", "Error", MB_OK);
	}
	hr = _pGraph->QueryInterface(IID_IMediaEventEx, (void **)&_pEvent);
	_wEvent = CreateEvent( 
        NULL,         // no security attributes
		FALSE,         // manual-reset event
        TRUE,         // initial state is signaled
        NULL/*(PCTSTR)"WriteEvent"*/  // object name
        );
}

HRESULT CAmbaCamGraphMngr::AddFilter(CAMERA_FILTER_TYPE _cft) 
{
	if (_pGraph == NULL)
	{
		MessageBox(NULL, "Add Filter Failed!\n No filter graph exists.", "Error", MB_OK);
		return E_FAIL;
	}
	HRESULT hr = NO_ERROR;
	CString strMsg;
	CString strFlt;
	
	switch (_cft)
	{
		case CFT_SOURCE:
			if(_pCamera == NULL)
			{
				switch(_csft)
				{
					case CSFT_TCP:
						hr = AddFilterByCLSID(_pGraph, CLSID_AMBA_IPCAMERA_WINDRV_TCP, L"amba camera", &_pCamera);
						strFlt = "AmbaTCPClient.ax";
						break;
					case CSFT_RTP:
						hr = AddFilterByCLSID(_pGraph, CLSID_AMBA_IPCAMERA_WINDRV_RTP, L"amba camera", &_pCamera);
						strFlt = "AmbaRTSPClent.ax";
						break;
					default:
						strMsg.Format("choose unexpected source filter!");
						hr =  E_UNEXPECTED;
						break;
				}
			}
			if(!CheckSourceVersion())
			{
					RmvFilter(_cft);
			}
			break;
		case CFT_DECODER:
			if(_pDecoder == NULL)
			{
				switch(_cdt)
				{
					case CDT_AVC:
						hr = AddFilterByCLSID(_pGraph, CLSID_AVC_DECODER, L"decoder", &_pDecoder);
						strFlt = "CoreAVC Video Decoder(coreavcdecoder.ax)";
						break;
					case CDT_FFDSHOW:
						hr = AddFilterByCLSID(_pGraph, CLSID_FFDSHOW_DECODER, L"decoder", &_pDecoder);
						strFlt = "ffdshow Video Decoder(ffdshow.ax)";
						break;
					default:
						strMsg.Format("choose unexpected decoder filter!");
						hr =  E_UNEXPECTED;
						break;
				}
			}
			break;
		case CFT_RENDER:
			if(_pRender == NULL)
			{
				hr = AddFilterByCLSID(_pGraph, CLSID_RENDERER, L"render", &_pRender);
				strFlt = "Video Render(quartz.dll)";
			}
			break;
		default:
			hr =  E_UNEXPECTED;
			strMsg = "choose unexpected filter type!";
			break;
	}
	if(FAILED(hr)) {
		if (strFlt != "" )
		{
			strMsg.Format("%s not installed correctly!! Please reinstall it or select other one.", strFlt.GetBuffer());
		}
		MessageBox(NULL, strMsg.GetBuffer(), "Error", MB_OK);
	}
	return hr;
}

HRESULT CAmbaCamGraphMngr::RmvFilter(CAMERA_FILTER_TYPE _cft)
{
	if (_pGraph == NULL)
	{
		MessageBox(NULL, "Remove Filter Failed!\n No filter graph exists.", "Error", MB_OK);
		return E_FAIL;
	}

	HRESULT hr = NO_ERROR;
	switch (_cft)
	{
		case CFT_SOURCE:
			if(_pCamera != NULL)
			{
				_pCamera->Release();
				hr = RmvlFilter(_pGraph, _pCamera);	
				_pCamera = NULL;
			}
			break;
		case CFT_DECODER:
			if(_pDecoder !=  NULL)
			{
				_pDecoder->Release();
				hr = RmvlFilter(_pGraph, _pDecoder);
				_pDecoder = NULL;
			}
			break;
		case CFT_RENDER:
			if(_pRender !=  NULL)
			{
				_pRender->Release();
				hr = RmvlFilter(_pGraph, _pRender);
				_pRender = NULL;
			}
			break;
		default:
			break;
	}
	return hr;	
}

bool CAmbaCamGraphMngr::prepare()
{
	if (_bPrepared && (_bFilterChanged || _bSourceChanged))
	{
		disconnect();
		if (_bFilterChanged)
		{
			RmvFilter(CFT_DECODER);
		}
		if(_bSourceChanged)
		{
			RmvFilter(CFT_SOURCE);
		}
		_bPrepared = false;
	}
	
	if ( !_bPrepared && SUCCEEDED(AddFilter(CFT_SOURCE)) &&
		SUCCEEDED(AddFilter(CFT_DECODER)) &&
		SUCCEEDED(AddFilter(CFT_RENDER)) )
	{
		setstreamId();
		sethostname();
		setstatwindowSize();
		HRESULT hr = ConnectServer();
		if (SUCCEEDED(hr))
		{
			_bPrepared = true;
		}
		else
		{
			RmvFilter(CFT_SOURCE);
			RmvFilter(CFT_DECODER);
			RmvFilter(CFT_RENDER);
		}
	}
	return _bPrepared;
}

bool CAmbaCamGraphMngr::connect()
{
	if(_bPrepared && !_bConnected)
	{
		HRESULT hr ;
		hr = ConnectFilters(_pGraph, _pCamera, _pDecoder);				
		if(!SUCCEEDED(hr))
			return false;
		hr = ConnectFilters(_pGraph, _pDecoder, _pRender);
		if(!SUCCEEDED(hr))
			return false;
		_bConnected = true;
		  // Set the graph clock.
		IMediaFilter *pMediaFilter = 0;
		_pGraph->QueryInterface(IID_IMediaFilter, (void**)&pMediaFilter);
		//if (_csft == CSFT_TCP)
		//{
			pMediaFilter->SetSyncSource(NULL);
		//}
		pMediaFilter->Release();
	}
	return true;
}

bool CAmbaCamGraphMngr::disconnect()
{
		HRESULT hr ;
		hr = DisconnectFilter(_pCamera,PINDIR_OUTPUT);
		if(!SUCCEEDED(hr))
			return false;
		hr = DisconnectFilter(_pDecoder,PINDIR_INPUT);
		if(!SUCCEEDED(hr))
			return false;
		hr = DisconnectFilter(_pDecoder,PINDIR_OUTPUT);
		if(!SUCCEEDED(hr))
			return false;
		hr = DisconnectFilter(_pRender,PINDIR_INPUT);
		if(!SUCCEEDED(hr))
			return false;
		_bConnected = false;
		return true;
}

bool CAmbaCamGraphMngr::run()
{	
	if (_pGraph == NULL)
	{
		return false;
	}
	bool ret = false;
	HRESULT hr;
	IMediaControl *pControl = NULL;
	hr = _pGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
	if(SUCCEEDED(hr)) 
	{
		hr = pControl->Run();
		if(SUCCEEDED(hr))
		{
			pControl->Release();
			ret = true;
		}
	}
	return ret;
}

bool CAmbaCamGraphMngr::stop()
{
	if (_pGraph == NULL)
	{
		return false;
	}
	bool ret = false;
	HRESULT hr;
	IMediaControl *pControl = NULL;
	hr = _pGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
	if(SUCCEEDED(hr)) 
	{
		hr = pControl->Stop();
		if(SUCCEEDED(hr))
		{
			pControl->Release();
			ret = true;
		}
	}
	return ret;
}

bool CAmbaCamGraphMngr::getstate(OAFilterState* pState)
{
	if (_pGraph == NULL)
	{
		return false;
	}
	bool ret = false;
	HRESULT hr;
	IMediaControl *pControl = NULL;
	hr = _pGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);
	if(SUCCEEDED(hr)) 
	{
		pControl->Release();
		hr = pControl->GetState(10,pState);
		if(SUCCEEDED(hr))
		{
			ret = true;
		}
	
	}
	return ret;
}

/*bool CAmbaCamGraphMngr::getresolution(unsigned short* width, unsigned short * height)
{
	if(_pCamera == NULL)
	{
		return false;
	}
	bool ret = false;
	HRESULT hr;
	IAmbaPin *pIAmbaPin = NULL;
	IPin *pPin = NULL;
	hr = _pCamera->FindPin(L"1",(IPin **)(&pPin));
	if(SUCCEEDED(hr))
	{
		hr = pPin->QueryInterface(IID_IAmbaPin, (void**)(&pIAmbaPin));
		if(SUCCEEDED(hr))
		{
			pIAmbaPin->GetResolution(width, height);
			pIAmbaPin->Release();
			ret = true;
		}
		pPin->Release();
	}
	return ret;
}*/

bool CAmbaCamGraphMngr::getvideoPos(long *pLeft, long *pTop,
										 long *pRight, long *pBottom)
{
	if(_videoLeft == 0 && _videoRight == 0 
		&& _videoTop == 0 && _videoTop == 0)
	{
		return false;
	}
	else
	{
		DWORD dwWaitResult = WaitForSingleObject(_wEvent,INFINITE);
		switch (dwWaitResult) 
		{
        // Both event objects were signaled.
		case WAIT_OBJECT_0: 
		// Read from the shared buffer.
		break; 
		// An error occurred.
		default:
			return false;
		}

		POINT pt_0 = {_videoLeft,_videoTop};
		POINT pt_1 = {_videoRight,_videoBottom};
		if(ClientToScreen((HWND)_hH264wnd,&pt_0))
		{
			*pLeft  = pt_0.x;
			*pTop = pt_0.y;
		}
		if(ClientToScreen((HWND)_hH264wnd,&pt_1))
		{
			*pRight = pt_1.x;
			*pBottom = pt_1.y;
		}
		SetEvent(_wEvent);
		return true;
	}

	/*if (_pGraph == NULL || _pRender == NULL)
	{
		return false;
	}
	bool ret = false;
	IVideoWindow* pVideoWnd = NULL;
	HRESULT hr;
	hr = _pRender->QueryInterface(IID_IVideoWindow, (void**)&pVideoWnd);
	if SUCCEEDED(hr)
	{
		hr = pVideoWnd->GetWindowPosition(pLeft,pTop,pRight,pBottom);
		*pRight += *pLeft;
		*pBottom += *pTop;
	}*/
}
bool CAmbaCamGraphMngr::getvideoSize(long *pWidth, long *pHeight)
{
	if (_videoWidth == 0 && _videoHeight == 0)
	{
		return false;
	}
	else
	{
		DWORD dwWaitResult = WaitForSingleObject(_wEvent,INFINITE);
		switch (dwWaitResult) 
		{
        // Both event objects were signaled.
		case WAIT_OBJECT_0: 
		// Read from the shared buffer.
		break; 
		// An error occurred.
		default:
			return false;
		}
		*pWidth = _videoWidth;
		*pHeight = _videoHeight;
		SetEvent(_wEvent);
		return true;
	}
}

void CAmbaCamGraphMngr::record(bool bOp)
{
	HRESULT hr;
	IAmbaRecord *pIAmbaRecord = NULL;
	if(_pCamera == NULL)
	{
		return;
	}
	hr = _pCamera->QueryInterface(IID_IAmbaRecord, (void**)&pIAmbaRecord);
	if(pIAmbaRecord != NULL)
	{
		pIAmbaRecord->SetRecordStatus(bOp);
		pIAmbaRecord->Release();
	}
}

bool CAmbaCamGraphMngr::getrecord()
{
	bool bStatus = false;
	HRESULT hr;
	IAmbaRecord *pIAmbaRecord = NULL;
	if(_pCamera == NULL)
	{
		return bStatus;
	}
	hr = _pCamera->QueryInterface(IID_IAmbaRecord, (void**)&pIAmbaRecord);
	if(pIAmbaRecord != NULL)
	{
		pIAmbaRecord->GetRecordStatus(&bStatus);
		pIAmbaRecord->Release();
	}
	return  bStatus;
}

void CAmbaCamGraphMngr::setstreamId()
{
	HRESULT hr;
	IAmbaPin *pIAmbaPin = NULL;
	if(_pCamera == NULL)
	{
		return;
	}
	IPin *pPin = NULL;
	hr = _pCamera->FindPin(L"1",(IPin **)(&pPin));
	if(pPin == NULL)
	{
		return;
	}
	hr = pPin->QueryInterface(IID_IAmbaPin, (void**)(&pIAmbaPin));
	if(SUCCEEDED(hr))
	{
		pIAmbaPin->SetStreamId(_streamId);
		pIAmbaPin->Release();
	}
	pPin->Release();
}

bool  CAmbaCamGraphMngr::setrecvType(short recv_type)
{
	bool ret = true;
	switch (recv_type) {
		case 0:
			_csft = CSFT_TCP;
			break;
		case 1:
			_csft = CSFT_RTP;
			break;
		default:
			ret = false;
			break;
	}
	return ret;
}

void CAmbaCamGraphMngr::sethostname()
{
	HRESULT hr;
	IAmbaPin *pIAmbaPin = NULL;
	if(_pCamera == NULL)
	{
		return;
	}
	IPin *pPin = NULL;
	hr = _pCamera->FindPin(L"1",(IPin **)(&pPin));
	if(pPin == NULL)
	{
		return;
	}
	hr = pPin->QueryInterface(IID_IAmbaPin, (void**)(&pIAmbaPin));
	if(SUCCEEDED(hr))
	{
		pIAmbaPin->SetHostname(_hostname);
		pIAmbaPin->Release();
	}
	pPin->Release();
}

bool CAmbaCamGraphMngr::getstat(ENC_STAT* stat)
{
	if(_pCamera == NULL)
	{
		return false;
	}
	bool ret = false;
	HRESULT hr;
	IAmbaPin *pIAmbaPin = NULL;
	IPin *pPin = NULL;
	hr = _pCamera->FindPin(L"1",(IPin **)(&pPin));
	if(SUCCEEDED(hr))
	{
		hr = pPin->QueryInterface(IID_IAmbaPin, (void**)(&pIAmbaPin));
		if(SUCCEEDED(hr))
		{
			pIAmbaPin->GetStatistics(stat);
			pIAmbaPin->Release();
			ret = true;
		}
		pPin->Release();
	}
	return ret;
}

void CAmbaCamGraphMngr::setstatwindowSize()
{
	if (_statWindowSize == 0)
	{
		return;
	}
	HRESULT hr;
	IAmbaPin *pIAmbaPin = NULL;
	if(_pCamera == NULL)
	{
		return;
	}
	IPin *pPin = NULL;
	hr = _pCamera->FindPin(L"1",(IPin **)(&pPin));
	if(pPin == NULL)
	{
		return;
	}
	hr = pPin->QueryInterface(IID_IAmbaPin, (void**)(&pIAmbaPin));
	if(SUCCEEDED(hr))
	{
		pIAmbaPin->SetStatWindowSize(_statWindowSize);
		pIAmbaPin->Release();
	}
	pPin->Release();
}

HRESULT CAmbaCamGraphMngr::ConnectServer()
{
	HRESULT hr;
	IAmbaPin *pIAmbaPin = NULL;
	if(_pCamera == NULL)
	{
		return E_FAIL;
	}
	IPin *pPin = NULL;
	hr = _pCamera->FindPin(L"1",(IPin **)(&pPin));
	if(FAILED(hr))
	{
		return hr;
	}
	hr = pPin->QueryInterface(IID_IAmbaPin, (void**)(&pIAmbaPin));
	if(SUCCEEDED(hr))
	{
		hr = pIAmbaPin->ConnectServer();
		if(FAILED(hr))
		{
			MessageBox(NULL, "Can't Connect Server", "Error",MB_OK);
		}
		pIAmbaPin->Release();
	} else {
		hr = NO_ERROR;
	}
	pPin->Release();
	return hr;
}

/*HRESULT CAmbaCamGraphMngr::DisconnectServer()
{
	HRESULT hr;
	IAmbaPin *pIAmbaPin = NULL;
	if(_pCamera == NULL)
	{
		return E_FAIL;
	}
	IPin *pPin = NULL;
	hr = _pCamera->FindPin(L"1",(IPin **)(&pPin));
	if(FAILED(hr))
	{
		return hr;
	}
	hr = pPin->QueryInterface(IID_IAmbaPin, (void**)(&pIAmbaPin));
	if(SUCCEEDED(hr))
	{
		hr = pIAmbaPin->DisconnectServer();
		if(FAILED(hr))
		{
			MessageBox(NULL, "Disconnect Server Errort", "Disonnect Server Error",MB_OK);
		}
		pIAmbaPin->Release();
	}
	pPin->Release();
	return hr;
}*/

bool CAmbaCamGraphMngr::CheckSourceVersion()
{
	if(_pCamera == NULL)
	{
		return false;
	}
	HRESULT hr;
	int real_version = 0;
	int expect_version = 0xFFFFFFFF;
	CString strFlt;
	IAmbaCom *pIAmbaCom = NULL;
	hr = _pCamera->QueryInterface(IID_IAmbaCom, (void**)&pIAmbaCom);
	if(SUCCEEDED(hr))
	{
		hr = pIAmbaCom->GetVersion(&real_version);
		if(FAILED(hr))
		{
			return false;
		}
		switch(_csft)
		{
		case CSFT_TCP: 
			expect_version = TCP_CLIENT_VERSION;
			strFlt = "AmbaTCPClient.ax";
			break;
		case CSFT_RTP:
			expect_version = RTSP_CLIENT_VERSION;
			strFlt = "AmbaRTSPClient.ax";
			break;
		default:
			break;
		}
		pIAmbaCom->Release();
	}
	if (real_version == expect_version)
	{
		return true;
	}
	else
	{
		CString strMsg;
		CString strVersion;
		strVersion.Format("%d.%d.%d.%d",(expect_version>>24) & 0xFF,(expect_version>>16) & 0xFF,(expect_version>>8) & 0xFF,expect_version & 0xFF);
		strMsg.Format("The version of %s should be %s!!\nPlease register the right one.", strFlt.GetBuffer(),strVersion.GetBuffer(),real_version,expect_version);
		MessageBox(NULL, strMsg, "Error", MB_OK);
		return false;
	}
}
void CAmbaCamGraphMngr::PopSourceFilterPropPage(HWND hwnd)
{
	if(_bPrepared)
		PopPropertyPage(_pCamera, hwnd);
}
void CAmbaCamGraphMngr::PopDecorderFilterPropPage(HWND hwnd)
{
	if(_bPrepared)
		PopPropertyPage(_pDecoder, hwnd);
}
void CAmbaCamGraphMngr::PopRenderFilterPropPage(HWND hwnd)
{
	if(_bPrepared)
		PopPropertyPage(_pRender, hwnd);
}


void CAmbaCamGraphMngr::SetWnd(OAHWND hwnd, RECT rc)
{
	_hH264wnd = hwnd;
	_H264rc = rc;	
}

void CAmbaCamGraphMngr::Resize(RECT rc)
{
	IVideoWindow  *m_pVW;
	LONG width, height;
	_pRender->QueryInterface(IID_IVideoWindow,(void **)&m_pVW);
	HRESULT hr;//  = m_pVW->put_Owner(hwnd);
	//hr = m_pVW->put_WindowStyle(WS_CHILD);
	m_pVW->put_Visible(OAFALSE);
		
	width =  rc.right - rc.left;
    height = rc.bottom - rc.top;

    // Ignore the video's original size and stretch to fit bounding rectangle
    hr = m_pVW->SetWindowPosition(rc.left, rc.top, width, height);

	hr = m_pVW->put_Visible(OATRUE);
	m_pVW->Release();
	//hr = m_pVW->SetWindowForeground(-1);
}


CAmbaCamGraphMngr::~CAmbaCamGraphMngr(void)
{		
	OAFilterState state;
	if (getstate(&state) && state != State_Stopped)
	{
		stop();
	}
	RmvFilter(CFT_SOURCE);
	RmvFilter(CFT_DECODER);
	RmvFilter(CFT_RENDER);
	if(_pGraph != NULL)
	{
		_pGraph->Release();
	}
	if(_pEvent != NULL)
	{
		_pEvent->Release();
	}
	if (_defconf)
	{
		char del_filename[64];
		char* config_dir = getenv("ProgramFiles");
		if (config_dir) {
			sprintf_s(del_filename, "%s\\Ambarella\\%s",config_dir,RTSP_CONFIG_FILE);
			remove(del_filename);
			sprintf_s(del_filename, "%s\\Ambarella\\%s",config_dir,TCP_CONFIG_FILE);
			remove(del_filename);
		}
	}
}

HRESULT CAmbaCamGraphMngr::AddToRot(IUnknown* pUnkGraph, DWORD* pdwRegister)
{
	IMoniker *pMoniker;
	IRunningObjectTable *pROT;
	if(FAILED(GetRunningObjectTable(0,&pROT)))
	{
		return E_FAIL;
	}
	WCHAR wsz[256];
	_stprintf((LPSTR)wsz, (LPSTR)L"FilterGraph %8x pid %08x", (DWORD_PTR)pUnkGraph, GetCurrentProcessId());
	HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);
	if(SUCCEEDED(hr))
	{
		hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph, pMoniker, pdwRegister);
		pMoniker->Release();
	}
	pROT->Release();
	return hr;
}

void CAmbaCamGraphMngr::RemoveFromRot(DWORD id)
{
	IRunningObjectTable *pROT;
	if(SUCCEEDED(GetRunningObjectTable(0, &pROT)))
	{
		pROT->Revoke(id);
		pROT->Release();
	}
}

HRESULT CAmbaCamGraphMngr::AddFilterByCLSID
(
    IGraphBuilder *pGraph,  // Pointer to the Filter Graph Manager.
    const GUID& clsid,      // CLSID of the filter to create.
    LPCWSTR wszName,        // A name for the filter.
    IBaseFilter **ppF		// Receives a pointer to the filter.
)      
{
    if (!pGraph || ! ppF) return E_POINTER;
    *ppF = 0;
    IBaseFilter *pF = 0;
    HRESULT hr = CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER,
      IID_IBaseFilter, reinterpret_cast<void**>(&pF));

    if (SUCCEEDED(hr))
    {
        hr = pGraph->AddFilter(pF, wszName);
        if (SUCCEEDED(hr))
            *ppF = pF;
        else
            pF->Release();
    }
    return hr;
}

HRESULT CAmbaCamGraphMngr::RmvlFilter
(
	IGraphBuilder *pGraph,
	IBaseFilter* pF
)
{
	if (!pGraph || ! pF) return E_POINTER;
	return pGraph->RemoveFilter(pF);
}

HRESULT CAmbaCamGraphMngr::GetUnconnectedPin(
    IBaseFilter *pFilter,   // Pointer to the filter.
    PIN_DIRECTION PinDir,   // Direction of the pin to find.
    IPin **ppPin)           // Receives a pointer to the pin.
{
    *ppPin = 0;
    IEnumPins *pEnum = 0;
    IPin *pPin = 0;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        return hr;
    }
    while (pEnum->Next(1, &pPin, NULL) == S_OK)
    {
        PIN_DIRECTION ThisPinDir;
        pPin->QueryDirection(&ThisPinDir);
        if (ThisPinDir == PinDir)
        {
            IPin *pTmp = 0;
            hr = pPin->ConnectedTo(&pTmp);
            if (SUCCEEDED(hr))  // Already connected, not the pin we want.
            {
                pTmp->Release();
            }
            else  // Unconnected, this is the pin we want.
            {
                pEnum->Release();
                *ppPin = pPin;
                return S_OK;
            }
        }
        pPin->Release();
    }
    pEnum->Release();
    // Did not find a matching pin.
    return E_FAIL;
}

HRESULT CAmbaCamGraphMngr::ConnectFilters(
    IGraphBuilder *pGraph, // Filter Graph Manager.
    IPin *pOut,            // Output pin on the upstream filter.
    IBaseFilter *pDest)    // Downstream filter.
{
    if ((pGraph == NULL) || (pOut == NULL) || (pDest == NULL))
    {
        return E_POINTER;
    }
#ifdef debug
        PIN_DIRECTION PinDir;
        pOut->QueryDirection(&PinDir);
        _ASSERTE(PinDir == PINDIR_OUTPUT);
	//pOut->AddRef();
	//cc = pOut->Release();
	//TRACE("2 refrence num : %d\n", cc);	
#endif
	// 4	
    // Find an input pin on the downstream filter.
    IPin *pIn = 0;
    HRESULT hr = GetUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
	// 4
    if (FAILED(hr))
    {
        return hr;
    }
    // Try to connect them.
    hr = pGraph->Connect(pOut, pIn);
    pIn->Release();
    return hr;

}

HRESULT CAmbaCamGraphMngr::ConnectFilters(
    IGraphBuilder *pGraph, 
    IBaseFilter *pSrc, 
    IBaseFilter *pDest)
{
    if ((pGraph == NULL) || (pSrc == NULL) || (pDest == NULL))
    {
        return E_POINTER;
    }
	// 3	
    // Find an output pin on the first filter.
    IPin *pOut = 0;
    HRESULT hr = GetUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
    if (FAILED(hr)) 
    {
        return hr;
    }
	// 4	
    hr = ConnectFilters(pGraph, pOut, pDest);
    pOut->Release();
    return hr;
}

HRESULT  CAmbaCamGraphMngr::DisconnectFilter(
    IBaseFilter *pFilter,   // Pointer to the filter.
    PIN_DIRECTION PinDir) // Direction of the pin to find.
{
    IEnumPins *pEnum = 0;
    IPin *pPin = 0;
    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        return hr;
    }
    while (pEnum->Next(1, &pPin, NULL) == S_OK)
    {
        PIN_DIRECTION ThisPinDir;
        pPin->QueryDirection(&ThisPinDir);
        if (ThisPinDir == PinDir)
        {
            IPin *pTmp = 0;
            hr = pPin->ConnectedTo(&pTmp);
            if (SUCCEEDED(hr))  // Already connected, not the pin we want.
            {
				pTmp->Release();
				hr = pPin->Disconnect();
				if(FAILED(hr))
				{
					return E_FAIL;
				}
            }
        }
        pPin->Release();
    }
    pEnum->Release();
    return S_OK;
}


HRESULT CAmbaCamGraphMngr::PopPropertyPage(IBaseFilter *pFilter, HWND hWnd)
{
	/* Obtain the filter's IBaseFilter interface. (Not shown) */
	ISpecifyPropertyPages *pProp;
	HRESULT hr = pFilter->QueryInterface(IID_ISpecifyPropertyPages, (void **)&pProp);
	if (SUCCEEDED(hr)) 
	{
		// Get the filter's name and IUnknown pointer.
		FILTER_INFO FilterInfo;
		hr = pFilter->QueryFilterInfo(&FilterInfo); 
		IUnknown *pFilterUnk;
		pFilter->QueryInterface(IID_IUnknown, (void **)&pFilterUnk);

		// Show the page. 
		CAUUID caGUID;
		pProp->GetPages(&caGUID);
		pProp->Release();
		OleCreatePropertyFrame(
			hWnd,                   		// Parent window
			0, 0,                   		// Reserved
			FilterInfo.achName,     	// Caption for the dialog box
			1,                      			// Number of objects (just the filter)
			&pFilterUnk,            		// Array of object pointers. 
			caGUID.cElems,          	// Number of property pages
			caGUID.pElems,          	// Array of property page CLSIDs
			0,                      			// Locale identifier
			0, NULL                 		// Reserved
	    	);

	    	// Clean up.
		pFilterUnk->Release();
		FilterInfo.pGraph->Release(); 
		CoTaskMemFree(caGUID.pElems);
	}
	//pProp->Release();
	return S_OK;
}


bool CAmbaCamGraphMngr::SetVideoWnd(HWND hwnd, bool showDPTZ)
{
	if (_pGraph == NULL || _pRender == NULL)
	{
		return false;
	}
	
	IVideoWindow* pVideoWnd;
	HRESULT hr;
	hr = _pRender->QueryInterface(IID_IVideoWindow, (void**)&pVideoWnd);
	if(SUCCEEDED(hr))
	{
		DWORD dwWaitResult = WaitForSingleObject(_wEvent,INFINITE);
		switch (dwWaitResult) 
		{
        // Both event objects were signaled.
		case WAIT_OBJECT_0: 
		// Read from the shared buffer.
		break; 
		// An error occurred.
		default:
			return false;
		}
		hr = pVideoWnd->put_MessageDrain(_hH264wnd);
		hr = pVideoWnd->put_Visible(OAFALSE);
		hr = pVideoWnd->put_Owner((OAHWND)hwnd);
		hr = pVideoWnd->put_WindowStyle(WS_CHILD);
		if ( hwnd == NULL)
		{
			long width =   GetSystemMetrics(SM_CXSCREEN);   
			long height =  GetSystemMetrics(SM_CYSCREEN);   
			hr = pVideoWnd->SetWindowPosition(_H264rc.left, _H264rc.top, width, height);
		}
		else 
		{	
			long width = _H264rc.right - _H264rc.left;
			long height = _H264rc.bottom - _H264rc.top - 40;

			_videoHeight = 0;
			_videoWidth = 0;
			_videoLeft = 0;
			_videoRight = 0;
			_videoTop = 0;
			_videoBottom = 0;
			IBasicVideo* pVideoInfo =NULL;
			_pRender->QueryInterface(IID_IBasicVideo, (void**)&pVideoInfo);
			if (pVideoInfo != NULL)
			{
				hr = pVideoInfo->get_SourceHeight(&_videoHeight);
				hr = pVideoInfo->get_SourceWidth(&_videoWidth);
				pVideoInfo->Release();
			}
			if (showDPTZ && _videoWidth >= width )
			{
				_videoHeight = _videoHeight - (_videoWidth - width*8/10 )*_videoHeight/_videoWidth;
				_videoWidth = width*8/10;
			}
			
			if( _videoWidth <= width  && _videoHeight <= height)
			{
				_videoLeft = _H264rc.left+(width-_videoWidth)/2;
				_videoTop = _H264rc.top+40+(height - _videoHeight)/2;
				_videoRight = _videoLeft + _videoWidth;
				_videoBottom = _videoTop + _videoHeight;
				hr = pVideoWnd->SetWindowPosition(_videoLeft, _videoTop,
													_videoWidth, _videoHeight);
				
			}
			else
			{
				if (_videoWidth * height > _videoHeight * width)
				{
					_videoLeft = _H264rc.left;
					_videoTop = _H264rc.top+40 + (height-_videoHeight*width/_videoWidth)/2;
					_videoRight = _videoLeft + width;
					_videoBottom = _videoTop + _videoHeight*width/_videoWidth;
					hr = pVideoWnd->SetWindowPosition(_videoLeft, _videoTop,
														width, 	_videoHeight*width/_videoWidth);
				}
				else
				{
					_videoLeft = _H264rc.left + (width - _videoWidth * height/_videoHeight)/2;
					_videoTop = _H264rc.top + 40;
					_videoRight = _videoLeft + _videoWidth * height/_videoHeight;
					_videoBottom = _videoTop + height;
					hr = pVideoWnd->SetWindowPosition(_videoLeft ,_videoTop,
													_videoWidth * height/_videoHeight, 
													height);
				}
			}
		}
		hr = pVideoWnd->put_Visible(OATRUE);			
		pVideoWnd->Release();
		SetEvent(_wEvent);
		return true;
	}
	else
	{
		return false;
	}
}

IMediaEventEx * CAmbaCamGraphMngr::GetEventHandle(void)
{
	return _pEvent;
}

bool CAmbaCamGraphMngr::SetNotifyWindow(HWND inWindow)
{
	if (_pEvent)
	{
		_pEvent->SetNotifyWindow((OAHWND)inWindow, WM_GRAPHNOTIFY, 0);
		return true;
	}
	return false;
}
