/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// JPEG video sources
// Implementation
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <basetypes.h>
#include "iav_ioctl.h"
#include "bsreader.h"
#include "MyJPEGVideoSource.hh"
#include "assert.h"

MyJPEGVideoSource::MyJPEGVideoSource(UsageEnvironment& env, int streamID)
	: FramedSource(env)
	, fStreamID(streamID)
	, fMaxOutputPacketSize(1428) //1448 -20
	, fPTS(0)
	, fSessionID(0)
	, fType(0)
	, fPrecision(0)
	, fWidth(0)
	, fHeight(0)
	, fFragmentOffset(0)
	, fFragmentBeginAddr(NULL)
	, fFragmentLeftSize(0)
{
	fQuantizationTables = new u_int8_t[128];
}

MyJPEGVideoSource::~MyJPEGVideoSource()
{
	delete[] fQuantizationTables;
}

MyJPEGVideoSource*
MyJPEGVideoSource::createNew(UsageEnvironment & env, int streamID) {
	return new MyJPEGVideoSource(env, streamID);
}

u_int8_t MyJPEGVideoSource::type() {
	return fType;
}

u_int8_t MyJPEGVideoSource::qFactor() {
	return fQuality;
}

u_int8_t MyJPEGVideoSource::width() {// # pixels/8 (or 0 for 2048 pixels)
	return (fWidth >> 3);
}

u_int8_t MyJPEGVideoSource::height() {// # pixels/8 (or 0 for 2048 pixels)
	return (fHeight >> 3);
}

u_int8_t const* MyJPEGVideoSource::quantizationTables(u_int8_t& precision,
	u_int16_t& length) {
	precision = fPrecision;
	length = fQtableLength;
	return fQuantizationTables;
}

u_int32_t MyJPEGVideoSource::fragmentOffset(){
	return fFragmentOffset;
}

Boolean MyJPEGVideoSource::isFrameEnd() {
	return (fFragmentLeftSize == 0);
}

Boolean MyJPEGVideoSource::isJPEGVideoSource() const {
	return True;
}

void MyJPEGVideoSource::setMaxOutputPacketSize(unsigned OutputPacketSize) {
	fMaxOutputPacketSize = OutputPacketSize;
}

void MyJPEGVideoSource::doGetNextFrame() {
	if (fFragmentLeftSize == 0) {
		int parse_error_num = 0;
		while (parse_error_num <=3)
		{
			//unsigned acquiredFrameSize;
			u_int64_t frameDuration_usec;

			bsreader_frame_info_t frame_info;
			if (bsreader_get_one_frame(fStreamID, &frame_info) < 0) {
				printf("bs reader gets frame error\n");
				return;
			}

			Boolean bIsNewSession = (frame_info.bs_info.session_id != fSessionID);
			fSessionID = frame_info.bs_info.session_id;
			fQuality = frame_info.bs_info.jpeg_quality;
			//acquiredFrameSize = frame_info.frame_size;

			if (!bIsNewSession)  {
				if (frame_info.bs_info.PTS < fPTS) {
					frameDuration_usec = ((1<<30) +frame_info.bs_info.PTS - fPTS) * 100 / 9;
				} else {
					frameDuration_usec = (frame_info.bs_info.PTS - fPTS) * 100 / 9;	//pts is creatmented @ 90000Hz
				}
			} else
				frameDuration_usec = 0;

			fPresentationTime.tv_usec += (long) frameDuration_usec;

			while(fPresentationTime.tv_usec >= 1000000)
			{
				fPresentationTime.tv_usec -= 1000000;
				++fPresentationTime.tv_sec;
			}
			if (frame_info.bs_info.stream_end == 0) {
				if (Parse(frame_info.frame_addr, frame_info.frame_size) ) {
					fPTS = frame_info.bs_info.PTS;
					break;
				} else {
					printf("Parse error, skip this one and read next\n");
					parse_error_num++;
				}
			} else {
				printf("stream ended.\n");
				break;
			}
		}
		fFragmentOffset = 0;
	}else {
		fFragmentOffset += fFrameSize;
	}

	fFrameSize = fFragmentLeftSize > fMaxOutputPacketSize ? fMaxOutputPacketSize : fFragmentLeftSize;
	memcpy(fTo,fFragmentBeginAddr, fFrameSize );
	fFragmentLeftSize -= fFrameSize;
	fFragmentBeginAddr += fFrameSize;
	afterGetting(this);
}

