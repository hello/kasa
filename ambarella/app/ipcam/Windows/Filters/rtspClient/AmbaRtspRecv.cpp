#include "AmbaVideoSink.h"
#include "AmbaRtspRecv.h"
#include "assert.h"

#define MAX_QUEUE_SIZE 32

#include <strsafe.h>
#ifdef _DEBUG
void TRACE(STRSAFE_LPCSTR fmt, ...);
#define VerbosityLevel	1
#define DEBUG
#else 
#define TRACE
#define VerbosityLevel	0
#endif
#include "wxdebug.h"

CDataProcess::CDataProcess():m_dataBuffer(NULL)
	,m_dataBufferSize(4 << 20)
	,m_write_cursor(0)
	,m_stopped(false)
{
	m_queue = new data_unit[MAX_QUEUE_SIZE];
	for(int i=0; i< READER_NUM; i++)
	{
		m_rEvent[i] = 0;
		m_wEvent[i] = 0;
		m_next_read_cursor[i] = 0;
		m_being_read_cursor[i] = -1;
	}
}

CDataProcess::~CDataProcess()
{
	if (m_queue != NULL)
	{
		delete[] m_queue;
		m_queue = NULL;
	}
	if (m_dataBuffer != NULL)
	{
		delete[] m_dataBuffer;
		m_dataBuffer = NULL;
	}
	closeEvent();
}

int CDataProcess::init()
{
	if (m_dataBuffer != NULL)
	{
		delete[] m_dataBuffer;
	}
	m_dataBuffer = new unsigned char[m_dataBufferSize];
	if (m_dataBuffer == NULL)
	{
		return 1;
	}
	for(int i = 0; i<MAX_QUEUE_SIZE; i++)
	{
		m_queue[i].start_addr = m_dataBuffer;
		m_queue[i].slice_info.data_size = 0;
		for(int j=0; j<READER_NUM; j++)
		{
			m_queue[i].read_flag[j] = false;
		}
	}
	closeEvent();
	m_write_cursor = 0;

	for(int i=0; i<READER_NUM; i++ )
	{
		m_rEvent[i] = CreateEvent( 
            NULL,     // no security attributes
            FALSE,    // auto-reset event
            TRUE,     // initial state is signaled
            NULL);    // object not named
		m_wEvent[i] = CreateEvent( 
        NULL,         // no security attributes
        TRUE,         // manual-reset event
        FALSE,         // initial state is signaled
        NULL/*(PCTSTR)"WriteEvent"*/  // object name
        );
		m_next_read_cursor[i] = 0;
		m_being_read_cursor[i] = -1;
	}
	m_stopped = false;
	return 0;
}

int CDataProcess::stop()
{
	for(int i=0; i<READER_NUM; i++ )
	{
		if (m_wEvent[i] !=0)
		{
			SetEvent(m_wEvent[i]);
		}
	}
	m_stopped = true;
	return 0;
}

int CDataProcess::closeEvent()
{
	for(int i=0; i<READER_NUM; i++ )
	{
		if (m_rEvent[i] != 0)
		{
   			CloseHandle(m_rEvent[i]);
			m_rEvent[i] = 0;
		}
		if (m_wEvent[i] !=0)
		{
			CloseHandle(m_wEvent[i]);
			m_wEvent[i] = 0;
		}
	}
	return 0;
}

void CDataProcess::workaround_for_cursor(int& cursor)
{
	if (cursor < 0)
	{
		cursor = MAX_QUEUE_SIZE-1;
	}
	if (cursor >= MAX_QUEUE_SIZE)
	{
		cursor = 0;
	}
}

