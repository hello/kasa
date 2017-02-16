/*******************************************************************************
 * Mp4FileWriter.h
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
#ifndef MP4_FILE_WRITER_H_
#define MP4_FILE_WRITER_H_

class AMIFileWriter;

class Mp4FileWriter
{
  public :
    static Mp4FileWriter* create(const char *file_name);
  private :
    Mp4FileWriter();
    virtual ~Mp4FileWriter();
    bool init(const char *file_name);
  public :
    void destroy();
    bool write_data(uint8_t *data, uint32_t data_len);
    bool seek_data(uint32_t offset, uint32_t whence);//SEEK_SET
    bool tell_file(uint64_t& offset);
    bool write_be_u8(uint8_t value);
    bool write_be_s8(int8_t value);
    bool write_be_u16(uint16_t value);
    bool write_be_s16(int16_t value);
    bool write_be_u24(uint32_t value);
    bool write_be_s24(int32_t value);
    bool write_be_u32(uint32_t value);
    bool write_be_s32(int32_t value);
    bool write_be_u64(uint64_t value);
    bool write_be_s64(int64_t value);
    void close_file();
  private :
    AMIFileWriter *m_file_writer;
};

#endif /* MP4_FILE_WRITER_H_ */
