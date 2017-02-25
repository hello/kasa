//------------------------------------------------------------------------------
// File: fambademo.cpp
//
// Copyright (c) Ambarella Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#include <winsock2.h>

#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <Ks.h>
#include <KsMedia.h>

#include "iso_muxer.h"
#include "fambademo.h"
#include "resource.h"
#include "amba_guid.h"

DEFINE_GUID(MEDIASUBTYPE_H264,
0x8D2D71CB, 0x243F, 0x45E3, 0xB2, 0xD8, 0x5F, 0xD7, 0x96, 0x7E, 0xC0, 0x9B);

DEFINE_GUID(MEDIASUBTYPE_AVC1, 
0x31435641, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

#define MJPEG_BUF_SIZE		(2*1024*1024)
#define H264_BUF_SIZE		(1*1024*1024)
#define AUDIO_BUF_SIZE		(64 * 1024)

char target_ip[16] = "10.0.0.2";
short control_port = 20000;
short main_port = 20001;
short second_port = 20002;
short audio_port = 20041;
short motion_port = 20081;
short face_port = motion_port+1;

CAmbaRecordControl *G_pRecordControl = NULL;
ULONG G_refCount = 0;
MUXER_PARAM *G_pMuxerParam = NULL;

#include "demo_page.h"
#include "demo_page.cpp"

#define MOTION_BUF_SIZE	  (100)
#define FACE_BUF_SIZE	  (100)
typedef struct tagMotionInfo
{
	//unsigned char* pMotionImageData;
	 long MotionStreamFlag ;
}MOTION_INFO, *pMotionInfo;

typedef struct tagFaceInfo
{
	//unsigned char* pMotionImageData;
	 long FaceStreamFlag ;
}FACE_INFO, *pFaceInfo;



#ifdef DEBUG

#define Error(msg, title)   MessageBox(0, msg, title, MB_OK)
void TRACE(const char *fmt, ...)
{
	 char out[1024];
	 va_list body;
	 va_start(body, fmt);
	 StringCchVPrintf(out, sizeof(out), fmt, body);
	 va_end(body);
	 OutputDebugString(out);
}

#else

#define Error(msg, title)
#define TRACE

#endif

// Setup data

const AMOVIESETUP_FILTER sudAmbaDemoax =
{
	&CLSID_A5S_AMBADEMO,		// Filter CLSID
	L"Ambarella A5S IPCamera Filter",// String name
	MERIT_DO_NOT_USE,	   // Filter merit
	0,
	NULL
};


// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {
// filter
	{ L"Amba A5S Demo"
	, &CLSID_A5S_AMBADEMO
	, CAmbaDemo::CreateInstance
	, NULL
	, &sudAmbaDemoax }

// encode format property page
	, {L"Encode Format"
	, &CLSID_EncodeFormat
	, &CAmbaEncodeFormatProp::CreateInstance
	, NULL, NULL }

// encode param property page
	, {L"Encode Param"
	, &CLSID_EncodeParam
	, &CAmbaEncodeParamProp::CreateInstance
	, NULL, NULL }

// imgproc param property page
	, {L"Img Param"
	, &CLSID_ImgParam
	, &CAmbaImgParamProp::CreateInstance
	, NULL, NULL }

/* md param property page */
	, {L"Md Param"
	, &CLSID_MdParam
	, &CAmbaMdParamProp::CreateInstance
	, NULL, NULL }

	, {L"Muxer Param"
	, &CLSID_MuxerParam
	, &CAmbaMuxerParamProp::CreateInstance
	, NULL, NULL }

// about property page
	, {L"About"
	, &CLSID_AmbaAbout
	, &CAmbaAboutProp::CreateInstance
	, NULL, NULL }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
//
STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2(TRUE);
} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2(FALSE);
} // DllUnregisterServer


//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
					  DWORD  dwReason, 
					  LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

HRESULT SendBuffer(SOCKET socket, char *pbuffer, int size)
{
	int iResult;
	iResult = send(socket, pbuffer, size, 0);
	if (iResult != size)
	{
		TRACE("send error: %d\n", WSAGetLastError());
		return E_FAIL;
	}
	return S_OK;
}

HRESULT ReceiveBuffer(SOCKET socket, char *pbuffer, int size)
{
	char *ptr = pbuffer;
	while (size > 0)
	{
		int iResult = ::recv(socket, ptr, size, 0);
		if (iResult <= 0)
		{
			TRACE("recv error: %d\n", WSAGetLastError());
			return E_FAIL;
		}
		ptr += iResult;
		size -= iResult;
	}
	return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// CAmbaRecordControl
//
////////////////////////////////////////////////////////////////////////
CAmbaRecordControl::CAmbaRecordControl(LPUNKNOWN lpunk, HRESULT *phr):
	inherited(NAME("RecordControl"), lpunk),
	m_socket(INVALID_SOCKET),
	m_bInfoValid(FALSE),
	m_bFormatValid(FALSE),
	m_bParamValid(FALSE),
	m_bImgParamValid(FALSE),
	m_bMdParamValid(FALSE)
{
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET)
	{
		Error("Cannot create socket", "CAmbaRecordControl");
		*phr = E_FAIL;
		return;
	}

	sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = inet_addr(target_ip);
	client.sin_port = htons(control_port);

	if (connect(m_socket, (SOCKADDR*)&client, sizeof(client)) == SOCKET_ERROR) {
		Error("Connection failed", "Connect");
		*phr = E_FAIL;
		return;
	}

	*phr = VerifyVersion();
}

