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
// File sinks
// Implementation

#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "livemedia.hh"

#include "AmbaRtspRecv.h"
#include "AmbaVideoSink.h"

#include <strsafe.h>
void TRACE(STRSAFE_LPCSTR fmt, ...);
////////// CAmbaVideoSink //////////

CAmbaVideoSink::CAmbaVideoSink(UsageEnvironment& env,CAmbaRtspRecv* pAmbaRtspRecv, unsigned bufferSize)
  : MediaSink(env),fBufferSize(bufferSize) {
	m_ambaRtspRecv = pAmbaRtspRecv;
	fBuffer = new unsigned char[bufferSize];
}

CAmbaVideoSink::~CAmbaVideoSink() {
  delete[] fBuffer;
}

CAmbaVideoSink* CAmbaVideoSink::createNew(UsageEnvironment& env,CAmbaRtspRecv* pAmbaRtspRecv,
			      unsigned bufferSize) {
	do {
		return new CAmbaVideoSink(env, pAmbaRtspRecv, bufferSize);
	} while (0);
	return NULL;
}

Boolean CAmbaVideoSink::continuePlaying() {
	if (fSource == NULL)
		return False;
	fSource->getNextFrame(fBuffer, fBufferSize,
		afterGettingFrame, this,
		onSourceClosure, this);
	return True;
}

void CAmbaVideoSink::afterGettingFrame(void* clientData, unsigned frameSize,
				 unsigned /*numTruncatedBytes*/,
				 struct timeval presentationTime,
				 unsigned /*durationInMicroseconds*/) {
	CAmbaVideoSink* sink = (CAmbaVideoSink*)clientData;
	sink->afterGettingFrame1(frameSize, presentationTime);
}

void CAmbaVideoSink::addData(unsigned char* data, unsigned dataSize,
		       struct timeval presentationTime) {
	static int cnt = 0;
//	TRACE(L"addData: %d\n",GetCurrentThreadId());
	if (data != NULL) {
		m_ambaRtspRecv->add_data(data, dataSize, presentationTime);
		SwitchToThread();
	}
}

void CAmbaVideoSink::afterGettingFrame1(unsigned frameSize,
				  struct timeval presentationTime) {
	addData(fBuffer, frameSize, presentationTime);
	/*if (fOutFid == NULL || fflush(fOutFid) == EOF) {
	// The output file has closed.  Handle this the same way as if the
	// input source had closed:
	onSourceClosure(this);

	stopPlaying();
	return;
	}*/
  	// Then try getting the next frame:
	continuePlaying();
}


CAmbaH264VideoSink::CAmbaH264VideoSink(UsageEnvironment& env, 
	CAmbaRtspRecv* pAmbaRtspRecv, unsigned bufferSize)
	: CAmbaVideoSink(env, pAmbaRtspRecv,bufferSize) {
}

CAmbaH264VideoSink::~CAmbaH264VideoSink() {
}

CAmbaH264VideoSink* CAmbaH264VideoSink::createNew(UsageEnvironment& env, 
			CAmbaRtspRecv* pAmbaRtspRecv, unsigned bufferSize) {
	do {
		return new CAmbaH264VideoSink(env, pAmbaRtspRecv, bufferSize);
	} while (0);
	return NULL;
}

Boolean CAmbaH264VideoSink::sourceIsCompatibleWithUs(MediaSource& source) {
  // Just return true, should be checking for H.264 video streams though
    return True;
}

void CAmbaH264VideoSink::afterGettingFrame1(unsigned frameSize,
					  struct timeval presentationTime) {
  unsigned char start_code[4] = {0x00, 0x00, 0x00, 0x01};
	addData(start_code, 4, presentationTime);
	// Call the parent class to complete the normal file write with the input data:
	CAmbaVideoSink::afterGettingFrame1(frameSize, presentationTime);
}

