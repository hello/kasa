#include <stdio.h>
#include <getopt.h>
#include <sched.h>
#include <pthread.h>
#include <signal.h>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "MyH264VideoStreamFramer.hh"
#include "MyH264VideoRTPSink.hh"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <basetypes.h>
#include "iav_ioctl.h"
#include "bsreader.h"

// To set up an internal RTSP server, uncomment the following:
//#define IMPLEMENT_RTSP_SERVER 1
// (Note that this RTSP server works for multicast only)
#define LIVE_STREAM "live stream"

UsageEnvironment* env;
UsageEnvironment* env_stream;
UsageEnvironment* env_stream2;

char const* stdInputFileName = "stdin";
static char inputFileName[256]= LIVE_STREAM;
static char inputFileName2[256]= "";

struct in_addr destinationAddress;
MyH264VideoStreamFramer* videoSource;
MyH264VideoStreamFramer* videoSource2;
MyH264VideoRTPSink* videoSink;
MyH264VideoRTPSink* videoSink2;
RTSPServer* rtspServer;


// options and usage
#define NO_ARG		0
#define HAS_ARG		1
static struct option long_options[] = {
	{0, 0, 0, 0}
};

static const char *short_options = "f:g:s";

void play();
void play2();
void stream();
void stream2();


int init_param(int argc, char **argv)
{
	int ch;
	int option_index = 0;

	opterr = 0;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (ch) {
		case 'f':
			strcpy(inputFileName, optarg);
			break;

		case 'g':
			strcpy(inputFileName2, optarg);
			break;

		case 's':
			strcpy(inputFileName, stdInputFileName);
			break;

		default:
			printf("unknown command %s \n", optarg);
			return -1;
			break;
		}
	}

	return 0;
}

void* schedule_stream(void *)
{
	*env << "Beginning to read from stream A...\n";
	videoSink->startPlaying(*videoSource, NULL, videoSink);	//must startPlaying before run sink scheduler

	env_stream->taskScheduler().doEventLoop();
	return NULL;
}

void* schedule_stream2(void *)
{
	*env << "Beginning to read from stream B...\n";
	videoSink2->startPlaying(*videoSource2, NULL, videoSink2);

	env_stream2->taskScheduler().doEventLoop(); //must startPlaying before run sink scheduler
	return NULL;
}

int bsreader()
{
	int fd_iav;
	if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return -1;
	}

	bsreader_init_data_t  init_data;
	memset(&init_data, 0, sizeof(init_data));
	init_data.fd_iav = fd_iav;
	init_data.max_stream_num = 4;
//	init_data.max_buffered_frame_num = 8;
	init_data.ring_buf_size[0] = 1024*1024*4;  //4MB
	init_data.ring_buf_size[1] = 1024*1024*2;  //2MB
	init_data.ring_buf_size[2] = 1024*1024*1;  //1MB
	init_data.ring_buf_size[3] = 1024*1024*1;  //1MB
	init_data.cavlc_possible = 1;		//enable cavlc encoding when configured
										//enable this option will use more memory in bsreader
	if (bsreader_init(&init_data) < 0) {
		printf("bsreader init failed \n");
		return -1;
	}

	if (bsreader_open() < 0) {
		printf("bsreader open failed \n");
		return -1;
	}
	return 0;
}

static void sighandler( int sig_no )
{
	exit(0);
}

int main(int argc, char** argv)
{
	int rval;
	pthread_t 	stream_thread_id;

	signal(SIGUSR1, sighandler);

	if (init_param(argc, argv) < 0) {
		printf("init param failed \n");
		return -1;
	}

	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);

 	// Create 'groupsocks' for RTP and RTCP:
//	destinationAddress.s_addr = chooseRandomIPv4SSMAddress(*env);
	destinationAddress.s_addr = 0x64DD03E8; //232.3.221.100
	*env << "dest_addr:" << inet_ntoa(destinationAddress) << "\n";

	rtspServer = RTSPServer::createNew(*env, 554);
	if(rtspServer == NULL)
	{
		*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
		exit(1);
	}
	bsreader();

	stream();
//	stream2();

	rval = pthread_create(&stream_thread_id, NULL, schedule_stream, NULL);
	if(rval != 0) {
		perror("Create pthread error!\n");
		return -1;
	}
//	rval = pthread_create(&stream2_thread_id, NULL, schedule_stream2, NULL);
//	if(rval != 0) {
//		perror("Create pthread error!\n");
//		return -1;
//	}

	env->taskScheduler().doEventLoop(); // does not return
	pthread_join(stream_thread_id, NULL);
//	pthread_join(stream2_thread_id, NULL);
	return 0; // only to prevent compiler warning
}


