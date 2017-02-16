/*
 * iso_muxer.cpp
 *
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <streams.h>
#include <Ks.h>
#include <KsMedia.h>
#include "iso_muxer.h"
#include "fambademo.h"


extern CAmbaRecordControl *G_pRecordControl;
extern MUXER_PARAM  *G_pMuxerParam;

#define _ScaledWidth		_Config._Width
#define _ScaledHeight		_Config._Height
#define FOURCC(a,b,c,d)		((a<<24)|(b<<16)|(c<<8)|(d))
#define CLOCK				90000

/*static struct
{
	AM_UINT width;
	AM_UINT height; 
	ULONG mode;
}

vout_resolution[] =
{
	{720,480, AMBA_VIDEO_MODE_480I},
	{720,576, AMBA_VIDEO_MODE_576I},
	{720,480, AMBA_VIDEO_MODE_D1_NTSC},
	{720,576, AMBA_VIDEO_MODE_D1_PAL},
	{1280,720, AMBA_VIDEO_MODE_720P},
	{1280,720, AMBA_VIDEO_MODE_720P_PAL},
	{1920,1080, AMBA_VIDEO_MODE_1080I},
	{1920,1080, AMBA_VIDEO_MODE_1080I_PAL},
};

#define NUM_VOUT_RESOLUTION (sizeof(vout_resolution) / sizeof(vout_resolution[0]))*/

CamIsoMuxer::CamIsoMuxer(AM_UINT vin_cnt, AM_UINT ain_cnt):
	_InputVCnt(vin_cnt),
	_InputACnt(ain_cnt),
	_pSink(NULL),
	_pSinkIdx(NULL),
	m_sp264(true)
{
}

CamIsoMuxer::~CamIsoMuxer()
{
	if (_pSink != NULL) delete _pSink;
	if (_pSinkIdx != NULL) delete _pSinkIdx;
}


//--------------------------------------------------

AM_UINT GetByte(BYTE *pBuffer,AM_UINT pos)
{ 
	return pBuffer[pos];
}

void CamIsoMuxer::put_byte(AM_UINT data)
{
	AM_U8 w[1];
	w[0] = data;	  //(data&0xFF);
	_pSink->Write((AM_U8 *)&w, 1);
	_curPos++;
}

void CamIsoMuxer::put_be16(AM_UINT data)
{
	AM_U8 w[2];
	/*w[1] = data;	  //(data&0x00FF);
	w[0] = data>>8;   //(data&0xFF00)>>8;*/
	w[1] = (AM_U8)(data&0x00FF);
	w[0] = (AM_U8)((data&0xFF00)>>8);
	_pSink->Write((AM_U8 *)&w, 2);
	_curPos += 2;
}

void CamIsoMuxer::put_be24(AM_UINT data)
{
	AM_U8 w[3];
	/*w[2] = data;	 //(data&0x0000FF);
	w[1] = data>>8;  //(data&0x00FF00)>>8;
	w[0] = data>>16; //(data&0xFF0000)>>16;*/
	w[2] = (AM_U8)(data&0x0000FF);
	w[1] = (AM_U8)((data&0x00FF00)>>8);
	w[0] = (AM_U8)((data&0xFF0000)>>16);
	_pSink->Write((AM_U8 *)&w, 3);
	_curPos += 3;
}

void CamIsoMuxer::put_be32(AM_UINT data)
{
	AM_U8 w[4];
	/*w[3] = data;	 //(data&0x000000FF);
	w[2] = data>>8;  //(data&0x0000FF00)>>8;
	w[1] = data>>16; //(data&0x00FF0000)>>16;
	w[0] = data>>24; //(data&0xFF000000)>>24;*/
	w[3] = (AM_U8)(data&0x000000FF);
	w[2] = (AM_U8)((data&0x0000FF00)>>8);
	w[1] = (AM_U8)((data&0x00FF0000)>>16);
	w[0] = (AM_U8)((data&0xFF000000)>>24);
	_pSink->Write((AM_U8 *)&w, 4);
	_curPos += 4;
}

void CamIsoMuxer::put_be64(AM_U64 data)
{
	AM_U8 w[8];
	/*w[7] = data;	 //(data&000000000x000000FFULL);
	w[6] = data>>8;  //(data&0x000000000000FF00ULL)>>8;
	w[5] = data>>16; //(data&0x0000000000FF0000ULL)>>16;
	w[4] = data>>24; //(data&0x00000000FF000000ULL)>>24;
	w[3] = data>>32; //(data&0x000000FF00000000ULL)>>32;
	w[2] = data>>40; //(data&0x0000FF0000000000ULL)>>40;
	w[1] = data>>48; //(data&0x00FF000000000000ULL)>>48;
	w[0] = data>>56; //(data&0xFF00000000000000ULL)>>56;*/
	w[7] = (AM_U8)(data&0x00000000000000FFULL);
	w[6] = (AM_U8)((data&0x000000000000FF00ULL)>>8);
	w[5] = (AM_U8)((data&0x0000000000FF0000ULL)>>16);
	w[4] = (AM_U8)((data&0x00000000FF000000ULL)>>24);
	w[3] = (AM_U8)((data&0x000000FF00000000ULL)>>32);
	w[2] = (AM_U8)((data&0x0000FF0000000000ULL)>>40);
	w[1] = (AM_U8)((data&0x00FF000000000000ULL)>>48);
	w[0] = (AM_U8)((data&0xFF00000000000000ULL)>>56);
	_pSink->Write((AM_U8 *)&w, 8);

	_curPos += 8;
}

void CamIsoMuxer::put_buffer(AM_U8 *pdata, AM_UINT size)
{
	_pSink->Write(pdata, size);
	//_pSink->Sync();
	_curPos += size;
}

AM_UINT CamIsoMuxer::get_byte()
{
	AM_U8 data[1];
	_pSink->Read(data, 1);
	return data[0];
}

AM_UINT CamIsoMuxer::get_be16()
{
	AM_U8 data[2];
	_pSink->Read(data, 2);
	return (data[1] | (data[0]<<8));
}

AM_UINT CamIsoMuxer::get_be32()
{
	AM_U8 data[4];
	_pSink->Read(data, 4);
	return (data[3] | (data[2]<<8) | (data[1]<<16) | (data[0]<<24));
}

void CamIsoMuxer::get_buffer(AM_U8 *pdata, AM_UINT size)
{
	_pSink->Read(pdata, size);
}

AM_UINT CamIsoMuxer::le_to_be32(AM_UINT data)
{
	AM_UINT rval;
	rval  = (data&0x000000FF)<<24;
	rval += (data&0x0000FF00)<<8;
	rval += (data&0x00FF0000)>>8;
	rval += (data&0xFF000000)>>24;
	return rval;
}

AM_U64 CamIsoMuxer::le_to_be64(AM_U64 data)
{
	AM_U64 rval = 0ULL;
   /* AM_U8 tmp;
	tmp = data;	 rval += (AM_U64)tmp<<56;
	tmp = data>>8;  rval += (AM_U64)tmp<<48;
	tmp = data>>16; rval += (AM_U64)tmp<<40;
	tmp = data>>24; rval += (AM_U64)tmp<<32;
	tmp = data>>32; rval += tmp<<24;
	tmp = data>>40; rval += tmp<<16;
	tmp = data>>48; rval += tmp<<8;
	tmp = data>>56; rval += tmp;*/
	rval += (data&&0x00000000000000FFULL)<<56;
	rval += (data&&0x000000000000FF00ULL)>>8<<48;
	rval += (data&&0x0000000000FF0000ULL)>>16<<40;
	rval += (data&&0x00000000FF000000ULL)>>24<<32;
	rval += (data&&0x000000FF00000000ULL)>>32<<24;
	rval += (data&&0x0000FF0000000000ULL)>>40<<16;
	rval += (data&&0x00FF000000000000ULL)>>48<<8;
	rval += (data&&0xFF00000000000000ULL)>>56;

	return rval;
}
//--------------------------------------------------

#define VideoMediaHeaderBox_SIZE			20
#define DataReferenceBox_SIZE			   28
#define DataInformationBox_SIZE			 (8+DataReferenceBox_SIZE) // 36
#define PixelAspectRatioBox_SIZE			16
#define CleanApertureBox_SIZE			   40
#define AvcConfigurationBox_SIZE			(19+_spsSize+_ppsSize)
#define BitrateBox_SIZE					 20
#define VisualSampleEntry_SIZE			  (86+PixelAspectRatioBox_SIZE+CleanApertureBox_SIZE+AvcConfigurationBox_SIZE+BitrateBox_SIZE) // 181+_spsSize+_ppsSize
#define VideoSampleDescriptionBox_SIZE	  (16+VisualSampleEntry_SIZE) // 197+_spsSize+_ppsSize
#define DecodingTimeToSampleBox_SIZE		24
#define CompositionTimeToSampleBox_SIZE	 (16+(_VPacketCnt<<3))
#define CompositionTimeToSampleBox_MAXSIZE  (16+(_MaxVPacketCnt<<3))
#define SampleToChunkBox_SIZE			   28
#define VideoSampleSizeBox_SIZE			 (20+(_VPacketCnt<<2))
#define VideoSampleSizeBox_MAXSIZE		  (20+(_MaxVPacketCnt<<2))
#define VideoChunkOffsetBox_SIZE64		  (16+(_VPacketCnt<<3))
#define VideoChunkOffsetBox_MAXSIZE64	   (16+(_MaxVPacketCnt<<3))
#define VideoChunkOffsetBox_SIZE			(16+(_VPacketCnt<<2))
#define VideoChunkOffsetBox_MAXSIZE		 (16+(_MaxVPacketCnt<<2))
#define SyncSampleBox_SIZE				  (16+(_IdrPacketCnt<<2))
#define SyncSampleBox_MAXSIZE			   (16+(_MaxIdrPacketCnt<<2))
#define SampleDependencyTypeBox_SIZE		(12+_VPacketCnt)
#define SampleDependencyTypeBox_MAXSIZE	 (12+_MaxVPacketCnt)
#define VideoSampleTableBox_SIZE64		  (8+VideoSampleDescriptionBox_SIZE+\
											 DecodingTimeToSampleBox_SIZE+\
											 CompositionTimeToSampleBox_SIZE+\
											 SampleToChunkBox_SIZE+\
											 VideoSampleSizeBox_SIZE+\
											 VideoChunkOffsetBox_SIZE64+\
											 SyncSampleBox_SIZE+\
											 SampleDependencyTypeBox_SIZE) // 337+_spsSize+_ppsSize+V*21+IDR*4
#define VideoSampleTableBox_MAXSIZE64	   (8+VideoSampleDescriptionBox_SIZE+\
											 DecodingTimeToSampleBox_SIZE+\
											 CompositionTimeToSampleBox_MAXSIZE+\
											 SampleToChunkBox_SIZE+\
											 VideoSampleSizeBox_MAXSIZE+\
											 VideoChunkOffsetBox_MAXSIZE64+\
											 SyncSampleBox_MAXSIZE+\
											 SampleDependencyTypeBox_MAXSIZE) // 337+_spsSize+_ppsSize+V*17+IDR*4
#define VideoSampleTableBox_SIZE			(8+VideoSampleDescriptionBox_SIZE+\
											 DecodingTimeToSampleBox_SIZE+\
											 CompositionTimeToSampleBox_SIZE+\
											 SampleToChunkBox_SIZE+\
											 VideoSampleSizeBox_SIZE+\
											 VideoChunkOffsetBox_SIZE+\
											 SyncSampleBox_SIZE+\
											 SampleDependencyTypeBox_SIZE) // 337+_spsSize+_ppsSize+V*17+IDR*4
#define VideoSampleTableBox_MAXSIZE		 (8+VideoSampleDescriptionBox_SIZE+\
											 DecodingTimeToSampleBox_SIZE+\
											 CompositionTimeToSampleBox_MAXSIZE+\
											 SampleToChunkBox_SIZE+\
											 VideoSampleSizeBox_MAXSIZE+\
											 VideoChunkOffsetBox_MAXSIZE+\
											 SyncSampleBox_MAXSIZE+\
											 SampleDependencyTypeBox_MAXSIZE) // 337+_spsSize+_ppsSize+V*17+IDR*4
#define VideoMediaInformationBox_SIZE64	 (8+VideoMediaHeaderBox_SIZE+\
											 DataInformationBox_SIZE+\
											 VideoSampleTableBox_SIZE64) // 401+_spsSize+_ppsSize+V*21+IDR*4
#define VideoMediaInformationBox_MAXSIZE64  (8+VideoMediaHeaderBox_SIZE+\
											 DataInformationBox_SIZE+\
											 VideoSampleTableBox_MAXSIZE64) // 401+_spsSize+_ppsSize+V*21+IDR*4
