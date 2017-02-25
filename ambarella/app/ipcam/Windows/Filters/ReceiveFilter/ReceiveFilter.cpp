// ReceiveFilter.cpp : Defines the entry point for the application.
//

#include <winsock2.h>

#include <fcntl.h>	 //for open O_* flags

#include <stdlib.h>	//for malloc/free
#include <string.h>	//for strlen/memset
#include <stdio.h>	 //for printf
#include <time.h>
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <assert.h>
#include <time.h>


#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <Ks.h>
#include <KsMedia.h>

#include "resource.h"
#include "ambaReceive_uids.h"
#include "ReceiveType.h"
#include "ReceiveFilter.h"
#include "mp4muxer.h"
#include <strsafe.h>
#pragma warning(disable:4995)

#ifdef DEBUG
#define Error(msg, title)   MessageBox(0, msg, title, MB_OK)
#ifdef	UNICODE
#define sprintf _swprintf
void TRACE(STRSAFE_LPCWSTR  fmt, ...)
{
	 LPTSTR out = new TCHAR[1024];
	 va_list body;
	 va_start(body, fmt);
	 StringCchVPrintf(out, 1024, fmt, body);
	 va_end(body);
	 OutputDebugString(out);
	 delete[] out;
}
#else
#define sprintf sprintf
void TRACE(STRSAFE_LPCSTR  fmt, ...)
{
	 LPTSTR out = new TCHAR[1024];
	 va_list body;
	 va_start(body, fmt);
	 StringCchVPrintf(out, 1024, fmt, body);
	 va_end(body);
	 OutputDebugString(out);
	 delete[] out;
}
#endif
#else
#define Error(msg, title)
#define TRACE
#endif

// Setup data

const AMOVIESETUP_FILTER sudAmbaReceiveDemoax =
{
	&CLSID_AMBARECEIVE_DEMO,		// Filter CLSID
	L"Ambarella A5S IPCamera(tcp client) Filter",// String name
	MERIT_DO_NOT_USE,	   // Filter merit
	0,
	NULL
};


// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {
// filter
	{ L"Amba A5S TCP Client"
	, &CLSID_AMBARECEIVE_DEMO
	, CAmbaTcpClientFilter::CreateInstance
	, NULL
	, &sudAmbaReceiveDemoax }

	, {L"Muxer Param"
	, &CLSID_MuxerParam
	, CAmbaMuxerParamProp::CreateInstance
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
	fd_set rfd;
	struct timeval timeout;
	timeout.tv_sec=5;
	timeout.tv_usec=0;
	FD_ZERO(&rfd);
	FD_SET(socket,&rfd);
	switch (select((int)socket+1,&rfd,0,0, &timeout))
	{
		case -1:
			return E_FAIL;
			break;
		case 0:
			return E_FAIL;
			break;
		default:
			if(FD_ISSET(socket,&rfd)) {
				while (size > 0) {
					int iResult = ::recv(socket, ptr, size, 0);
					TRACE("size=%d,recv=%d\n",size,iResult);
					if (iResult < 0)
					{
						TRACE("recv error: %d\n", WSAGetLastError());
						return E_FAIL;
					}
					else if (iResult == 0)
					{
						TRACE("connection has been gracefully closed\n");
						return E_FAIL;
					}
					ptr += iResult;
					size -= iResult;
				}
			}else{
					return E_FAIL;
			}
	}
	return S_OK;
}

#define main_port 2000
////////////////////////////////////////////////////////////////////////
//
// CAmbaTcpClientFilter
//
////////////////////////////////////////////////////////////////////////
#define NUM_PINS	1
#define CONFIG_FILE "TcpClient.ini"
CAmbaTcpClientFilter::CAmbaTcpClientFilter( LPUNKNOWN pUnk, HRESULT *phr) :
	inherited(NAME("Amba TCP Client"), pUnk, CLSID_AMBARECEIVE_DEMO)
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

	m_paStreams = (CSourceStream **) new CAmbaTCPStream*[NUM_PINS];
	if(m_paStreams == NULL)
		goto End;

	for (int i = 0; i < NUM_PINS; i++)
		m_paStreams[i] = NULL;

	m_paStreams[0] = new CAmbaTCPStream(phr, this, L"Video", main_port);
	if(m_paStreams[0] == NULL || FAILED(*phr))
		goto End;
	SetConfItems();
	LoadConfig();
	return;

