/*
This program is distributed under the terms of the 'MIT license'. The text
of this licence follows...

Copyright (c) 2004 J.D.Medhurst (a.k.a. Tixy)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/**
@file

@brief Implementation of ITU-T (formerly CCITT) Recomendation %G711
*/


//#include "common.h"
#include "audio_encode.h"

/*
Members of class G711
*/


EXPORT uint8_t G711::ALawEncode(int16_t pcm16)
	{
	int p = pcm16;
	unsigned a;  // A-law value we are forming
	if(p<0)
		{
		// -ve value
		// Note, ones compliment is used here as this keeps encoding symetrical
		// and equal spaced around zero cross-over, (it also matches the standard).
		p = ~p;
		a = 0x00; // sign = 0
		}
	else
		{
		// +ve value
		a = 0x80; // sign = 1
		}

	// Calculate segment and interval numbers
	p >>= 4;
	if(p>=0x20)
		{
		if(p>=0x100)
			{
			p >>= 4;
			a += 0x40;
			}
		if(p>=0x40)
			{
			p >>= 2;
			a += 0x20;
			}
		if(p>=0x20)
			{
			p >>= 1;
			a += 0x10;
			}
		}
	// a&0x70 now holds segment value and 'p' the interval number

	a += p;  // a now equal to encoded A-law value

	return a^0x55;	// A-law has alternate bits inverted for transmission
	}


EXPORT int G711::ALawDecode(uint8_t alaw)
	{
	alaw ^= 0x55;  // A-law has alternate bits inverted for transmission

	unsigned sign = alaw&0x80;

	int linear = alaw&0x1f;
	linear <<= 4;
	linear += 8;  // Add a 'half' bit (0x08) to place PCM value in middle of range

	alaw &= 0x7f;
	if(alaw>=0x20)
		{
		linear |= 0x100;  // Put in MSB
		unsigned shift = (alaw>>4)-1;
		linear <<= shift;
		}

	if(!sign)
		return -linear;
	else
		return linear;
	}


EXPORT uint8_t G711::ULawEncode(int16_t pcm16)
	{
	int p = pcm16;
	unsigned u;  // u-law value we are forming

	if(p<0)
		{
		// -ve value
		// Note, ones compliment is used here as this keeps encoding symetrical
		// and equal spaced around zero cross-over, (it also matches the standard).
		p = ~p;
		u = 0x80^0x10^0xff;  // Sign bit = 1 (^0x10 because this will get inverted later) ^0xff to invert final u-Law code
		}
	else
		{
		// +ve value
		u = 0x00^0x10^0xff;  // Sign bit = 0 (-0x10 because this amount extra will get added later) ^0xff to invert final u-Law code
		}

	p += 0x84; // Add uLaw bias

	if(p>0x7f00)
		p = 0x7f00;  // Clip to 15 bits

	// Calculate segment and interval numbers
	p >>= 3;	// Shift down to 13bit
	if(p>=0x100)
		{
		p >>= 4;
		u ^= 0x40;
		}
	if(p>=0x40)
		{
		p >>= 2;
		u ^= 0x20;
		}
	if(p>=0x20)
		{
		p >>= 1;
		u ^= 0x10;
		}
	// (u^0x10)&0x70 now equal to the segment value and 'p' the interval number (^0x10)

	u ^= p; // u now equal to encoded u-law value (with all bits inverted)

	return u;
	}


EXPORT int G711::ULawDecode(uint8_t ulaw)
	{
	ulaw ^= 0xff;  // u-law has all bits inverted for transmission

	int linear = ulaw&0x0f;
	linear <<= 3;
	linear |= 0x84;  // Set MSB (0x80) and a 'half' bit (0x04) to place PCM value in middle of range

	unsigned shift = ulaw>>4;
	shift &= 7;
	linear <<= shift;

	linear -= 0x84; // Subract uLaw bias
	
	if(ulaw&0x80)
		return -linear;
	else
		return linear;
	}


EXPORT uint8_t G711::ALawToULaw(uint8_t alaw)
	{
	uint8_t sign=alaw&0x80;
	alaw ^= sign;
	alaw ^= 0x55;
	unsigned ulaw;
	if(alaw<45)
		{
		if(alaw<24)
			ulaw = (alaw<8) ? (alaw<<1)+1 : alaw+8;
		else
			ulaw = (alaw<32) ? (alaw>>1)+20 : alaw+4;
		}
	else
		{
		if(alaw<63)
			ulaw = (alaw<47) ? alaw+3 : alaw+2;
		else
			ulaw = (alaw<79) ? alaw+1 : alaw;
		}
	ulaw ^= sign;
	return ulaw^0x7f;
	}


EXPORT uint8_t G711::ULawToALaw(uint8_t ulaw)
	{
	uint8_t sign=ulaw&0x80;
	ulaw ^= sign;
	ulaw ^= 0x7f;
	unsigned alaw;
	if(ulaw<48)
		{
		if(ulaw<=32)
			alaw = (ulaw<=15) ? ulaw>>1 : ulaw-8;
		else
			alaw = (ulaw<=35) ? (ulaw<<1)-40 : ulaw-4;
		}
	else
		{
		if(ulaw<=63)
			alaw = (ulaw==48) ? ulaw-3 : ulaw-2;
		else
			alaw = (ulaw<=79) ? ulaw-1 : ulaw;
		}
	alaw ^= sign;
	return alaw^0x55;
	}


EXPORT unsigned G711::ALawEncode(uint8_t* dst, int16_t* src, size_t srcSize)
	{
	srcSize >>= 1;
	uint8_t* end = dst+srcSize;
	while(dst<end)
		*dst++ = ALawEncode(*src++);
	return srcSize;
	}


EXPORT unsigned G711::ALawDecode(int16_t* dst, const uint8_t* src, size_t srcSize)
	{
	int16_t* end = dst+srcSize;
	while(dst<end)
		*dst++ = ALawDecode(*src++);
	return srcSize<<1;
	}