#define VideoMediaInformationBox_SIZE	   (8+VideoMediaHeaderBox_SIZE+\
											 DataInformationBox_SIZE+\
											 VideoSampleTableBox_SIZE) // 401+_spsSize+_ppsSize+V*17+IDR*4
#define VideoMediaInformationBox_MAXSIZE	(8+VideoMediaHeaderBox_SIZE+\
											 DataInformationBox_SIZE+\
											 VideoSampleTableBox_MAXSIZE) // 401+_spsSize+_ppsSize+V*17+IDR*4
void CamIsoMuxer::put_VideoMediaInformationBox()
{
	AM_UINT i, j, k, idr_frm_no;
	//MediaInformationBox
	if (_largeFile == true)
	{
	put_be32(VideoMediaInformationBox_SIZE64);   //uint32 size
	}
	else
	{
	put_be32(VideoMediaInformationBox_SIZE);	 //uint32 size
	}
	put_be32(FOURCC('m', 'i', 'n', 'f'));		//'minf'

	//VideoMediaHeaderBox
	put_be32(VideoMediaHeaderBox_SIZE);		  //uint32 size
	put_be32(FOURCC('v', 'm', 'h', 'd'));		//'vmhd'
	put_byte(0);								 //uint8 version
	//This is a compatibility flag that allows QuickTime to distinguish between movies created with QuickTime
	//1.0 and newer movies. You should always set this flag to 1, unless you are creating a movie intended
	//for playback using version 1.0 of QuickTime
	put_be24(1);								 //bits24 flags
	put_be16(0);								 //uint16 graphicsmode  //0=copy over the existing image
	put_be16(0);								 //uint16 opcolor[3]	  //(red, green, blue)
	put_be16(0);
	put_be16(0);

	//DataInformationBox
	put_be32(DataInformationBox_SIZE);		   //uint32 size
	put_be32(FOURCC('d', 'i', 'n', 'f'));		//'dinf'
	//DataReferenceBox
	put_be32(DataReferenceBox_SIZE);			 //uint32 size
	put_be32(FOURCC('d', 'r', 'e', 'f'));		//'dref'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(1);								 //uint32 entry_count
	put_be32(12);								//uint32 size
	put_be32(FOURCC('u', 'r', 'l', ' '));		//'url '
	put_byte(0);								 //uint8 version
	put_be24(1);								 //bits24 flags	//1=media data is in the same file as the MediaBox

	//SampleTableBox
	if (_largeFile == true)
	{
	put_be32(VideoSampleTableBox_SIZE64);		//uint32 size
	}
	else
	{
	put_be32(VideoSampleTableBox_SIZE);		  //uint32 size
	}
	put_be32(FOURCC('s', 't', 'b', 'l'));		//'stbl'

	//SampleDescriptionBox
	put_be32(VideoSampleDescriptionBox_SIZE);	//uint32 size
	put_be32(FOURCC('s', 't', 's', 'd'));		//uint32 type
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(1);								 //uint32 entry_count
	//VisualSampleEntry
	AM_U8 EncoderName[31]="Ambarella AVC encoder\0\0\0\0\0\0\0\0\0";
	put_be32(VisualSampleEntry_SIZE);			//uint32 size
	put_be32(FOURCC('a', 'v', 'c', '1'));		//'avc1'
	put_byte(0);								 //uint8 reserved[6]
	put_byte(0);
	put_byte(0);
	put_byte(0);
	put_byte(0);
	put_byte(0);
	put_be16(1);								 //uint16 data_reference_index
	put_be16(0);								 //uint16 pre_defined
	put_be16(0);								 //uint16 reserved
	put_be32(0);								 //uint32 pre_defined[3]
	put_be32(0);
	put_be32(0);
	put_be16(_Config._Width);					//uint16 width
	put_be16(_Config._Height);				   //uint16 height
	put_be32(0x00480000);						//uint32 horizresolution  72dpi
	put_be32(0x00480000);						//uint32 vertresolution   72dpi
	put_be32(0);								 //uint32 reserved
	put_be16(1);								 //uint16 frame_count
	AM_UINT len = strlen((char *)EncoderName);
	put_byte(len);							   //char compressor_name[32]   //[0] is actual length
	put_buffer(EncoderName, 31);
	put_be16(0x0018);							//uint16 depth   //0x0018=images are in colour with no alpha
	put_be16(-1);								//int16 pre_defined
	//PixelAspectRatioBox
	put_be32(PixelAspectRatioBox_SIZE);		  //uint32 size
	put_be32(FOURCC('p', 'a', 's', 'p'));		//'pasp'
	put_be32(_PixelAspectRatioWidth);			//uint32 hSpacing
	put_be32(_PixelAspectRatioHeight);		   //uint32 vSpacing
	//CleanApertureBox
	put_be32(CleanApertureBox_SIZE);			 //uint32 size
	put_be32(FOURCC('c', 'l', 'a', 'p'));		//'clap'
	put_be32(_Config._Width);					//int32 apertureWidth_N (numerator)
	put_be32(1);								 //int32 apertureWidth_D (denominator)
	put_be32(_Config._Height);				   //int32 apertureHeight_N (numerator)
	put_be32(1);								 //int32 apertureHeight_D (denominator)
	put_be32(0);								 //int32 horizOff_N (numerator)
	put_be32(1);								 //int32 horizOff_D (denominator)
	put_be32(0);								 //int32 vertOff_N (numerator)
	put_be32(1);								 //int32 vertOff_D (denominator)
	//AvcConfigurationBox
	put_be32(AvcConfigurationBox_SIZE);		  //uint32 size
	
	DbgLog((LOG_TRACE, 1, TEXT("AvcConfigurationBox_SIZE=%u"),AvcConfigurationBox_SIZE));
	put_be32(FOURCC('a', 'v', 'c', 'C'));		//'avcC'
	put_byte(1);								 //uint8 configurationVersion
	put_byte(_sps[1]);						   //uint8 AVCProfileIndication
	put_byte(_sps[2]);						   //uint8 profile_compatibility
	put_byte(_sps[3]);						   //uint8 level
	put_byte(0xFF);							  //uint8 nal_length  //(nal_length&0x03)+1 [reserved:6, lengthSizeMinusOne:2]
	put_byte(0xE1);							  //uint8 sps_count  //sps_count&0x1f [reserved:3, numOfSequenceParameterSets:5]
	put_be16(_spsSize);						  //uint16 sps_size	  //sequenceParameterSetLength
	put_buffer(_sps, _spsSize);				  //uint8 sps[sps_size] //sequenceParameterSetNALUnit
	put_byte(1);								 //uint8 pps_count	  //umOfPictureParameterSets
	put_be16(_ppsSize);						  //uint16 pps_size	  //pictureParameterSetLength

	put_buffer(_pps, _ppsSize);				  //uint8 pps[pps_size] //pictureParameterSetNALUnit
	DbgLog((LOG_TRACE, 1, TEXT("_pps=%d, _ppsSize=%d"),_pps, _ppsSize));				   
	//BitrateBox
	put_be32(BitrateBox_SIZE);				   //uint32 size
	put_be32(FOURCC('b', 't', 'r', 't'));		//'btrt'
	put_be32(0);								 //uint32 buffer_size
	put_be32(0);								 //uint32 max_bitrate
	put_be32(0);								 //uint32 avg_bitrate

	//DecodingTimeToSampleBox
	put_be32(DecodingTimeToSampleBox_SIZE);	  //uint32 size
	put_be32(FOURCC('s', 't', 't', 's'));		//'stts'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(1);								 //uint32 entry_count
	put_be32(_VPacketCnt);					   //uint32 sample_count
	AM_UINT delta = (AM_UINT)(1.0*_frame_field_set*_Config._scale*_Config._clk/_Config._rate);
	put_be32(delta);							 //uint32 sample_delta

	//CompositionTimeToSampleBox
	put_be32(CompositionTimeToSampleBox_SIZE);   //uint32 size
	put_be32(FOURCC('c', 't', 't', 's'));		//'ctts'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(_VPacketCnt);					   //uint32 entry_count
	if (_VPacketCnt > 0) _pSinkIdx->Seek(_vctts_fpos, SEEK_SET);
	for (i=0; i<_VPacketCnt/_IDX_SIZE; i++)	  //uint32 sample_count + uint32 sample_delta
	{
		_pSinkIdx->Read((AM_U8 *)_vctts, _IDX_SIZE*2*sizeof(AM_UINT));
		put_buffer((AM_U8 *)_vctts, _IDX_SIZE*2*sizeof(AM_UINT));
	}
	if ((j=_VPacketCnt%_IDX_SIZE) > 0)
	{
		_pSinkIdx->Read((AM_U8 *)_vctts, j*2*sizeof(AM_UINT));
		put_buffer((AM_U8 *)_vctts, j*2*sizeof(AM_UINT));
	}

	//SampleToChunkBox
	put_be32(SampleToChunkBox_SIZE);			 //uint32 size
	put_be32(FOURCC('s', 't', 's', 'c'));		//'stsc'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(1);								 //uint32 entry_count
	put_be32(1);								 //uint32 first_chunk
	put_be32(1);								 //uint32 samples_per_chunk
	put_be32(1);								 //uint32 sample_description_index

	//SampleSizeBox
	put_be32(VideoSampleSizeBox_SIZE);		   //uint32 size
	put_be32(FOURCC('s', 't', 's', 'z'));		//'stsz'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(0);								 //uint32 sample_size
	put_be32(_VPacketCnt);					   //uint32 sample_count
	if (_VPacketCnt > 0) _pSinkIdx->Seek(_vstsz_fpos, SEEK_SET);
	for (i=0; i<_VPacketCnt/_IDX_SIZE; i++)	  //uint32 entry_size[sample_count] (if sample_size==0)
	{
		_pSinkIdx->Read((AM_U8 *)_vstsz, _IDX_SIZE*sizeof(AM_UINT));
		put_buffer((AM_U8 *)_vstsz, _IDX_SIZE*sizeof(AM_UINT));
	}
	if ((j=_VPacketCnt%_IDX_SIZE) > 0)
	{
		_pSinkIdx->Read((AM_U8 *)_vstsz, j*sizeof(AM_UINT));
		put_buffer((AM_U8 *)_vstsz, j*sizeof(AM_UINT));
	}

	//ChunkOffsetBox
	if (_largeFile == true)
	{
		put_be32(VideoChunkOffsetBox_SIZE64);		//uint32 size
		put_be32(FOURCC('c', 'o', '6', '4'));		//'co64'
		put_byte(0);								 //uint8 version
		put_be24(0);								 //bits24 flags
		put_be32(_VPacketCnt);					   //uint32 entry_count
		if (_VPacketCnt > 0) _pSinkIdx->Seek(_vstco_fpos, SEEK_SET);
		for (i=0; i<_VPacketCnt/_IDX_SIZE; i++)	  //uint64 chunk_offset[entry_count]
		{
			_pSinkIdx->Read((AM_U8 *)_vstco, _IDX_SIZE*sizeof(AM_U64));
			put_buffer((AM_U8 *)_vstco, _IDX_SIZE*sizeof(AM_U64));
		}
		if ((j=_VPacketCnt%_IDX_SIZE) > 0)
		{
			_pSinkIdx->Read((AM_U8 *)_vstco, j*sizeof(AM_U64));
			put_buffer((AM_U8 *)_vstco, j*sizeof(AM_U64));
		}
	}
	else
	{
		put_be32(VideoChunkOffsetBox_SIZE);		  //uint32 size
		put_be32(FOURCC('s', 't', 'c', 'o'));		//'stco'
		put_byte(0);								 //uint8 version
		put_be24(0);								 //bits24 flags
		put_be32(_VPacketCnt);					   //uint32 entry_count
		if (_VPacketCnt > 0) _pSinkIdx->Seek(_vstco_fpos, SEEK_SET);
		for (i=0; i<_VPacketCnt/_IDX_SIZE; i++)	  //uint32 chunk_offset[entry_count]
		{
			_pSinkIdx->Read((AM_U8 *)_vstco, _IDX_SIZE*sizeof(AM_UINT));
			put_buffer((AM_U8 *)_vstco, _IDX_SIZE*sizeof(AM_UINT));

		}
		if ((j=_VPacketCnt%_IDX_SIZE) > 0)
		{
			_pSinkIdx->Read((AM_U8 *)_vstco, j*sizeof(AM_UINT));
			DbgLog((LOG_TRACE, 1, TEXT("_VPacketCnt=%d"),_VPacketCnt));
			put_buffer((AM_U8 *)_vstco, j*sizeof(AM_UINT));
		}
	}

	//SyncSampleBox
	put_be32(SyncSampleBox_SIZE);				//uint32 size
	put_be32(FOURCC('s', 't', 's', 's'));		//'stss'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(_IdrPacketCnt);					 //uint32 entry_count
	if (_IdrPacketCnt > 0)					   //uint32 sample_number[entry_count]
	{
		idr_frm_no = 1;
		DbgLog((LOG_TRACE, 1, TEXT("idr_frm_no=%d"),idr_frm_no));
		put_be32(idr_frm_no);					//1st idr
	}
	if (_Config._new_gop == 1)
	{
		idr_frm_no += _Config._idr_interval*_Config._N-(_Config._M-1);
		if (_IdrPacketCnt > 1)
		{
			put_be32(idr_frm_no);				//2nd idr
		}
		k = 2;
	}
	else
	{
		k = 1;
	}
	for (i=k; i<_IdrPacketCnt; i++)
	{
		idr_frm_no += _Config._idr_interval*_Config._N;
		put_be32(idr_frm_no);
	}
	DbgLog((LOG_TRACE, 1, TEXT("_IdrPacketCnt=%d"),_IdrPacketCnt));
	//SampleDependencyTypeBox
	put_be32(SampleDependencyTypeBox_SIZE);	  //uint32 size
	put_be32(FOURCC('s', 'd', 't', 'p'));		//'sdtp'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	//SampleSizeBox.sample_count entries
	if (_VPacketCnt > 0) put_byte(0x00);		 //first entry		
	
	for (i=1; i<_VPacketCnt; i++)
	{
		if (_Config._new_gop == 1)
		{
			k = i;
		}
		else
		{
			k = i+1;
		}
		k = k%_Config._M; if (k==0) k=_Config._M;
		if (_lv_idc[k] == _lv_idc[0])
		{
			put_byte(0x08);//no other sample depends on this one (disposable)
			DbgLog((LOG_TRACE, 1, TEXT("enter 0x08,k=%d"),k));
		}
		else
		{
			put_byte(0x00);
		}
	}
}

