/*******************************************************************************
 * am_mp4_file_splitter_if.h
 *
 * History:
 *   2015-9-16 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_MP4_FILE_SPLITTER_IF_H_
#define AM_MP4_FILE_SPLITTER_IF_H_

#include "version.h"

class AMIMp4FileSplitter
{
  public :
    static AMIMp4FileSplitter* create(const char *source_mp4_file_path);
    virtual void destroy() = 0;
    virtual bool get_proper_mp4_file(const char* file_path,
                                     uint32_t begin_point_time,
                                     uint32_t end_point_time) = 0;
    virtual ~AMIMp4FileSplitter(){}
};



#endif /*AM_MP4_FILE_SPLITTER_IF_H_ */
