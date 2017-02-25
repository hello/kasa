#ifndef __RTSPCLIENT_FILTER_H__
#define __RTSPCLIENT_FILTER_H__

#include <Ks.h>
#include <KsMedia.h>
#include "AmbaInterface.h"

class qosMeasurementRecord;
class CAmbaRtspClientFilter;
class CAmbaRtspStream;
class CAmbaRtspRecv;
class CAmbaMp4muxer;

//------------------------------------------------------------------------------
// Class CAmbaRtspClientFilter
//------------------------------------------------------------------------------
class CAmbaRtspClientFilter: public CSource
	,public ISpecifyPropertyPages
	,public IAmbaCom
	,public CAmbaRecord
	,public CAmbaNetwork
	,public CAmbaPlay
{
	typedef CSource inherited;
	friend class CAmbaRtspStream;

public:
	CAmbaRtspClientFilter( LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CAmbaRtspClientFilter();

	DECLARE_IUNKNOWN
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	//ISpecifyPropertyPages
	STDMETHODIMP GetPages (CAUUID *pages);
	//IAmbaCom
	STDMETHODIMP GetVersion (int* version) {
		*version = RTSP_CLIENT_VERSION;
		return NO_ERROR;
	}
	STDMETHODIMP SetConfItems();
	STDMETHODIMP LoadConfig();
	STDMETHODIMP SaveConfig();
private:
	items_s* m_groups[3];
};
//------------------------------------------------------------------------------
// Class CAmbaRecordStream
//------------------------------------------------------------------------------
class CAmbaRecordStream
{
public:
	CAmbaRecordStream(IAmbaRecord* pIAmbaRecord,unsigned short encType, int streamId);
	~CAmbaRecordStream();
private:
	IAmbaRecord* m_IAmbaRecord;
	unsigned int m_totle_size;
	unsigned int m_record_totle_frames;
	
	int m_record_handle;
	unsigned int m_maxfilesize;
	CAmbaMp4muxer *m_mp4muxer;
	short m_rectype;
	unsigned short m_enc_type;
	int m_streamId;
	unsigned short m_video_width;
	unsigned short m_video_height;
public:
	void open_record_file();
	void close_record_file();
	int write_record_file(unsigned char* pBuffer, int nSize);
	int record_file(unsigned char* pBuffer,int nSize, long long sample_delta, short nal_unit_type);

};

//------------------------------------------------------------------------------
// Class CAmbaRtspStream
//------------------------------------------------------------------------------
class CAmbaRtspStream : public CSourceStream
	, IAmbaPin
{
	typedef CSourceStream inherited;

public:
	CAmbaRtspStream(HRESULT *phr,  CAmbaRtspClientFilter* pParent, 
		LPCWSTR pPinName);
	~CAmbaRtspStream();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	STDMETHOD(SetStreamId)(int stream_id); 
	STDMETHOD(SetHostname)(const char* hostname);
	STDMETHODIMP ConnectServer();
	STDMETHODIMP DisconnectServer();
	STDMETHODIMP GetResolution(unsigned short* width, unsigned short* height) { 
		if (m_video_width != 0 && m_video_height !=0 ) {
			*width = m_video_width;
			*height = m_video_height;
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
		EnterCriticalSection(&(m_rtsp_recv->m_stat_cs));
		pStat->avg_fps = m_rtsp_recv->m_avg_fps;
		pStat->avg_bitrate = m_rtsp_recv->m_avg_bitrate;
		pStat->avg_pts = m_rtsp_recv->m_avg_pts;
		LeaveCriticalSection(&(m_rtsp_recv->m_stat_cs));
		return NO_ERROR;
	}

	STDMETHODIMP SetStatWindowSize(unsigned int size) {
		if (size > 0 && size < 1024)
		{
			EnterCriticalSection(&(m_rtsp_recv->m_stat_cs));
			m_rtsp_recv->m_stat_size = size;
			LeaveCriticalSection(&(m_rtsp_recv->m_stat_cs));
		}
		else
		{
			MessageBox(0,(PCTSTR)"The statistics window size should between 1 and 1024!", (PCTSTR)"Notice", MB_OK);
		}
		return NO_ERROR;
	}

	static DWORD WINAPI CAmbaRtspStream::playingStreams(PVOID arg);
	static DWORD WINAPI CAmbaRtspStream::recordStreams(PVOID arg);
protected:
	HRESULT DecideAllocator(IMemInputPin * pin, IMemAllocator **alloc);
	//HRESULT DoBufferProcessingLoop(void); 
	HRESULT FillBuffer(IMediaSample *pms);

	HRESULT Active(void);
	HRESULT Inactive(void);  

	HRESULT OnThreadCreate(void);
	HRESULT OnThreadDestroy(void);

	STDMETHODIMP Notify(IBaseFilter *pSender, Quality q) { return E_FAIL; }

	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc,
							 ALLOCATOR_PROPERTIES *pProperties);

	HRESULT setup_for_rtsp();

private:
	char* m_hostname;
	KS_MPEGVIDEOINFO2 m_mpeg_video_info2;

	DWORD m_cachingTime;
	bool m_first_slice_flag;
	unsigned short m_video_width;
	unsigned short m_video_height;
	unsigned short m_enc_type;
	bool m_connect_status;

	HANDLE m_recv_thread;
	HANDLE m_record_thread;
	HANDLE m_timeup_event;
	UINT m_timerId;

public:
	CAmbaRtspRecv* m_rtsp_recv;
	int m_streamId;
	bool m_enableRecord;

	//static bool m_timeup_flag;
	DWORD m_first_slice_time;
	DWORD m_expect_time;
	long long m_last_pts;
	long long m_begin_pts;
};

//------------------------------------------------------------------------------
// Class CAmbaRecordParamProp
//------------------------------------------------------------------------------
class CAmbaRecordParamProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaRecordParamProp(LPUNKNOWN lpunk, HRESULT *phr);

	virtual HRESULT OnConnect(IUnknown *punk);
	virtual HRESULT OnDisconnect();

	virtual HRESULT OnActivate();
	virtual HRESULT OnDeactivate();
	virtual HRESULT OnApplyChanges();

	virtual INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam); 

