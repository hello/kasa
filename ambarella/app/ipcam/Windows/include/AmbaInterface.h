#ifndef __AMBA_INTERFACE_H_
#define __AMBA_INTERFACE_H_

#include "AmbaComm.h"
#pragma warning(disable:4995)

#define TCP_CLIENT_VERSION 0x01000010
#define RTSP_CLIENT_VERSION 0x01000018

// {84E7A774-750F-4df1-8E69-6B61C109A07C}
DEFINE_GUID(IID_IAmbaPin, 
0x84e7a774, 0x750f, 0x4df1, 0x8e, 0x69, 0x6b, 0x61, 0xc1, 0x9, 0xa0, 0x7c);

// {C0B016AD-8F5D-41fc-9147-B2A2C3A12788}
DEFINE_GUID(IID_IAmbaRecord, 
0xc0b016ad, 0x8f5d, 0x41fc, 0x91, 0x47, 0xb2, 0xa2, 0xc3, 0xa1, 0x27, 0x88);

// {E3E521AC-015B-4a15-8EAC-CBBD44024A6F}
DEFINE_GUID(IID_IAmbaCom, 
0xe3e521ac, 0x15b, 0x4a15, 0x8e, 0xac, 0xcb, 0xbd, 0x44, 0x2, 0x4a, 0x6f);

// {E69C6A21-4818-42e0-A30C-FDA73862E9FA}
DEFINE_GUID(IID_IAmbaNetwork, 
0xe69c6a21, 0x4818, 0x42e0, 0xa3, 0xc, 0xfd, 0xa7, 0x38, 0x62, 0xe9, 0xfa);

// {81E9E3F6-07AA-4c62-B16E-60849B339F2B}
DEFINE_GUID(IID_IAmbaPlay, 
0x81e9e3f6, 0x7aa, 0x4c62, 0xb1, 0x6e, 0x60, 0x84, 0x9b, 0x33, 0x9f, 0x2b);



//interface IAmbaPinType: public IUnknown
DECLARE_INTERFACE_(IAmbaPin,IUnknown)
{
public:
	enum
	{
		OTHER,
		VIDEO,
		AUDIO,
	};

public:
	//STDMETHOD_(DWORD, GetType)() = 0;
	STDMETHOD(SetStreamId)(int stream_id) = 0;
	STDMETHOD(SetHostname)(const char* hostname) = 0;
	STDMETHOD(SetStatWindowSize)(unsigned int size) = 0;
	STDMETHOD(ConnectServer)() = 0;
	STDMETHOD(DisconnectServer)() = 0;
	STDMETHOD(GetResolution)(unsigned short* width, unsigned short* height) = 0;
	STDMETHOD(GetConnectStatus)(bool* status) = 0;
	STDMETHOD(GetStatistics)(ENC_STAT* stat) = 0;
};

DECLARE_INTERFACE_(IAmbaRecord,IUnknown)
{
public:
	STDMETHOD(SetRecordStatus)(bool bStatus) = 0;
	STDMETHOD(GetRecordStatus)(bool *pStatus) = 0;
	STDMETHOD(SetRecordFilename)(const char* strFilename) = 0;
	STDMETHOD(GetRecordFilename)(char* strFilename) = 0;
	STDMETHOD(SetRecordFileMaxsize)(unsigned int nMaxsize) = 0;
	STDMETHOD(GetRecordFileMaxsize)(unsigned int* pMaxsize) = 0;
	STDMETHOD(SetRecordType)(short nType) = 0;
	STDMETHOD(GetRecordType)(short* pType) = 0;

	STDMETHOD(SetEncodeType)(short nType) = 0;
	STDMETHOD(GetEncodeType)(short* pType) = 0;
};

DECLARE_INTERFACE_(IAmbaCom,IUnknown)
{
public:
	class items_s {
	public:
		items_s(const char* item_name):next_item(NULL){
			strcpy(m_item,item_name);
		};
		~items_s(){
			while(next_item){
				del_item();
			}
		};
		void add_item(const char* item_name) {
			items_s* next_item = this->next_item;
			this->next_item = new items_s(item_name);
			this->next_item->next_item = next_item;
		};
		void del_item(){
			items_s* del_item = this->next_item;
			if (del_item){
				this->next_item = del_item->next_item;
				del_item->next_item = NULL;
				delete del_item;
			}
		};

		char m_item[32];	//the first item is group name
		class items_s* next_item;
	};
	
	STDMETHOD(GetVersion)(int* version) = 0;
	STDMETHOD(SetConfItems)() = 0;
	STDMETHOD(LoadConfig)() = 0;
	STDMETHOD(SaveConfig)() = 0;
};

DECLARE_INTERFACE_(IAmbaNetwork,IUnknown)
{
	STDMETHOD(GetTransType)(short* pType) = 0;
	STDMETHOD(SetTransType)(short nType) = 0;
};