CAmbaRecordControl::~CAmbaRecordControl()
{
	closesocket(m_socket);
}

STDMETHODIMP CAmbaRecordControl::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	return inherited::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CAmbaRecordControl::VerifyVersion()
{
	HRESULT hr;

	hr = SendRequest(REQ_GET_VERSION);
	if (FAILED(hr))
		return hr;

	VERSION version;
	hr = Receive((char *)&version, sizeof(version));
	if (FAILED(hr))
		return hr;

	if (version.magic != DATA_MAGIC || version.version != DATA_VERSION)
	{
		char msg[128];
		::StringCchPrintf(msg, sizeof(msg),
			"version not match!\nserver: 0x%04x\nfilter: 0x%04x\n%s", version.version, DATA_VERSION,
			version.version > DATA_VERSION ? "Server is new." : "Server is old.");
		::MessageBox(0, msg, "error", MB_OK | MB_ICONEXCLAMATION);
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CAmbaRecordControl::GetInfo(INFO **ppInfo)
{
	if (!m_bInfoValid)
	{
		HRESULT hr;

		hr = SendRequest(REQ_GET_INFO);
		if (FAILED(hr))
			return hr;

		hr = Receive((char *)&m_info, sizeof(m_info));
		if (FAILED(hr))
			return hr;

		m_bInfoValid = TRUE;
	}

	*ppInfo = &m_info;
	return S_OK;
}

STDMETHODIMP CAmbaRecordControl::GetFormat(FORMAT **ppFormat)
{
	if (!m_bFormatValid)
	{
		HRESULT hr;

		hr = SendRequest(REQ_GET_FORMAT);
		if (FAILED(hr))
			return hr;

		hr = Receive((char *)&m_format, sizeof(m_format));
		if (FAILED(hr))
			return hr;

		m_bFormatValid = TRUE;
	}

	*ppFormat = &m_format;
	return S_OK;
}

STDMETHODIMP CAmbaRecordControl::SetFormat(int *pResult)
{
	HRESULT hr;

	if (!m_bFormatValid)
		return E_FAIL;

	hr = SendRequest(REQ_SET_FORMAT);
	if (FAILED(hr))
		return hr;

	hr = Send((char *)&m_format, sizeof(m_format));
	if (FAILED(hr))
		return hr;

	ACK ack;
	hr = Receive((char *)&ack, sizeof(ack));
	if (FAILED(hr))
		return hr;

	*pResult = ack.result;

	return S_OK;
}

STDMETHODIMP CAmbaRecordControl::GetParam(PARAM **ppParam)
{
	if (!m_bParamValid)
	{
		HRESULT hr;

		hr = SendRequest(REQ_GET_PARAM);
		if (FAILED(hr))
			return hr;

		hr = Receive((char *)&m_param, sizeof(m_param));
		if (FAILED(hr))
			return hr;

		m_bParamValid = TRUE;
	}

	*ppParam = &m_param;
	return S_OK;
}

STDMETHODIMP CAmbaRecordControl::SetParam(int *pResult)
{
	HRESULT hr;

	if (!m_bParamValid)
		return E_FAIL;

	hr = SendRequest(REQ_SET_PARAM);
	if (FAILED(hr))
		return hr;

	hr = Send((char *)&m_param, sizeof(m_param));
	if (FAILED(hr))
		return hr;

	ACK ack;
	hr = Receive((char *)&ack, sizeof(ack));
	if (FAILED(hr))
		return hr;

	*pResult = ack.result;

	return S_OK;
}

STDMETHODIMP CAmbaRecordControl::GetImgParam(IMG_PARAM **ppImgParam)
{
	if (!m_bImgParamValid)
	{
		HRESULT hr;

		hr = SendRequest(REQ_GET_IMG_PARAM);
		if (FAILED(hr))
			return hr;

		hr = Receive((char *)&m_imgParam, sizeof(m_imgParam));
		if (FAILED(hr))
			return hr;

		m_bImgParamValid = TRUE;
	}

	*ppImgParam = &m_imgParam;
	return S_OK;
}

STDMETHODIMP CAmbaRecordControl::SetImgParam(int *pResult)
{
	HRESULT hr;

	if (!m_bImgParamValid)
		return E_FAIL;

	hr = SendRequest(REQ_SET_IMG_PARAM);
	if (FAILED(hr))
		return hr;

	hr = Send((char *)&m_imgParam, sizeof(m_imgParam));
	if (FAILED(hr))
		return hr;

	ACK ack;
	hr = Receive((char *)&ack, sizeof(ack));
	if (FAILED(hr))
		return hr;

	*pResult = ack.result;

	return S_OK;
}

STDMETHODIMP CAmbaRecordControl::GetMdParam(MD_PARAM **ppMdParam)
{
	if (!m_bMdParamValid)
	{
		HRESULT hr;

		hr = SendRequest(REQ_GET_MD_PARAM);
		if (FAILED(hr))
			return hr;

		hr = Receive((char *)&m_mdParam, sizeof(m_mdParam));
		if (FAILED(hr))
			return hr;

		m_bMdParamValid = TRUE;
	}

	*ppMdParam = &m_mdParam;
	return S_OK;
}

STDMETHODIMP CAmbaRecordControl::SetMdParam(int *pResult)
{
	HRESULT hr;

	if (!m_bMdParamValid)
		return E_FAIL;

	hr = SendRequest(REQ_SET_MD_PARAM);
	if (FAILED(hr))
		return hr;

	hr = Send((char *)&m_mdParam, sizeof(m_mdParam));
	if (FAILED(hr))
		return hr;

	ACK ack;
	hr = Receive((char *)&ack, sizeof(ack));
	if (FAILED(hr))
		return hr;

	*pResult = ack.result;

	return S_OK;
}

STDMETHODIMP CAmbaRecordControl::SetForceIDR(int *pResult)
{
	HRESULT hr;

	hr = SendRequest(REQ_SET_FORCEIDR);
	if (FAILED(hr))
		return hr;

	ACK ack;
	hr = Receive((char *)&ack, sizeof(ack));
	if (FAILED(hr))
		return hr;

	*pResult = ack.result;

	return S_OK;
}

HRESULT CAmbaRecordControl::Send(char *buffer, ULONG size)
{
	return ::SendBuffer(m_socket, buffer, size);
}

HRESULT CAmbaRecordControl::Receive(char *buffer, ULONG size)
{
	return ::ReceiveBuffer(m_socket, buffer, size);
}

HRESULT CAmbaRecordControl::SendRequest(int code)
{
	REQUEST req;
	req.code = code;
	return Send((char *)&req, sizeof(req));
}

////////////////////////////////////////////////////////////////////////
//
// GetRegistryDword
// GetRegistrySettings
//
////////////////////////////////////////////////////////////////////////
void GetRegistryDword(HKEY Key, short *pValue, LPCSTR name)
{
	DWORD type = REG_DWORD;
	DWORD value;
	DWORD cbData = sizeof(value);
	LONG rval;
	if ((rval = RegQueryValueEx(Key, name, NULL, &type, (LPBYTE)&value, &cbData)) == ERROR_SUCCESS)
		*pValue = (short)value;
}

void GetRegistrySettings()
{
	HKEY Key;
	BYTE buffer[64];
	LONG rval;
	DWORD type;
	DWORD cbData;
	LPCSTR path = TEXT("SOFTWARE\\Ambarella\\IpCam");

	if ((rval = RegOpenKeyEx(HKEY_LOCAL_MACHINE, path,
		0, KEY_READ, &Key)) != ERROR_SUCCESS)
		return;

	type = REG_SZ;
	cbData = sizeof(buffer);
	if ((rval = RegQueryValueEx(Key, TEXT("Server"), NULL, &type, buffer, &cbData)) == ERROR_SUCCESS)
	{
		strncpy(target_ip, (const char*)buffer, sizeof(target_ip));
	}

	GetRegistryDword(Key, &control_port, TEXT("ControlPort"));
	GetRegistryDword(Key, &main_port, TEXT("VideoPort"));
	GetRegistryDword(Key, &second_port, TEXT("MjpegPort"));
	GetRegistryDword(Key, &audio_port, TEXT("AudioPort"));
	GetRegistryDword(Key, &motion_port, TEXT("MotionPort"));
	GetRegistryDword(Key, &face_port, TEXT("FacePort"));
}

////////////////////////////////////////////////////////////////////////
//
// CAmbaDemo
//
////////////////////////////////////////////////////////////////////////
CUnknown * WINAPI CAmbaDemo::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	if (G_refCount == 0)
	{
		GetRegistrySettings();
	}

	CUnknown *punk = new CAmbaDemo(lpunk, phr);
	if (FAILED(*phr))
	{
		delete punk;
		punk = NULL;
	}
	else if (punk == NULL)
	{
		*phr = E_OUTOFMEMORY;
	}

	return punk;
}

#define NUM_PINS		4

CAmbaDemo::CAmbaDemo(LPUNKNOWN lpunk, HRESULT *phr) :
	inherited(NAME("Amba demo"), lpunk, CLSID_A5S_AMBADEMO)
{
	if (FAILED(*phr))
		return;

	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != NO_ERROR) {
		Error("WSAStartup() failed", "WSAStartup");
		*phr = E_FAIL;
		return;
	}

	if (G_pRecordControl == NULL)
	{
		G_pRecordControl = new CAmbaRecordControl(NULL, phr);
		if (G_pRecordControl == NULL || FAILED(*phr)) {
			*phr = E_OUTOFMEMORY;
			return;
		}
	}
	if (G_pMuxerParam == NULL)
	{
		G_pMuxerParam = new MUXER_PARAM();
	}

	G_refCount++;

	m_paStreams = (CSourceStream **) new CAmbaStream*[NUM_PINS];
	if(m_paStreams == NULL)
		goto End;

	for (int i = 0; i < NUM_PINS; i++)
		m_paStreams[i] = NULL;

	m_paStreams[0] = new CAmbaVideoStream(phr, this, L"Video (Main)", main_port, 0, TRUE);
	if(m_paStreams[0] == NULL || FAILED(*phr))
		goto End;

	m_paStreams[1] = new CAmbaVideoStream(phr, this, L"Video (Secondary)", second_port, 1, FALSE);
	if(m_paStreams[1] == NULL || FAILED(*phr))
		goto End;

	m_paStreams[2] = new CAmbaAudioStream(phr, this, L"Audio", audio_port);
	if (m_paStreams[2] == NULL || FAILED(*phr))
		goto End;

	m_paStreams[3] = new CAmbaMotionDataStream(phr, this, L"MotionData", motion_port); 
	if (m_paStreams[3] == NULL || FAILED(*phr))
		goto End;

	m_paStreams[4] = new CAmbaFaceDataStream(phr, this, L"FaceData", face_port); 
	if (m_paStreams[4] == NULL || FAILED(*phr))
		goto End;

	main_port += 2;
	second_port += 2;
	audio_port += 2;
	motion_port += 2;
	face_port+=2;

	return;

End:
	*phr = E_OUTOFMEMORY;
}

