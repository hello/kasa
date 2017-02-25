#ifndef __AMBA_RTSP_PLAY_H__
#define __AMBA_RTSP_PLAY_H__

#include "AmbaComm.h"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "livemedia.hh"

class qosMeasurementRecord;
class CAmbaRtspRecv;
	
typedef struct MediaSubsession_s {
	MediaSubsession* pMediaSubsession;
	CAmbaRtspRecv* pAmbaRtspRecv;
}MediaSubsession_t;

#define READER_NUM 2
class CDataProcess
{
public:
	struct data_unit
	{
		unsigned char* start_addr;
		SLICE_INFO slice_info;
		bool read_flag[READER_NUM]; //true -- reader1 not fetch out data 
	};
public:
	CDataProcess();
	~CDataProcess();
	void set_databuffer_size(unsigned int size){
		m_dataBufferSize = size;
	};
	int init();
	int stop();
	int closeEvent();
	int add_to_queue(unsigned char* pData, const SLICE_INFO* pInfo);
	int get_from_queue(unsigned char* pData, SLICE_INFO* pInfo, int reader_idx = 0);

private:
	int m_dataBufferSize;
	unsigned char* m_dataBuffer;
	data_unit* m_queue;
	int m_write_cursor; // write_cursor
	int m_next_read_cursor[READER_NUM];
	int m_being_read_cursor[READER_NUM]; // -1:not being read
	HANDLE m_wEvent[READER_NUM];
	HANDLE m_rEvent[READER_NUM];
	bool m_stopped;

	void workaround_for_cursor(int& cursor);
};

class CAmbaRtspRecv 
{
struct stat_data
{
	unsigned int frame_cnt;
	int totle_size;
	long long totle_pts;
	long long last_pts;
	DWORD begin_time;
	DWORD *old_time;
	int *old_size;
	long long *old_pts;
	unsigned int window_size;
};

public:
	CAmbaRtspRecv();
	~CAmbaRtspRecv();
private:
	char* getSDPDescriptionFromURL(Medium* client, char const* url,
				char const* username, char const* password,
			       char const* /*proxyServerName*/,
			       unsigned short /*proxyServerPortNum*/,
			       unsigned short /*clientStartPort*/) ;
	Medium* createClient(UsageEnvironment& env,
				int verbosityLevel, char const* applicationName) ;

	void printQOSData();
	void closeMediaSinks();
	void tearDownStreams();
	Boolean clientTearDownSession(Medium* client,  MediaSession* session) ;
	Boolean setupStreams(short transfertype);
	Boolean startPlayingStreams();
	Boolean clientSetupSubsession(Medium* client,
			MediaSubsession* subsession,
			short transfertype);

	void beginQOSMeasurement();

	//void periodicQOSMeasurement(void* /*clientData*/);
	//void sessionTimerHandler(void* /*clientData*/);
	Boolean clientStartPlayingSession(Medium* client,MediaSession* session);

	bool check_pts();

	UsageEnvironment* m_env;
	Medium* m_ourClient;
	
	TaskToken m_arrivalCheckTimerTask;
	TaskToken m_interPacketGapCheckTimerTask;
	TaskToken m_qosMeasurementTimerTask;
	unsigned m_qosMeasurementIntervalMS; // 0 means: Don't output QOS data
	MediaSession* m_session;
	unsigned m_fileSinkBufferSize;
	unsigned m_nextQOSMeasurementUSecs;

	double m_duration;
	double m_scale;
	double m_initialSeekTime;

	//struct timeval m_lastTime;
	struct timeval m_beginTime;
	unsigned char* m_buf;
	/*int m_size;
	short m_nal_unit_type;
	long long m_sample_delta;*/
	SLICE_INFO m_info;
	CDataProcess* m_dataProcess;

	stat_data m_stat;
	
	char m_watch;
public:
	void sessionAfterPlaying(void* clientData = NULL);
	void scheduleNextQOSMeasurement();
	static void subsessionAfterPlaying(void* clientData) ;
	static void subsessionByeHandler(void* clientData);
	static void sessionTimerHandler(void* clientData); 		
	static void periodicQOSMeasurement(void* clientData); 

	struct timeval m_startTime;
	TaskToken m_sessionTimerTask;
	qosMeasurementRecord* m_qosRecordHead;

	int setup(const char* url, const short transfertype, const short buffersize = 0);
	void play();
	void stop();
	void shutdown();
	void add_data(unsigned char* data, unsigned dataSize, struct timeval presentationTime);
	int fetch_data(unsigned char* pData, SLICE_INFO* pInfo, int reader_idx);
	ENC_TYPE get_encode_type(){
		return m_encType;
	};
	ENC_TYPE m_encType;
	float m_avg_fps;
	int m_avg_bitrate;
	int m_avg_pts;
	unsigned int m_stat_size;
	CRITICAL_SECTION m_stat_cs;
};

class qosMeasurementRecord 
{
public:
	qosMeasurementRecord(struct timeval const& startTime, RTPSource* src)
			: fSource(src), fNext(NULL),
			kbits_per_second_min(1e20), kbits_per_second_max(0),
			kBytesTotal(0.0),
			packet_loss_fraction_min(1.0), packet_loss_fraction_max(0.0),
			totNumPacketsReceived(0), totNumPacketsExpected(0) 
	{
		measurementEndTime = measurementStartTime = startTime;

		RTPReceptionStatsDB::Iterator statsIter(src->receptionStatsDB());
		// Assume that there's only one SSRC source (usually the case):
		RTPReceptionStats* stats = statsIter.next(True);
		if (stats != NULL) {
			kBytesTotal = stats->totNumKBytesReceived();
			totNumPacketsReceived = stats->totNumPacketsReceived();
			totNumPacketsExpected = stats->totNumPacketsExpected();
		}
  	}
	virtual ~qosMeasurementRecord() { delete fNext; }

	void periodicQOSMeasurement(struct timeval const& timeNow);

public:
	RTPSource* fSource;
	qosMeasurementRecord* fNext;
public:
	struct timeval measurementStartTime, measurementEndTime;
	double kbits_per_second_min, kbits_per_second_max;
	double kBytesTotal;
	double packet_loss_fraction_min, packet_loss_fraction_max;
	unsigned totNumPacketsReceived, totNumPacketsExpected;
};

#endif //__AMBA_RTSP_PLAY_H__

