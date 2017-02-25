/*******************************************************************************
 * am_dec7z.h
 *
 * History:
 *   2015-3-9 - [longli] created file
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
#ifndef AM_DEC7Z_H_
#define AM_DEC7Z_H_

#include "7z.h"
#include "7zFile.h"
#include "am_dec7z_if.h"
#include <atomic>

class AMDec7z: public AMIDec7z
{
    friend class AMIDec7z;

  public:
    virtual bool get_filename_in_7z(std::string &name_list) override;
    virtual bool dec7z(const std::string &path) override;

  protected:
    static AMDec7z *create(const std::string &filename);
    virtual void release() override;
    virtual void inc_ref() override;

  private:
    AMDec7z();
    virtual ~AMDec7z();
    bool init(const std::string &filename);
    int32_t create_dir(const std::string &path);

  private:
    CFileInStream       m_archive_stream;
    ISzAlloc            m_alloc_imp;
    ISzAlloc            m_alloc_temp_imp;
    CLookToRead         m_look_stream;
    CSzArEx             m_db;
    std::atomic_int     m_ref_counter;
    bool                m_f_open;
    bool                m_ex_init;
};

#endif /* AM_DEC7Z_H_ */
