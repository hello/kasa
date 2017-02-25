/*******************************************************************************
 * am_qrcode.cpp
 *
 * History:
 *   2014/12/16 - [Long Li] created file
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

#include <unistd.h>
#include <errno.h>
#include <zbar.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "am_log.h"
#include "am_define.h"
#include "am_qrcode.h"
#include "am_qrcode_if.h"
#include "am_video_reader_if.h"
#include "am_video_address_if.h"

using namespace std;
using namespace zbar;

AMQrcode::AMQrcode()
{
}

AMQrcode::~AMQrcode()
{
}

AMIQrcode *AMIQrcode::create()
{
  return AMQrcode::create();
}

AMQrcode *AMQrcode::create()
{
  return new AMQrcode();
}

void AMQrcode::destroy()
{
  delete this;
}

bool AMQrcode::set_config_path(const string &config_path)
{
  bool ret = true;
  do {
    if (config_path.empty()) {
      ERROR("config_path string is empty\n");
      ret = false;
      break;
    }

    if (!access(m_config_path.c_str(), F_OK) &&
        rename(m_config_path.c_str(), config_path.c_str())) {
      ret = false;
      NOTICE("Failed change file name %s to %s.",
             m_config_path.c_str(), config_path.c_str());
      break;
    }
    m_config_path = config_path;
  } while (0);
  return ret;
}

/* IAV state must be preview or encoding.
 * Parameter: timeout is in second.
 */
bool AMQrcode::qrcode_read(const int32_t buf_index, const uint32_t timeout)
{
  bool ret = true;
  AM_RESULT result = AM_RESULT_OK;
  ofstream out_file;
  uint8_t *y_data_buf = nullptr;
  uint8_t *y_start = nullptr;
  uint8_t *y_buffer = nullptr;
  uint32_t i = 0;
  int32_t ret_val = -1;
  clock_t start = 0;
  clock_t end = 0;
  uint32_t pass_time = 0;
  string file_tmp;
  AMIVideoReaderPtr reader = nullptr;
  AMIVideoAddressPtr address = nullptr;
  AMQueryFrameDesc frame_desc;
  AMAddress addr;

  do {
    if ((buf_index < AM_SOURCE_BUFFER_MAIN)
        || (buf_index > AM_SOURCE_BUFFER_4TH)) {
      ERROR("Selected encode source buffer index: %d is invalid\n", buf_index);
      ret = false;
      break;
    }
    if (m_config_path.empty()) {
      ERROR("config_path string is empty\n");
      ret = false;
      break;
    }

    file_tmp = (m_config_path + ".bak");
    out_file.open(file_tmp, ios::trunc);
    if (out_file.fail()) {
      ERROR("Failed to open %s: %s", m_config_path.c_str(), strerror(errno));
      ret = false;
      break;
    }
    if (!(reader = AMIVideoReader::get_instance())) {
      ERROR("Unable to get AMVideoReader instance!");
      ret = false;
      break;
    }

    if (!(address = AMIVideoAddress::get_instance())) {
      ERROR("Failed to get AMVideoAddress instance!");
      ret = false;
      break;
    }

    start = clock();
    do {
      result = reader->query_yuv_frame(frame_desc,
                                       AM_SOURCE_BUFFER_ID(buf_index),
                                       false);
      if (result == AM_RESULT_ERR_AGAIN) {
        /* DSP state not ready to query video,
         * small pause and query state again
         */
        usleep(100 * 1000);
        continue;
      } else if (result != AM_RESULT_OK) {
        ERROR("Query yuv frame failed \n");
        ret = false;
        break;
      }

      if (address->yuv_y_addr_get(frame_desc, addr) != AM_RESULT_OK) {
        ERROR("Failed to get y address!");
        ret = false;
        break;
      }

      if (!y_data_buf) {
        y_data_buf = new uint8_t[frame_desc.yuv.width * frame_desc.yuv.height];
      }
      if (y_data_buf) {
        y_buffer = y_data_buf;
        y_start = addr.data;
        for (i = 0; i < frame_desc.yuv.height; ++i) {
          memcpy(y_buffer, y_start, frame_desc.yuv.width);
          y_start += frame_desc.yuv.pitch;
          y_buffer += frame_desc.yuv.width;
        }

        ImageScanner zbar_scanner;
        Image image(frame_desc.yuv.width,
                    frame_desc.yuv.height,
                    "Y800",
                    y_data_buf,
                    frame_desc.yuv.width * frame_desc.yuv.height);
        if ((ret_val = zbar_scanner.scan(image)) > 0) {
          printf("QR code detected:\n");
          for (Image::SymbolIterator symbol = image.symbol_begin();
              symbol != image.symbol_end(); ++ symbol) {
            out_file << symbol->get_data();
            PRINTF("%s", symbol->get_data().c_str());
          }
        } else if (ret_val == 0) {
          NOTICE("No symbol found");
        } else {
          NOTICE("Zbar scan error!");
        }
      }
      end = clock();
      pass_time = (end -start) / CLOCKS_PER_SEC;
    } while ( (ret_val <= 0) && ( !timeout || (pass_time < timeout)));
    if ((ret_val <= 0) && timeout && (pass_time >= timeout)) {
      if(ret_val < 0) {
        ERROR("Failed to resolve symbol!");
      } else if (ret_val == 0) {
        NOTICE("No QR code detected and Time out after %d second(s).\n",
               timeout);
      }
      ret = false;
      break;
    }
    usleep(100 * 1000);
  } while (0);

  if (y_data_buf) {
    delete[] y_data_buf;
    y_data_buf = nullptr;
  }
  if (out_file.is_open()) {
    out_file<<"\n";
    out_file.close();
  }
  if (!ret) {
    /* delete wifi.conf when qr code fails to read wifi configuration */
    if (!file_tmp.empty() && (!access(file_tmp.c_str(), F_OK))
        && (remove(file_tmp.c_str()) < 0)) {
      NOTICE("Failed to remove %s: %s", file_tmp.c_str(),
             strerror (errno));
    }
  } else {
    if ((!access(m_config_path.c_str(), F_OK))
        && (remove(m_config_path.c_str()) < 0)) {
      ret = false;
      NOTICE("Failed to remove %s: %s", m_config_path.c_str(),
             strerror (errno));
    } else {
      if (rename(file_tmp.c_str(), m_config_path.c_str())) {
        ret = false;
        NOTICE("Failed to rename file %s to %s.",
               file_tmp.c_str(), m_config_path.c_str());
        if (remove(file_tmp.c_str())) {
          NOTICE("Failed to remove file %s.", file_tmp.c_str());
        }
      }
    }
  }

  return ret;
}