CAmbaDemo::~CAmbaDemo()
{
	if (--G_refCount == 0) {
		delete G_pRecordControl;
		G_pRecordControl = NULL;
		delete G_pMuxerParam;
		G_pMuxerParam = NULL;
	}
}

STDMETHODIMP CAmbaDemo::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_ISpecifyPropertyPages)
		return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);

	if (riid == IID_IAmbaPlatformInfo)
		return GetInterface(static_cast<IAmbaPlatformInfo*>(this), ppv);

	return inherited::NonDelegatingQueryInterface(riid, ppv);
}

STDMETHODIMP CAmbaDemo::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
	UNREFERENCED_PARAMETER(dwMSecs);
	CheckPointer(State,E_POINTER);
	ValidateReadWritePtr(State,sizeof(FILTER_STATE));

	*State = m_State;
	if (m_State == State_Paused)
	{
		TRACE("Cannot cue\n");
		return VFW_S_CANT_CUE;
	}

	return S_OK;
}

static GUID AmbaPages[] =
{
	CLSID_EncodeFormat,
	CLSID_EncodeParam,
	CLSID_ImgParam,
	CLSID_MdParam,
	CLSID_MuxerParam,
	CLSID_AmbaAbout
};

STDMETHODIMP CAmbaDemo::GetPages (CAUUID *pages)
{
	int page_num = sizeof(AmbaPages) / sizeof(AmbaPages[0]);

	if (pages == NULL)
		return E_POINTER;

	/* Allocate memory for GUIDs */
	pages->cElems = page_num;
	pages->pElems = (GUID *) ::CoTaskMemAlloc (sizeof(GUID) * page_num);
	if (pages->pElems == NULL)
	{
		return E_OUTOFMEMORY;
	}

	/* Assign GUIDs */
	for (int i = 0; i < page_num; i++)
	{
		pages->pElems[i] = AmbaPages[i];
	}

	return S_OK;
}