#define ElementaryStreamDescriptorBox_SIZE  50
#define AudioSampleEntry_SIZE			   (36+ElementaryStreamDescriptorBox_SIZE) // 86
#define AudioSampleDescriptionBox_SIZE	  (16+AudioSampleEntry_SIZE) // 102
#define AudioSampleSizeBox_SIZE			 (20+(_APacketCnt<<2))
#define AudioSampleSizeBox_MAXSIZE		  (20+(_MaxAPacketCnt<<2))
#define AudioChunkOffsetBox_SIZE64		  (16+(_APacketCnt<<3))
#define AudioChunkOffsetBox_MAXSIZE64	   (16+(_MaxAPacketCnt<<3))
#define AudioChunkOffsetBox_SIZE			(16+(_APacketCnt<<2))
#define AudioChunkOffsetBox_MAXSIZE		 (16+(_MaxAPacketCnt<<2))
#define AudioSampleTableBox_SIZE64		  (8+AudioSampleDescriptionBox_SIZE+\
											 DecodingTimeToSampleBox_SIZE+\
											 SampleToChunkBox_SIZE+\
											 AudioSampleSizeBox_SIZE+\
											 AudioChunkOffsetBox_SIZE64) // 198+A*12
#define AudioSampleTableBox_MAXSIZE64	   (8+AudioSampleDescriptionBox_SIZE+\
											 DecodingTimeToSampleBox_SIZE+\
											 SampleToChunkBox_SIZE+\
											 AudioSampleSizeBox_MAXSIZE+\
											 AudioChunkOffsetBox_MAXSIZE64) // 198+A*12
#define AudioSampleTableBox_SIZE			(8+AudioSampleDescriptionBox_SIZE+\
											 DecodingTimeToSampleBox_SIZE+\
											 SampleToChunkBox_SIZE+\
											 AudioSampleSizeBox_SIZE+\
											 AudioChunkOffsetBox_SIZE) // 198+A*8
#define AudioSampleTableBox_MAXSIZE		 (8+AudioSampleDescriptionBox_SIZE+\
											 DecodingTimeToSampleBox_SIZE+\
											 SampleToChunkBox_SIZE+\
											 AudioSampleSizeBox_MAXSIZE+\
											 AudioChunkOffsetBox_MAXSIZE) // 198+A*8
#define SoundMediaHeaderBox_SIZE			16
#define AudioMediaInformationBox_SIZE64	 (8+SoundMediaHeaderBox_SIZE+\
											 DataInformationBox_SIZE+\
											 AudioSampleTableBox_SIZE64) // 258+A*12
#define AudioMediaInformationBox_MAXSIZE64  (8+SoundMediaHeaderBox_SIZE+\
											 DataInformationBox_SIZE+\
											 AudioSampleTableBox_MAXSIZE64) // 258+A*12
#define AudioMediaInformationBox_SIZE	   (8+SoundMediaHeaderBox_SIZE+\
											 DataInformationBox_SIZE+\
											 AudioSampleTableBox_SIZE) // 258+A*8
#define AudioMediaInformationBox_MAXSIZE	(8+SoundMediaHeaderBox_SIZE+\
											 DataInformationBox_SIZE+\
											 AudioSampleTableBox_MAXSIZE) // 258+A*8

void CamIsoMuxer::put_AudioMediaInformationBox()
{
	AM_UINT i, j;
	//MediaInformationBox
	if (_largeFile == true)
	{
	put_be32(AudioMediaInformationBox_SIZE64);   //uint32 size
	}
	else
	{
	put_be32(AudioMediaInformationBox_SIZE);	 //uint32 size
	}
	put_be32(FOURCC('m', 'i', 'n', 'f'));		//'minf'

	//SoundMediaHeaderBox
	put_be32(SoundMediaHeaderBox_SIZE);		  //uint32 size
	put_be32(FOURCC('s', 'm', 'h', 'd'));		//'smhd'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be16(0);								 //int16 balance
	put_be16(0);								 //uint16 reserved

	//DataInformationBox
	put_be32(DataInformationBox_SIZE);		   //uint32 size
	put_be32(FOURCC('d', 'i', 'n', 'f'));		//'dinf'
	//DataReferenceBox
	put_be32(DataReferenceBox_SIZE);			 //uint32 size
	put_be32(FOURCC('d', 'r', 'e', 'f'));		//'dref'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(1);								 //uint32 entry_count
	put_be32(12);								//uint32 size
	put_be32(FOURCC('u', 'r', 'l', ' '));		//'url '
	put_byte(0);								 //uint8 version
	put_be24(1);								 //bits24 flags	//1=media data is in the same file as the MediaBox

	//SampleTableBox
	if (_largeFile == true)
	{
		put_be32(AudioSampleTableBox_SIZE64);		//uint32 size
	}
	else
	{
		put_be32(AudioSampleTableBox_SIZE);		  //uint32 size
	}
	put_be32(FOURCC('s', 't', 'b', 'l'));		//'stbl'

	//SampleDescriptionBox
	put_be32(AudioSampleDescriptionBox_SIZE);	//uint32 size
	put_be32(FOURCC('s', 't', 's', 'd'));		//uint32 type
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(1);								 //uint32 entry_count
	//AudioSampleEntry
	put_be32(AudioSampleEntry_SIZE);			//uint32 size
	put_be32(FOURCC('m', 'p', '4', 'a'));		//'mp4a'
	put_byte(0);								 //uint8 reserved[6]
	put_byte(0);
	put_byte(0);
	put_byte(0);
	put_byte(0);
	put_byte(0);
	put_be16(1);								 //uint16 data_reference_index
	put_be32(0);								 //uint32 reserved[2]
	put_be32(0);
	put_be16(_Config._Channels);				 //uint16 channelcount
	put_be16(_Config._SampleSize);			   //uint16 samplesize
	put_be16(0xfffe);							//uint16 pre_defined   //for QT sound
	put_be16(0);								 //uint16 reserved
	put_be32(_Config._SampleFreq<<16);		   //uint32 samplerate   //= (timescale of media << 16)
	//ElementaryStreamDescriptorBox
	put_be32(ElementaryStreamDescriptorBox_SIZE);//uint32 size
	put_be32(FOURCC('e', 's', 'd', 's'));		//'esds'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	//ES descriptor takes 38 bytes
	put_byte(3);								 //ES descriptor type tag 
	put_be16(0x8080);
	put_byte(34);								//descriptor type length
	put_be16(0);								 //ES ID
	put_byte(0);								 //stream priority
	//Decoder config descriptor takes 26 bytes (include decoder specific info)
	put_byte(4);								 //decoder config descriptor type tag
	put_be16(0x8080);
	put_byte(22);								//descriptor type length
	put_byte(0x40);							  //object type ID MPEG-4 audio=64 AAC
	put_byte(0x15);							  //stream type:6, upstream flag:1, reserved flag:1 (audio=5)	Audio stream
	put_be24(8192);							  // buffer size
	put_be32(128000);							// max bitrate
	put_be32(128000);							// avg bitrate
	//Decoder specific info descriptor takes 9 bytes
	put_byte(5);								 //decoder specific descriptor type tag
	put_be16(0x8080);
	put_byte(5);								 //descriptor type length
	put_be16(GetAacInfo(_Config._SampleFreq, _Config._Channels));
	put_be16(0x0000);
	put_byte(0x00);
	//SL descriptor takes 5 bytes
	put_byte(6);								 //SL config descriptor type tag 
	put_be16(0x8080);
	put_byte(1);								 //descriptor type length
	put_byte(2);								 //SL value

	//DecodingTimeToSampleBox
	put_be32(DecodingTimeToSampleBox_SIZE);	  //uint32 size
	put_be32(FOURCC('s', 't', 't', 's'));		//'stts'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(1);								 //uint32 entry_count
	put_be32(_APacketCnt);					   //uint32 sample_count
	put_be32(1024);							  //uint32 sample_delta

	//SampleToChunkBox
	put_be32(SampleToChunkBox_SIZE);			 //uint32 size
	put_be32(FOURCC('s', 't', 's', 'c'));		//'stsc'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(1);								 //uint32 entry_count
	put_be32(1);								 //uint32 first_chunk
	put_be32(1);								 //uint32 samples_per_chunk
	put_be32(1);								 //uint32 sample_description_index

	//SampleSizeBox
	put_be32(AudioSampleSizeBox_SIZE);		   //uint32 size
	put_be32(FOURCC('s', 't', 's', 'z'));		//'stsz'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(0);								 //uint32 sample_size
	put_be32(_APacketCnt);					   //uint32 sample_count
	if (_APacketCnt > 0) _pSinkIdx->Seek(_astsz_fpos, SEEK_SET);
	for (i=0; i<_APacketCnt/_IDX_SIZE; i++)	  //uint32 entry_size[sample_count] (if sample_size==0)
	{
		_pSinkIdx->Read((AM_U8 *)_astsz, _IDX_SIZE*sizeof(AM_UINT));
		put_buffer((AM_U8 *)_astsz, _IDX_SIZE*sizeof(AM_UINT));
	}
	if ((j=_APacketCnt%_IDX_SIZE) > 0)
	{
		_pSinkIdx->Read((AM_U8 *)_astsz, j*sizeof(AM_UINT));
		put_buffer((AM_U8 *)_astsz, j*sizeof(AM_UINT));
	}

	//ChunkOffsetBox
	if (_largeFile == true)
	{
		put_be32(AudioChunkOffsetBox_SIZE64);		//uint32 size
		put_be32(FOURCC('c', 'o', '6', '4'));		//'co64'
		put_byte(0);								 //uint8 version
		put_be24(0);								 //bits24 flags
		put_be32(_APacketCnt);					   //uint32 entry_count
		if (_APacketCnt > 0) _pSinkIdx->Seek(_astco_fpos, SEEK_SET);
		for (i=0; i<_APacketCnt/_IDX_SIZE; i++)	  //uint64 chunk_offset[entry_count]
		{
			_pSinkIdx->Read((AM_U8 *)_astco, _IDX_SIZE*sizeof(AM_U64));
			put_buffer((AM_U8 *)_astco, _IDX_SIZE*sizeof(AM_U64));
		}
		if ((j=_APacketCnt%_IDX_SIZE) > 0)
		{
			_pSinkIdx->Read((AM_U8 *)_astco, j*sizeof(AM_U64));
			put_buffer((AM_U8 *)_astco, j*sizeof(AM_U64));
		}
	}
	else
	{
		put_be32(AudioChunkOffsetBox_SIZE);		  //uint32 size
		put_be32(FOURCC('s', 't', 'c', 'o'));		//'stco'
		put_byte(0);								 //uint8 version
		put_be24(0);								 //bits24 flags
		put_be32(_APacketCnt);					   //uint32 entry_count
		if (_APacketCnt > 0) _pSinkIdx->Seek(_astco_fpos, SEEK_SET);
		for (i=0; i<_APacketCnt/_IDX_SIZE; i++)	  //uint32 chunk_offset[entry_count]
		{
			_pSinkIdx->Read((AM_U8 *)_astco, _IDX_SIZE*sizeof(AM_UINT));
			put_buffer((AM_U8 *)_astco, _IDX_SIZE*sizeof(AM_UINT));
		}
		if ((j=_APacketCnt%_IDX_SIZE) > 0)
		{
			_pSinkIdx->Read((AM_U8 *)_astco, j*sizeof(AM_UINT));
			put_buffer((AM_U8 *)_astco, j*sizeof(AM_UINT));
		}
	}
}