End:
	*phr = E_OUTOFMEMORY;
}

CUnknown * WINAPI CAmbaTcpClientFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	ASSERT(phr);

	CAmbaTcpClientFilter *pNewObject = new CAmbaTcpClientFilter(pUnk, phr);
	if (pNewObject == NULL) {
       	if (phr)
            		*phr = E_OUTOFMEMORY;
    	}

     	return pNewObject;

}

STDMETHODIMP CAmbaTcpClientFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_ISpecifyPropertyPages)
	{
		return GetInterface(( ISpecifyPropertyPages*) (this), ppv);
	}
	else if( riid == IID_IAmbaRecord)
	{
		return GetInterface((IAmbaRecord *) this, ppv);
	}
	else if( riid == IID_IAmbaCom)
	{
		return GetInterface((IAmbaCom*) this, ppv);
	}
	return inherited::NonDelegatingQueryInterface(riid, ppv);
}


static GUID AmbaPages[] =
{
	CLSID_MuxerParam,
};

STDMETHODIMP CAmbaTcpClientFilter::GetPages (CAUUID *pages)
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


CAmbaTcpClientFilter::~CAmbaTcpClientFilter()
{
	SaveConfig();
	delete m_groups[0];
}

STDMETHODIMP CAmbaTcpClientFilter::SetConfItems()
{
	m_groups[0] = new items_s("Record");
	m_groups[0]->add_item("rec_type");
	m_groups[0]->add_item("max_size");
	m_groups[0]->add_item("file_name");
	return NO_ERROR;
}

STDMETHODIMP CAmbaTcpClientFilter::LoadConfig()
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
							((IAmbaRecord *)this)->SetRecordFilename(value);
						} else if ( (one_item = one_item->next_item) &&
							(0 == strcmp(item,one_item->m_item)) ){
							((IAmbaRecord *)this)->SetRecordFileMaxsize(atoi(value));
						} else if ( (one_item = one_item->next_item) &&
							(0 == strcmp(item,one_item->m_item)) ){
							((IAmbaRecord *)this)->SetRecordType(atoi(value));
						}
					}
				}
			}
			fclose(fd);
		}
	}
	return NO_ERROR;
}

