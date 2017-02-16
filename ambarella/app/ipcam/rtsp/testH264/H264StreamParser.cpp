#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <basetypes.h>
#include "iav_ioctl.h"
#include "bsreader.h"
#include "H264StreamParser.hh"
#include "MyH264VideoStreamFramer.hh"
#include "assert.h"

#define USR_DEFINED_ERROR			2

//+++base64encode+++
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static void base64encode(char *out,const unsigned char* in,int len)
{
	for(;len >= 3; len -= 3)
	{
		*out ++ = cb64[ in[0]>>2];
		*out ++ = cb64[ ((in[0]&0x03)<<4)|(in[1]>>4) ];
		*out ++ = cb64[ ((in[1]&0x0f) <<2)|((in[2] & 0xc0)>>6) ];
		*out ++ = cb64[ in[2]&0x3f];
		in += 3;
	}
	if(len > 0)
	{
		unsigned char fragment;
		*out ++ = cb64[ in[0]>>2];
		fragment =	(in[0] &0x03)<<4 ;
		if(len > 1)
			fragment |= in[1] >>4 ;
		*out ++ = cb64[ fragment ];
		*out ++ = (len <2) ? '=' : cb64[ ((in[1]&0x0f)<<2)];
		*out ++ = '=';
	}
	*out = '\0';
}
//---base64encode---


static int findNalu_amba (NALU * nalus, u_int32_t * count, u_int8_t *bitstream, u_int32_t streamSize)
{
	int index = -1;
	u_int8_t * bs = bitstream;
	u_int32_t head;
	u_int8_t nalu_type;

	u_int8_t *last_byte_bitstream = bitstream + streamSize - 1;

	while (bs <= last_byte_bitstream) {

		head = (bs[3] << 24) | (bs[2] << 16) | (bs[1] << 8) | bs[0];
	//	printf("head 0x%x: 0x%x, 0x%x, 0x%x, 0x%x\n", head, bs[0], bs[1], bs[2], bs[3]);

		if (head == 0x01000000) {	// little ending
			index++;
			// we find a nalu
			bs += 4;		// jump to nalu type
			nalu_type = 0x1F & bs[0];
			nalus[index].nalu_type = nalu_type;
			nalus[index].addr = bs;

			if (index  > 0) {	// Not the first NALU in this stream
				nalus[index -1].size = nalus[index].addr - nalus[index -1].addr - 4; // cut off 4 bytes of delimiter
			}
//			printf("	nalu type %d\n", nalus[index].nalu_type);

			if ((nalu_type == NALU_TYPE_NONIDR) ||
				(nalu_type == NALU_TYPE_IDR)) {
				// Calculate the size of the last NALU in this stream
				nalus[index].size =  last_byte_bitstream - bs + 1;
				break;
			}
		}
		else if (bs[3] != 0) {
			bs += 4;
		} else if (bs[2] != 0) {
			bs += 3;
		} else if (bs[1] != 0) {
			bs += 2;
		} else {
			bs += 1;
		}
	}

	*count = index + 1;
	if (*count == 0) {
		if (streamSize == 0) {
			printf("stream ended.\n");
		} else {
			printf("No nalu found in the bitstream!\n");
		}
		return -1;
	}
	return 0;
}


//H264FileStreamParser
int fd_iav;
extern struct timeval timeNow, timeNow2;

H264BitStreamParser::H264BitStreamParser(int streamID)
	:fsps(NULL), fpps(NULL), fTo(NULL), fNaluCurrIndex(0), fNaluCount(0),
	fStreamID(streamID), fFrameNum(0)
{
	int i;
	bsreader_reset(streamID);
	for (i = 0; i < 3; i++) {
		profileLevelID[i] = 0;
	}
}

H264BitStreamParser::~H264BitStreamParser()
{
	if (fsps)
		delete[] fsps;
	if (fpps)
		delete[] fpps;
}

void H264BitStreamParser:: registerReadInterest(unsigned char* to,unsigned maxSize)
{
	fTo = to;
	fLimit = to + maxSize;
}

