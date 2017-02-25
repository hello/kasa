/*******************************************************************************
 * platform_al_if.h
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

#ifndef __PLATFORM_AL_IF_H__
#define __PLATFORM_AL_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

typedef enum {
    EVisualSourceType_Camera = 0x00,
    EVisualSourceType_WindowsDirectShow = 0x01,
} EVisualSourceType;

typedef enum {
    EAudioSourceType_Camera = 0x00,
    EAudioSourceType_DirectShow = 0x01,
} EAudioSourceType;

//EECode gfGetScreenNumber(TU32& number);
//EECode gfGetScreenResolution(TDimension& width, TDimension& height, TDimension& bpp, TDimension& frequency, TU32 index = 0);

typedef struct {
    float coordinate[3];
    TS16 nomal[3];
} VCOM_SMeshVert;

typedef struct {
    TU32 v1, v2;
} VCOM_SMeshEdge;

typedef struct {
    TU32 v1, v2, v3, v4;
} VCOM_SMeshFace;


typedef struct {
    float uv[4][2];
    TS8 flag, transp;
    TS16 mode, tile, unwrap;
} VCOM_SMeshTFace;

typedef struct {
    TU32 def_nr;
    float weight;
} VCOM_SMeshDeformWeight;

typedef struct {
    TU32 *dw_index;//rangerayu,caution,this need to be checked
    TInt totweight;
    TInt flag;  /* flag only in use for weightpaint now */
} VCOM_SMeshDeformVert;

typedef struct {
    TS8 a, r, g, b;
} VCOM_SMeshColFace;

typedef struct {
    VCOM_SMeshVert *p_verts;
    VCOM_SMeshEdge *p_edges;
    VCOM_SMeshFace *p_faces;
    VCOM_SMeshTFace *p_tfaces;
    VCOM_SMeshDeformVert *p_dverts;
    VCOM_SMeshColFace *p_colors;
    TU32 num_of_verts, num_of_edges, num_of_faces, num_of_dweights;
    VCOM_SMeshDeformWeight *p_dweights;//all shared dweights
} VCOM_SMesh;

//new mesh related end

//amature,bone,pose,action related begin

typedef struct {
    float vec[3][3];
    float alfa, weight, radius; /* alfa: tilt in 3D View, weight: used for softbody goal weight, radius: for bevel tapering */
    TS16 h1, h2;
    TS8 f1, f2, f3, hide;
} VCOM_SCurveBezTriple;

typedef struct {
    VCOM_SCurveBezTriple *bez_list;
    TS32 totvert;
    float curval;

    TS16 blocktype, adrcode, vartype;    /* blocktype= ipo-blocktype; adrcode= type of ipo-curve; vartype= 'format' of data */
    TS16 ipo, extrap;    /* interpolation and extrapolation modes  */
    TS16 flag, rt;   /* flag= settings; rt= ??? */
    float ymin, ymax;   /* minimum/maximum y-extents for curve */
    TS32 bitmask;   /* ??? */
    float slide_min, slide_max;  /* minimum/maximum values for sliders (in action editor) */
} VCOM_SCurveBez;

typedef struct _s_VCOM_SArmatureBone {
    struct _s_VCOM_SArmatureBone *parent_bone;
    TU32 number_of_childs;
    struct _s_VCOM_SArmatureBone *child_bones;
    TChar name[32];    /*  Name of the bone - must be unique within the armature */
    TS32 flag;
    float roll;   /*  roll is input for editmode, length calculated */
    float head[3];
    float tail[3];    /*    head/tail and roll in Bone Space    */
    float bone_mat[3][3]; /*  rotation derived from head/tail/roll */
    float arm_head[3];
    float arm_tail[3];    /*    head/tail and roll in Armature Space (rest pos) */
    float arm_mat[4][4];  /*  matrix: (bonemat(b)+head(b))*arm_mat(b-1), rest pos*/
    float dist, weight;   /*  dist, weight: for non-deformgroup deforms */
    float xwidth, length, zwidth; /*  width: for block bones. keep in this order, transform! */
    float ease1, ease2;   /*  length of bezier handles */
    float rad_head, rad_tail; /* radius for head/tail sphere, defining deform as well, parent->rad_tip overrides rad_head*/
} VCOM_SArmatureBone;

