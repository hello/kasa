/*******************************************************************************
 * am_mp4_file_splitter.h
 *
 * History:
 *   2015-09-10 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef MP4_FILE_SPLITTER_H_
#define MP4_FILE_SPLITTER_H_
#include "am_mp4_file_splitter_if.h"

class AMMp4Parser;
class AMMp4FileSplitter : public AMIMp4FileSplitter
{
  public :
    static AMMp4FileSplitter* create(const char *source_mp4_file_path);
  public :
    virtual void destroy();
    virtual bool get_proper_mp4_file(const char* file_path,
                                        uint32_t begin_point_time,
                                        uint32_t end_point_time);
    virtual ~AMMp4FileSplitter();
  private :
    AMMp4FileSplitter();
    bool init(const char *source_mp4_file_path);
  private :
    AMMp4Parser *m_mp4_parser;
};




#endif /* MP4_FILE_SPLITTER_H_ */
