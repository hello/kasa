
#ifndef __AMBA_PAGE_H__
#define __AMBA_PAGE_H__

class CAmbaEncodeFormatProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaEncodeFormatProp(LPUNKNOWN lpunk, HRESULT *phr);

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

private:
	IAmbaRecordControl *m_pRecordControl;
	IAmbaRecordControl::FORMAT *m_pFormat;
};

class CAmbaEncodeParamProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaEncodeParamProp(LPUNKNOWN lpunk, HRESULT *phr);

	virtual HRESULT OnConnect(IUnknown *punk);
	virtual HRESULT OnDisconnect();

	virtual HRESULT OnActivate();
	virtual HRESULT OnDeactivate();
	virtual HRESULT OnApplyChanges();

	virtual INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg,
		WPARAM wParam, LPARAM lParam); 

private:
	void SetConfig(IAmbaRecordControl::H264_PARAM *pParam,
		WORD *pControls, DWORD numControls, BOOL enable);
	void SetLowDelay(IAmbaRecordControl::H264_PARAM *pParam, WORD *pControls);

private:
	void SetDirty();
	int EditGetInt(HWND hwnd);

private:
	IAmbaRecordControl *m_pRecordControl;
	IAmbaRecordControl::FORMAT *m_pFormat;
	IAmbaRecordControl::PARAM *m_pParam;
};

class CAmbaImgParamProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaImgParamProp(LPUNKNOWN lpunk, HRESULT *phr);

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

private:
	IAmbaRecordControl *m_pRecordControl;
	IAmbaRecordControl::IMG_PARAM *m_pImgParam;
};

class CAmbaMdParamProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaMdParamProp(LPUNKNOWN lpunk, HRESULT *phr);

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

private:
	IAmbaRecordControl *m_pRecordControl;
	IAmbaRecordControl::MD_PARAM *m_pMdParam;
};

class CAmbaAboutProp: public CBasePropertyPage
{
	typedef CBasePropertyPage inherited;

public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CAmbaAboutProp(LPUNKNOWN lpunk, HRESULT *phr);
};

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
	int EditGetInt(HWND hwnd);
	int EnableWindowById(WORD id, BOOL enable);

	HRESULT GetAllInfo();
	HRESULT SendAllInfo();
	void OnButtonBrowse();

private:
	IAmbaRecordControl *m_pRecordControl;
	IAmbaRecordControl::FORMAT *m_pFormat;
};


#endif // __AMBA_PAGE_H__