void AMQrcode::remove_duplicate(AMQrcodeConfig &config_list,
                                const string &config_name)
{
  AMQrcodeConfig::iterator l_it;
  if (!m_config.empty()) {
    for (l_it = m_config.begin(); l_it != m_config.end(); ++l_it) {
      if(!strcasecmp(l_it->first.c_str(), config_name.c_str())){
        m_config.erase(l_it);
        return;
      }
    }
  }
}

/* parse config file which qrcode_read() generates */
bool AMQrcode::qrcode_parse()
{
  bool ret = true;
  bool format_error = false;
  uint32_t len = 0;
  uint32_t s_pos = 0;
  uint32_t e_pos = 0;
  uint32_t pos = 0;
  ifstream in_file;
  string config_str;
  string config_name;
  string config_substr;
  string order_str;
  AMQrcodeCfgDetail config_detail;

  string::iterator it;

  if (!m_config.empty()) {
    m_config.clear();
  }
  do {
    if (m_config_path.empty()) {
      ret = false;
      break;
    }
    in_file.open(m_config_path, ios::in);
    if (in_file.fail()) {
      PRINTF("Failed to open file: %s, please call qrcode_read generates "
          "this file first!.", m_config_path.c_str());
      ret = false;
      break;
    }

    while (getline(in_file, config_str)) {
      if (config_str.empty()) {
        ERROR("Read file %s error, get nothing from this file",
              m_config_path.c_str());
        ret = false;
        break;
      }
      for (it = config_str.begin(); it != config_str.end(); ++it) {
        if (*it == '\\') {
          config_str.erase(it);
        }
      }
      if (config_str.rfind(";;") == string::npos) {
        ret = false;
        format_error = true;
      }
      while ((e_pos = config_str.find(";;", s_pos)) != string::npos) {
        len = e_pos + 2 - s_pos -1; //exclude one ';'
        config_substr = config_str.substr(s_pos, len);
        pos = config_substr.find(':');
        if (pos == string::npos) {
          ret = false;
          format_error = true;
          break;
        }
        config_name = config_substr.substr(0, pos);
        config_substr = config_substr.substr(pos + 1);

        if (!parse_keyword(config_detail, config_substr))
        {
          ret = false;
          format_error = true;
          break;
        }
        /* check whether have duplicated one, if have remove it */
        remove_duplicate(m_config, config_name);
        m_config.push_back(make_pair(config_name, config_detail));
        config_detail.clear();
        s_pos = e_pos + 2;
      }
      if (format_error) {
        printf("\nWrong format! qrcode_parse get string:\n");
        fflush(stdout);
        PRINTF( "    %s", config_str.c_str());
        printf("The string format is not compatible to Ambarella "
            "Qrcode spec for WiFi setup.\n");
        printf("Please follow the following format:\n");
        printf("    config_name:key_name1:key_value1;"
            "key_name2:key_value2;;config_name2...\n");
        printf("    eg. wifi:S:myAP;P:passwd;;\n\n");
        break;
      }
      s_pos = 0;
    }
  } while (0);

  if (in_file.is_open()) {
    in_file.close();
  }

  return ret;
}
/* Filt out the key word from the string which
 * qrcode reads from config file.
 */
