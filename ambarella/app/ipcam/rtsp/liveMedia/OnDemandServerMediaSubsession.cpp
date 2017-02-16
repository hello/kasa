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
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand.
// Implementation

#include "OnDemandServerMediaSubsession.hh"
#include "RTCP.hh"
#include "BasicUDPSink.hh"
#include <GroupsockHelper.hh>

OnDemandServerMediaSubsession
::OnDemandServerMediaSubsession(UsageEnvironment& env,
				Boolean reuseFirstSource,
				portNumBits initialPortNum,
				netAddressBits multicastDest)
	: ServerMediaSubsession(env),
	  fSDPLines(NULL), fReuseFirstSource(reuseFirstSource), fInitialPortNum(initialPortNum), 
	  fLastStreamToken(NULL), multicastAddress(multicastDest) {
	fDestinationsHashTable = HashTable::create(ONE_WORD_HASH_KEYS);
	gethostname(fCNAME, sizeof fCNAME);
	fCNAME[sizeof fCNAME-1] = '\0'; // just in case
}

class Destinations {
public:
  Destinations(struct in_addr const& destAddr,
				 Port const& rtpDestPort,
				 Port const& rtcpDestPort)
		: isTCP(False), addr(destAddr), rtpPort(rtpDestPort), rtcpPort(rtcpDestPort) {
	}
  Destinations(int tcpSockNum, unsigned char rtpChanId, unsigned char rtcpChanId)
		: isTCP(True), rtpPort(0) /*dummy*/, rtcpPort(0) /*dummy*/,
		  tcpSocketNum(tcpSockNum), rtpChannelId(rtpChanId), rtcpChannelId(rtcpChanId) {
	}

public:
  Boolean isTCP;
  struct in_addr addr;
  Port rtpPort;
  Port rtcpPort;
  int tcpSocketNum;
  unsigned char rtpChannelId, rtcpChannelId;
};

OnDemandServerMediaSubsession::~OnDemandServerMediaSubsession() {
  delete[] fSDPLines;

	// Clean out the destinations hash table:
  while (1) {
	  Destinations* destinations
			= (Destinations*)(fDestinationsHashTable->RemoveNext());
	  if (destinations == NULL) break;
	  delete destinations;
	}
  delete fDestinationsHashTable;
}

char const*
OnDemandServerMediaSubsession::sdpLines() {
  if (fSDPLines == NULL) {
		// We need to construct a set of SDP lines that describe this
		// subsession (as a unicast stream).  To do so, we first create
		// dummy (unused) source and "RTPSink" objects,
		// whose parameters we use for the SDP lines:
	  unsigned estBitrate;
	  FramedSource* inputSource = createNewStreamSource(0, estBitrate);
	  if (inputSource == NULL) return NULL; // file not found

	  struct in_addr dummyAddr;
	  dummyAddr.s_addr = 0;
	  Groupsock dummyGroupsock(envir(), dummyAddr, 0, 0);
	  unsigned char rtpPayloadType = 96 + trackNumber()-1; // if dynamic
	  RTPSink* dummyRTPSink
			= createNewRTPSink(&dummyGroupsock, rtpPayloadType, inputSource);

	  setSDPLinesFromRTPSink(dummyRTPSink, inputSource, estBitrate);
	  Medium::close(dummyRTPSink);
	  closeStreamSource(inputSource);
	}

  return fSDPLines;
}

// A class that represents the state of an ongoing stream
class StreamState {
public:
  StreamState(OnDemandServerMediaSubsession& master,
						  Port const& serverRTPPort, Port const& serverRTCPPort,
			  RTPSink* rtpSink, BasicUDPSink* udpSink,
			  unsigned totalBW, FramedSource* mediaSource,
			  Groupsock* rtpGS, Groupsock* rtcpGS, UsageEnvironment& fRtcpEnv);
	virtual ~StreamState();

	void startPlaying(Destinations* destinations,
			  TaskFunc* rtcpRRHandler, void* rtcpRRHandlerClientData);
	Boolean isPlaying() { return fAreCurrentlyPlaying; }
	
	void pause();
	void endPlaying(Destinations* destinations);
	void reclaim();

