/*******************************************************************************
 * am_archive.h
 *
 * History:
 *   Apr 15, 2016 - [longli] created file
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
#ifndef AM_LIBARCHIVE_IF_H_
#define AM_LIBARCHIVE_IF_H_

enum AM_COMPRESS_TYPE
{
  AM_COMPRESS_TAR = 0,
  AM_COMPRESS_XZ,
  AM_COMPRESS_BZIP2,
  AM_COMPRESS_GZIP,
  AM_COMPRESS_COMPRESS,
  AM_COMPRESS_7Z,
  AM_COMPRESS_AUTO
};

class AMIArchive
{
  public:
    static AMIArchive* create();

    virtual bool compress(const std::string &dst_path,
                          const std::string &src_list,
                          const AM_COMPRESS_TYPE type = AM_COMPRESS_XZ,
                          const std::string &base_dir="") = 0;

    virtual bool get_file_list(const std::string &src_path,
                               std::string &name_list,
                               const AM_COMPRESS_TYPE type = AM_COMPRESS_AUTO) = 0;

    virtual bool decompress(const std::string &src_path,
                            const std::string &extract_path,
                            const AM_COMPRESS_TYPE type = AM_COMPRESS_AUTO) = 0;

    virtual ~AMIArchive(){};

};

#endif /* AM_LIBARCHIVE_IF_H_ */