STDMETHODIMP CAmbaTcpClientFilter::SaveConfig()
{
	char config_file[64];
	char* config_dir = getenv("ProgramFiles");
	if (config_dir) {
		sprintf_s(config_file, "%s\\Ambarella\\%s",config_dir,CONFIG_FILE);
		//lock
		HANDLE hMutex = CreateMutex(
							NULL,                        // default security descriptor
							FALSE,                       // mutex not owned
							TEXT("TcpConfigMutex"));  // object name

		if (GetLastError() == ERROR_ALREADY_EXISTS)
			hMutex =  OpenMutex(
				MUTEX_ALL_ACCESS,            // request full access
				FALSE,                       // handle not inheritable
				TEXT("TcpConfigMutex"));
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
				char file_name[64];
				((IAmbaRecord *)this)->GetRecordFilename(file_name);
				sprintf(line,"%s = %s\n",one_item->m_item,file_name);
				fputs(line,fd);
			}
			if (NULL != (one_item = one_item->next_item)) {
				unsigned int max_size;
				((IAmbaRecord *)this)->GetRecordFileMaxsize(&max_size);
				sprintf(line,"%s = %d\n",one_item->m_item,max_size);
				fputs(line,fd);
			}

			if (NULL != (one_item = one_item->next_item)) {
				short rec_type;
				((IAmbaRecord *)this)->GetRecordType(&rec_type);
				sprintf(line,"%s = %d\n",one_item->m_item,rec_type);
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

////////////////////////////////////////////////////////////////////////
//
// CAmbaTCPStream
//
////////////////////////////////////////////////////////////////////////
CAmbaTCPStream::CAmbaTCPStream(HRESULT *phr,
						 CAmbaTcpClientFilter *pParent,
						 LPCWSTR pPinName,
						 short port):
	CSourceStream(NAME("Amba Receive Demo"), phr, pParent, pPinName)
	,m_stream_id(0)
	,m_port(port)
	,m_socket(-1)
	,m_total_bytes(0)
	,m_total_frames(0)
	,m_connect_status(false)
	,m_mp4muxer(NULL)
	,m_IAmbaRecord(NULL)
	,m_totle_size(0)
	,m_nal_unit_type(0)
	,m_record_handle(0)
	,m_record_totle_frames(0)
	,m_maxfilesize(2048)
	,m_rectype (REC_ORG)
	,m_last_pts(0)
	,m_stat_size(DEFAULT_STAT_WINDOW_SIZE)
{
	m_format.encode_type = 0;
	m_format.encode_width = 0;
	m_format.encode_height = 0;
	m_format.frame_interval= 0;
	ZeroMemory(&m_mpeg_video_info2, sizeof(m_mpeg_video_info2));
	strncpy(m_hostname, DEFAULT_HOST, strlen(DEFAULT_HOST)+1);
	InitializeCriticalSection(&m_stat_cs);
	m_stat.begin_time = 0;
	m_stat.frame_cnt = 0;
	m_stat.totle_size = 0;
	m_stat.totle_pts = 0;
	m_stat.last_pts = 0;
	m_stat.window_size = m_stat_size;
	m_stat.old_time = NULL;
	m_stat.old_size = NULL;
	m_stat.old_pts = NULL;
}

CAmbaTCPStream::~CAmbaTCPStream()
{
	if (m_stat.old_time)
	{
		delete[] m_stat.old_time;
	}
	if (m_stat.old_size)
	{
		delete[] m_stat.old_size;
	}
	if (m_stat.old_pts)
	{
		delete[] m_stat.old_pts;
	}
}

STDMETHODIMP CAmbaTCPStream::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if( riid == IID_IAmbaPin)
	{
		return GetInterface((IAmbaPin *) this, ppv);
	}
	return inherited::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CAmbaTCPStream::DecideAllocator(IMemInputPin * pin, IMemAllocator **alloc)
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


HRESULT CAmbaTCPStream::FillBuffer(IMediaSample *pms)
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
	TRACE("info.size=%d\n",info.size);
	if (FAILED(hr)) {
		closesocket(m_socket);
		m_connect_status = false;
		return S_FALSE;
	}

	hr = ReceiveBuffer(m_socket, (char *)pbuffer, info.size);
	if (FAILED(hr)) {
		closesocket(m_socket);
		m_socket = -1;
		m_connect_status = false;
		return S_FALSE;
	}

	pms->SetSyncPoint(info.pic_type == 1 ? TRUE : FALSE);
	pms->SetActualDataLength(info.size);

	/*m_stat_lock.Lock();

	m_total_bytes += info.size;
	m_total_frames++;

	m_stat_lock.Unlock();*/

		DWORD curTime = timeGetTime();
		DWORD passTime = 0;
		int curPTS = info.pts - m_stat.last_pts;
		if (curPTS < 0)
		{
			curPTS += 1<<30;
		}
		if (m_stat.frame_cnt == 0)
		{
			m_stat.begin_time = curTime;
			REFERENCE_TIME begin = 0;
			REFERENCE_TIME end = 333670;
			pms->SetTime(&begin,&end);
		}
		else
		{
			int stat_idx = (m_stat.frame_cnt - 1) % m_stat.window_size;
			if (m_stat.frame_cnt <= m_stat.window_size)
			{
				passTime = curTime - m_stat.begin_time;
				if (passTime == 0)
				{
					passTime = 1;
				}
				m_stat.totle_size += info.size;
				m_stat.totle_pts += curPTS;
				EnterCriticalSection(&m_stat_cs);
				m_avg_fps = 1.0f * m_stat.frame_cnt/passTime * 1000;
				m_avg_bitrate = (int)((1.0f * m_stat.totle_size * 8 / 1024 )/(1.0f * passTime / 1000));
				m_avg_pts = (int)(1.0f * m_stat.totle_pts / m_stat.frame_cnt + 0.5f); //round
				LeaveCriticalSection(&m_stat_cs);
			}
			else
			{
				passTime = curTime - m_stat.old_time[stat_idx];
				m_stat.totle_size += info.size - m_stat.old_size[stat_idx];
				m_stat.totle_pts += curPTS - m_stat.old_pts[stat_idx];
				EnterCriticalSection(&m_stat_cs);
				m_avg_fps = 1.0f * m_stat.window_size/passTime * 1000;
				m_avg_bitrate = (int)((1.0f * m_stat.totle_size * 8 / 1024 )/(1.0f * passTime / 1000));
				m_avg_pts = (int)(1.0f * m_stat.totle_pts / m_stat.window_size + 0.5f); //round
				LeaveCriticalSection(&m_stat_cs);
			}
			m_stat.old_time[stat_idx] = curTime;
			m_stat.old_size[stat_idx] = info.size;
			m_stat.old_pts[stat_idx] = curPTS;
		}
		m_stat.frame_cnt++;
		m_stat.last_pts = info.pts;

	bool bRecordstatus = false;
	m_IAmbaRecord->GetRecordStatus(&bRecordstatus);
	if (bRecordstatus)
	{
		if ( m_record_handle <= 0 &&
			( (m_format.encode_type == ENC_H264 && info.pic_type == 1) ||
			(m_format.encode_type != ENC_H264) ) ) //start recording
		{
	 		open_record_file();
		}
		if (m_record_handle > 0 )
		{
			if (m_totle_size > (m_maxfilesize<<20) && (info.pic_type == 1)) // or wait till next IDR
			{
				close_record_file();
				open_record_file();
			}
			int nLength = write_record_file(pbuffer, info.size);
			if (nLength <= 0)
			{
				return E_UNEXPECTED;
			}
			else
			{
				if (m_mp4muxer != NULL)
				{
					m_mp4muxer->UpdateIdx(info.pts, (unsigned int)nLength, m_totle_size-nLength);
					if (info.pic_type == 1) //IDR idx
					{
						m_mp4muxer->UpdateIdx2(m_record_totle_frames);
					}
				}
			}
		}
	}
	else
	{
		if ( m_record_handle > 0 ) //stop recording
		{
			close_record_file();
		}
	}

	return NOERROR;
}


HRESULT CAmbaTCPStream::SetStreamId( int stream_id)
{
	m_stream_id = stream_id;
	return NOERROR;
}

HRESULT CAmbaTCPStream::SetHostname(const char* hostname)
{
	strncpy(m_hostname,hostname,strlen(hostname)+1);
	return NOERROR;
}

HRESULT CAmbaTCPStream::ConnectServer()
{
	if (m_connect_status) {
		return NO_ERROR;
	}
	HRESULT hr;
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET) {
		int errCode = WSAGetLastError();
		Error("socket() error", "socket");
			return E_FAIL;
	}

	sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = inet_addr(m_hostname);
	client.sin_port = htons(m_port);

	if (connect(m_socket, (SOCKADDR*)&client, sizeof(client)) == SOCKET_ERROR) {
		Error("Connection failed", "Connect");
		return E_FAIL;
	}
	hr = SendBuffer(m_socket, (char *)&m_stream_id, sizeof(int));
	if (FAILED(hr)) {
		return E_FAIL;
	}

	m_format.encode_type = ENC_NONE;
	m_format.encode_width =0;
	m_format.encode_height = 0;
	m_format.frame_interval= 1;

	hr = ReceiveBuffer(m_socket,(char *)&m_format,sizeof(format_t));
	if (FAILED(hr)) {
		closesocket(m_socket);
		m_socket = -1;
		return E_FAIL;
	}
	if ( !m_format.encode_type) {
		closesocket(m_socket);
		m_socket = -1;
		return E_FAIL;
	}

	m_connect_status = true;
	return NOERROR;

}

