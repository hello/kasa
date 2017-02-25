/*******************************************************************************
 * cloud_sacp_client_agent.h
 *
 * History:
 *  2013/11/28 - [Zhi He] create file
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

#ifndef __CLOUD_SACP_CLIENT_AGENT_H__
#define __CLOUD_SACP_CLIENT_AGENT_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
//  CSACPCloudClientAgent
//
//-----------------------------------------------------------------------

class CSACPCloudClientAgent: public ICloudClientAgent
{
public:
    static CSACPCloudClientAgent *Create(TU16 local_port, TU16 server_port);

protected:
    EECode Construct();

    CSACPCloudClientAgent(TU16 local_port, TU16 server_port);
    virtual ~CSACPCloudClientAgent();

public:
    virtual EECode ConnectToServer(TChar *server_url, TU64 &hardware_authentication_input, TU16 local_port = 0, TU16 server_port = 0, TU8 *authentication_buffer = NULL, TU16 authentication_length = 0);
    //virtual EECode HardwareAuthenticate(TU32 hash_value);
    virtual EECode DisconnectToServer();

public:
    virtual EECode QueryVersion(SCloudAgentVersion *version);
    //virtual EECode QuerySourceList(SCloudSourceItems* items);

    //virtual EECode AquireChannelControl(TU32 channel_index);
    //virtual EECode ReleaseChannelControl(TU32 channel_index);

    virtual EECode GetLastErrorHint(EECode &last_error_code, TU32 &error_hint1, TU32 &error_hint2);

public:
    virtual EECode Uploading(TU8 *p_data, TMemSize size, ESACPDataChannelSubType data_type, TU8 extra_flag = 0);

public:
    virtual EECode UpdateSourceBitrate(TChar *source_channel, TU32 bitrate, TU32 reply = 0);
    virtual EECode UpdateSourceFramerate(TChar *source_channel, TU32 framerate, TU32 reply = 0);
    virtual EECode UpdateSourceDisplayLayout(TChar *source_channel, TU32 layout, TU32 focus_index = 0, TU32 reply = 0);
    virtual EECode UpdateSourceAudioParams(TU8 audio_format, TU8 audio_channnel_number, TU32 audio_sample_frequency, TU32 audio_bitrate, TU8 *audio_extradata, TU16 audio_extradata_size, TU32 reply = 0);

    virtual EECode SwapWindowContent(TChar *source_channel, TU32 window_id_1, TU32 window_id_2, TU32 reply = 0);
    virtual EECode CircularShiftWindowContent(TChar *source_channel, TU32 shift_count = 1, TU32 is_ccw = 0, TU32 reply = 0);

    virtual EECode SwitchVideoHDSource(TChar *source_channel, TU32 hd_index, TU32 reply = 0);
    virtual EECode SwitchAudioSourceIndex(TChar *source_channel, TU32 audio_index, TU32 reply = 0);
    virtual EECode SwitchAudioTargetIndex(TChar *source_channel, TU32 audio_index, TU32 reply = 0);
    virtual EECode ZoomSource(TChar *source_channel, TU32 window_index, float zoom_factor, float zoom_input_center_x, float zoom_input_center_y, TU32 reply = 0);
    virtual EECode ZoomSource2(TChar *source_channel, TU32 window_index, float width_factor, float height_factor, float input_center_x, float input_center_y, TU32 reply = 0);
    virtual EECode UpdateSourceWindow(TChar *source_channel, TU32 window_index, float pos_x, float pos_y, float size_x, float size_y, TU32 reply = 0);
    virtual EECode SourceWindowToTop(TChar *source_channel, TU32 window_index, TU32 reply = 0);
    virtual EECode ShowOtherWindows(TChar *source_channel, TU32 window_index, TU32 show, TU32 reply = 0);

    virtual EECode DemandIDR(TChar *source_channel, TU32 demand_param = 0, TU32 reply = 0);

    virtual EECode SwitchGroup(TUint group = 0, TU32 reply = 0);
    virtual EECode SeekTo(SStorageTime *time, TU32 reply = 0);
    virtual EECode TrickPlay(ERemoteTrickplay trickplay, TU32 reply = 0);
    virtual EECode FastForward(TUint try_speed, TU32 reply = 0);
    virtual EECode FastBackward(TUint try_speed, TU32 reply = 0);

    //for extention
    //virtual EECode CustomizedCommand(TChar* source_channel, TU32 param1, TU32 param2, TU32 param3, TU32 param4, TU32 param5, TU32 reply = 0);

    virtual EECode QueryStatus(SSyncStatus *status, TU32 reply = 0);
    virtual EECode SyncStatus(SSyncStatus *status, TU32 reply = 0);

public:
    virtual EECode WaitCmd(TU32 &cmd_type);
    virtual EECode PeekCmd(TU32 &type, TU32 &payload_size, TU16 &header_ext_size);
    virtual EECode ReadData(TU8 *p_data, TMemSize size);
    virtual EECode WriteData(TU8 *p_data, TMemSize size);

public:
    virtual TInt GetHandle() const;

public:
    virtual EECode DebugPrintCloudPipeline(TU32 index, TU32 param_0, TU32 reply = 0);
    virtual EECode DebugPrintStreamingPipeline(TU32 index, TU32 param_0, TU32 reply = 0);
    virtual EECode DebugPrintRecordingPipeline(TU32 index, TU32 param_0, TU32 reply = 0);
    virtual EECode DebugPrintChannel(TU32 index, TU32 param_0, TU32 reply = 0);
    virtual EECode DebugPrintAllChannels(TU32 param_0, TU32 reply = 0);

    //for externtion, textable API
    //    virtual EECode SendStringJson(TChar* input, TU32 input_size, TChar* output, TU32 output_size, TU32 reply = 1);
    //    virtual EECode SendStringXml(TChar* input, TU32 input_size, TChar* output, TU32 output_size, TU32 reply = 1);

public:
    virtual void Delete();

private:
    void fillHeader(TU32 type, TU32 size);

private:
    TU16 mLocalPort;
    TU16 mServerPort;

    TU8 mbConnected;
    TU8 mbAuthenticated;
    TU8 mbReserved5;
    TU8 mbGetPeerVersion;

    TSocketHandler mSocket;

    //
    TU32 mSeqCount;
    TU8 mbReceiveConnectStatus;
    TU8 mHeaderExtentionType;

    TU8 mEncryptionType1;
    TU8 mEncryptionType2;

    TU8 mbAquireControl;
    TU8 mReserved1;
    TU16 mControlChannelIndex;

private:
    TU8 *mpReservedBuffer;
    TMemSize mReservedDataSize;
    TMemSize mReservedBufferSize;

private:
    TU8 *mpSenderBuffer;
    TMemSize mSenderBufferSize;

private:
    SSACPHeader *mpSACPHeader;

private:
    SCloudAgentVersion mVersion;

private:
    TU32 mLastErrorHint1;
    TU32 mLastErrorHint2;
    EECode mLastErrorCode;

private:
    IMutex *mpMutex;
};


DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

