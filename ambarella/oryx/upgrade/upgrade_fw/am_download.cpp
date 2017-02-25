/*******************************************************************************
 * am_download.cpp
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

#include "am_base_include.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <curl/curl.h>
#include "am_log.h"
#include "am_download.h"

using namespace std;
static char error_buf[CURL_ERROR_SIZE];

/* src_url: from where to download */
AMDownload *AMDownload::create()
{
  AMDownload *dl_ptr = nullptr;

  INFO("AMDownload::create() \n");
  dl_ptr = new AMDownload();
  if (dl_ptr && !dl_ptr->construct()) {
    delete dl_ptr;
    dl_ptr = nullptr;
    ERROR("AMDownload::Failed to create an instance of AMDownload\n");
  }

  return dl_ptr;
}

AMDownload::AMDownload():
    m_curl_handle(nullptr),
    m_dl_percent(0),
    m_url(""),
    m_dst_file("")
{
}

AMDownload::~AMDownload()
{
  curl_easy_cleanup(m_curl_handle);
  m_curl_handle = nullptr;
  curl_global_cleanup();
}

static int32_t progress_function(void *clientp,
                                 double dltotal,
                                 double dlnow,
                                 double ultotal,
                                 double ulnow)
{
  int32_t percent;
  if (dltotal < 1) {
    percent = 0;
    dlnow = 0;
  } else {
    percent = (int32_t)((dlnow / dltotal) * 100);
  }
  *(int32_t *)clientp = percent;
  printf("\r%d%%\t|", percent);
  int32_t i = 0;
  for (; i < (percent / 2); ++i) {
    putchar('*');
  }

  for (int32_t j = 0; j < 50-i; ++j) {
    putchar(' ');
  }

  if (dlnow < 1024) {
    printf("| %dB done.", (int32_t)dlnow);
  } else {
    printf("| %dkB done.", (int32_t)(dlnow/1024));
  }

  return 0;
}

static size_t write_data(void *buffer,
                         size_t size,
                         size_t nmemb,
                         void *userp)
{
  size_t written = fwrite(buffer, size, nmemb, (FILE *)userp);
  return written;
}

bool AMDownload::set_url(const string &src_file, const string &dst_path)
{
  bool ret = true;
  struct stat buf;
  string tmp_str(dst_path);

  do {
    if (src_file.empty()) {
      ERROR("Download url is empty.");
      ret = false;
      break;
    }

    if (!dst_path.empty()) {
      if (stat(dst_path.c_str(), &buf)) {
        if (errno != ENOENT) {
          NOTICE("stat error");
          ret = false;
          break;
        }
      }
      if (S_ISDIR(buf.st_mode)) {
        if (dst_path.find_last_of('/') != (dst_path.size() - 1)) {
          tmp_str += "/";
        }
      }
    }

    /* set dst_file according to src_file */
    if (dst_path.empty() || S_ISDIR(buf.st_mode)) {
      uint32_t pos = src_file.find_last_of('/');
      if (pos != string::npos) {
        if (pos != (src_file.size() - 1)) {
          tmp_str += src_file.substr(pos + 1);
        } else {
          pos = src_file.find_last_not_of('/');
          if (pos != string::npos) {
            uint32_t s_pos = src_file.find_last_of('/', pos);
            if (s_pos != string::npos) {
              tmp_str += src_file.substr(s_pos + 1, pos - s_pos);
            } else {
              tmp_str += src_file.substr(0, pos + 1);
            }
          } else {
            ERROR("Invalid download url.");
            ret = false;
            break;
          }
        }
      } else {
        tmp_str += src_file;
      }
    }

    if (stat(tmp_str.c_str(), &buf)) {
      if (errno != ENOENT) {
        NOTICE("stat error");
        ret = false;
        break;
      }
    } else {
      if (S_ISDIR(buf.st_mode)) {
        NOTICE("%s is a directory and cannot be a download dst_file",
               tmp_str.c_str());
        ret = false;
        break;
      }
    }

    if (CURLE_OK != curl_easy_setopt(m_curl_handle,
                                     CURLOPT_URL, src_file.c_str())) {
      NOTICE("curl easy set url failed.\n");
      ret = false;
      break;
    }

    m_url = src_file;
    m_dst_file = tmp_str;
    m_dl_percent = 0;
  }while (0);

  return ret;
}