typedef struct {
    TChar name[32];

    float loc[3];
    float size[3];
    float quat[4];

    float chan_mat[4][4];
    float pose_mat[4][4];

    float pose_head[3];
    float pose_tail[3];

    VCOM_SArmatureBone *bone;
    TU32 parent_index;
    TU32 child_index;
    float limitmin[3];
    float limitmax[3];
    float stiffness[3];
    float ikstretch;

    float *path;
    TS16 flag;
    TS16 constflag;
    TS16 ikflag;
    TS16 selectflag;
    TS16 protectflag;
    TS32 pathlen;
    TS32 pathsf;
    TS32 pathef;
} VCOM_SArmaturePoseChannel;

typedef struct {
    TU32 num_of_posechannel;
    VCOM_SArmaturePoseChannel *posechannel_list;
    TS16 flag, proxy_layer;    /* proxy layer: copy from armature, gets synced */
    float ctime;    /* local action time of this pose */
    float stride_offset[3]; /* applied to object */
    float cyclic_offset[3]; /* result of match and cycles, applied in where_is_pose() */
} VCOM_SArmaturePose;

typedef struct {
    VCOM_SCurveBez *curve_list;
    TU32 num_of_curve;

    SRectF rctf;
    TU16 blocktype, showkey;
} VCOM_SArmatureIPO;

typedef struct {
    VCOM_SArmatureIPO ipo;

    TS32 flag;
    TChar  name[32];
} VCOM_SArmatureActionChannel;

typedef struct {
    VCOM_SArmatureActionChannel *action_list;
    TU32 num_of_actionchannels;
} VCOM_SArmatureAction;

typedef struct {
    TU32 num_of_bones;
    VCOM_SArmatureBone *bone_list;
    TS32 flag;

    VCOM_SArmatureAction action;

    VCOM_SArmaturePose pose;
} VCOM_SArmature;


//amature,bone,pose,action related end

typedef struct {
    TU32 width, height;
    TU32 *p_data;
} VCOM_STextureData;

typedef enum {
    VCOM_EImageDataFormat_none = 0,
    VCOM_EImageDataFormat_RGBA32,
} VCOM_TImageDataFormat;

typedef struct {
    TU32 width, height;
    TU8 *p_data;
    VCOM_TImageDataFormat format;
} VCOM_SImageData;


typedef struct {
    TS32 event_type;
    TS32 type_ext;
    TS32 value1;
    TS32 value2;
} VCOM_SEvent;

typedef enum {
    VCOM_ERuningStatus_uninited = 0,
    VCOM_ERuningStatus_inited,
} VCOM_TRuningStatus;

//key mapping
typedef enum {
    VCOM_EEventType_none = 0,
    VCOM_EEventType_keyPress,
    VCOM_EEventType_keyRelease,
    VCOM_EEventType_leftButtonPress,
    VCOM_EEventType_leftButtonRelease,
    VCOM_EEventType_rightButtonPress,
    VCOM_EEventType_rightButtonRelease,
    VCOM_EEventType_middleButtonPress,
    VCOM_EEventType_middleButtonRelease,
    VCOM_EEventType_sysCmd,
} VCOM_TEventType;

typedef enum {
    VCOM_ESysCmdValue_none = 0,
    VCOM_ESysCmdValue_closeWin,
    VCOM_ESysCmdValue_size,
} VCOM_TSysCmdValue;