EXPORT unsigned G711::ULawEncode(uint8_t* dst, int16_t* src, size_t srcSize)
	{
	srcSize >>= 1;
	uint8_t* end = dst+srcSize;
	while(dst<end)
		*dst++ = ULawEncode(*src++);
	return srcSize;
	}


EXPORT unsigned G711::ULawDecode(int16_t* dst, const uint8_t* src, size_t srcSize)
	{
	int16_t* end = dst+srcSize;
	while(dst<end)
		*dst++ = ULawDecode(*src++);
	return srcSize<<1;
	}


EXPORT unsigned G711::ALawToULaw(uint8_t* dst, const uint8_t* src, size_t srcSize)
	{
	uint8_t* end = dst+srcSize;
	while(dst<end)
		*dst++ = ALawToULaw(*src++);
	return srcSize;
	}


EXPORT unsigned G711::ULawToALaw(uint8_t* dst, const uint8_t* src, size_t srcSize)
	{
	uint8_t* end = dst+srcSize;
	while(dst<end)
		*dst++ = ULawToALaw(*src++);
	return srcSize;
	}




/*************************************************************

                                                        G726
                                                        
**************************************************************/


/**
@defgroup g726_section4 Internal - Individual functions from Section 4 of G726
@ingroup g726
@{
*/


/**
@defgroup g726_section4m Internal - Range checking macros
@ingroup g726_section4
Macros for checking the range of variables used within the codec algorithm.
They are also useful as they indicate the type of the variable being checked.
@{
*/

/**
Check that a signed magnitude value lies entirely withing the given number of bits
@param x	The value
@param bits Number of bits
*/
#define CHECK_SM(x,bits) ASSERT_DEBUG(((x)>>bits)==0)

/**
Check that a unsigned magnitude value lies entirely withing the given number of bits
@param x	The value
@param bits Number of bits
*/
#define CHECK_UM(x,bits) ASSERT_DEBUG(((x)>>bits)==0)

/**
Check that a twos compliment value lies entirely withing the given number of bits
@param x	The value
@param bits Number of bits
*/
#define CHECK_TC(x,bits) ASSERT_DEBUG(((x)>>(bits-1))==((x)<0?-1:0))

/**
Check that a float value lies entirely withing the given number of bits
@param x	The value
@param bits Number of bits
*/
#define CHECK_FL(x,bits) ASSERT_DEBUG(((x)>>bits)==0)

/**
Check that an unsigned integer value lies entirely withing the given number of bits
@param x	The value
@param bits Number of bits
*/
#define CHECK_UNSIGNED(x,bits) ASSERT_DEBUG(((x)>>bits)==0)

/** @} */ // End of group


/**
@brief EXPAND function from %G726 Section 4.2.1 - Input PCM format conversion and difference signal computation
*/
static void EXPAND(unsigned S,unsigned LAW,int& SL)
	{
	CHECK_UNSIGNED(S,8);
	CHECK_UNSIGNED(LAW,1);
	int linear;
	if(LAW)
		linear = G711::ALawDecode(S);
	else
		linear = G711::ULawDecode(S);
	SL = linear>>2;
	CHECK_TC(SL,14);
	}


/**
@brief SUBTA function from %G726 Section 4.2.1 - Input PCM format conversion and difference signal computation
*/
inline static void SUBTA(int SL,int SE,int& D)
	{
	CHECK_TC(SL,14);
	CHECK_TC(SE,15);
	D = SL-SE;
	CHECK_TC(D,16);
	}


/**
@brief LOG function from %G726 Section 4.2.2 - Adaptive quantizer
*/
static void LOG(int D,unsigned& DL,int& DS)
	{
	CHECK_TC(D,16);

	DS = D>>15;

	unsigned DQM = (D<0) ? -D : D;
	DQM &= 0x7fff;

	unsigned EXP = 0;
	unsigned x = DQM;
	if(x>=(1<<8))
		{
		EXP |= 8;
		x >>= 8;
		}
	if(x>=(1<<4))
		{
		EXP |= 4;
		x >>= 4;
		}
	if(x>=(1<<2))
		{
		EXP |= 2;
		x >>= 2;
		}
	EXP |= x>>1;

	unsigned MANT = ((DQM<<7)>>EXP)&0x7f;
	DL = (EXP<<7) + MANT;

	CHECK_UM(DL,11);
	CHECK_TC(DS,1);
	}


/**
@brief QUAN function from %G726 Section 4.2.2 - Adaptive quantizer
*/
static void QUAN(unsigned RATE,int DLN,int DS,unsigned& I)
	{
	CHECK_TC(DLN,12);
	CHECK_TC(DS,1);

	int x;

	if(RATE==2)
		x = (DLN>=261);
	else
		{
		static const int16_t quan3[4] = {8,218,331,0x7fff};
		static const int16_t quan4[8] = {3972-0x1000,80,178,246,300,349,400,0x7fff};
		static const int16_t quan5[16] = {3974-0x1000,4080-0x1000,68,139,198,250,298,339,378,413,445,475,502,528,553,0x7fff};
		static const int16_t* const quan[3] = {quan3,quan4,quan5};

		const int16_t* levels = quan[RATE-3];
		const int16_t* levels0 = levels;

		while(DLN>=*levels++) {}

		x = levels-levels0-1;
		if(!x)
			x = ~DS;
		}
	int mask = (1<<RATE)-1;
	I = (x^DS)&mask;

	CHECK_UNSIGNED(I,RATE);
	}


/**
@brief SUBTB function from %G726 Section 4.2.2 - Adaptive quantizer
*/
inline static void SUBTB(unsigned DL,unsigned Y,int& DLN)
	{
	CHECK_UM(DL,11);
	CHECK_UM(Y,13);
	DLN = DL-(Y>>2);
	CHECK_TC(DLN,12);
	}


