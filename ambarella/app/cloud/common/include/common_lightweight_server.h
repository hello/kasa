/**
 * common_lightweight_server.h
 *
 * History:
 *  2014/12/30 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __COMMON_LIGHTWEIGHT_SERVER_H__
#define __COMMON_LIGHTWEIGHT_SERVER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

enum {
    ELWServerState_idle,
    ELWServerState_listening,
    ELWServerState_running,
    ELWServerState_error,
};

enum {
    ELWDataType_AVC = 0x00,
    ELWDataType_HEVC = 0x01,

    ELWDataType_AAC = 0x10,
    ELWDataType_G726 = 0x11,
    ELWDataType_G711mu = 0x12,
    ELWDataType_G711a = 0x13,
    ELWDataType_RawPCM = 0x14,
};

enum {
    ELWFrameType_Invalid = 0x00,
    ELWFrameType_IDR = 0x01,
    ELWFrameType_I = 0x02,
    ELWFrameType_P = 0x03,
    ELWFrameType_B = 0x04,
};

typedef struct {
    TULong data_offset;

    TU8 *p_data;
    TULong data_length;

    //data params
    TU8 data_type; // ELWDataType_xxx
    TU8 stream_index;
    TU8 is_key_frame;
    TU8 is_params_changes;

    TU32 sequence_count;

    TU32 video_width;
    TU32 video_height;

    TU8 video_frame_type; //ELWFrameType_xxx
    TU8 audio_channel_number;
    TU8 audio_sample_size;
    TU8 audio_is_channel_interleave;

    TU32 audio_sample_rate;
    TU32 audio_frame_size;

    TTime pts;
    TTime dts;
} SLWData;

typedef EECode(*TFLightweightDataCallback)(void *p_context, SLWData *output_data);

class ILightweightClient
{
public:
    virtual void Destroy() = 0;

protected:
    virtual ~ILightweightClient() {}

    virtual EECode ConnectToServer(TChar *server_url, TSocketPort port, TU32 active_mode) = 0;

public:
    //passive mode, for native client
    virtual EECode SetDataCallback(void *callback_context, TFLightweightDataCallback callback) = 0;
    //active mode, for ipc client
    virtual EECode ReadData(SLWData *output_data) = 0;
};

class ILightweightServer
{
public:
    virtual CObject *GetObject0() const = 0;

protected:
    virtual ~ILightweightServer() {}

public:
    virtual EECode CreateContext(TChar *url, TSocketPort port) = 0;
    virtual void DestroyContext() = 0;

    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

public:
    virtual EECode SendData(SLWData *input_data) = 0;

public:
    //for native passive mode client only
    virtual EECode RegisterNativeClient(ILightweightClient *native_client) = 0;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