void stream()
{
	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	env_stream  = BasicUsageEnvironment::createNew(*scheduler);

	const unsigned short rtpPortNum = 50000;
	const unsigned short rtcpPortNum = rtpPortNum+1;
	const unsigned char ttl = 255;

	const Port rtpPort(rtpPortNum);
	const Port rtcpPort(rtcpPortNum);

	static Groupsock rtpGroupsock(*env, destinationAddress, rtpPort, ttl);
	rtpGroupsock.multicastSendOnly(); // we're a SSM source
	static Groupsock rtcpGroupsock(*env, destinationAddress, rtcpPort, ttl);
	rtcpGroupsock.multicastSendOnly(); // we're a SSM source

	// Create a 'H264 Video RTP' sink from the RTP 'groupsock':
	videoSink = MyH264VideoRTPSink::createNew(*env_stream, &rtpGroupsock, 96 ,0X42,"h264");
//	videoSink->createFifo();

	// Create (and start) a 'RTCP instance' for this RTP sink:
	const unsigned estimatedSessionBandwidth = 500; // in kbps; for RTCP b/w share
	const unsigned maxCNAMElen = 100;
	unsigned char CNAME[maxCNAMElen+1];
	gethostname((char*)CNAME, maxCNAMElen);
	CNAME[maxCNAMElen] = '\0'; // just in case
	RTCPInstance* rtcp
	= RTCPInstance::createNew(*env, &rtcpGroupsock,
				  estimatedSessionBandwidth, CNAME,
				  videoSink, NULL /* we're a server */,
				  True /* we're a SSM source */);
	// Note: This starts RTCP running automatically

	ServerMediaSession* sms
		= ServerMediaSession::createNew(*env, "stream1", inputFileName,
			"Session streamed by \"testH264VideoStreamer\"",
			True /*SSM*/);
	sms->addSubsession(PassiveServerMediaSubsession::createNew(*videoSink, rtcp));
	rtspServer->addServerMediaSession(sms);

	char* url = rtspServer->rtspURL(sms);
	*env << "Play this stream using the URL \"" << url << "\"\n";
	delete[] url;

	// Start the streaming:
	*env << "Beginning streaming...\n";

	play();
}

void stream2()
{
	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	env_stream2  = BasicUsageEnvironment::createNew(*scheduler);

	const unsigned short rtpPortNum = 50010;
	const unsigned short rtcpPortNum = rtpPortNum+1;
	const unsigned char ttl = 255;

	const Port rtpPort(rtpPortNum);
	const Port rtcpPort(rtcpPortNum);

	static Groupsock rtpGroupsock(*env, destinationAddress, rtpPort, ttl);
	rtpGroupsock.multicastSendOnly(); // we're a SSM source
	static Groupsock rtcpGroupsock(*env, destinationAddress, rtcpPort, ttl);
	rtcpGroupsock.multicastSendOnly(); // we're a SSM source

	// Create a 'H264 Video RTP' sink from the RTP 'groupsock':
	videoSink2 = MyH264VideoRTPSink::createNew(*env_stream2, &rtpGroupsock, 96 ,0X42,"h264");

	// Create (and start) a 'RTCP instance' for this RTP sink:
	const unsigned estimatedSessionBandwidth = 500; // in kbps; for RTCP b/w share
	const unsigned maxCNAMElen = 100;
	unsigned char CNAME[maxCNAMElen+1];
	gethostname((char*)CNAME, maxCNAMElen);
	CNAME[maxCNAMElen] = '\0'; // just in case
	RTCPInstance* rtcp
	= RTCPInstance::createNew(*env, &rtcpGroupsock,
				  estimatedSessionBandwidth, CNAME,
				  videoSink2, NULL /* we're a server */,
				  True /* we're a SSM source */);
	// Note: This starts RTCP running automatically


	ServerMediaSession* sms
		= ServerMediaSession::createNew(*env, "stream2", inputFileName2,
			"Session streamed by \"testH264VideoStreamer\"",
			True /*SSM*/);
	sms->addSubsession(PassiveServerMediaSubsession::createNew(*videoSink2, rtcp));
	rtspServer->addServerMediaSession(sms);

	char* url = rtspServer->rtspURL(sms);
	*env << "Play this stream using the URL \"" << url << "\"\n";
	delete[] url;

	// Start the streaming:
	*env << "Beginning streaming...\n";

	play2();
}

void play()
{
	if (!strncmp (inputFileName, LIVE_STREAM, strlen(LIVE_STREAM))) {
		// Create a framer for Stream A
		videoSource = MyH264VideoStreamFramer::createNew(*env, 0);
	} else {
		// Open the input file as a 'byte-stream file source':
		ByteStreamFileSource* fileSource
		= ByteStreamFileSource::createNew(*env_stream, inputFileName);
		if (fileSource == NULL) {
		*env << "Unable to open file \"" << inputFileName
		 << "\" as a byte-stream file source\n";
		exit(1);
		}
		FramedSource* videoES = fileSource;
		// Create a framer for file Stream
		videoSource = MyH264VideoStreamFramer::createNew(*env,videoES);
	}

}

void play2()
{
	// Create a framer for Stream B
	videoSource2 = MyH264VideoStreamFramer::createNew(*env, 1);
}