/**
@brief ADDA function from %G726 Section 4.2.3 - Inverse adaptive quantizer
*/
inline static void ADDA(int DQLN,unsigned Y,int& DQL)
	{
	CHECK_TC(DQLN,12);
	CHECK_UM(Y,13);
	DQL = DQLN+(Y>>2);
	CHECK_TC(DQL,12);
	}


/**
@brief ANTILOG function from %G726 Section 4.2.3 - Inverse adaptive quantizer
*/
inline static void ANTILOG(int DQL,int DQS,unsigned& DQ)
	{
	CHECK_TC(DQL,12);
	CHECK_TC(DQS,1);
	unsigned DEX = (DQL >> 7) & 15;
	unsigned DMN = DQL & 127;
	unsigned DQT = (1 << 7) + DMN;

	unsigned DQMAG;
	if(DQL>=0)
		DQMAG = (DQT << 7) >> (14 - DEX);
	else
		DQMAG = 0;

	DQ = DQS ? DQMAG+(1<<15) : DQMAG;

	CHECK_SM(DQ,16);
	}


/**
@brief RECONST function from %G726 Section 4.2.3 - Inverse adaptive quantizer
*/
inline static void RECONST(unsigned RATE,unsigned I,int& DQLN,int& DQS)
	{
	CHECK_UNSIGNED(I,RATE);

	// Tables 11-14
	static const int16_t reconst2[2] = {116,365};
	static const int16_t reconst3[4] = {2048-4096,135,273,373};
	static const int16_t reconst4[8] = {2048-4096,4,135,213,273,323,373,425};
	static const int16_t reconst5[16] = {2048-4096,4030-4096,28,104,169,224,274,318,358,395,429,459,488,514,539,566};
	static const int16_t* const reconst[4] = {reconst2,reconst3,reconst4,reconst5};

	int x = I;
	int m = 1<<(RATE-1);
	if(x&m)
		{
		DQS = -1;
		x = ~x;
		}
	else
		DQS = 0;
	DQLN = reconst[RATE-2][x&(m-1)];

	CHECK_TC(DQLN,12);
	CHECK_TC(DQS,1);
	}


/**
@brief FILTD function from %G726 Section 4.2.4 - Quantizer scale factor adaptation
*/
inline static void FILTD(int WI,unsigned Y,unsigned& YUT)
	{
	CHECK_TC(WI,12);
	CHECK_UM(Y,13);
	int DIF = (WI<<5)-Y;
	int DIFSX = DIF>>5;
	YUT = (Y+DIFSX); // & 8191
	CHECK_UM(YUT,13);
	}


/**
@brief FILTE function from %G726 Section 4.2.4 - Quantizer scale factor adaptation
*/
inline static void FILTE(unsigned YUP,unsigned YL,unsigned& YLP)
	{
	CHECK_UM(YUP,13);
	CHECK_UM(YL,19);
	int DIF = (YUP<<6)-YL;
	int DIFSX = DIF>>6;
	YLP = (YL+DIFSX); // & 524287
	CHECK_UM(YLP,19);
	}


/**
@brief FUNCTW function from %G726 Section 4.2.4 - Quantizer scale factor adaptation
*/
inline static void FUNCTW(unsigned RATE,unsigned I,int& WI)
	{
	CHECK_UNSIGNED(I,RATE);

	static const int16_t functw2[2] = {4074-4096,439};
	static const int16_t functw3[4] = {4092-4096,30,137,582};
	static const int16_t functw4[8] = {4084-4096,18,41,64,112,198,355,1122};
	static const int16_t functw5[16] = {14,14,24,39,40,41,58,100,141,179,219,280,358,440,529,696};
	static const int16_t* const functw[4] = {functw2,functw3,functw4,functw5};

	unsigned signMask = 1<<(RATE-1);
	unsigned n = (I&signMask) ? (2*signMask-1)-I : I;
	WI = functw[RATE-2][n];

	CHECK_TC(WI,12);
	}


/**
@brief LIMB function from %G726 Section 4.2.4 - Quantizer scale factor adaptation
*/
inline static void LIMB(unsigned YUT,unsigned& YUP)
	{
	CHECK_UM(YUT,13);
	unsigned GEUL = (YUT+11264)&(1<<13);
	unsigned GELL = (YUT+15840)&(1<<13);
	if(GELL)
		YUP = 544;
	else if (!GEUL)
		YUP = 5120;
	else
		YUP = YUT;
	CHECK_UM(YUP,13);
	}


/**
@brief MIX function from %G726 Section 4.2.4 - Quantizer scale factor adaptation
*/
inline static void MIX(unsigned AL,unsigned YU,unsigned YL,unsigned& Y)
	{
	CHECK_UM(AL,7);
	CHECK_UM(YU,13);
	CHECK_UM(YL,19);
	int DIF = YU-(YL>>6);
	int PROD = DIF*AL;
	if(DIF<0) PROD += (1<<6)-1; // Force round towards zero for following shift
	PROD >>= 6;
	Y = ((YL>>6)+PROD); // & 8191;
	CHECK_UM(Y,13);
	}


/**
@brief FILTA function from %G726 Section 4.2.5 - Adaptation speed control
*/
inline static void FILTA(unsigned FI,unsigned DMS,unsigned& DMSP)
	{
	CHECK_UM(FI,3);
	CHECK_UM(DMS,12);
	int DIF = (FI<<9)-DMS;
	int DIFSX = (DIF>>5);
	DMSP = (DIFSX+DMS); // & 4095;
	CHECK_UM(DMSP,12);
	}


/**
@brief FILTB function from %G726 Section 4.2.5 - Adaptation speed control
*/
inline static void FILTB(unsigned FI,unsigned DML,unsigned& DMLP)
	{
	CHECK_UM(FI,3);
	CHECK_UM(DML,14);
	int DIF = (FI<<11)-DML;
	int DIFSX = (DIF>>7);
	DMLP = (DIFSX+DML); // & 16383;
	CHECK_UM(DMLP,14);
	}


