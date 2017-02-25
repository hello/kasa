#include <fcntl.h>	 //for open O_* flags
#include <stdlib.h>	//for malloc/free
#include <string.h>	//for strlen/memset
#include <stdio.h>	 //for printf
#include <assert.h>
#include <io.h>
#include "Receivetype.h"
#include "mp4muxer.h"



#define _ScaledWidth		_Config._Width
#define _ScaledHeight		_Config._Height
#define FOURCC(a,b,c,d)		((a<<24)|(b<<16)|(c<<8)|(d))
#define CLOCK				90000


AM_UINT GetByte(unsigned char* pBuffer,AM_UINT pos)
{ 
	return pBuffer[pos];
}

//Class CamFileSink

CamFileSink::CamFileSink()
	:_cnt(0),_handle(-1)
{
}

CamFileSink::~CamFileSink()
{
	Close();	
}

AM_INT CamFileSink::Open(char *name, AM_UINT flags, AM_UINT mode)
{
	if (_handle == -1)
	{
		_handle = ::_open(name, flags, mode);
		_cnt = 0;
		return _handle;
	}
	else //can not open again before close it
	{
		return -1;
	}
}

AM_INT CamFileSink::Read(AM_U8 *data, AM_INT len)
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

AM_U64 CamFileSink::Seek(AM_U64 offset, AM_INT whence)
{
	assert(_handle != -1);
	_FlushBuffer();
	return ::_lseeki64(_handle, offset, whence);
}

