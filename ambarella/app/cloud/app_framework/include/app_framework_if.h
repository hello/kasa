/*******************************************************************************
 * app_framework_if.h
 *
 * History:
 *  2014/10/08 - [Zhi He] create file
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

#ifndef __APP_FRAMEWORK_IF_H__
#define __APP_FRAMEWORK_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

typedef enum {

    EInputEvent_key_0 = 0x30,
    EInputEvent_key_1 = 0x31,
    EInputEvent_key_2 = 0x32,
    EInputEvent_key_3 = 0x33,
    EInputEvent_key_4 = 0x34,
    EInputEvent_key_5 = 0x35,
    EInputEvent_key_6 = 0x36,
    EInputEvent_key_7 = 0x37,
    EInputEvent_key_8 = 0x38,
    EInputEvent_key_9 = 0x39,

    EInputEvent_key_colon = 0x3a,
    EInputEvent_key_subcolon = 0x3b,

    EInputEvent_key_A = 0x41,
    EInputEvent_key_B = 0x42,
    EInputEvent_key_C = 0x43,
    EInputEvent_key_D = 0x44,
    EInputEvent_key_E = 0x45,
    EInputEvent_key_F = 0x46,
    EInputEvent_key_G = 0x47,
    EInputEvent_key_H = 0x48,
    EInputEvent_key_I = 0x49,
    EInputEvent_key_J = 0x4a,
    EInputEvent_key_K = 0x4b,
    EInputEvent_key_L = 0x4c,
    EInputEvent_key_M = 0x4d,
    EInputEvent_key_N = 0x4e,
    EInputEvent_key_O = 0x4f,
    EInputEvent_key_P = 0x50,
    EInputEvent_key_Q = 0x51,
    EInputEvent_key_R = 0x52,
    EInputEvent_key_S = 0x53,
    EInputEvent_key_T = 0x54,
    EInputEvent_key_U = 0x55,
    EInputEvent_key_V = 0x56,
    EInputEvent_key_W = 0x57,
    EInputEvent_key_X = 0x58,
    EInputEvent_key_Y = 0x59,
    EInputEvent_key_Z = 0x5a,

    EInputEvent_key_a = 0x61,
    EInputEvent_key_b = 0x62,
    EInputEvent_key_c = 0x63,
    EInputEvent_key_d = 0x64,
    EInputEvent_key_e = 0x65,
    EInputEvent_key_f = 0x66,
    EInputEvent_key_g = 0x67,
    EInputEvent_key_h = 0x68,
    EInputEvent_key_i = 0x69,
    EInputEvent_key_j = 0x6a,
    EInputEvent_key_k = 0x6b,
    EInputEvent_key_l = 0x6c,
    EInputEvent_key_m = 0x6d,
    EInputEvent_key_n = 0x6e,
    EInputEvent_key_o = 0x6f,
    EInputEvent_key_p = 0x70,
    EInputEvent_key_q = 0x71,
    EInputEvent_key_r = 0x72,
    EInputEvent_key_s = 0x73,
    EInputEvent_key_t = 0x74,
    EInputEvent_key_u = 0x75,
    EInputEvent_key_v = 0x76,
    EInputEvent_key_w = 0x77,
    EInputEvent_key_x = 0x78,
    EInputEvent_key_y = 0x79,
    EInputEvent_key_z = 0x7a,

    EInputEvent_leftclick = 0x81,
    EInputEvent_rightclick = 0x82,
    EInputEvent_middleclick = 0x83,
    EInputEvent_doubleclick = 0x84,

} EInputEvent;

#define DGUIAreaFlags_focus (0x1 << 0)
#define DGUIAreaFlags_showing (0x1 << 1)

#define DGUIAreaFlags_needredraw (0x1 << 3)

//-----------------------------------------------------------------------
//
// CGUIArea
//
//-----------------------------------------------------------------------
typedef enum {
    //preset
    EGUIAreaType_Texture2D = 0x01,
} EGUIAreaType;

class CGUIArea
{
protected:
    CGUIArea();
    EECode Construct();
    virtual ~CGUIArea();

public:
    virtual EECode SetupContext();
    virtual EECode PrepareDrawing();
    virtual EECode Draw(TU32 &updated);
    virtual EECode Action(EInputEvent event);
    virtual void NeedReDraw(TU32 &needed) const;

    EECode SetDisplayArea(TDimension pos_x, TDimension pos_y, TDimension size_x, TDimension size_y);
    EECode SetDisplayAreaRelative(float pos_x, float pos_y, float size_x, float size_y);

    void Focus();
    void UnFocus();
    TU32 IsFocused() const;

public:
    virtual void Destroy();

protected:
    IMutex *mpMutex;

protected:
    TDimension mPosX;
    TDimension mPosY;
    TDimension mSizeX;
    TDimension mSizeY;

    float mRelativePosX;
    float mRelativePosY;
    float mRelativeSizeX;
    float mRelativeSizeY;

    TU32 mFlags;
};

CGUIArea *gfCreateGUIArea(EGUIAreaType type, IVisualDirectRendering *&p_direct_rendering);

//-----------------------------------------------------------------------
//
// CButton
//
//-----------------------------------------------------------------------
class CButton : public CGUIArea
{
    typedef CGUIArea inherited;

public:
    static CButton *Create();

protected:
    CButton();

    EECode Construct();
    virtual ~CButton();

public:
    virtual EECode Draw(TU32 &updated);
    virtual EECode Action(EInputEvent event);

public:
    virtual void Destroy();
};

//-----------------------------------------------------------------------
//
// CTextEditor
//
//-----------------------------------------------------------------------
class CTextEditor : public CGUIArea
{
    typedef CGUIArea inherited;

public:
    static CTextEditor *Create();

protected:
    CTextEditor();

    EECode Construct();
    virtual ~CTextEditor();

public:
    virtual EECode Draw(TU32 &updated);
    virtual EECode Action(EInputEvent event);

public:
    virtual void Destroy();
};

//-----------------------------------------------------------------------
//
// CListCtl
//
//-----------------------------------------------------------------------
class CListCtl : public CGUIArea
{
    typedef CGUIArea inherited;

public:
    static CListCtl *Create();

protected:
    CListCtl();

    EECode Construct();
    virtual ~CListCtl();

public:
    virtual EECode Draw(TU32 &updated);
    virtual EECode Action(EInputEvent event);

public:
    virtual void Destroy();
};

//-----------------------------------------------------------------------
//
// CTreeCtl
//
//-----------------------------------------------------------------------
class CTreeCtl : public CGUIArea
{
    typedef CGUIArea inherited;

public:
    static CTreeCtl *Create();

protected:
    CTreeCtl();

    EECode Construct();
    virtual ~CTreeCtl();

public:
    virtual EECode Draw(TU32 &updated);
    virtual EECode Action(EInputEvent event);

public:
    virtual void Destroy();
};

//-----------------------------------------------------------------------
//
// CGUIAreaCalculator
//
//-----------------------------------------------------------------------

typedef enum {
    EAlignMethod_noalignment = 0x00,
    EAlignMethod_vertical = 0x01,
    EAlignMethod_horizontal = 0x02,
    EAlignMethod_rectangle = 0x03,
} EAlignMethod;

class CIGUIAreaCalculator
{
public:
    EECode AddArea(CGUIArea *area);
    void RemoveAllArea();
    void RemoveArea(CGUIArea *area);

public:
    EECode SetDisplayArea(TDimension pos_x, TDimension pos_y, TDimension size_x, TDimension size_y);
    EECode SetDisplayAreaRelative(float pos_x, float pos_y, float size_x, float size_y);

    EECode Align(EAlignMethod method);

public:
    void Destroy();
};

//-----------------------------------------------------------------------
//
// CIGUILayer
//
//-----------------------------------------------------------------------
class CIGUILayer
{
public:
    static CIGUILayer *Create();

protected:
    CIGUILayer();

    EECode Construct();
    virtual ~CIGUILayer();

public:
    EECode AddArea(CGUIArea *area);
    void RemoveArea(CGUIArea *area);

public:
    EECode SetupContexts();
    EECode PrepareDrawing();
    EECode Draw(TU32 &updated);
    void NeedReDraw(TU32 &needed);

public:
    void Destroy();

private:
    CIDoubleLinkedList *mpAreaList;

};

//-----------------------------------------------------------------------
//
// CISoundTrack
//
//-----------------------------------------------------------------------
class ISoundComposer;

class CISoundTrack : public ISoundDirectRendering
{
public:
    static CISoundTrack *Create(TU32 buffer_size);

protected:
    CISoundTrack();

    EECode Construct(TU32 buffer_size);
    ~CISoundTrack();

public:
    virtual EECode SetConfig(TU32 samplerate, TU32 framesize, TU32 channel_number, TU32 sample_format);
    virtual EECode Render(SSoundDirectRenderingContent *p_context, TU32 reuse);
    virtual EECode ControlOutput(TU32 control_flag);

public:
    void Enable();
    void Disable();
    void SetSoundComposer(ISoundComposer *composer);

public:
    void Lock();
    void Unlock();
    void QueryData(TU8 *&p, TU32 &size);
    void ClearData();

public:
    void Destroy();

private:
    void wirteData(TU8 *p, TU32 size);

private:
    IMutex *mpMutex;
    ISoundComposer *mpComposer;

    TU8 *mpDataBuffer;
    TU32 mCurrentSize;
    TU32 mDataBufferSize;

private:
    TU8 mbEnabled;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;
};

//-----------------------------------------------------------------------
//
// CIScene
//
//-----------------------------------------------------------------------
class CIScene
{
public:
    static CIScene *Create();

protected:
    CIScene();

    EECode Construct();
    ~CIScene();

public:
    EECode AddLayer(CIGUILayer *layer);
    void RemoveLayer(CIGUILayer *layer);

public:
    EECode SetupContexts();
    EECode PrepareDrawing();
    EECode Draw(TU32 &updated);
    EECode Hide();

    void NeedReDraw(TU32 &needed);

public:
    void Destroy();

private:
    CIDoubleLinkedList *mpLayerList;
};

//-----------------------------------------------------------------------
//
// CISceneDirector
//
//-----------------------------------------------------------------------

typedef enum {
    ESceneDirectorState_idle = 0x0,
    ESceneDirectorState_running = 0x01,
    ESceneDirectorState_pending = 0x02,
    ESceneDirectorState_error = 0x03,
    ESceneDirectorState_stepmode = 0x04,
} ESceneDirectorState;

typedef enum {
    ESceneDirectorType_basic = 0x0,
} ESceneDirectorType;

class ISceneDirector
{
public:
    virtual EECode AddScene(CIScene *scene) = 0;
    virtual void RemoveScene(CIScene *scene) = 0;

public:
    virtual EECode SwitchScene(CIScene *scene) = 0;

public:
    virtual EECode StartRunning() = 0;
    virtual EECode ExitRunning() = 0;

public:
    virtual EECode PreferMainWindowSetting() = 0;
    virtual EECode PreferVisualSetting(EVisualSetting setting = EVisualSetting_opengl) = 0;
    virtual EECode PreferAudioSetting() = 0;

public:
    virtual EECode CreateMainWindow(const TChar *name, TDimension posx, TDimension posy, TDimension width, TDimension height, EWindowState state) = 0;
    virtual EECode MoveMainWindow(TDimension posx, TDimension posy) = 0;
    virtual EECode ReSizeMainWindow(TDimension width, TDimension height) = 0;
    virtual EECode ReSizeClientWindow(TDimension width, TDimension height) = 0;
    virtual TULong GetWindowHandle() const = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~ISceneDirector() {};
};

extern ISceneDirector *gfCreateSceneDirector(ESceneDirectorType type, EVisualSettingPrefer visual_platform_prefer, SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component);

//-----------------------------------------------------------------------
//
// ISoundComposer
//
//-----------------------------------------------------------------------

typedef enum {
    ESoundComposerState_idle = 0x0,
    ESoundComposerState_wait_config = 0x01,
    ESoundComposerState_preparing = 0x02,
    ESoundComposerState_running = 0x03,
    ESoundComposerState_pending = 0x04,
    ESoundComposerState_error = 0x05,
    ESoundComposerState_resample_1 = 0x06,
    ESoundComposerState_resample_2 = 0x07,
} ESoundComposerState;

typedef enum {
    ESoundComposerType_basic = 0x0,
} ESoundComposerType;

class ISoundComposer
{
public:
    virtual EECode SetHandle(TULong handle) = 0;
    virtual EECode SetConfig(TU32 samplerate, TU32 framesize, TU32 channel_number, TU32 sample_format) = 0;

public:
    virtual EECode RegisterTrack(CISoundTrack *track) = 0;
    virtual void UnRegisterTrack(CISoundTrack *track) = 0;

public:
    virtual void DataNotify() = 0;
    virtual void ControlNotify(TU32 control_flag) = 0;

public:
    virtual EECode StartRunning() = 0;
    virtual EECode ExitRunning() = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~ISoundComposer() {};
};

extern ISoundComposer *gfCreateSoundComposer(ESoundComposerType type, SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