STDMETHODIMP_(DWORD) CAmbaDemo::GetChannelCount()
{
	IAmbaRecordControl::INFO *pInfo;
	G_pRecordControl->GetInfo(&pInfo);
	return pInfo->video_channels;
}

////////////////////////////////////////////////////////////////////////
//
// CAmbaStream
//
////////////////////////////////////////////////////////////////////////
CAmbaStream::CAmbaStream(HRESULT *phr,
						 CAmbaDemo *pParent,
						 LPCWSTR pPinName, 
						 short port):
	CSourceStream(NAME("Amba Demo"), phr, pParent, pPinName),
	m_port(port),
	m_total_bytes(0),
	m_total_frames(0),
	m_isomuxer(NULL), 
	m_pClock(NULL)
{
}

CAmbaStream::~CAmbaStream()
{
	if (m_pClock != NULL)
		m_pClock->Release();
}

STDMETHODIMP CAmbaStream::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IAmbaPinType)
		return GetInterface(static_cast<IAmbaPinType*>(this), ppv);

	return inherited::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CAmbaStream::DecideAllocator(IMemInputPin * pin, IMemAllocator **alloc)
{
	HRESULT hr;
	ALLOCATOR_PROPERTIES	prop;

	/* Get allocator requirement */
	memset (&prop, 0, sizeof (prop));
	pin->GetAllocatorRequirements (&prop);

	if (prop.cbAlign == 0) {
		prop.cbAlign = 1;
	}

	/* Get our allocator */
	hr = InitAllocator (alloc);
	if (SUCCEEDED(hr)) {
		hr = DecideBufferSize (*alloc, &prop);
		if (SUCCEEDED(hr)) {
			hr = pin->NotifyAllocator (*alloc, FALSE);
			if (SUCCEEDED(hr)) {
				return NOERROR;
			}
		}
	}

	/* Likewise we may not have an interface to release */
	if (*alloc) {
		(*alloc)->Release ();
		*alloc = NULL;
	}

	return hr;
}