AM_INT CamFileSink::Write(AM_U8 *data, AM_INT len)
{
	assert(_handle != -1);
	//we buffer some content to minimize the transfer on the network
	AM_INT ret_len = 0;
	int size = _SINK_BUF_SIZE-_cnt;
	int wrt_len;
	if (len >= size)
	{
		//out of buffer, we have to actually write
		::memcpy(_buf+_cnt, data, size);
		wrt_len = ::_write(_handle, _buf, _SINK_BUF_SIZE);
		_cnt = 0;
		//_Sync();
		data += size;
		len -= size;
		ret_len += size;
		size = _SINK_BUF_SIZE;
		while (len >= size)
		{
			wrt_len = ::_write(_handle, data, _SINK_BUF_SIZE);
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

AM_ERR CamFileSink::Close()
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
void CamFileSink::_FlushBuffer()
{
	int wrt_len =0;
	if (_cnt != 0)
	{
		wrt_len = ::_write(_handle,_buf, _cnt);
		assert(_cnt == wrt_len);
		_cnt = 0;
	}
}


CamMp4Muxer::CamMp4Muxer(char* filename):_pSink(NULL)
	,_pSinkIdx(NULL)
	,_pSinkIdx2(NULL)
	,_v_idx(0)
	,_vctts_fpos(0)
	,_vstsz_fpos(_IDX_SIZE*2*sizeof(AM_UINT))
	,_vstco_fpos(_IDX_SIZE*3*sizeof(AM_UINT))
	,_v_ss_idx(0)
	,_vstss_fpos(0)
	,_VPacketCnt(0)
	,_IdrPacketCnt(0)
	
{
		strncpy(_filename, filename,256);
}	

CamMp4Muxer::~CamMp4Muxer()
{
	if (_pSink != NULL) delete _pSink;
	if (_pSinkIdx != NULL) delete _pSinkIdx;
	if (_pSinkIdx2 != NULL) delete _pSinkIdx2;
}

//update index
AM_ERR CamMp4Muxer::UpdateIdx(AM_UINT pts, AM_UINT stsz, AM_UINT stco)
{
	static AM_UINT last_pts = pts;
	AM_UINT ctts;
	if (_VPacketCnt == 0)
	{
		ctts = _Config._rate*_Config._clk/_Config._scale;
	}
	else
	{
		ctts = pts -last_pts;
		last_pts = pts;
	}
	
	_vctts[_v_idx*2+0] = le_to_be32(1);	 //sample_count
	_vctts[_v_idx*2+1] = le_to_be32(ctts);  //sample_delta
	_vstsz[_v_idx] = le_to_be32(stsz);
	_vstco[_v_idx] = stco;
	_average_ctts += ctts;
	_VPacketCnt++;
	_v_idx++;
	if (_v_idx >= _IDX_SIZE) 
	{
		return IdxWriter();
	}	
	else
	{
		return ME_OK;
	}
}

//update index
AM_ERR CamMp4Muxer::UpdateIdx2(AM_UINT stss)
{
	_vstss[_v_ss_idx] = le_to_be32(stss);
	_v_ss_idx++;
	_IdrPacketCnt++;
	if (_v_ss_idx >= _IDX_SIZE)
	{
		
		return IdxWriter2();
	}	
	else
	{
		return ME_OK;
	}
}

AM_ERR CamMp4Muxer::UpdateSps(AM_U8* sps, AM_UINT sps_size)
{
	::memcpy(_sps, sps, sps_size);
	_spsSize = sps_size;
	return ME_OK;
}

AM_ERR CamMp4Muxer::UpdatePps(AM_U8* pps, AM_UINT pps_size)
{
	::memcpy(_pps, pps, pps_size);
	_ppsSize = pps_size;
	return ME_OK;
}


AM_ERR CamMp4Muxer::Mux(int* p_mdat_handle)
{	
	if ((_pSink = new CamFileSink()) == NULL)
	{
		return ME_NO_MEMORY;
	}
	char mp4_filename[256];
	strncpy(mp4_filename,_filename, strlen(_filename)+1);
	strncpy(mp4_filename+strlen(_filename), ".mp4", strlen(".mp4")+1);
	if (_pSink->Open(mp4_filename, O_WRONLY|O_CREAT|O_TRUNC|_O_BINARY , 0644) == -1)
	{
		return ME_IO_ERROR;
	}
	put_IsoFile(*p_mdat_handle);
	
	_pSink->Close();
	_pSinkIdx->Close();
	_pSinkIdx2->Close();

	::_close(*p_mdat_handle);
	*p_mdat_handle = 0;

	char del_filename[256];
	sprintf(del_filename,"%s%s",_filename,".mdat");
	remove(del_filename);
	sprintf(del_filename,"%s%s",_filename,".idx");
	remove(del_filename);
	sprintf(del_filename,"%s%s",_filename,".idx2");
	remove(del_filename);
	return ME_OK;
}
void CamMp4Muxer::put_byte(AM_UINT data)
{
	AM_U8 w[1];
	w[0] = data;	  //(data&0xFF);
	_pSink->Write((AM_U8 *)&w, 1);
	_curPos++;
}

void CamMp4Muxer::put_be16(AM_UINT data)
{
	AM_U8 w[2];
	w[1] = (AM_U8)(data&0x00FF);
	w[0] = (AM_U8)((data&0xFF00)>>8);
	_pSink->Write((AM_U8 *)&w, 2);
	_curPos += 2;
}

void CamMp4Muxer::put_be24(AM_UINT data)
{
	AM_U8 w[3];
	w[2] = (AM_U8)(data&0x0000FF);
	w[1] = (AM_U8)((data&0x00FF00)>>8);
	w[0] = (AM_U8)((data&0xFF0000)>>16);
	_pSink->Write((AM_U8 *)&w, 3);
	_curPos += 3;
}

void CamMp4Muxer::put_be32(AM_UINT data)
{
	AM_U8 w[4];
	w[3] = (AM_U8)(data&0x000000FF);
	w[2] = (AM_U8)((data&0x0000FF00)>>8);
	w[1] = (AM_U8)((data&0x00FF0000)>>16);
	w[0] = (AM_U8)((data&0xFF000000)>>24);
	_pSink->Write((AM_U8 *)&w, 4);
	_curPos += 4;
}

void CamMp4Muxer::put_be64(AM_U64 data)
{
	AM_U8 w[8];
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

void CamMp4Muxer::put_buffer(AM_U8 *pdata, AM_UINT size)
{
	_pSink->Write(pdata, size);
	_curPos += size;
}

AM_UINT CamMp4Muxer::get_byte()
{
	AM_U8 data[1];
	_pSink->Read(data, 1);
	return data[0];
}

AM_UINT CamMp4Muxer::get_be16()
{
	AM_U8 data[2];
	_pSink->Read(data, 2);
	return (data[1] | (data[0]<<8));
}

AM_UINT CamMp4Muxer::get_be32()
{
	AM_U8 data[4];
	_pSink->Read(data, 4);
	return (data[3] | (data[2]<<8) | (data[1]<<16) | (data[0]<<24));
}

void CamMp4Muxer::get_buffer(AM_U8 *pdata, AM_UINT size)
{
	_pSink->Read(pdata, size);
}

AM_UINT CamMp4Muxer::le_to_be32(AM_UINT data)
{
	AM_UINT rval;
	rval  = (data&0x000000FF)<<24;
	rval += (data&0x0000FF00)<<8;
	rval += (data&0x00FF0000)>>8;
	rval += (data&0xFF000000)>>24;
	return rval;
}

AM_U64 CamMp4Muxer::le_to_be64(AM_U64 data)
{
	AM_U64 rval = 0ULL;
/*	rval += (data&&0x00000000000000FFULL)<<56;
	rval += (data&&0x000000000000FF00ULL)>>8<<48;
	rval += (data&&0x0000000000FF0000ULL)>>16<<40;
	rval += (data&&0x00000000FF000000ULL)>>24<<32;
	rval += (data&&0x000000FF00000000ULL)>>32<<24;
	rval += (data&&0x0000FF0000000000ULL)>>40<<16;
	rval += (data&&0x00FF000000000000ULL)>>48<<8;
	rval += (data&&0xFF00000000000000ULL)>>56;*/

	return rval;
}
//--------------------------------------------------

#define VideoMediaHeaderBox_SIZE			20
#define DataReferenceBox_SIZE			   28
#define DataInformationBox_SIZE			 (8+DataReferenceBox_SIZE) // 36
#define AvcConfigurationBox_SIZE			(19+_spsSize+_ppsSize)
#define BitrateBox_SIZE					 20
#define VisualSampleEntry_SIZE			  (86+AvcConfigurationBox_SIZE+BitrateBox_SIZE) // 181+_spsSize+_ppsSize
#define VideoSampleDescriptionBox_SIZE	  (16+VisualSampleEntry_SIZE) // 197+_spsSize+_ppsSize
#define DecodingTimeToSampleBox_SIZE		24
#define CompositionTimeToSampleBox_SIZE	 (16+(_VPacketCnt<<3))
#define CompositionTimeToSampleBox_MAXSIZE  (16+(_MaxVPacketCnt<<3))
#define SampleToChunkBox_SIZE			   28
#define VideoSampleSizeBox_SIZE			 (20+(_VPacketCnt<<2))
#define VideoSampleSizeBox_MAXSIZE		  (20+(_MaxVPacketCnt<<2))
#define VideoChunkOffsetBox_SIZE			(16+(_VPacketCnt<<2))
#define VideoChunkOffsetBox_MAXSIZE		 (16+(_MaxVPacketCnt<<2))
#define SyncSampleBox_SIZE				  (16+(_IdrPacketCnt<<2))
#define SyncSampleBox_MAXSIZE			   (16+(_MaxIdrPacketCnt<<2))
#define SampleDependencyTypeBox_SIZE		(12+_VPacketCnt)
#define SampleDependencyTypeBox_MAXSIZE	 (12+_MaxVPacketCnt)
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
#define VideoMediaInformationBox_SIZE	   (8+VideoMediaHeaderBox_SIZE+\
											 DataInformationBox_SIZE+\
											 VideoSampleTableBox_SIZE) // 401+_spsSize+_ppsSize+V*17+IDR*4
#define VideoMediaInformationBox_MAXSIZE	(8+VideoMediaHeaderBox_SIZE+\
											 DataInformationBox_SIZE+\
											 VideoSampleTableBox_MAXSIZE) // 401+_spsSize+_ppsSize+V*17+IDR*4

//--------------------------------------------------

#define VIDEO_HANDLER_NAME			 ((AM_U8 *)"Ambarella AVC")
#define VIDEO_HANDLER_NAME_LEN		 14 //strlen(VIDEO_HANDLER_NAME)+1
#define MediaHeaderBox_SIZE			32
#define VideoHandlerReferenceBox_SIZE  (32+VIDEO_HANDLER_NAME_LEN) // 46
#define VideoMediaBox_SIZE			 (8+MediaHeaderBox_SIZE+\
										VideoHandlerReferenceBox_SIZE+\
										VideoMediaInformationBox_SIZE) // 487+_spsSize+_ppsSize+V*17+IDR*4
#define VideoMediaBox_MAXSIZE		  (8+MediaHeaderBox_SIZE+\
										VideoHandlerReferenceBox_SIZE+\
										VideoMediaInformationBox_MAXSIZE) // 487+_spsSize+_ppsSize+V*17+IDR*4