/**
@brief FILTC function from %G726 Section 4.2.5 - Adaptation speed control
*/
inline static void FILTC(unsigned AX,unsigned AP,unsigned& APP)
	{
	CHECK_UM(AX,1);
	CHECK_UM(AP,10);
	int DIF = (AX<<9)-AP;
	int DIFSX = (DIF>>4);
	APP = (DIFSX+AP); // & 1023;
	CHECK_UM(APP,10);
	}


/**
@brief FUNCTF function from %G726 Section 4.2.5 - Adaptation speed control
*/
inline static void FUNCTF(unsigned RATE,unsigned I,unsigned& FI)
	{
	CHECK_UNSIGNED(I,RATE);

	static const int16_t functf2[2] = {0,7};
	static const int16_t functf3[4] = {0,1,2,7};
	static const int16_t functf4[8] = {0,0,0,1,1,1,3,7};
	static const int16_t functf5[16] = {0,0,0,0,0,1,1,1,1,1,2,3,4,5,6,6};
	static const int16_t* const functf[4] = {functf2,functf3,functf4,functf5};

	unsigned x = I;
	int mask=(1<<(RATE-1));
	if(x&mask)
		x = ~x;
	x &= mask-1;
	FI = functf[RATE-2][x];

	CHECK_UM(FI,3);
	}


/**
@brief LIMA function from %G726 Section 4.2.5 - Adaptation speed control
*/
inline static void LIMA(unsigned AP,unsigned& AL)
	{
	CHECK_UM(AP,10);
	AL = (AP>256) ? 64 : AP>>2;
	CHECK_UM(AL,7);
	}


/**
@brief SUBTC function from %G726 Section 4.2.5 - Adaptation speed control
*/
inline static void SUBTC(unsigned DMSP,unsigned DMLP,unsigned TDP,unsigned Y,unsigned& AX)
	{
	CHECK_UM(DMSP,12);
	CHECK_UM(DMLP,14);
	CHECK_UNSIGNED(TDP,1);
	CHECK_UM(Y,13);
	int DIF = (DMSP<<2)-DMLP;
	unsigned DIFM;
	if(DIF<0)
		DIFM = -DIF;
	else
		DIFM = DIF;
	unsigned DTHR = DMLP >> 3;
	AX = (Y>=1536 && DIFM<DTHR) ? TDP : 1;
	CHECK_UM(AX,1);
	}


/**
@brief TRIGA function from %G726 Section 4.2.5 - Adaptation speed control
*/
inline static void TRIGA(unsigned TR,unsigned APP,unsigned& APR)
	{
	CHECK_UNSIGNED(TR,1);
	CHECK_UM(APP,10);
	APR = TR ? 256 : APP;
	CHECK_UM(APR,10);
	}

