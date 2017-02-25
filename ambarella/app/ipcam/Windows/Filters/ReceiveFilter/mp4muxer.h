/*
 * Copyright (C) 2007-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __MP4MUXER_H__
#define __MP4MUXER_H__

class CamFileSink;
class CamMp4Muxer;

typedef struct
{
	//====video encoder attributes
	//_ar_x:_ar_y is aspect ratio
	AM_U16 _ar_x;				   //16
	AM_U16 _ar_y;				   //9
	AM_U8 _mode;					// 1:frame encoding, 2:field encoding
	AM_U8 _M;					   //=4, every M frames has one I frame or P frame
	AM_U8 _N;					   //=32, every N frames has one I frame, 1 GOP is made of N frames
	AM_U8 _advanced;				//=0, xxxyyyyy, xxx:level, yyyyy:GOP level, see DSP pg18
	AM_UINT _idr_interval;		  //=4, one idr frame per idr_interval GOPs
	//here scale and rate meaning is same as DV, just opposite to iav_drv in ipcam
	AM_UINT _scale;				 //fps=rate/scale, fps got from (video_enc_info.vout_frame_rate&0x7F)
	AM_UINT _rate;				  //=180000
	AM_UINT _brate;
	AM_UINT _brate_min;
	//1=first GOP is a new GOP, 0=first GOP is a normal GOP
	//_IdrPacketCnt==1: _VPacketCnt%_M==0 ==>new_gop=0, else new_gop=1
	//_IdrPacketCnt>1:(2nd idr frame number-1st idr frame number)%_M==0 ==>new_gop=0, else new_gop=1
	AM_UINT _new_gop;			   //=1
	AM_UINT _Width;				 //=1280
	AM_UINT _Height;				//=720
	//====general attributes
	AM_UINT _clk;				   //=90000, system clock
	AM_U8 _FileType;
}CMP4MUXER_CONFIG;

#define AM_TRUE	 1
#define AM_FALSE	0

#define _IDX_SIZE 256 //keep it equal 2^N, 8.5 seconds of video@30fps, or 5.3 seconds of audio@48Khz
#define _SPS_SIZE 64
#define _PPS_SIZE 64

#define _SINK_BUF_SIZE	32000 //==0x7d00
#define PTS_UNIT 90000

typedef AM_U64 AM_PTS;

class CamMp4Muxer
{
public:
	CamMp4Muxer(char* filename);
	virtual ~CamMp4Muxer();

public:
	AM_ERR Init(format_t* p_format,AM_U8 rec_type); 
	AM_ERR UpdateIdx(AM_UINT ctts, AM_UINT stsz, AM_UINT stco);
	AM_ERR UpdateIdx2(AM_UINT stss);
	AM_ERR UpdateSps(AM_U8* sps, AM_UINT sps_size);
	AM_ERR UpdatePps(AM_U8* pps, AM_UINT pps_size);
	AM_ERR Mux(int* p_mdat_handle);
	
private:
	AM_ERR IdxWriter();
	AM_ERR IdxWriter2();
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
	void put_IsoFile(int mdat_handle);
	void put_FileTypeBox();
	void put_MovieBox();
	void put_videoTrackBox(AM_UINT TrackId, AM_UINT Duration);
	void put_VideoMediaBox(AM_UINT Duration);
	void put_VideoMediaInformationBox();
	void put_FreeSpaceBox(AM_UINT FreeSpaceSize);
	void put_MediaDataBox(int mdat_handle);

	void InitSdtp();

protected:
	char _filename[256];
	//file writer
	CamFileSink *_pSink;  //mp4 file writer
	CamFileSink *_pSinkIdx;	 //(tmp) index data writer
	CamFileSink *_pSinkIdx2;	 //(tmp) index data writer for stss
	AM_U64 _curPos;				 //next file written position for mp4 file writer
	
	//mp4 file information
	CMP4MUXER_CONFIG _Config;

	AM_UINT _CreationTime;		  //seconds from 1904-01-01 00:00:00 UTC
	AM_UINT _ModificationTime;	  //seconds from 1904-01-01 00:00:00 UTC

	AM_U16 _PixelAspectRatioWidth;  //0
	AM_U16 _PixelAspectRatioHeight; //0
	AM_U8 _sps[_SPS_SIZE];
	AM_UINT _spsSize;	  //_spsSize include _spsPad padding bytes to align to 4 bytes, but exclude the 0x00000001 start code
	AM_U8 _pps[_PPS_SIZE];
	AM_UINT _ppsSize;	  //_ppsSize include _ppsPad padding bytes to align to 4 bytes, but exclude the 0x00000001 start code
	AM_UINT _FreeSpaceSize;		 //free space size between ftyp atom and mdat atom (include free atom header 8 bytes)
	AM_UINT _MdatFreeSpaceSize;	 //free space size in mdat atom (excluding the mdat header itself)
	//xxx_SIZE macros need these 3 member
	AM_UINT _VPacketCnt;			//current video frame count
	AM_UINT _IdrPacketCnt;		  //current IDR frame count

	AM_UINT _SpsPps_pos;			//file position to store sps and pps (for recovery)
	//for sdtp atom
	AM_U8 _lv_idc[32];			  //[0]=last level, [1.._M+1], map frame to level idc

	AM_UINT _v_delta;			   //time count of one video frame
	AM_U64 _vdts;				   //video decoding time stamp, ==_v_delta*_VPacketCnt
	//CamH264BitstreamBuffer *_pH264Buffer0;//for field0 data when field mode encoding

	AM_UINT _frame_field_set;	   //0:init state, 1:frame mode, 2:field mode

	//index management
	AM_UINT _v_idx;
	//the following tmp buffers are located just before the mdat atom in the final mp4 file
	AM_UINT _vctts[_IDX_SIZE*2+128*2];	 //ctts.sample_count+sample_delta for video
	AM_UINT _vctts_fpos;			//file pos of the tmp file buffer for vctts
	AM_UINT _vstsz[_IDX_SIZE*1+128*1];	 //stsz.entry_size for video
	AM_UINT _vstsz_fpos;			//file pos of the tmp file buffer for vctts
	AM_UINT _vstco[_IDX_SIZE*1+128*1];	 //stco.chunk_offset for video
	AM_UINT _vstco_fpos;			//file pos of the tmp file buffer for vctts

	//index management for SyncSampleBox
	AM_UINT _v_ss_idx;
	AM_UINT _vstss[_IDX_SIZE*1+128*1];
	AM_UINT _vstss_fpos, _vstss_cur_fpos;
	
	AM_U64 _average_ctts;
};

//Class CamFileSink
class CamFileSink
{
public:
	CamFileSink();
	virtual ~CamFileSink();
	AM_INT Open(char* name, AM_UINT flags, AM_UINT mode);
	AM_INT Read(AM_U8 *data, AM_INT len);
	AM_U64 Seek(AM_U64 offset, AM_INT whence);
	AM_INT Write(AM_U8 *data, AM_INT len);
	AM_ERR Close();	

	//actually write the buffer content to file
	void _FlushBuffer();


protected:
	AM_U8 _buf[_SINK_BUF_SIZE];
	AM_INT _cnt;
	AM_INT _handle;
};

#endif //__MP4MUXER_H__