//--------------------------------------------------

#define TrackHeaderBox_SIZE	 92
#define clefBox_SIZE			0//20
#define profBox_SIZE			0//20
#define enofBox_SIZE			0//20
#define taptBox_SIZE			0//(8+clefBox_SIZE+profBox_SIZE+enofBox_SIZE) // 68
#define VideoTrackBox_SIZE	  (8+TrackHeaderBox_SIZE+taptBox_SIZE+VideoMediaBox_SIZE) // 655+_spsSize+_ppsSize+V*17+IDR*4
#define VideoTrackBox_MAXSIZE   (8+TrackHeaderBox_SIZE+taptBox_SIZE+VideoMediaBox_MAXSIZE) // 655+_spsSize+_ppsSize+V*17+IDR*4
//--------------------------------------------------
#define MovieHeaderBox_SIZE  108
#define AMBABox_SIZE		 0//40
#define ObjDescrpBox_SIZE		24//33
#define UserDataBox_SIZE	 (8+AMBABox_SIZE) // 48
#define MovieBox_SIZE		(8+MovieHeaderBox_SIZE+\
							  ObjDescrpBox_SIZE+\
							  UserDataBox_SIZE+\
							  VideoTrackBox_SIZE)
#define MovieBox_MAXSIZE	 (8+MovieHeaderBox_SIZE+\
							  UserDataBox_SIZE+\
							  VideoTrackBox_MAXSIZE)