//--------------------------------------------------

#define VIDEO_HANDLER_NAME			 ((AM_U8 *)"Ambarella AVC")
#define VIDEO_HANDLER_NAME_LEN		 14 //strlen(VIDEO_HANDLER_NAME)+1
#define MediaHeaderBox_SIZE			32
#define VideoHandlerReferenceBox_SIZE  (32+VIDEO_HANDLER_NAME_LEN) // 46
#define VideoMediaBox_SIZE64		   (8+MediaHeaderBox_SIZE+\
										VideoHandlerReferenceBox_SIZE+\
										VideoMediaInformationBox_SIZE64) // 487+_spsSize+_ppsSize+V*21+IDR*4
#define VideoMediaBox_MAXSIZE64		(8+MediaHeaderBox_SIZE+\
										VideoHandlerReferenceBox_SIZE+\
										VideoMediaInformationBox_MAXSIZE64) // 487+_spsSize+_ppsSize+V*21+IDR*4
#define VideoMediaBox_SIZE			 (8+MediaHeaderBox_SIZE+\
										VideoHandlerReferenceBox_SIZE+\
										VideoMediaInformationBox_SIZE) // 487+_spsSize+_ppsSize+V*17+IDR*4
#define VideoMediaBox_MAXSIZE		  (8+MediaHeaderBox_SIZE+\
										VideoHandlerReferenceBox_SIZE+\
										VideoMediaInformationBox_MAXSIZE) // 487+_spsSize+_ppsSize+V*17+IDR*4

void CamIsoMuxer::put_VideoMediaBox(AM_UINT Duration)
{
	//MediaBox
	if (_largeFile == true)
	{
	put_be32(VideoMediaBox_SIZE64);			 //uint32 size
	}
	else
	{
	put_be32(VideoMediaBox_SIZE);			   //uint32 size
	}
	put_be32(FOURCC('m', 'd', 'i', 'a'));	   //'mdia'

	//MediaHeaderBox
	put_be32(MediaHeaderBox_SIZE);			  //uint32 size
	put_be32(FOURCC('m', 'd', 'h', 'd'));	   //'mdhd'
	put_byte(0);								//uint8 version
	put_be24(0);								//bits24 flags
	put_be32(_CreationTime);					//uint32 creation_time [version==0] uint64 creation_time [version==1]
	put_be32(_ModificationTime);				//uint32 modification_time [version==0] uint64 modification_time [version==1]
	put_be32(_Config._clk);					 //uint32 timescale
	put_be32(Duration);						 //uint32 duration [version==0] uint64 duration [version==1]
	put_be16(0);								//bits5 language[3]  //ISO-639-2/T language code
	put_be16(0);								//uint16 pre_defined

	//HandlerReferenceBox
	put_be32(VideoHandlerReferenceBox_SIZE);	//uint32 size
	put_be32(FOURCC('h', 'd', 'l', 'r'));	   //'hdlr'
	put_byte(0);								//uint8 version
	put_be24(0);								//bits24 flags
	put_be32(0);								//uint32 pre_defined
	put_be32(FOURCC('v', 'i', 'd', 'e'));	   //'vide':video track
	put_be32(0);								//uint32 reserved[3]
	put_be32(0);
	put_be32(0);
	put_byte(VIDEO_HANDLER_NAME_LEN);		   //char name[], name[0] is actual length
	put_buffer(VIDEO_HANDLER_NAME, VIDEO_HANDLER_NAME_LEN-1);

	put_VideoMediaInformationBox();
}

#define AUDIO_HANDLER_NAME			 ((AM_U8 *)"Ambarella AAC")
#define AUDIO_HANDLER_NAME_LEN		 14 //strlen(AUDIO_HANDLER_NAME)+1
#define AudioHandlerReferenceBox_SIZE  (32+AUDIO_HANDLER_NAME_LEN) // 46
#define AudioMediaBox_SIZE64		   (8+MediaHeaderBox_SIZE+\
										AudioHandlerReferenceBox_SIZE+\
										AudioMediaInformationBox_SIZE64) // 344+A*12
#define AudioMediaBox_MAXSIZE64		(8+MediaHeaderBox_SIZE+\
										AudioHandlerReferenceBox_SIZE+\
										AudioMediaInformationBox_MAXSIZE64) // 344+A*12
#define AudioMediaBox_SIZE			 (8+MediaHeaderBox_SIZE+\
										AudioHandlerReferenceBox_SIZE+\
										AudioMediaInformationBox_SIZE) // 344+A*8
#define AudioMediaBox_MAXSIZE		  (8+MediaHeaderBox_SIZE+\
										AudioHandlerReferenceBox_SIZE+\
										AudioMediaInformationBox_MAXSIZE) // 344+A*8

void CamIsoMuxer::put_AudioMediaBox()
{
	//MediaBox
	if (_largeFile == true)
	{
	put_be32(AudioMediaBox_SIZE64);			 //uint32 size
	}
	else
	{
	put_be32(AudioMediaBox_SIZE);			   //uint32 size
	}
	put_be32(FOURCC('m', 'd', 'i', 'a'));	   //'mdia'

	//MediaHeaderBox
	put_be32(MediaHeaderBox_SIZE);			  //uint32 size
	put_be32(FOURCC('m', 'd', 'h', 'd'));	   //'mdhd'
	put_byte(0);								//uint8 version
	put_be24(0);								//bits24 flags
	put_be32(_CreationTime);					//uint32 creation_time [version==0] uint64 creation_time [version==1]
	put_be32(_ModificationTime);				//uint32 modification_time [version==0] uint64 modification_time [version==1]
	put_be32(_Config._SampleFreq);			  //uint32 timescale
	put_be32(1024*_APacketCnt);				 //uint32 duration [version==0] uint64 duration [version==1]
	put_be16(0);								//bits5 language[3]  //ISO-639-2/T language code
	put_be16(0);								//uint16 pre_defined

	//HandlerReferenceBox
	put_be32(AudioHandlerReferenceBox_SIZE);	//uint32 size
	put_be32(FOURCC('h', 'd', 'l', 'r'));	   //'hdlr'
	put_byte(0);								//uint8 version
	put_be24(0);								//bits24 flags
	put_be32(0);								//uint32 pre_defined
	put_be32(FOURCC('s', 'o', 'u', 'n'));	   //'soun':audio track
	put_be32(0);								//uint32 reserved[3]
	put_be32(0);
	put_be32(0);
	put_byte(AUDIO_HANDLER_NAME_LEN);		   //char name[], name[0] is actual length
	put_buffer(AUDIO_HANDLER_NAME, AUDIO_HANDLER_NAME_LEN-1);

	put_AudioMediaInformationBox();
}


//--------------------------------------------------

#define TrackHeaderBox_SIZE	 92
#define clefBox_SIZE			20
#define profBox_SIZE			20
#define enofBox_SIZE			20
#define taptBox_SIZE			(8+clefBox_SIZE+profBox_SIZE+enofBox_SIZE) // 68
#define VideoTrackBox_SIZE64	(8+TrackHeaderBox_SIZE+taptBox_SIZE+VideoMediaBox_SIZE64) // 655+_spsSize+_ppsSize+V*21+IDR*4
#define VideoTrackBox_MAXSIZE64 (8+TrackHeaderBox_SIZE+taptBox_SIZE+VideoMediaBox_MAXSIZE64) // 655+_spsSize+_ppsSize+V*21+IDR*4
#define VideoTrackBox_SIZE	  (8+TrackHeaderBox_SIZE+taptBox_SIZE+VideoMediaBox_SIZE) // 655+_spsSize+_ppsSize+V*17+IDR*4
#define VideoTrackBox_MAXSIZE   (8+TrackHeaderBox_SIZE+taptBox_SIZE+VideoMediaBox_MAXSIZE) // 655+_spsSize+_ppsSize+V*17+IDR*4
void CamIsoMuxer::put_videoTrackBox(AM_UINT TrackId, AM_UINT Duration)
{
	//TrackBox
	if (_largeFile == true)
	{
	put_be32(VideoTrackBox_SIZE64);		//uint32 size
	}
	else
	{
	put_be32(VideoTrackBox_SIZE);		  //uint32 size
	}
	put_be32(FOURCC('t', 'r', 'a', 'k'));  //'trak'

	//TrackHeaderBox
	put_be32(TrackHeaderBox_SIZE);		 //uint32 size
	put_be32(FOURCC('t', 'k', 'h', 'd'));  //'tkhd'
	put_byte(0);						   //uint8 version
	//0x01:track_enabled, 0x02:track_in_movie, 0x04:track_in_preview
	put_be24(0x07);						//bits24 flags
	put_be32(_CreationTime);			   //uint32 creation_time [version==0] uint64 creation_time [version==1]
	put_be32(_ModificationTime);		   //uint32 modification_time [version==0] uint64 modification_time [version==1]
	put_be32(TrackId);					 //uint32 track_ID
	put_be32(0);						   //uint32 reserved
	put_be32(Duration);					//uint32 duration [version==0] uint64 duration [version==1]
	put_be32(0);						   //uint32 reserved[2]
	put_be32(0);
	put_be16(0);						   //int16 layer
	put_be16(0);						   //int16 alternate_group
	put_be16(0x0000);					  //int16 volume
	put_be16(0);						   //uint16 reserved
	put_be32(0x00010000);				  //int32 matrix[9]
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0x00010000);
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0x40000000);
	put_be32(_Config._Width<<16);		  //uint32 width  //16.16 fixed-point
	put_be32(_Config._Height<<16);		 //uint32 height //16.16 fixed-point

	//taptBox
	put_be32(taptBox_SIZE);
	put_be32(FOURCC('t', 'a', 'p', 't'));
	//clefBox
	put_be32(clefBox_SIZE);
	put_be32(FOURCC('c', 'l', 'e', 'f'));
	put_be32(0);
	put_be16(_ScaledWidth);
	put_be16(0);
	put_be16(_ScaledHeight);
	put_be16(0);
	//profBox
	put_be32(profBox_SIZE);
	put_be32(FOURCC('p', 'r', 'o', 'f'));
	put_be32(0);
	put_be16(_ScaledWidth);
	put_be16(0);
	put_be16(_ScaledHeight);
	put_be16(0);
	//enofBox
	put_be32(enofBox_SIZE);
	put_be32(FOURCC('e', 'n', 'o', 'f'));
	put_be32(0);
	put_be16(_Config._Width);
	put_be16(0);
	put_be16(_Config._Height);
	put_be16(0);

	put_VideoMediaBox(Duration);
}

#define AudioTrackBox_SIZE64	 (8+TrackHeaderBox_SIZE+AudioMediaBox_SIZE64) // 444+A*12
#define AudioTrackBox_MAXSIZE64  (8+TrackHeaderBox_SIZE+AudioMediaBox_MAXSIZE64) // 444+A*12
#define AudioTrackBox_SIZE	   (8+TrackHeaderBox_SIZE+AudioMediaBox_SIZE) // 444+A*8
#define AudioTrackBox_MAXSIZE	(8+TrackHeaderBox_SIZE+AudioMediaBox_MAXSIZE) // 444+A*8
void CamIsoMuxer::put_AudioTrackBox(AM_UINT TrackId, AM_UINT Duration)
{
	//TrackBox
	if (_largeFile == true)
	{
	put_be32(AudioTrackBox_SIZE64);		//uint32 size
	}
	else
	{
	put_be32(AudioTrackBox_SIZE);		  //uint32 size
	}
	put_be32(FOURCC('t', 'r', 'a', 'k'));  //'trak'

	//TrackHeaderBox
	put_be32(TrackHeaderBox_SIZE);		 //uint32 size
	put_be32(FOURCC('t', 'k', 'h', 'd'));  //'tkhd'
	put_byte(0);						   //uint8 version
	//0x01:track_enabled, 0x02:track_in_movie, 0x04:track_in_preview
	put_be24(0x07);						//bits24 flags
	put_be32(_CreationTime);			   //uint32 creation_time [version==0] uint64 creation_time [version==1]
	put_be32(_ModificationTime);		   //uint32 modification_time [version==0] uint64 modification_time [version==1]
	put_be32(TrackId);					 //uint32 track_ID
	put_be32(0);						   //uint32 reserved
	put_be32(Duration);					//uint32 duration [version==0] uint64 duration [version==1]
	put_be32(0);						   //uint32 reserved[2]
	put_be32(0);
	put_be16(0);						   //int16 layer
	put_be16(0);						   //int16 alternate_group
	put_be16(0x0100);					  //int16 volume
	put_be16(0);						   //uint16 reserved
	put_be32(0x00010000);				  //int32 matrix[9]
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0x00010000);
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0x40000000);
	put_be32(0);						   //uint32 width  //16.16 fixed-point
	put_be32(0);						   //uint32 height //16.16 fixed-point

	put_AudioMediaBox();
}


