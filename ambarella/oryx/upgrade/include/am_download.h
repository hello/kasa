/*******************************************************************************
 * am_download.h
 *
 * History:
 *   2015-1-7 - [longli] created file
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
#ifndef ORYX_UPGRADE_INCLUDE_AM_DOWNLOAD_H_
#define ORYX_UPGRADE_INCLUDE_AM_DOWNLOAD_H_

class AMDownload
{
  public:
    static AMDownload* create();
    /* src_file:from where to download  dst_file:where to save download file*/
    virtual bool set_url(const std::string &src_url,
                         const std::string &dst_path);
    virtual int32_t get_dl_file_size();
    /* set download authentication if needed */
    virtual bool set_dl_user_passwd(const std::string &user_name,
                                 const std::string &passwd);
    virtual bool set_dl_connect_timeout(const uint32_t timeout);
    /* If the download receives less than "min_speed" bytes/second
     * during "timeout" seconds, the operations is aborted.*/
    virtual bool set_low_speed_limit(const uint32_t min_speed,
                                     const uint32_t lowspeed_timeout);
    virtual bool set_show_progress(bool show);
    virtual bool download();
    virtual void destroy();

  private:
    AMDownload();
    virtual ~AMDownload();
    bool construct();

  private:
    void *m_curl_handle;
    int32_t m_dl_percent;
    std::string m_url;
    std::string m_dst_file;
};

#endif /* ORYX_UPGRADE_INCLUDE_AM_DOWNLOAD_H_ */