HRESULT CAmbaTCPStream::DisconnectServer( )
{
	if (m_connect_status)
	{
		closesocket(m_socket);
		m_socket = -1;
		m_connect_status = false;
	}
	return NOERROR;
}

HRESULT CAmbaTCPStream::Active(void)
{
	TRACE("Active\n");
	return inherited::Active();
}

HRESULT CAmbaTCPStream::Inactive(void)
{
	TRACE("Inactive\n");
	return inherited::Inactive();

}

HRESULT CAmbaTCPStream::OnThreadCreate()
{
	/*m_total_bytes = 0;
	m_total_frames = 0;*/
	m_stat.begin_time = 0;
	m_stat.frame_cnt = 0;
	m_stat.totle_size = 0;
	m_stat.totle_pts = 0;
	m_stat.last_pts = 0;

	if (m_stat.old_size)
	{
		delete[] m_stat.old_size;
	}
	if (m_stat.old_time)
	{
		delete[] m_stat.old_time;
	}
	if (m_stat.old_pts)
	{
		delete[] m_stat.old_pts;
	}
	m_stat.window_size = m_stat_size;
	m_stat.old_time = new DWORD[m_stat.window_size]();
	m_stat.old_size = new int[m_stat.window_size]();
	m_stat.old_pts = new int[m_stat.window_size]();

	timeBeginPeriod(1);
	EnterCriticalSection(&m_stat_cs);
	m_avg_fps = 0.0f;
	m_avg_bitrate = 0;
	m_avg_pts = 0;
	LeaveCriticalSection(&m_stat_cs);

	HRESULT hr = m_pFilter->QueryInterface(IID_IAmbaRecord, (void**)(&m_IAmbaRecord));
	if (FAILED(hr))
	{
		return E_NOINTERFACE;
	}
	else
	{
		hr = ConnectServer();
		m_IAmbaRecord->SetEncodeType(m_format.encode_type);
	}
	return hr;
}