/**
@brief ACCUM function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline static void ACCUM(int WAn[2],int WBn[6],int& SE,int& SEZ)
	{
	CHECK_TC(WAn[0],16);
	CHECK_TC(WAn[1],16);
	CHECK_TC(WBn[0],16);
	CHECK_TC(WBn[1],16);
	CHECK_TC(WBn[2],16);
	CHECK_TC(WBn[3],16);
	CHECK_TC(WBn[4],16);
	CHECK_TC(WBn[5],16);
	int16_t SEZI = (int16_t)(WBn[0]+WBn[1]+WBn[2]+WBn[3]+WBn[4]+WBn[5]);
	int16_t SEI = (int16_t)(SEZI+WAn[0]+WAn[1]);
	SEZ = SEZI >> 1;
	SE = SEI >> 1;	
	CHECK_TC(SE,15);
	CHECK_TC(SEZ,15);
	}


/**
@brief ACCUM function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline static void ADDB(unsigned DQ,int SE,int& SR)
	{
	CHECK_SM(DQ,16);
	CHECK_TC(SE,15);
	int DQI;
	if(DQ&(1<<15))
		DQI = (1<<15)-DQ;
	else
		DQI = DQ;
	SR = (int16_t)(DQI+SE);
	CHECK_TC(SR,16);
	}


/**
@brief ADDC function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline static void ADDC(unsigned DQ,int SEZ,int& PK0,unsigned& SIGPK)
	{
	CHECK_SM(DQ,16);
	CHECK_TC(SEZ,15);
	int DQI;
	if(DQ&(1<<15))
		DQI = (1<<15)-DQ;
	else
		DQI = DQ;
	int DQSEZ = (int16_t)(DQI+SEZ);
	PK0 = DQSEZ>>15;
	SIGPK = DQSEZ ? 0 : 1;
	CHECK_TC(PK0,1);
	CHECK_UNSIGNED(SIGPK,1);
	}


static void MagToFloat(unsigned mag,unsigned& exp,unsigned& mant)
	{
	unsigned e = 0;
	unsigned m = mag<<1;
	if(m>=(1<<8))
		{
		e |= 8;
		m >>= 8;
		}
	if(m>=(1<<4))
		{
		e |= 4;
		m >>= 4;
		}
	if(m>=(1<<2))
		{
		e |= 2;
		m >>= 2;
		}
	e |= m>>1;
	exp = e;
	mant = mag ? (mag<<6)>>e : 1<<5;
	}


/**
@brief FLOATA function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline static void FLOATA(unsigned DQ, unsigned& DQ0)
	{
	CHECK_SM(DQ,16);
	unsigned DQS = (DQ>>15);
	unsigned MAG = DQ&32767;
	unsigned EXP;
	unsigned MANT;
	MagToFloat(MAG,EXP,MANT);
	DQ0 = (DQS<<10) + (EXP<<6) + MANT;
	CHECK_FL(DQ0,11);
	}


/**
@brief FLOATB function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline static void FLOATB(int SR, unsigned& SR0)
	{
	CHECK_TC(SR,16);
	unsigned SRS = (SR>>15)&1;
	unsigned MAG = SRS ? (-SR)&32767 : SR;
	unsigned EXP;
	unsigned MANT;
	MagToFloat(MAG,EXP,MANT);
	SR0 = (SRS<<10) + (EXP<<6) + MANT;
	CHECK_FL(SR0,11);
	}


/**
@brief FMULT function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
static void FMULT(int An,unsigned SRn,int& WAn)
	{
	CHECK_TC(An,16);
	CHECK_FL(SRn,11);
	unsigned AnS = (An>>15)&1;
	unsigned AnMAG = AnS ? (-(An>>2))&8191 : An>>2;
	unsigned AnEXP;
	unsigned AnMANT;
	MagToFloat(AnMAG,AnEXP,AnMANT);

	unsigned SRnS = SRn>>10;
	unsigned SRnEXP = (SRn>>6) & 15;
	unsigned SRnMANT = SRn&63;

	unsigned WAnS = SRnS^AnS;
	unsigned WAnEXP = SRnEXP+AnEXP;
	unsigned WAnMANT = ((SRnMANT*AnMANT)+48)>>4;
	unsigned WAnMAG;
	if(WAnEXP<=26)
		WAnMAG = (WAnMANT<<7) >> (26-WAnEXP);
	else
		WAnMAG = ((WAnMANT<<7) << (WAnEXP-26)) & 32767;
	WAn = WAnS ? -(int)WAnMAG : WAnMAG;
	CHECK_TC(WAn,16);
	}


/**
@brief LIMC function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline static void LIMC(int A2T,int& A2P)
	{
	CHECK_TC(A2T,16);
	const int A2UL = 12288;
	const int A2LL = 53248-65536;
	if(A2T<=A2LL)
		A2P = A2LL;
	else if(A2T>=A2UL)
		A2P = A2UL;
	else
		A2P = A2T;
	CHECK_TC(A2P,16);
	}


/**
@brief LIMD function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline static void LIMD(int A1T,int A2P,int& A1P)
	{
	CHECK_TC(A1T,16);
	CHECK_TC(A2P,16);
	const int OME = 15360;
	int A1UL = (int16_t)(OME-A2P);
	int A1LL = (int16_t)(A2P-OME);
	if(A1T<=A1LL)
		A1P = A1LL;
	else if(A1T>=A1UL)
		A1P = A1UL;
	else
		A1P = A1T;
	CHECK_TC(A1P,16);
	}


/**
@brief TRIGB function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline static void TRIGB(unsigned TR,int AnP,int& AnR)
	{
	CHECK_UNSIGNED(TR,1);
	CHECK_TC(AnP,16);
	AnR = TR ? 0 : AnP;
	CHECK_TC(AnR,16);
	}


/**
@brief UPA1 function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline static void UPA1(int PK0,int PK1,int A1,unsigned SIGPK,int& A1T)
	{
	CHECK_TC(PK0,1);
	CHECK_TC(PK1,1);
	CHECK_TC(A1,16);
	CHECK_UNSIGNED(SIGPK,1);
	int UGA1;
	if(SIGPK==0)
		{
		if(PK0^PK1)
			UGA1 = -192;
		else
			UGA1 = 192;
		}
	else
		UGA1 = 0;
	A1T = (int16_t)(A1+UGA1-(A1>>8));
	CHECK_TC(A1T,16);
	}


/**
@brief UPA2 function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline static void UPA2(int PK0,int PK1,int PK2,int A1,int A2,unsigned SIGPK,int& A2T)
	{
	CHECK_TC(PK0,1);
	CHECK_TC(PK1,1);
	CHECK_TC(PK2,1);
	CHECK_TC(A1,16);
	CHECK_TC(A2,16);
	CHECK_UNSIGNED(SIGPK,1);

	unsigned UGA2;
	if(SIGPK==0)
		{
		int UGA2A = (PK0^PK2) ? -16384 : 16384;

		int FA1;
		if(A1<-8191)	 FA1 = -8191<<2;
		else if(A1>8191) FA1 = 8191<<2;
		else			 FA1 = A1<<2;

		int FA = (PK0^PK1) ? FA1 : -FA1;

		UGA2 = (UGA2A+FA) >> 7;
		}
	else
		UGA2 = 0;
	A2T = (int16_t)(A2+UGA2-(A2>>7));
		
	CHECK_TC(A2T,16);
	}


/**
@brief UPB function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline static void UPB(unsigned RATE,int Un,int Bn,unsigned DQ,int& BnP)
	{
	CHECK_TC(Un,1);
	CHECK_TC(Bn,16);
	CHECK_SM(DQ,16);
	int UGBn;
	if(DQ&32767)
		UGBn = Un ? -128 : 128;
	else
		UGBn = 0;
	int ULBn = (RATE==5) ? Bn>>9 : Bn>>8;
	BnP = (int16_t)(Bn+UGBn-ULBn);
	CHECK_TC(BnP,16);
	}


/**
@brief XOR function from %G726 Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline static void XOR(unsigned DQn,unsigned DQ,int& Un)
	{
	CHECK_FL(DQn,11);
	CHECK_SM(DQ,16);
	Un = -(int)((DQn>>10)^(DQ>>15));
	CHECK_TC(Un,1);
	}


/**
@brief TONE function from %G726 Section 4.2.7 - Tone and transition detector
*/
inline static void TONE(int A2P,unsigned& TDP)
	{
	CHECK_TC(A2P,16);
	TDP = A2P<(53760-65536);
	CHECK_UNSIGNED(TDP,1);
	}