int32_t AMDownload::get_dl_file_size()
{
  int32_t  file_size = -1;
  CURL *handle_ptr = nullptr;

  do {
    if (m_url.empty()) {
      ERROR("url is empty.");
      break;
    }

    handle_ptr = curl_easy_init();
    if (!handle_ptr) {
      ERROR("curl_easy_init error");
      break;
    }
    curl_easy_setopt(handle_ptr, CURLOPT_ERRORBUFFER, error_buf);
    curl_easy_setopt(handle_ptr, CURLOPT_URL, m_url.c_str());
    curl_easy_setopt(handle_ptr, CURLOPT_HEADER, 1L);
    curl_easy_setopt(handle_ptr, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(handle_ptr, CURLOPT_CONNECTTIMEOUT, 5L);
    if (curl_easy_perform(handle_ptr) != CURLE_OK) {
      PRINTF("%s\n", error_buf);
      break;
    }

    curl_easy_getinfo(handle_ptr, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &file_size);
  } while (0);

  if (handle_ptr) {
    curl_easy_cleanup(handle_ptr);
    handle_ptr = nullptr;
  }

  return file_size;
}

bool AMDownload::set_dl_user_passwd(const string &user_name,
                                    const string &passwd)
{
  bool ret = true;
  CURLcode retv;
  string login_str;

  login_str = user_name + ":" + passwd;
  retv = curl_easy_setopt(m_curl_handle, CURLOPT_USERPWD, login_str.c_str());
  if (retv != CURLE_OK) {
    NOTICE("curl easy set user:passwd failed, ret=%d\n", retv);
    ret = false;
  }

  return ret;
}

bool AMDownload::set_dl_connect_timeout(const uint32_t connect_timeout)
{
  bool ret = true;
  CURLcode retv;

  retv = curl_easy_setopt(m_curl_handle, CURLOPT_CONNECTTIMEOUT,
                          connect_timeout);
  if (retv != CURLE_OK) {
    NOTICE("curl easy set connect_timeout failed, ret=%d\n", retv);
    ret = false;
  }

  return ret;
}

/* If the download receives less than "low_speed" bytes/second
 * during "timeout" seconds, the operations is aborted.*/
bool AMDownload::set_low_speed_limit(uint32_t low_speed,
                                     uint32_t lowspeed_timeout)
{
  bool ret = true;
  CURLcode retv;

  do {
    retv = curl_easy_setopt(m_curl_handle, CURLOPT_LOW_SPEED_TIME,
                            lowspeed_timeout);
    if (retv != CURLE_OK) {
      NOTICE("curl easy set low speed timeout failed, ret=%d\n", retv);
      ret = false;
      break;
    }

    retv = curl_easy_setopt(m_curl_handle, CURLOPT_LOW_SPEED_LIMIT, low_speed);
    if (retv != CURLE_OK) {
      NOTICE("curl easy set low speed limit failed, ret=%d\n", retv);
      ret = false;
      break;
    }
  } while (0);

  return ret;
}

bool AMDownload::set_show_progress(bool show)
{
  bool ret = true;
  CURLcode retv;

  do {
    if (show) {
      retv = curl_easy_setopt(m_curl_handle, CURLOPT_NOPROGRESS, 0L);
      if (retv != CURLE_OK) {
        NOTICE("curl easy set NOPROGRESS failed, ret=%d\n", retv);
        ret = false;
        break;
      }

      retv = curl_easy_setopt(m_curl_handle,
                              CURLOPT_PROGRESSFUNCTION,
                              progress_function);
      if (retv != CURLE_OK) {
        NOTICE("curl easy set progress function failed, ret=%d\n", retv);
        ret = false;
        break;
      }

      retv = curl_easy_setopt(m_curl_handle,
                              CURLOPT_PROGRESSDATA,
                              &m_dl_percent);
      if (retv != CURLE_OK) {
        NOTICE("curl easy set progress function failed, ret=%d\n", retv);
        ret = false;
        break;
      }
    } else {
      retv = curl_easy_setopt(m_curl_handle, CURLOPT_NOPROGRESS, 1L);
      if (retv != CURLE_OK) {
        NOTICE("curl easy set NOPROGRESS failed, ret=%d\n", retv);
        ret = false;
        break;
      }
    }
  } while (0);

  return ret;
}

bool AMDownload::download()
{
  bool ret = true;
  CURLcode retv;
  FILE *fp = nullptr;

  do {
    fp = fopen(m_dst_file.c_str(), "wb");
    if (fp == nullptr) {
      NOTICE("%s open filed!\n", m_dst_file.c_str());
      ret = false;
      break;
    }

    retv = curl_easy_setopt(m_curl_handle, CURLOPT_WRITEDATA, fp);
    if (retv != CURLE_OK) {
      NOTICE("curl easy set file failed, ret=%d\n", retv);
      ret = false;
      break;
    }

    retv = curl_easy_perform(m_curl_handle);
    putchar('\n');
    if (retv != CURLE_OK) {
      PRINTF("%s\n", error_buf);
      m_dl_percent = 0;
      ret = false;
      break;
    }
  } while (0);

  if (fp) {
    fclose(fp);
  }

  return ret;
}

void AMDownload::destroy()
{
  delete this;
}

bool AMDownload::construct()
{
  bool ret = true;
  CURLcode retv;

  do {
    retv = curl_global_init(CURL_GLOBAL_ALL);
    if (retv != CURLE_OK) {
      NOTICE("curl global init failed!\n");
      ret = false;
      break;
    }

    m_curl_handle = curl_easy_init();
    if (m_curl_handle == nullptr) {
      NOTICE("curl easy init failed!\n");
      ret = false;
      break;
    }

    retv = curl_easy_setopt(m_curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    if (retv != CURLE_OK) {
      NOTICE("curl easy set write function failed, ret=%d\n", retv);
      ret = false;
      break;
    }

    retv = curl_easy_setopt(m_curl_handle, CURLOPT_ERRORBUFFER, error_buf);
    if (retv != CURLE_OK) {
      NOTICE("curl easy set error buffer failed, ret=%d\n", retv);
      ret = false;
      break;
    }
  } while (0);

  return ret;
}