HRESULT CAmbaTCPStream::OnThreadDestroy(void)
{
	DisconnectServer();
	close_record_file();
	if( m_IAmbaRecord != NULL )
	{
		m_IAmbaRecord->Release();
		m_IAmbaRecord = NULL;
	}
	timeEndPeriod(1);
	return S_OK;
}


/*void CAmbaTCPStream::GetClockTime(REFERENCE_TIME& time)
{
	if (m_pClock == NULL) {
		if (FAILED(m_pFilter->GetSyncSource(&m_pClock))) {
			TRACE("No reference clock\n");
			return;
		}
	}

	m_pClock->GetTime(&time);
}
*/


HRESULT CAmbaTCPStream::CheckMediaType(const CMediaType *pmt)
{
	CheckPointer(pmt, E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	int enc_type = 	m_format.encode_type;

	switch (enc_type) {
	case ENC_H264:
		if (*pmt->Type() != MEDIATYPE_Video ||
			(*pmt->Subtype() != MEDIASUBTYPE_H264 && *pmt->Subtype() != MEDIASUBTYPE_AVC1) ||
			*pmt->FormatType() != FORMAT_MPEG2_VIDEO)
			return E_FAIL;
		return NOERROR;

	case ENC_MJPEG:
		if (*pmt->Type() != MEDIATYPE_Video ||
			*pmt->Subtype() != MEDIASUBTYPE_MJPG ||
			*pmt->FormatType() != FORMAT_VideoInfo)
			return E_FAIL;
		return NOERROR;

	default:
		return E_FAIL;
	}
}

/*void CAmbaTCPStream::GetFrameDuration(int framerate)
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
}*/



HRESULT CAmbaTCPStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	CheckPointer(pmt, E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	if (iPosition < 0)
		return E_INVALIDARG;
	ConnectServer();

	int enc_type = m_format.encode_type;
	//AM_U32 vinFrate = m_format.framerate;

	switch (enc_type) {
	case ENC_H264:
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

			//GetFrameDuration(vinFrate);

			KS_VIDEOINFOHEADER2	*vhdr = &m_mpeg_video_info2.hdr;
			KS_BITMAPINFOHEADER	*bmhdr = &vhdr->bmiHeader;
			bmhdr->biSize = sizeof(KS_BITMAPINFOHEADER);

			bmhdr->biWidth = m_format.encode_width;
			bmhdr->biHeight = m_format.encode_height;

			bmhdr->biCompression = MAKEFOURCC('a', 'v', 'c', '1');

			*((KS_MPEGVIDEOINFO2 *) fmt_buf) = m_mpeg_video_info2;
		}
		return NOERROR;

	case ENC_MJPEG:
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

			//GetFrameDuration(vinFrate);
			VIDEOINFOHEADER *vih = (VIDEOINFOHEADER*)fmt_buf;
			ZeroMemory(vih, sizeof(*vih));
			//vih->AvgTimePerFrame = m_AvgTimePerFrame;
			vih->dwBitRate = 1000;
			vih->dwBitErrorRate = 0;
			vih->AvgTimePerFrame = 0;

			BITMAPINFOHEADER *bmi = &vih->bmiHeader;
			bmi->biSize = sizeof(BITMAPINFOHEADER);
			bmi->biWidth = m_format.encode_width;
			bmi->biHeight = m_format.encode_height;

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

HRESULT CAmbaTCPStream::DecideBufferSize(IMemAllocator *pAlloc,
						 ALLOCATOR_PROPERTIES *pProperties)
{
	CheckPointer(pAlloc,E_POINTER);
	CheckPointer(pProperties,E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	HRESULT hr;

	int enc_type = m_format.encode_type;
	switch (enc_type) {
	case ENC_H264:
		pProperties->cBuffers = 8;
		pProperties->cbBuffer = H264_BUF_SIZE;
		break;

	case ENC_MJPEG:
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

void CAmbaTCPStream::open_record_file()
{
	char * filename = new char[RECORD_FILENAME_LENGTH];
	m_IAmbaRecord->GetRecordFilename(filename);
	m_IAmbaRecord->GetRecordType(&m_rectype);
	char file_exname[8] = {'\0'};
	if (m_rectype == REC_ORG)
	{
		if ( m_format.encode_type == ENC_H264 )
		{
			strncpy(file_exname,".264",strlen(".264")+1);
		}
		else if (m_format.encode_type  == ENC_MJPEG)
		{
			strncpy(file_exname,".mjpeg",strlen(".mjpeg")+1);
		}
	}
	else
	{

		strncpy(file_exname,".mdat",strlen(".mdat")+1);
	}
	size_t tmp_len = 0;
	tmp_len = strlen(filename);
	switch (m_stream_id)
	{
		case 0:
			strncpy(filename+tmp_len,"_A",2);
			tmp_len += 2;
			break;
		case 1:
			strncpy(filename+tmp_len,"_B",2);
			tmp_len += 2;
			break;
		case 2:
			strncpy(filename+tmp_len,"_C",2);
			tmp_len += 2;
			break;
		case 3:
			strncpy(filename+tmp_len,"_D",2);
			tmp_len += 2;
			break;
		default:
			break;
	}

	time_t t = time(0);
	char tmp_time[64];
	sprintf_s(tmp_time, "_%04d%02d%02d%02d%02d%02d",(localtime(&t)->tm_year+1900),
		(localtime(&t)->tm_mon+1),
		localtime(&t)->tm_mday,
		localtime(&t)->tm_hour,
		localtime(&t)->tm_min,localtime(&t)->tm_sec);
	strncpy(filename+tmp_len,tmp_time,strlen(tmp_time)+1);

	if (m_rectype == REC_MP4 || m_rectype == REC_F4V)
	{
		if(m_mp4muxer == NULL)
		{
			m_mp4muxer = new CamMp4Muxer(filename);
			if (m_mp4muxer != NULL)
			{
				m_mp4muxer->Init(&m_format, (AM_U8)m_rectype);
			}
		}
	}

	tmp_len += strlen(tmp_time);
	strncpy(filename+ tmp_len, file_exname, strlen(file_exname)+1);
	char dir[RECORD_FILENAME_LENGTH];
	int end_of_dir=0;
	int i = 0;
	while(filename[i] != '\0')
	{
		if (filename[i] == '\\' || filename[i] == '/')
		{
			end_of_dir = i;
		}
		i++;
	}
	strncpy(dir, filename, end_of_dir);
	dir[end_of_dir] = '\0';
	if(end_of_dir != 0)
	{
		_mkdir(dir);
	}

	m_record_handle = ::_open(filename, O_RDWR|O_CREAT|O_TRUNC|_O_BINARY , 0644);

	delete[] filename;

	m_IAmbaRecord->GetRecordFileMaxsize(&m_maxfilesize);


	return ;
}

int CAmbaTCPStream::write_record_file(unsigned char* pBuffer, int nSize)
{
	int write_size = 0;
	if (m_format.encode_type == ENC_H264 &&
		(m_rectype == REC_MP4 || m_rectype == REC_F4V ) )
	{
		//write "mdat box" for mp4 file
		short nal_unit_type = 0;
		int nal_unit_length = 0;
		int cursor = 0;
		do
		{
			nal_unit_length = CParseH264::get_nal_unit_type_length(&nal_unit_type, pBuffer+cursor, nSize-cursor);
			if (nal_unit_length > 0 && nal_unit_type > 0 )
			{
				if (nal_unit_type ==  7) //not write sps
				{
					if (m_mp4muxer != NULL)
					{
						m_mp4muxer->UpdateSps(pBuffer+cursor+4, nal_unit_length -4);
					}
				}
				else if (nal_unit_type == 8) //not write pps
				{
					if (m_mp4muxer != NULL)
					{
						m_mp4muxer->UpdatePps(pBuffer+cursor+4, nal_unit_length -4);
					}
				}
				else
				{
					int length = nal_unit_length -4;
					unsigned char length_32[4];
					length_32[3] = (unsigned char)(length&0x000000FF);
					length_32[2] = (unsigned char)((length&0x0000FF00)>>8);
					length_32[1] = (unsigned char)((length&0x00FF0000)>>16);
					length_32[0] = (unsigned char)((length&0xFF000000)>>24);
					write_size += ::_write(m_record_handle, (unsigned char *)&length_32, 4); // write length
					write_size += ::_write(m_record_handle, pBuffer+cursor+4, length); //write data, nal_unit without startcode 0x00000001
				}
				cursor += nal_unit_length;
			}
			else
			{
				return 0;
			}
		}while (cursor < nSize);
	}
	else
	{
		write_size += ::_write(m_record_handle, pBuffer, nSize);
	}
	m_totle_size += write_size;
	m_record_totle_frames ++;
	return write_size;

}
void CAmbaTCPStream::close_record_file()
{
	if (m_rectype == REC_MP4 || m_rectype == REC_F4V)
	{
		if ( m_mp4muxer != NULL)
		{
			m_mp4muxer->Mux(&m_record_handle);
			delete m_mp4muxer;
			m_mp4muxer = NULL;
		}
	}
	if (m_record_handle > 0)
	{
		::_close(m_record_handle);
		m_record_handle = 0;
	}
	m_record_totle_frames = 0;
	m_totle_size  = 0;
	m_last_pts = 0;
	return;
}


////////////////////////////////////////////////////////////////////////
//
// CAmbaMuxerParamProp
//
////////////////////////////////////////////////////////////////////////
CUnknown * WINAPI CAmbaMuxerParamProp::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	CUnknown *punk = new CAmbaMuxerParamProp(lpunk, phr);
	if (punk == NULL && phr)
	{
		*phr = E_OUTOFMEMORY;
	}

	return punk;
}

CAmbaMuxerParamProp::CAmbaMuxerParamProp(LPUNKNOWN lpunk, HRESULT *phr):
inherited(NAME("RecordControlProp"), lpunk, IDD_DIALOG_MUXER_PARAM,IDS_PROPPAGE_MUXER_PARAM)
,m_IAmbaRecord(NULL)
{
	ASSERT(phr);
	*phr = S_OK;
}

HRESULT CAmbaMuxerParamProp::OnConnect(IUnknown *punk)
{
	HRESULT hr = punk->QueryInterface(IID_IAmbaRecord, (void**)&m_IAmbaRecord);

	if(FAILED(hr))
	{
		return E_NOINTERFACE;
	}
	return NO_ERROR;
}

HRESULT CAmbaMuxerParamProp::OnDisconnect()
{
	if (m_IAmbaRecord == NULL)
	{
		return E_NOINTERFACE;
	}
	m_IAmbaRecord->Release();
	m_IAmbaRecord = NULL;
	return S_OK;
}

HRESULT CAmbaMuxerParamProp::OnActivate()
{
	char* filename= new char[RECORD_FILENAME_LENGTH ];
	m_IAmbaRecord->GetRecordFilename(filename);
	::SetDlgItemText(m_hwnd,IDC_EDIT3,(LPTSTR)filename);
	delete[] filename;
	unsigned int maxfilesize = 2048;
	char strMaxfilesize[16];
	m_IAmbaRecord->GetRecordFileMaxsize(&maxfilesize);
	sprintf(strMaxfilesize, "%d",maxfilesize);
	::SetDlgItemText(m_hwnd,IDC_EDIT4,(LPTSTR)strMaxfilesize);

	EnableWindowById(IDC_RADIO4, TRUE);

	short enctype = ENC_NONE;
	m_IAmbaRecord->GetEncodeType(&enctype);
	if ( enctype == ENC_MJPEG)
	{
		EnableWindowById(IDC_RADIO2, FALSE);
		EnableWindowById(IDC_RADIO3, FALSE);
		::SetDlgItemText(m_hwnd,IDC_RADIO4,(LPTSTR)"mjpeg");
	}
	else if (enctype == ENC_H264)
	{
		EnableWindowById(IDC_RADIO2, TRUE);
		EnableWindowById(IDC_RADIO3, TRUE);
		::SetDlgItemText(m_hwnd,IDC_RADIO4,(LPTSTR)"h264");
	}
	short rectype = REC_ORG;
	m_IAmbaRecord->GetRecordType(&rectype);

	if (rectype == REC_MP4)
	{
		::SendDlgItemMessage(m_hwnd, IDC_RADIO2, BM_SETCHECK, BST_CHECKED, 0);
	} else if (rectype== REC_F4V)
	{
		::SendDlgItemMessage(m_hwnd, IDC_RADIO3, BM_SETCHECK, BST_CHECKED, 0);
	} else if (rectype == REC_ORG)
	{
		::SendDlgItemMessage(m_hwnd, IDC_RADIO4, BM_SETCHECK, BST_CHECKED, 0);
	}

	return S_OK;
}

HRESULT CAmbaMuxerParamProp::OnDeactivate()
{
	::KillTimer(m_hwnd, 1);
	return S_OK;
}

HRESULT CAmbaMuxerParamProp::OnApplyChanges()
{
	if ( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO2))
	{
		m_IAmbaRecord->SetRecordType(REC_MP4);
	}else if ( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO3))
	{
		m_IAmbaRecord->SetRecordType(REC_F4V);
	}else if ( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO4))
	{
		m_IAmbaRecord->SetRecordType(REC_ORG);
	}

	HWND hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT3);
	if (::IsWindowEnabled(hwnd))
	{
		char filename[RECORD_FILENAME_LENGTH];
		::GetDlgItemText(m_hwnd,IDC_EDIT3,(LPTSTR)filename,RECORD_FILENAME_LENGTH);
		m_IAmbaRecord->SetRecordFilename(filename);
		int size;
		char strSize[16];
		::GetDlgItemText(m_hwnd,IDC_EDIT4,(LPTSTR)strSize,16);
		size =  atoi((const char *)strSize);
		if (size <=0 )
		{
			MessageBox(0, "The maximum file size should between 1 and 2048 !", "Notice", MB_OK);
		}
		else
		{
			m_IAmbaRecord->SetRecordFileMaxsize(size);
		}
	}
	return S_OK;
}