	unsigned& referenceCount() { return fReferenceCount; }

	Port const& serverRTPPort() const { return fServerRTPPort; }
	Port const& serverRTCPPort() const { return fServerRTCPPort; }

	RTPSink* rtpSink() const { return fRTPSink; }

	float streamDuration() const { return fStreamDuration; }

	FramedSource* mediaSource() const { return fMediaSource; }
	
	UsageEnvironment& envir() const {return fMaster.envir(); }
	int createWorkingThread();
	int cancelWorkingThread();
	

private:	
	static void* workingThreadFunc(void *);
	
private:
	OnDemandServerMediaSubsession& fMaster;
	Boolean fAreCurrentlyPlaying;
	unsigned fReferenceCount;

	Port fServerRTPPort, fServerRTCPPort;

	RTPSink* fRTPSink;
	BasicUDPSink* fUDPSink;

	float fStreamDuration;
	unsigned fTotalBW; RTCPInstance* fRTCPInstance;

	FramedSource* fMediaSource;

	Groupsock* fRTPgs; Groupsock* fRTCPgs;
	UsageEnvironment& fRtcpEnv;

	pthread_t fWorkingThreadID;
};

void OnDemandServerMediaSubsession
::getStreamParameters(unsigned clientSessionId,
				  netAddressBits clientAddress,
				  Port const& clientRTPPort,
				  Port const& clientRTCPPort,
				  int tcpSocketNum,
				  unsigned char rtpChannelId,
				  unsigned char rtcpChannelId,
				  netAddressBits& destinationAddress,
				  u_int8_t& /*destinationTTL*/,
				  Boolean& isMulticast,
				  Port& serverRTPPort,
				  Port& serverRTCPPort,
				  void*& streamToken) {

	if (isMulticast) {
		destinationAddress = multicastAddress;
		printf("client requests mulicast, port %d\n", fInitialPortNum);	//jay
	} else if (destinationAddress == 0) {
		destinationAddress = clientAddress;
		printf("client requests unicast 0x%x\n", clientAddress);	//jay
	}

  	struct in_addr destinationAddr; destinationAddr.s_addr = destinationAddress;

  if (fLastStreamToken != NULL && fReuseFirstSource) {
		// Special case: Rather than creating a new 'StreamState',
		// we reuse the one that we've already created:
	  serverRTPPort = ((StreamState*)fLastStreamToken)->serverRTPPort();
	  serverRTCPPort = ((StreamState*)fLastStreamToken)->serverRTCPPort();
		++((StreamState*)fLastStreamToken)->referenceCount();
	  streamToken = fLastStreamToken;
	} else {
		// Normal case: Create a new media source:
	  unsigned streamBitrate;
	  FramedSource* mediaSource
			= createNewStreamSource(clientSessionId, streamBitrate);

		// Create 'groupsock' and 'sink' objects for the destination,
		// using previously unused server port numbers:
	  RTPSink* rtpSink;
	  BasicUDPSink* udpSink;
	  Groupsock* rtpGroupsock;
	  Groupsock* rtcpGroupsock;
	  portNumBits serverPortNum;
	  if (clientRTCPPort.num() == 0) {
			// We're streaming raw UDP (not RTP). Create a single groupsock:
		NoReuse dummy; // ensures that we skip over ports that are already in use
		  
		for (serverPortNum = fInitialPortNum; ; ++serverPortNum) {
			struct in_addr dummyAddr; dummyAddr.s_addr = 0;

			serverRTPPort = serverPortNum;
			rtpGroupsock = new Groupsock(envir(), dummyAddr, serverRTPPort, 255);
			if (rtpGroupsock->socketNum() >= 0) break; // success
		}

		rtcpGroupsock = NULL;
		rtpSink = NULL;
		udpSink = BasicUDPSink::createNew(envir(), rtpGroupsock);
	} else {
		// Normal case: We're streaming RTP (over UDP or TCP).  Create a pair of
		// groupsocks (RTP and RTCP), with adjacent port numbers (RTP port number even):
		NoReuse dummy; // ensures that we skip over ports that are already in use
		for (portNumBits serverPortNum = fInitialPortNum; ; serverPortNum += 2) {
			struct in_addr dummyAddr; dummyAddr.s_addr = 0;
			
			serverRTPPort = serverPortNum;
			rtpGroupsock = new Groupsock(envir(), dummyAddr, serverRTPPort, 255);
			if (rtpGroupsock->socketNum() < 0) {
				delete rtpGroupsock;
				continue; // try again
			}

			serverRTCPPort = serverPortNum+1;
			// envir for rtcp is the same as rtsp server
			rtcpGroupsock = new Groupsock(envir(), dummyAddr, serverRTCPPort, 255);
			if (rtcpGroupsock->socketNum() < 0) {
				delete rtpGroupsock;
				delete rtcpGroupsock;
				continue; // try again
			}

			break; // success
		}

		  unsigned char rtpPayloadType = 96 + trackNumber()-1; // if dynamic
		  rtpSink = createNewRTPSink(rtpGroupsock, rtpPayloadType, mediaSource);
//		  rtpSink->createFifo();
		  udpSink = NULL;
	}

		// Turn off the destinations for each groupsock.  They'll get set later
		// (unless TCP is used instead):
	  if (rtpGroupsock != NULL) rtpGroupsock->removeAllDestinations();
	  if (rtcpGroupsock != NULL) rtcpGroupsock->removeAllDestinations();

	  if (rtpGroupsock != NULL) {
			// Try to use a big send buffer for RTP -  at least 0.1 second of
			// specified bandwidth and at least 50 KB
		  unsigned rtpBufSize = streamBitrate * 25 / 2; // 1 kbps * 0.1 s = 12.5 bytes
		  if (rtpBufSize < 50 * 1024) rtpBufSize = 50 * 1024;
		  increaseSendBufferTo(envir(), rtpGroupsock->socketNum(), rtpBufSize);
		}

		// Set up the state of the stream.  The stream will get started later:
	  streamToken = fLastStreamToken
			= new StreamState(*this, serverRTPPort, serverRTCPPort, rtpSink, udpSink,
			streamBitrate, mediaSource,
			rtpGroupsock, rtcpGroupsock,
			fParentSession->envir());
	}


	// Record these destinations as being for this client session id:
	Destinations* destinations;
	if (tcpSocketNum < 0) { // UDP
		if (isMulticast) {
			//multicast server will always send RTP/RTCP packet to the same port as its local port
			Port MultiClientRTPPort(serverRTPPort);	
			Port MultiClientRTCPPort(serverRTCPPort);
			destinations = new Destinations(destinationAddr, MultiClientRTPPort, MultiClientRTCPPort);
		} else {
			destinations = new Destinations(destinationAddr, clientRTPPort, clientRTCPPort);
		}
	} else { // TCP
	  destinations = new Destinations(tcpSocketNum, rtpChannelId, rtcpChannelId);
	}
  fDestinationsHashTable->Add((char const*)clientSessionId, destinations);
  
}