Boolean MyJPEGVideoSource::Parse(unsigned char* begin_addr, u_int32_t size)
{
	do {
		unsigned char *pFrame = begin_addr;
		unsigned char *pFrame_end = begin_addr + size-2;
		if (*pFrame_end != 0xFF || *(pFrame_end+1) !=0xD9) {
			break;
		}
		if ((*pFrame++ != 0xFF) || (*pFrame++ != 0xD8)) {// SOI
			break;
		}
		if ((pFrame >pFrame_end) || (*pFrame++ != 0xFF) || (*pFrame++ != 0xE0)) {// APP0
			break;
		}
		u_int32_t segment_length = (pFrame[0]<<8) + pFrame[1];
		pFrame += segment_length;

		fQtableLength = 0;
		int j = fQtableLength;
		if ( (pFrame >pFrame_end) || (*pFrame++ != 0xFF) || (*pFrame++ != 0xDB)) { // DQT Y
			break;
		}
		segment_length = (pFrame[0]<<8) + pFrame[1];
		assert(segment_length == 67);

		u_int8_t element_precision = pFrame[2] >> 4;
		assert(element_precision == 0);

		if ( 0 == element_precision) {
			fPrecision &= ~0x1;
			fQtableLength+= 64;
		} else if ( 1 == element_precision) {
			fPrecision |= 0x1;
			fQtableLength+= 64*2;
		} else {
			break;
		}
		for (int k=3; j <fQtableLength; j++,k++) {
			fQuantizationTables[j] = pFrame[k];
		}

		pFrame += segment_length;

		if ((pFrame >pFrame_end) || (*pFrame++ != 0xFF) || (*pFrame++ != 0xDB))  {// DQT Cb/Cr
			break;
		}
		segment_length = (pFrame[0]<<8) + pFrame[1];
		assert(segment_length == 67);

		element_precision = pFrame[2] >> 4;
		assert(element_precision == 0);

		if ( 0 == element_precision) {
			fPrecision &= ~0x2;
			fQtableLength+= 64;
		} else if ( 1 == element_precision) {
			fPrecision |= 0x2;
			fQtableLength+= 64*2;
		} else {
			break;
		}
		for (int k=3; j <fQtableLength; j++,k++) {
			fQuantizationTables[j] = pFrame[k];
		}

		pFrame += segment_length;

		if ((pFrame > pFrame_end) || (*pFrame++ != 0xFF) || (*pFrame++ != 0xC0))  {// SOF0
			break;
		}
		segment_length = (pFrame[0]<<8) + pFrame[1];
		unsigned int i = 2;
		u_int8_t precision = pFrame[i++]; // 1 byte
		assert (precision == 8);
		fHeight = pFrame[i++];
		fHeight <<=8;
		fHeight += pFrame[i++];

		fWidth = pFrame[i++];
		fWidth <<=8;
		fWidth += pFrame[i++];
		u_int8_t component_num = pFrame[i++];
		assert (component_num == 3); //YUV

		u_int8_t component_id[3];
		u_int8_t sampling_factor[3];
		u_int8_t Qtable_num[3];
		for (int j = 0; j <component_num; j++) {
			component_id[j] = pFrame[i++];
			sampling_factor[j] = pFrame[i++];
			Qtable_num[j] = pFrame[i++];
		}
		if ((component_id[0] == 1 && sampling_factor[0] == 0x21 && Qtable_num[0] == 0) &&
			(component_id[1] == 2 && sampling_factor[1] == 0x11 && Qtable_num[1] == 1) &&
			(component_id[2] == 3 && sampling_factor[2] == 0x11 && Qtable_num[2] == 1)) {
			fType = 0;	// YUV 4:2:2
		} else if ((component_id[0] == 1 && sampling_factor[0] == 0x22 && Qtable_num[0] == 0) &&
			(component_id[1] == 2 && sampling_factor[1] == 0x11 && Qtable_num[1] == 1) &&
			(component_id[2] == 3 && sampling_factor[2] == 0x11 && Qtable_num[2] == 1)) {
			fType = 1;	// YUV 4:2:0
		} else {
			break;
		}
		assert(i == segment_length);
		pFrame += segment_length;

		for (int k=0; k<4; k++) {
			if ((pFrame > pFrame_end) || (*pFrame++ != 0xFF) || (*pFrame++ != 0xC4)) {// DHT Y-DC diff, Y-AC-Coef, Cb/Cr-DC diff, Cb/Cr-AC-Coef
				break;
			}
			segment_length = (pFrame[0]<<8) + pFrame[1];
			pFrame += segment_length;
		}
		if ((pFrame > pFrame_end) || (*pFrame++ != 0xFF) || (*pFrame++ != 0xDA)) {
			break;
		}
		segment_length = (pFrame[0]<<8) + pFrame[1];
		pFrame += segment_length;
		fFragmentLeftSize = pFrame_end -pFrame;
		fFragmentBeginAddr = pFrame;
		return True;
	}while(0);
	return False;
}