INT_PTR CAmbaMuxerParamProp::OnReceiveMessage(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (HIWORD(wParam) ==  BN_CLICKED)
		{
			switch (LOWORD(wParam))
			{
			case IDC_BUTTON1:
				OnButtonBrowse();
				break;
			default:
				break;
			}

			SetDirty();
			return (LRESULT)1;
		}
		else if (HIWORD(wParam) == EN_CHANGE )
		{
			SetDirty();
			return (LRESULT)1;
		}
	}

	return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

void CAmbaMuxerParamProp::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
	{
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	}
}

void CAmbaMuxerParamProp::OnButtonBrowse()
{
	OPENFILENAME ofn;	   // common dialog box structure
	char szFile[256];	   // buffer for file name

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hwnd;
	ofn.lpstrFile = szFile;
//
// Set lpstrFile[0] to '\0' so that GetOpenFileName does not
// use the contents of szFile to initialize itself.
//
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	/*if( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO2))
	{
		ofn.lpstrFilter = "mp4(*.mp4)\0*.mp4\0All(*.*) \0*.*\0" ;
		ofn.lpstrDefExt = "mp4";
	}
	else if( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO3))
	{
		ofn.lpstrFilter = "f4v(*.f4v)\0*.f4v\0All(*.*) \0*.*\0" ;
		ofn.lpstrDefExt = "f4v";
	}
	else if( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO4))
	{
		if (m_pFormat->main.enc_type == IAmbaRecordControl::ENC_H264)
		{
			ofn.lpstrFilter = "H264(*.264)\0*.264\0All(*.*) \0*.*\0" ;
			ofn.lpstrDefExt = "264";
		}
		else if(m_pFormat->main.enc_type == IAmbaRecordControl::ENC_MJPEG)
		{
			ofn.lpstrFilter = "MJPEG(*.mjpeg)\0*.mjpeg\0All(*.*) \0*.*\0" ;
			ofn.lpstrDefExt = "mjpeg";
		}
	}*/
	ofn.lpstrFilter = NULL;
	ofn.lpstrDefExt = NULL;
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_OVERWRITEPROMPT;

// Display the Open dialog box.

	if (GetSaveFileName(&ofn)==TRUE)
	{
		::SetDlgItemText(m_hwnd,IDC_EDIT3,ofn.lpstrFile);
	}

}
int CAmbaMuxerParamProp::EnableWindowById(WORD id, BOOL enable)
{
	HWND hwnd = ::GetDlgItem(m_hwnd, id);
	return ::EnableWindow(hwnd, enable);
}