//--------------------------------------------------
#define EmptyFreeSpaceBox_SIZE 8 //(16+_SPS_SIZE+_PPS_SIZE) // 144  16=8 bytes header + 8 bytes data
//--------------------------------------------------
#define FileTypeBox_SIZE  32
#define EmptyMediaDataBox_SIZE   8
#define FileHeader_SIZE		  (FileTypeBox_SIZE+MovieBox_SIZE+EmptyFreeSpaceBox_SIZE) // 1439+_spsSize+_ppsSize+V*17+IDR*4+A*8
#define FileHeader_MAXSIZE	   (FileTypeBox_SIZE+MovieBox_MAXSIZE+EmptyFreeSpaceBox_SIZE) // 1439+_spsSize+_ppsSize+V*17+IDR*4+A*8



void CamMp4Muxer::put_VideoMediaInformationBox()
{
	AM_UINT i,  k;
	//MediaInformationBox

	put_be32(VideoMediaInformationBox_SIZE);	 //uint32 size
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
	put_be32(VideoSampleTableBox_SIZE);		  //uint32 size
	put_be32(FOURCC('s', 't', 'b', 'l'));		//'stbl'

	//SampleDescriptionBox
	put_be32(VideoSampleDescriptionBox_SIZE);	//uint32 size
	put_be32(FOURCC('s', 't', 's', 'd'));		//uint32 type
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(1);								 //uint32 entry_count
	//VisualSampleEntry
	AM_U8 EncoderName[31]="AVC Coding";;
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
	AM_UINT len = (AM_UINT)strlen((char *)EncoderName);
	put_byte(len);							   //char compressor_name[32]   //[0] is actual length
	put_buffer(EncoderName, 31);
	put_be16(0x0018);							//uint16 depth   //0x0018=images are in colour with no alpha
	put_be16(-1);								//int16 pre_defined
	//AvcConfigurationBox
	put_be32(AvcConfigurationBox_SIZE);		  //uint32 size
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
	AM_UINT delta = (AM_UINT)(1.0*_frame_field_set*_Config._rate*_Config._clk/_Config._scale);
	if(_VPacketCnt != 0)
	{
		delta= (AM_U32)(_average_ctts / _VPacketCnt);
	}
	put_be32(delta);							 //uint32 sample_delta

	//CompositionTimeToSampleBox
	put_be32(CompositionTimeToSampleBox_SIZE);   //uint32 size
	put_be32(FOURCC('c', 't', 't', 's'));		//'ctts'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(_VPacketCnt);					   //uint32 entry_count
	if (_VPacketCnt > 0) 
	{
		int rlen = 0;
		AM_UINT vctts[_IDX_SIZE*2];
		while (_pSinkIdx->Seek(_vctts_fpos, SEEK_SET) >= 0)
		{
			rlen = _pSinkIdx->Read((AM_U8*)vctts,_IDX_SIZE*2*sizeof(AM_UINT));
			if (rlen > 0)
			{
				put_buffer((AM_U8 *)vctts,rlen);
				_vctts_fpos += _IDX_SIZE*4*sizeof(AM_UINT);
			}
			else
			{
				break;
			}		
		}
		put_buffer((AM_U8 *)_vctts, _v_idx * 2*sizeof(AM_UINT));		
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

	if (_VPacketCnt > 0) 
	{
		int rlen = 0;
		AM_UINT vstsz[_IDX_SIZE];
		while (_pSinkIdx->Seek(_vstsz_fpos, SEEK_SET) >= 0)
		{
			rlen = _pSinkIdx->Read((AM_U8 *)vstsz,_IDX_SIZE*sizeof(AM_UINT));
			if (rlen > 0)
			{
				put_buffer((AM_U8 *)vstsz,rlen);
				_vstsz_fpos += _IDX_SIZE*4*sizeof(AM_UINT);
			}
			else
			{
				break;
			}		
		}
		put_buffer((AM_U8 *)_vstsz, _v_idx*sizeof(AM_UINT));		
	}

	//ChunkOffsetBox

	put_be32(VideoChunkOffsetBox_SIZE);		  //uint32 size
	put_be32(FOURCC('s', 't', 'c', 'o'));		//'stco'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(_VPacketCnt);					   //uint32 entry_count
	int tmp = MovieBox_SIZE  ;
	tmp = MovieHeaderBox_SIZE;
       tmp = UserDataBox_SIZE;
        tmp =     VideoTrackBox_SIZE;
	if (_VPacketCnt > 0) 
	{
		int rlen = 0;
		AM_UINT vstco[_IDX_SIZE];
		while (_pSinkIdx->Seek(_vstco_fpos, SEEK_SET) >= 0)
		{
			rlen = _pSinkIdx->Read((AM_U8 *)&vstco,_IDX_SIZE*sizeof(AM_UINT));
			if (rlen > 0)
			{
				for (unsigned int i = 0;i <rlen/(sizeof(AM_UINT)); i++)
				{
					vstco[i] += FileHeader_SIZE + EmptyMediaDataBox_SIZE;
					vstco[i] =  le_to_be32(vstco[i]);
				}
				put_buffer((AM_U8 *)vstco,rlen);
				_vstco_fpos += _IDX_SIZE*4*sizeof(AM_UINT);
			}
			else
			{
				break;
			}		
		}

		for (unsigned int i = 0;i < _v_idx; i++)
		{
			_vstco[i] +=  FileHeader_SIZE + EmptyMediaDataBox_SIZE;
			_vstco[i] =  le_to_be32(_vstco[i]);
		}
		put_buffer((AM_U8 *)_vstco, _v_idx*sizeof(AM_UINT));		
	}

	//SyncSampleBox
	put_be32(SyncSampleBox_SIZE);				//uint32 size
	put_be32(FOURCC('s', 't', 's', 's'));		//'stss'
	put_byte(0);								 //uint8 version
	put_be24(0);								 //bits24 flags
	put_be32(_IdrPacketCnt);					 //uint32 entry_count
	if (_IdrPacketCnt > 0) 
	{
		int rlen = 0;
		AM_UINT vstss[_IDX_SIZE];
		while (_pSinkIdx2->Seek(_vstss_fpos, SEEK_SET) >= 0)
		{
			rlen = _pSinkIdx2->Read((AM_U8 *)_vstss,_IDX_SIZE*sizeof(AM_UINT));
			if (rlen > 0)
			{
				put_buffer((AM_U8 *)vstss,rlen);
				_vstss_fpos += _IDX_SIZE*sizeof(AM_UINT);
			}
			else
			{
				break;
			}		
		}
		put_buffer((AM_U8 *)_vstss, _v_ss_idx*sizeof(AM_UINT));		
	}
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
		}
		else
		{
			put_byte(0x00);
		}
	}
}


