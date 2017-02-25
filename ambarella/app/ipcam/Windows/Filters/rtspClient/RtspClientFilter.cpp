#include "AmbaRtspRecv.h"

#include <streams.h>
#include <initguid.h>
#include <fcntl.h>
#include <io.h>
#include <direct.h>
#include <time.h>
#include <assert.h>

#include "resource.h"
#include "AmbaRtspClient_uids.h"
#include "RtspClientFilter.h"
#include "Mp4Muxer.h"
#include <strsafe.h>

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



const AMOVIESETUP_FILTER sudAmbaRtspClientax =
{
	&CLSID_AMBARTSPCLIENT,		// Filter CLSID
	L"Ambarella A5S IPCamera(rtsp client) Filter",// String name
	MERIT_DO_NOT_USE,	   // Filter merit
	0,
	NULL
};


// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {
// filter
	{ L"Amba A5S RTSP Client"
	, &CLSID_AMBARTSPCLIENT
	, CAmbaRtspClientFilter::CreateInstance
	, NULL
	, &sudAmbaRtspClientax  }

	, {L"Record Param"
	, &CLSID_RECORDPARAM
	, CAmbaRecordParamProp::CreateInstance
	, NULL, NULL }

	, {L"Network"
	, &CLSID_NETWORK
	, CAmbaNetworkProp::CreateInstance
	, NULL, NULL }

	, {L"Play"
	, &CLSID_PLAY
	, CAmbaPlayProp::CreateInstance
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


////////////////////////////////////////////////////////////////////////
//
// CAmbaRtspClientFilter
//
////////////////////////////////////////////////////////////////////////
#define NUM_PINS	1
#define CONFIG_FILE "RtspClient.ini"

CAmbaRtspClientFilter::CAmbaRtspClientFilter( LPUNKNOWN pUnk, HRESULT *phr) :
	inherited(NAME("Amba RtspClient"), pUnk, CLSID_AMBARTSPCLIENT)
{
	if (FAILED(*phr))
		return;

	m_paStreams = (CSourceStream **) new CAmbaRtspStream*[NUM_PINS];
	if(m_paStreams == NULL)
		goto End;

	for (int i = 0; i < NUM_PINS; i++)
		m_paStreams[i] = NULL;

	m_paStreams[0] = new CAmbaRtspStream(phr, this, L"Video");
	if(m_paStreams[0] == NULL || FAILED(*phr))
		goto End;

	SetConfItems();
	LoadConfig();
	return;

End:
	*phr = E_OUTOFMEMORY;
}

CUnknown * WINAPI CAmbaRtspClientFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	ASSERT(phr);

	CAmbaRtspClientFilter *pNewObject = new CAmbaRtspClientFilter(pUnk, phr);
	if (pNewObject == NULL) {
       	if (phr)
			*phr = E_OUTOFMEMORY;
	}
	return pNewObject;

}

STDMETHODIMP CAmbaRtspClientFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_ISpecifyPropertyPages)
	{
		return GetInterface(( ISpecifyPropertyPages*) (this), ppv);
	}
	else if(riid == IID_IAmbaRecord)
	{
		return GetInterface((IAmbaRecord *) this, ppv);
	}
	else if(riid == IID_IAmbaCom)
	{
		return GetInterface((IAmbaCom*) this, ppv);
	}
	else if(riid == IID_IAmbaNetwork)
	{
		return GetInterface((IAmbaNetwork*) this, ppv);
	}
	else if(riid == IID_IAmbaPlay)
	{
		return GetInterface((IAmbaPlay*) this, ppv);
	}
	return inherited::NonDelegatingQueryInterface(riid, ppv);
}


static GUID AmbaPages[] =
{
	CLSID_RECORDPARAM,
	CLSID_NETWORK,
	CLSID_PLAY,
};

STDMETHODIMP CAmbaRtspClientFilter::GetPages (CAUUID *pages)
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


CAmbaRtspClientFilter::~CAmbaRtspClientFilter()
{
	SaveConfig();
	for (int i=0;i<3;i++) {
		delete m_groups[i];
	}
}

STDMETHODIMP CAmbaRtspClientFilter::SetConfItems()
{
	m_groups[0] = new items_s("Record");
	m_groups[0]->add_item("rec_type");
	m_groups[0]->add_item("max_size");
	m_groups[0]->add_item("file_name");

	m_groups[1] = new items_s("Network");
	m_groups[1]->add_item("trans_type");

	m_groups[2] = new items_s("Play");
	m_groups[2]->add_item("buffer_size");
	m_groups[2]->add_item("caching_time");
	return NO_ERROR;
}

