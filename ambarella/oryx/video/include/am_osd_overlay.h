/*******************************************************************************
 * am_osd_overlay.h
 *
 * Histroy:
 *  2012-2-20 - [ypchang] created file
 *  2014-6-24 - [Louis ] modified and created am_video_types.h, removed non video stuffs
 *  2015-6-24 - [Huaiqing Wang] reconstruct
 * Copyright (C) 2008-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef AM_OSD_OVERLAY_H_
#define AM_OSD_OVERLAY_H_

#include "am_thread.h"
#include "iav_ioctl.h"
#include "am_osd_overlay_if.h"

#define OSD_OBJ_MAX_NUM          (AM_STREAM_MAX_NUM * OSD_AREA_MAX_NUM)
#define OSD_CLUT_MAX_NUM         (OSD_OBJ_MAX_NUM) // max CLUT tables
#define OSD_CLUT_SIZE            (1024)  //256 entries * 4 bytes each entry
#define OSD_CLUT_OFFSET          (0)
#define OSD_YUV_OFFSET           (OSD_CLUT_MAX_NUM * OSD_CLUT_SIZE)

#define OSD_TIME_STRING_MAX_SIZE (128)
#define OSD_DIGIT_NUM            (10)
#define OSD_LETTER_NUM           (26)  // Alphabet number

#define OSD_BMP_MAGIC            (0x4D42)
#define OSD_BMP_BIT              (8)   //just support 8 bit bitmap

#define OSD_PITCH_ALIGN          (0x20)
#define OSD_WIDTH_ALIGN          (0x10)
#define OSD_OFFSET_X_ALIGN       (0x2)
#define OSD_HEIGHT_ALIGN         (0x4)
#define OSD_OFFSET_Y_ALIGN       (0x4)

//any possible modes of OSD overlay mixing,  not all can be supported by HW,
enum AMOSDColorMode
{
  AM_OSD_COLOR_8BIT_CLUT_YUVA = 0, //A5s compatible YUVA 8-bit CLUT, default for S2L
  AM_OSD_COLOR_8BIT_CLUT_RGBA = 1,
  AM_OSD_COLOR_16BIT_RGBA_5551 = 2,
  AM_OSD_COLOR_16BIT_YUVA_5551 = 3,
  AM_OSD_COLOR_32BIT_RGBA = 4,
  AM_OSD_COLOR_32BIT_YUVA = 5,
};

//rgb clut, if 256 bit map it needs,
struct AMOSDRGB
{
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t reserved;
};

struct AMOSDBmpFileHeader
{
    uint16_t  bfType;     // file type
    uint32_t  bfSize;     //file size
    uint16_t  bfReserved1;
    uint16_t  bfReserved2;
    uint32_t  bfOffBits;
};

struct AMOSDBmpInfoHeader
{
    uint32_t  biSize;
    uint32_t  biWidth;     //bmp width
    uint32_t  biHeight;    //bmp height
    uint16_t  biPlanes;
    uint16_t  biBitCount;  // 1,4,8,16,24 ,32 color attribute
    uint32_t  biCompression;
    uint32_t  biSizeImage;   //Image size
    uint32_t  biXPelsPerMerer;
    uint32_t  biYPelsPerMerer;
    uint32_t  biClrUsed;
    uint32_t  biClrImportant;
};

class AMOSD
{
  public:
    AMOSD();
    virtual ~AMOSD() { destroy();}
    virtual bool create(AM_STREAM_ID stream_id, int32_t area_id, AMOSDArea *osd_area); //initial and check parameter
    virtual bool render(AM_STREAM_ID stream_id, int32_t area_id) = 0; //fill clut and data into overlay memory
    virtual void update() = 0; //update object with new data
    bool save_attribute(AMOSDAttribute *attr);

  protected:
    void set_overlay_size();
    void update_area_info(AMOSDArea *area);
    bool get_encode_format(AM_STREAM_ID stream_id);
    bool fill_overlay_data();
    bool check_overlay();
    bool alloc_display_mem();
    bool rotate_fill(uint8_t *dst, uint32_t dst_pitch, uint32_t dst_width,
                     uint32_t dst_height, uint32_t dst_x, uint32_t dst_y,
                     uint8_t *src, uint32_t src_pitch, uint32_t src_height,
                     uint32_t src_x, uint32_t src_y, uint32_t data_width, uint32_t data_height);

    AMOSDAttribute  m_attribute;
    AM_STREAM_ID    m_streamid;
    uint8_t         m_alpha; //0~255  0:non-transparent 1:full transparent
    AMOSDCLUT       *m_clut; //color look up table for 8-bit CLUT mode use mmap from iav driver
    uint8_t         *m_data; //OSD data buffer (2D) use mmap from iav driver
    uint16_t        m_startx; //area offset x
    uint16_t        m_starty; //area offset y
    uint16_t        m_width;  //area width set to mmap memory
    uint16_t        m_height; //area height set to mmap memory
    uint32_t        m_pitch; //data buffer pitch in bytes
    uint32_t        m_color_num; //color number, <= 256
    uint32_t        m_flip_rotate; //area osd flip and rotate flag
    uint16_t        m_area_w; //app local buffer width
    uint16_t        m_area_h; //app local buffer height
    uint8_t         *m_buffer; //app local buffer for manipulate data
    uint32_t        m_stream_w; //encode stream width
    uint32_t        m_stream_h; //encode stream height

  private:
    void destroy();
    void set_positon(AMOSDLayout *position);
};

class AMTestPatternOSD: public AMOSD
{
  public:
    AMTestPatternOSD() {};
    ~AMTestPatternOSD() {};
    virtual bool create(AM_STREAM_ID stream_id, int32_t area_id, AMOSDArea *osd_area);
    virtual bool render(AM_STREAM_ID stream_id, int32_t area_id);
    virtual void update() {};

  private:
    void fill_pattern_clut(uint32_t clut_offset);
    bool create_pattern_data(uint32_t area_size);
};

class AMPictureOSD: public AMOSD
{
  public:
    AMPictureOSD():m_fp(NULL) {};
    ~AMPictureOSD() {};
    virtual bool create(AM_STREAM_ID stream_id, int32_t area_id, AMOSDArea *osd_area);
    virtual bool render(AM_STREAM_ID stream_id, int32_t area_id);
    virtual void update() {};

  private:
    bool init_bitmap_info(FILE *fp);
    void fill_bitmap_clut(FILE *fp);
    bool create_bitmap_data(FILE *fp);

    std::string m_pic_filename;
    FILE *m_fp;
};

class AMTextOSD: public AMOSD
{
  public:
    AMTextOSD();
    virtual ~AMTextOSD() {};
    virtual bool create(AM_STREAM_ID stream_id, int32_t area_id, AMOSDArea *osd_area);
    virtual bool render(AM_STREAM_ID stream_id, int32_t area_id);
    virtual void update() {};

  protected:
    bool find_available_font(char *font);
    bool init_text_info(AMOSDTextBox *textbox);
    void fill_text_clut();
    bool open_textinsert_lib();
    bool close_textinsert_lib();
    bool create_text_data(char *str);
    bool char_to_wchar(wchar_t *wide_str, const char *str, uint32_t max_len);

    AMOSDFont     m_font;
    AMOSDCLUT     m_font_color;
    AMOSDCLUT     m_outline_color;
    AMOSDCLUT     m_background_color;
    uint32_t      m_font_width;
    uint32_t      m_font_height;
    uint32_t      m_font_pitch;
};

class AMTimeTextOSD: public AMTextOSD
{
  public:
    AMTimeTextOSD();
    ~AMTimeTextOSD();
    virtual bool create(AM_STREAM_ID stream_id, int32_t area_id, AMOSDArea *osd_area);
    virtual bool render(AM_STREAM_ID stream_id, int32_t area_id);
    virtual void update();

  private:
    void get_time_string(char *time_str);
    bool prepare_time_data();
    bool is_lower_letter(char c) {return c >= 'a' && c <= 'z';}
    bool is_upper_letter(char c) {return c >= 'A' && c <= 'Z';}
    bool is_digit(char c) {return c >= '0' && c <= '9';}
    bool create_time_data(char *str);

    uint32_t    m_time_length;
    uint32_t    m_offsetx[OSD_TIME_STRING_MAX_SIZE];
    uint32_t    m_offsety[OSD_TIME_STRING_MAX_SIZE];
    uint8_t     *m_digit[OSD_DIGIT_NUM];
    uint8_t     *m_upper_letter[OSD_LETTER_NUM];
    uint8_t     *m_lower_letter[OSD_LETTER_NUM];
    char        m_time_string[OSD_FILENAME_MAX_NUM];
};

//AMOSDOverlay and stream are in 1:1  relationship
class AMOSDOverlay: public AMIOSDOverlay
{
  public:
    static AMOSDOverlay* create();
    virtual AM_RESULT init(int32_t fd_iav, AMEncodeDevice *encode_device);
    virtual bool destroy();
    virtual bool add(AM_STREAM_ID stream_id, AMOSDInfo *overlay_info);
    virtual bool remove(AM_STREAM_ID stream_id, int32_t area_id = OSD_AREA_MAX_NUM);
    virtual bool enable(AM_STREAM_ID stream_id, int32_t area_id = OSD_AREA_MAX_NUM);
    virtual bool disable(AM_STREAM_ID stream_id, int32_t area_id = OSD_AREA_MAX_NUM);
    virtual void print_osd_infomation(AM_STREAM_ID stream_id = AM_STREAM_MAX_NUM);

    int32_t get_iva_hanle();//get m_iva
    int32_t get_area_buffer_maxsize() {return m_area_buffer_size;}
    AMMemMapInfo* get_mem_info() {return &m_osd_mem;} //get mmap addr and length
    AMOSDArea* get_object_area_info(int32_t id); //get osd object area info

    static AMOSDOverlay *m_osd_instance; //instance handle

  protected:
    AMOSDOverlay();
    virtual ~AMOSDOverlay() { destroy();}

  private:
    bool add_osd_area(AM_STREAM_ID stream_id, int32_t area_id, AMOSDAttribute *osd_attribute,
              AMOSDArea *osd_area); //initial a osd area information called by add
    void apply(AM_STREAM_ID stream_id); //call IAV ioctl to insert OSD overlay to video
    bool map_overlay();
    bool unmap_overlay();
    bool check_stream_id(AM_STREAM_ID stream_id);
    bool check_area_id(int32_t area_id);
    bool check_encode_state();
    bool get_osd_object_id_range(AM_STREAM_ID stream_id, int32_t area_id,
                                 int32_t &begin, int32_t &end);
    AMOSD *create_osd_object(AM_STREAM_ID stream_id, AMOSDType type);
    static void m_thread_func(void*); //used to update time osd for all stream

    AMEncodeDevice *m_encode_device;
    int32_t       m_iav; //iav file handle
    int32_t       m_area_buffer_size;  //max buffer size for each area
    int32_t       m_stream_buffer_size; //max buffer size for each stream
    int32_t       m_time_num; //stream number which running time osd
    bool          m_is_mapped;
    AMMemMapInfo  m_osd_mem;
    AMOSDArea     m_osd_area[OSD_OBJ_MAX_NUM]; //area info used for create area osd
    AMOSDLayout   m_layout[OSD_OBJ_MAX_NUM]; //not use current
    AMOSD         *m_osd[OSD_OBJ_MAX_NUM]; //all area osd object pointer
    AMThread      *m_thread_hand;   //time osd thread hand
    AMOSD         *m_time_osd[AM_STREAM_MAX_NUM]; //backup time osd pointer for every stream,
    //each stream just have one time osd
};
#endif