/**
@brief TRANS function from %G726 Section 4.2.7 - Tone and transition detector
*/
inline static void TRANS(unsigned TD,unsigned YL,unsigned DQ,unsigned& TR)
	{
	CHECK_UNSIGNED(TD,1);
	CHECK_UM(YL,19);
	CHECK_SM(DQ,16);
	unsigned DQMAG = DQ&32767;
	unsigned YLINT = YL>>15;
	unsigned YLFRAC = (YL>>10) & 31;
	unsigned THR1 = (32+YLFRAC)<<YLINT;
	unsigned THR2;
	if(YLINT>9)
		THR2 = 31<<10;
	else
		THR2 = THR1;
	unsigned DQTHR = (THR2+(THR2>>1)) >> 1;
	TR = DQMAG>DQTHR && TD;
	CHECK_UNSIGNED(TR,1);
	}


/**
@brief COMPRESS function from %G726 Section 4.2.8 - Output PCM format conversion and synchronous coding adjustment
*/
inline static void COMPRESS(int SR,unsigned LAW,unsigned& SP)
	{
	CHECK_TC(SR,16);
	CHECK_UNSIGNED(LAW,1);
	int x = SR;

#ifdef IMPLEMENT_G191_BUGS
	// reproduce bugs in G191 reference implementation...
	if(x==-0x8000)
		x=-1; 
	else
		if(!LAW && x<0) x--;
#endif

	// Clamp to 14 bits...
	if(x>=(1<<13))
		x = (1<<13)-1;
	else if(x<-(1<<13))
		x = -(1<<13);

	if(LAW)
		SP = G711::ALawEncode(x<<2);
	else
		SP = G711::ULawEncode(x<<2);

	CHECK_UNSIGNED(SP,8);
	}


/**
@brief SYNC function from %G726 Section 4.2.8 - Output PCM format conversion and synchronous coding adjustment
*/
inline static void SYNC(unsigned RATE,unsigned I,unsigned SP,int DLNX,int DSX,unsigned LAW,unsigned& SD)
	{
	CHECK_UNSIGNED(SP,8);
	CHECK_TC(DLNX,12);
	CHECK_TC(DSX,1);
	CHECK_UNSIGNED(LAW,1);

	unsigned ID;
	unsigned IM;
	QUAN(RATE,DLNX,DSX,ID);
	unsigned signMask = 1<<(RATE-1);
	ID = ID^signMask;
	IM = I^signMask;

	unsigned s;
	if(LAW)
		{
		s = SP^0x55;
		if(!(s&0x80))
			s = s^0x7f;
		}
	else
		{
		s = SP;
		if(s&0x80)
			s = s^0x7f;
		}
	// s = ALAW/uLAW code converted to range 0x00..0xff
	// where 0x00 is most-negative and 0xff is most-positive

	if(ID<IM)
		{
		if(s<0xff)
			++s;
		}
	else if(ID>IM)
		{
		if(s>0x00)
			{
			--s;
			if(s==0x7f && !LAW) --s; // TABLE 20/G.726 says uLaw 0xFF decrements to 0x7e and not 0x7f (!?)
			}
		}

	// convert s back to ALAW/uLAW code
	if(LAW)
		{
		if(!(s&0x80))
			s = s^0x7f;
		s = s^0x55;
		}
	else
		{
		if(s&0x80)
			s = s^0x7f;
		}
	SD = s;
	CHECK_UNSIGNED(SD,8);
	}


/**
@brief LIMO function from %G726 Section A.3.5 - Output limiting (decoder only)
*/
inline static void LIMO(int SR,int& SO)
	{
	CHECK_TC(SR,16);
	if(SR>=(1<<13))
		SO = (1<<13)-1;
	else if(SR<-(1<<13))
		SO = -(1<<13);
	else
		SO = SR;
	CHECK_TC(SO,14);
	}


/** @} */ // End of group


/**
@defgroup g726_section4B Internal - Functional blocks from Section 4 of G726
@ingroup g726
Some of these have been broken into two parts, the first of which can generate
it's outputs using only the saved internal state from the previous itteration
of the algorithm.
@{
*/

/**
@brief FIGURE 4/G.726 from Section 4.2.1 - Input PCM format conversion and difference signal computation
*/
inline void G726::InputPCMFormatConversionAndDifferenceSignalComputation(unsigned S,int SE,int& D)
	{
	int SL;
	EXPAND(S,LAW,SL);
	SUBTA(SL,SE,D);
	}


/**
@brief FIGURE 5/G.726 from Section 4.2.2 - Adaptive quantizer
*/
inline void G726::AdaptiveQuantizer(int D,unsigned Y,unsigned& I)
	{
	unsigned DL;
	int DS;
	LOG(D,DL,DS);

	int DLN;
	SUBTB(DL,Y,DLN);

	QUAN(RATE,DLN,DS,I);
	}


/**
@brief FIGURE 6/G.726 from Section 4.2.3 - Inverse adaptive quantizer
*/
inline void G726::InverseAdaptiveQuantizer(unsigned I,unsigned Y,unsigned& DQ)
	{
	int DQLN;
	int DQS;
	RECONST(RATE,I,DQLN,DQS);

	int DQL;
	ADDA(DQLN,Y,DQL);

	ANTILOG(DQL,DQS,DQ);
	}


/**
@brief FIGURE 7/G.726 (Part 1) from Section 4.2.4 - Quantizer scale factor adaptation
*/
inline void G726::QuantizerScaleFactorAdaptation1(unsigned AL,unsigned& Y)
	{
	MIX(AL,YU,YL,Y);
	}
/**
@brief FIGURE 7/G.726 (Part 2) from Section 4.2.4 - Quantizer scale factor adaptation
*/
inline void G726::QuantizerScaleFactorAdaptation2(unsigned I,unsigned Y)
	{
	int WI;
	FUNCTW(RATE,I,WI);

	unsigned YUT;
	FILTD(WI,Y,YUT);

	unsigned YUP;
	LIMB(YUT,YUP);

	unsigned YLP;
	FILTE(YUP,YL,YLP);

	YU = YUP; // Delay
	YL = YLP; // Delay
	}


