#ifndef __RECEIVETYPE_H__
#define __RECEIVETYPE_H__

typedef unsigned char AM_U8;
typedef unsigned short AM_U16;
typedef unsigned long AM_U32;
typedef unsigned long long AM_U64;
typedef unsigned int AM_UINT;
typedef int AM_INT;
/*
same with tcp_processor.h
*/
struct FRAME_INFO
{
	AM_UINT	size;
	AM_UINT	pts;
	AM_UINT	pic_type;
	AM_UINT	reserved;
};

struct format_t {
	AM_U8	encode_type;		//0: none   1: H.264 (IAV_STREAM_TYPE_H264)   2: MJPEG (IAV_STREAM_TYPE_MJPEG)
	AM_U8	frame_interval;		// 1 ~ n	 ( final frame rate =  VIN frame rate / frame_interval )
	AM_U16	encode_width;		//encode width
	AM_U16	encode_height;
};

enum BR_TYPE
{
	CBR = 1,
	VBR = 3,
};

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

#endif//__RECEIVEFTYPE_H__
