//------------------------------------------------------------------------------
// File: fambademo.h
//
// Desc: DirectShow sample code - main header file for the amba-demo
//	   source filter.  For more information refer to ambademo.cpp
//
// Copyright (c) Ambarella Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include "amba_ifs.h"

//------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------
class CAmbaDemo;
class CAmbaStream;

// {A70EB973-8A34-4b9c-9BB9-D09EECF39C1F}
DEFINE_GUID(CLSID_A5S_AMBADEMO,
0xa70eb973, 0x8a34, 0x4b9c, 0x9b, 0xb9, 0xd0, 0x9e, 0xec, 0xf3, 0x9c, 0x1f);

class CAmbaRecordControl: public CUnknown, public IAmbaRecordControl
{
	typedef CUnknown inherited;

public:
	CAmbaRecordControl(LPUNKNOWN lpunk, HRESULT *phr);
	virtual ~CAmbaRecordControl();
	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	// IAmbaRecordControl
	STDMETHODIMP VerifyVersion();

	STDMETHODIMP GetInfo(INFO **ppInfo);

	STDMETHODIMP GetFormat(FORMAT **ppFormat);
	STDMETHODIMP SetFormat(int *pResult);

	STDMETHODIMP GetParam(PARAM **ppParam);
	STDMETHODIMP SetParam(int *pResult);

	STDMETHODIMP GetImgParam(IMG_PARAM **ppImgParam);
	STDMETHODIMP SetImgParam(int *pResult);

	STDMETHODIMP GetMdParam(MD_PARAM **ppMdParam);
	STDMETHODIMP SetMdParam(int *pResult);

	STDMETHODIMP SetForceIDR(int *pResult);

private:
	HRESULT Send(char *buffer, ULONG size);
	HRESULT Receive(char *buffer, ULONG size);

	HRESULT SendRequest(int code);

private:
	CAmbaDemo *m_pFilter;
	SOCKET m_socket;

	BOOL m_bInfoValid;
	BOOL m_bFormatValid;
	BOOL m_bParamValid;
	BOOL m_bImgParamValid;
	BOOL m_bMdParamValid;

	INFO	m_info;
	FORMAT	m_format;
	PARAM	m_param;
	IMG_PARAM	m_imgParam;
	MD_PARAM	m_mdParam;
};


typedef unsigned int AM_UINT;
typedef unsigned short AM_U16;

//------------------------------------------------------------------------------
// Class CAmbaDemo
//
// This is the main class for the ambademo filter. It inherits from
// CSource, the DirectShow base class for source filters.
//------------------------------------------------------------------------------
class CAmbaDemo : public CSource
	,public ISpecifyPropertyPages
	,public IAmbaPlatformInfo
{
	typedef CSource inherited;
	friend class CAmbaStream;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	virtual ~CAmbaDemo();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

	// ISpecifyPropertyPages
	STDMETHODIMP GetPages (CAUUID *pages);

	// IAmbaPlatformInfo
	STDMETHODIMP_(DWORD) GetChannelCount();

private:
	CAmbaDemo(LPUNKNOWN lpunk, HRESULT *phr);
};

//------------------------------------------------------------------------------
// Class CAmbaStream
//------------------------------------------------------------------------------
class CAmbaStream : public CSourceStream, public IAmbaPinType
{
	typedef CSourceStream inherited;

public:

	CAmbaStream(HRESULT *phr, CAmbaDemo *pParent,
		LPCWSTR pPinName, short tcp_port);
	virtual ~CAmbaStream();

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

	HRESULT DecideAllocator(IMemInputPin * pin, IMemAllocator **alloc);
	HRESULT FillBuffer(IMediaSample *pms);

	HRESULT Active(void);
	HRESULT Inactive(void);

	HRESULT OnThreadCreate(void);
	HRESULT OnThreadDestroy(void);

	STDMETHODIMP Notify(IBaseFilter *pSender, Quality q) { return E_FAIL; }

	STDMETHODIMP_(DWORD) GetType() { return OTHER; }

protected:
	virtual HRESULT CalcSampleTime(FRAME_INFO& info, REFERENCE_TIME& startTime, REFERENCE_TIME& endTime);
	virtual void InitTdiffs() {}
	virtual void GetTdiff(FRAME_INFO& info, REFERENCE_TIME& time) {}
	void GetClockTime(REFERENCE_TIME& time);

protected:

	short	m_port;
	SOCKET	m_socket;
	int		m_shutdown;
	REFERENCE_TIME	m_AvgTimePerFrame;

	ULONG	m_maxFrameSize;

	CCritSec	m_stat_lock;
	ULONG	m_total_bytes;
	ULONG	m_total_frames;
	ULONG	m_start_tick;
	ULONG	m_end_tick;

	IReferenceClock *m_pClock;
	CamIsoMuxer *m_isomuxer;
};

//------------------------------------------------------------------------------
// Class CAmbaVideoStream
//------------------------------------------------------------------------------
class CAmbaVideoStream: public CAmbaStream
{
	typedef CAmbaStream inherited;

public:
	CAmbaVideoStream(HRESULT *phr, CAmbaDemo *pParent,
		LPCWSTR pPinName, short tcp_port, int index, BOOL bMain);

public:
	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc,
							 ALLOCATOR_PROPERTIES *pProperties);

	STDMETHODIMP_(DWORD) GetType() { return m_bMain ? VIDEO_MAIN : VIDEO_SECOND; }

	HRESULT OnThreadCreate(void);

private:
	void GetFrameDuration(int framerate);

protected:
	BOOL m_bMain;
	int m_videoIndex;
	KS_MPEGVIDEOINFO2 m_mpeg_video_info2;

protected:
	enum { NUM_TDIFF = 3 };

	REFERENCE_TIME	m_tdiffs[NUM_TDIFF];
	REFERENCE_TIME	m_prevTime;
	REFERENCE_TIME	m_prevSysTime;
	REFERENCE_TIME	m_tdiff;
	int	m_ntdiff;
	int	m_tdiff_index;
	BOOL	m_bPrev;

	virtual void InitTdiffs();
	virtual void GetTdiff(FRAME_INFO& info, REFERENCE_TIME& time);
};

//------------------------------------------------------------------------------
// Class CAmbaAudioStream
//------------------------------------------------------------------------------
class CAmbaAudioStream: public CAmbaStream
{
	typedef CAmbaStream inherited;

public:
	CAmbaAudioStream(HRESULT *phr, CAmbaDemo *pParent,
		LPCWSTR pPinName, short tcp_port);

public:
	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc,
							 ALLOCATOR_PROPERTIES *pProperties);
};

//------------------------------------------------------------------------------
// Class CAmbaMotionDataStream
//------------------------------------------------------------------------------
class CAmbaMotionDataStream: public CAmbaStream
{
	typedef CAmbaStream inherited;

public:
	CAmbaMotionDataStream(HRESULT *phr, CAmbaDemo *pParent,
		LPCWSTR pPinName, short tcp_port);

public:
	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc,
							 ALLOCATOR_PROPERTIES *pProperties);
};

//------------------------------------------------------------------------------
// Class CAmbaFaceDataStream
//------------------------------------------------------------------------------
class CAmbaFaceDataStream: public CAmbaStream
{
	typedef CAmbaStream inherited;

public:
	CAmbaFaceDataStream(HRESULT *phr, CAmbaDemo *pParent,
		LPCWSTR pPinName, short tcp_port);

public:
	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc,
							 ALLOCATOR_PROPERTIES *pProperties);
};