//--------------------------------------------------

#define FileTypeBox_SIZE  32
void CamIsoMuxer::put_FileTypeBox()
{
	if (_Config._FileType == 2)
	{
		put_be32(FileTypeBox_SIZE);			//uint32 size
		put_be32(FOURCC('f', 't', 'y', 'p'));  //'ftyp'
		put_be32(FOURCC('f', '4', 'v', ' '));  //uint32 major_brand
		put_be32(0);						   //uint32 minor_version
		put_be32(FOURCC('i', 's', 'o', 'm'));  //uint32 compatible_brands[]
		put_be32(FOURCC('m', 'p', '4', '2'));
		put_be32(FOURCC('m', '4', 'v', ' '));
		put_be32(0);
	}
	else //default to mp4 file
	{
		put_be32(FileTypeBox_SIZE);			//uint32 size
		put_be32(FOURCC('f', 't', 'y', 'p'));  //'ftyp'
		put_be32(FOURCC('a', 'v', 'c', '1'));  //uint32 major_brand
		put_be32(0);						   //uint32 minor_version
		put_be32(FOURCC('a', 'v', 'c', '1'));  //uint32 compatible_brands[]
		put_be32(FOURCC('i', 's', 'o', 'm'));
		put_be32(0);
		put_be32(0);
	}
}

#define MovieHeaderBox_SIZE  108
#define AMBABox_SIZE		 40
#define UserDataBox_SIZE	 (8+AMBABox_SIZE) // 48
#define MovieBox_SIZE64	  (8+MovieHeaderBox_SIZE+\
							  UserDataBox_SIZE+\
							  VideoTrackBox_SIZE64+\
							  AudioTrackBox_SIZE64) // 1263+_spsSize+_ppsSize+V*21+IDR*4+A*12
#define MovieBox_MAXSIZE64   (8+MovieHeaderBox_SIZE+\
							  UserDataBox_SIZE+\
							  VideoTrackBox_MAXSIZE64+\
							  AudioTrackBox_MAXSIZE64) // 1263+_spsSize+_ppsSize+V*21+IDR*4+A*12
#define MovieBox_SIZE		(8+MovieHeaderBox_SIZE+\
							  UserDataBox_SIZE+\
							  VideoTrackBox_SIZE)
							  //+\
							  //AudioTrackBox_SIZE) // 1263+_spsSize+_ppsSize+V*17+IDR*4+A*8
#define MovieBox_MAXSIZE	 (8+MovieHeaderBox_SIZE+\
							  UserDataBox_SIZE+\
							  VideoTrackBox_MAXSIZE)
							  //+\
							  //AudioTrackBox_MAXSIZE) // 1263+_spsSize+_ppsSize+V*17+IDR*4+A*8
void CamIsoMuxer::put_MovieBox()
{
	AM_UINT i, vDuration, aDuration, Duration;

	//MovieBox
	if (_largeFile == true)
	{
	put_be32(MovieBox_SIZE64);			 //uint32 size
	}
	else
	{
	put_be32(MovieBox_SIZE);			   //uint32 size
	}
	put_be32(FOURCC('m', 'o', 'o', 'v'));  //'moov'

	//MovieHeaderBox
	vDuration = (AM_UINT)(1.0*_frame_field_set*_Config._clk*_VPacketCnt*_Config._scale/_Config._rate);
	aDuration = (AM_UINT)(1.0*_Config._clk*_APacketCnt*1024/_Config._SampleFreq);
	Duration = (vDuration>aDuration)?vDuration:aDuration;
	put_be32(MovieHeaderBox_SIZE);		 //uint32 size
	put_be32(FOURCC('m', 'v', 'h', 'd'));  //'mvhd'
	put_byte(0);						   //uint8 version
	put_be24(0);						   //bits24 flags
	put_be32(_CreationTime);			   //uint32 creation_time [version==0] uint64 creation_time [version==1]
	put_be32(_ModificationTime);		   //uint32 modification_time [version==0] uint64 modification_time [version==1]
	put_be32(_Config._clk);				//uint32 timescale
	put_be32(Duration);					//uint32 duration [version==0] uint64 duration [version==1]
	put_be32(0x00010000);				  //int32 rate
	put_be16(0x0100);					  //int16 volume
	put_be16(0);						   //bits16 reserved
	put_be32(0);						   //uint32 reserved[2]
	put_be32(0);
	put_be32(0x00010000);				  //int32 matrix[9]
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0x00010000);
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0x40000000);
	put_be32(0);						   //bits32 pre_defined[6]
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(0);
	put_be32(_InputVCnt+_InputACnt+1);	 //uint32 next_track_ID

	//UserDataBox
	put_be32(UserDataBox_SIZE);			//uint32 size
	put_be32(FOURCC('u', 'd', 't', 'a'));  //'udta'
	//AMBABox
	put_be32(AMBABox_SIZE);						  //uint32 size
	put_be32(FOURCC('A', 'M', 'B', 'A'));  //'AMBA'
	put_be16(_Config._ar_x);			   //uint16 ar_x
	put_be16(_Config._ar_y);			   //uint16 ar_y
	put_byte(_Config._mode);			   //uint8 mode
	put_byte(_Config._M);				  //uint8 M
	put_byte(_Config._N);				  //uint8 N
	put_byte(_Config._advanced);		   //uint8 advanced
	put_be32(_Config._idr_interval);	   //uint32 idr_interval
	put_be32(_Config._scale);			  //uint32 scale
	put_be32(_Config._rate);			   //uint32 rate
	put_be32(_Config._brate);			  //uint32 brate
	put_be32(_Config._brate_min);		  //uint32 brate_min
	put_be32(0);

	//for (i=1; i<=_InputVCnt; i++)
	//	put_videoTrackBox(i);
	i=1; put_videoTrackBox(i, vDuration);
	//for ( ;i<=_InputVCnt+_InputACnt; i++)
	//put_AudioTrackBox(i, aDuration);
	//	i=_InputVCnt+1; put_AudioTrackBox(i, aDuration);
}


//--------------------------------------------------

#define EmptyFreeSpaceBox_SIZE (16+_SPS_SIZE+_PPS_SIZE) // 144  16=8 bytes header + 8 bytes data
void CamIsoMuxer::put_FreeSpaceBox(AM_UINT FreeSpaceSize)
{
	AM_U8 zero[64];
	AM_UINT i;
	assert(FreeSpaceSize >= 8);
	memset(zero, 0, sizeof(zero));
	put_be32(FreeSpaceSize);
	put_be32(FOURCC('f', 'r', 'e', 'e'));
	FreeSpaceSize -= 8;
	for (i=0; i<FreeSpaceSize/sizeof(zero); i++)
	{
		put_buffer(zero, sizeof(zero));
	}
	FreeSpaceSize %= sizeof(zero);
	put_buffer(zero, FreeSpaceSize);
}

#define EmptyMediaDataBox_SIZE64 16			  //64-bit (long) format atom
#define FileHeader_SIZE64		(FileTypeBox_SIZE+MovieBox_SIZE64+EmptyFreeSpaceBox_SIZE) // 1439+_spsSize+_ppsSize+V*21+IDR*4+A*12
#define FileHeader_MAXSIZE64	 (FileTypeBox_SIZE+MovieBox_MAXSIZE64+EmptyFreeSpaceBox_SIZE) // 1439+_spsSize+_ppsSize+V*21+IDR*4+A*12
#define EmptyMediaDataBox_SIZE   8
#define FileHeader_SIZE		  (FileTypeBox_SIZE+MovieBox_SIZE+EmptyFreeSpaceBox_SIZE) // 1439+_spsSize+_ppsSize+V*17+IDR*4+A*8
#define FileHeader_MAXSIZE	   (FileTypeBox_SIZE+MovieBox_MAXSIZE+EmptyFreeSpaceBox_SIZE) // 1439+_spsSize+_ppsSize+V*17+IDR*4+A*8
void CamIsoMuxer::put_IsoFile(AM_UINT FreeSpaceSize)
{
	_pSink->Seek(0, SEEK_SET);
	_curPos = 0;
	//FileTypeBox
	put_FileTypeBox();
	//MovieBox
	put_MovieBox();
	//FreeSpaceBox
	put_FreeSpaceBox(FreeSpaceSize);
	//MediaDataBox is generated during RUNNING
	AM_U64 size = _pSink->Seek(0, SEEK_END);
	size -= _curPos;
	_pSink->Seek(_curPos, SEEK_SET);
	if (_largeFile == true)
	{
		put_be32(1);							   //uint32 size
		put_be32(FOURCC('m', 'd', 'a', 't'));	  //'mdat'
		put_be64(size);							//uint64 totalsize [exist if size==1]
	}
	else
	{
		put_be32((AM_U32)size);							//uint32 size
		put_be32(FOURCC('m', 'd', 'a', 't'));	  //'mdat'
	}
}


//--------------------------------------------------

//copy from DV
void CamIsoMuxer::InitSdtp()
{
	AM_UINT lv0=(AM_UINT)_Config._M, lv1, lv2, lv3, lv4, lv5, lv6=1;
	AM_UINT EntryNo;
	AM_UINT ctr_L1, ctr_L2, ctr_L3, ctr_L4, ctr_L5, ctr_L6;
	AM_U8 last_lv;

	if (_Config._advanced)
	{
		lv1 = (lv0/2==0)?1:lv0/2;
		lv2 = (lv0/4==0)?1:lv0/4;
		lv3 = (lv0/8==0)?1:lv0/8;
		lv4 = (lv0/16==0)?1:lv0/16;
		lv5 = (lv0/32==0)?1:lv0/32;
		if (lv0/2 == 1)
		{
			last_lv = 1;
		}
		else if (lv0/4 == 1)
		{
			last_lv = 2;
		}
		else if (lv0/8 == 1)
		{
			last_lv = 3;
		}
		else if (lv0/16 == 1)
		{
			last_lv = 4;
		}
		else
		{
			last_lv = 5;
		}
		EntryNo = 0;
		_lv_idc[EntryNo] = last_lv;
		EntryNo++;
		_lv_idc[EntryNo] = 0;
		EntryNo++;
		for (ctr_L1=lv1; ctr_L1<=lv0; ctr_L1+=lv1)
		{
			if (ctr_L1 != lv0)
			{
				_lv_idc[EntryNo] = 1;
				EntryNo++;
			}
			for (ctr_L2=lv2; ctr_L2<=lv1; ctr_L2+=lv2)
			{
				if (ctr_L2 != lv1)
				{
					_lv_idc[EntryNo] = 2;
					EntryNo++;
				}
				for (ctr_L3=lv3; ctr_L3<=lv2; ctr_L3+=lv3)
				{
					if (ctr_L3 != lv2)
					{
						_lv_idc[EntryNo] = 3;
						EntryNo++;
					}
					for (ctr_L4=lv4; ctr_L4<=lv3; ctr_L4+=lv4)
					{
						if (ctr_L4 != lv3)
						{
							_lv_idc[EntryNo] = 4;
							EntryNo++;
						}
						for (ctr_L5=lv5; ctr_L5<=lv4; ctr_L5+=lv5)
						{
							if (ctr_L5 != lv4)
							{
								_lv_idc[EntryNo] = 5;
								EntryNo++;
							}
							for (ctr_L6=lv6; ctr_L6<=lv5; ctr_L6+=lv6)
							{
								if (ctr_L6 != lv5)
								{
									_lv_idc[EntryNo] = 6;
									EntryNo++;
								}
							}
						}
					}
				}
			}
		}
	}
	else
	{
		EntryNo = 0;
		_lv_idc[EntryNo] = 1;
		EntryNo++;
		_lv_idc[EntryNo] = 0;
		EntryNo++;
		for (ctr_L1=1; ctr_L1<lv0; ctr_L1++)
		{
			_lv_idc[EntryNo] = 1;
			EntryNo++;
		}
	}
}