HRESULT CAmbaStream::FillBuffer(IMediaSample *pms)
{
	CheckPointer(pms,E_POINTER);
	CAutoLock lock(&m_stat_lock);

	HRESULT hr;
	BYTE *pbuffer;
	hr = pms->GetPointer(&pbuffer);
	if (FAILED(hr))
		return hr;

	FRAME_INFO info;
	hr = ReceiveBuffer(m_socket, (char *)&info, sizeof(info));
	if (FAILED(hr)) {
		if (m_shutdown)
			return S_FALSE;
		return hr;
	}

/*
	if (m_pin_info.enc_type == IAmbaRecordControl::ENC_AUDIO)
		ASSERT(info.size < AUDIO_BUF_SIZE);
	else if (m_pin_info.enc_type == IAmbaRecordControl::ENC_MJPEG)
		ASSERT(info.size < MJPEG_BUF_SIZE);
	else
		ASSERT(info.size < H264_BUF_SIZE);
*/

	hr = ReceiveBuffer(m_socket, (char *)pbuffer, info.size);
	if (FAILED(hr)) {
		if (m_shutdown)
			return S_FALSE;
		return hr;
	}

	if(m_isomuxer != NULL) {																	   
		m_isomuxer->ProcessVideoData(&info,pbuffer);					
	}  

	pms->SetSyncPoint(info.bSync ? TRUE : FALSE);
	pms->SetActualDataLength(info.size);

	REFERENCE_TIME startTime;
	REFERENCE_TIME endTime;
	if (SUCCEEDED(CalcSampleTime(info, startTime, endTime)))
		pms->SetTime(&startTime, &endTime);

	m_stat_lock.Lock();

	m_total_bytes += info.size;
	m_total_frames++;
	m_end_tick = ::GetTickCount();

	m_stat_lock.Unlock();

	return NOERROR;
}

HRESULT CAmbaStream::Active(void)
{
	TRACE("Active\n");
	return inherited::Active();
}

HRESULT CAmbaStream::Inactive(void)
{
	TRACE("Inactive\n");
	m_shutdown = TRUE;
	closesocket(m_socket);
	//shutdown(m_socket, SD_BOTH);
	return inherited::Inactive();
}

HRESULT CAmbaStream::OnThreadCreate()
{
	InitTdiffs();

	m_shutdown = FALSE;

	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET) {
		Error("socket() error", "socket");
		return E_FAIL;
	}

	sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = inet_addr(target_ip);
	client.sin_port = htons(m_port);

	if (connect(m_socket, (SOCKADDR*)&client, sizeof(client)) == SOCKET_ERROR) {
		Error("Connection failed", "Connect");
		return E_FAIL;
	}

	m_total_bytes = 0;
	m_total_frames = 0;
	m_start_tick = ::GetTickCount();
	m_end_tick = ::GetTickCount();

	return NOERROR;
}

HRESULT CAmbaStream::OnThreadDestroy(void)
{
	if(m_isomuxer != NULL) {
		m_isomuxer->Stop();
	}
	if (!m_shutdown) {
		TRACE("shutdown\n");
		closesocket(m_socket);
	}

	return S_OK;
}

HRESULT CAmbaStream::CalcSampleTime(FRAME_INFO& info, 
	REFERENCE_TIME& startTime, REFERENCE_TIME& endTime)
{
	startTime = info.pts_high;
	startTime <<= 32;
	startTime += info.pts;
	startTime = (startTime * 1000) / 9;

	GetTdiff(info, startTime);

	endTime = startTime + m_AvgTimePerFrame;
	endTime = startTime;

	return S_OK;
}

void CAmbaStream::GetClockTime(REFERENCE_TIME& time)
{
	if (m_pClock == NULL) {
		if (FAILED(m_pFilter->GetSyncSource(&m_pClock))) {
			TRACE("No reference clock\n");
			return;
		}
	}

	m_pClock->GetTime(&time);
}

////////////////////////////////////////////////////////////////////////
//
// CAmbaVideoStream
//
////////////////////////////////////////////////////////////////////////
CAmbaVideoStream::CAmbaVideoStream(HRESULT *phr, CAmbaDemo *pParent, 
	LPCWSTR pPinName, short tcp_port, int index, BOOL bMain):
	inherited(phr, pParent, pPinName, tcp_port),
	m_videoIndex(index),
	m_bMain(bMain)
{
	ZeroMemory(&m_mpeg_video_info2, sizeof(m_mpeg_video_info2));
}

void CAmbaVideoStream::InitTdiffs()
{
	m_ntdiff = 0;
	m_tdiff_index = 0;
	m_tdiff = 0;
	m_bPrev = FALSE;
}