STDMETHODIMP CAmbaRtspClientFilter::LoadConfig()
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
					} else if (0 == strcmp(group, m_groups[1]->m_item)){
						one_item = m_groups[1];
						if ( (one_item = one_item->next_item) &&
							(0 == strcmp(item,one_item->m_item)) ){
							((IAmbaNetwork *)this)->SetTransType(atoi(value));
						}
					} else if (0 == strcmp(group, m_groups[2]->m_item)) {
						one_item = m_groups[2];
						if ( (one_item = one_item->next_item) &&
							(0 == strcmp(item,one_item->m_item)) ){
							((IAmbaPlay *)this)->SetCachingTime(atoi(value));
						} else if ( (one_item = one_item->next_item) &&
							(0 == strcmp(item,one_item->m_item)) ){
							((IAmbaPlay *)this)->SetBufferSize(atoi(value));
						}
					}
				}
			}
			fclose(fd);
		}
	}
	return NO_ERROR;
}

STDMETHODIMP CAmbaRtspClientFilter::SaveConfig()
{
	char config_file[64];
	char* config_dir = getenv("ProgramFiles");
	if (config_dir) {
		sprintf_s(config_file, "%s\\Ambarella\\%s",config_dir,CONFIG_FILE);
		//lock
		HANDLE hMutex = CreateMutex(
							NULL,                        // default security descriptor
							FALSE,                       // mutex not owned
							TEXT("RtspConfigMutex"));  // object name

		if (GetLastError() == ERROR_ALREADY_EXISTS)
			hMutex =  OpenMutex(
				MUTEX_ALL_ACCESS,            // request full access
				FALSE,                       // handle not inheritable
				TEXT("RtspConfigMutex"));
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

			sprintf(line,"[%s]\n",m_groups[1]->m_item);
			fputs(line,fd);
			one_item = m_groups[1];
			if (NULL != (one_item = one_item->next_item)) {
				short trans_type;
				((IAmbaNetwork *)this)->GetTransType(&trans_type);
				sprintf(line,"%s = %d\n",one_item->m_item,trans_type);
				fputs(line,fd);
			}
			fputs("\n",fd);

			sprintf(line,"[%s]\n",m_groups[2]->m_item);
			fputs(line,fd);
			one_item = m_groups[2];
			if (NULL != (one_item = one_item->next_item)) {
				DWORD caching_time;
				((IAmbaPlay *)this)->GetCachingTime(&caching_time);
				sprintf(line,"%s = %d\n",one_item->m_item,caching_time);
				fputs(line,fd);
			}
			if (NULL != (one_item = one_item->next_item)) {
				short buffer_size;
				((IAmbaPlay *)this)->GetBufferSize(&buffer_size);
				sprintf(line,"%s = %d\n",one_item->m_item,buffer_size);
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
// CAmbaRecordStream
//
////////////////////////////////////////////////////////////////////////
CAmbaRecordStream::CAmbaRecordStream(IAmbaRecord* pIAmbaRecord,
				unsigned short encType,int streamId)
	:m_IAmbaRecord(pIAmbaRecord)
	,m_enc_type(encType)
	,m_streamId(streamId)
	,m_totle_size(0)
	,m_record_totle_frames(0)
	,m_record_handle(0)
	,m_maxfilesize(2048)
	,m_rectype(REC_ORG)
	,m_mp4muxer(NULL)
	,m_video_width(0)
	,m_video_height(0)
{
	m_IAmbaRecord->SetEncodeType(encType);
}

CAmbaRecordStream::~CAmbaRecordStream()
{
	close_record_file();
}

void CAmbaRecordStream::open_record_file()
{
	assert(m_IAmbaRecord);
	char * filename = new char[RECORD_FILENAME_LENGTH];
	m_IAmbaRecord->GetRecordFilename(filename);
	m_IAmbaRecord->GetRecordType(&m_rectype);
	char file_exname[8] = {'\0'};
	if (m_rectype == REC_ORG)
	{
		if ( m_enc_type == ENC_H264 )
		{
			strncpy(file_exname,".264",strlen(".264")+1);
		}
		else if (m_enc_type   == ENC_MJPEG)
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
	switch (m_streamId)
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
			m_mp4muxer = new CAmbaMp4muxer(filename);
			if (m_mp4muxer != NULL)
			{
				m_mp4muxer->Init((AM_U8)m_rectype);
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

int CAmbaRecordStream::write_record_file(unsigned char* pBuffer, int nSize)
{
	assert(m_IAmbaRecord);
	int write_size = 0;
	if (m_enc_type == ENC_H264 &&
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
					if (m_video_width == 0 && m_video_height == 0)
					{
						CParseH264::h264_param_t h264_param;
						CParseH264::get_pic_width_height(pBuffer+cursor+5, nal_unit_length-5, &h264_param);
						m_video_width = h264_param.pic_width;
						m_video_height = h264_param.pic_height;
					}
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

void CAmbaRecordStream::close_record_file()
{
	if (m_rectype == REC_MP4 || m_rectype == REC_F4V)
	{
		if ( m_mp4muxer != NULL)
		{
			h264_config_t h264_config = {m_video_width, m_video_height};
			m_mp4muxer->Config(h264_config);
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
	return;
}

int CAmbaRecordStream::record_file(unsigned char* pBuffer,int nSize, long long sample_delta, short nal_unit_type)
{
	bool bRecordstatus = false;
	m_IAmbaRecord->GetRecordStatus(&bRecordstatus);

	if (bRecordstatus)
	{
		if ( m_record_handle <= 0 &&
			( (m_enc_type == ENC_H264 && nal_unit_type == 5) ||
			(m_enc_type != ENC_H264) ) ) //start recording
		{
	 		open_record_file();
		}
		if (m_record_handle > 0 )
		{
			if (m_totle_size > (m_maxfilesize<<20)) // or wait till next IDR
			{
				close_record_file();
				open_record_file();
			}
			int nLength = write_record_file(pBuffer, nSize);
			if (nLength <= 0)
			{
				return 1;
			}
			else
			{
				if (m_mp4muxer != NULL)
				{
					m_mp4muxer->UpdateIdx(sample_delta*9/100, (unsigned int)nLength, m_totle_size-nLength);
					if (nal_unit_type == 5) //IDR idx
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
	return 0;
}

////////////////////////////////////////////////////////////////////////
//
// CAmbaRtspStream
//
////////////////////////////////////////////////////////////////////////
#define MAX_URL_LENGTH 256

//bool CAmbaRtspStream::m_timeup_flag = false;
void CALLBACK TimeProc(
  UINT uID,
  UINT uMsg,
  DWORD dwUser,
  DWORD dw1,
  DWORD dw2
)
{
	//if (uID == 1)
	//{
		DbgLog((LOG_TRACE, 1, TEXT("time is up %d"),timeGetTime()));
		HANDLE timeup_event = (HANDLE)dwUser;
		SetEvent(timeup_event);
	//}
}
CAmbaRtspStream::CAmbaRtspStream(HRESULT *phr,
						 CAmbaRtspClientFilter *pParent,
						 LPCWSTR pPinName):
	CSourceStream(NAME("Amba RtspStream"), phr, pParent, pPinName)
	,m_video_width(1280)
	,m_video_height(720)
	,m_enc_type(ENC_H264)
	,m_connect_status(false)
	,m_streamId(0)
	,m_cachingTime(0)
	,m_timeup_event(0)
	,m_last_pts(0)
	,m_begin_pts(0)
	,m_expect_time(0)
	,m_first_slice_time(0)
{
	// Begin by setting up our usage environment:
	m_rtsp_recv = new CAmbaRtspRecv;
	m_hostname = new char[64];
	strncpy(m_hostname, DEFAULT_HOST, 64);
	ZeroMemory(&m_mpeg_video_info2, sizeof(m_mpeg_video_info2));
}

CAmbaRtspStream::~CAmbaRtspStream()
{
	delete[] m_hostname;
	//delete[] m_buf;
	delete m_rtsp_recv;
}


HRESULT CAmbaRtspStream::SetStreamId(int streamId)
{
	//sprintf(m_url, "%s%d", RTSP_URL_PREFIX, (streamId+1));
	m_streamId = streamId;
	return NOERROR;
}

HRESULT CAmbaRtspStream::SetHostname(const char* hostname)
{
	strncpy(m_hostname,hostname,strlen(hostname)+1);
	return NOERROR;
}

STDMETHODIMP CAmbaRtspStream::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if( riid == IID_IAmbaPin)
	{
		return GetInterface((IAmbaPin *) this, ppv);
	}
	return inherited::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CAmbaRtspStream::DecideAllocator(IMemInputPin * pin, IMemAllocator **alloc)
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

HRESULT CAmbaRtspStream::FillBuffer(IMediaSample *pms)
{
	CheckPointer(pms,E_POINTER);
	HRESULT hr;
	BYTE *pBuffer;
	hr = pms->GetPointer(&pBuffer);
	if (FAILED(hr))
	{
		return hr;
	}

	SLICE_INFO slice_info;
	int fetch_result = 0;
begin:
	while(1)
	{
		fetch_result = m_rtsp_recv->fetch_data(pBuffer,&slice_info,0);
		DbgLog((LOG_TRACE, 1, TEXT("fetch_result = %d"), fetch_result));
		if (fetch_result < 0)
		{
			DbgLog((LOG_TRACE, 1, TEXT("unexpected error")));
			return S_FALSE;
		}
		if (!m_first_slice_flag)
		{
			if (slice_info.nal_unit_type == 5 || slice_info.nal_unit_type == 0xFF)
			{
					m_first_slice_flag = true;
					DbgLog((LOG_TRACE, 1, TEXT("***** first IDR ****\n")));
			}
			else
			{
				m_begin_pts = slice_info.pts;
				m_last_pts = slice_info.pts;
				m_expect_time = m_cachingTime;
				continue;
			}
		}
		break;
	}

	if (!slice_info.pts_valid || fetch_result == 32)
	{
		DbgLog((LOG_TRACE, 1, TEXT("pts not correct, play immediatly")));
		if (fetch_result == 0 || fetch_result == 32)
		{
			m_expect_time = m_cachingTime;
			m_begin_pts = slice_info.pts;
			m_last_pts = slice_info.pts;
		}
	}
	else
	{
		DWORD delay;
		if (m_expect_time == m_cachingTime)
		{
			m_first_slice_time = timeGetTime();
			delay = m_cachingTime;
			m_timerId = timeSetEvent(delay,0,TimeProc,(DWORD)m_timeup_event,TIME_ONESHOT);
			DbgLog((LOG_TRACE, 1, TEXT("delay = %d"),delay));
		}
		else
		{
			DWORD duration = (DWORD)(((slice_info.pts - m_last_pts)/1000) & 0xFFFFFFFF);
			DWORD real_time = timeGetTime() - m_first_slice_time;
			DWORD delay = m_expect_time - real_time;
			if (delay == 0 || ( (real_time > m_expect_time) && ((0-delay) <= 2*duration)) )
			{
				DbgLog((LOG_TRACE, 1, TEXT("play immediatly")));
				goto end;
			}
			else if (delay <= (m_cachingTime + duration)) //normal
			{
				m_timerId = timeSetEvent(delay,0,TimeProc,(DWORD)m_timeup_event,TIME_ONESHOT);
				DbgLog((LOG_TRACE, 1, TEXT("delay = %d"),delay));
			}
			else if ((real_time > m_expect_time) && ((0-delay) > 2*duration))
			{
				DbgLog((LOG_TRACE, 1, TEXT("skip & wait until next IDR")));
				m_first_slice_flag = false;
				goto begin;
			}
			else
			{
				//DbgLog((LOG_TRACE, 1, TEXT("unexpected")));
				//return E_FAIL;
				DbgLog((LOG_TRACE, 1, TEXT("skip & wait until next IDR")));
				m_first_slice_flag = false;
				goto begin;
			}
			DbgLog((LOG_TRACE, 1, TEXT("expect_time = %d, real_time = %d,delay = %d"),m_expect_time,real_time, real_time,delay));
		}

		DWORD dwWaitResult;
		DbgLog((LOG_TRACE, 1, TEXT("before wait %d"),timeGetTime()));
		dwWaitResult = WaitForSingleObject(m_timeup_event,INFINITE);
		m_timerId = 0;
		ResetEvent(m_timeup_event);
		DbgLog((LOG_TRACE, 1, TEXT("get timeup signal %d"),timeGetTime()));
	}

end:
	m_last_pts = slice_info.pts;
	m_expect_time = (DWORD)((m_cachingTime + (slice_info.pts - m_begin_pts)/1000) & 0xFFFFFFFF);
	pms->SetActualDataLength(slice_info.data_size);
	REFERENCE_TIME begin = 0;
	REFERENCE_TIME end = 333670;
	pms->SetTime(&begin, &end); //these two time make no sense, just to make it compatible with ffdshow
	//pms->SetTime(NULL, NULL);
	DbgLog((LOG_TRACE, 1, TEXT("sample_delta = %I64d,time = %u\n"),slice_info.pts - m_begin_pts, timeGetTime()));
	return NOERROR;
}

DWORD WINAPI CAmbaRtspStream::playingStreams(PVOID arg) {
   //HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, lpszMutex);
   //WaitForSingleObject( hMutex, INFINITE );
	CAmbaRtspRecv* pRtspRecv = (CAmbaRtspRecv *)arg;
	pRtspRecv->play();
  	return 0;
}

DWORD WINAPI CAmbaRtspStream::recordStreams(PVOID arg) {
   //HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, lpszMutex);
   //WaitForSingleObject( hMutex, INFINITE );
	CAmbaRtspStream* pAmbaRtspStream = (CAmbaRtspStream*)arg;

	IAmbaRecord* pIAmbaRecord = NULL;
	HRESULT hr = pAmbaRtspStream->m_pFilter->QueryInterface(IID_IAmbaRecord, (void**)(&pIAmbaRecord));
	if (FAILED(hr))
	{
		return 1;
	}
	CAmbaRtspRecv* pRtspRecv = pAmbaRtspStream->m_rtsp_recv;
	CAmbaRecordStream* pAmbaRecordStream = new CAmbaRecordStream(pIAmbaRecord, pAmbaRtspStream->m_enc_type,pAmbaRtspStream->m_streamId);

	int fetch_result = 0;
	bool bRecordstatus = false;
	unsigned char* pBuffer = new unsigned char[BUF_SIZE];
	SLICE_INFO slice_info;
	int left_cnt = 0;
	while(pAmbaRtspStream->m_enableRecord)
	{
		fetch_result = pRtspRecv->fetch_data(pBuffer,&slice_info, 1);
		if (fetch_result < 0)
		{
			goto end;
		}
		pAmbaRecordStream->record_file(pBuffer,slice_info.data_size,slice_info.pts,slice_info.nal_unit_type);
	}

	end:
	delete[] pBuffer;
	delete pAmbaRecordStream;
	if( pIAmbaRecord != NULL )
	{
		pIAmbaRecord->Release();
		pIAmbaRecord = NULL;
	}


  	return 0;

}
HRESULT CAmbaRtspStream::Active(void)
{
	DbgLog((LOG_TRACE, 1, TEXT("Active")));
	m_timerId = 0;
	return inherited::Active();
}

HRESULT CAmbaRtspStream::Inactive(void)
{
	DbgLog((LOG_TRACE, 1, TEXT("Inactive")));
	if (m_timerId > 0)
	{
		timeKillEvent(m_timerId);
		timeSetEvent(1,0,TimeProc,(DWORD)m_timeup_event,TIME_ONESHOT);
	}
	return inherited::Inactive();
}

HRESULT CAmbaRtspStream::OnThreadCreate()
{
	TRACE(TEXT("OnThreadCreate \n"));

	timeBeginPeriod(1);
	HRESULT hr = ConnectServer();
	if (SUCCEEDED(hr)) {
		//m_begin_flag = true;
		m_video_width= 1280; //default
		m_video_height = 720;
		m_first_slice_flag = false;
		//startPlayingStreams();
		//create a new thread to start playing
		// Create a new thread.
		DWORD a_threadId;
    		m_recv_thread = CreateThread(NULL, 0, playingStreams, (PVOID)m_rtsp_recv, 0,
		&a_threadId);

		DWORD a_threadId2;
		m_enableRecord = true;
    	m_record_thread = CreateThread(NULL, 0, recordStreams, (PVOID)this, 0,
		&a_threadId2);

		if (!m_timeup_event)
		{
			m_timeup_event= CreateEvent(
			NULL,         // no security attributes
			TRUE,         // manual-reset event
			FALSE,         // initial state is signaled
			NULL/*(PCTSTR)"WriteEvent"*/  // object name
			);
		}
		m_last_pts = 0;
		m_begin_pts = 0;
		m_expect_time = m_cachingTime;
		return NOERROR;
	} else {
		return E_FAIL;
	}

}

HRESULT CAmbaRtspStream::OnThreadDestroy(void)
{
	TRACE(TEXT("OnThreadDestroy \n"));
	timeEndPeriod(1);
	//shutdown(NOERROR);
	//exit the thread which 'play' the rtsp stream
	m_rtsp_recv->stop();
	m_enableRecord = false;
	HANDLE end_threads[2]={m_recv_thread, m_record_thread};
	WaitForMultipleObjects(2,end_threads,true,INFINITE);

	if (FAILED(DisconnectServer()))
	{
		return E_FAIL;
	}
	if (m_timeup_event)
	{
		CloseHandle(m_timeup_event);
		m_timeup_event = 0;
	}
	return NOERROR;
}



HRESULT CAmbaRtspStream::CheckMediaType(const CMediaType *pmt)
{
	CheckPointer(pmt, E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	/*int enc_type = 	m_format.encode_type;

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
	}*/
	return NOERROR;
}

HRESULT CAmbaRtspStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	CheckPointer(pmt, E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());
	ConnectServer();

	if (iPosition < 0)
		return E_INVALIDARG;

	//int enc_type = m_format.encode_type;
	//AM_U32 vinFrate = m_format.framerate;
	switch (m_enc_type) {
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
			//pmt->SetFormatType(&FORMAT_VideoInfo);

			BYTE *fmt_buf = pmt->AllocFormatBuffer(sizeof (KS_MPEGVIDEOINFO2));
			if (fmt_buf == NULL)
				return E_OUTOFMEMORY;

			//GetFrameDuration(vinFrate);

			KS_VIDEOINFOHEADER2	*vhdr = &m_mpeg_video_info2.hdr;
			KS_BITMAPINFOHEADER	*bmhdr = &vhdr->bmiHeader;
			bmhdr->biSize = sizeof(KS_BITMAPINFOHEADER);

			bmhdr->biWidth = m_video_width;
			bmhdr->biHeight = m_video_height;

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
			//bmi->biWidth = m_format.encode_width;
			//bmi->biHeight = m_format.encode_height;

			bmi->biPlanes = 1;
			bmi->biBitCount = 24;
			bmi->biCompression = MAKEFOURCC('M','J','P','G');
			bmi->biSizeImage = 0;
		}
		return NOERROR;

	default:
		return E_FAIL;
	}
	return NOERROR;
}

HRESULT CAmbaRtspStream::DecideBufferSize(IMemAllocator *pAlloc,
						 ALLOCATOR_PROPERTIES *pProperties)
{
	CheckPointer(pAlloc,E_POINTER);
	CheckPointer(pProperties,E_POINTER);
	CAutoLock lock(m_pFilter->pStateLock());

	HRESULT hr;

	//int enc_type = m_format.encode_type;
	enum {
		ENC_H264,
		ENC_MJPEG,
	};
	int enc_type = ENC_H264;
	switch (enc_type) {
	case ENC_H264:
		pProperties->cBuffers = 8;
		pProperties->cbBuffer = H264_BUF_SIZE;
		break;

	case ENC_MJPEG:
		pProperties->cBuffers = 8;
		pProperties->cbBuffer = 2*1024*1024;//MJPEG_BUF_SIZE;
		break;

	default:
		return E_FAIL;
	}
	pProperties->cBuffers = 8;
	pProperties->cbBuffer = H264_BUF_SIZE;

	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties, &Actual);
	if (FAILED(hr))
		return hr;

	return NOERROR;
}


HRESULT CAmbaRtspStream::ConnectServer()
{
	if (m_connect_status) {
		return NO_ERROR;
	}
	char url[MAX_URL_LENGTH];
	sprintf_s((char *)url,MAX_URL_LENGTH, "rtsp://%s/stream%d",m_hostname,(m_streamId+1));
	short transferType = TRANS_UDP;
	IAmbaNetwork* pIAmbaNetwork = NULL;
	m_pFilter->QueryInterface(IID_IAmbaNetwork, (void**)(&pIAmbaNetwork));
	if (pIAmbaNetwork != NULL)
	{
		pIAmbaNetwork->GetTransType(&transferType);
		pIAmbaNetwork->Release();
		pIAmbaNetwork = NULL;
	}
	short buffersize = 0;
	IAmbaPlay* pIAmbaPlay = NULL;
	m_pFilter->QueryInterface(IID_IAmbaPlay, (void**)(&pIAmbaPlay));
	if (pIAmbaPlay != NULL)
	{
		pIAmbaPlay->GetBufferSize(&buffersize);
		pIAmbaPlay->GetCachingTime(&m_cachingTime);
		pIAmbaPlay->Release();
		pIAmbaPlay = NULL;
	}
	if (0 == m_rtsp_recv->setup(url,transferType, buffersize)) {
		m_connect_status = true;
		m_enc_type = m_rtsp_recv->m_encType;
		return NO_ERROR;
	} else {
		return E_FAIL;
	}
};

HRESULT CAmbaRtspStream::DisconnectServer(){
	if ( m_connect_status)
	{
		m_rtsp_recv->shutdown();
		m_connect_status =false;
	}
	return NO_ERROR;
};

CUnknown * WINAPI CAmbaRecordParamProp::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	CUnknown *punk = new CAmbaRecordParamProp(lpunk, phr);
	if (punk == NULL && phr)
	{
		*phr = E_OUTOFMEMORY;
	}

	return punk;
}

CAmbaRecordParamProp::CAmbaRecordParamProp(LPUNKNOWN lpunk, HRESULT *phr):
inherited(NAME("RecordControlProp"), lpunk, IDD_DIALOG_RECORD,IDS_PROPPAGE_RECORD_PARAM)
,m_IAmbaRecord(NULL)
{
	ASSERT(phr);
	*phr = S_OK;
}

HRESULT CAmbaRecordParamProp::OnConnect(IUnknown *punk)
{
	HRESULT hr = punk->QueryInterface(IID_IAmbaRecord, (void**)&m_IAmbaRecord);

	if(FAILED(hr))
	{
		return E_NOINTERFACE;
	}
	return NO_ERROR;
}

HRESULT CAmbaRecordParamProp::OnDisconnect()
{
	if (m_IAmbaRecord == NULL)
	{
		return E_NOINTERFACE;
	}
	m_IAmbaRecord->Release();
	m_IAmbaRecord = NULL;
	return NO_ERROR;
}

HRESULT CAmbaRecordParamProp::OnActivate()
{
	TCHAR* filename= new TCHAR[RECORD_FILENAME_LENGTH ];
	m_IAmbaRecord->GetRecordFilename(filename);
	::SetDlgItemText(m_hwnd,IDC_EDIT1,(LPTSTR)filename);
	delete[] filename;
	unsigned int maxfilesize = 2048;
	TCHAR strMaxfilesize[16];
	m_IAmbaRecord->GetRecordFileMaxsize(&maxfilesize);
	sprintf(strMaxfilesize, TEXT("%d"),maxfilesize);
	::SetDlgItemText(m_hwnd,IDC_EDIT2,(LPTSTR)strMaxfilesize);
	short enctype = ENC_NONE;
	m_IAmbaRecord->GetEncodeType(&enctype);
	if (enctype == ENC_MJPEG)
	{
		EnableWindowById(IDC_RADIO2, FALSE);
		::SetDlgItemText(m_hwnd,IDC_RADIO3,(LPTSTR)"mjpeg");
	}
	else if (enctype == ENC_H264)
	{
		::SetDlgItemText(m_hwnd,IDC_RADIO3,(LPTSTR)"h264");
	}
	short rectype = REC_ORG;
	m_IAmbaRecord->GetRecordType(&rectype);

	if (rectype == REC_MP4)
	{
		::SendDlgItemMessage(m_hwnd, IDC_RADIO2, BM_SETCHECK, BST_CHECKED, 0);
	}
	/*else if (rectype== REC_F4V)
	{
		::SendDlgItemMessage(m_hwnd, IDC_RADIO3, BM_SETCHECK, BST_CHECKED, 0);
	} */else if (rectype == REC_ORG)
	{
		::SendDlgItemMessage(m_hwnd, IDC_RADIO3, BM_SETCHECK, BST_CHECKED, 0);
	}

	return S_OK;
}

HRESULT CAmbaRecordParamProp::OnDeactivate()
{
	::KillTimer(m_hwnd, 1);
	return S_OK;
}

HRESULT CAmbaRecordParamProp::OnApplyChanges()
{
	if ( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO2))
	{
		m_IAmbaRecord->SetRecordType(REC_MP4);
	}/*else if ( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO3))
	{
		m_IAmbaRecord->SetRecordType(REC_F4V);
	}*/else if ( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO3))
	{
		m_IAmbaRecord->SetRecordType(REC_ORG);
	}
	char filename[RECORD_FILENAME_LENGTH ];
	HWND hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT1);
	if (::IsWindowEnabled(hwnd))
	{
		::GetDlgItemText(m_hwnd,IDC_EDIT1,(LPTSTR)filename,RECORD_FILENAME_LENGTH);
		m_IAmbaRecord->SetRecordFilename(filename);
		int size;
		char strSize[16];
		::GetDlgItemText(m_hwnd,IDC_EDIT2,(LPTSTR)strSize,16);
		size =  atoi((const char *)strSize);
		if (size <=0 )
		{
			MessageBox(0,(PCTSTR)"The maximum file size should between 1 and 2048 !", (PCTSTR)"Notice", MB_OK);
		}
		else
		{
			m_IAmbaRecord->SetRecordFileMaxsize(size);
		}
	}
	return S_OK;
}

INT_PTR CAmbaRecordParamProp::OnReceiveMessage(
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

void CAmbaRecordParamProp::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
	{
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	}
}

void CAmbaRecordParamProp::OnButtonBrowse()
{
	OPENFILENAME ofn;	   // common dialog box structure
	char szFile[RECORD_FILENAME_LENGTH ];	   // buffer for file name

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hwnd;
	ofn.lpstrFile = (LPTSTR)szFile;
//
// Set lpstrFile[0] to '\0' so that GetOpenFileName does not
// use the contents of szFile to initialize itself.
//
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);

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
		::SetDlgItemText(m_hwnd,IDC_EDIT1,ofn.lpstrFile);
	}

}
int CAmbaRecordParamProp::EnableWindowById(WORD id, BOOL enable)
{
	HWND hwnd = ::GetDlgItem(m_hwnd, id);
	return ::EnableWindow(hwnd, enable);
}

int CAmbaRecordParamProp::EditGetInt(HWND hwnd)
{
	LRESULT size;
	char buffer[RECORD_FILENAME_LENGTH ];

	size = ::SendMessage(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
	if (size > 0)
		return atoi((const char *)buffer);

	return 0;
}


CUnknown * WINAPI CAmbaNetworkProp::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	CUnknown *punk = new CAmbaNetworkProp(lpunk, phr);
	if (punk == NULL && phr)
	{
		*phr = E_OUTOFMEMORY;
	}

	return punk;
}

CAmbaNetworkProp::CAmbaNetworkProp(LPUNKNOWN lpunk, HRESULT *phr):
inherited(NAME("NetworkProp"), lpunk, IDD_DIALOG_NETWORK,IDS_PROPPAGE_NETWORK)
,m_IAmbaNetwork(NULL)
{
	ASSERT(phr);
	*phr = S_OK;
}

HRESULT CAmbaNetworkProp::OnConnect(IUnknown *punk)
{
	HRESULT hr = punk->QueryInterface(IID_IAmbaNetwork, (void**)&m_IAmbaNetwork);

	if(FAILED(hr))
	{
		return E_NOINTERFACE;
	}
	return NO_ERROR;
}

HRESULT CAmbaNetworkProp::OnDisconnect()
{
	if (m_IAmbaNetwork == NULL)
	{
		return E_NOINTERFACE;
	}
	m_IAmbaNetwork->Release();
	m_IAmbaNetwork = NULL;
	return NO_ERROR;
}

HRESULT CAmbaNetworkProp::OnActivate()
{
	short type = TRANS_UDP;
	m_IAmbaNetwork->GetTransType(&type);
	switch (type)
	{
		case TRANS_UDP:
			::SendDlgItemMessage(m_hwnd, IDC_RADIO1, BM_SETCHECK, BST_CHECKED, 0);
			break;
		case TRANS_TCP:
			::SendDlgItemMessage(m_hwnd, IDC_RADIO2, BM_SETCHECK, BST_CHECKED, 0);
			break;
		case TRANS_MULTI:
			::SendDlgItemMessage(m_hwnd, IDC_RADIO3, BM_SETCHECK, BST_CHECKED, 0);
			break;
		default:
			break;
	}
	return S_OK;
}

HRESULT CAmbaNetworkProp::OnDeactivate()
{
	::KillTimer(m_hwnd, 1);
	return S_OK;
}

HRESULT CAmbaNetworkProp::OnApplyChanges()
{
	if( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO1))
	{
		m_IAmbaNetwork->SetTransType(TRANS_UDP);
	}
	else if ( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO2))
	{
		m_IAmbaNetwork->SetTransType(TRANS_TCP);
	}else if ( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO3))
	{
        	m_IAmbaNetwork->SetTransType(TRANS_MULTI);
	}

	return S_OK;
}

INT_PTR CAmbaNetworkProp::OnReceiveMessage(
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
			/*switch (LOWORD(wParam))
			{
			default:
				break;
			}*/
			SetDirty();
			return (LRESULT)1;
		}
		else if (HIWORD(wParam) == EN_CHANGE )
		{
			SetDirty();
			return (LRESULT)1;
		}
		break;
	default:
		break;
	}

	return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

void CAmbaNetworkProp::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
	{
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	}
}

CUnknown * WINAPI CAmbaPlayProp::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	CUnknown *punk = new CAmbaPlayProp(lpunk, phr);
	if (punk == NULL && phr)
	{
		*phr = E_OUTOFMEMORY;
	}

	return punk;
}