int CDataProcess::add_to_queue(unsigned char* pData, const SLICE_INFO* pInfo)
{
	DbgLog((LOG_TRACE, 1, TEXT("begin add_to_queue %d"),timeGetTime()));
	DWORD dwWaitResult;
	unsigned char* write_addr_begin;
	for (int i=0; i<READER_NUM; i++)
	{
		dwWaitResult = WaitForSingleObject(	m_rEvent[i],
			1000
			);
		switch (dwWaitResult) 
		{
			case WAIT_OBJECT_0: 
				break;
			default:
				DbgLog((LOG_TRACE, 1, TEXT("Wait error: %d"),GetLastError()));
				return -1;
		}
	

		if (m_being_read_cursor[i] == m_write_cursor)
		{
			m_write_cursor++;
			workaround_for_cursor(m_write_cursor);
		}

		int pre_write_cursor = m_write_cursor-1;
		workaround_for_cursor(pre_write_cursor);
		bool write_pos_occupied = false;
		write_addr_begin = m_queue[pre_write_cursor].start_addr + m_queue[pre_write_cursor].slice_info.data_size;
		if (write_addr_begin >= (m_dataBuffer + m_dataBufferSize))
		{
			write_addr_begin -= m_dataBufferSize;
		}
		unsigned char* write_addr_end = write_addr_begin + pInfo->data_size;

		if (m_queue[m_write_cursor].read_flag[i] == true) //not fetch out
		{
			write_pos_occupied = true; //max caching number MAX_QUEUE_SIZE reached
		}
		else
		{
			int next_read_cursor = m_next_read_cursor[i];
			if (m_queue[next_read_cursor].read_flag[i] == true) //not fetch out
			{
				//max caching size m_dataBufferSize reached
				unsigned char* next_read_addr_begin = m_queue[next_read_cursor].start_addr;
				unsigned char* next_read_addr_end = next_read_addr_begin + m_queue[next_read_cursor].slice_info.data_size;
				if ((write_addr_begin >=  next_read_addr_begin && write_addr_begin <  next_read_addr_end)
					|| (write_addr_end >  next_read_addr_begin && write_addr_end <=  next_read_addr_end))
				{
					write_pos_occupied = true;
				}
			}
		}
		if (write_pos_occupied == true)
		{
			DbgLog((LOG_TRACE, 1, TEXT("buffer is full for reader%d"),i));
			for (int j=0; j< MAX_QUEUE_SIZE; j++)
			{
				if (j != m_being_read_cursor[i])
				{
					m_queue[j].read_flag[i] = false; //discard data which not being fetch out
				}
			}
			m_next_read_cursor[i] = m_write_cursor;
			write_pos_occupied = false;
		}

		SetEvent(m_rEvent[i]); //unlock
		assert(m_queue[m_write_cursor].read_flag[i] == false);
	}
	
	int offset_to_end = int(m_dataBuffer + m_dataBufferSize - write_addr_begin);
	if (offset_to_end >= pInfo->data_size)
	{
		memcpy(write_addr_begin, pData, pInfo->data_size);
	}
	else
	{
		memcpy(write_addr_begin, pData, offset_to_end);
		memcpy(m_dataBuffer, pData+offset_to_end, pInfo->data_size -offset_to_end);
		assert(pInfo->data_size -offset_to_end <= write_addr_begin-m_dataBuffer);
	}
	m_queue[m_write_cursor].start_addr = write_addr_begin;
	m_queue[m_write_cursor].slice_info.data_size = pInfo->data_size;
	m_queue[m_write_cursor].slice_info.pts = pInfo->pts;
	m_queue[m_write_cursor].slice_info.nal_unit_type = pInfo->nal_unit_type;
	m_queue[m_write_cursor].slice_info.pts_valid = pInfo->pts_valid;

	for (int i=0; i< READER_NUM; i++)
	{
		dwWaitResult = WaitForSingleObject(	m_rEvent[i],1000);	
		switch (dwWaitResult) 
		{
			case WAIT_OBJECT_0: 
				break;
			default: 
				DbgLog((LOG_TRACE, 1, TEXT("Wait error: %d"),GetLastError()));
				return -1;
		}

		SetEvent(m_rEvent[i]); //unlock
		m_queue[m_write_cursor].read_flag[i] = true;
		//SetEvent(m_wEvent[i]);
	}
	DbgLog((LOG_TRACE, 1, TEXT("add to queue[%d] %d"),m_write_cursor,timeGetTime()));
	m_write_cursor++;
	workaround_for_cursor(m_write_cursor);
	for (int i=0; i< READER_NUM; i++)
	{
		SetEvent(m_wEvent[i]);
	}
	return 0;
}

int CDataProcess::get_from_queue(unsigned char* pData, SLICE_INFO* pInfo, int reader_idx)
{
	assert( reader_idx < READER_NUM);
	int cursor = m_next_read_cursor[reader_idx];
	DbgLog((LOG_TRACE, 1, TEXT("reader %d get queue[%d]**"),reader_idx,cursor));
	//wait until data is prepared
	DWORD dwWaitResult;
	int ret = 0;//(m_write_cursor - cursor+32)%32;
	if (m_queue[cursor].read_flag[reader_idx] == false) //data not prepared
	{
		ret = 32;
		ResetEvent(m_wEvent[reader_idx]);
		dwWaitResult = WaitForSingleObject(m_wEvent[reader_idx],3000);
		switch (dwWaitResult) 
		{
			// write-event object was signaled.
			case WAIT_OBJECT_0: 
			break;
			default: 
			DbgLog((LOG_TRACE, 1, TEXT("Wait m_wEvent error: %d"), GetLastError()));
			return -1;
		}
		if (m_stopped)
		{
			return -2;
		}
	}
	else
	{
		ret = (m_write_cursor - cursor+31)%32;
	}
	//lock 
	dwWaitResult = WaitForSingleObject(m_rEvent[reader_idx],1000);
	switch (dwWaitResult) 
	{
		case WAIT_OBJECT_0: 
			break;
		default: 
			DbgLog((LOG_TRACE, 1, TEXT("Wait m_rEvent[%d] error(begin): %d"),reader_idx, GetLastError()));
			return -1;
	}
	m_being_read_cursor[reader_idx] = cursor;
	m_next_read_cursor[reader_idx]++;
	workaround_for_cursor(m_next_read_cursor[reader_idx]);
	//unlock
	SetEvent(m_rEvent[reader_idx]);

	//copy data to destination
	int offset_to_end = (int)(m_dataBuffer + m_dataBufferSize - m_queue[cursor].start_addr);
	int data_size = m_queue[cursor].slice_info.data_size;

	if (offset_to_end < data_size)
	{
		memcpy(pData, m_queue[cursor].start_addr,offset_to_end);
		memcpy(pData+offset_to_end, m_dataBuffer, data_size - offset_to_end); 
	}
	else
	{
		memcpy(pData, m_queue[cursor].start_addr, data_size);
	}
	pInfo->data_size = data_size;
	pInfo->pts = m_queue[cursor].slice_info.pts;
	pInfo->nal_unit_type = m_queue[cursor].slice_info.nal_unit_type;
	pInfo->pts_valid = m_queue[cursor].slice_info.pts_valid;
	
	//lock 
	dwWaitResult = WaitForSingleObject(m_rEvent[reader_idx],INFINITE);
	switch (dwWaitResult) 
	{
		case WAIT_OBJECT_0: 
			break;
		default: 
			DbgLog((LOG_TRACE, 1, TEXT("Wait m_rEvent[%d] error(begin): %d"),m_rEvent[reader_idx], GetLastError()));
			return -1;
	}
	m_queue[cursor].read_flag[reader_idx] = false;
	m_being_read_cursor[reader_idx] = -1;
	DbgLog((LOG_TRACE, 1, TEXT("reader %d get queue[%d]"),reader_idx,cursor));
	//unlock
	SetEvent(m_rEvent[reader_idx]);
	
	return ret;
}