bool AMQrcode::parse_keyword(AMQrcodeCfgDetail &config_detail,
                             string &entry)
{
  bool ret = true;
  uint32_t s = 0;
  uint32_t e = 0;
  uint32_t len = 0;
  uint32_t pos = 0;
  string key_name;
  string key_value;
  string substr;

  while ((e = entry.find(';', s)) != string::npos) {
    len = e - s;//exclude ';'
    substr = entry.substr(s, len);

    pos = substr.find(':');
    if (!pos || pos == string::npos) {
      ret = false;
      break;
    }
    key_name = substr.substr(0, pos);
    if (key_name.size() == (len - 1)) {
      key_value = "";
    } else {
      key_value = substr.substr(pos + 1);
    }
    config_detail.push_back(make_pair(key_name, key_value));
    s = e + 1;
  }

  return ret;
}

const string AMQrcode::get_key_value(const string &config_name,
                                     const string &key_name)
{
  bool config_name_exist = false;
  bool found = false;
  string ret("");
  AMQrcodeConfig::iterator l_it;
  AMQrcodeCfgDetail::iterator ll_it;

  do {
    if (config_name.empty()) {
      ERROR ("config_name is null!");
      break;
    }
    if (key_name.empty()) {
      ERROR ("key_name is null!");
      break;
    }

    if (m_config.empty()) {
      PRINTF("No QR code config has been parsed! "
          "Please call qrcode_parse() first!\n");
      break;
    }

    for (l_it = m_config.begin(); l_it != m_config.end(); ++l_it) {
      if(!strcasecmp(l_it->first.c_str(), config_name.c_str())) {
        for (ll_it = l_it->second.begin();
            ll_it != l_it->second.end();
            ++ll_it) {
          if (!strcasecmp(ll_it->first.c_str(), key_name.c_str())) {
            ret = ll_it->second;
            found = true;
            break;
          }
        }
        if (found) {
          break;
        } else {
          config_name_exist = true;
        }
      }
    }
    if (l_it == m_config.end()) {
      if (config_name_exist) {
        PRINTF("Not have '%s' item in config '%s'",
               key_name.c_str(), config_name.c_str());
      } else {
        PRINTF("Not have config '%s' item", config_name.c_str());
      }
    }

  } while (0);

  return ret;
}

static void print_septal_line()
{
  printf("------------------------\n");
}
bool AMQrcode::print_config_name_list()
{
  AMQrcodeConfig::iterator l_it;
  print_septal_line();
  printf("config name list:\n");
  if (m_config.empty()) {
    printf("\n");
    print_septal_line();
    return false;
  }
  for (l_it = m_config.begin(); l_it != m_config.end(); ++l_it) {
    printf("%s\n", l_it->first.c_str());
  }
  print_septal_line();
  return true;
}

bool AMQrcode::print_key_name_list(const string &config_name)
{
  AMQrcodeConfig::iterator l_it;
  AMQrcodeCfgDetail::iterator ll_it;

  print_septal_line();
  printf("%s's key_name list:\n", config_name.c_str());
  if (config_name.empty() || m_config.empty()) {
    printf("\n");
    print_septal_line();
    return false;
  }
  for (l_it = m_config.begin(); l_it != m_config.end(); ++l_it) {
    if(!strcasecmp(l_it->first.c_str(), config_name.c_str())) {
      for (ll_it = l_it->second.begin();
          ll_it != l_it->second.end();
          ++ll_it) {
        printf("%s\n", ll_it->first.c_str());
      }
      break;
    }
  }
  print_septal_line();
  if (l_it == m_config.end()) {
    return false;
  }
  return true;
}

void AMQrcode::print_qrcode_config()
{
  if (m_config.empty()) {
    PRINTF("No QR code config has been parsed! "
        "Please call qrcode_parse() first!\n");
    return;
  }
  AMQrcodeConfig::iterator l_it;
  AMQrcodeCfgDetail::iterator ll_it;
  for (l_it = m_config.begin(); l_it != m_config.end(); ++l_it) {
    print_septal_line();
    printf("config name: %s\n", l_it->first.c_str());
    for (ll_it = l_it->second.begin(); ll_it != l_it->second.end(); ++ll_it) {
      printf("%s: %s\n", ll_it->first.c_str(), ll_it->second.c_str());
    }
  }
  print_septal_line();
}
