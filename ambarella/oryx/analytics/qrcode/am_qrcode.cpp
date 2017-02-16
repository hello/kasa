/*******************************************************************************
 * am_qrcode.cpp
 *
 * History:
 *   2014/12/16 - [Long Li] created file
 *
 * Copyright (C) 2008-2016, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
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
  uint8_t *y_data_buf = NULL;
  uint8_t *y_start = NULL;
  uint8_t *y_buffer = NULL;
  uint32_t i = 0;
  int32_t ret_val = -1;
  clock_t start = 0;
  clock_t end = 0;
  uint32_t pass_time = 0;
  string file_tmp;
  AMIVideoReaderPtr reader = NULL;
  AMQueryDataFrameDesc frame_desc;
  AMMemMapInfo dsp_mem;


  do {
    if ((buf_index < AM_ENCODE_SOURCE_BUFFER_MAIN)
        || (buf_index > AM_ENCODE_SOURCE_BUFFER_4TH)) {
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
    reader = AMIVideoReader::get_instance();
    if (!reader) {
      ERROR("Unable to get AMVideoReader instance \n");
      ret = false;
      break;
    }
    result = reader->init();
    if (result != AM_RESULT_OK) {
      ERROR("AMVideoReader init fail\n");
      ret = false;
      break;
    }
    /* try to get dsp mem */
    result = reader->get_dsp_mem(&dsp_mem);
    if (result != AM_RESULT_OK) {
      ERROR("Get dsb mem failed \n");
      ret = false;
      break;
    }

    start = clock();
    do {
      result = reader->query_yuv_frame(&frame_desc,
                                       AM_ENCODE_SOURCE_BUFFER_ID(buf_index),
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
      if (!y_data_buf) {
        y_data_buf = new uint8_t[frame_desc.yuv.width * frame_desc.yuv.height];
      }
      if (y_data_buf) {
        y_buffer = y_data_buf;
        y_start = dsp_mem.addr + frame_desc.yuv.y_addr_offset;
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
    y_data_buf = NULL;
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
