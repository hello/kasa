#pragma once
#include "AmbaInterface.h"
#include "AmbaComm.h"

#define WM_GRAPHNOTIFY  (WM_USER+20)

enum CAMERA_MNGR_STATE
{
	CMNGR_IDLE = 0,
	CMNGR_RUNING,
};

enum CAMERA_SOURCE_FILTER_TYPE
{
	CSFT_TCP = 0,
	CSFT_RTP,
};

enum CAMERA_DECODER_TYPE
{
	CDT_AVC = 0,
	CDT_FFDSHOW,
};

enum CAMERA_FILTER_TYPE
{
	CFT_SOURCE = 0,
	CFT_DECODER,
	CFT_RENDER,
};

class CAmbaCamGraphMngr
{
public:
	CAmbaCamGraphMngr(void);
	~CAmbaCamGraphMngr(void);

	void SetCameraSourceType(CAMERA_SOURCE_FILTER_TYPE csft){ if (_csft != csft){_bSourceChanged = true;}_csft = csft;};
	CAMERA_SOURCE_FILTER_TYPE GetCameraSourceType(){return _csft;};
	void SetCameraDecorderType(CAMERA_DECODER_TYPE cdt){ if (_cdt != cdt ){_bFilterChanged = true;} _cdt = cdt;};
	CAMERA_DECODER_TYPE GetCameraDecorderType(){return _cdt;};		

	void SetWnd(OAHWND hwnd, RECT rc);
	void Resize(RECT rc);
	void GetSize(RECT & rc);

	bool getstate(OAFilterState* pState);
	//bool getresolution(unsigned short* width, unsigned short * height);
	bool getvideoPos(long *pLeft, long *pTop,long *pRight, long *pBottom);
	bool getvideoSize(long *pWidth, long *pHeight);
	bool prepare();
	bool connect();
	bool disconnect();
	bool run();
	bool stop();
	void record(bool bOp);
	bool getrecord();
	void setstreamId();
	void sethostname();
	bool setrecvType(short recv_type);
	bool getstat(ENC_STAT* stat);
	void setstatwindowSize();
	bool SetVideoWnd(HWND hwnd, bool showDPTZ = false);

	//int StartRec(int pin, char* filepath);
	//int StopRec(int pin);

	void PopSourceFilterPropPage(HWND hwnd);
	void PopDecorderFilterPropPage(HWND hwnd);
	void PopRenderFilterPropPage(HWND hwnd);
	
	IMediaEventEx * GetEventHandle(void);
	bool SetNotifyWindow(HWND inWindow);
//	void GetStreamInfo(SourceStreamInfo *p);

	bool CheckSourceVersion();


private:
	HRESULT AddFilter(CAMERA_FILTER_TYPE _cft);
	HRESULT RmvFilter(CAMERA_FILTER_TYPE _cft);
	
	HRESULT AddToRot(IUnknown* pUnkGraph, DWORD* pdwRegister);
	void RemoveFromRot(DWORD id);
	HRESULT AddFilterByCLSID(IGraphBuilder *pGraph,const GUID& clsid,LPCWSTR wszName,IBaseFilter **ppF);
	HRESULT GetUnconnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir,IPin **ppPin);

	HRESULT ConnectFilters(IGraphBuilder *pGraph,IPin *pOut,IBaseFilter *pDest);
	HRESULT ConnectFilters(IGraphBuilder *pGraph,IBaseFilter *pSrc,IBaseFilter *pDest);
	HRESULT DisconnectFilter(IBaseFilter *pFilter,PIN_DIRECTION PinDir);
	HRESULT RmvlFilter(IGraphBuilder *pGraph, IBaseFilter* pF);
	HRESULT PopPropertyPage(IBaseFilter *pFilter , HWND hWnd);

	HRESULT ConnectServer();

private:
	IMediaEventEx *		_pEvent;
	IBaseFilter *		_pCamera;
	IBaseFilter *		_pDecoder;
	IBaseFilter *		_pRender;
	DWORD			_GraphRegisterID;

	OAHWND 		_hH264wnd;
	RECT 			_H264rc;

	bool								_bAudioOn;
	bool								_bPrepared;
	bool								_bConnected;
	bool								_bFilterChanged;
	bool								_bSourceChanged;

	long	_videoTop;
	long	_videoBottom;
	long	_videoLeft;
	long	_videoRight;
	long	_videoWidth;
	long	_videoHeight;

public:
	CAMERA_SOURCE_FILTER_TYPE	_csft;
	CAMERA_DECODER_TYPE			_cdt;
	IGraphBuilder*	_pGraph;
	int				_streamId;
	char	_hostname[64];
	unsigned int	_statWindowSize;
	bool	_defconf;
	HANDLE _wEvent;
};