void CAmbaVideoStream::GetTdiff(FRAME_INFO& info, REFERENCE_TIME& time)
{
	if (!info.bSync) {
		time -= m_tdiff;
		return;
	}

	if (!m_bPrev) {
		m_prevTime = time;
		m_prevSysTime = time;
		GetClockTime(m_prevSysTime);
		m_bPrev = TRUE;
	} else {
		REFERENCE_TIME sysTime = time;
		GetClockTime(sysTime);

		m_tdiffs[m_tdiff_index] = (time - m_prevTime) - (sysTime - m_prevSysTime);
		m_tdiff_index++;
		m_tdiff_index %= NUM_TDIFF;
		if (m_ntdiff < NUM_TDIFF)
			m_ntdiff++;

		//m_prevTime = time;
		//m_prevSysTime = sysTime;

		int i;
		REFERENCE_TIME diff = 0;
		for (i = 0; i < m_ntdiff; i++)
			diff += m_tdiffs[i];
		m_tdiff = diff / m_ntdiff;

		TRACE("diff = %lld\n", m_tdiff * 9/1000);
		time -= m_tdiff;
	}
}

HRESULT CAmbaVideoStream::CheckMediaType(const CMediaType *pmt)
{
	CheckPointer(pmt, E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	HRESULT hr;
	IAmbaRecordControl::FORMAT *pFormat;

	hr = G_pRecordControl->GetFormat(&pFormat);
	if (FAILED(hr))
		return hr;

	int enc_type = (m_videoIndex == 0) ? 
		pFormat->main.enc_type : pFormat->secondary.enc_type;

	switch (enc_type) {
	case IAmbaRecordControl::ENC_H264:
		if (*pmt->Type() != MEDIATYPE_Video ||
			(*pmt->Subtype() != MEDIASUBTYPE_H264 && *pmt->Subtype() != MEDIASUBTYPE_AVC1) ||
			*pmt->FormatType() != FORMAT_MPEG2_VIDEO)
			return E_FAIL;
		return NOERROR;

	case IAmbaRecordControl::ENC_MJPEG:
		if (*pmt->Type() != MEDIATYPE_Video ||
			*pmt->Subtype() != MEDIASUBTYPE_MJPG ||
			*pmt->FormatType() != FORMAT_VideoInfo)
			return E_FAIL;
		return NOERROR;

	default:
		return E_FAIL;
	}
}

void CAmbaVideoStream::GetFrameDuration(int framerate)
{
	switch (framerate) {
	case 0: // 29.97
		m_AvgTimePerFrame = 333667;
		break;

	case 1: // 59.94
		m_AvgTimePerFrame = 166833;
		break;

	default:
		m_AvgTimePerFrame = 10000000/framerate;
		break;
	}
	m_mpeg_video_info2.hdr.AvgTimePerFrame = m_AvgTimePerFrame;
}

HRESULT CAmbaVideoStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	CheckPointer(pmt, E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	if (iPosition < 0)
		return E_INVALIDARG;

	HRESULT hr;
	IAmbaRecordControl::FORMAT *pFormat;

	hr = G_pRecordControl->GetFormat(&pFormat);
	if (FAILED(hr))
		return hr;

	IAmbaRecordControl::VIDEO_FORMAT *pVideoFormat;
	pVideoFormat = (m_videoIndex == 0) ? 
		&pFormat->main : &pFormat->secondary;

	switch (pVideoFormat->enc_type) {
	case IAmbaRecordControl::ENC_H264:
		{
			if (iPosition > 1)
				return VFW_S_NO_MORE_ITEMS;

			pmt->SetType(&MEDIATYPE_Video);

			if (iPosition == 0)
				pmt->SetSubtype(&MEDIASUBTYPE_H264);
			else
				pmt->SetSubtype(&MEDIASUBTYPE_AVC1);

			pmt->SetVariableSize();
			pmt->SetTemporalCompression(TRUE);
			pmt->SetFormatType (&FORMAT_MPEG2_VIDEO);

			BYTE *fmt_buf = pmt->AllocFormatBuffer(sizeof (KS_MPEGVIDEOINFO2));
			if (fmt_buf == NULL)
				return E_OUTOFMEMORY;

			GetFrameDuration(pFormat->vinFrate);

			KS_VIDEOINFOHEADER2	*vhdr = &m_mpeg_video_info2.hdr;
			KS_BITMAPINFOHEADER	*bmhdr = &vhdr->bmiHeader;
			bmhdr->biSize = sizeof(KS_BITMAPINFOHEADER);
			bmhdr->biWidth = pVideoFormat->width;
			bmhdr->biHeight = pVideoFormat->height;
			bmhdr->biCompression = MAKEFOURCC('a', 'v', 'c', '1');

			*((KS_MPEGVIDEOINFO2 *) fmt_buf) = m_mpeg_video_info2;
		}
		return NOERROR;

	case IAmbaRecordControl::ENC_MJPEG:
		{
			if (iPosition > 0)
				return VFW_S_NO_MORE_ITEMS;

			pmt->SetType(&MEDIATYPE_Video);
			pmt->SetSubtype (&MEDIASUBTYPE_MJPG);
			pmt->SetVariableSize ();
			pmt->SetTemporalCompression (FALSE);
			pmt->SetFormatType (&FORMAT_VideoInfo);

			BYTE *fmt_buf = pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
			if (fmt_buf == NULL)
				return E_OUTOFMEMORY;

			GetFrameDuration(pFormat->vinFrate);

			VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*)fmt_buf;

			ZeroMemory(vih, sizeof(*vih));
			vih->AvgTimePerFrame = m_AvgTimePerFrame;
			vih->dwBitRate = 1000;
			vih->dwBitErrorRate = 0;
			vih->AvgTimePerFrame = 0;

			BITMAPINFOHEADER *bmi = &vih->bmiHeader;
			bmi->biSize = sizeof(BITMAPINFOHEADER);
			bmi->biWidth = pVideoFormat->width;
			bmi->biHeight = pVideoFormat->height;
			bmi->biPlanes = 1;
			bmi->biBitCount = 24;
			bmi->biCompression = MAKEFOURCC('M','J','P','G');
			bmi->biSizeImage = 0;
		}
		return NOERROR;

	default:
		return E_FAIL;
	}
}

