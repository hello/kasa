/*******************************************************************************
 * mp4_file_reader.h
 *
 * History:
 *   2015-09-07 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef MP4_FILE_READER_H_
#define MP4_FILE_READER_H_
class AMIFileReader;

class Mp4FileReader
{
  public :
    static Mp4FileReader* create(const char* file_name);
  private :
    Mp4FileReader();
    virtual ~Mp4FileReader();
    bool init(const char* file_name);
  public :
    void destroy();
  public :
    bool seek_file(uint32_t offset);//from the begin of the file
    bool advance_file(uint32_t offset);//from the current position
    int  read_data(uint8_t *data, uint32_t size);
    bool read_le_u8(uint8_t& value);
    bool read_le_s8(int8_t& value);
    bool read_le_u16(uint16_t& value);
    bool read_le_s16(int16_t& value);
    bool read_le_u24(uint32_t& value);
    bool read_le_s24(int32_t& value);
    bool read_le_u32(uint32_t& value);
    bool read_le_s32(int32_t& value);
    bool read_le_u64(uint64_t& value);
    bool read_le_s64(int64_t& value);
    bool read_box_tag(uint32_t& value);
    uint64_t get_file_size();
    void close_file();
  private :
    AMIFileReader *m_file_reader;
};

#endif /* MP4_FILE_READER_H_ */
