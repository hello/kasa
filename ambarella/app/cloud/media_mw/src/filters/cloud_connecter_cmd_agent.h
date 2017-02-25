/*******************************************************************************
 * cloud_connecter_cmd_agent.h
 *
 * History:
 *    2014/03/19 - [Zhi He] create file
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

#ifndef __CLOUD_CONNECTER_CMD_AGENT_H__
#define __CLOUD_CONNECTER_CMD_AGENT_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CCloudConnecterCmdAgent
//
//-----------------------------------------------------------------------
class CCloudConnecterCmdAgent
    : public CObject
    , public IFilter
{
    typedef CObject inherited;

public:
    static IFilter *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);
    virtual CObject *GetObject0() const;

protected:
    CCloudConnecterCmdAgent(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index = 0);

    EECode Construct();
    virtual ~CCloudConnecterCmdAgent();

public:
    virtual void Delete();

public:
    virtual EECode Initialize(TChar *p_param = NULL);
    virtual EECode Finalize();

    virtual EECode Run();
    virtual EECode Exit();

    virtual EECode Start();
    virtual EECode Stop();

    virtual void Pause();
    virtual void Resume();
    virtual void Flush();
    virtual void ResumeFlush();

    virtual EECode Suspend(TUint release_context = 0);
    virtual EECode ResumeSuspend(TComponentIndex input_index = 0);

    virtual EECode SwitchInput(TComponentIndex focus_input_index = 0);

    virtual EECode FlowControl(EBufferType flowcontrol_type);

    virtual EECode SendCmd(TUint cmd);
    virtual void PostMsg(TUint cmd);

    virtual EECode AddOutputPin(TUint &index, TUint &sub_index, TUint type);
    virtual EECode AddInputPin(TUint &index, TUint type);

    virtual IOutputPin *GetOutputPin(TUint index, TUint sub_index = 0);
    virtual IInputPin *GetInputPin(TUint index);

    virtual EECode FilterProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);

    virtual void GetInfo(INFO &info);
    virtual void PrintStatus();

public:
    EECode SendBuffer(CIBuffer *buffer, TU16 data_type);

public:
    static EECode CmdCallback(void *owner, TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4, TU8 *pcmd, TU32 cmdsize);
    static EECode DataCallback(void *owner, TMemSize read_length, TU16 data_type, TU8 extra_flag);

private:
    EECode ProcessCmdCallback(TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4, TU8 *pcmd, TU32 cmdsize);
    EECode ProcessDataCallback(TMemSize read_length, TU16 data_type, TU8 extra_flag);

private:
    void fillHeader(TU32 type, TU16 size);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;
    IMutex *mpMutex;

private:
    IScheduler *mpScheduler;
    ICloudServerAgent *mpAgent;
    EProtocolType mProtocolType;

private:
    TU8 mbRunning;
    TU8 mbStarted;
    TU8 mbSuspended;
    TU8 mbAddedToScheduler;

private:
    TChar *mpSourceUrl;
    TU32 mSourceUrlLength;

private:
    SSACPHeader mSACPHeader;

private:
    TU8 mbRelayCmd;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;

    TU16 mRelaySeqNumber0;
    TU16 mRelaySeqNumber1;

    IFilter *mpRelayTarget;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