////////////////////////////////////////////////////////////////////////
//
// CAmbaRtspRecv
//
////////////////////////////////////////////////////////////////////////
void CAmbaRtspRecv::subsessionAfterPlaying(void* clientData) {
	// Begin by closing this media subsession's stream:
	MediaSubsession_t*  clientData_t = (MediaSubsession_t*)clientData;
	MediaSubsession* subsession = clientData_t->pMediaSubsession;
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession& session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL) return; // this subsession is still active
	}

	// All subsessions' streams have now been closed
	//clientData_t->pAmbaRtspRecv->sessionAfterPlaying();
	clientData_t->pAmbaRtspRecv->shutdown();
}

void CAmbaRtspRecv::subsessionByeHandler(void* clientData) {
	MediaSubsession_t*  clientData_t = (MediaSubsession_t*)clientData;
	MediaSubsession* subsession = clientData_t->pMediaSubsession;
	CAmbaRtspRecv* ambaRtspRecv= clientData_t->pAmbaRtspRecv;
	struct timeval timeNow;
	gettimeofday(&timeNow, NULL);
	unsigned secsDiff = timeNow.tv_sec - (ambaRtspRecv->m_startTime).tv_sec;

	*(ambaRtspRecv->m_env) << "Received RTCP \"BYE\" on \"" << subsession->mediumName()
		<< "/" << subsession->codecName()
		<< "\" subsession (after " << secsDiff
		<< " seconds)\n";

  // Act now as if the subsession had closed:
	subsessionAfterPlaying(clientData_t);
}

void CAmbaRtspRecv::sessionTimerHandler(void* clientData) {
	CAmbaRtspRecv* ambaRtspRecv = (CAmbaRtspRecv*)clientData;
	ambaRtspRecv->m_sessionTimerTask = NULL;
  	//ambaRtspRecv->sessionAfterPlaying();
  	ambaRtspRecv->shutdown();
}

void CAmbaRtspRecv::periodicQOSMeasurement(void* clientData) {
	CAmbaRtspRecv* ambaRtspRecv = (CAmbaRtspRecv*)clientData;
	struct timeval timeNow;
	gettimeofday(&timeNow, NULL);

	for (qosMeasurementRecord* qosRecord = ambaRtspRecv->m_qosRecordHead;
		qosRecord != NULL; qosRecord = qosRecord->fNext)
	{
		qosRecord->periodicQOSMeasurement(timeNow);
	}

	// Do this again later:
	ambaRtspRecv->scheduleNextQOSMeasurement();
}

#define MAX_URL_LENGTH 256

CAmbaRtspRecv::CAmbaRtspRecv(): m_ourClient(NULL)
	, m_sessionTimerTask(NULL)
	, m_arrivalCheckTimerTask(NULL)
	, m_interPacketGapCheckTimerTask(NULL)
	, m_qosMeasurementTimerTask(NULL)
	, m_qosMeasurementIntervalMS(0) // 0 means: Don't output QOS data
	, m_session(NULL)
	, m_qosRecordHead(NULL)
	, m_fileSinkBufferSize(H264_BUF_SIZE)
	, m_duration(0)
	, m_scale(1.0f)
	, m_initialSeekTime(0.0f)
	, m_encType(ENC_NONE)
	//, m_size(0)
	//, m_sample_delta(0)
	, m_avg_fps(0.0)
	, m_avg_bitrate(0)
	, m_stat_size(DEFAULT_STAT_WINDOW_SIZE)
{
	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	m_env = BasicUsageEnvironment::createNew(*scheduler);
	m_dataProcess  = new CDataProcess();
	m_buf = new unsigned char[H264_BUF_SIZE];
	m_watch = 0;
	//m_lastTime.tv_sec = 0;
	//m_lastTime.tv_usec = 0;
	m_beginTime.tv_sec = 0;
	m_beginTime.tv_usec = 0;
	m_info.data_size = 0;
	m_info.pts = 0;
	m_info.nal_unit_type = 0;
	m_info.pts_valid = 0;
	InitializeCriticalSection(&m_stat_cs);
	m_stat.begin_time = 0;
	m_stat.frame_cnt = 0;
	m_stat.totle_size = 0;
	m_stat.totle_pts = 0;
	m_stat.last_pts = 0;
	m_stat.window_size = m_stat_size;
	m_stat.old_time = NULL;
	m_stat.old_size = NULL;
	m_stat.old_pts = NULL;

}

CAmbaRtspRecv::~CAmbaRtspRecv()
{
	shutdown();
	delete m_dataProcess;
	delete[] m_buf;
	if (m_stat.old_time)
	{
		delete[] m_stat.old_time;
	}
	if (m_stat.old_size)
	{
		delete[] m_stat.old_size;
	}
	if (m_stat.old_pts)
	{
		delete[] m_stat.old_pts;
	}
	DeleteCriticalSection(&m_stat_cs);
}

