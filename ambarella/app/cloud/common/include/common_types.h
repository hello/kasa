/*******************************************************************************
 * common_types.h
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#ifndef __COMMON_TYPES_H__
#define __COMMON_TYPES_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

#pragma once

//-----------------------------------------------------------------------
//
//  General types
//
//-----------------------------------------------------------------------

typedef int TInt;
typedef unsigned int TUint;

typedef unsigned char TU8;
typedef unsigned short TU16;
typedef unsigned int TU32;

typedef signed char TS8;
typedef signed short TS16;
typedef signed int TS32;

typedef signed long long TS64;
typedef unsigned long long TU64;

typedef long TLong;
typedef unsigned long TULong;

typedef TS64        TTime;
typedef char TChar;
typedef unsigned char TUChar;

typedef TS64 TFileSize;
typedef TS64 TIOSize;

typedef volatile int TAtomic;

typedef TULong TMemSize;
typedef TULong TPointer;
typedef TULong TFlag;

typedef TU64 TUniqueID;
#define DMainIDMask 0xffffffff00000000LL
#define DMainIDShift 32
#define DExtIDDataNodeMask 0xffff0000
#define DExtIDDataNodeShift 16
#define DExtIDCompanyMask 0x0c00
#define DExtIDCompanyShift 10
#define DExtIDTypeMask 0x0300
#define DExtIDTypeShift 8

#define DExtIDIndexMask 0xff
#define DExtIDIndexShift 0
#define DInvalidUniqueID 0x0

#define DDefaultTimeScale 90000
#define DDefaultVideoFramerateNum 90000
#ifdef BUILD_OS_WINDOWS
#define DDefaultVideoFramerateDen 3003
#define DDefaultAudioSampleRate 44100
#else
#define DDefaultVideoFramerateDen 3003
#define DDefaultAudioSampleRate 48000
#endif
#define DDefaultAudioFrameSize 1024
#define DDefaultAudioChannelNumber 1
#define DDefaultAudioBufferNumber 24
#define DDefaultAudioOutputBufferFrameCount 48

#define DDefaultAVSYNCAudioLatencyFrameCount 2
#define DDefaultAVSYNCVideoLatencyFrameCount 0
#define DDefaultAVSYNCVideoMaxAccumulatedFrameCount 4

typedef TInt THandler;

typedef TS32 TDimension;

typedef TInt TSocketSize;
typedef TU16 TSocketPort;

#define DInvalidTimeStamp (-1LL)

typedef struct
{
    TDimension width;
    TDimension height;
    TDimension pos_x;
    TDimension pos_y;
} SRect;

typedef struct
{
    float x;
    float y;
} SVertext2F;

typedef struct
{
    float x;
    float y;
    float z;
} SVertext3F;

typedef struct
{
    SVertext2F vert[4];
} SQuanVertext2F;

typedef struct
{
    SVertext3F vert[4];
} SQuanVertext3F;

//-----------------------------------------------------------------------
//
// constant
//
//-----------------------------------------------------------------------
#define D_M_PI        3.14159265358979323846

//-----------------------------------------------------------------------
//
// Macros
//
//-----------------------------------------------------------------------

// align must be power of 2
#ifndef DROUND_UP
#define DROUND_UP(_size, _align)    (((_size) + ((_align) - 1)) & ~((_align) - 1))
#endif

#ifndef DROUND_DOWN
#define DROUND_DOWN(_size, _align)  ((_size) & ~((_align) - 1))
#endif

#ifndef DCAL_MIN
#define DCAL_MIN(_a, _b)    ((_a) < (_b) ? (_a) : (_b))
#endif

#ifndef DCAL_MAX
#define DCAL_MAX(_a, _b)    ((_a) > (_b) ? (_a) : (_b))
#endif

#ifndef DARRAY_SIZE
#define DARRAY_SIZE(_array) (sizeof(_array) / sizeof(_array[0]))
#endif

#ifndef DPTR_OFFSET
#define DPTR_OFFSET(_type, _member) ((int)&((_type*)0)->member)
#endif

#ifndef DPTR_ADD
#define DPTR_ADD(_ptr, _size)   (void*)((char*)(_ptr) + (_size))
#endif

#define DCAL_BITMASK(x) (1 << x)

#define DREAD_BE16(x) (((*((TU8*)x))<<8) | (*((TU8*)x + 1)))
#define DREAD_BE32(x) (((*((TU8*)x))<<24) | ((*((TU8*)x + 1))<<16) | ((*((TU8*)x + 2))<<8) | (*((TU8*)x + 3)))
#define DREAD_BE64(x) (((TU64)(*((TU8*)x))<<56) | ((TU64)(*((TU8*)x + 1))<<48) | ((TU64)(*((TU8*)x + 2))<<40) | ((TU64)(*((TU8*)x + 3))<<32) | ((TU64)(*((TU8*)x + 4))<<24) | ((TU64)(*((TU8*)x + 5))<<16) | ((TU64)(*((TU8*)x + 6))<<8) | (TU64)(*((TU8*)x + 7)))
#define DMIN(a,b) ((a) > (b) ? (b) : (a))
#define DMAX(a,b) ((a) > (b) ? (a) : (b))

#ifndef BUILD_OS_WINDOWS
#define DLikely(x)   __builtin_expect(!!(x),1)
#define DUnlikely(x)   __builtin_expect(!!(x),0)
#else
#define DLikely
#define DUnlikely
#endif

//-----------------------------------------------------------------------
//
//  Error code
//
//-----------------------------------------------------------------------

typedef enum {
    EECode_OK = 0,

    EECode_NotInitilized = 0x001,
    EECode_NotRunning = 0x002,

    EECode_Error = 0x003,
    EECode_Closed = 0x004,
    EECode_Busy = 0x005,
    EECode_OSError = 0x006,
    EECode_IOError = 0x007,
    EECode_TimeOut = 0x008,
    EECode_TooMany = 0x009,
    EECode_OutOfCapability = 0x00a,
    EECode_TimeTick = 0x00b,

    EECode_NoMemory = 0x00c,
    EECode_NoPermission = 0x00d,
    EECode_NoImplement = 0x00e,
    EECode_NoInterface = 0x00f,

    EECode_NotExist = 0x010,
    EECode_NotSupported = 0x011,

    EECode_BadState = 0x012,
    EECode_BadParam = 0x013,
    EECode_BadCommand = 0x014,
    EECode_BadFormat = 0x015,
    EECode_BadMsg = 0x016,
    EECode_BadSessionNumber = 0x017,

    EECode_TryAgain = 0x018,

    EECode_DataCorruption = 0x019,
    EECode_DataMissing = 0x01a,

    EECode_InternalMemoryBug = 0x01b,
    EECode_InternalLogicalBug = 0x01c,
    EECode_InternalParamsBug = 0x01d,

    EECode_ProtocolCorruption = 0x01e,
    EECode_AbortTimeOutAPI = 0x01f,
    EECode_AbortSessionQuitAPI = 0x020,

    EECode_ParseError = 0x021,

    EECode_ExternalUnexpected = 0x028,

    EECode_UnknownError = 0x030,
    EECode_MandantoryDelete = 0x031,
    EECode_ExitIndicator = 0x32,
    EECode_AutoDelete = 0x33,

    EECode_GLError = 0x38,

    //not error, but a case, or need further API invoke
    EECode_OK_IsolateAccess = 0x040,
    EECode_OK_NeedHardwareAuthenticate = 0x041,
    EECode_OK_NeedSetOwner = 0x042,
    EECode_OK_NoOutputYet = 0x43,
    EECode_OK_NoActionNeeded = 0x44,
    EECode_OK_EOF = 0x45,
    EECode_OK_BOF = 0x46,
    EECode_OK_SKIP = 0x47,

    EECode_NetSendHeader_Fail = 0x100,
    EECode_NetSendPayload_Fail = 0x101,
    EECode_NetReceiveHeader_Fail = 0x102,
    EECode_NetReceivePayload_Fail = 0x103,

    EECode_NetConnectFail = 0x104,
    EECode_ServerReject_NoSuchChannel = 0x105,
    EECode_ServerReject_ChannelIsBusy = 0x106,
    EECode_ServerReject_BadRequestFormat = 0x107,
    EECode_ServerReject_CorruptedProtocol = 0x108,
    EECode_ServerReject_AuthenticationDataTooLong = 0x109,
    EECode_ServerReject_NotProperPassword = 0x10a,
    EECode_ServerReject_NotSupported = 0x10b,
    EECode_ServerReject_AuthenticationFail = 0x10c,
    EECode_ServerReject_TimeOut = 0x10d,
    EECode_ServerReject_Unknown = 0x10e,

    EECode_NetSocketSend_Error = 0x110,
    EECode_NetSocketRecv_Error = 0x111,

    EECode_NotLogin = 0x130,
    EECode_AlreadyExist = 0x131,
    EECode_NotOnline = 0x132,

    EECode_PossibleAttackFromNetwork = 0x140,

    EECode_NoRelatedComponent = 0x150,

    EECode_UpdateDeviceDynamiccodeFail = 0x164,
    EECode_UpdateUserDynamiccodeFail = 0x165,
} EECode;

typedef enum {
    EWindowState_Nomal = 0x00,
    EWindowState_Fullscreen = 0x01,
    EWindowState_Minmize = 0x02,
    EWindowState_Maxmize = 0x03,
} EWindowState;

typedef enum {
    EMouseShape_Standard = 0x00,
    EMouseShape_Wait = 0x01,
    EMouseShape_Circle = 0x02,
    EMouseShape_Custom = 0x03,
} EMouseShape;

typedef enum {
    EVisualSetting_invalid = 0x00,
    EVisualSetting_opengl = 0x01,
    EVisualSetting_opengles = 0x02,
    EVisualSetting_directdraw = 0x03,
} EVisualSetting;

typedef enum {
    EVisualSettingPrefer_native = 0x00,
    EVisualSettingPrefer_freeglut = 0x01,
} EVisualSettingPrefer;

typedef enum {
    ESoundSetting_invalid = 0x00,
    ESoundSetting_directsound = 0x01,
} ESoundSetting;

extern const TChar *gfGetErrorCodeString(EECode code);

typedef enum {
    EECodeHint_GenericType = 0,

    EECodeHint_BadSocket = 0x001,
    EECodeHint_SocketError = 0x002,
    EECodeHint_SendHeaderFail = 0x003,
    EECodeHint_SendPayloadFail = 0x004,
    EECodeHint_ReceiveHeaderFail = 0x005,
    EECodeHint_ReceivePayloadFail = 0x006,

    EECodeHint_BadFileDescriptor = 0x007,
    EECodeHint_BadFileIOError = 0x008,
} EECodeHintType;

typedef enum {
    EServerState_running = 0x0,
    EServerState_shuttingdown = 0x01,
    EServerState_closed = 0x02,
} EServerState;

typedef enum {
    EServerManagerState_idle = 0x0,
    EServerManagerState_noserver_alive = 0x01,
    EServerManagerState_running = 0x02,
    EServerManagerState_halt = 0x03,
    EServerManagerState_error = 0x04,
} EServerManagerState;

class CIRectInt
{
public:
    EECode QueryRectPosition(TInt &offset_x, TInt &offset_y);
    EECode QueryRectCenter(TInt &center_x, TInt &center_y);
    EECode QueryRectSize(TInt &size_x, TInt &size_y);

public:
    EECode SetRect(TInt offset_x, TInt offset_y, TInt size_x, TInt size_y);
    EECode SetRectPosition(TInt offset_x, TInt offset_y);
    EECode SetRectSize(TInt offset_x, TInt offset_y);

    EECode MoveRect(TInt offset_x, TInt offset_y);
    EECode MoveRectCenter(TInt offset_x, TInt offset_y);

private:
    TInt mOffsetX, mOffsetY;
    TInt mSizeX, mSizeY;
};

typedef struct {
    TU16 year;
    TU8 month;
    TU8 day;

    TU8 hour;
    TU8 minute;
    TU8 seconds;

    TU8 weekday : 4;
    TU8 specify_weekday : 1;
    TU8 specify_hour : 1;
    TU8 specify_minute : 1;
    TU8 specify_seconds : 1;
} SDateTime;

typedef struct {
    TS32 days;

    TU64 total_hours;
    TU32 total_minutes;
    TU32 total_seconds;

    TU64 overall_seconds;
} SDateTimeGap;

void gfDateCalculateGap(SDateTime &current_date, SDateTime &start_date, SDateTimeGap &gap);
void gfDateNextDay(SDateTime &date, TU32 days);
void gfDatePreviousDay(SDateTime &date, TU32 days);

typedef struct {
    TU16 year;
    TU8 month;
    TU8 day;

    TU8 hour;
    TU8 minute;
    TU8 second;
    TU8 frames;
} SStorageTime;

#define DDATA_PLANE_NUM 4

typedef struct {
    TU8 *p_data[DDATA_PLANE_NUM];
    TMemSize data_size[DDATA_PLANE_NUM];

    TU8 *p_buffer_base[DDATA_PLANE_NUM];
    TMemSize buffer_size[DDATA_PLANE_NUM];

    TDimension linesize[DDATA_PLANE_NUM];
    TDimension lines[DDATA_PLANE_NUM];
} SDataPlane;

#define DINVALID_VALUE_TAG 0xFEDCFEFE
#define DINVALID_VALUE_TAG_64 0xFEDCFEFEFEDCFEFELL

enum {
    //from dsp define
    EPredefinedPictureType_IDR = 1,
    EPredefinedPictureType_I = 2,
    EPredefinedPictureType_P = 3,
    EPredefinedPictureType_B = 4,
};

//refer to 264 spec table 7.1
enum {
    ENalType_unspecified = 0,
    ENalType_IDR = 5,
    ENalType_SEI = 6,
    ENalType_SPS = 7,
    ENalType_PPS = 8,
};

//refer to 265 spec table 7.1
enum {
    EHEVCNalType_TRAIL_N = 0,
    EHEVCNalType_TRAIL_R = 1,
    EHEVCNalType_TSA_N = 2,
    EHEVCNalType_TSA_R = 3,
    EHEVCNalType_STSA_N = 4,
    EHEVCNalType_STSA_R = 5,
    EHEVCNalType_RADL_N = 6,
    EHEVCNalType_RADL_R = 7,
    EHEVCNalType_RASL_N = 8,
    EHEVCNalType_RASL_R = 9,
    EHEVCNalType_RSV_VCL_N10 = 10,
    EHEVCNalType_RSV_VCL_R11 = 11,
    EHEVCNalType_RSV_VCL_N12 = 12,
    EHEVCNalType_RSV_VCL_R13 = 13,
    EHEVCNalType_RSV_VCL_N14 = 14,
    EHEVCNalType_RSV_VCL_R15 = 15,
    EHEVCNalType_BLA_W_LP = 16,
    EHEVCNalType_BLA_W_RADL = 17,
    EHEVCNalType_BLA_N_LP = 18,
    EHEVCNalType_IDR_W_RADL = 19,
    EHEVCNalType_IDR_N_LP = 20,
    EHEVCNalType_CRA_NUT = 21,
    EHEVCNalType_RSV_IRAP_VCL22 = 22,
    EHEVCNalType_RSV_IRAP_VCL23 = 23,
    EHEVCNalType_RSV_VCL24 = 24,
    EHEVCNalType_RSV_VCL25 = 25,
    EHEVCNalType_RSV_VCL26 = 26,
    EHEVCNalType_RSV_VCL27 = 27,
    EHEVCNalType_RSV_VCL28 = 28,
    EHEVCNalType_RSV_VCL29 = 29,
    EHEVCNalType_RSV_VCL30 = 30,
    EHEVCNalType_RSV_VCL31 = 31,
    EHEVCNalType_VPS = 32,
    EHEVCNalType_SPS = 33,
    EHEVCNalType_PPS = 34,
    EHEVCNalType_AUD = 35,
    EHEVCNalType_EOS = 36,
    EHEVCNalType_EOB = 37,
    EHEVCNalType_FD = 38,
    EHEVCNalType_PREFIX_SEI = 39,
    EHEVCNalType_SUFFIX_SEI = 40,
    EHEVCNalType_RSV_NVCL41 = 41,
    EHEVCNalType_RSV_NVCL42 = 42,
    EHEVCNalType_RSV_NVCL43 = 43,
    EHEVCNalType_RSV_NVCL44 = 44,
    EHEVCNalType_RSV_NVCL45 = 45,
    EHEVCNalType_RSV_NVCL46 = 46,
    EHEVCNalType_RSV_NVCL47 = 47,

    EHEVCNalType_VCL_END = 31,
};

enum {
    ERTCPType_SR = 200,
    ERTCPType_RR = 201,
    ERTCPType_SDC = 202,
    ERTCPType_BYE = 203,
};

typedef enum {
    ERunTimeConfigType_XML = 0x0,
    ERunTimeConfigType_SimpleINI = 0x1,
} ERunTimeConfigType;

typedef enum {
    EConfigType_XML = 0x0,
} EConfigType;

typedef enum {
    EIPCAgentType_Invalid = 0x0,
    EIPCAgentType_UnixDomainSocket = 0x1,
    EIPCAgentType_Socket = 0x2,
} EIPCAgentType;

typedef struct _Link {
    struct _Link *next, *prev;
} SLink;

typedef struct {
    void *first, *last;
} SListBase;

#define BE_16(x) (((TU8 *)(x))[0] <<  8 | ((TU8 *)(x))[1])

#define DBEW64(x, p) do { \
        p[0] = (x >> 56) & 0xff; \
        p[1] = (x >> 48) & 0xff; \
        p[2] = (x >> 40) & 0xff; \
        p[3] = (x >> 32) & 0xff; \
        p[4] = (x >> 24) & 0xff; \
        p[5] = (x >> 16) & 0xff; \
        p[6] = (x >> 8) & 0xff; \
        p[7] = x & 0xff; \
    } while(0)

#define DBEW48(x, p) do { \
        p[0] = (x >> 40) & 0xff; \
        p[1] = (x >> 32) & 0xff; \
        p[2] = (x >> 24) & 0xff; \
        p[3] = (x >> 16) & 0xff; \
        p[4] = (x >> 8) & 0xff; \
        p[5] = x & 0xff; \
    } while(0)

#define DBEW32(x, p) do { \
        p[0] = (x >> 24) & 0xff; \
        p[1] = (x >> 16) & 0xff; \
        p[2] = (x >> 8) & 0xff; \
        p[3] = x & 0xff; \
    } while(0)

#define DBEW24(x, p) do { \
        p[0] = (x >> 16) & 0xff; \
        p[1] = (x >> 8) & 0xff; \
        p[2] = (x) & 0xff; \
    } while(0)

#define DBEW16(x, p) do { \
        p[0] = (x >> 8) & 0xff; \
        p[1] = x & 0xff; \
    } while(0)

#define DBER64(x, p) do { \
        x = ((TU64)p[0] << 56) | ((TU64)p[1] << 48) | ((TU64)p[2] << 40) | ((TU64)p[3] << 32) | ((TU64)p[4] << 24) | ((TU64)p[5] << 16) | ((TU64)p[6] << 8) | (TU64)p[7]; \
    } while(0)

#define DBER48(x, p) do { \
        x = ((TU64)p[0] << 40) | ((TU64)p[1] << 32) | ((TU64)p[2] << 24) | ((TU64)p[3] << 16) | ((TU64)p[4] << 8) | (TU64)p[5] ; \
    } while(0)

#define DBER32(x, p) do { \
        x = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; \
    } while(0)

#define DBER24(x, p) do { \
        x = (p[0] << 16) | (p[1] << 8) | p[2]; \
    } while(0)

#define DBER16(x, p) do { \
        x = (p[0] << 8) | p[1]; \
    } while(0)

#define DLEW64(x, p) do { \
        p[0] = x & 0xff; \
        p[1] = (x >> 8) & 0xff; \
        p[2] = (x >> 16) & 0xff; \
        p[3] = (x >> 24) & 0xff; \
        p[4] = (x >> 32) & 0xff; \
        p[5] = (x >> 40) & 0xff; \
        p[6] = (x >> 48) & 0xff; \
        p[7] = (x >> 56) & 0xff; \
    } while(0)

#define DLEW32(x, p) do { \
        p[0] = x & 0xff; \
        p[1] = (x >> 8) & 0xff; \
        p[2] = (x >> 16) & 0xff; \
        p[3] = (x >> 24) & 0xff; \
    } while(0)

#define DLEW16(x, p) do { \
        p[0] = x & 0xff; \
        p[1] = (x >> 8) & 0xff; \
    } while(0)

#define DLER64(x, p) do { \
        x = ((TU64)p[7] << 56) | ((TU64)p[6] << 48) | ((TU64)p[5] << 40) | ((TU64)p[4] << 32) | ((TU64)p[3] << 24) | ((TU64)p[2] << 16) | ((TU64)p[1] << 8) | (TU64)p[0]; \
    } while(0)

#define DLER32(x, p) do { \
        x = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0]; \
    } while(0)

#define DLER16(x, p) do { \
        x = (p[1] << 8) | p[0]; \
    } while(0)

//some configurations
#define DMAX_ACCOUNT_NAME_LENGTH 24
#define DMAX_ACCOUNT_NAME_LENGTH_EX 32
#define DMAX_ACCOUNT_INFO_LENGTH 48
#define DMAX_PRODUCTION_CODE_LENGTH 128
#define DMAX_ID_AUTHENTICATION_STRING_LENGTH 32
#define DMAX_SECURE_QUESTION_LENGTH 64
#define DMAX_SECURE_ANSWER_LENGTH 32

#define DMaxConfStringLength 512

#define DMaxServerUrlLength 64

#define DMaxChannelNameLength 128
#define DMaxFileNameLength 512
#define DSingleFileDuration 3//minutes, TODO: should be specified by user?
#define DInitialMaxFriendsNumber 64
#define DInitialMaxDeviceNumber 64
#define DOfflineMessageStepBufferSize 1024

#define DDefaultMaxConnectionNumber 200
#define DMaxDeviceCountOneGroup 50

#define DMaxCommunicationPortSourceNumber 20

#define DSYSTEM_MAX_CHANNEL_NUM 512

#define DMAX_FILE_NAME_LENGTH 512

// common configure struct
// sound cache setting
typedef struct {
    TU8 sound_output_buffer_count;
    TU8 sound_output_precache_count;

    TU8 reserved0;
    TU8 reserved1;
} SPersistCommonConfig_SoundOutput;

typedef struct {
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
    TU8 reserved3;
} SPersistCommonConfig_ViusalOutput;

typedef struct {
    TU8 host_tag_higher_bit;
    TU8 host_tag_lower_bit;

    TU8 reserved0;
    TU8 reserved1;

    TU8 level1_higher_bit;
    TU8 level1_lower_bit;

    TU8 level2_higher_bit;
    TU8 level2_lower_bit;
} SPersistCommonConfig_DatabBase;

typedef struct {
    TU8 dump_debug_sound_input_pcm;
    TU8 dump_debug_sound_output_pcm;

    TU8 debug_print_video_realtime;
    TU8 debug_print_audio_realtime;
} SPersistCommonConfig_DebugDump;

typedef struct {
    SPersistCommonConfig_SoundOutput sound_output;
    SPersistCommonConfig_ViusalOutput visual_output;
    SPersistCommonConfig_DatabBase database_config;

    SPersistCommonConfig_DebugDump debug_dump;
} SPersistCommonConfig;

#define DPresetSocketTimeOutUintSeconds 0
#define DPresetSocketTimeOutUintUSeconds 300000

#define DDefaultClientPort 0
#define DDefaultSACPAdminPort 8260
#define DDefaultSACPServerPort 8270
#define DDefaultSACPIMServerPort 8280

#define DDefaultRTSPServerPort 8290

#define DDefaultIMServerGuardianPort 8250
#define DDefaultDataServerGuardianPort 8252
#define DDefaultDeviceAgentGuardianPort 8254

#define DDefaultDirectSharingServerPort 8210

typedef enum {
    ECMDType_Invalid = 0x0000,

    ECMDType_FirstEnum = 0x0001,

    //common used
    ECMDType_Terminate = ECMDType_FirstEnum,
    ECMDType_StartRunning = 0x0002, //for AO, enter/exit OnRun
    ECMDType_ExitRunning = 0x0003,

    ECMDType_RunProcessing = 0x0004, //for processing, module specific work related, can not re-invoked start processing after exit processing
    ECMDType_ExitProcessing = 0x0005,

    ECMDType_Stop = 0x0010, //module start/stop, can be re-invoked
    ECMDType_Start = 0x0011,

    ECMDType_Pause = 0x0012,
    ECMDType_Resume = 0x0013,
    ECMDType_ResumeFlush = 0x0014,
    ECMDType_Flush = 0x0015,
    ECMDType_FlowControl = 0x0016,
    ECMDType_Suspend = 0x0017,
    ECMDType_ResumeSuspend = 0x0018,

    ECMDType_SwitchInput = 0x0019,

    ECMDType_SendData = 0x001a,
    ECMDType_Seek = 0x001b,

    ECMDType_Step = 0x0020,
    ECMDType_DebugDump = 0x0021,
    ECMDType_PrintCurrentStatus = 0x0022,
    ECMDType_SetRuntimeLogConfig = 0x0023,
    ECMDType_PlayUrl = 0x0024,
    ECMDType_PlayNewUrl = 0x0025,

    ECMDType_PurgeChannel = 0x0028,
    ECMDType_ResumeChannel = 0x0029,
    ECMDType_NavigationSeek = 0x002a,

    ECMDType_AddContent = 0x0030,
    ECMDType_RemoveContent = 0x0031,
    ECMDType_AddClient = 0x0032,
    ECMDType_RemoveClient = 0x0033,
    ECMDType_UpdateUrl = 0x0034,
    ECMDType_RemoveClientSession = 0x0035,
    ECMDType_UpdateRecSavingStrategy = 0x0036,

    //specific to each case
    ECMDType_ForceLowDelay = 0x0040,
    ECMDType_Speedup = 0x0041,
    ECMDType_DeleteFile = 0x0042,
    ECMDType_UpdatePlaybackSpeed = 0x0043,
    ECMDType_UpdatePlaybackLoopMode = 0x0044,
    ECMDType_DecoderZoom = 0x0045,

    ECMDType_EnableVideo = 0x0046,
    ECMDType_EnableAudio = 0x0047,

    ECMDType_NotifySynced = 0x0050,
    ECMDType_NotifySourceFilterBlocked = 0x0051,
    ECMDType_NotifyUDECInRuningState = 0x0052,
    ECMDType_NotifyBufferRelease = 0x0053,

    ECMDType_MuteAudio = 0x0060,
    ECMDType_UnMuteAudio = 0x0061,
    ECMDType_ConfigAudio = 0x0062,

    ECMDType_MiscCheckLicence = 0x0070,

    //app framework
    ECMDType_CreateMainWindow = 0x0078,
    ECMDType_DestoryMainWindow = 0x0079,
    ECMDType_MoveMainWindow = 0x007a,
    ECMDType_ReSizeMainWindow = 0x007b,
    ECMDType_ReSizeClientWindow = 0x007c,

    //cloud based
    ECMDType_CloudSourceClient_UnknownCmd = 0x0100,

    ECMDType_CloudSourceClient_UpdateAudioParams = 0x0104,
    ECMDType_CloudSourceClient_UpdateVideoParams = 0x0105,

    ECMDType_CloudSourceClient_DiscIndicator = 0x0106,
    ECMDType_CloudSourceClient_VideoSync = 0x0107,
    ECMDType_CloudSourceClient_AudioSync = 0x0108,

    //ECMDType_CloudSinkClient_ReAuthentication = 0x0110,
    ECMDType_CloudSinkClient_UpdateBitrate = 0x0111,
    ECMDType_CloudSinkClient_UpdateFrameRate = 0x0112,
    ECMDType_CloudSinkClient_UpdateDisplayLayout = 0x0113,
    ECMDType_CloudSinkClient_SelectAudioSource = 0x0114,
    ECMDType_CloudSinkClient_SelectAudioTarget = 0x0115,
    ECMDType_CloudSinkClient_Zoom = 0x0116,
    ECMDType_CloudSinkClient_UpdateDisplayWindow = 0x0117,
    ECMDType_CloudSinkClient_SelectVideoSource = 0x0118,
    ECMDType_CloudSinkClient_ShowOtherWindows = 0x0119,
    ECMDType_CloudSinkClient_DemandIDR = 0x011a,
    ECMDType_CloudSinkClient_SwapWindowContent = 0x011b,
    ECMDType_CloudSinkClient_CircularShiftWindowContent = 0x011c,
    ECMDType_CloudSinkClient_SwitchGroup = 0x011d,
    ECMDType_CloudSinkClient_ZoomV2 = 0x011e,
    ECMDType_CloudSinkClient_UpdateResolution = 0x011f,

    ECMDType_CloudClient_PeerClose = 0x0140,

    ECMDType_CloudClient_QueryVersion = 0x0141,
    ECMDType_CloudClient_QueryStatus = 0x0142,
    ECMDType_CloudClient_SyncStatus = 0x0143,
    ECMDType_CloudClient_QuerySourceList  = 0x0144,
    ECMDType_CloudClient_AquireChannelControl  = 0x0145,
    ECMDType_CloudClient_ReleaseChannelControl  = 0x0146,

    //for extention
    ECMDType_CloudClient_CustomizedCommand = 0x0180,

    //debug
    ECMDType_DebugClient_PrintCloudPipeline = 0x200,
    ECMDType_DebugClient_PrintStreamingPipeline = 0x201,
    ECMDType_DebugClient_PrintRecordingPipeline = 0x202,
    ECMDType_DebugClient_PrintSingleChannel = 0x203,
    ECMDType_DebugClient_PrintAllChannels = 0x204,

    ECMDType_LastEnum = 0x0400,

} ECMDType;

#define DMAX_SCREEN_NUMBER 4
#define DMAX_SOUND_INPUT_DEVICE_NUMBER 4

typedef enum {
    EPixelFMT_invalid = 0x00,
    EPixelFMT_rgba32 = 0x01,
    EPixelFMT_yuv420p = 0x02,
} EPixelFMT;

typedef struct {
    TDimension screen_width;
    TDimension screen_height;
    TU16 refresh_rate;
    TU16 pixel_format; // EPixelFMT
} SSystemScreenSettings;

typedef struct {
    SSystemScreenSettings screens[DMAX_SCREEN_NUMBER];
    TU32 screen_number;
} SSystemSettings;

typedef struct {
    const TChar *p_desc;
    const TChar *p_devname;
    void *p_context;
} SSystemSoundInputDevice;

typedef struct {
    SSystemSoundInputDevice devices[DMAX_SOUND_INPUT_DEVICE_NUMBER];
    TU32 sound_input_devices_number;
} SSystemSoundInputDevices;

typedef enum {

    //im
    ESystemNotify_IM_FriendInvitation = 0x30,
    ESystemNotify_IM_DeviceDonation = 0x31,
    ESystemNotify_IM_DeviceSharing = 0x32,
    ESystemNotify_IM_UpdateOwnedDeviceList = 0x33,
    ESystemNotify_IM_DeviceOwnedByUser = 0x34,

    ESystemNotify_IM_UserOnline = 0x35,
    ESystemNotify_IM_UserOffline = 0x36,

    ESystemNotify_IM_TargetNotReachable = 0x37,

} ESystemNotify;

typedef struct {
    TChar invitor_name[DMAX_ACCOUNT_NAME_LENGTH_EX];
    TUniqueID invitor_id;
} SSystemNotifyIMFriendInvitation;

typedef struct {
    TUniqueID target;
} SSystemNotifyIMTargetNotReachable;

typedef struct {
    TUniqueID target;
} SSystemNotifyIMTargetOffline;

//TLV
typedef enum {
    ETLV8Type_reserved = 0x0,
} ETLV8Type;

typedef enum {
    ETLV16Type_Invalid = 0x0,

    //common type
    ETLV16Type_String = 0x0100,
    ETLV16Type_IDList = 0x0101,
    ETLV16Type_DynamicInput = 0x0102,
    ETLV16Type_AuthenticationResult32 = 0x0103,
    ETLV16Type_DynamicCode = 0x0104,
    ETLV16Type_ClientDynamicInput = 0x0105,
    ETLV16Type_SecurityQuestion = 0x0106,
    ETLV16Type_SecurityAnswer = 0x0107,

    ETLV16Type_TimeUnit = 0x0120,

    //only flags
    //ETLV16Type_DiscIndicator = 0x0180,

    //error description
    ETLV16Type_ErrorDescription = 0x0200,

    //account management
    //base info
    ETLV16Type_AccountID = 0x0300,
    ETLV16Type_AccountName = 0x0301,
    ETLV16Type_AccountPassword = 0x0302,
    ETLV16Type_AccountNickName = 0x0303,

    //source device
    ETLV16Type_SourceDeviceUploadingPort = 0x0304,
    ETLV16Type_SourceDeviceStreamingPort = 0x0305,
    ETLV16Type_SourceDeviceShareList = 0x0306,
    ETLV16Type_SourceDeviceSharePrivilegeMaskList = 0x0307,
    ETLV16Type_SourceDeviceSharePrivilegeComboList = 0x0308,
    ETLV16Type_SourceDeviceOwner = 0x309,
    ETLV16Type_SourceDeviceProductCode = 0x30a,
    ETLV16Type_SourceDeviceDataServerAddress = 0x030b,
    ETLV16Type_SourceDeviceStreamingTag = 0x030c,

    //user account
    ETLV16Type_UserFriendList = 0x030e,
    ETLV16Type_UserOwnedDeviceList = 0x030f,
    ETLV16Type_UserSharedDeviceList = 0x0310,
    ETLV16Type_UserOnline = 0x0311,

    ETLV16Type_UserPrivilegeMask = 0x0315,
    ETLV16Type_DeviceIDList = 0x0316,

    //data node account
    ETLV16Type_DataNodeChannelIDList = 0x0320,
    ETLV16Type_DataNodeAdminPort = 0x0321,
    ETLV16Type_DataNodeCloudPort = 0x0322,
    ETLV16Type_DataNodeRTSPPort = 0x0323,
    ETLV16Type_DataNodeMaxChannelNumber = 0x0324,
    ETLV16Type_DataNodeCurrentChannelNumber = 0x0325,
    ETLV16Type_DataNodeUrl = 0x0326,

    // to do

    //device params
    ETLV16Type_DeviceAudioParams_Format = 0x0400,
    ETLV16Type_DeviceAudioParams_ChannelNum = 0x0401,
    ETLV16Type_DeviceAudioParams_SampleFrequency = 0x0402,
    ETLV16Type_DeviceAudioParams_Bitrate = 0x0403,
    ETLV16Type_DeviceAudioParams_ExtraData = 0x0404,

    ETLV16Type_DeviceAudioSyncSeqNumber = 0x0405,
    ETLV16Type_DeviceAudioSyncTime = 0x0406,

    ETLV16Type_DeviceVideoParams_Format = 0x0410,
    ETLV16Type_DeviceVideoParams_FrameRate = 0x0411,
    ETLV16Type_DeviceVideoParams_Resolution = 0x0412,
    ETLV16Type_DeviceVideoParams_Bitrate = 0x0413,
    ETLV16Type_DeviceVideoParams_ExtraData = 0x0414,

    ETLV16Type_DeviceVideoSyncSeqNumber = 0x0415,
    ETLV16Type_DeviceVideoSyncTime = 0x0416,
    ETLV16Type_DeviceVideoGOPStructure = 0x0417,

    //device setup
    ETLV16Type_IMServerUrl = 0x0500,
    ETLV16Type_IMServerPort = 0x0501,
    ETLV16Type_DeviceID = 0x0502,
    ETLV16Type_DeviceName = 0x0503,
    ETLV16Type_DevicePassword = 0x0504,
    //ETLV16Type_DeviceIDString = 0x0505,//obsolete
    ETLV16Type_DeviceProductionCode = 0x0506,//obsolete

    ETLV16Type_DeviceNickName = 0x0507,
    ETLV16Type_DeviceStatus = 0x0508,
    ETLV16Type_DeviceStorageCapacity = 0x0509,

} ETLV16Type;

typedef struct {
    TU8 type;
    TU8 length;
} STLV8Header;

typedef struct {
    TU16 type;
    TU16 length;
} STLV16Header;

typedef struct {
    TU8 type_high;
    TU8 type_low;
    TU8 length_high;
    TU8 length_low;
} STLV16HeaderBigEndian;

#define DGetTLV16Type(type, header) do { \
        STLV16HeaderBigEndian* h = (STLV16HeaderBigEndian*) (header); \
        type = (((TU32)h->type_high) << 8) | ((TU32)h->type_low); \
    } while (0)

#define DGetTLV16Length(length, header) do { \
        STLV16HeaderBigEndian* h = (STLV16HeaderBigEndian*) (header); \
        length = (((TU32)h->length_high) << 8) | ((TU32)h->length_low); \
    } while (0)

//data in database
typedef struct {
    TUniqueID id;
    TU32 total_memory_length;
} SMemVariableHeader;

typedef enum {
    ETimeUnit_us = 0x00, // 1/1000000 seconds
    ETimeUnit_native = 0x01, // 1/90000 seconds
    ETimeUnit_specific = 0x02, // media spesific
} ETimeUnit;

#ifndef BUILD_OS_WINDOWS
//guardian
typedef EECode(*TGuardianCallBack)(void *owner, TU32 cur_state);

typedef enum {
    EGuardianType_socket = 0x0,
} EGuardianType;

typedef enum {
    EGuardianState_idle = 0x0,
    EGuardianState_wait_paring = 0x02,
    EGuardianState_guard = 0x03,
    EGuardianState_halt = 0x04,
    EGuardianState_error = 0x05,
} EGuardianState;

class IGuardian
{
public:
    virtual EECode Start(TU16 port, TGuardianCallBack callback, void *context) = 0;
    virtual EECode Stop() = 0;

public:
    virtual void Destroy() = 0;
};

IGuardian *gfCreateGuardian(EGuardianType type = EGuardianType_socket);
TSocketHandler gfCreateGuardianAgent(EGuardianType type, TSocketPort port);
#endif

#ifdef WORDS_BIGENDIAN
#define DFOURCC( a, b, c, d ) \
    ( ((TU32)d) | ( ((TU32)c) << 8 ) \
      | ( ((TU32)b) << 16 ) | ( ((TU32)a) << 24 ) )
#else
#define DFOURCC( a, b, c, d ) \
    ( ((TU32)a) | ( ((TU32)b) << 8 ) \
      | ( ((TU32)c) << 16 ) | ( ((TU32)d) << 24 ) )
#endif

typedef enum {
    VCOM_EFileIOStatus_uninited = 0,
    VCOM_EFileIOStatus_fileopened,
    VCOM_EFileIOStatus_writingobject,
    VCOM_EFileIOStatus_writingsection,
} VCOM_TFileIOStatus;

typedef enum {
    VCOM_EFileIOError_nothing = 0,
    VCOM_EFileIOError_file_openfail = 1,
    VCOM_EFileIOError_file_closefail = 2,
    VCOM_EFileIOError_null_pointer = 4,
    VCOM_EFileIOError_invalid_input = 8,
    VCOM_EFileIOError_memalloc_fail = 16,
    VCOM_EFileIOError_internal_error = 32,
    VCOM_EFileIOError_warings = 64,
} VCOM_TFileIOErrors;

//8bit section type
typedef enum {
    VCOM_EFileIOSectionType_data = 0,
    VCOM_EFileIOSectionType_sectionentry = 1,
} VCOM_TFileIOSectionType;

//fileIO section header length, determined by struct VCOM_SFileIOSection blow
//#define VCOM_DFileIOSectionHeaderLength 28
typedef struct VCOM_SFileIOSection_s {
    TU32 section_name, section_index;
    TU16 size, type, flag, recusive_depth; //flag used for indicate the section is or not a sub section
    TU64 length;
    TU32 used_num_of_subsections;
    void *p_data;

    TU32 total_num_of_subsections;
    struct VCOM_SFileIOSection_s *p_subsection;//subsection
    //    TU32 noneed_recal;//flags
} VCOM_SFileIOSection;

typedef struct {
    TU32 object_type;//globle
    TU32 index;
    TU64 tail_offset;
} VCOM_SFileIOObjectHead;

typedef struct {
    TU64 total_section_number;
    TU64 *p_section_offset;
} VCOM_SFileIOObjectTail;

typedef struct {
    VCOM_SFileIOObjectHead head;
    VCOM_SFileIOObjectTail tail;

    VCOM_SFileIOSection *p_sections;
    TU64 total_length;
    TU64 total_number_of_sections;
    TU64 used_number;
    //    TU32 noneed_recal;//flags
} VCOM_SFileIOObject;

typedef struct {
    TU32 reserved, type, type_ext, info;
    TU64 tail_offset, tail_length; //tail length no used now
} VCOM_SFileIOFileHead;

typedef struct {
    TU64 object_number;
    TU64 *p_object_offset;
} VCOM_SFileIOFileTail;

typedef struct {
    VCOM_SFileIOFileHead head;
    VCOM_SFileIOFileTail tail;

    TU64 total_length;
    VCOM_SFileIOObject *p_objects;
    TU64 total_number_of_object;
    TU64 used_number;
    //    TU32 noneed_recal;//flags
} VCOM_SFileIOFile;

extern void gfMovelisttolist(SListBase *dst, SListBase *src);
extern void gfLinkAddHead(SListBase *listbase, void *vlink);
extern void gfLinkAddTail(SListBase *listbase, void *vlink);
extern void gfRemLink(SListBase *listbase, void *vlink);
extern TInt gfLinkFindIndex(const SListBase *listbase, const void *vlink);
extern bool gfRemLinkSafe(SListBase *listbase, void *vlink);
extern TInt gfLinkCount(const SListBase *listbase);
extern void *gfLinkPopHead(SListBase *listbase);
extern void *gfLinkPopTail(SListBase *listbase);

#define DMaxKeyNum 400

#define DEmbededDataProcessingReSyncFlag 0x01
typedef EECode(*TEmbededDataProcessingCallBack)(void *owner, TU8 *pdata, TU32 datasize, TU32 dataflag);

#define D_MAX_LANGUAGE_TYPE_LENGTH 32
#define D_MAX_LANGUAGE_TYPE_NUMBER 16
#define D_MAX_LANGUAGE_BUILDIN_STRING_NUMBER 256
//extern TChar gcMultiLanguageType[D_MAX_LANGUAGE_TYPE_NUMBER][D_MAX_LANGUAGE_TYPE_LENGTH];
//extern TU32 gnCurrentLanguageIndex;
extern EECode gfMultiLanguageSet(TChar *lan);
extern const TChar *gfMultiLanguageGet();
extern const TChar *gfMultiLanguageBuildinString(TU32 buildin_index, TU32 index);
#define DML(x) gfMultiLanguageBuildinString(x)
extern EECode gfInitializeMultiLanguage(const TChar conf_file, const TChar en_file);
extern void gfDeInitializeMultiLanguage();

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