class CAmbaRecord:public IAmbaRecord
{
public:
	CAmbaRecord( ):m_maxfilesize(2048)	
		,m_recordstatus(false)
		,m_recordtype(REC_ORG)
		,m_encodetype(ENC_NONE)
	{
		strncpy(m_filename,DEFAULT_FILE_NAME, strlen(DEFAULT_FILE_NAME)+1);
	};
	virtual ~CAmbaRecord(){};

	//IAmabRecord
	STDMETHODIMP SetRecordStatus(bool bStatus)
	{
		if (m_recordstatus != bStatus)
		{
			m_recordstatus = bStatus;
			if (m_recordstatus)
			{
				MessageBox(0, (LPCTSTR)"Recording started!", (LPCTSTR)"Notice", MB_OK);
			}
			else
			{
				MessageBox(0, (LPCTSTR)"Recording stopped!", (LPCTSTR)"Notice", MB_OK);
			}
		}
		else 
		{
			if (m_recordstatus)
			{
				MessageBox(0, (LPCTSTR)"Already being recording!", (LPCTSTR)"Notice", MB_OK);
			}
			else
			{
				MessageBox(0, (LPCTSTR)"Already stopped recording!", (LPCTSTR)"Notice", MB_OK);
			}
		}
		return NO_ERROR;
	};

	STDMETHODIMP  GetRecordStatus(bool *pStatus)
	{
		*pStatus = m_recordstatus;
		return NO_ERROR;
	};

	STDMETHODIMP  SetRecordFilename(const char* strFilename)
	{
		strncpy(m_filename,strFilename, strlen(strFilename)+1);
		return NO_ERROR;
	};

	STDMETHODIMP  GetRecordFilename(char* strFilename)
	{
		strncpy(strFilename, m_filename, strlen(m_filename)+1);
		return NO_ERROR;
	};

	STDMETHODIMP  SetRecordFileMaxsize(unsigned int nMaxSize)
	{
		m_maxfilesize = nMaxSize;
		return NO_ERROR;
	};

	STDMETHODIMP  GetRecordFileMaxsize(unsigned int* pMaxSize)
	{
		*pMaxSize = m_maxfilesize;
		return NO_ERROR;
	};

	STDMETHODIMP  SetRecordType(short nType)
	{
		m_recordtype = nType;
		return NO_ERROR;
	};

	STDMETHODIMP  GetRecordType(short* pType)
	{
		*pType = m_recordtype;
		return NO_ERROR;
	};

	STDMETHODIMP  SetEncodeType(short nType)
	{
		m_encodetype = nType;
		return NO_ERROR;
	};

	STDMETHODIMP  GetEncodeType(short* pType)
	{
		*pType = m_encodetype;
		return NO_ERROR;
	};
protected:
	char m_filename[RECORD_FILENAME_LENGTH];
	int m_maxfilesize;
	bool m_recordstatus;
	short m_recordtype;
	short m_encodetype;
};

class CAmbaNetwork:public IAmbaNetwork
{	
public:
	CAmbaNetwork( ):m_transtype(TRANS_UDP){};
	virtual ~CAmbaNetwork(){};

	//IAmabNetwork
	STDMETHODIMP GetTransType(short* pType) 
	{
		*pType =  m_transtype;
		return NO_ERROR;
	}

	STDMETHODIMP SetTransType(short nType) 
	{
		m_transtype = nType;
		return NO_ERROR;
	}

protected:
	short m_transtype;
	
};

DECLARE_INTERFACE_(IAmbaPlay,IUnknown)
{
	STDMETHOD(GetCachingTime)(DWORD* pMsec) = 0;
	STDMETHOD(SetCachingTime)(DWORD nMsec) = 0;
	STDMETHOD(GetBufferSize)(short* pSize) = 0;
	STDMETHOD(SetBufferSize)(short nSize) = 0;
};

class CAmbaPlay:public IAmbaPlay
{
public:
	CAmbaPlay( ):m_buffersize(4)	
		,m_cachingtime(50)
	{
	};
	virtual ~CAmbaPlay(){};

	//IAmabPlay
	STDMETHODIMP GetCachingTime(DWORD* pMsec)
	{
		*pMsec = m_cachingtime;
		return NOERROR;
	};

	STDMETHODIMP SetCachingTime(DWORD nMsec)
	{
		if (nMsec < 0 || nMsec > 60000)
		{
			MessageBox(0, (LPCTSTR)"Caching time should between 0 ms and 60000 ms (10 sec)!", (LPCTSTR)"Notice", MB_OK);
			return E_INVALIDARG;
		}
		m_cachingtime = nMsec;
		return NOERROR;
	};

	STDMETHODIMP GetBufferSize(short* pSize)
	{
		*pSize = m_buffersize;
		return NOERROR;
	};

	STDMETHODIMP SetBufferSize(short nSize)
	{
		if (nSize < 0 || nSize > 64)
		{
			MessageBox(0, (LPCTSTR)"Buffer Size should between 0  and 64M byte !", (LPCTSTR)"Notice", MB_OK);
			return E_INVALIDARG;
		}
		m_buffersize = nSize;
		return NOERROR;
	};
protected:
	int m_cachingtime;
	short m_buffersize;
};
#endif //__AMBA_INTERFACE_H_
