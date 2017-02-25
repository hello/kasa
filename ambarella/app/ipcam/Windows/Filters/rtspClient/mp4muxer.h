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

class CAmbaFileSink;

#define AM_TRUE	 1
#define AM_FALSE	0

#define _IDX_SIZE 256 //keep it equal 2^N, 8.5 seconds of video@30fps, or 5.3 seconds of audio@48Khz
#define _SPS_SIZE 64
#define _PPS_SIZE 64

#define _SINK_BUF_SIZE	32000 //==0x7d00
#define PTS_UNIT 90000

typedef unsigned char AM_U8;
typedef unsigned short AM_U16;
typedef unsigned long AM_U32;
typedef unsigned long long AM_U64;
typedef unsigned int AM_UINT;
typedef int AM_INT;
typedef AM_U64 AM_PTS;

enum AM_ERR
{
	ME_OK = 0,
	ME_WAIT,
	ME_ERROR,
	ME_CLOSED,
	ME_BUSY,
	ME_INTERNAL_ERROR,
	ME_NO_IMPL,
	ME_OS_ERROR,
	ME_IO_ERROR,
	ME_TIMEOUT,
	ME_NO_MEMORY,
	ME_TOO_MANY,

	ME_NOT_EXIST,
	ME_NOT_SUPPORT,
	ME_NO_INTERFACE,
	ME_BAD_STATE,
	ME_BAD_PARAM,
	ME_BAD_COMMAND,

	ME_UNKNOWN_FMT,
};

typedef struct h264_config_s
{
	unsigned short _width;
	unsigned short _height;
	//unsigned short _scale;
	//unsigned short _rate;
}h264_config_t;

class CAmbaMp4muxer
{
public:
	CAmbaMp4muxer(char* filename);
	virtual ~CAmbaMp4muxer();

public:
	AM_ERR Init(AM_U8 rec_type);
	AM_ERR UpdateIdx(AM_U64 pts, AM_UINT stsz, AM_UINT stco);
	AM_ERR UpdateIdx2(AM_UINT stss);
	AM_ERR UpdateSps(AM_U8* sps, AM_UINT sps_size);
	AM_ERR UpdatePps(AM_U8* pps, AM_UINT pps_size);
	AM_ERR Config(h264_config_t h264_config);
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

protected:
	char _filename[256];
	AM_U8 _filetype;
	//file writer
	CAmbaFileSink *_pSink;  //mp4 file writer
	CAmbaFileSink *_pSinkIdx;	 //(tmp) index data writer
	CAmbaFileSink *_pSinkIdx2;	 //(tmp) index data writer for stss
	AM_U64 _curPos;				 //next file written position for mp4 file writer

	//mp4 file information
	h264_config_t _Config;

	AM_UINT _CreationTime;		  //seconds from 1904-01-01 00:00:00 UTC
	AM_UINT _ModificationTime;	  //seconds from 1904-01-01 00:00:00 UTC

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

	AM_UINT _average_ctts;
};

//Class CAmbaFileSink
class CAmbaFileSink
{
public:
	CAmbaFileSink();
	virtual ~CAmbaFileSink();
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
