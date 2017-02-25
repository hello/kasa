/*******************************************************************************
 * directsharing_if.h
 *
 * History:
 *  2015/03/06 - [Zhi He] create file
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

#ifndef __DIRECTSHARING_IF_H__
#define __DIRECTSHARING_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

typedef enum {
    EDirectSharingHeaderType_Connect = 0x01,
    EDirectSharingHeaderType_Authenticate = 0x02,
    EDirectSharingHeaderType_QueryResource = 0x03,
    EDirectSharingHeaderType_RequestResource = 0x04,
    EDirectSharingHeaderType_ShareResource = 0x05,

    EDirectSharingHeaderType_GoodBye = 0x08,

    EDirectSharingHeaderType_Data = 0x10,
    EDirectSharingHeaderType_EOS = 0x11,
} EDirectSharingHeaderType;

typedef enum {
    EDirectSharingStatusCode_OK = 0x00,
    EDirectSharingStatusCode_NeedAuthenticate = 0x02,

    EDirectSharingStatusCode_AuthenticateFail = 0x10,
    EDirectSharingStatusCode_NoAvailableResource = 0x11,
    EDirectSharingStatusCode_NoPermission = 0x12,
    EDirectSharingStatusCode_NoSuchResource = 0x13,
    EDirectSharingStatusCode_NotSuppopted = 0x14,
    EDirectSharingStatusCode_BadState = 0x15,
    EDirectSharingStatusCode_InvalidArgument = 0x16,

    EDirectSharingStatusCode_CorruptedProtocol = 0x18,

    EDirectSharingStatusCode_SendHeaderFail = 0x20,
    EDirectSharingStatusCode_SendPayloadFail = 0x21,
    EDirectSharingStatusCode_SendReplyFail = 0x22,
} EDirectSharingStatusCode;

#define DDirectSharingHeaderFlagIsReply (0x80)
#define DDirectSharingHeaderFlagStatusCodeMask (0x1f)

#define DDirectSharingPayloadFlagEOS (0x80)
//#define DDirectSharingPayloadFlagFrameStartIndicator (0x40)
//#define DDirectSharingPayloadFlagFrameEndIndicator (0x20)
#define DDirectSharingPayloadFlagKeyFrameIndicator (0x10)
#define DDirectSharingPayloadFlagStreamIndexMask (0x07)

typedef struct {
    TU8 header_type;
    TU8 header_flag;
    TU8 val0;
    TU8 val1;

    TU8 payload_flag;
    TU8 payload_length_0;
    TU8 payload_length_1;
    TU8 payload_length_2;
} SDirectSharingHeader;

typedef enum {
    ESharedResourceType_File = 0x0,
    ESharedResourceType_LiveStreamVideo = 0x1,
    ESharedResourceType_LiveStreamAudio = 0x2,
} ESharedResourceType;

typedef enum {
    ESharedResourceFormat_H264 = 0x0,
    ESharedResourceFormat_H265 = 0x1,
} ESharedResourceVideoFormat;

#define DMAX_DIRECT_SHARING_STRING_SIZE 511
#define DDirectSharingFileDescripsionFormat "[file]:index=%d,size=%lld,description=%s;"
#define DDirectSharingLiveStreamVideoDescripsionFormat "[live video]:index=%d,format=%d,width=%d,height=%d,fr_num=%d,fr_den=%d,bitrate=%d,m=%d,n=%d;"
#define DDirectSharingLiveStreamAudioDescripsionFormat "[live audio]:index=%d,format=%d,channels=%d,samplerate=%d,framesize=%d,bitrate=%d;"

typedef struct {
    TU8 format;
    TU8 m;
    TU16 n;

    TU32 width;
    TU32 height;
    TU32 framerate_num;
    TU32 framerate_den;

    TU32 bitrate;
} SVideoLiveStreamProperty;

typedef struct {
    TU8 format;
    TU8 channel_number;
    TU8 sample_format;
    TU8 channel_interleave;

    TU32 samplerate;
    TU32 framesize;

    TU32 bitrate;
} SAudioLiveStreamProperty;

typedef struct {
    TU64 filesize;
    TChar *filename;
} SFileProperty;

typedef struct {
    TU8 type;
    TU8 reserved0;
    TU16 index;

    union {
        SVideoLiveStreamProperty video;
        SAudioLiveStreamProperty audio;
        SFileProperty file;
    } property;

    TChar description[64];
} SSharedResource;

typedef void (*TRequestMemoryCallBack)(void *owner, TU8 *&p_buf, TInt buf_length);
typedef void (*TDataReceiveCallBack)(void *owner, TU8 *p_buf, TInt buf_length, EECode err, TU8 flag);

//-----------------------------------------------------------------------
//
//  IDirectSharingSender
//
//-----------------------------------------------------------------------
typedef void (*TFileTransferFinishCallBack)(void *owner, void *p_sender, EECode ret_code);
typedef void (*TTransferUpdateProgressCallBack)(void *owner, TInt progress);

class IDirectSharingSender
{
public:
    virtual EDirectSharingStatusCode SendData(TU8 *pdata, TU32 datasize, TU8 data_flag) = 0;

    virtual EDirectSharingStatusCode StartSendFile(TChar *filename, TFileTransferFinishCallBack callback, void *callback_context) = 0;

public:
    virtual void SetProgressCallBack(void *owner, TTransferUpdateProgressCallBack progress_callback) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IDirectSharingSender() {}
};

//-----------------------------------------------------------------------
//
//  IDirectSharingReceiver
//
//-----------------------------------------------------------------------
class IDirectSharingReceiver
{
public:
    virtual void SetCallBack(void *owner, TRequestMemoryCallBack request_memory_callback, TDataReceiveCallBack receive_callback) = 0;

public:
    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IDirectSharingReceiver() {}

};

//-----------------------------------------------------------------------
//
//  IDirectSharingDispatcher
//
//-----------------------------------------------------------------------
typedef void (*TDataSendCallBack)(void *owner, TU8 *p_buf, TInt &buf_length, EECode err, TU8 flag);

class IDirectSharingDispatcher
{
public:
    virtual EECode SetResource(SSharedResource *resource) = 0;
    virtual EECode QueryResource(SSharedResource *&resource) const = 0;

public:
    virtual EECode AddSender(IDirectSharingSender *sender) = 0;
    virtual EECode RemoveSender(IDirectSharingSender *sender) = 0;

public:
    virtual EECode SendData(TU8 *pdata, TU32 datasize, TU8 data_flag) = 0;

public:
    virtual void SetProgressCallBack(void *owner, TTransferUpdateProgressCallBack progress_callback) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IDirectSharingDispatcher() {}
};

//-----------------------------------------------------------------------
//
//  IDirectSharingClient
//
//-----------------------------------------------------------------------
class IDirectSharingClient
{
public:
    virtual EECode ConnectServer(TChar *server_url, TSocketPort server_port, TChar *username, TChar *password) = 0;
    virtual EDirectSharingStatusCode QuerySharedResource(SSharedResource *&p_resource_list, TU32 &resource_number) = 0;

    //request resource
    virtual EDirectSharingStatusCode RequestResource(TU8 type, TU16 index) = 0;

    //share source
    virtual EDirectSharingStatusCode ShareResource(SSharedResource *resource) = 0;

public:
    virtual TSocketHandler RetrieveSocketHandler() = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IDirectSharingClient() {}
};

//-----------------------------------------------------------------------
//
//  IDirectSharingServer
//
//-----------------------------------------------------------------------
class IDirectSharingServer
{
public:
    virtual EECode StartServer() = 0;
    virtual EECode StopServer() = 0;

    virtual EECode AddDispatcher(IDirectSharingDispatcher *dispatcher) = 0;
    virtual EECode RemoveDispatcher(IDirectSharingDispatcher *dispatcher) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IDirectSharingServer() {}
};

IDirectSharingSender *gfCreateDSSender(TSocketHandler socket, TU8 resource_type);
IDirectSharingReceiver *gfCreateDSReceiver(TSocketHandler socket);
IDirectSharingDispatcher *gfCreateDSDispatcher(ESharedResourceType type);
IDirectSharingClient *gfCreateDSClient();
IDirectSharingServer *gfCreateDSServer(TSocketPort port);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