void CAmbaRtspRecv::add_data(unsigned char* data, unsigned int dataSize, 
	struct timeval presentationTime)
{
	if (m_beginTime.tv_sec == 0 && m_beginTime.tv_usec == 0)
	{
		m_beginTime.tv_sec = presentationTime.tv_sec;
		m_beginTime.tv_usec = presentationTime.tv_usec;
	}
	/*bool deliver_flag = false;
	if ( (m_lastTime.tv_sec != 0 && m_lastTime.tv_usec != 0) &&
		 (presentationTime.tv_usec != m_lastTime.tv_usec || presentationTime.tv_sec != m_lastTime.tv_sec) )
	{
		deliver_flag = true;
	}
	
	if (deliver_flag) {
		//int sample_delta = presentationTime.tv_usec -m_lastTime.tv_usec + 
		//				(presentationTime.tv_sec -m_lastTime.tv_sec ) * 1000000;
		//sample_delta = sample_delta*9/100;
		m_info.pts = (long long)(presentationTime.tv_usec -m_beginTime.tv_usec) + 
						(long long)(presentationTime.tv_sec -m_beginTime.tv_sec ) * 1000000;
		m_info.pts_valid = check_pts();
		m_dataProcess->add_to_queue(m_buf, &m_info, check_pts());
		m_info.data_size = 0;
	}*/
	memcpy(m_buf + m_info.data_size, data, dataSize);
	m_info.data_size += dataSize;

	//m_lastTime.tv_sec = presentationTime.tv_sec;
	//m_lastTime.tv_usec = presentationTime.tv_usec;
	if (m_encType == ENC_H264) 
	{
		m_info.nal_unit_type = CParseH264::get_nal_unit_type(data);
	} else if (m_encType == ENC_MJPEG)
	{
		m_info.nal_unit_type = 0xFF; //MJPEG
	}

	if (m_info.nal_unit_type  == 1 || m_info.nal_unit_type == 5 || m_info.nal_unit_type == 0xFF)
	{
		m_info.pts = (long long)(presentationTime.tv_usec -m_beginTime.tv_usec) + 
						(long long)(presentationTime.tv_sec -m_beginTime.tv_sec ) * 1000000;
		/*if(m_stat.frame_cnt < 60)
		{
			m_info.pts_valid = 0;
		}
		else
		{*/
			m_info.pts_valid = check_pts();
		//}
		m_dataProcess->add_to_queue(m_buf, &m_info);

		DWORD curTime = timeGetTime();
		DWORD passTime = 0;
		long long curPTS = m_info.pts*9/100 - m_stat.last_pts;;
		if (m_stat.frame_cnt == 0)
		{
			m_stat.begin_time = curTime;
		}
		else
		{
			int stat_idx = (m_stat.frame_cnt - 1) % m_stat.window_size;
			if (m_stat.frame_cnt <= m_stat.window_size)
			{
				passTime = curTime - m_stat.begin_time;
				if (passTime == 0)
				{
					passTime = 1;
				}
				m_stat.totle_size += m_info.data_size;
				m_stat.totle_pts += curPTS;
				EnterCriticalSection(&m_stat_cs);
				m_avg_fps = 1.0f * m_stat.frame_cnt / passTime * 1000;
				m_avg_bitrate = (int)((1.0f * m_stat.totle_size * 8 / 1024 )/(1.0f * passTime / 1000));
				m_avg_pts = (int)(1.0f * m_stat.totle_pts / m_stat.frame_cnt + 0.5f); //round
				LeaveCriticalSection(&m_stat_cs);
			}
			else
			{
				passTime = curTime - m_stat.old_time[stat_idx];
				m_stat.totle_size += m_info.data_size - m_stat.old_size[stat_idx];
				m_stat.totle_pts += curPTS - m_stat.old_pts[stat_idx];
				EnterCriticalSection(&m_stat_cs);			
				m_avg_fps = 1.0f * m_stat.window_size / passTime * 1000;
				m_avg_bitrate = (int)((1.0f * m_stat.totle_size * 8 / 1024 )/(1.0f * passTime / 1000));
				m_avg_pts = (int)(1.0f * m_stat.totle_pts / m_stat.window_size + 0.5f); //round
				LeaveCriticalSection(&m_stat_cs);
			}
			m_stat.old_time[stat_idx] = curTime;
			m_stat.old_size[stat_idx] = m_info.data_size;
			m_stat.old_pts[stat_idx] = curPTS;
		}
		m_stat.frame_cnt++;
		m_stat.last_pts = m_info.pts*9/100;
		m_info.data_size = 0;
	}		
}

bool CAmbaRtspRecv::check_pts()
{
	bool pts_valid = true;
	if (m_stat.frame_cnt > 1)
	{
		float expect_pts = 90000.0f / m_avg_fps;
		float deviation = 1.0f * (m_avg_pts - expect_pts) / (m_avg_pts > expect_pts ? expect_pts:m_avg_pts);
		if (deviation > 0.1 || deviation < -0.1)
		{
			pts_valid = false;
		}
		DbgLog((LOG_TRACE, 1, TEXT("pts valid = %d, deviation %f"),pts_valid, deviation));
	}
	return pts_valid;
}