void OnDemandServerMediaSubsession::startStream(unsigned clientSessionId,
						void* streamToken,
						TaskFunc* rtcpRRHandler,
						void* rtcpRRHandlerClientData,
						unsigned short& rtpSeqNum,
						unsigned& rtpTimestamp) {
  StreamState* streamState = (StreamState*)streamToken;
  printf("startStream\n");
  Destinations* destinations
		= (Destinations*)(fDestinationsHashTable->Lookup((char const*)clientSessionId));
  if (streamState != NULL) {
	if (streamState->rtpSink() != NULL) {
		rtpSeqNum = streamState->rtpSink()->currentSeqNo();
		if (streamState->isPlaying())
			rtpTimestamp = streamState->rtpSink()->currentTimeStamp();
		else
			rtpTimestamp = streamState->rtpSink()->presetNextTimestamp();
		}
	}
  	streamState->startPlaying(destinations,
		  rtcpRRHandler, rtcpRRHandlerClientData);
}

void OnDemandServerMediaSubsession::pauseStream(unsigned /*clientSessionId*/,
						void* streamToken) {
	// Pausing isn't allowed if multiple clients are receiving data from
	// the same source:
  if (fReuseFirstSource) return;

  StreamState* streamState = (StreamState*)streamToken;
  if (streamState != NULL) streamState->pause();
}