int H264BitStreamParser::parse()
{
	bsreader_frame_info_t frame_info;
//	static struct timeval timeNow, timeNow2;

	while (fNaluCurrIndex >= fNaluCount) { 	//No NAL unit currently in the buffer.	Read a new one.
//		gettimeofday(&timeNow, NULL);
//		printf("get one frame...\n"); //jay
//		printf("diff %d\n", (timeNow2.tv_sec - timeNow.tv_sec)*1000000 + timeNow2.tv_usec - timeNow.tv_usec);
		if (bsreader_get_one_frame(fStreamID, &frame_info) < 0) {
			printf("bs reader gets frame error\n");
			return -1;
		}
#if 1		//check frame lose
//		printf("frame_num %d\n", frame_info.bs_info.frame_num);
		if ((frame_info.bs_info.frame_num - fFrameNum) != 1 && fFrameNum != 0)
			printf("parser: stream[%d] lost frame!\n", fStreamID);
		fFrameNum = frame_info.bs_info.frame_num;
#endif
//		gettimeofday(&timeNow2, NULL);
		fIsNewSession = (frame_info.bs_info.session_id != fSessionID);
		fPTS = frame_info.bs_info.mono_pts;
		fSessionID = frame_info.bs_info.session_id;

		findNalu_amba(fNalus, &fNaluCount, frame_info.frame_addr, frame_info.frame_size);
		fNaluCurrIndex = 0;
	}

	// filter out the SEI nalu
	while (fNalus[fNaluCurrIndex].nalu_type == NALU_TYPE_SEI) {
		assert(fNaluCurrIndex <= fNaluCount);
		fNaluCurrIndex++;
	}
	fNaluSize = fNalus[fNaluCurrIndex].size;
	assert(fNaluSize > 0);
	fNaluType = fNalus[fNaluCurrIndex].nalu_type;

	//printf("[%d] nalu_type %d, size %d, mono_pts %llu\n", fStreamID,
	//	fNalus[fNaluCurrIndex].nalu_type, fNalus[fNaluCurrIndex].size, fPTS);
	if (fTo != NULL) {
		memcpy(fTo, fNalus[fNaluCurrIndex].addr, fNalus[fNaluCurrIndex].size);
	} else {
		if (fNaluType == NALU_TYPE_SPS) {
			unsigned char* sps = fNalus[fNaluCurrIndex].addr;
			if (fsps != NULL)
				delete[] fsps;
			fsps = new char[fNaluSize/3 *4 + 4 + 4]; // 1 extra byte for '\0'
			base64encode(fsps, sps, fNaluSize);
			memcpy(profileLevelID, sps + 1, 3);
		}
		if (fNaluType == NALU_TYPE_PPS) {
			unsigned char* pps = fNalus[fNaluCurrIndex].addr;
			if (fpps != NULL)
				delete[] fpps;
			fpps = new char[fNaluSize/3 *4 + 4 + 4]; // 1 extra byte for '\0'
			base64encode(fpps, pps, fNaluSize);
		}
	}
	fNaluCurrIndex++;
	return 0;
}

char* H264BitStreamParser::getParsersps()
{
	return fsps;
}

char* H264BitStreamParser::getParserpps()
{
	return fpps;
}

char* H264BitStreamParser::getPreID()
{
	return profileLevelID;
}

u_int32_t H264BitStreamParser::getFrameNum()
{
	return fFrameNum;
}

//H264FileStreamParser
H264FileStreamParser::H264FileStreamParser(FramedSource * usingSource, FramedSource * inputSource)
:StreamParser(inputSource, FramedSource::handleClosure, usingSource, &MyH264VideoStreamFramer::continueReadProcessing, usingSource),
fUsingSource(usingSource),fsps(NULL),fpps(NULL), profileLevelID(0), fCurrentParseState(PARSING_START_SEQUENCE)
{

}

H264FileStreamParser::~H264FileStreamParser()
{
	delete[] fsps;
	delete[] fpps;
}

//reset saved the parser state
void H264FileStreamParser::restoreSavedParserState()
{
	StreamParser::restoreSavedParserState();
	fTo = fSavedTo;
	fNumTruncatedBytes = fSavedNumTruncatedBytes;
}

void H264FileStreamParser::setParseState(MyH264ParseState parseState)
{
	fSavedTo = fTo;
	fSavedNumTruncatedBytes = fNumTruncatedBytes;
	fCurrentParseState = parseState;
	saveParserState();
}

unsigned H264FileStreamParser::getParseState()
{
	return fCurrentParseState;
}