int CAmbaRtspRecv::fetch_data(unsigned char* pData, SLICE_INFO* pInfo, int reader_idx)
{
	return m_dataProcess->get_from_queue(pData, pInfo, reader_idx);
}

int CAmbaRtspRecv::setup(const char* url,const short transfertype, const short buffersize)
{
	do {
		// Create our client object:
		gettimeofday(&m_startTime, NULL);
		m_ourClient = createClient(*m_env, VerbosityLevel, "RtspClientFilter");
		if (m_ourClient == NULL) {
			*m_env << "Failed to create RTSP client: " << m_env->getResultMsg() << "\n";
			break;
		}

		// Open the URL, to get a SDP description:
		char* sdpDescription = getSDPDescriptionFromURL(
							m_ourClient, url, NULL,NULL,
							NULL, 0, 0);
		if (sdpDescription == NULL) {
			*m_env << "Failed to get a SDP description from URL \"" << url
			<< "\": " << m_env->getResultMsg() << "\n";
			break;
		}

		*m_env << "Opened URL \"" << url
		  << "\", returning a SDP description:\n" << sdpDescription << "\n";

		// Create a media session object from this SDP description:
		m_session = MediaSession::createNew(*m_env, sdpDescription);
		delete[] sdpDescription;
		if (m_session == NULL) {
			*m_env << "Failed to create a MediaSession object from the SDP description: " << m_env->getResultMsg() << "\n";
			break;
		} else if (!m_session->hasSubsessions()) {
			*m_env << "This session has no media subsessions (i.e., \"m=\" lines)\n";
			break;
		}
		// Then, setup the "RTPSource"s for the session:
		MediaSubsessionIterator iter(*m_session);
		MediaSubsession *subsession;
		Boolean madeProgress = False;
		while ((subsession = iter.next()) != NULL) {
			if (!subsession->initiate(-1)) {
				*m_env << "Unable to create receiver for \"" << subsession->mediumName()
						<< "/" << subsession->codecName()
						<< "\" subsession: " << m_env->getResultMsg() << "\n";
			} else {
				*m_env << "Created receiver for \"" << subsession->mediumName()
						<< "/" << subsession->codecName()
						<< "\" subsession (client ports " << subsession->clientPortNum()
						<< "-" << subsession->clientPortNum()+1 << ")\n";
				madeProgress = True;
			}
		}
		if (!madeProgress) {
			break;
		}
		
		// Perform additional 'setup' on each subsession, before playing them:
		madeProgress = setupStreams(transfertype);
		if (!madeProgress) {
			break;
		}
		// Create and start "FileSink"s for each subsession:
		iter.reset();
		while ((subsession = iter.next()) != NULL) {
			if (subsession->rtpSource() != NULL) {
				// Because we're saving the incoming data, rather than playing
				// it in real time, allow an especially large time threshold
				// (1 second) for reordering misordered incoming packets:
				unsigned const thresh = 1000000; // 1 second
				subsession->rtpSource()->setPacketReorderingThresholdTime(thresh);

				// Set the RTP source's OS socket buffer size as appropriate - either if we were explicitly asked (using -B),
				// or if the desired FileSink buffer size happens to be larger than the current OS socket buffer size.
				// (The latter case is a heuristic, on the assumption that if the user asked for a large FileSink buffer size,
				// then the input data rate may be large enough to justify increasing the OS socket buffer size also.)
				int socketNum = subsession->rtpSource()->RTPgs()->socketNum();
				unsigned curBufferSize = getReceiveBufferSize(*m_env, socketNum);
				if ( m_fileSinkBufferSize > curBufferSize) {
					unsigned newBufferSize = m_fileSinkBufferSize;
					newBufferSize = setReceiveBufferTo(*m_env, socketNum, newBufferSize);
				}
			}
			if (subsession->readSource() == NULL) continue; // was not initiated
			//m_video_width = subsession->videoWidth();
			//m_video_height =  subsession->videoHeight();
			CAmbaVideoSink* fileSink;
			if (strcmp(subsession->mediumName(), "video") == 0 &&
				strcmp(subsession->codecName(), "H264") == 0) {
				// For H.264 video stream, we use a special sink that insert start_codes:
				fileSink = CAmbaH264VideoSink::createNew(*m_env, this, m_fileSinkBufferSize);
				m_encType = ENC_H264;
			} else if (strcmp(subsession->mediumName(), "video") == 0 &&
				strcmp(subsession->codecName(), "JPEG") == 0 ){ //not supported now
				// Normal case:
				fileSink = CAmbaVideoSink::createNew(*m_env, this, m_fileSinkBufferSize);
				m_encType = ENC_MJPEG;
			} else {
				m_encType = ENC_OTHERS;
			}

			subsession->sink = fileSink;
			if (subsession->sink == NULL) {
				*m_env << "Failed to create AmbaVideoSink \": " << m_env->getResultMsg() << "\n";
			} else {
				MediaSubsession_t* subsession_t = new MediaSubsession_t;
				subsession_t->pAmbaRtspRecv = this;
				subsession_t->pMediaSubsession = subsession;
				subsession->sink->startPlaying(*(subsession->readSource()),
						 subsessionAfterPlaying,subsession_t);
				if (subsession->rtcpInstance() != NULL) {
					subsession->rtcpInstance()->setByeHandler(subsessionByeHandler,subsession_t);
				}
				//delete subsession_t;
				madeProgress = True;
			}
		}
		if (!madeProgress) {
			break;
		}

		if (buffersize > 0)
		{
			unsigned int size = (unsigned int)buffersize << 20;
			m_dataProcess->set_databuffer_size(size);
		}
		m_dataProcess->init();
		//m_lastTime.tv_sec = 0;
		//m_lastTime.tv_usec = 0;
		m_beginTime.tv_sec = 0;
		m_beginTime.tv_usec = 0;
		m_info.data_size = 0;
		m_info.pts = 0;

		m_stat.begin_time = 0;
		m_stat.frame_cnt = 0;
		m_stat.last_pts = 0;
		m_stat.totle_pts = 0;
		m_stat.totle_size = 0;
		if (m_stat.old_size)
		{
			delete[] m_stat.old_size;
		}
		if (m_stat.old_time)
		{
			delete[] m_stat.old_time;
		}
		if (m_stat.old_pts)
		{
			delete[] m_stat.old_pts;
		}
		m_stat.window_size = m_stat_size;
		m_stat.old_time = new DWORD[m_stat.window_size]();
		m_stat.old_size = new int[m_stat.window_size]();
		m_stat.old_pts = new long long[m_stat.window_size]();

		timeBeginPeriod(1);
		EnterCriticalSection(&m_stat_cs);
		m_avg_fps = 0.0;
		m_avg_bitrate = 0;
		m_avg_pts = 0;
		LeaveCriticalSection(&m_stat_cs);
		return 0;

	} while(0);

	shutdown();
	return 1;
}