//Generate AAC decoder information
AM_UINT CamIsoMuxer::GetAacInfo(AM_UINT samplefreq, AM_UINT channel)
{
	//bits5: Audio Object Type  AAC Main=1, AAC LC (low complexity)=2, AAC SSR=3, AAC LTP=4
	//bits4: Sample Frequency Index
	//	 bits24: Sampling Frequency [if Sample Frequency Index == 0xf]
	//bits4: Channel Configuration
	//bits1: FrameLength 0:1024, 1:960
	//bits1: DependsOnCoreCoder
	//	 bits14: CoreCoderDelay [if DependsOnCoreCoder==1]
	//bits1: ExtensionFlag
	AM_UINT info = 0x1000;//AAC LC
	AM_UINT sfi = 0x0f;
	//ISO/IEC 14496-3 Table 1.16 Sampling Frequency Index
	switch (samplefreq)
	{
	case 96000:
		sfi = 0;
		break;
	case 88200:
		sfi = 1;
		break;
	case 64000:
		sfi = 2;
		break;
	case 48000:
		sfi = 3;
		break;
	case 44100:
		sfi = 4;
		break;
	case 32000:
		sfi = 5;
		break;
	case 24000:
		sfi = 6;
		break;
	case 22050:
		sfi = 7;
		break;
	case 16000:
		sfi = 8;
		break;
	case 12000:
		sfi = 9;		
		break;
	case 11025:
		sfi = 10;
		break;
	case 8000:
		sfi = 11;
		break;
	}
	info |= (sfi << 7);
	info |= (channel << 3);
	return info;
}


//AM_U8 * filename
AM_ERR CamIsoMuxer::Init(int videoIndex)
{
	HRESULT hr;
	IAmbaRecordControl::FORMAT *pFormat;
	hr = G_pRecordControl->GetFormat(&pFormat);

	if ( (videoIndex == 0 && pFormat->main.enc_type != IAmbaRecordControl::ENC_H264 ) 
		|| ( videoIndex == 1 && pFormat->secondary.enc_type !=  IAmbaRecordControl::ENC_H264 ) )
	{
		m_sp264 = false;
	}

	
	if( G_pMuxerParam->MuxerType == 3 || m_sp264 == false)
	{
		if (videoIndex == 0)
		{
			::memcpy(_Config._FileName,G_pMuxerParam->MuxerFilename, strlen(G_pMuxerParam->MuxerFilename)+1);
		}
		else if(videoIndex == 1)
		{
			char *filename = strrchr(G_pMuxerParam->MuxerFilename, '.');
			int file_len;

			if(filename != NULL)
			{
				file_len = filename - G_pMuxerParam->MuxerFilename;
			}
			else
			{
				file_len = strlen(G_pMuxerParam->MuxerFilename);
			}
			::memcpy(_Config._FileName,	G_pMuxerParam->MuxerFilename,file_len);
			::memcpy(_Config._FileName + file_len,"_second", strlen("_second"));
			file_len += strlen("_second");
			if (pFormat->secondary.enc_type !=  IAmbaRecordControl::ENC_MJPEG)
			{
				::memcpy(_Config._FileName+file_len, filename, strlen(filename)+1);
			}else
			{
				::memcpy(_Config._FileName+file_len, ".mjpeg", strlen(".mjpeg")+1);
			}
		}
		// G_pMuxerParam->MaxFileSize = 16;
		_Config._FileSize = G_pMuxerParam->MaxFileSize*1024*1024;//2*1024*1024*1024;
		_fileCount = 0;
		if ((_pSink = new CamFileSink()) == NULL) return ME_NO_MEMORY;
		return ME_OK;
	}
	::memset(&_Config, 0, sizeof(_Config));
	 

	IAmbaRecordControl::PARAM *pParam;
	hr = G_pRecordControl->GetParam(&pParam);
		

	if (videoIndex == 0)
	{
		_Config._M = pParam->main.M;
		_Config._N = pParam->main.N;
		_Config._advanced = pParam->main.gop_model;
		_Config._idr_interval = pParam->main.idr_interval;
		_Config._ar_x = pParam->main.ar_x;
		_Config._ar_y = pParam->main.ar_y;
		_Config._mode = pParam->main.frame_mode;
		//_Config._scale = pParam->main.rate;
		//_Config._rate = pParam->main.scale;

		//_Config._brate = _Config._brate_min = 4000000;//assmue CBR
		if (pParam->main.bitrate_control == IAmbaRecordControl::CBR)
		{
			_Config._brate = _Config._brate_min = pParam->main.average_bitrate;
		}
		else
		{
			_Config._brate = (AM_UINT)( 1.0 * pParam->main.average_bitrate * pParam->main.max_vbr_rate_factor / 100);
			if (pParam->main.min_vbr_rate_factor < 40) pParam->main.min_vbr_rate_factor = 40;
			 _Config._brate_min = (AM_UINT)(1.0 * pParam->main.average_bitrate * pParam->main.min_vbr_rate_factor / 100);
			 assert(_Config._brate_min != 0);
		}
		_Config._Width = pFormat->main.width;
		_Config._Height = pFormat->main.height;

		::memcpy(_Config._FileName,G_pMuxerParam->MuxerFilename, strlen(G_pMuxerParam->MuxerFilename)+1);

	}
	else if(videoIndex == 1 )
	{
		_Config._M = pParam->secondary.M;
		_Config._N = pParam->secondary.N;
		_Config._advanced = pParam->secondary.gop_model;
		_Config._idr_interval = pParam->secondary.idr_interval;
		_Config._ar_x = pParam->secondary.ar_x;
		_Config._ar_y = pParam->secondary.ar_y;
		_Config._mode = pParam->secondary.frame_mode;
		//_Config._scale = pParam->secondary.rate;
		//_Config._rate = pParam->secondary.scale;
		 //_Config._brate = _Config._brate_min = 4000000;//assmue CBR
		if (pParam->secondary.bitrate_control == IAmbaRecordControl::CBR)
		{
			_Config._brate = _Config._brate_min = pParam->secondary.average_bitrate;
		}
		else
		{
			_Config._brate = (AM_UINT)( 1.0 * pParam->secondary.average_bitrate * pParam->secondary.max_vbr_rate_factor / 100);
			if (pParam->secondary.min_vbr_rate_factor < 40) pParam->secondary.min_vbr_rate_factor = 40;
			 _Config._brate_min = (AM_UINT)(1.0 * pParam->secondary.average_bitrate * pParam->secondary.min_vbr_rate_factor / 100);
			 assert(_Config._brate_min != 0);
		}
		_Config._Width = pFormat->secondary.width;
		_Config._Height = pFormat->secondary.height;

		char *filename = strrchr(G_pMuxerParam->MuxerFilename, '.');
		int file_len;
		if(filename != NULL)
		{
			file_len = filename - G_pMuxerParam->MuxerFilename;
		}
		else
		{
			file_len = strlen(G_pMuxerParam->MuxerFilename);
		}
		::memcpy(_Config._FileName,	G_pMuxerParam->MuxerFilename,file_len);
		::memcpy(_Config._FileName + file_len,"_second", strlen("_second"));
		file_len += strlen("_second");
		::memcpy(_Config._FileName+file_len, filename, strlen(filename)+1);
	}
	

	AM_U8 tmp = (AM_U8)pFormat->vinFrate;
	//NOTE: exchange scale and rate
	tmp &= 0x7f; //remove MSB interlaced flag
	if (tmp < 2) {
	_Config._scale = 6006;
	} else if (tmp < 4) {
	_Config._scale = 3003;
	} else {
	_Config._scale = 180000 / tmp;
	}

	_Config._rate = 180000;

	_Config._new_gop = 1;

	_Config._SampleFreq = 48000;
	_Config._Channels = 2;
	_Config._SampleSize = 16;
	_Config._clk = 90000;
	_Config._FileType = G_pMuxerParam->MuxerType;
	_Config._FileSize = G_pMuxerParam->MaxFileSize*1024*1024;//2*1024*1024*1024;
	_PixelAspectRatioWidth = 0;
	_PixelAspectRatioHeight = 0;
	InitSdtp();
  
	if ((_pSink = new CamFileSink()) == NULL) return ME_NO_MEMORY;
	if ((_pSinkIdx=new CamFileSink()) == NULL)
	{
		delete _pSink; _pSink = NULL;
		return ME_NO_MEMORY;
	}

	_largeFile = false;
	_fileCount = 0;
	//others init during run

	return ME_OK;

}



