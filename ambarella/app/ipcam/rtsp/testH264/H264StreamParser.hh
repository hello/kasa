#ifndef __H264_STREAM_PARSER_HH__
#define __H264_STREAM_PARSER_HH__
#include "../liveMedia/StreamParser.hh"

struct NALU
{
	u_int32_t timeStamp;
	u_int16_t don;
	u_int32_t size;
	u_int8_t * addr;
	u_int8_t nalu_type;
};

#define MAX_NALU_NUM_PER_FRAME	8
class H264BitStreamParser
{
public:
	H264BitStreamParser(int streamID);
	virtual ~H264BitStreamParser();
	void registerReadInterest(unsigned char* to,unsigned maxSize);

	virtual int parse();
	char* getParsersps();
	char* getParserpps();
	char* getPreID();
	u_int32_t getFrameNum();
	u_int32_t fNaluType;
	u_int32_t fNaluSize;
	u_int64_t fPTS;
	Boolean fIsNewSession;
protected:
	unsigned parseNALUnit();
	unsigned curFrameSize();
protected:
	char* fsps;
	char* fpps;
	char profileLevelID[3];
	unsigned char* fTo;
	unsigned char* fLimit;
	NALU fNalus[MAX_NALU_NUM_PER_FRAME];
	u_int32_t fNaluCurrIndex;
	u_int32_t fNaluCount;
	int fStreamID;
	u_int32_t fSessionID;
	u_int32_t fFrameNum;
private:
};


enum MyH264ParseState
{
	PARSING_START_SEQUENCE,
	PARSING_NAL_UNIT
};

class H264FileStreamParser:public StreamParser
{
public:
	H264FileStreamParser(FramedSource* usingSource,FramedSource* inputSource);
	virtual ~H264FileStreamParser();
	void registerReadInterest(unsigned char* to,unsigned maxSize);

	virtual unsigned parse();
	char* getParsersps();
	char* getParserpps();
	unsigned int getPreID();
	unsigned numTruncatedBytes() const;
	unsigned getParseState();
	unsigned fNaluType;
protected:
	void setParseState(MyH264ParseState parseState);
	unsigned parseStartSequence();
	unsigned parseNALUnit();
	void saveByte(u_int8_t byte);
	void save4Bytes(u_int32_t word);
	void saveToNextCode(u_int32_t& curWord);
	void skipToNextCode(u_int32_t& curWord);
	unsigned curFrameSize();
protected:
	FramedSource* fUsingSource;
	char* fsps;
	char* fpps;
	unsigned int profileLevelID;
	unsigned char* fStartOfFrame;
	unsigned char* fTo;
	unsigned char* fLimit;
	unsigned char* fSavedTo;
	unsigned fNumTruncatedBytes; //·Ö°ü?
	unsigned fSavedNumTruncatedBytes;
private:
	virtual void restoreSavedParserState();
	MyH264ParseState fCurrentParseState;
};


//-----------------------
#define AUDHEAD 0x00000109
#define SEIHEAD 0x00000106
#define SPSHEAD 0x00000107
#define PPSHEAD 0x00000108
#define IDRHEAD 0x00000105
#define NALUHEAD 0x00000101


#define NALU_TYPE_NONIDR	1
#define NALU_TYPE_IDR		5
#define NALU_TYPE_SEI		6
#define NALU_TYPE_SPS		7
#define NALU_TYPE_PPS		8
#define NALU_TYPE_AUD		9


#endif