private:
	void SetDirty();
	int EditGetInt(HWND hwnd);
	int EnableWindowById(WORD id, BOOL enable);

	HRESULT GetAllInfo();
	HRESULT SendAllInfo();
	void OnButtonBrowse();

	IAmbaRecord* m_IAmbaRecord;
};

//------------------------------------------------------------------------------
// Class CAmbaNetworkProp
//------------------------------------------------------------------------------
class CAmbaNetworkProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaNetworkProp(LPUNKNOWN lpunk, HRESULT *phr);

	virtual HRESULT OnConnect(IUnknown *punk);
	virtual HRESULT OnDisconnect();

	virtual HRESULT OnActivate();
	virtual HRESULT OnDeactivate();
	virtual HRESULT OnApplyChanges();

	virtual INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam); 

private:
	void SetDirty();
	int EditGetInt(HWND hwnd);
	//int EnableWindowById(WORD id, BOOL enable);

	HRESULT GetAllInfo();
	HRESULT SendAllInfo();

	IAmbaNetwork* m_IAmbaNetwork;
};

//------------------------------------------------------------------------------
// Class CAmbaPlayProp
//------------------------------------------------------------------------------
class CAmbaPlayProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaPlayProp(LPUNKNOWN lpunk, HRESULT *phr);

	virtual HRESULT OnConnect(IUnknown *punk);
	virtual HRESULT OnDisconnect();

	virtual HRESULT OnActivate();
	virtual HRESULT OnDeactivate();
	virtual HRESULT OnApplyChanges();

	virtual INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam); 

private:
	void SetDirty();
	int EditGetInt(HWND hwnd);

	HRESULT GetAllInfo();
	HRESULT SendAllInfo();

	IAmbaPlay* m_IAmbaPlay;
};

#endif //__RTSPCLIENT_FILTER_H__