void OnDemandServerMediaSubsession::seekStream(unsigned /*clientSessionId*/,
								 void* streamToken, double seekNPT) {
	// Seeking isn't allowed if multiple clients are receiving data from
	// the same source:
  if (fReuseFirstSource) return;

  StreamState* streamState = (StreamState*)streamToken;
  if (streamState != NULL && streamState->mediaSource() != NULL) {
	  seekStreamSource(streamState->mediaSource(), seekNPT);
	}
}

void OnDemandServerMediaSubsession::setStreamScale(unsigned /*clientSessionId*/,
							 void* streamToken, float scale) {
	// Changing the scale factor isn't allowed if multiple clients are receiving data
	// from the same source:
  if (fReuseFirstSource) return;

  StreamState* streamState = (StreamState*)streamToken;
  if (streamState != NULL && streamState->mediaSource() != NULL) {
	  setStreamSourceScale(streamState->mediaSource(), scale);
	}
}

void OnDemandServerMediaSubsession::deleteStream(unsigned clientSessionId,
						 void*& streamToken) {
  StreamState* streamState = (StreamState*)streamToken;
  printf("deleteStream...\n");

	// Look up (and remove) the destinations for this client session:
  Destinations* destinations
		= (Destinations*)(fDestinationsHashTable->Lookup((char const*)clientSessionId));
  if (destinations != NULL) {
	  fDestinationsHashTable->Remove((char const*)clientSessionId);

		// Stop streaming to these destinations:
	  if (streamState != NULL) streamState->endPlaying(destinations);
	}

	// Delete the "StreamState" structure if it's no longer being used:
  if (streamState != NULL) {
	  if (streamState->referenceCount() > 0) --streamState->referenceCount();
	  if (streamState->referenceCount() == 0) {
	//  		envir().taskScheduler().pauseTask();
		  delete streamState;
		  if (fLastStreamToken == streamToken) fLastStreamToken = NULL;
		  streamToken = NULL;
		}
	}

	// Finally, delete the destinations themselves:
  delete destinations;
}

char const* OnDemandServerMediaSubsession
::getAuxSDPLine(RTPSink* rtpSink, FramedSource* /*inputSource*/) {
	// Default implementation:
  return rtpSink == NULL ? NULL : rtpSink->auxSDPLine();
}

void OnDemandServerMediaSubsession::seekStreamSource(FramedSource* /*inputSource*/,
								 double /*seekNPT*/) {
	// Default implementation: Do nothing
}

void OnDemandServerMediaSubsession
::setStreamSourceScale(FramedSource* /*inputSource*/, float /*scale*/) {
	// Default implementation: Do nothing
}

void OnDemandServerMediaSubsession::closeStreamSource(FramedSource *inputSource) {
  Medium::close(inputSource);
}

void OnDemandServerMediaSubsession
::setSDPLinesFromRTPSink(RTPSink* rtpSink, FramedSource* inputSource, unsigned estBitrate) {
  if (rtpSink == NULL) return;

  char const* mediaType = rtpSink->sdpMediaType();
  unsigned char rtpPayloadType = rtpSink->rtpPayloadType();
  struct in_addr serverAddrForSDP; serverAddrForSDP.s_addr = fServerAddressForSDP;
  char* const ipAddressStr = strDup(our_inet_ntoa(serverAddrForSDP));
  char* rtpmapLine = rtpSink->rtpmapLine();
  char const* rangeLine = rangeSDPLine();
  char const* auxSDPLine = getAuxSDPLine(rtpSink, inputSource);
  if (auxSDPLine == NULL) auxSDPLine = "";

  char const* const sdpFmt =
		"m=%s %u RTP/AVP %d\r\n"
		"c=IN IP4 %s\r\n"
		"b=AS:%u\r\n"
		"%s"
		"%s"
		"%s"
		"a=control:%s\r\n";
  unsigned sdpFmtSize = strlen(sdpFmt)
		+ strlen(mediaType) + 5 /* max short len */ + 3 /* max char len */
		+ strlen(ipAddressStr)
		+ 20 /* max int len */
		+ strlen(rtpmapLine)
		+ strlen(rangeLine)
		+ strlen(auxSDPLine)
		+ strlen(trackId());
  char* sdpLines = new char[sdpFmtSize];
  sprintf(sdpLines, sdpFmt,
	  mediaType, // m= <media>
	  fPortNumForSDP, // m= <port>
	  rtpPayloadType, // m= <fmt list>
	  ipAddressStr, // c= address
	  estBitrate, // b=AS:<bandwidth>
	  rtpmapLine, // a=rtpmap:... (if present)
	  rangeLine, // a=range:... (if present)
	  auxSDPLine, // optional extra SDP line
	  trackId()); // a=control:<track-id>
  delete[] (char*)rangeLine; delete[] rtpmapLine; delete[] ipAddressStr;

  fSDPLines = strDup(sdpLines);
  delete[] sdpLines;
}