/**
@brief FIGURE 8/G.726 (Part 1) from Section 4.2.5 - Adaptation speed control
*/
inline void G726::AdaptationSpeedControl1(unsigned& AL)
	{
	LIMA(AP,AL);
	}
/**
@brief FIGURE 8/G.726 (Part 2) from Section 4.2.5 - Adaptation speed control
*/
inline void G726::AdaptationSpeedControl2(unsigned I,unsigned Y,unsigned TDP,unsigned TR)
	{
	unsigned FI;
	FUNCTF(RATE,I,FI);

	FILTA(FI,DMS,DMS); // Result 'DMSP' straight to delay storage 'DMS'
	FILTB(FI,DML,DML); // Result 'DMSP' straight to delay storage 'DMS'

	unsigned AX;
	SUBTC(DMS,DML,TDP,Y,AX); // DMSP and DMLP are read from delay storage 'DMS' and 'DML'

	unsigned APP;
	FILTC(AX,AP,APP);

	TRIGA(TR,APP,AP); // Result 'APR' straight to delay storage 'AP'
	}


/**
@brief FIGURE 9/G.726 (Part1) from Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline void G726::AdaptativePredictorAndReconstructedSignalCalculator1(int& SE,int& SEZ)
	{
	int WBn[6];
	for(int i=0; i<6; i++)
		FMULT(Bn[i],DQn[i],WBn[i]);
	int WAn[2];
	FMULT(A1,SR1,WAn[0]);
	FMULT(A2,SR2,WAn[1]);
	ACCUM(WAn,WBn,SE,SEZ);
	}
/**
@brief FIGURE 9/G.726 (Part2) from Section 4.2.6 - Adaptative predictor and reconstructed signal calculator
*/
inline void G726::AdaptativePredictorAndReconstructedSignalCalculator2(unsigned DQ,unsigned TR,int SE,int SEZ,int& SR,int& A2P)
	{
	int PK0;
	unsigned SIGPK;
	ADDC(DQ,SEZ,PK0,SIGPK);

	ADDB(DQ,SE,SR);
	SR2 = SR1; // Delay
	FLOATB(SR,SR1);  // Result 'SR0' straight to delay storage 'SR1'

	unsigned DQ0;
	FLOATA(DQ,DQ0);

	int i;
	for(i=0; i<6; i++)
		{
		int Un;
		XOR(DQn[i],DQ,Un);
		int BnP;
		UPB(RATE,Un,Bn[i],DQ,BnP);
		TRIGB(TR,BnP,Bn[i]); // Result 'BnR' straight to delay storage 'Bn'
		}

	int A2T;
	UPA2(PK0,PK1,PK2,A1,A2,SIGPK,A2T);
	LIMC(A2T,A2P);
	TRIGB(TR,A2P,A2); // Result 'A2R' straight to delay storage 'A2'

	int A1T;
	UPA1(PK0,PK1,A1,SIGPK,A1T);
	int A1P;
	LIMD(A1T,A2P,A1P);
	TRIGB(TR,A1P,A1); // Result 'A1R' straight to delay storage 'A1'

	PK2 = PK1; // Delay
	PK1 = PK0; // Delay

	for(i=5; i>0; i--)
		DQn[i] = DQn[i-1]; // Delay
	DQn[0] = DQ0; // Delay
	}


/**
@brief FIGURE 10/G.726 (Part 1) from Section 4.2.7 - Tone and transition detector
*/
inline void G726::ToneAndTransitionDetector1(unsigned DQ,unsigned& TR)
	{
	TRANS(TD,YL,DQ,TR);
	}
/**
@brief FIGURE 10/G.726 (Part 2) from Section 4.2.7 - Tone and transition detector
*/
inline void G726::ToneAndTransitionDetector2(int A2P,unsigned TR,unsigned& TDP)
	{
	TONE(A2P,TDP);
	TRIGB(TR,TDP,(int&)TD);  // Result 'TDR' straight to delay storage 'TD'
	}


/**
@brief FIGURE 11/G.726 from Section 4.2.8 - Output PCM format conversion and synchronous coding adjustment
*/
inline void G726::OutputPCMFormatConversionAndSynchronousCodingAdjustment(int SR,int SE,unsigned Y,unsigned I,unsigned& SD)
	{
	unsigned SP;
	COMPRESS(SR,LAW,SP);
	int SLX;
	EXPAND(SP,LAW,SLX);
	int DX;
	SUBTA(SLX,SE,DX);
	unsigned DLX;
	int DSX;
	LOG(DX,DLX,DSX);
	int DLNX;
	SUBTB(DLX,Y,DLNX);
	SYNC(RATE,I,SP,DLNX,DSX,LAW,SD);
	}


/**
@brief FIGURE A.4/G.726 from Section A.3.3 - Difference signal computation
*/
inline void G726::DifferenceSignalComputation(int SL,int SE,int& D)
	{
	SUBTA(SL,SE,D);
	}


/**
@brief FIGURE A.5/G.726 from Section A.3.5 - Output limiting (decoder only)
*/
inline void G726::OutputLimiting(int SR,int& SO)
	{
	LIMO(SR,SO);
	}


/** @} */ // End of group


