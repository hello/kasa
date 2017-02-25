/**
 * am_rest_api_utils.cpp
 *
 *  History:
 *		2015/08/19 - [Huaiqing Wang] created file
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
 */
#include <stdio.h>
#include <stdlib.h>
#include "am_rest_api_utils.h"
#include <mutex>
#include <signal.h>

static std::mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::mutex> lck (m_mtx);
AMRestAPIUtils * AMRestAPIUtils::m_instance = nullptr;
AMRestAPIUtils::AMRestAPIUtils():
    m_obj_str("{}"),
    m_arr_str("[]")
{
  memset(&m_env, 0, sizeof(m_env));
  m_response.data = json_object_new_object();
  m_response.array_data = json_object_new_array();
  m_obj_data = json_object_new_object();
  m_arr_data = json_object_new_array();
}

AMRestAPIUtils::~AMRestAPIUtils()
{
  json_object_put(m_response.data);
  json_object_put(m_response.array_data);
  json_object_put(m_obj_data);
  json_object_put(m_arr_data);
  m_response.msg.clear();
  m_data.clear();
}

AMRestAPIUtilsPtr AMRestAPIUtils::get_instance()
{
  DECLARE_MUTEX;
  if (!m_instance) {
    m_instance = new AMRestAPIUtils();
  }
  return m_instance;
}