////////// StreamState implementation //////////

static void afterPlayingStreamState(void* clientData) {
  StreamState* streamState = (StreamState*)clientData;
  if (streamState->streamDuration() == 0.0) {
		// When the input stream ends, tear it down.  This will cause a RTCP "BYE"
		// to be sent to each client, teling it that the stream has ended.
		// (Because the stream didn't have a known duration, there was no other
		//  way for clients to know when the stream ended.)
	  streamState->reclaim();
	}
	// Otherwise, keep the stream alive, in case a client wants to
	// subsequently re-play the stream starting from somewhere other than the end.
	// (This can be done only on streams that have a known duration.)
}

StreamState::StreamState(OnDemandServerMediaSubsession& master,
												 Port const& serverRTPPort, Port const& serverRTCPPort,
			 RTPSink* rtpSink, BasicUDPSink* udpSink,
			 unsigned totalBW, FramedSource* mediaSource,
			 Groupsock* rtpGS, Groupsock* rtcpGS, UsageEnvironment& rtcpEnv)
	: fMaster(master), fAreCurrentlyPlaying(False), fReferenceCount(1),
	  fServerRTPPort(serverRTPPort), fServerRTCPPort(serverRTCPPort),
	  fRTPSink(rtpSink), fUDPSink(udpSink), fStreamDuration(master.duration()),
	  fTotalBW(totalBW), fRTCPInstance(NULL) /* created later */,
	  fMediaSource(mediaSource), fRTPgs(rtpGS), fRTCPgs(rtcpGS),
	  fRtcpEnv(rtcpEnv){
}

StreamState::~StreamState() {
	printf("~StreamState\n");
  reclaim();
}

static int set_realtime_schedule(pthread_t thread_id)
{
	struct sched_param param;
	int policy = SCHED_FIFO;
	int priority = 90;
	if (!thread_id)
		return -1;
	memset(&param, 0, sizeof(param));
	param.sched_priority = priority;
	if (pthread_setschedparam(thread_id, policy, &param) < 0)
		perror("pthread_setschedparam");
	pthread_getschedparam(thread_id, &policy, &param);
	if (param.sched_priority != priority)
		return -1;
	return 0;
}

void* StreamState::workingThreadFunc(void* arg) 
{
	StreamState *streamState = (StreamState *)arg;
	printf("scheduler %p looping...\n", &(streamState->envir().taskScheduler()));
	streamState->envir().taskScheduler().doEventLoop();
	
	return NULL;
}

int StreamState::createWorkingThread()
{
	if (pthread_create(&fWorkingThreadID, NULL, workingThreadFunc, this) < 0) {
		printf("createWorkingThread error\n");
		return -1;
	}
	if (set_realtime_schedule(fWorkingThreadID) < 0) {
		printf("set realtime schedule error\n");
		return -1;
	}
	return 0;
}

int StreamState::cancelWorkingThread()
{
//	printf("cancel stream Thread...\n");	//jay
	pthread_cancel(fWorkingThreadID);
	pthread_join(fWorkingThreadID, NULL);
//	printf("cancel done\n");
	return 0;
}

