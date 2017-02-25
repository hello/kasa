#ifndef __RECEIVEFILTER_H__
#define __RECEIVEFILTER_H__

class CAmbaTcpClientFilter;
class CAmbaTCPStream;
class CamMp4Muxer;

#include "AmbaInterface.h"
//------------------------------------------------------------------------------
// Class CAmbaTcpClientFilter
//------------------------------------------------------------------------------
class CAmbaTcpClientFilter: public CSource
	,public ISpecifyPropertyPages
	,public IAmbaCom
	,public CAmbaRecord
	//,public ISpecifyPropertyPages
{
	typedef CSource inherited;
	friend class CAmbaTCPStream;

public:
	CAmbaTcpClientFilter( LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CAmbaTcpClientFilter();

	DECLARE_IUNKNOWN
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	STDMETHODIMP GetPages (CAUUID *pages);
	STDMETHODIMP GetVersion (int* version) {
		*version = TCP_CLIENT_VERSION;
		return NO_ERROR;
	};
	STDMETHODIMP SetConfItems();
	STDMETHODIMP LoadConfig();
	STDMETHODIMP SaveConfig();
private:
	items_s* m_groups[1];
};

//------------------------------------------------------------------------------
// Class CAmbaTCPStream
//------------------------------------------------------------------------------
class CAmbaTCPStream : public CSourceStream
	,IAmbaPin
{
typedef CSourceStream inherited;
struct stat_data
{
	unsigned int frame_cnt;
	int totle_size;
	int totle_pts;
	int last_pts;
	DWORD begin_time;
	DWORD *old_time;
	int *old_size;
	int *old_pts;
	unsigned int window_size;
};
public:

	CAmbaTCPStream(HRESULT *phr, CAmbaTcpClientFilter *pParent, 
		LPCWSTR pPinName, short tcp_port);
	~CAmbaTCPStream();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	HRESULT DecideAllocator(IMemInputPin * pin, IMemAllocator **alloc);
	HRESULT FillBuffer(IMediaSample *pms);

	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc,
							 ALLOCATOR_PROPERTIES *pProperties);

	HRESULT Active(void);
	HRESULT Inactive(void);  

	HRESULT OnThreadCreate(void);
	HRESULT OnThreadDestroy(void);
	STDMETHODIMP Notify(IBaseFilter *pSender, Quality q) { return E_FAIL; }

	STDMETHOD(SetStreamId)(int stream_id); 
	STDMETHOD(SetHostname)(const char* hostname);
	STDMETHODIMP ConnectServer();
	STDMETHODIMP DisconnectServer();
	STDMETHODIMP GetResolution(unsigned short* width, unsigned short* height) { 
		if (m_format.encode_width != 0 && m_format.encode_height !=0 ) {
			*width = m_format.encode_width;
			*height = m_format.encode_height;
		} else {
			*width = 1280;
			*height = 720;
		}
		return NO_ERROR;
	}
	STDMETHODIMP GetConnectStatus(bool* status) { 
		*status = m_connect_status;
		return NO_ERROR;
	}
	STDMETHODIMP GetStatistics(ENC_STAT* pStat) {
		EnterCriticalSection(&m_stat_cs);
		pStat->avg_fps = m_avg_fps;
		pStat->avg_bitrate = m_avg_bitrate;
		pStat->avg_pts = m_avg_pts;
		LeaveCriticalSection(&m_stat_cs);
		return NO_ERROR;
	}
	STDMETHODIMP SetStatWindowSize(unsigned int size) {
		if (size > 0 && size < 1024)
		{
			EnterCriticalSection(&m_stat_cs);
			m_stat_size = size;
			LeaveCriticalSection(&m_stat_cs);
		}
		else
		{
			MessageBox(0,(PCTSTR)"The statistics window size should between 1 and 1024!", (PCTSTR)"Notice", MB_OK);
		}
		return NO_ERROR;
	}
	//STDMETHODIMP_(DWORD) GetType() { return OTHER; }

private:
	void open_record_file();
	void close_record_file();
	int write_record_file(unsigned char* pBuffer, int nSize);

	bool m_connect_status;
	int	m_stream_id;
	char m_hostname[64];
	short	m_port;
	SOCKET	m_socket;
	sockaddr_in my_addr;
	REFERENCE_TIME	m_AvgTimePerFrame;

	ULONG	m_maxFrameSize;

	CCritSec	m_stat_lock;
	ULONG	m_total_bytes;
	ULONG	m_total_frames;
	//ULONG	m_start_tick;
	//ULONG	m_end_tick;

	//IReferenceClock *m_pClock;
	format_t m_format;
	CamMp4Muxer *m_mp4muxer;

	KS_MPEGVIDEOINFO2 m_mpeg_video_info2;

	IAmbaRecord* m_IAmbaRecord;
	unsigned int m_totle_size;
	short  m_nal_unit_type;
	int m_record_handle;
	int m_record_totle_frames;
	unsigned int m_maxfilesize;
	short m_rectype;
	unsigned int m_last_pts;

	stat_data m_stat;
	float m_avg_fps;
	int m_avg_bitrate;
	int m_avg_pts;
	unsigned int m_stat_size;
	CRITICAL_SECTION m_stat_cs;
};


//------------------------------------------------------------------------------
// Class CAmbaMuxerParamProp
//------------------------------------------------------------------------------

class CAmbaMuxerParamProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaMuxerParamProp(LPUNKNOWN lpunk, HRESULT *phr);

	virtual HRESULT OnConnect(IUnknown *punk);
	virtual HRESULT OnDisconnect();

	virtual HRESULT OnActivate();
	virtual HRESULT OnDeactivate();
	virtual HRESULT OnApplyChanges();

	virtual INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam); 

private:
	void SetDirty();
	int EnableWindowById(WORD id, BOOL enable);

	HRESULT GetAllInfo();
	HRESULT SendAllInfo();
	void OnButtonBrowse();

	IAmbaRecord* m_IAmbaRecord;
};

#endif //__RECEIVEFILTER_H__