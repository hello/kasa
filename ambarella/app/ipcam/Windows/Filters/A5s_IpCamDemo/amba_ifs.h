
#ifndef __AMBA_IFS_H__
#define __AMBA_IFS_H__

interface IAmbaRecordControl: public IUnknown
{
public:
	typedef unsigned char AM_U8;
	typedef unsigned short AM_U16;
	typedef unsigned long AM_U32;

	struct REQUEST
	{
		AM_U32	  code;
		AM_U32	  dataSize;
	};

	enum
	{
		REQ_GET_VERSION = 3,

		REQ_GET_INFO,

		REQ_GET_FORMAT,
		REQ_SET_FORMAT,

		REQ_GET_PARAM,
		REQ_SET_PARAM,

		REQ_GET_IMG_PARAM,
		REQ_SET_IMG_PARAM,	 

		REQ_GET_MD_PARAM,
		REQ_SET_MD_PARAM,  

		REQ_SET_FORCEIDR,
	};

	enum
	{
		DATA_MAGIC = 0x20091111,
		DATA_VERSION = 0x00000001,
	};

	struct ACK
	{
		AM_U32 result;
		AM_U32 code;
	};

	// REQ_GET_VERSION
	struct VERSION
	{
		AM_U32	  magic;
		AM_U32	  version;
		VERSION()
		{
			magic = DATA_MAGIC;
			version = DATA_VERSION;
		}
	};

	// REQ_GET_INFO
	struct INFO
	{
		AM_U32	  arch;
		AM_U32	  video_channels;
	};

	// must keep same with driver
	enum
	{
		ENC_NONE,
		ENC_H264,
		ENC_MJPEG,
	};

	// must keep same with driver
	enum
	{
		CBR = 1,
		VBR = 3,
	};

	// must keep same with driver
	enum
	{
		GOP_SIMPLE = 0,
		GOP_ADVANCED = 1,
	};

	// must keep same with driver
	enum
	{
		MAIN = 0,
		BASELINE = 1,
	};

	// must keep same with imgproc lib
	enum
	{
		ANTI_FLICKER_60HZ = 0,
		ANTI_FLICKER_50HZ = 1,
	};

	enum
	{
		MODE_OFF = 0,
		MODE_ON = 1,
		MODE_AUTO = 2,
	};

	enum
	{
		MD_SRC_AAA = 0,
		MD_SRC_PREV = 1,
	};
			
	struct VIDEO_FORMAT
	{
		AM_U32	enc_type;
		AM_U32	width;
		AM_U32	height;
		AM_U32	frame_interval;
	};

	struct MIRROR_MODE
	{
		AM_U32	pattern;
		AM_U32	bayer_pattern;
	};

	// REQ_GET_FORMAT,
	// REQ_SET_FORMAT,
	struct FORMAT
	{
		AM_U32	vinMode;
		AM_U32	vinFrate;
		AM_U32	voutMode;

		MIRROR_MODE	mirrorMode;
		VIDEO_FORMAT	main;
		VIDEO_FORMAT	secondary;
	};

	struct H264_PARAM
	{
		AM_U8	M;
		AM_U8	N;
		AM_U8	idr_interval;
		AM_U8	gop_model;			  // 0 - simple; 1 - advanced
		AM_U8	bitrate_control;		// 1 - CBR; 3 - VBR
		AM_U8	profile;				// 0 - main; 1 - baseline
		AM_U8	vbr_ness;			   // 0 - 100
		AM_U8	min_vbr_rate_factor;	// 0 - 100
		AM_U16	max_vbr_rate_factor;	// 0 - 400
		AM_U32	average_bitrate;   

		AM_U16	ar_x;
		AM_U16	ar_y;
		AM_U8	frame_mode;
		AM_U32	rate;
		AM_U32	scale;
	};

	struct AUDIO_PARAM
	{
		AM_U32	bitsPerSample;
		AM_U32	nchannels;
		AM_U32	samplesPerSec;
		AM_U32	pcmFormat;
	};


	// REQ_GET_PARAM,
	// REQ_SET_PARAM,
	struct PARAM
	{
		H264_PARAM	main;
		H264_PARAM	secondary;
		AM_U32		mjpeg_quality;
		AUDIO_PARAM	audio;
	};
	
	struct IMG_PARAM
	{
		AM_U32	lens_type;
		AM_U32	anti_flicker_mode;
		AM_U32	black_white_mode;
		AM_U32	slow_shutter_mode;   
		AM_U32	aaa_enable_mode;
	};

	struct MD_PARAM
	{
		AM_U16	x_lu[4];		/* left up */
		AM_U16	y_lu[4];
		AM_U16	x_rd[4];		/* right down */
		AM_U16	y_rd[4];
		AM_U16	threshold[4];
		AM_U16	sensitivity[4];
		AM_U8	valid[4];
		AM_U8	data_src;
	};
	
	STDMETHOD(VerifyVersion)() = 0;

	STDMETHOD(GetInfo)(INFO **ppInfo) = 0;

	STDMETHOD(GetFormat)(FORMAT **ppFormat) = 0;
	STDMETHOD(SetFormat)(int *pResult) = 0;

	STDMETHOD(GetParam)(PARAM **ppParam) = 0;
	STDMETHOD(SetParam)(int *pResult) = 0;

	STDMETHOD(GetImgParam)(IMG_PARAM **ppImgParam) = 0;
	STDMETHOD(SetImgParam)(int *pResult) = 0;

	STDMETHOD(GetMdParam)(MD_PARAM **ppMdParam) = 0;
	STDMETHOD(SetMdParam)(int *pResult) = 0;
};

interface IAmbaPlatformInfo: public IUnknown
{
public:
	STDMETHOD_(DWORD, GetChannelCount)() = 0;
};

interface IAmbaPinType: public IUnknown
{
public:
	enum
	{
		OTHER,
		VIDEO_MAIN,
		VIDEO_SECOND,
		AUDIO,
		DATA,
	};

public:
	STDMETHOD_(DWORD, GetType)() = 0;
};

#endif