void StreamState
::startPlaying(Destinations* dests,
				 TaskFunc* rtcpRRHandler, void* rtcpRRHandlerClientData) {
	if (dests == NULL) return;
	
	//skip play action if it's already being played
	if (!fAreCurrentlyPlaying && fMediaSource != NULL) {
		if (fRTPSink != NULL) {
			fRTPSink->startPlaying(*fMediaSource, afterPlayingStreamState, this);
			fAreCurrentlyPlaying = True;
		} else if (fUDPSink != NULL) {
			fUDPSink->startPlaying(*fMediaSource, afterPlayingStreamState, this);
			fAreCurrentlyPlaying = True;
		}
		createWorkingThread();
	}

	if (fRTCPInstance == NULL && fRTPSink != NULL) {
		// Create (and start) a 'RTCP instance' for this RTP sink:
		fRTCPInstance
	//		= RTCPInstance::createNew(fRTPSink->envir(), fRTCPgs,
			= RTCPInstance::createNew(fRtcpEnv, fRTCPgs,
			fTotalBW, (unsigned char*)fMaster.fCNAME,
			fRTPSink, NULL /* we're a server */);
		// Note: This starts RTCP running automatically
	}

	if (dests->isTCP) {
		// Change RTP and RTCP to use the TCP socket instead of UDP:
		if (fRTPSink != NULL) {
			fRTPSink->addStreamSocket(dests->tcpSocketNum, dests->rtpChannelId);
		}
		if (fRTCPInstance != NULL) {
			fRTCPInstance->addStreamSocket(dests->tcpSocketNum, dests->rtcpChannelId);
			fRTCPInstance->setSpecificRRHandler(dests->tcpSocketNum, dests->rtcpChannelId,
				rtcpRRHandler, rtcpRRHandlerClientData);
		}
	} else {
		// Tell the RTP and RTCP 'groupsocks' about this destination
		// (in case they don't already have it):
		//	printf("addDestination %s\n", inet_ntoa(dests->addr));	//jay
		if (fRTPgs != NULL) fRTPgs->addDestination(dests->addr, dests->rtpPort);
		if (fRTCPgs != NULL) fRTCPgs->addDestination(dests->addr, dests->rtcpPort);
		if (fRTCPInstance != NULL) {
			if (!IsMulticastAddress(dests->addr.s_addr)) {
				fRTCPInstance->setSpecificRRHandler(dests->addr.s_addr, dests->rtcpPort,
				rtcpRRHandler, rtcpRRHandlerClientData);
			}
		}
	}
}

void StreamState::pause() {
  if (fRTPSink != NULL) fRTPSink->stopPlaying();
  if (fUDPSink != NULL) fUDPSink->stopPlaying();
  fAreCurrentlyPlaying = False;
}

void StreamState::endPlaying(Destinations* dests) {
  if (dests->isTCP) {
	  if (fRTPSink != NULL) {
		  fRTPSink->removeStreamSocket(dests->tcpSocketNum, dests->rtpChannelId);
		}
	  if (fRTCPInstance != NULL) {
		  fRTCPInstance->removeStreamSocket(dests->tcpSocketNum, dests->rtcpChannelId);
		  fRTCPInstance->setSpecificRRHandler(dests->tcpSocketNum, dests->rtcpChannelId,
					  NULL, NULL);
		}
	} else {
		// Tell the RTP and RTCP 'groupsocks' to stop using these destinations:
	  if (fRTPgs != NULL) fRTPgs->removeDestination(dests->addr, dests->rtpPort);
	  if (fRTCPgs != NULL) fRTCPgs->removeDestination(dests->addr, dests->rtcpPort);
	  if (fRTCPInstance != NULL) {
		  fRTCPInstance->setSpecificRRHandler(dests->addr.s_addr, dests->rtcpPort,
					  NULL, NULL);
		}
	}
}

void StreamState::reclaim() {
	// Delete allocated media objects	
	cancelWorkingThread();
	Medium::close(fRTCPInstance) /* will send a RTCP BYE */; fRTCPInstance = NULL;
	Medium::close(fRTPSink); fRTPSink = NULL;
	Medium::close(fUDPSink); fUDPSink = NULL;
	
	fMaster.closeStreamSource(fMediaSource); fMediaSource = NULL;

	delete fRTPgs; fRTPgs = NULL;
	delete fRTCPgs; fRTCPgs = NULL;
}