void CAmbaRtspRecv::play()
{
	if (startPlayingStreams())
	{
		m_watch = 0;
		m_env->taskScheduler().doEventLoop(&m_watch);
	}
	else
	{
		shutdown();
	}
}

void CAmbaRtspRecv::stop()
{
	m_watch = 1;
	m_dataProcess->stop();
}

char* CAmbaRtspRecv::getSDPDescriptionFromURL(Medium* client, char const* url,
			       char const* username, char const* password,
			       char const* /*proxyServerName*/,
			       unsigned short /*proxyServerPortNum*/,
			       unsigned short /*clientStartPort*/) 
{
	RTSPClient* rtspClient = (RTSPClient*)client;
	char* result;
	if (username != NULL && password != NULL) {
		result = rtspClient->describeWithPassword(url, username, password);
	} else {
		result = rtspClient->describeURL(url);
	}
	return result;
}

Medium* CAmbaRtspRecv::createClient(UsageEnvironment& env,
		int verbosityLevel, char const* applicationName)
{
	return RTSPClient::createNew(env, verbosityLevel, applicationName,0);
}

void CAmbaRtspRecv::shutdown() {
	if (m_env != NULL) {
		m_env->taskScheduler().unscheduleDelayedTask(m_sessionTimerTask);
		m_env->taskScheduler().unscheduleDelayedTask(m_arrivalCheckTimerTask);
		m_env->taskScheduler().unscheduleDelayedTask(m_interPacketGapCheckTimerTask);
		m_env->taskScheduler().unscheduleDelayedTask(m_qosMeasurementTimerTask);
	}

	if (m_qosMeasurementIntervalMS > 0) {
		printQOSData( );
	}

	// Close our output files:
	closeMediaSinks();

	// Teardown, then shutdown, any outstanding RTP/RTCP subsessions
	tearDownStreams();
	Medium::close(m_session);
	m_session = NULL;

	// Finally, shut down our client:
	Medium::close(m_ourClient);
	m_ourClient = NULL;

	m_dataProcess->closeEvent();
	timeEndPeriod(1);
	// Adios...
	//exit(exitCode);
}

