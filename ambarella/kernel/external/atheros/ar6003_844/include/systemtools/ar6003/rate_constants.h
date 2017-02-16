
#ifndef __INCrateconstantsh
#define __INCrateconstantsh

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
    RATE_INDEX_6=0,
    RATE_INDEX_9,
    RATE_INDEX_12,
    RATE_INDEX_18,
    RATE_INDEX_24,
    RATE_INDEX_36,
    RATE_INDEX_48,
    RATE_INDEX_54,
    RATE_INDEX_1L,
    RATE_INDEX_2L,
    RATE_INDEX_2S,
    RATE_INDEX_5L,
    RATE_INDEX_5S,
    RATE_INDEX_11L,
    RATE_INDEX_11S,
    RATE_INDEX_HT20_MCS0=32,
    RATE_INDEX_HT20_MCS1,
    RATE_INDEX_HT20_MCS2,
    RATE_INDEX_HT20_MCS3,
    RATE_INDEX_HT20_MCS4,
    RATE_INDEX_HT20_MCS5,
    RATE_INDEX_HT20_MCS6,
    RATE_INDEX_HT20_MCS7,
    RATE_INDEX_HT20_MCS8,
    RATE_INDEX_HT20_MCS9,
    RATE_INDEX_HT20_MCS10,
    RATE_INDEX_HT20_MCS11,
    RATE_INDEX_HT20_MCS12,
    RATE_INDEX_HT20_MCS13,
    RATE_INDEX_HT20_MCS14,
    RATE_INDEX_HT20_MCS15,
    RATE_INDEX_HT40_MCS0=48,
    RATE_INDEX_HT40_MCS1,
    RATE_INDEX_HT40_MCS2,
    RATE_INDEX_HT40_MCS3,
    RATE_INDEX_HT40_MCS4,
    RATE_INDEX_HT40_MCS5,
    RATE_INDEX_HT40_MCS6,
    RATE_INDEX_HT40_MCS7,
    RATE_INDEX_HT40_MCS8,
    RATE_INDEX_HT40_MCS9,
    RATE_INDEX_HT40_MCS10,
    RATE_INDEX_HT40_MCS11,
    RATE_INDEX_HT40_MCS12,
    RATE_INDEX_HT40_MCS13,
    RATE_INDEX_HT40_MCS14,
    RATE_INDEX_HT40_MCS15,
    RATE_INDEX_MAX,
}RATE_INDEX ;

//const A_UINT16 numRateCodes = sizeof(rateCodes)/sizeof(A_UCHAR);
#define numRateCodes 64

extern const A_UCHAR rateValues[numRateCodes];
extern const A_UCHAR rateCodes[numRateCodes];

#define UNKNOWN_RATE_CODE 0xff
#define IS_2STREAM_RATE_INDEX(x) (((x) >= 40 && (x) <= 47) || ((x) >= 56 && (x) <= 63))
#define IS_HT40_RATE_INDEX(x)    ((x) >= 48 && (x) <= 63)

extern A_UINT32 descRate2bin(A_UINT32 descRateCode);
extern A_UINT32 descRate2RateIndex(A_UINT32 descRateCode, A_BOOL ht40);
extern A_UINT32 rate2bin(A_UINT32 rateCode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