HRESULT CAmbaVideoStream::DecideBufferSize(IMemAllocator *pAlloc,
						 ALLOCATOR_PROPERTIES *pProperties)
{
	CheckPointer(pAlloc,E_POINTER);
	CheckPointer(pProperties,E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	HRESULT hr;
	IAmbaRecordControl::FORMAT *pFormat;

	hr = G_pRecordControl->GetFormat(&pFormat);
	if (FAILED(hr))
		return hr;

	int enc_type = (m_videoIndex == 0) ? 
		pFormat->main.enc_type : pFormat->secondary.enc_type;

	switch (enc_type) {
	case IAmbaRecordControl::ENC_H264:
		pProperties->cBuffers = 8;
		pProperties->cbBuffer = H264_BUF_SIZE;
		break;

	case IAmbaRecordControl::ENC_MJPEG:
		pProperties->cBuffers = 8;
		pProperties->cbBuffer = MJPEG_BUF_SIZE;
		break;

	default:
		return E_FAIL;
	}

	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties, &Actual);
	if (FAILED(hr))
		return hr;

	return NOERROR;
}

 //added by zy:for muxer   
HRESULT CAmbaVideoStream::OnThreadCreate()
{
	if (NOERROR != inherited::OnThreadCreate()) {
		return E_FAIL;
	}
			  
	if (G_pMuxerParam->MuxerType == 1
		|| G_pMuxerParam->MuxerType == 2
		|| G_pMuxerParam->MuxerType == 3) {
		m_isomuxer = new CamIsoMuxer(1,1);
		if (ME_NOT_SUPPORT == m_isomuxer->Init(m_videoIndex)) {
			return NOERROR;
		}
		if (ME_OK != m_isomuxer->Run()) {
			return E_FAIL;
		} else {
			return NOERROR;
		}
	} else {
		return NOERROR;
	}

}

////////////////////////////////////////////////////////////////////////
//
// CAmbaAudioStream
//
////////////////////////////////////////////////////////////////////////
CAmbaAudioStream::CAmbaAudioStream(HRESULT *phr, CAmbaDemo *pParent, 
		LPCWSTR pPinName, short tcp_port):
		inherited(phr, pParent, pPinName, tcp_port)
{
}

HRESULT CAmbaAudioStream::CheckMediaType(const CMediaType *pmt)
{
	if (*pmt->Type() != MEDIATYPE_Audio
		|| *pmt->Subtype() != MEDIASUBTYPE_PCM
		|| *pmt->FormatType() != FORMAT_WaveFormatEx) {
		return E_FAIL;
	}

	return NOERROR;
}

HRESULT CAmbaAudioStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	CheckPointer(pmt, E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	if (iPosition < 0)
		return E_INVALIDARG;

	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;

	HRESULT hr;
	IAmbaRecordControl::PARAM *pParam;

	hr = G_pRecordControl->GetParam(&pParam);
	if (FAILED(hr))
		return hr;

	pmt->SetType(&MEDIATYPE_Audio);
	pmt->SetSubtype(&MEDIASUBTYPE_PCM);
	pmt->SetSampleSize(1);
	pmt->SetTemporalCompression(FALSE);
	pmt->SetFormatType(&FORMAT_WaveFormatEx);

	BYTE *fmt_buf = pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
	if (fmt_buf == NULL)
		return E_OUTOFMEMORY;

	WAVEFORMATEX *wf = (WAVEFORMATEX*)fmt_buf;
	wf->wFormatTag = (WORD)pParam->audio.pcmFormat;
	wf->nChannels = (WORD)pParam->audio.nchannels;
	wf->wBitsPerSample = (WORD)pParam->audio.bitsPerSample;
	wf->nSamplesPerSec = pParam->audio.samplesPerSec;
	wf->nBlockAlign = (WORD)(pParam->audio.nchannels * 
					pParam->audio.bitsPerSample / 8);
	wf->nAvgBytesPerSec = wf->nSamplesPerSec * wf->nBlockAlign;
	wf->cbSize = 0;

	return NOERROR;
}