void AMRestAPIUtils::release()
{
  DECLARE_MUTEX;
  if((m_ref_counter) > 0 && (--m_ref_counter == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

void AMRestAPIUtils::inc_ref()
{
  ++ m_ref_counter;
}

AM_REST_RESULT AMRestAPIUtils::cgi_parse_client_params()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    char tmp_buf[MAX_BUF_LENGHT];
    m_env.query_str = getenv("QUERY_STRING");
    if (!m_env.query_str) {
      ret = AM_REST_RESULT_ERR_SERVER;
      set_response_msg(AM_REST_SERVER_INIT_ERR, "no QUERY_STRING env variable");
      break;
    }
    strcpy(tmp_buf, m_env.query_str);
    char *start = tmp_buf;
    char *saveptr = nullptr;
    char *token = nullptr;
    char tmp_key[MAX_PARA_LENGHT];
    char tmp_value[MAX_PARA_LENGHT];

    //store form query data from url
    while((token = strtok_r(start, "&", &saveptr)) != nullptr) {
      if (sscanf(token, "%[^=]=%s" ,tmp_key, tmp_value) == 2) {
        string key = tmp_key;
        string value = tmp_value;
        m_data.insert(map<string, string>::value_type(key, value));
      }
      start = nullptr;
    }

    m_env.method = getenv("REQUEST_METHOD");
    if (m_env.method) {
      if (!strcmp(m_env.method, "POST")) {
        m_env.content_len = getenv("CONTENT_LENGTH");
        int32_t len = atoi(getenv("CONTENT_LENGTH"));
        if (len >= MAX_BUF_LENGHT) {
          ret = AM_REST_RESULT_ERR_PARAM;
          set_response_msg(AM_REST_PARAM_LENGTH_ERR, "parameters is too long");
          break;
        } else if (len > 0) {
          m_env.content_type = getenv("CONTENT_TYPE");
          if (!m_env.content_type ||
              !strstr(m_env.content_type, "application/json")) {
            ret = AM_REST_RESULT_ERR_MEDIA_TYPE;
            char msg[MAX_MSG_LENGHT];
            snprintf(msg, MAX_MSG_LENGHT, "invalid media type %s, available "
                "media type:[application/json]", m_env.content_type);
            set_response_msg(AM_REST_MEDIA_TYPE_ERR, msg);
            break;
          }
          //store json data
          char json_data[MAX_BUF_LENGHT];
          fgets(json_data, len+1, stdin);
          if ((ret = json_string_parse(json_data)) != AM_REST_RESULT_OK) {
            break;
          }
        }
      } else if (strcmp(m_env.method, "GET")) {
        ret = AM_REST_RESULT_ERR_METHOD;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid http method:%s, "
            "available method:[GET,POST]", m_env.method);
        set_response_msg(AM_REST_METHOD_ERR, msg);
        break;
      }
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIUtils::json_string_parse(const char *str)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  string key, value;
  char tmp[MAX_PARA_LENGHT];
  do {
    if (!str) {
      ret = AM_REST_RESULT_ERR_PARAM;
      set_response_msg(AM_REST_PARAM_JSON_ERR, "no json string");
      break;
    }
    json_object *jobj = json_tokener_parse(str);
    if (!jobj) {
      ret = AM_REST_RESULT_ERR_PARAM;
      set_response_msg(AM_REST_PARAM_JSON_ERR, "invalid json string");
      break;
    }
    json_object_iterator it = json_object_iter_begin(jobj);
    json_object_iterator itend = json_object_iter_end(jobj);
    while (!json_object_iter_equal(&it, &itend)) {
      json_object *tmp_obj = nullptr;
      const char *tmp_key = json_object_iter_peek_name(&it);
      if (json_object_object_get_ex(jobj, tmp_key, &tmp_obj)) {
        key = tmp_key;
        json_type type = json_object_get_type(tmp_obj);
        switch (type) {
          case json_type_string:
            value = json_object_get_string(tmp_obj);
            m_data.insert(map<string, string>::value_type(key, value));
            break;
          case json_type_int:
            snprintf(tmp, MAX_PARA_LENGHT, "%d", json_object_get_int(tmp_obj));
            value = tmp;
            m_data.insert(map<string, string>::value_type(key, value));
            break;
          case json_type_double:
            snprintf(tmp, MAX_PARA_LENGHT, "%f",
                     json_object_get_double(tmp_obj));
            value = tmp;
            m_data.insert(map<string, string>::value_type(key, value));
            break;
          case json_type_boolean:
            snprintf(tmp, MAX_PARA_LENGHT, "%d",
                     json_object_get_boolean(tmp_obj));
            value = tmp;
            m_data.insert(map<string, string>::value_type(key, value));
            break;
          case json_type_array:
          {
            int32_t arr_len = json_object_array_length(tmp_obj);
            for (int i=0; i<arr_len; ++i) {
              json_object *arr_val = json_object_array_get_idx(tmp_obj, i);
              json_type arr_type = json_object_get_type(arr_val);
              switch (arr_type) {
                case json_type_string:
                  value = json_object_get_string(arr_val);
                  break;
                case json_type_int:
                  snprintf(tmp, MAX_PARA_LENGHT, "%d",
                           json_object_get_int(arr_val));
                  value = tmp;
                  break;
                case json_type_double:
                  snprintf(tmp, MAX_PARA_LENGHT, "%f",
                           json_object_get_double(arr_val));
                  value = tmp;
                  break;
                case json_type_boolean:
                  snprintf(tmp, MAX_PARA_LENGHT, "%d",
                           json_object_get_boolean(arr_val));
                  value = tmp;
                  break;
                default:
                  ret = AM_REST_RESULT_ERR_PARAM;
                  char msg[MAX_MSG_LENGHT];
                  snprintf(msg, MAX_MSG_LENGHT, "invalid type: %d, json array "
                      "value just support string/int/double/boolean type",
                      arr_type);
                  set_response_msg(AM_REST_PARAM_JSON_ERR, msg);
                  break;
              }
              if (ret != AM_REST_RESULT_OK) {
                break;
              }
              snprintf(tmp, MAX_PARA_LENGHT, "%s%d", tmp_key, i);
              key = tmp;
              m_data.insert(map<string, string>::value_type(key, value));
            }
            break;
          }
          default:
            ret = AM_REST_RESULT_ERR_PARAM;
            char msg[MAX_MSG_LENGHT];
            snprintf(msg, MAX_MSG_LENGHT, "unsupported json type %d, "
                "available type:[string,int,double,boolean,array]", type);
            set_response_msg(AM_REST_PARAM_JSON_ERR, msg);
            break;
        }
        if (ret != AM_REST_RESULT_OK) {
          break;
        }
      }
      json_object_iter_next(&it);
    }
    json_object_put(jobj);
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIUtils::get_value(const string &key, string &value)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  map<string, string>::iterator it = m_data.find(key);
  if( it != m_data.end()) {
    value = it->second;
  } else {
    ret = AM_REST_RESULT_ERR_PARAM;
  }
  return ret;
}

void AMRestAPIUtils::set_response_msg(const AM_REST_MSG_CODE &msg_code,
                                      const string &msg)
{
  m_response.msg_code = msg_code;
  if (msg.size() >= MAX_MSG_LENGHT) {
    char tmp_msg[MAX_MSG_LENGHT];
    snprintf(tmp_msg, MAX_MSG_LENGHT, "%s", msg.c_str());
    m_response.msg = tmp_msg;
  } else {
    m_response.msg = msg;
  }
}

void AMRestAPIUtils::set_response_data(const string &key, const int32_t &value)
{
  json_object_object_add(m_response.data, key.c_str(),
                         json_object_new_int(value));
}

void AMRestAPIUtils::set_response_data(const string &key, const int64_t &value)
{
  json_object_object_add(m_response.data, key.c_str(),
                         json_object_new_int64(value));
}

void AMRestAPIUtils::set_response_data(const string &key, const double &value)
{
  json_object_object_add(m_response.data, key.c_str(),
                         json_object_new_double(value));
}

void AMRestAPIUtils::set_response_data(const string &key, const bool &value)
{
  json_object_object_add(m_response.data, key.c_str(),
                         json_object_new_boolean(value));
}

void AMRestAPIUtils::set_response_data(const string &key, const string &value)
{
  char tmp[MAX_MSG_LENGHT];
  if (value.size()>=MAX_MSG_LENGHT) {
    snprintf(tmp,MAX_MSG_LENGHT,"%s",value.c_str());
    json_object_object_add(m_response.data, key.c_str(),
                           json_object_new_string(tmp));
  } else {
    json_object_object_add(m_response.data, key.c_str(),
                           json_object_new_string(value.c_str()));
  }
}

void AMRestAPIUtils::set_response_data(const int32_t &value)
{
  json_object_array_add(m_response.array_data,  json_object_new_int(value));
}

void AMRestAPIUtils::set_response_data(const int64_t &value)
{
  json_object_array_add(m_response.array_data, json_object_new_int64(value));
}

void AMRestAPIUtils::set_response_data(const double &value)
{
  json_object_array_add(m_response.array_data, json_object_new_double(value));
}

void AMRestAPIUtils::set_response_data(const bool &value)
{
  json_object_array_add(m_response.array_data, json_object_new_boolean(value));
}

void AMRestAPIUtils::set_response_data(const string &value)
{
  char tmp[MAX_MSG_LENGHT];
  if (value.size()>=MAX_MSG_LENGHT) {
    snprintf(tmp,MAX_MSG_LENGHT,"%s", value.c_str());
    json_object_array_add(m_response.array_data, json_object_new_string(tmp));
  } else {
    json_object_array_add(m_response.array_data,
                          json_object_new_string(value.c_str()));
  }
}

void  AMRestAPIUtils::create_tmp_obj_data(const string &key,
                                          const int32_t &value)
{
  json_object_object_add(m_obj_data, key.c_str(), json_object_new_int(value));
}

void  AMRestAPIUtils::create_tmp_obj_data(const string &key,
                                          const int64_t &value)
{
  json_object_object_add(m_obj_data, key.c_str(), json_object_new_int64(value));
}

void  AMRestAPIUtils::create_tmp_obj_data(const string &key,
                                          const double &value)
{
  json_object_object_add(m_obj_data, key.c_str(),
                         json_object_new_double(value));
}

void  AMRestAPIUtils::create_tmp_obj_data(const string &key, const bool &value)
{
  json_object_object_add(m_obj_data, key.c_str(),
                         json_object_new_boolean(value));
}

void  AMRestAPIUtils::create_tmp_obj_data(const string &key,
                                          const string &value)
{
  char tmp[MAX_MSG_LENGHT];
  if (value.size()>=MAX_MSG_LENGHT) {
    snprintf(tmp,MAX_MSG_LENGHT,"%s",value.c_str());
    json_object_object_add(m_obj_data, key.c_str(),
                           json_object_new_string(tmp));
  } else {
    json_object_object_add(m_obj_data, key.c_str(),
                           json_object_new_string(value.c_str()));
  }
}

void  AMRestAPIUtils::create_tmp_arr_data(const int32_t &value)
{
  json_object_array_add(m_arr_data, json_object_new_int(value));
}

void  AMRestAPIUtils::create_tmp_arr_data(const int64_t &value)
{
  json_object_array_add(m_arr_data, json_object_new_int64(value));
}

void  AMRestAPIUtils::create_tmp_arr_data(const double &value)
{
  json_object_array_add(m_arr_data, json_object_new_double(value));
}

void  AMRestAPIUtils::create_tmp_arr_data(const bool &value)
{
  json_object_array_add(m_arr_data, json_object_new_boolean(value));
}

void  AMRestAPIUtils::create_tmp_arr_data(const string &value)
{
  char tmp[MAX_MSG_LENGHT];
  if (value.size()>=MAX_MSG_LENGHT) {
    snprintf(tmp,MAX_MSG_LENGHT,"%s",value.c_str());
    json_object_array_add(m_arr_data, json_object_new_string(tmp));
  } else {
    json_object_array_add(m_arr_data, json_object_new_string(value.c_str()));
  }
}

void  AMRestAPIUtils::move_tmp_obj_to_tmp_arr()
{
  json_object_array_add(m_arr_data, m_obj_data);
  string str = json_object_to_json_string(m_arr_data);

  json_object_put(m_arr_data);
  m_arr_data  = json_tokener_parse(str.c_str());
  m_obj_data = json_object_new_object();
}

void  AMRestAPIUtils::move_tmp_arr_to_tmp_obj(const string &key)
{
  json_object_object_add(m_obj_data, key.c_str(), m_arr_data);
  string str = json_object_to_json_string(m_obj_data);

  json_object_put(m_obj_data);
  m_obj_data  = json_tokener_parse(str.c_str());
  m_arr_data = json_object_new_array();
}

void  AMRestAPIUtils::save_tmp_obj_to_local_obj(const string &key)
{
  json_object *obj  = json_tokener_parse(m_obj_str.c_str());
  if (!obj) {
    return;
  }
  json_object_object_add(obj, key.c_str(), m_obj_data);

  m_obj_str = json_object_to_json_string(obj);
  json_object_put(obj);
  //recreate tmp data object because it was deleted by up action
  m_obj_data = json_object_new_object();
}

void  AMRestAPIUtils::save_tmp_obj_to_local_arr()
{
  json_object *arr  = json_tokener_parse(m_arr_str.c_str());
  if (!arr) {
    return;
  }
  json_object_array_add(arr, m_obj_data);

  m_arr_str = json_object_to_json_string(arr);
  json_object_put(arr);
  //recreate tmp data object because it was deleted by up action
  m_obj_data = json_object_new_object();
}

void  AMRestAPIUtils::save_tmp_arr_to_local_obj(const string &key)
{
  json_object *obj  = json_tokener_parse(m_obj_str.c_str());
  if (!obj) {
    return;
  }
  json_object_object_add(obj, key.c_str(), m_arr_data);

  m_obj_str = json_object_to_json_string(obj);
  json_object_put(obj);
  //recreate tmp data object because it was deleted by up action
  m_arr_data = json_object_new_array();
}

void  AMRestAPIUtils::save_tmp_arr_to_local_arr()
{
  json_object *arr  = json_tokener_parse(m_arr_str.c_str());
  if (!arr) {
    return;
  }
  json_object_array_add(arr, m_arr_data);

  m_arr_str = json_object_to_json_string(arr);
  json_object_put(arr);
  //recreate tmp data object because it was deleted by up action
  m_arr_data = json_object_new_array();
}

void  AMRestAPIUtils::move_local_obj_to_local_arr()
{
  json_object *obj  = json_tokener_parse(m_obj_str.c_str());
  json_object *arr  = json_tokener_parse(m_arr_str.c_str());
  if (!obj || !arr) {
    return;
  }
  json_object_array_add(arr, obj);

  m_arr_str = json_object_to_json_string(arr);
  json_object_put(arr);
  //clean local object string
  m_obj_str = "{}";
}

void  AMRestAPIUtils::move_local_arr_to_local_obj(const string &key)
{
  json_object *obj  = json_tokener_parse(m_obj_str.c_str());
  json_object *arr  = json_tokener_parse(m_arr_str.c_str());
  if (!obj || !arr) {
    return;
  }
  json_object_object_add(obj, key.c_str(), arr);

  m_obj_str = json_object_to_json_string(obj);
  json_object_put(obj);
  //clean local array string
  m_arr_str = "[]";
}

void  AMRestAPIUtils::set_local_obj_to_response(const string &key)
{
  json_object *obj = json_tokener_parse(m_obj_str.c_str());
  if (!obj) {
    return;
  }
  json_object_object_add(m_response.data, key.c_str(), obj);
  m_obj_str = "{}";
}

void  AMRestAPIUtils::set_local_obj_to_response()
{
  json_object *obj = json_tokener_parse(m_obj_str.c_str());
  if (!obj) {
    return;
  }
  json_object_array_add(m_response.array_data, obj);
  m_obj_str = "{}";
}

void  AMRestAPIUtils::set_local_arr_to_response(const string &key)
{
  json_object *arr = json_tokener_parse(m_arr_str.c_str());
  if (!arr) {
    return;
  }
  json_object_object_add(m_response.data, key.c_str(), arr);
  m_arr_str = "[]";
}

void  AMRestAPIUtils::set_local_arr_to_response()
{
  json_object *arr = json_tokener_parse(m_arr_str.c_str());
  if (!arr) {
    return;
  }
  json_object_array_add(m_response.array_data, arr);
  m_arr_str = "[]";
}

void AMRestAPIUtils::json_string_construct()
{
  json_object *response = json_object_new_object();
  json_object_object_add(response, "status_code",
                         json_object_new_int(m_response.status_code));
  json_object_object_add(response, "msg_code",
                         json_object_new_int(m_response.msg_code));
  json_object_object_add(response, "msg",
                         json_object_new_string(m_response.msg.c_str()));

  if (json_object_object_length(m_response.data) > 0){
    json_object_object_add(response, "data", m_response.data);
  } else if (json_object_object_length(m_response.array_data) > 0) {
    json_object_object_add(response, "data", m_response.array_data);
  }
  string str = json_object_to_json_string(response);
  //response header, media type is application/json
  printf("Content-Type: application/json\n\n");
  //response body, data format is json string
  printf("%s\n", str.c_str());
  json_object_put(response);
}

void AMRestAPIUtils::cgi_response_client(AM_REST_RESULT status)
{
  if (AM_REST_RESULT_ERR_PARAM == status) {
    m_response.status_code = 400;
  } else if (AM_REST_RESULT_ERR_URL == status) {
    m_response.status_code = 404;
  } else if (AM_REST_RESULT_ERR_METHOD == status) {
    m_response.status_code = 405;
  } else if (AM_REST_RESULT_ERR_MEDIA_TYPE == status) {
    m_response.status_code = 415;
  } else if (AM_REST_RESULT_ERR_SERVER == status) {
    m_response.status_code = 500;
  } else if (AM_REST_RESULT_ERR_PERM == status){
    m_response.status_code = 503;
    set_response_msg(AM_REST_SERVER_UNAVAILABLE_ERR,
                     "apps_launcher is not launch");
  } else {
    m_response.status_code = 200;
    set_response_msg(AM_REST_OK, "ok");
  }
  json_string_construct();
}