typedef enum {
    VCOM_EKeyValue_none = 0,
    VCOM_EKeyValue_keyA,
    VCOM_EKeyValue_keyB,
    VCOM_EKeyValue_keyC,
    VCOM_EKeyValue_keyD,
    VCOM_EKeyValue_keyE,
    VCOM_EKeyValue_keyF,
    VCOM_EKeyValue_keyG,
    VCOM_EKeyValue_keyH,
    VCOM_EKeyValue_keyI,
    VCOM_EKeyValue_keyJ,
    VCOM_EKeyValue_keyK,
    VCOM_EKeyValue_keyL,
    VCOM_EKeyValue_keyM,
    VCOM_EKeyValue_keyN,
    VCOM_EKeyValue_keyO,
    VCOM_EKeyValue_keyP,
    VCOM_EKeyValue_keyQ,
    VCOM_EKeyValue_keyR,
    VCOM_EKeyValue_keyS,
    VCOM_EKeyValue_keyT,
    VCOM_EKeyValue_keyU,
    VCOM_EKeyValue_keyV,
    VCOM_EKeyValue_keyW,
    VCOM_EKeyValue_keyX,
    VCOM_EKeyValue_keyY,
    VCOM_EKeyValue_keyZ,

    VCOM_EKeyValue_keya,
    VCOM_EKeyValue_keyb,
    VCOM_EKeyValue_keyc,
    VCOM_EKeyValue_keyd,
    VCOM_EKeyValue_keye,
    VCOM_EKeyValue_keyf,
    VCOM_EKeyValue_keyg,
    VCOM_EKeyValue_keyh,
    VCOM_EKeyValue_keyi,
    VCOM_EKeyValue_keyj,
    VCOM_EKeyValue_keyk,
    VCOM_EKeyValue_keyl,
    VCOM_EKeyValue_keym,
    VCOM_EKeyValue_keyn,
    VCOM_EKeyValue_keyo,
    VCOM_EKeyValue_keyp,
    VCOM_EKeyValue_keyq,
    VCOM_EKeyValue_keyr,
    VCOM_EKeyValue_keys,
    VCOM_EKeyValue_keyt,
    VCOM_EKeyValue_keyu,
    VCOM_EKeyValue_keyv,
    VCOM_EKeyValue_keyw,
    VCOM_EKeyValue_keyx,
    VCOM_EKeyValue_keyy,
    VCOM_EKeyValue_keyz,

    VCOM_EKeyValue_key1,
    VCOM_EKeyValue_key2,
    VCOM_EKeyValue_key3,
    VCOM_EKeyValue_key4,
    VCOM_EKeyValue_key5,
    VCOM_EKeyValue_key6,
    VCOM_EKeyValue_key7,
    VCOM_EKeyValue_key8,
    VCOM_EKeyValue_key9,
    VCOM_EKeyValue_key0,

    VCOM_EKeyValue_keyF1,
    VCOM_EKeyValue_keyF2,
    VCOM_EKeyValue_keyF3,
    VCOM_EKeyValue_keyF4,
    VCOM_EKeyValue_keyF5,
    VCOM_EKeyValue_keyF6,
    VCOM_EKeyValue_keyF7,
    VCOM_EKeyValue_keyF8,
    VCOM_EKeyValue_keyF9,
    VCOM_EKeyValue_keyF10,
    VCOM_EKeyValue_keyF11,
    VCOM_EKeyValue_keyF12,

    VCOM_EKeyValue_keyCtl,
    VCOM_EKeyValue_keyShift,
    VCOM_EKeyValue_keyAlt,
    VCOM_EKeyValue_keyWin,
    VCOM_EKeyValue_keyEsc,
    VCOM_EKeyValue_keyTab,
    VCOM_EKeyValue_keySpace,
    VCOM_EKeyValue_keyEnter,
    VCOM_EKeyValue_keyNumlock,
    //tobe continued
} VCOM_TKeyValue;

typedef enum {
    VCOM_EKeyModifier_none = 0,
    VCOM_EKeyModifier_Ctl = 1,
    VCOM_EKeyModifier_Shift = 2,
    VCOM_EKeyModifier_Alt = 4,
} VCOM_TKeyModifier;

//-----------------------------------------------------------------------
//
// CIMesh
//
//-----------------------------------------------------------------------

class CIMesh
{
public:
    static CIMesh *Create();
    void Destroy();

protected:
    CIMesh();

    EECode Construct();
    virtual ~CIMesh();

public:
    EECode LoadDataFromFile(TChar *filename);
    EECode LoadDataFromMemory(void *p_data);