void CAmbaRtspRecv::printQOSData() {
	*m_env << "begin_QOS_statistics\n";
  
	// Print out stats for each active subsession:
	qosMeasurementRecord* curQOSRecord = m_qosRecordHead;
	if (m_session != NULL) {
		MediaSubsessionIterator iter(*m_session);
		MediaSubsession* subsession;
		while ((subsession = iter.next()) != NULL) {
			RTPSource* src = subsession->rtpSource();
			if (src == NULL) continue;
	      
			*m_env << "subsession\t" << subsession->mediumName()
	   		<< "/" << subsession->codecName() << "\n";
	      
			unsigned numPacketsReceived = 0, numPacketsExpected = 0;
	      
			if (curQOSRecord != NULL) {
				numPacketsReceived = curQOSRecord->totNumPacketsReceived;
				numPacketsExpected = curQOSRecord->totNumPacketsExpected;
			}
			*m_env << "num_packets_received\t" << numPacketsReceived << "\n";
			*m_env << "num_packets_lost\t" << numPacketsExpected - numPacketsReceived << "\n";
	      
			if (curQOSRecord != NULL) {
				unsigned secsDiff = curQOSRecord->measurementEndTime.tv_sec
					- curQOSRecord->measurementStartTime.tv_sec;
				int usecsDiff = curQOSRecord->measurementEndTime.tv_usec
					- curQOSRecord->measurementStartTime.tv_usec;
				double measurementTime = secsDiff + usecsDiff/1000000.0;
				*m_env << "elapsed_measurement_time\t" << measurementTime << "\n";
			
				*m_env << "kBytes_received_total\t" << curQOSRecord->kBytesTotal << "\n";
		
				*m_env << "measurement_sampling_interval_ms\t" << m_qosMeasurementIntervalMS << "\n";
		
				if (curQOSRecord->kbits_per_second_max == 0) {
					// special case: we didn't receive any data:
					*m_env <<
						"kbits_per_second_min\tunavailable\n"
	    					"kbits_per_second_ave\tunavailable\n"
						"kbits_per_second_max\tunavailable\n";
				} else {
					*m_env << "kbits_per_second_min\t" << curQOSRecord->kbits_per_second_min << "\n";
					*m_env << "kbits_per_second_ave\t"
						<< (measurementTime == 0.0 ? 0.0 : 8*curQOSRecord->kBytesTotal/measurementTime) << "\n";
					*m_env << "kbits_per_second_max\t" << curQOSRecord->kbits_per_second_max << "\n";
				}
		
				*m_env << "packet_loss_percentage_min\t" << 100*curQOSRecord->packet_loss_fraction_min << "\n";
				double packetLossFraction = numPacketsExpected == 0 ? 1.0
					: 1.0 - numPacketsReceived/(double)numPacketsExpected;
				if (packetLossFraction < 0.0) packetLossFraction = 0.0;
				*m_env << "packet_loss_percentage_ave\t" << 100*packetLossFraction << "\n";
				*m_env << "packet_loss_percentage_max\t"
					<< (packetLossFraction == 1.0 ? 100.0 : 100*curQOSRecord->packet_loss_fraction_max) << "\n";
		
				RTPReceptionStatsDB::Iterator statsIter(src->receptionStatsDB());
				// Assume that there's only one SSRC source (usually the case):
				RTPReceptionStats* stats = statsIter.next(True);
				if (stats != NULL) {
					*m_env << "inter_packet_gap_ms_min\t" << stats->minInterPacketGapUS()/1000.0 << "\n";
					struct timeval totalGaps = stats->totalInterPacketGaps();
					double totalGapsMS = totalGaps.tv_sec*1000.0 + totalGaps.tv_usec/1000.0;
					unsigned totNumPacketsReceived = stats->totNumPacketsReceived();
					*m_env << "inter_packet_gap_ms_ave\t"
						<< (totNumPacketsReceived == 0 ? 0.0 : totalGapsMS/totNumPacketsReceived) << "\n";
					*m_env << "inter_packet_gap_ms_max\t" << stats->maxInterPacketGapUS()/1000.0 << "\n";
				}
		
				curQOSRecord = curQOSRecord->fNext;
			}
		}
	}

	*m_env << "end_QOS_statistics\n";
	delete m_qosRecordHead;
}

void CAmbaRtspRecv::tearDownStreams() 
{
	if (m_session == NULL) return;
	clientTearDownSession(m_ourClient, m_session);
}

void CAmbaRtspRecv::closeMediaSinks()
{
	if (m_session == NULL) return;
	MediaSubsessionIterator iter(*m_session);
	MediaSubsession* subsession;
	while ((subsession = iter.next()) != NULL) {
		Medium::close(subsession->sink);
		subsession->sink = NULL;
	}
}

Boolean CAmbaRtspRecv::clientTearDownSession(Medium* client,
			      MediaSession* session) 
{
	if (client == NULL || session == NULL) return False;
	RTSPClient* rtspClient = (RTSPClient*)client;
	return rtspClient->teardownMediaSession(*session);
}

Boolean CAmbaRtspRecv::setupStreams(short transfertype)
{
	MediaSubsessionIterator iter(*m_session);
	MediaSubsession *subsession;
	Boolean madeProgress = False;

	while ((subsession = iter.next()) != NULL) {
		if (subsession->clientPortNum() == 0) continue; // port # was not set

		if (!clientSetupSubsession(m_ourClient, subsession, transfertype)) {
			*m_env << "Failed to setup \"" << subsession->mediumName()
			<< "/" << subsession->codecName()
			<< "\" subsession: " << m_env->getResultMsg() << "\n";
		} else {
			*m_env << "Setup \"" << subsession->mediumName()
				<< "/" << subsession->codecName()
				<< "\" subsession (client ports " << subsession->clientPortNum()
				<< "-" << subsession->clientPortNum()+1 << ")\n";
			madeProgress = True;
		}
	}
	return madeProgress;
}

Boolean CAmbaRtspRecv::clientSetupSubsession(Medium* client,
			MediaSubsession* subsession,
			short transfertype)
{
	if (client == NULL || subsession == NULL) return False;
	RTSPClient* rtspClient = (RTSPClient*)client;
	return rtspClient->setupMediaSubsession(*subsession,
		False, transfertype& 0x1, (transfertype>>1)&0x1);
}

//void CAmbaRtspRecv::sessionAfterPlaying(void* /*clientData*/) {
//	shutdown(0);
//}