HRESULT CAmbaAudioStream::DecideBufferSize(IMemAllocator *pAlloc,
						 ALLOCATOR_PROPERTIES *pProperties)
{
	CheckPointer(pAlloc,E_POINTER);
	CheckPointer(pProperties,E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	pProperties->cBuffers = 16;
	pProperties->cbBuffer = AUDIO_BUF_SIZE;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAlloc->SetProperties(pProperties, &Actual);
	if (FAILED(hr))
		return hr;

	return NOERROR;
}

////////////////////////////////////////////////////////////////////////
//
// CAmbaMotionDataStream  (Deal with Motion Detetection Data)
//
////////////////////////////////////////////////////////////////////////
CAmbaMotionDataStream::CAmbaMotionDataStream(HRESULT *phr, CAmbaDemo *pParent, 
		LPCWSTR pPinName, short tcp_port):
		inherited(phr, pParent, pPinName, tcp_port)
{
}

HRESULT CAmbaMotionDataStream::CheckMediaType(const CMediaType *pmt)
{
	if (*pmt->Type() != MEDIATYPE_Text
		|| *pmt->Subtype() != MEDIASUBTYPE_DVD_SUBPICTURE
		|| *pmt->FormatType() != FORMAT_None) {
		return E_FAIL;
	}

	return NOERROR;
}

HRESULT CAmbaMotionDataStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	CheckPointer(pmt, E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	if (iPosition < 0)
		return E_INVALIDARG;

	//if (iPosition > 0)
	//	return VFW_S_NO_MORE_ITEMS;

	HRESULT hr;
	IAmbaRecordControl::PARAM *pParam;

	hr = G_pRecordControl->GetParam(&pParam);
	if (FAILED(hr))
		return hr;

	pmt->SetType(&MEDIATYPE_Text);
	pmt->SetSubtype(&MEDIASUBTYPE_DVD_SUBPICTURE);
	pmt->SetSampleSize(1);
	pmt->SetTemporalCompression(FALSE);
	pmt->SetFormatType(&FORMAT_None);

	BYTE *fmt_buf = pmt->AllocFormatBuffer(sizeof(MOTION_INFO)); 
	if (fmt_buf == NULL)
		return E_OUTOFMEMORY;

	MOTION_INFO *wf = (MOTION_INFO*)fmt_buf;
	wf->MotionStreamFlag = 0xdeadbeaf;
	
	return NOERROR;
}

HRESULT CAmbaMotionDataStream::DecideBufferSize(IMemAllocator *pAlloc,
						 ALLOCATOR_PROPERTIES *pProperties)
{
	CheckPointer(pAlloc,E_POINTER);
	CheckPointer(pProperties,E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	pProperties->cBuffers = 10;
	pProperties->cbBuffer = MOTION_BUF_SIZE;  
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAlloc->SetProperties(pProperties, &Actual);
	if (FAILED(hr))
		return hr;

	return NOERROR;
}


////////////////////////////////////////////////////////////////////////
//
// CAmbaFaceDataStream  (Deal with Face Detetection Data)
//
////////////////////////////////////////////////////////////////////////
CAmbaFaceDataStream::CAmbaFaceDataStream(HRESULT *phr, CAmbaDemo *pParent, 
		LPCWSTR pPinName, short tcp_port):
		inherited(phr, pParent, pPinName, tcp_port)
{
}

HRESULT CAmbaFaceDataStream::CheckMediaType(const CMediaType *pmt)
{
	if (*pmt->Type() != MEDIATYPE_Text
		|| *pmt->Subtype() != MEDIASUBTYPE_DVD_SUBPICTURE
		|| *pmt->FormatType() != FORMAT_None)
		return E_FAIL;

	return NOERROR;
}

HRESULT CAmbaFaceDataStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	CheckPointer(pmt, E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	if (iPosition < 0)
		return E_INVALIDARG;

	//if (iPosition > 0)
	//	return VFW_S_NO_MORE_ITEMS;

	HRESULT hr;
	IAmbaRecordControl::PARAM *pParam;

	hr = G_pRecordControl->GetParam(&pParam);
	if (FAILED(hr))
		return hr;

	pmt->SetType(&MEDIATYPE_Text);
	pmt->SetSubtype(&MEDIASUBTYPE_DVD_SUBPICTURE);
	pmt->SetSampleSize(1);
	pmt->SetTemporalCompression(FALSE);
	pmt->SetFormatType(&FORMAT_None);

	BYTE *fmt_buf = pmt->AllocFormatBuffer(sizeof(FACE_INFO)); 
	if (fmt_buf == NULL)
		return E_OUTOFMEMORY;

	FACE_INFO *wf = (FACE_INFO*)fmt_buf;
	wf->FaceStreamFlag = 0xdeadbeaf;
	
	return NOERROR;
}

HRESULT CAmbaFaceDataStream::DecideBufferSize(IMemAllocator *pAlloc,
						 ALLOCATOR_PROPERTIES *pProperties)
{
	CheckPointer(pAlloc,E_POINTER);
	CheckPointer(pProperties,E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	pProperties->cBuffers = 10;
	pProperties->cbBuffer = FACE_BUF_SIZE;  
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAlloc->SetProperties(pProperties, &Actual);
	if (FAILED(hr))
		return hr;

	return NOERROR;
}