    EECode FreeMeshData();

public:
    TU8 mbLoaded;

public:
    VCOM_SMesh mMeshData;
};


//-----------------------------------------------------------------------
//
// ITexture2D
//
//-----------------------------------------------------------------------
typedef enum {
    ETextureSourceType_RGBA32 = 0x00,
    ETextureSourceType_YUV420p = 0x01,
} ETextureSourceType;

typedef struct {
    TU8 *pdata[4];
    TU32 linesize[4];

    TDimension width;
    TDimension height;

#ifdef DCONFIG_TEST_END2END_DELAY
    TTime debug_time;
    TU64 debug_count;
#endif

} STextureDataSource;

class ITexture2D
{
public:
    virtual EECode SetupContext() = 0;
    virtual void DestroyContext() = 0;

public:
    virtual EECode UpdateSourceRect(SRect *crop_rect) = 0;
    virtual EECode UpdateDisplayQuad(SQuanVertext3F *quan_vert) = 0;
    virtual EECode PrepareTexture() = 0;
    virtual EECode DrawTexture() = 0;
    virtual EECode UpdateData(STextureDataSource *p_data_source) = 0;

protected:
    virtual ~ITexture2D() {}
};

extern ITexture2D *gfCreate2DTexture(ETextureSourceType type);

//-----------------------------------------------------------------------
//
// ISoundOutput
//
//-----------------------------------------------------------------------
typedef struct {
    TU8 *pdata[8];
    TU32 datasize;

    TU32 samples; // not used yet
} SSoundDataSource;

typedef struct {
    TU8 channels;
    TU8 sample_format;
    TU8 is_channel_interlave;
    TU8 bytes_per_sample;

    TU32 sample_frequency;
    TU32 frame_size;
} SSoundConfigure;

class ISoundOutput
{
public:
    virtual EECode SetupContext(SSoundConfigure *config, TULong handle) = 0;
    virtual void DestroyContext() = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~ISoundOutput() {}

public:
    virtual EECode Start() = 0;
    virtual EECode Stop() = 0;

public:
    virtual EECode WriteSoundData(TU8 *p, TU32 size) = 0;

public:
    virtual EECode QueryBufferSize(TULong &mem_size) const = 0;
    virtual EECode QueryCurrentPosition(TULong &write_offset, TULong &play_offset) = 0;
    virtual EECode SetCurrentPlayPosition(TULong play_offset) = 0;
};

extern ISoundOutput *gfCreatePlatformSoundOutput(ESoundSetting setting, SPersistCommonConfig *pPersistCommonConfig);

//-----------------------------------------------------------------------
//
// IPlatformWindow
//
//-----------------------------------------------------------------------

class IPlatformWindow
{
public:
    virtual EECode Initialize() = 0;
    virtual void DeInitialize() = 0;
    virtual EECode CreateMainWindow(const TChar *name, TDimension posx, TDimension posy, TDimension width, TDimension height, EWindowState state) = 0;
    virtual EECode MoveMainWindow(TDimension posx, TDimension posy) = 0;
    virtual EECode ReSizeMainWindow(TDimension width, TDimension height) = 0;
    virtual EECode ReSizeClientWindow(TDimension width, TDimension height) = 0;
    virtual void DestroyMainWindow() = 0;

public:
    virtual EECode GetWindowRect(SRect &rect) = 0;
    virtual EECode GetClientRect(SRect &rect) = 0;

    virtual EECode SetClientWidth(TS32 width) = 0;
    virtual EECode SetClientHeight(TS32 height) = 0;
    virtual EECode SetClientSize(TDimension width, TDimension height) = 0;

    virtual EWindowState GetWindowState() const = 0;
    virtual EECode SetWindowState(EWindowState state) = 0;

    virtual EECode PreDrawing() = 0;
    virtual EECode InitOrthoDrawing(TDimension width, TDimension height) = 0;
    virtual EECode SwapBuffers() = 0;
    virtual EECode ActivateDrawingContext() = 0;
    virtual EECode Invalidate() = 0;
    virtual EECode LoadCursor(TU8 visible, EMouseShape cursor) const = 0;
    virtual EECode ProcessEvent() = 0;

