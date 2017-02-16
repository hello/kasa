#ifndef __MY_H264_VIDEO_STREAM_FRAMER_HH__
#define __MY_H264_VIDEO_STREAM_FRAMER_HH__
#include "../liveMedia/include/H264VideoStreamFramer.hh"
#include "H264StreamParser.hh"


class MyH264VideoStreamFramer : public H264VideoStreamFramer
{
public:
	static MyH264VideoStreamFramer* createNew(UsageEnvironment& env, FramedSource* inputSource);
	static MyH264VideoStreamFramer* createNew(UsageEnvironment& env, int streamID);
	virtual Boolean currentNALUnitEndsAccessUnit();
	Boolean pictureEndMarker();
	static void continueReadProcessing(void* clientData,unsigned char* ptr, unsigned size, struct timeval presentationTime);
	void continueReadProcessing();
	char* getSPS();
	char* getPPS();
	char* getProfileLevelID();
	int getStreamID();

protected:
	MyH264VideoStreamFramer(UsageEnvironment& env, FramedSource* inputSource);
	MyH264VideoStreamFramer(UsageEnvironment& env, int streamID);
	virtual ~MyH264VideoStreamFramer();

private:
	virtual Boolean isH264VideoStreamFramer() const;
	virtual void doGetNextFrame();

protected:
	double fFrameRate;
	unsigned fPictureCount;
	Boolean fPictureEndMarker;
private:
	class H264FileStreamParser* fParser;
	struct timeval fPresentationTimeBase;
	int	fStreamID;
	u_int64_t fPTS;
	u_int32_t fFrameNum;
public:
	class H264BitStreamParser* fBitstreamParser;
};

#endif