Boolean CAmbaRtspRecv::startPlayingStreams() {
	double durationSlop = -1.0; // extra seconds to play at the end

	if (m_duration == 0) {
		if (m_scale > 0) m_duration = m_session->playEndTime() - m_initialSeekTime; // use SDP end time
		else if (m_scale < 0) m_duration = m_initialSeekTime;
 	 }
	if (m_duration < 0) m_duration = 0.0;

	if (!clientStartPlayingSession(m_ourClient, m_session)) {
		*m_env << "Failed to start playing session: " << m_env->getResultMsg() << "\n";
		return false;
	} else {
		*m_env << "Started playing session\n";
	}

	if (m_qosMeasurementIntervalMS > 0) {
		// Begin periodic QOS measurements:
		beginQOSMeasurement();
	}

	// Figure out how long to delay (if at all) before shutting down, or
	// repeating the playing
	Boolean timerIsBeingUsed = False;
	double secondsToDelay = m_duration;
	if (m_duration > 0) {
		timerIsBeingUsed = True;
		double absScale = m_scale > 0 ? m_scale : -m_scale; // ASSERT: scale != 0
		secondsToDelay = m_duration/absScale + durationSlop;

		int64_t uSecsToDelay = (int64_t)(secondsToDelay*1000000.0);
		m_sessionTimerTask = m_env->taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)sessionTimerHandler, (void*)NULL);
	}

	char const* actionString
		= "Receiving streamed data";
	if (timerIsBeingUsed) {
		*m_env << actionString
			<< " (for up to " << secondsToDelay
			<< " seconds)...\n";
	} else {
	    *m_env << actionString << "...\n";
	}
	return true;
	// Watch for incoming packets (if desired):
	//checkForPacketArrival(NULL);
	//checkInterPacketGaps(NULL);
}

void CAmbaRtspRecv::beginQOSMeasurement(){
	// Set up a measurement record for each active subsession:
	struct timeval startTime;
	gettimeofday(&startTime, NULL);
	m_nextQOSMeasurementUSecs = startTime.tv_sec*1000000 + startTime.tv_usec;
	qosMeasurementRecord* qosRecordTail = NULL;
	MediaSubsessionIterator iter(*m_session);
	MediaSubsession* subsession;
	while ((subsession = iter.next()) != NULL) {
		RTPSource* src = subsession->rtpSource();
		if (src == NULL) continue;

		qosMeasurementRecord* qosRecord = new qosMeasurementRecord(startTime, src);
		if (m_qosRecordHead == NULL) m_qosRecordHead = qosRecord;
		if (qosRecordTail != NULL) qosRecordTail->fNext = qosRecord;
		qosRecordTail  = qosRecord;
 	 }

  	// Then schedule the first of the periodic measurements:
	scheduleNextQOSMeasurement();
}

void CAmbaRtspRecv::scheduleNextQOSMeasurement() {
	//m_nextQOSMeasurementUSecs += qosMeasurementIntervalMS*1000;
	struct timeval timeNow;
	gettimeofday(&timeNow, NULL);
	unsigned timeNowUSecs = timeNow.tv_sec*1000000 + timeNow.tv_usec;
	unsigned usecsToDelay = m_nextQOSMeasurementUSecs - timeNowUSecs;
     // Note: This works even when nextQOSMeasurementUSecs wraps around

	m_qosMeasurementTimerTask = m_env->taskScheduler().scheduleDelayedTask(
		usecsToDelay, (TaskFunc*)periodicQOSMeasurement, (void*)this);
}

Boolean CAmbaRtspRecv::clientStartPlayingSession(Medium* client,
				  MediaSession* session) {
	double endTime = m_initialSeekTime;
	if (m_scale > 0) {
		if (m_duration <= 0) endTime = -1.0f;
		else endTime = m_initialSeekTime + m_duration;
	} else {
		endTime = m_initialSeekTime - m_duration;
		if (endTime < 0) endTime = 0.0f;
	}

	RTSPClient* rtspClient = (RTSPClient*)client;
	return rtspClient->playMediaSession(*session, m_initialSeekTime, endTime, (float)m_scale);
}

void qosMeasurementRecord::periodicQOSMeasurement(struct timeval const& timeNow) {
	unsigned secsDiff = timeNow.tv_sec - measurementEndTime.tv_sec;
	int usecsDiff = timeNow.tv_usec - measurementEndTime.tv_usec;
	double timeDiff = secsDiff + usecsDiff/1000000.0;
	measurementEndTime = timeNow;

	RTPReceptionStatsDB::Iterator statsIter(fSource->receptionStatsDB());
	// Assume that there's only one SSRC source (usually the case):
	RTPReceptionStats* stats = statsIter.next(True);
	if (stats != NULL) {
		double kBytesTotalNow = stats->totNumKBytesReceived();
		double kBytesDeltaNow = kBytesTotalNow - kBytesTotal;
		kBytesTotal = kBytesTotalNow;

		double kbpsNow = timeDiff == 0.0 ? 0.0 : 8*kBytesDeltaNow/timeDiff;
		if (kbpsNow < 0.0) kbpsNow = 0.0; // in case of roundoff error
		if (kbpsNow < kbits_per_second_min) kbits_per_second_min = kbpsNow;
		if (kbpsNow > kbits_per_second_max) kbits_per_second_max = kbpsNow;

		unsigned totReceivedNow = stats->totNumPacketsReceived();
		unsigned totExpectedNow = stats->totNumPacketsExpected();
		unsigned deltaReceivedNow = totReceivedNow - totNumPacketsReceived;
		unsigned deltaExpectedNow = totExpectedNow - totNumPacketsExpected;
		totNumPacketsReceived = totReceivedNow;
		totNumPacketsExpected = totExpectedNow;

		double lossFractionNow = deltaExpectedNow == 0 ? 0.0
			: 1.0 - deltaReceivedNow/(double)deltaExpectedNow;
		//if (lossFractionNow < 0.0) lossFractionNow = 0.0; //reordering can cause
		if (lossFractionNow < packet_loss_fraction_min) {
			packet_loss_fraction_min = lossFractionNow;
		}
		if (lossFractionNow > packet_loss_fraction_max) {
			packet_loss_fraction_max = lossFractionNow;
		}
	}
}