void H264FileStreamParser:: registerReadInterest(unsigned char* to,unsigned maxSize)
{
	fStartOfFrame = fTo = fSavedTo = to;
	fLimit = to + maxSize;
	fNumTruncatedBytes = fSavedNumTruncatedBytes = 0;
}

unsigned H264FileStreamParser::parse()
{
	while (1) {
		try {
			switch(fCurrentParseState)
			{
				case PARSING_START_SEQUENCE:
//					printf("parseStartSequence\n");	//jay
					return parseStartSequence();
				case PARSING_NAL_UNIT:
//					printf("parseNALUnit\n");	//jay
					return parseNALUnit();
				default:
					return 0;
			}
		} catch (int e)
		{
//			printf("catch %d!\n", e);		//jay
			if (e == USR_DEFINED_ERROR)
				continue;
			else
				return 0;
		}
	}
}

unsigned H264FileStreamParser::parseStartSequence()
{
//get 4 bytes
	u_int32_t test = test4Bytes();
//find the startcode
	while(test != 0x00000001)
	{
		skipBytes(1);
		test = test4Bytes();
	}
//set the state to PARSING_START_SEQUNCE
	setParseState(PARSING_START_SEQUENCE);
	return parseNALUnit();
}

unsigned H264FileStreamParser::parseNALUnit()
{
	unsigned char* sps = NULL;
	unsigned char* pps = NULL;
	skipBytes(1);
	u_int32_t head= test4Bytes();
	skipBytes(3);
	u_int8_t Type = 0;

	u_int32_t test = test4Bytes();
	switch(head&0xffffff1f)
	{
		case SPSHEAD:
			Type = 7;
			sps = fTo + 1;
			break;
		case PPSHEAD:
			Type = 8;
			pps = fTo + 1;
			break;
		case IDRHEAD:
			Type = 5;
			break;
		case NALUHEAD:
			Type = 1;
			break;
		case AUDHEAD:
			Type = 9;
	//		throw USR_DEFINED_ERROR;
			break;
		case SEIHEAD:
			Type = 6;
	//		throw USR_DEFINED_ERROR;
			break;
		default:
			Type = 0;
			break;
	}

	while(test != 0x00000001)
	{
		saveByte(get1Byte());
		test = test4Bytes();
	}

	if(Type == 7)
	{
		if (fsps != NULL)
			delete[] fsps;
		fsps = new char[curFrameSize()*4/3 +4];
		base64encode(fsps,sps,curFrameSize()-1);
		memcpy(((char*)&profileLevelID),sps,1);
		memcpy(((char*)&profileLevelID)+1,sps+1,1);
		memcpy(((char*)&profileLevelID)+2,sps+2,1);
	}
	else if(Type == 8)
	{
		if (fpps != NULL)
			delete[] fpps;
		fpps = new char[curFrameSize()*4/3 +4];
		base64encode(fpps,pps,curFrameSize()-1);
	}
//	printf("curFrameSize %d\n", curFrameSize());
	fNaluType = Type;
	return curFrameSize();
}

void H264FileStreamParser::saveByte(u_int8_t byte)
{
//	printf("saveByte 0x%x\n", byte);
	if(fTo >= fLimit)
	{
		++fNumTruncatedBytes;
		return;
	}
	*fTo++ = byte;
}

void H264FileStreamParser::save4Bytes(u_int32_t word)
{
	if(fTo+4 > fLimit)
	{
		fNumTruncatedBytes += 4;
		return;
	}
	*fTo++ = (word>>24);
	*fTo++ = (word>>16);
	*fTo++ = (word>>8);
	*fTo++ = (word);
}

//save data until we see a sync word(0x00000001)
void H264FileStreamParser::saveToNextCode(u_int32_t& curWord)
{
	while(curWord != 0x00000001)
	{
		save4Bytes(curWord);
		curWord = get4Bytes();
	}
}

void H264FileStreamParser::skipToNextCode(u_int32_t& curWord)
{
	while(curWord != 0x00000001)
	{
		curWord = get4Bytes();
	}
}

unsigned H264FileStreamParser::curFrameSize()
{
	return fTo - fStartOfFrame;
}
char* H264FileStreamParser::getParsersps()
{
	return fsps;
}

char* H264FileStreamParser::getParserpps()
{
	return fpps;
}
unsigned int H264FileStreamParser::getPreID()
{
	return profileLevelID;
}


