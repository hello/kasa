/*******************************************************************************
 * am_dec7z.h
 *
 * History:
 *   2015-3-9 - [longli] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_DEC7Z_H_
#define AM_DEC7Z_H_

#include "7z.h"
#include "7zFile.h"
#include "pba_upgrade.h"

class AMDec7z
{
  public:
    static AMDec7z *create(const std::string &filename);
    virtual bool get_filename_in_7z(std::string &name_list);
    virtual bool dec7z(const std::string &path);
    virtual void destroy();

  private:
    AMDec7z();
    virtual ~AMDec7z();
    bool init(const std::string &filename);
    bool create_dir(const std::string &path);
    AMDec7z(AMDec7z const &copy) = delete;
    AMDec7z& operator=(AMDec7z const &copy) = delete;

  private:
    CFileInStream m_archive_stream;
    ISzAlloc m_alloc_imp;
    ISzAlloc m_alloc_temp_imp;
    CLookToRead m_look_stream;
    CSzArEx m_db;
    bool m_f_open;
    bool m_ex_init;
};

#endif /* AM_DEC7Z_H_ */