    virtual TULong GetWindowHandle() const = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IPlatformWindow() {}
};

extern IPlatformWindow *gfCreatePlatformWindow(CIQueue *p_msg_queue, EVisualSetting setting, EVisualSettingPrefer prefer);

//-----------------------------------------------------------------------
//
// IFont
//
//-----------------------------------------------------------------------

typedef enum {
    ELWFontType_FT2 = 0x00,
} ELWFontType;

typedef enum {
    ECharMap_Invalid = 0x00,
    ECharMap_ASCII = 0x01,
    ECharMap_Unicode16 = 0x02,
} ECharMap;

typedef struct {
    TU8 *pmem;
    TU32 width;
    TU32 height;
    TU32 pitch;

    TS32 hor_bearing_x;
    TS32 hor_bearing_y;
    TS32 hor_advance;

    TS32 vert_bearing_x;
    TS32 vert_bearing_y;
    TS32 vert_advance;
} SGlyphData;

class IFont
{
protected:
    virtual ~IFont() {}

public:
    virtual void Destroy() = 0;

public:
    virtual EECode Load(TChar *filename) = 0;
    virtual void UnLoad() = 0;

    virtual EECode SetCharMap(ECharMap map) = 0;
    virtual EECode SetSize(TU32 width, TU32 height) = 0;
    virtual EECode GetSize(TU32 &width, TU32 &height) const = 0;

    virtual EECode GetAsciiGlyph(TChar c, SGlyphData *p_glyph) = 0;
    virtual EECode GetUTF8Glyph(TChar *utf8, TU32 &utf8_len, SGlyphData *p_glyph) = 0;
    virtual EECode GetUnicode16Glyph(TS16 uchar, SGlyphData *p_glyph) = 0;
};

IFont *gfCreateLWFont(ELWFontType type);

//-----------------------------------------------------------------------
//
// CIFontManager
//
//-----------------------------------------------------------------------
typedef enum {
    EFontCategory_UI_0 = 0x00, // for ui buttons
    EFontCategory_UI_1 = 0x01,
    EFontCategory_UI_2 = 0x02,
    EFontCategory_UI_3 = 0x03,
    EFontCategory_Text_0 = 0x04, //for ui text
    EFontCategory_Text_1 = 0x05,
    EFontCategory_Text_2 = 0x06,
    EFontCategory_Text_3 = 0x07,
    EFontCategory_Custom_0 = 0x08,
    EFontCategory_Custom_1 = 0x09,
    EFontCategory_Custom_2 = 0x0a,
    EFontCategory_Custom_3 = 0x0b,

    EFontCategory_TotalNumber = 0x0c,
} EFontCategory;

class CIFontManager
{
public:
    static CIFontManager *GetInstance();

protected:
    CIFontManager();
    virtual ~CIFontManager();

public:
    virtual EECode RegisterFont(IFont *font, EFontCategory cat);
    virtual EECode UnRegisterFont(IFont *font);

    virtual IFont *GetFont(EFontCategory cat) const;

protected:
    static CIFontManager *mpInstance;

protected:
    IFont *mpFontEntry[EFontCategory_TotalNumber];
};

//gpu al, drawer

typedef enum {
    EGPUDrawerType_none = 0,
    EGPUDrawerType_OpenGL = 0x01,
} EGPUDrawerType;

typedef enum {
    EImageDataFormat_none = 0,
    EImageDataFormat_RGBA32 = 0x01,
} EImageDataFormat;

typedef struct {
    TU32 width, height;
    TU8 *p_data;
    EImageDataFormat format;
} SImageData;

typedef EECode (*TFGPUALDrawRGBA32Rect)(SImageData *data, SRect *rect);

typedef struct {
    TFGPUALDrawRGBA32Rect f_draw_rgba32_rect;
} SFGPUDrawerAL;

void gfSetupGPUDrawerAlContext(SFGPUDrawerAL *al, EGPUDrawerType type = EGPUDrawerType_OpenGL);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