AM_ERR CamIsoMuxer::Run()
{
	if ( G_pMuxerParam->MuxerType == 3 || m_sp264 == false)
	{
		if (_pSink->Open( _Config._FileName, O_WRONLY|O_CREAT|O_TRUNC|_O_BINARY , 0666) == -1)
		{
			return ME_IO_ERROR;
		}
		_isStopped = false;
		return ME_OK;
	}
	//time
	time_t t = ::time(NULL);//seconds since 1970-01-01 00:00:00 local
	printf("local seconds %u\n", (AM_UINT)t);
	struct tm *utc = ::gmtime(&t);//UTC time yyyy-mm-dd HH:MM:SS
	printf("UTC %04d-%02d-%02d %02d:%02d:%02d\n", 1900+utc->tm_year, utc->tm_mon+1, utc->tm_mday, utc->tm_hour, utc->tm_min, utc->tm_sec);
	t = ::mktime(utc);//seconds since 1970-01-01 00:00:00 UTC
	printf("UTC seconds %u\n", (AM_UINT)t);
	//1904-01-01 00:00:00 UTC -> 1970-01-01 00:00:00 UTC
	//66 years plus 17 days of the 17 leap years [1904, 1908, ..., 1968]
	_CreationTime = _ModificationTime = t+66*365*24*60*60+17*24*60*60;

	//_Config should be ready

	::memset(_sps, 0, sizeof(_sps));
	_spsSize = _spsPad = 0;
	::memset(_pps, 0, sizeof(_pps));
	_ppsSize = _ppsPad = 0;

	_VPacketCnt = _APacketCnt = _IdrPacketCnt = 0;
	 _v_delta = (AM_UINT)(1.0*_Config._scale*CLOCK/_Config._rate);

	//one audio frame have 1024 samples, CLOCK/samples==count of time units per sample
	_a_delta = (AM_UINT)(1.0*1024*CLOCK/_Config._SampleFreq);
	_vdts = _adts = _pts_off = 0;
	_FrameDropped = 0;
	_v_dropped = _a_dropped = 0;

	//file size related limitation settings
	AM_UINT vr = (_Config._rate + (_Config._scale>>1)) / _Config._scale;  //frame rate: fps
	printf("%d video frame per second\n", vr);
	AM_UINT ar = (_Config._SampleFreq + 512) / 1024;		 //K audio samples per second
	printf("%dK audio samples per second\n", ar);
	//so much seconds can be stored in freespace
	AM_UINT d = _Config._FileSize/(_Config._brate_min>>3);
	d += (d>>3); // tolerance
	printf("can record appr. %d seconds\n", d);
	_MaxVPacketCnt = vr * d;  //one frame video per packet
	_MaxAPacketCnt = ar * d;  //1024 samples audio per packet
	//make max an even number so tmp index data stored in file will always align to 8 bytes
	//then Recover() will easy to find them
	if (_MaxVPacketCnt&1) _MaxVPacketCnt++;
	if (_MaxAPacketCnt&1) _MaxAPacketCnt++;
	_MaxAPacketCnt = 0;
	_MaxIdrPacketCnt = 1 + _MaxVPacketCnt/(_Config._idr_interval*_Config._N);
	AM_UINT er = (ar*2 + vr*3);
	printf("max video packet %d, max audio packet %d, %d index per second\n", _MaxVPacketCnt, _MaxAPacketCnt, er);
	//file header size (the part before mdat box)
	//AM_UINT tmpsize = (_MaxVPacketCnt*17 + _MaxVPacketCnt*4/(_Config._idr_interval*_Config._N) + _MaxAPacketCnt*8 + FileHeader_SIZE + 1023)/1024;
	//AM_UINT tmpsize = (_MaxVPacketCnt*21 + _MaxVPacketCnt*4/(_Config._idr_interval*_Config._N) + _MaxAPacketCnt*12 + FileHeader_SIZE64 + 1023)/1024;
	AM_UINT tmpsize;
	if (_largeFile == true)
	{
		tmpsize = (FileHeader_MAXSIZE64+1023)/1024;
		//add some tolerance
		tmpsize += 2; tmpsize *= 1024;
		printf("file size=%d, min header size=%d, max header size=%d\n", _Config._FileSize, FileHeader_SIZE64, tmpsize);
		_FreeSpaceSize = tmpsize-FileTypeBox_SIZE;
		_MdatFreeSpaceSize = _Config._FileSize - FileTypeBox_SIZE - _FreeSpaceSize - EmptyMediaDataBox_SIZE64;
	}
	else
	{
		tmpsize = (FileHeader_MAXSIZE+1023)/1024;
		//add some tolerance
		tmpsize += 2; tmpsize *= 1024;
		printf("file size=%d, min header size=%d, max header size=%d\n", _Config._FileSize, FileHeader_SIZE, tmpsize);
		_FreeSpaceSize = tmpsize-FileTypeBox_SIZE;
		_MdatFreeSpaceSize = _Config._FileSize - FileTypeBox_SIZE - _FreeSpaceSize - EmptyMediaDataBox_SIZE;
	}
	//0666==everyone can read/write
	if (_pSink->Open( _Config._FileName, O_WRONLY|O_CREAT|O_TRUNC|_O_BINARY , 0666) == -1)
	{
		return ME_IO_ERROR;
	}

	AM_U8 tmpfile[256];
	::memcpy((char *)tmpfile,(char *)_Config._FileName,strlen((char *)_Config._FileName)-4);
	::memcpy((char *)tmpfile+strlen((char *)(_Config._FileName))-4,".264",5);
	_curPos = 0;

	//index settings
	_v_idx = _a_idx = 0;
	AM_UINT fpos;
	//make sure that each start file pos of the tmp index data is aligned to 8 bytes
	if (_largeFile == true)
	{
		put_IsoFile(_FreeSpaceSize-MovieBox_SIZE64);
		fpos = (AM_UINT)(_curPos - EmptyMediaDataBox_SIZE64);	   //just before mdat box
		printf("mdat box file pos=%d [0x%08x] ==header size(%d) ?\n", fpos, fpos, tmpsize);
		DbgLog((LOG_TRACE, 1, TEXT("mdat box file pos=%d [0x%08x] ==header size(%d) ?\n"),fpos, fpos, tmpsize));
		//between each kind of index, there is 16 bytes gap
		fpos -= _MaxAPacketCnt*1*sizeof(AM_UINT) + _MaxAPacketCnt*1*sizeof(AM_U64) + _MaxVPacketCnt*3*sizeof(AM_UINT) + _MaxVPacketCnt*1*sizeof(AM_U64) + 80;
		fpos &= 0xfffffff8;
		_vctts_cur_fpos = _vctts_fpos = fpos; fpos+=_MaxVPacketCnt*2*sizeof(AM_UINT)+16;
		_vstsz_cur_fpos = _vstsz_fpos = fpos; fpos+=_MaxVPacketCnt*1*sizeof(AM_UINT)+16;
		_vstco_cur_fpos = _vstco_fpos = fpos; fpos+=_MaxVPacketCnt*1*sizeof(AM_U64)+16;
		_astsz_cur_fpos = _astsz_fpos = fpos; fpos+=_MaxAPacketCnt*1*sizeof(AM_UINT)+16;
		_astco_cur_fpos = _astco_fpos = fpos; fpos+=_MaxAPacketCnt*1*sizeof(AM_U64)+16;
	}
	else
	{
		put_IsoFile(_FreeSpaceSize-MovieBox_SIZE);
		fpos = (AM_UINT)(_curPos - EmptyMediaDataBox_SIZE);	   //just before mdat box
		printf("mdat box file pos=%d [0x%08x] ==header size(%d) ?\n", fpos, fpos, tmpsize);
		//between each kind of index, there is 16 bytes gap
		DbgLog((LOG_TRACE, 1, TEXT("mdat box file pos=%d [0x%08x] ==header size(%d) ?\n"),fpos, fpos, tmpsize));

		fpos -= _MaxAPacketCnt*2*sizeof(AM_UINT) + _MaxVPacketCnt*4*sizeof(AM_UINT) + 80;  //start of tmp index buffer
		fpos &= 0xfffffff8;
		_vctts_cur_fpos = _vctts_fpos = fpos; fpos+=_MaxVPacketCnt*2*sizeof(AM_UINT)+16;
		_vstsz_cur_fpos = _vstsz_fpos = fpos; fpos+=_MaxVPacketCnt*1*sizeof(AM_UINT)+16;
		_vstco_cur_fpos = _vstco_fpos = fpos; fpos+=_MaxVPacketCnt*1*sizeof(AM_UINT)+16;
		_astsz_cur_fpos = _astsz_fpos = fpos; fpos+=_MaxAPacketCnt*1*sizeof(AM_UINT)+16;
		_astco_cur_fpos = _astco_fpos = fpos; fpos+=_MaxAPacketCnt*1*sizeof(AM_UINT)+16;
	}
	printf("_vctts_fpos=%d[0x%08x], _vstsz_fpos=%d[0x%08x], _vstco_fpos=%d[0x%08x]\n",
		_vctts_fpos, _vctts_fpos, _vstsz_fpos, _vstsz_fpos, _vstco_fpos, _vstco_fpos);
	DbgLog((LOG_TRACE, 1, TEXT("_vctts_fpos=%d[0x%08x], _vstsz_fpos=%d[0x%08x], _vstco_fpos=%d[0x%08x]\n"),_vctts_fpos, _vctts_fpos, _vstsz_fpos, _vstsz_fpos, _vstco_fpos, _vstco_fpos));
	printf("_astsz_fpos=%d[0x%08x], _astco_fpos=%d[0x%08x]\n", _astsz_fpos, _astsz_fpos, _astco_fpos, _astco_fpos);
	DbgLog((LOG_TRACE, 1, TEXT("_astsz_fpos=%d[0x%08x], _astco_fpos=%d[0x%08x]\n"),_astsz_fpos, _astsz_fpos, _astco_fpos, _astco_fpos));
	printf("tmp index file end pos=%d [0x%08x]\n", fpos, fpos);
	DbgLog((LOG_TRACE, 1, TEXT("tmp index file end pos=%d [0x%08x]\n"),fpos, fpos));
	if (_pSinkIdx->Open( _Config._FileName, O_RDWR|_O_BINARY , 0666) == -1)
	{
		return ME_IO_ERROR;
	}
	if (_largeFile == true)
	{
		_SpsPps_pos = FileTypeBox_SIZE+MovieBox_SIZE64+8;//8 for the free atom header
	}
	else
	{
		_SpsPps_pos = FileTypeBox_SIZE+MovieBox_SIZE+8;//8 for the free atom header
	}

	_HaveSpace = 1;
	_frame_field_set = 0;
	_isStopped = false;

	return ME_OK;
}

AM_ERR CamIsoMuxer::IdxWriter_2()
{
	bool write_v = false;
	bool write_a = false;

	if (_v_idx >= _IDX_SIZE)
	{
		write_v = true;
	}
	if (_a_idx >= _IDX_SIZE)
	{
		write_a = true;
	}

	if (write_v == true)
	{
		DbgLog((LOG_TRACE, 1, TEXT("write video index: 0x%08x 0x%08x 0x%08x\n"),_vctts_cur_fpos, _vstsz_cur_fpos, _vstco_cur_fpos));
		printf("write video index: 0x%08x 0x%08x 0x%08x\n", _vctts_cur_fpos, _vstsz_cur_fpos, _vstco_cur_fpos);
		_pSinkIdx->Seek(_vctts_cur_fpos, SEEK_SET);
		_pSinkIdx->Write((AM_U8 *)_vctts, _IDX_SIZE*2*sizeof(AM_UINT));
		_vctts_cur_fpos += _IDX_SIZE*2*sizeof(AM_UINT);
		_pSinkIdx->Seek(_vstsz_cur_fpos, SEEK_SET);
		_pSinkIdx->Write((AM_U8 *)_vstsz, _IDX_SIZE*1*sizeof(AM_UINT));
		_vstsz_cur_fpos += _IDX_SIZE*1*sizeof(AM_UINT);
		_pSinkIdx->Seek(_vstco_cur_fpos, SEEK_SET);
		if (_largeFile == true)
		{
		_pSinkIdx->Write((AM_U8 *)_vstco64, _IDX_SIZE*1*sizeof(AM_U64));
		_vstco_cur_fpos += _IDX_SIZE*1*sizeof(AM_U64);
		}
		else
		{
		_pSinkIdx->Write((AM_U8 *)_vstco, _IDX_SIZE*1*sizeof(AM_UINT));
		DbgLog((LOG_TRACE, 1, TEXT("sinkIdx write _IDX_SIZE")));
		_vstco_cur_fpos += _IDX_SIZE*1*sizeof(AM_UINT);
		}

		_v_idx -= _IDX_SIZE;
		if (_v_idx > 0)
		{
			::memcpy(&_vctts[0], &_vctts[_IDX_SIZE*2], _v_idx*2*sizeof(AM_UINT));
			::memcpy(&_vstsz[0], &_vstsz[_IDX_SIZE*1], _v_idx*1*sizeof(AM_UINT));
			if (_largeFile == true)
			{
			::memcpy(&_vstco64[0], &_vstco64[_IDX_SIZE*1], _v_idx*1*sizeof(AM_U64));
			}
			else
			{
			::memcpy(&_vstco[0], &_vstco[_IDX_SIZE*1], _v_idx*1*sizeof(AM_UINT));
			}
		}

	}
	if (write_a == true)
	{
		printf("write audio index: 0x%08x 0x%08x\n", _astsz_cur_fpos, _astco_cur_fpos);
		_pSinkIdx->Seek(_astsz_cur_fpos, SEEK_SET);
		_pSinkIdx->Write((AM_U8 *)_astsz, _IDX_SIZE*1*sizeof(AM_UINT));
		_astsz_cur_fpos += _IDX_SIZE*1*sizeof(AM_UINT);
		_pSinkIdx->Seek(_astco_cur_fpos, SEEK_SET);
		if (_largeFile == true)
		{
		_pSinkIdx->Write((AM_U8 *)_astco64, _IDX_SIZE*1*sizeof(AM_U64));
		_astco_cur_fpos += _IDX_SIZE*1*sizeof(AM_U64);
		}
		else
		{
		_pSinkIdx->Write((AM_U8 *)_astco, _IDX_SIZE*1*sizeof(AM_UINT));
		_astco_cur_fpos += _IDX_SIZE*1*sizeof(AM_UINT);
		}
		_a_idx -= _IDX_SIZE;
		if (_a_idx > 0)
		{
			::memcpy(&_astsz[0], &_astsz[_IDX_SIZE*1], _a_idx*1*sizeof(AM_UINT));
			if (_largeFile == true)
			{
			::memcpy(&_astco64[0], &_astco64[_IDX_SIZE*1], _a_idx*1*sizeof(AM_U64));
			}
			else
			{
			::memcpy(&_astco[0], &_astco[_IDX_SIZE*1], _a_idx*1*sizeof(AM_UINT));
			}
		}
	}

	return ME_OK;
}

AM_ERR CamIsoMuxer::Stop()
{
	if(_isStopped)
	{
		return ME_OK;
	}
	if( G_pMuxerParam->MuxerType == 3 || m_sp264 == false)
	{
		 _pSink->Close();
		 return ME_OK;
	}

	//write tmp index in memory to file
	if (_v_idx != 0)
	{
		_pSinkIdx->Seek(_vctts_cur_fpos, SEEK_SET);
		_pSinkIdx->Write((AM_U8 *)_vctts, _v_idx*2*sizeof(AM_UINT));
		_vctts_cur_fpos += _v_idx*2*sizeof(AM_UINT);
		_pSinkIdx->Seek(_vstsz_cur_fpos, SEEK_SET);
		_pSinkIdx->Write((AM_U8 *)_vstsz, _v_idx*1*sizeof(AM_UINT));
		_vstsz_cur_fpos += _v_idx*1*sizeof(AM_UINT);
		_pSinkIdx->Seek(_vstco_cur_fpos, SEEK_SET);
		if (_largeFile == true)
		{
		_pSinkIdx->Write((AM_U8 *)_vstco64, _v_idx*1*sizeof(AM_U64));
		_vstco_cur_fpos += _v_idx*1*sizeof(AM_U64);
		}
		else
		{
		_pSinkIdx->Write((AM_U8 *)_vstco, _v_idx*1*sizeof(AM_UINT));
		_vstco_cur_fpos += _v_idx*1*sizeof(AM_UINT);
		}
	}
	if (_a_idx != 0)
	{
		_pSinkIdx->Seek(_astsz_cur_fpos, SEEK_SET);
		_pSinkIdx->Write((AM_U8 *)_astsz, _a_idx*1*sizeof(AM_UINT));
		_astsz_cur_fpos += _a_idx*1*sizeof(AM_UINT);
		_pSinkIdx->Seek(_astco_cur_fpos, SEEK_SET);
		if (_largeFile == true)
		{
		_pSinkIdx->Write((AM_U8 *)_astco64, _a_idx*1*sizeof(AM_U64));
		_astco_cur_fpos += _a_idx*1*sizeof(AM_U64);
		}
		else
		{
		_pSinkIdx->Write((AM_U8 *)_astco, _a_idx*1*sizeof(AM_UINT));
		_astco_cur_fpos += _a_idx*1*sizeof(AM_UINT);
		}
	}
   
	if (_largeFile == true)
	{
		put_IsoFile(_FreeSpaceSize-MovieBox_SIZE64);
	}
	else
	{
		put_IsoFile(_FreeSpaceSize-MovieBox_SIZE);
	}
	_pSink->Close();
	_pSinkIdx->Close();
	printf("_VPacketCnt=%d, _APacketCnt=%d\n", _VPacketCnt, _APacketCnt);
	DbgLog((LOG_TRACE, 1, TEXT("_VPacketCnt=%d, _APacketCnt=%d\n"), _VPacketCnt, _APacketCnt));
	printf("_v_dropped=%d, _a_dropped=%d\n", _v_dropped, _a_dropped);
	_isStopped = true;
	return ME_OK;
}