void CamMp4Muxer::put_VideoMediaBox(AM_UINT Duration)
{
	//MediaBox
	put_be32(VideoMediaBox_SIZE);			   //uint32 size
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


void CamMp4Muxer::put_videoTrackBox(AM_UINT TrackId, AM_UINT Duration)
{
	//TrackBox

	put_be32(VideoTrackBox_SIZE);		  //uint32 size
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
	/*put_be32(taptBox_SIZE);
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
	put_be16(0);*/

	put_VideoMediaBox(Duration);
}

//--------------------------------------------------
void CamMp4Muxer::put_FileTypeBox()
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



void CamMp4Muxer::put_MovieBox()
{
	AM_UINT Duration;

	//MovieBox

	put_be32(MovieBox_SIZE);			   //uint32 size
	put_be32(FOURCC('m', 'o', 'o', 'v'));  //'moov'

	//MovieHeaderBox
	Duration = (AM_U32)_average_ctts;
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
	//put_be32(_InputVCnt+_InputACnt+1);	 //uint32 next_track_ID
	put_be32(2);

	put_be32(ObjDescrpBox_SIZE); 	 //uint32 size
	put_be32(FOURCC('i', 'o', 'd', 's'));  //'iods'
	put_byte(0);                           //uint8 version
	put_be24(0);                           //bits24 flags
	put_be32(0x10808080); 
	put_be32(0x07004FFF );
	put_be32(0xFF0F7FFF);

	//UserDataBox
	put_be32(UserDataBox_SIZE);			//uint32 size
	put_be32(FOURCC('u', 'd', 't', 'a'));  //'udta'
	//AMBABox
	/*put_be32(AMBABox_SIZE);						  //uint32 size
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
	put_be32(0);*/

	AM_UINT i=1; put_videoTrackBox(i, Duration);
}


void CamMp4Muxer::put_FreeSpaceBox(AM_UINT FreeSpaceSize)
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

void CamMp4Muxer::put_MediaDataBox(int mdat_handle)
{
	AM_U64 size = ::_lseeki64(mdat_handle, 0, SEEK_END);
	put_be32((AM_U32)size);							//uint32 size
	put_be32(FOURCC('m', 'd', 'a', 't'));	  //'mdat'
	
	::_lseeki64(mdat_handle, 0, SEEK_SET);
	unsigned char buf[_SINK_BUF_SIZE];
	int rlen = 0;
	do
	{
		rlen = ::_read(mdat_handle,buf,_SINK_BUF_SIZE);
		if (rlen > 0)
		{
			_pSink->Write(buf, rlen);
		}
		else
		{
			break;
		}
	} while (rlen > 0);
}

void CamMp4Muxer::put_IsoFile(int mdat_handle)
{
	_pSink->Seek(0, SEEK_SET);
	_curPos = 0;
	//FileTypeBox
	put_FileTypeBox();
	//MovieBox
	put_MovieBox();
	//FreeSpaceBox
	put_FreeSpaceBox(8);
	//MediaDataBox is generated during RUNNING
	put_MediaDataBox(mdat_handle);
}


//--------------------------------------------------

//copy from DV
void CamMp4Muxer::InitSdtp()
{
	AM_UINT lv0=(AM_UINT)_Config._M;
	AM_UINT EntryNo;
	AM_UINT ctr_L1;
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


//AM_U8 * filename
AM_ERR CamMp4Muxer::Init(format_t* p_format, AM_U8 rec_type)
{		
	_Config._FileType =  rec_type;
	/*_Config._M = (AM_U8)(p_h264_config->M);
	_Config._N = (AM_U8)(p_h264_config->N);
	_Config._idr_interval = p_h264_config->idr_interval;
	_Config._ar_x = p_h264_config->pic_info.ar_x;
	_Config._ar_x = p_h264_config->pic_info.ar_y;
	_Config._mode = p_h264_config->pic_info.frame_mode;
	_Config._scale = p_h264_config->pic_info.scale;
	_Config._rate = p_h264_config->pic_info.rate;
	
	if (p_h264_config->bitrate_control == CBR)
	{
		_Config._brate = _Config._brate_min = p_h264_config->average_bitrate ;
	}
	else
	{
		_Config._brate = (AM_UINT)( 1.0 * p_h264_config->average_bitrate * p_h264_config->max_vbr_rate_factor/ 100);
		if (p_h264_config->min_vbr_rate_factor < 40) p_h264_config->min_vbr_rate_factor = 40;
			_Config._brate_min = (AM_UINT)(1.0 * p_h264_config->average_bitrate * p_h264_config->min_vbr_rate_factor / 100);
			assert(_Config._brate_min != 0);
	}*/
	_Config._Width = p_format->encode_width;
	_Config._Height = p_format->encode_height;
	_Config._clk = 90000;
	_PixelAspectRatioWidth = 0;
	_PixelAspectRatioHeight = 0;
  
	if ((_pSinkIdx = new CamFileSink()) == NULL)
	{
		return ME_NO_MEMORY;
	}
	if ((_pSinkIdx2 = new CamFileSink()) == NULL)
	{
		return ME_NO_MEMORY;
	}
	char idx_filename[256];
	strncpy(idx_filename,_filename, strlen(_filename)+1);
	strncpy(idx_filename+strlen(_filename), ".idx", strlen(".idx")+1);
	if (_pSinkIdx->Open(idx_filename, O_RDWR|O_CREAT|O_TRUNC|_O_BINARY , 0644) == -1)
	{
		delete _pSinkIdx;
		_pSinkIdx = NULL;
		return ME_IO_ERROR;
	}
	strncpy(idx_filename+strlen(_filename), ".idx2", strlen(".idx2")+1);
	if (_pSinkIdx2->Open(idx_filename, O_RDWR|O_CREAT|O_TRUNC|_O_BINARY, 0644) == -1)
	{
		delete _pSinkIdx2;
		_pSinkIdx2 = NULL;
		return ME_IO_ERROR;
	} 

	_average_ctts = 0;
	//others init during run
	return ME_OK;

}

AM_ERR CamMp4Muxer::IdxWriter()
{
	bool write_v = false;
	if (_v_idx >= _IDX_SIZE)
	{
		write_v = true;
	}
	if (write_v)
	{
		_pSinkIdx->Write((AM_U8 *)_vctts, _IDX_SIZE*2*sizeof(AM_UINT));		
		_pSinkIdx->Write((AM_U8 *)_vstsz, _IDX_SIZE*1*sizeof(AM_UINT));	
		_pSinkIdx->Write((AM_U8 *)_vstco, _IDX_SIZE*1*sizeof(AM_UINT));		
		_v_idx -= _IDX_SIZE;
	}
	if (_v_idx > 0)
	{
		::memcpy(&_vctts[0], &_vctts[_IDX_SIZE*2], _v_idx*2*sizeof(AM_UINT));
		::memcpy(&_vstsz[0], &_vstsz[_IDX_SIZE*1], _v_idx*1*sizeof(AM_UINT));
		::memcpy(&_vstco[0], &_vstco[_IDX_SIZE*1], _v_idx*1*sizeof(AM_UINT));
	}
	return ME_OK;
}


AM_ERR CamMp4Muxer::IdxWriter2()
{
	bool write_v = false;
	if (_v_ss_idx >= _IDX_SIZE)
	{
		write_v = true;
	}
	if (write_v)
	{
		_pSinkIdx2->Write((AM_U8 *)_vstss, _IDX_SIZE*2*sizeof(AM_UINT));	
		_v_ss_idx -= _IDX_SIZE;
	}
	if (_v_ss_idx > 0)
	{
		::memcpy(&_vstss[0], &_vstss[_IDX_SIZE*1], _v_ss_idx*1*sizeof(AM_UINT));	
	}
	return ME_OK;
}