/**
@brief The top level method which implements the complete algorithm for both
encoding and decoding.
@param input Either the PCM input to the encoder or the ADPCM input to the decoder.
@param encode A flag which if true makes this method perform the encode function.
			  If the flag is false then the decode function is performed.
@return Either the ADPCM output to the encoder or the PCM output to the decoder.
*/
unsigned G726::EncodeDecode(unsigned input,bool encode)
	{
	unsigned AL;
	AdaptationSpeedControl1(AL);
	unsigned Y;
	QuantizerScaleFactorAdaptation1(AL,Y);
	int SE;
	int SEZ;
	AdaptativePredictorAndReconstructedSignalCalculator1(SE,SEZ);

	unsigned I;
	if(encode)
		{
		int D;
		//if(LAW==G726::PCM16)   //caorr
		if(LAW==PCM16)
			{
			int SL = (int16_t)input;
			SL >>= 2; // Convert input from 16bit to 14bit
			DifferenceSignalComputation(SL,SE,D);
			}
		else
			InputPCMFormatConversionAndDifferenceSignalComputation(input,SE,D);
		AdaptiveQuantizer(D,Y,I);
		}
	else
		I = input;

	unsigned DQ;
	InverseAdaptiveQuantizer(I,Y,DQ);
	unsigned TR;
	ToneAndTransitionDetector1(DQ,TR);
	int SR;
	int A2P;
	AdaptativePredictorAndReconstructedSignalCalculator2(DQ,TR,SE,SEZ,SR,A2P);
	unsigned TDP;
	ToneAndTransitionDetector2(A2P,TR,TDP);
	AdaptationSpeedControl2(I,Y,TDP,TR);
	QuantizerScaleFactorAdaptation2(I,Y);

	if(encode)
		return I;

	//if(LAW==G726::PCM16) //caorr
	if(LAW==PCM16)
		{
		int SO;
		OutputLimiting(SR,SO);
		return SO<<2; // Convert result from 14bit to 16 bit
		}
	else
		{
		unsigned SD;
		OutputPCMFormatConversionAndSynchronousCodingAdjustment(SR,SE,Y,I,SD);
		return SD;
		}
	}


/*
Public members of class G726
*/


EXPORT void G726::Reset()
	{
	int i;
	for(i=0; i<6; i++)
		{
		Bn[i] = 0;
		DQn[i] = 32;
		}
	A1 = 0;
	A2 = 0;
	AP = 0;
	DML = 0;
	DMS = 0;
	PK1 = 0;
	PK2 = 0;
	SR1 = 32;
	SR2 = 32;
	TD = 0;
	YL = 34816;
	YU = 544;
	}


EXPORT void G726::SetLaw(Law law)
	{
	ASSERT_DEBUG((unsigned)law<=PCM16);
	LAW = law;
	}


EXPORT void G726::SetRate(Rate rate)
	{
	ASSERT_DEBUG((unsigned)(rate-Rate16kBits)<=(unsigned)(Rate40kBits-Rate16kBits));
	RATE = rate;
	}


EXPORT unsigned G726::Encode(unsigned S)
	{
	return EncodeDecode(S,true);
	}


EXPORT unsigned G726::Decode(unsigned I)
	{
	I &= (1<<RATE)-1; // Mask off un-needed bits
	return EncodeDecode(I,false);
	}


EXPORT unsigned G726::Encode(void* dst, int dstOffset, const void* src, size_t srcSize)
	{
	// convert pointers into more useful types
	uint8_t* out = (uint8_t*)dst;
	union
		{
	const uint8_t* ptr8;
	const uint16_t* ptr16;
		}
	in;
	in.ptr8 = (const uint8_t*)src;

	// use given bit offset
	out += dstOffset>>3;
	unsigned bitOffset = dstOffset&7;

	unsigned bits = RATE;			// bits per adpcm sample
	unsigned mask = (1<<bits)-1;	// bitmask for an adpcm sample

	// calculate number of bits to be written
	unsigned outBits;
	if(LAW!=PCM16)
		outBits = bits*srcSize;
	else
		{
		outBits = bits*(srcSize>>1);
		srcSize &= ~1; // make sure srcSize represents a whole number of samples
		}

	// calculate end of input buffer
	const uint8_t* end = in.ptr8+srcSize;

	while(in.ptr8<end)
		{
		// read a single PCM value from input
		unsigned pcm;
		if(LAW==PCM16)
			pcm = *in.ptr16++;
		else
			pcm = *in.ptr8++;

		// encode the pcm value as an adpcm value
		unsigned adpcm = Encode(pcm);
		// shift it to the required output position
		adpcm <<= bitOffset;

		// write adpcm value to buffer...
		unsigned b = *out;			// get byte from ouput
		b &= ~(mask<<bitOffset);	// clear bits which we want to write to
		b |= adpcm;					// or in adpcm value
		*out = (uint8_t)b;			// write value back to output

		// update bitOffset for next adpcm value
		bitOffset += bits;

		// loop if not moved on to next byte
		if(bitOffset<8)
			continue;

		// move pointers on to next byte
		++out;
		bitOffset &= 7;

		// write any left-over bits from the last adpcm value
		if(bitOffset)
			*out = (uint8_t)(adpcm>>8);
		}

	// return number bits written to dst
	return outBits;
	}


EXPORT unsigned G726::Decode(void* dst, const void* src, int srcOffset, unsigned srcSize)
	{
	// convert pointers into more useful types
	union
		{
	uint8_t* ptr8;
	uint16_t* ptr16;
		}
	out;
	out.ptr8 = (uint8_t*)dst;
	const uint8_t* in = (const uint8_t*)src;

	// use given bit offset
	in += srcOffset>>3;
	unsigned bitOffset = srcOffset&7;

	unsigned bits = RATE;		// bits per adpcm sample

	while(srcSize>=bits)
		{
		// read adpcm value from input
		unsigned adpcm = *in;
		if(bitOffset+bits>8)
			adpcm |= in[1]<<8;	// need bits from next byte as well

		// allign adpcm value to bit 0
		adpcm >>= bitOffset;

		// decode the adpcm value into a pcm value
		unsigned pcm = Decode(adpcm);

		// write pcm value to output
		if(LAW==PCM16)
			*out.ptr16++ = (int16_t)pcm;
		else
			*out.ptr8++ = (uint8_t)pcm;

		// update bit values for next adpcm value
		bitOffset += bits;
		srcSize -= bits;

		// move on to next byte of input if required
		if(bitOffset>=8)
			{
			bitOffset &= 7;
			++in;
			}
		}

	// return number of bytes written to dst
	return out.ptr8-(uint8_t*)dst;
	}