CAmbaPlayProp::CAmbaPlayProp(LPUNKNOWN lpunk, HRESULT *phr):
inherited(NAME("PlayProp"), lpunk, IDD_DIALOG_PLAY,IDS_PROPPAGE_PLAY)
,m_IAmbaPlay(NULL)
{
	ASSERT(phr);
	*phr = S_OK;
}

HRESULT CAmbaPlayProp::OnConnect(IUnknown *punk)
{
	HRESULT hr = punk->QueryInterface(IID_IAmbaPlay, (void**)&m_IAmbaPlay);

	if(FAILED(hr))
	{
		return E_NOINTERFACE;
	}
	return NO_ERROR;
}

HRESULT CAmbaPlayProp::OnDisconnect()
{
	if (m_IAmbaPlay == NULL)
	{
		return E_NOINTERFACE;
	}
	m_IAmbaPlay->Release();
	m_IAmbaPlay = NULL;
	return NO_ERROR;
}

HRESULT CAmbaPlayProp::OnActivate()
{
	short buffersize = 4;
	TCHAR strBufferSize[16];
	m_IAmbaPlay->GetBufferSize(&buffersize);
	sprintf(strBufferSize,TEXT("%d"),buffersize);
	::SetDlgItemText(m_hwnd,IDC_EDIT2,(LPTSTR)strBufferSize);

	DWORD cachingtime = 0;
	TCHAR strCachingTime[16];
	m_IAmbaPlay->GetCachingTime(&cachingtime);
	sprintf(strCachingTime, TEXT("%d"),cachingtime);
	::SetDlgItemText(m_hwnd,IDC_EDIT1,(LPTSTR)strCachingTime);
	return S_OK;
}

HRESULT CAmbaPlayProp::OnDeactivate()
{
	::KillTimer(m_hwnd, 1);
	return S_OK;
}

HRESULT CAmbaPlayProp::OnApplyChanges()
{
	char strBufferSize[16];
	::GetDlgItemText(m_hwnd,IDC_EDIT2,(LPTSTR)strBufferSize,16);
	short size =  (short)atoi((const char *)strBufferSize);
	m_IAmbaPlay->SetBufferSize(size);

	char strCachingTime[16];
	::GetDlgItemText(m_hwnd,IDC_EDIT1,(LPTSTR)strCachingTime,16);
	int time = atoi((const char *)strCachingTime);
	m_IAmbaPlay->SetCachingTime(time);
	return S_OK;
}

INT_PTR CAmbaPlayProp::OnReceiveMessage(
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
			/*switch (LOWORD(wParam))
			{
			default:
				break;
			}*/
			SetDirty();
			return (LRESULT)1;
		}
		else if (HIWORD(wParam) == EN_CHANGE )
		{
			SetDirty();
			return (LRESULT)1;
		}
		break;
	default:
		break;
	}

	return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

void CAmbaPlayProp::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
	{
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	}
}