//return 0 means pBuffer processed, return 1 means pBuffer should not return to pool
AM_INT CamIsoMuxer::ProcessVideoData(FRAME_INFO *info, BYTE * pBuffer)
{
	if( G_pMuxerParam->MuxerType == 3 ||  m_sp264 == false)
	{
		_pSink->Write(pBuffer, info->size);
		return ME_OK;
	}

	AM_UINT ctts;
	static AM_UINT positive_ctts_delta;
	if (_frame_field_set == 0)
	{
		if(info->pic_struct == 1)_frame_field_set = 2;
		else _frame_field_set = 1;
		if (info->pic_struct == 1) _v_delta = (AM_UINT)(2.0*_Config._scale*CLOCK/_Config._rate);
		
		if (_Config._advanced)
		{
			// Advanced GOP
			positive_ctts_delta = _v_delta * (_Config._M >> 1);
		}
		else
		{
			// Simple GOP
			positive_ctts_delta = _v_delta;
		}
	}
	
	AM_U64 pts = info->pts_high;
	pts = ( pts << 32) + info->pts;

	AM_UINT size1, len1, sei_len1, sei_len;
	size1 = info->size;
	len1 = GetAuDelimiter(pBuffer, 0);

	len1 += (info->flags==0x01 || info->flags==0x02 ||
		(_Config._M == 255 && _Config._N == 255 && _VPacketCnt%10 == 0))?GetSpsPps(pBuffer, len1):0;
	
	size1 -= len1;//we do not write sps/pps packet

	// AU delimiter
	static AM_U8 aubuf[8]={0x00,0x00,0x00,0x02,0x09,0x00,0x00,0x00}; 
	size1 += 6;

	if (_MdatFreeSpaceSize < size1 )
	{		
		Stop();
		HRESULT hr;
		int result;
		_MdatFreeSpaceSize = 0;
		hr = G_pRecordControl->SetForceIDR(&result);
				
		/*if(_VPacketCnt % (_Config._N *_Config._idr_interval) != 0)
		{
			 _VPacketCnt++;
			return ME_OK;
		}*/

		if (info->flags != 0x01)   
		{
			return ME_OK;
		}

		_fileCount ++;
		char tmp_count[4];
		char tmp_ext[5];
		_itoa(_fileCount, (char *)tmp_count, 10);
		int tmp_len = strlen(G_pMuxerParam->MuxerFilename);
		tmp_len = tmp_len - 4;
		::memcpy(tmp_ext,G_pMuxerParam->MuxerFilename + tmp_len ,5);
		if( NULL != strstr((char *)_Config._FileName,"_second") )
		{
			::memcpy(_Config._FileName+tmp_len,"_second",strlen("_second"));
			tmp_len += strlen("_second");
		}
		_Config._FileName[tmp_len++]='_';
		
		::memcpy(_Config._FileName+tmp_len,tmp_count,strlen(tmp_count));
		tmp_len += strlen(tmp_count);
		::memcpy(_Config._FileName+tmp_len,tmp_ext,5);
		Run();
		_pts_off = pts;
	}	

	//ctts = (AM_UINT)((pts-_pts_off+positive_ctts_delta-_vdts)*_Config._clk/CLOCK);
	ctts = _v_delta;
	DbgLog((LOG_TRACE, 1, TEXT("ctts[%d]=%d,pts=%lld"),_VPacketCnt,ctts,pts));
	AM_U64 curPos = _curPos;
		  
	//write AU delimiter
	if (info->flags==1 || info->flags==2 )
	{
		aubuf[5] = 0x10; 
	}
	else if (info->flags == 4)
	{
		aubuf[5] = 0x50;
	}
	else
	{
		aubuf[5] = 0x30;
	}
	put_buffer(aubuf, 6);

	//write SEI packet
	for (sei_len1=0;;)
	{
		sei_len = GetSeiLen(pBuffer, len1+sei_len1);
		if (sei_len > 0)
		{
			put_be32(sei_len-4);
			AM_UINT offset = len1+sei_len1+4;
			AM_UINT remain = sei_len-4;
  
			if (offset < info->size)
			{	
				AM_UINT left = info->size-offset;							
				if (remain <= left)						   
				{								
					put_buffer(pBuffer+offset, remain);						   
				}						   
				else
				{								
					put_buffer(pBuffer+offset, left);
					remain -= left; offset = 0;
				}
			}						
			else
			{
				offset -= info->size;
			}
			sei_len1 += sei_len;
		}
		else
		{
			break;
		}
	}

	//write I/P/B frame
	if (size1-6-sei_len1 > 0)
	{
		put_be32(size1-6-sei_len1-4);
		AM_UINT offset = len1+sei_len1+4;
		AM_UINT remain = size1-6-sei_len1-4;

		if (offset < info->size)
		{
			AM_UINT left = info->size - offset;
			if (remain <= left)
			{
				put_buffer(pBuffer+offset, remain);
			}
			else
			{
				put_buffer(pBuffer+offset, left);
				remain -= left; offset = 0;
			}
		}
		else
		{
			offset -= info->size;
		}

	}

	//make sure current frame not wrapped by encoder

	//update index

	_vctts[_v_idx*2+0] = le_to_be32(1);	 //sample_count
	_vctts[_v_idx*2+1] = le_to_be32(ctts);  //sample_delta
	_vstsz[_v_idx] = le_to_be32(size1);
	if (_largeFile == true)
	{
		_vstco64[_v_idx] = le_to_be64(curPos);
	}
	else
	{
		_vstco[_v_idx] = le_to_be32((AM_UINT)curPos);
	}
	_v_idx++;

	if (_v_idx >= _IDX_SIZE) 
	{
		IdxWriter_2();
	}	

	_VPacketCnt++;
	if (_VPacketCnt >= _MaxVPacketCnt)
	{
		if (_HaveSpace == 1) printf("file full for video 1\n");
		_HaveSpace = 0;
	}

	_MdatFreeSpaceSize -= size1;
	if (info->flags == 0x01)
	{
		_IdrPacketCnt++;
	}
	//_vdts = (AM_U64)(1.0*_VPacketCnt*_scale*CLOCK/_rate);
	_vdts += (AM_U64)_v_delta;

	return 0;
}


//Sequence Parameters Set, Picture Parameters Set
AM_UINT CamIsoMuxer::GetSpsPps(BYTE *pBuffer,AM_UINT offset)
{
	AM_UINT i, pos, len;
	AM_UINT code, tmp;
	pos = 4;
	tmp = GetByte(pBuffer, offset+pos);
	if ((tmp&0x1F) != 0x07)
	{
		/* no SPS */
		return 0;
	}
	if (_spsSize != 0)
	{
		/* we do get SPS & PPS, skip the repeats */
		return _spsSize+_ppsSize+8-_spsPad-_ppsPad;
	}
	/* find SPS */
	for (i=0,code=0xffffffff; ; i++,pos++)
	{
		tmp = GetByte(pBuffer, offset+pos);
#if 1
		if (tmp == 0xffffffff)
		{
			for (AM_UINT m=0; m<64; m++)
			{
				printf("%02x ", pBuffer[offset+m]);
			}
			printf("\n");
			return 0;
		}
#endif
		if ((code=(code<<8)|tmp) == 0x00000001)
		{
			break;//found next start code
		}
		_sps[i] = tmp;
		DbgLog((LOG_TRACE, 1, TEXT("_sps[%d]=%d"),i,_sps[i]));
	}
	//000000 of the next start code 00000001 also written into _sps[]
	//full_sps_len = pos-4+1;//including 00000001 start code
	//len: sps size excluding 00000001 start code
	len = (pos-4+1) - 4;
	_spsPad = len & 3;
	if (_spsPad > 0) _spsPad = 4 - _spsPad;
	_spsSize = len + _spsPad;
	//000000 of the next start code written will done this
	//for (i=len; i<_spsSize; i++)
	//{
	//	_sps[i] = 0;
	//}

	/* find PPS */
	pos++;//now pos point to one byte after 0x00000001
	tmp = GetByte(pBuffer, offset+pos);
	if ((tmp&0x1F) != 0x08)
	{
		/* no PPS */
		return 0;
	}
	for (i=0,code=0xffffffff; ; i++,pos++)
	{
		tmp = GetByte(pBuffer, offset+pos);
#if 1
		if (tmp == 0xffffffff)
		{
			for (AM_UINT m=0; m<64; m++)
			{
				printf("%02x ", pBuffer[offset+m]);
			}
			printf("\n");
			return 0;
		}
#endif
		if ((code=(code<<8)|tmp) == 0x00000001)
		{
			break;
		}
		_pps[i] = tmp;
		DbgLog((LOG_TRACE, 1, TEXT("_pps[%d]=%d"),i,_pps[i]));
	}
	//000000 of the next start code 00000001 also written into _pps[]
	//len: pps size excluding 00000001 start code
	//full_pps_len = (pos-4+1) - full_sps_len; //including 00000001 start code
	len = (pos-4+1) - (len+4) - 4;
	_ppsPad = len & 3;
	if (_ppsPad > 0) _ppsPad = 4 - _ppsPad;
	_ppsSize = len + _ppsPad;
	//000000 of the next start code written will done this
	//for (i=len; i<_ppsSize; i++)
	//{
	//	_pps[i] = 0;
	//}
	//backup sps and pps
	if (_SpsPps_pos != 0)
	{
		_pSinkIdx->Seek(_SpsPps_pos, SEEK_SET);
		_pSinkIdx->Write((AM_U8 *)&_spsSize, 4); _pSinkIdx->Write(_sps, _SPS_SIZE);
		_pSinkIdx->Write((AM_U8 *)&_ppsSize, 4); _pSinkIdx->Write(_pps, _PPS_SIZE);
		_pSinkIdx->Write((AM_U8 *)&_frame_field_set, 4);
		_SpsPps_pos = 0;
	}
	return _spsSize+_ppsSize+8-_spsPad-_ppsPad;
}

//Sequence Enhancement Information
AM_UINT CamIsoMuxer::GetSeiLen(BYTE *pBuffer, AM_UINT offset)
{
	AM_UINT code, tmp, pos;
	pos = 4;
	tmp = GetByte(pBuffer, offset+pos);
	if ((tmp&0x1F) != 0x06)
	{
		/* no SEI */
		return 0;
	}
	/* find SEI */
	pos++;
	for (code=0xffffffff; ; pos++)
	{
		tmp = GetByte(pBuffer, offset+pos);
#if 1
		if (tmp == 0xffffffff)
		{
			for (AM_UINT m=0; m<64; m++)
			{
				printf("%02x ", pBuffer[offset+m]);
			}
			printf("\n");
			return 0;
		}
#endif
		if ((code=(code<<8)|tmp) == 0x00000001)
		{
			break;//found next start code
		}
	}
	//pos point to 01 of start code 0x00000001
	//pos-4 is the last position of SEI
	return pos-4+1;
}

//access unit delimiter 0000000109x0
AM_UINT CamIsoMuxer::GetAuDelimiter(BYTE *pBuffer, AM_UINT offset)
{
	AM_UINT tmp, pos;
	pos = 4;
	tmp = GetByte(pBuffer, offset+pos);
	if ((tmp&0x1F) == 0x09)
	{
		return 6;
	}
	return 0;
}


