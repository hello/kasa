/*
 * iso_muxer.h
 *

 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __ISO_MUXER_H__
#define __ISO_MUXER_H__


#include <fcntl.h>	 //for open O_* flags

#include <stdlib.h>	//for malloc/free
#include <string.h>	//for strlen/memset
#include <stdio.h>	 //for printf
#include <windows.h>
#include <io.h>
#include <assert.h>
#include <time.h>
#include "ambas_common.h"

class CamFileSink;
class CamIsoMuxer;

struct FRAME_INFO
{
	AM_UINT	size;
	AM_UINT	pts;
	AM_UINT	pts_high;
	AM_U16	bSync;
	AM_U16	flags;
	AM_UINT	counter;
	AM_UINT	pic_struct;
};
struct MUXER_PARAM
{
	MUXER_PARAM(){
		MuxerType=0;
		MaxFileSize=2048;
		::memcpy(MuxerFilename,"D:\\test.mp4",256);
	}
	AM_UINT MuxerType;
	char MuxerFilename[256];
	AM_UINT MaxFileSize; 
};


typedef struct
{
	//====video encoder attributes
	//_ar_x:_ar_y is aspect ratio
	AM_U16 _ar_x;				   //16
	AM_U16 _ar_y;				   //9
	AM_U8 _mode;					//1:frame encoding, 2:field encoding
	AM_U8 _M;					   //=4, every M frames has one I frame or P frame
	AM_U8 _N;					   //=32, every N frames has one I frame, 1 GOP is made of N frames
	AM_U8 _advanced;				//=0, xxxyyyyy, xxx:level, yyyyy:GOP level, see DSP pg18
	AM_UINT _idr_interval;		  //=4, one idr frame per idr_interval GOPs
	//here scale and rate meaning is same as DV, just opposite to iav_drv in ipcam
	AM_UINT _scale;				 //fps=rate/scale, fps got from (video_enc_info.vout_frame_rate&0x7F)
	AM_UINT _rate;				  //=180000
	//for Pseudo CBR or VBR
	//brate_min=h264_config_main.average_bitrate*h264_config_main.min_vbr_rate_factor/100=video_enc_info.average_bitrate*((video_enc_info.vbr_cntl>>16)&0x7f)/100;
	//brate=h264_config_main.average_bitrate*h264_config_main.max_vbr_rate_factor/100=(video_enc_info.average_bitrate*((video_enc_info.vbr_cntl>>23)))>>6;
	//for CBR
	//brate=brate_min=h264_config_main.average_bitrate=video_enc_info.average_bitrate
	AM_UINT _brate;
	AM_UINT _brate_min;
	//1=first GOP is a new GOP, 0=first GOP is a normal GOP
	//_IdrPacketCnt==1: _VPacketCnt%_M==0 ==>new_gop=0, else new_gop=1
	//_IdrPacketCnt>1:(2nd idr frame number-1st idr frame number)%_M==0 ==>new_gop=0, else new_gop=1
	AM_UINT _new_gop;			   //=1
	AM_UINT _Width;				 //=1280
	AM_UINT _Height;				//=720
	//====audio encoder attributes
	AM_UINT _SampleFreq;			//=48000, 44.1K 48K
	AM_U16 _Channels;			   //=2
	AM_U16 _SampleSize;			 //=16
	//====general attributes
	AM_UINT _clk;				   //=90000, system clock
	AM_UINT _FileSize;			  //final mp4 file size not exceed this value
	//====also used by Recover()
	AM_UINT _FileType;			  //=1 1:mp4, 2:f4v
	AM_U8 _FileName[256];		   //="/mnt/test.mp4"

}CISOMUXER_CONFIG;



#define AM_TRUE	 1
#define AM_FALSE	0

#define _IDX_SIZE 256 //keep it equal 2^N, 8.5 seconds of video@30fps, or 5.3 seconds of audio@48Khz
//#define _IDX_SIZE 1024 //keep it equal 2^N, 34 seconds of video@30fps, or 21 seconds of audio@48Khz
#define _SPS_SIZE 64
#define _PPS_SIZE 64

#define _SINK_BUF_SIZE	32000 //==0x7d00
#define PTS_UNIT 90000

typedef AM_U64 AM_PTS;


enum AM_IF_ID
{
	IF_MSG_SINK,
	IF_MSG_SYS,
	IF_MEDIA_CONTROLLER,
	IF_ACTIVE_OBJECT,

	IF_FILTER,
	IF_PIN,
	IF_MEDIA_PROCESSOR,
	IF_BUFFER_POOL,
	IF_PIPELINE,

	IF_VIN_CONTROL,
	IF_AIN_CONTROL,
	IF_VOUT_CONTROL,
	IF_AOUT_CONTROL,
	IF_VIDEO_ENCODER,
	IF_AUDIO_ENCODER,

	IF_FILE_READER,
	IF_PB_CONTROL,
	IF_DEMUXER,
	IF_VIDEO_DECODER,
	IF_AAC_DECODER,
	IF_MOTION_DETECTOR,
	IF_AAAFACE_DETECTOR,
	IF_TS_MUXER,
	IF_IPTS_PIPELINE,

	IF_ISO_MUXER,
	IF_TS_DEMUXER,
	IF_TS_PB_PIPELINE,

	IF_ENABLE_NETWORK,
	IF_IMG_CONTROL,
};


class CamIsoMuxer
{
public:
	CamIsoMuxer(AM_UINT vin_cnt, AM_UINT ain_cnt);
	AM_ERR Construct();
	virtual ~CamIsoMuxer();

public:
	// IamInterface
   // virtual void *GetInterface(AM_IF_ID ifID);
   // virtual void Delete();
  // IamFilter
	AM_ERR Init(int videoIndex); 
	AM_ERR Run();
	AM_ERR Stop();
	// CamActiveFilter
protected:
   // virtual AM_ERR MainLoop();

	// IamIsoMuxer
public:
   // virtual AM_ERR GetConfig(CISOMUXER_CONFIG *pConfig);
   // virtual AM_ERR SetConfig(CISOMUXER_CONFIG *pConfig);
   // virtual AM_ERR Recover(CISOMUXER_CONFIG *pConfig);
	AM_INT ProcessVideoData(FRAME_INFO *info, BYTE * pBuffer);
protected:
   // bool WaitInput(CamIsoMuxerInput*& pInput);
	//static AM_ERR ThreadEntry(void *p);

	AM_ERR IdxWriter_2();
	//AM_INT ProcessVideoData(FRAME_INFO *info, BYTE * pBuffer);
	AM_INT ProcessAudioData(FRAME_INFO *info, BYTE * pBuffer);
	AM_ERR MainLoop(FRAME_INFO *info, BYTE * pBuffer);

	void put_byte(AM_UINT data);
	void put_be16(AM_UINT data);
	void put_be24(AM_UINT data);
	void put_be32(AM_UINT data);
	void put_be64(AM_U64 data);
	void put_buffer(AM_U8 *pdata, AM_UINT size);
	AM_UINT get_byte();
	AM_UINT get_be16();
	AM_UINT get_be32();
	void get_buffer(AM_U8 *pdata, AM_UINT size);
	AM_UINT le_to_be32(AM_UINT data);
	AM_U64 le_to_be64(AM_U64 data);
	void put_IsoFile(AM_UINT FreeSpaceSize);
	void put_FileTypeBox();
	void put_MovieBox();
	void put_videoTrackBox(AM_UINT TrackId, AM_UINT Duration);
	void put_VideoMediaBox(AM_UINT Duration);
	void put_VideoMediaInformationBox();
	void put_AudioTrackBox(AM_UINT TrackId, AM_UINT Duration);
	void put_AudioMediaBox();
	void put_AudioMediaInformationBox();
	void put_FreeSpaceBox(AM_UINT FreeSpaceSize);

	AM_UINT GetSpsPps(BYTE *pBuffer, AM_UINT offset=0);
	AM_UINT GetSeiLen(BYTE *pBuffer, AM_UINT offset=0);
	AM_UINT GetAuDelimiter(BYTE *pBuffer, AM_UINT offset=0);
	void InitSdtp();
	AM_UINT GetAacInfo(AM_UINT samplefreq, AM_UINT channel);

protected:
	//filter pins
	AM_UINT _InputVCnt, _InputACnt;
	AM_UINT _OutputCnt;

	//file writer
	CamFileSink *_pSink;		//mp4 file writer
	CamFileSink *_pSinkIdx;	 //(tmp) index data writer

	LONG dwNotifyCount;
	AM_U64 _curPos;				 //next file written position for mp4 file writer

	//mp4 file information
	CISOMUXER_CONFIG _Config;

	AM_UINT _CreationTime;		  //seconds from 1904-01-01 00:00:00 UTC
	AM_UINT _ModificationTime;	  //seconds from 1904-01-01 00:00:00 UTC

	AM_U16 _PixelAspectRatioWidth;  //0
	AM_U16 _PixelAspectRatioHeight; //0
	AM_U8 _sps[_SPS_SIZE];
	AM_UINT _spsSize, _spsPad;	  //_spsSize include _spsPad padding bytes to align to 4 bytes, but exclude the 0x00000001 start code
	AM_U8 _pps[_PPS_SIZE];
	AM_UINT _ppsSize, _ppsPad;	  //_ppsSize include _ppsPad padding bytes to align to 4 bytes, but exclude the 0x00000001 start code
	AM_UINT _FreeSpaceSize;		 //free space size between ftyp atom and mdat atom (include free atom header 8 bytes)
	AM_UINT _MdatFreeSpaceSize;	 //free space size in mdat atom (excluding the mdat header itself)
	//xxx_SIZE macros need these 3 member
	AM_UINT _VPacketCnt;			//current video frame count
	AM_UINT _APacketCnt;			//current audio frame count
	AM_UINT _IdrPacketCnt;		  //current IDR frame count

	AM_UINT _SpsPps_pos;			//file position to store sps and pps (for recovery)
	//for sdtp atom
	AM_U8 _lv_idc[32];			  //[0]=last level, [1.._M+1], map frame to level idc

	//runtime information
	AM_UINT _HaveSpace;
	//xxx_MAXSIZE macros need these 3 member
	AM_UINT _MaxVPacketCnt;		 //max video frames due to index space limit
	AM_UINT _MaxAPacketCnt;		 //max audio frames due to index space limit
	AM_UINT _MaxIdrPacketCnt;	   //max idr video frames
	AM_UINT _v_delta;			   //time count of one video frame
	AM_UINT _a_delta;			   //time count fo one audio frame
	AM_U64 _vdts;				   //video decoding time stamp, ==_v_delta*_VPacketCnt
	AM_U64 _adts;				   //audio decoding time stamp, ==_a_delta*_APacketCnt
	AM_U64 _pts_off;				//pts gap due to dropped frames
	AM_INT _FrameDropped;
	//CamBuffer _prevBuffer;		  //used to check whether video encoder ring buffer wrapped or not
	AM_UINT _v_dropped;			 //dropped video packets
	AM_UINT _a_dropped;			 //dropped audio packets
	//CamH264BitstreamBuffer *_pH264Buffer0;//for field0 data when field mode encoding

	AM_UINT _frame_field_set;	   //0:init state, 1:frame mode, 2:field mode

	bool _largeFile;				//true:write co64 atom and 64-bit mdat atom
	//index management
	AM_UINT _v_idx, _a_idx;
	//the following tmp buffers are located just before the mdat atom in the final mp4 file
	AM_UINT _vctts[_IDX_SIZE*2+128*2];	 //ctts.sample_count+sample_delta for video
	AM_UINT _vctts_fpos, _vctts_cur_fpos;			//file pos of the tmp file buffer for vctts
	AM_UINT _vstsz[_IDX_SIZE*1+128*1];	 //stsz.entry_size for video
	AM_UINT _vstsz_fpos, _vstsz_cur_fpos;			//file pos of the tmp file buffer for vctts
	AM_U64  _vstco64[_IDX_SIZE*1+128*1];   //stco.chunk_offset for video
	AM_UINT _vstco[_IDX_SIZE*1+128*1];	 //stco.chunk_offset for video
	AM_UINT _vstco_fpos, _vstco_cur_fpos;			//file pos of the tmp file buffer for vctts
	AM_UINT _astsz[_IDX_SIZE*1+256*1];	 //stsz.entry_size for audio
	AM_UINT _astsz_fpos, _astsz_cur_fpos;			//file pos of the tmp file buffer for vctts
	AM_U64  _astco64[_IDX_SIZE*1+256*1];   //stco.chunk_offset for audio
	AM_UINT _astco[_IDX_SIZE*1+256*1];	 //stco.chunk_offset for audio
	AM_UINT _astco_fpos, _astco_cur_fpos;			//file pos of the tmp file buffer for vctts

	bool _isStopped;
	int _fileCount;
	bool m_sp264;
};

class CamFileSink
{
public:
	CamFileSink():_cnt(0),_handle(-1){}
	AM_INT Open(AM_U8 *name, AM_UINT flags, AM_UINT mode)
	{
		if (_handle == -1)
		{
			_handle = ::_open((char *)name, flags, mode);
			_cnt = 0;
			return _handle;
		}
		else //can not open again before close it
		{
			return -1;
		}
	}

	AM_INT Read(AM_U8 *data, AM_INT len)
	{
		assert(_handle != -1);
		//before read, we must sync the content we have buffered (maybe read will want this part of data)
		if (len >= 0)
		{
			_FlushBuffer();
			return ::_read(_handle, data, len);
		}
		else
		{
			return -1;
		}
	}

	AM_U64 Seek(AM_U64 offset, AM_INT whence)
	{
		assert(_handle != -1);
		_FlushBuffer();
		return ::_lseeki64(_handle, offset, whence);
	}

	AM_INT Write(AM_U8 *data, AM_INT len)
	{
		assert(_handle != -1);
		//we buffer some content to minimize the transfer on the network
		AM_INT ret_len = 0;
		int size = _SINK_BUF_SIZE-_cnt;
		int tmp, tmp2, wrt_len;
		if (len >= size)
		{
			//out of buffer, we have to actually write
			::memcpy(_buf+_cnt, data, size);
			tmp = ::_tell(_handle);
			wrt_len = ::_write(_handle, _buf, _SINK_BUF_SIZE);
			tmp2 = ::_tell(_handle);
			assert((tmp2 - tmp) == wrt_len);
			_cnt = 0;
			//_Sync();
			data += size;
			len -= size;
			ret_len += size;
			size = _SINK_BUF_SIZE;
			while (len >= size)
			{
				tmp = ::_tell(_handle);
				wrt_len = ::_write(_handle, data, _SINK_BUF_SIZE);
				tmp2 = ::_tell(_handle);
				assert((tmp2 - tmp) == wrt_len);
				//_Sync();
				data += size;
				len -= size;
				ret_len += size;
			}
			if (len > 0) //len<size is sure
			{
				::memcpy(_buf+_cnt, data, len);
				_cnt += len;
				ret_len += len;
			}
		}
		else if (len > 0)
		{
			//buffer it and not actually write
			::memcpy(_buf+_cnt, data, len);
			_cnt += len;
			ret_len += len;
		}

		return ret_len;
	}	  
	AM_ERR Close()
	{
		if (_handle != -1)
		{
			_FlushBuffer();
			_handle = ::_close(_handle);
			AM_ERR err = ME_OK;
			_handle = -1;
			return err;
		}
		else
		{
			return ME_ERROR;
		}
	
	}

	//actually write the buffer content to file
	void _FlushBuffer()
	{
		int tmp, tmp2, wrt_len;
		tmp = tmp2 = wrt_len = 0;
		if (_cnt != 0)
		{
			tmp = ::_tell(_handle);
			wrt_len = ::_write(_handle,_buf, _cnt);
			tmp2 = ::_tell(_handle);
			assert((tmp2 - tmp) == wrt_len);
			_cnt = 0;
		}
	}

protected:
	AM_U8 _buf[_SINK_BUF_SIZE];
	AM_INT _cnt;
	AM_INT _handle;
};

#endif //__ISO_MUXER_H__

