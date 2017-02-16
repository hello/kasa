/*******************************************************************************
 * am_download.h
 *
 * History:
 *   2015-1-7 - [longli] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
