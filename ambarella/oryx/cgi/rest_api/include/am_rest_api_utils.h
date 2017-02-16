/**
 * am_rest_api_base.h
 *
 *  History:
 *		2015年8月19日 - [Huaiqing Wang] created file
 *
 * Copyright (C) 2007-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef ORYX_CGI_INCLUDE_AM_REST_API_UTILS_H_
#define ORYX_CGI_INCLUDE_AM_REST_API_UTILS_H_

#include <sys/types.h>
#include <string.h>
#include <map>
#include <atomic>
#include "am_pointer.h"
#include "json.h"

#define MAX_BUF_LENGHT      512
#define MAX_PARA_LENGHT     32
#define MAX_ENV_VAR_LENGHT  64
#define MAX_MSG_LENGHT      128

//response msg code for client
enum AM_REST_MSG_CODE
{
  AM_REST_OK                 = 0,   //cgi program run success
  AM_REST_SERVER_INIT_ERR,          //server initialize failed
  AM_REST_MEDIA_TYPE_ERR,           //media type is not support
  AM_REST_METHOD_ERR,               //method type is not support
  AM_REST_PARAM_LENGTH_ERR,         //content lenght is too long to handle
  AM_REST_PARAM_JSON_ERR,           //invalid json data
  AM_REST_URL_ARG0_ERR,             //invalid arg 0 in url(service type)
  AM_REST_URL_ARG1_ERR,             //invalid arg 1 in url
  AM_REST_URL_ARG2_ERR,             //invalid arg 2 in url
  AM_REST_URL_ARG3_ERR,             //invalid arg 3 in url
  AM_REST_URL_ARG4_ERR,             //invalid arg 4 in url
  AM_REST_URL_ARG5_ERR,             //invalid arg 5 in url
  AM_REST_URL_ARG6_ERR,             //invalid arg 6 in url
  AM_REST_URL_ARG7_ERR,             //invalid arg 7 in url

  //video msg code
  AM_REST_VIDEO_OK           = 10000,
  //video overlay
  AM_REST_OVERLAY_OK         = 10100,
  AM_REST_OVERLAY_HANDLE_ERR,       //server overlay handle failed
  AM_REST_OVERLAY_ACTION_ERR,       //invalid overlay action type
  AM_REST_OVERLAY_STREAM_ERR,       //invalid overlay stream id
  AM_REST_OVERLAY_AREA_ERR,         //invalid overlay area id
  AM_REST_OVERLAY_TYPE_ERR,         //invalid overlay type
  AM_REST_OVERLAY_LAYOUT_ERR,       //invalid overlay layout type
  AM_REST_OVERLAY_W_H_ERR,          //invalid overlay width or height
  AM_REST_OVERLAY_FONT_ERR,          //invalid overlay font
  AM_REST_OVERLAY_STR_ERR,          //invalid overlay string
  AM_REST_OVERLAY_PIC_ERR,          //invalid overlay picture
  //video dptz
  AM_REST_DPTZ_OK      = 10200,
  AM_REST_DPTZ_HANDLE_ERR,       //server dptz handle failed
  AM_REST_DPTZ_ACTION_ERR,       //invalid dptz action type
  AM_REST_DPTZ_BUFFER_ERR,       //invalid dptz buffer id
  AM_REST_DPTZ_ZOOM_RATIO_ERR,   //invalid dptz zoom_ratio
  AM_REST_DPTZ_PAN_RATIO_ERR,    //invalid dptz pan_ratio
  AM_REST_DPTZ_TILT_RATIO_ERR,    //invalid dptz tilt_ratio
  //video dewarp
  AM_REST_DEWARP_OK      = 10300,
  AM_REST_DEWARP_HANDLE_ERR,       //server dewarp handle failed
  AM_REST_DEWARP_ACTION_ERR,       //invalid dewarp action type
  AM_REST_DEWARP_LDC_STRENGTH_ERR, //invalid dewarp ldc_strenght parameter
  //video encode control
  AM_REST_ENC_CTRL_OK   = 10400,
  AM_REST_ENC_CTRL_HANDLE_ERR,     //server control encode failed
  AM_REST_ENC_CTRL_OPTION_ERR,     //invalid enc_ctrl option
  AM_REST_ENC_CTRL_ACTION_ERR,       //invalid enc_ctrl action type
  AM_REST_ENC_CTRL_STREAM_ERR,       //invalid stream id

  //audio msg code
  AM_REST_AUDIO_OK           = 20000,

  //media msg code
  AM_REST_MEDIA_OK           = 30000,
  //recording
  AM_REST_RECORDING_OK       = 30100,
  AM_REST_RECORDING_HANDLE_ERR,       //server recording handle failed
  AM_REST_RECORDING_ACTION_ERR,       //invalid recording action type
  AM_REST_RECORDING_TYPE_ERR,       //invalid recording type
  //playback
  AM_REST_PLAYBACK_OK        = 30200,
  AM_REST_PLAYBACK_HANDLE_ERR,       //server playback handle failed
  AM_REST_PLAYBACK_ACTION_ERR,       //invalid playback action type
  AM_REST_PLAYBACK_TYPE_ERR,       //invalid playback type
  AM_REST_PLAYBACK_FILE_ERR,       //invalid playback file

  //image msg code
  AM_REST_IMAGE_OK           = 40000,

  //event msg code
  AM_REST_EVENT_OK           = 50000,

  //system msg code
  AM_REST_SYSTEM_OK          = 60000,

  //sip msg code
  AM_REST_SIP_OK             = 70000,
};

enum AM_REST_RESULT
{
  AM_REST_RESULT_OK              = 0,      //OK is same as NO_ERROR
  AM_REST_RESULT_ERR_PARAM       = -1,     //invalid argument
  AM_REST_RESULT_ERR_URL         = -2,     //resource is not exist
  AM_REST_RESULT_ERR_METHOD      = -3,     //method is not support
  AM_REST_RESULT_ERR_MEDIA_TYPE  = -4,     //unsupport media type, current just support json
  AM_REST_RESULT_ERR_SERVER      = -5,     //all errors return by services
};

struct AMRestEnvVar
{
    char *query_str;
    char *method;
    char *content_type;
    char *content_len;
};

struct AMRestMsg
{
    int32_t   status_code;  //represent http status code
    AM_REST_MSG_CODE  msg_code; //represent particular status code
    std::string   msg;  //description for msg code
    json_object  *data; //specify data client wants
    json_object  *array_data; //specify data client wants
};

class AMRestAPIUtils;
typedef AMPointer<AMRestAPIUtils> AMRestAPIUtilsPtr;
class AMRestAPIUtils
{
    friend AMRestAPIUtilsPtr;

  public:
    //get a reference of the AMRestAPIUtils object.
    static AMRestAPIUtilsPtr get_instance();

    //parse GET or POST method parameters from client
    AM_REST_RESULT  cgi_parse_client_params();

    //return data as response to client
    void  cgi_response_client(AM_REST_RESULT status);

    //get needed value with specify key
    AM_REST_RESULT  get_value(const std::string &key, std::string &value);

    //set response msg info
    void  set_response_msg(const AM_REST_MSG_CODE &msg_code, const std::string &msg);

    //set response data section
    void  set_response_data(const std::string &key, const int &value);
    void  set_response_data(const std::string &key, const double &value);
    void  set_response_data(const std::string &key, const bool &value);
    void  set_response_data(const std::string &key, const std::string &value);
    void  set_response_data(const std::string &array_value);

  private:
    AMRestAPIUtils();
    ~AMRestAPIUtils();
    AM_REST_RESULT json_string_parse(const char *str);
    void json_string_construct();
    void release();
    void inc_ref();
    static AMRestAPIUtils *m_instance;
    std::atomic_int m_ref_counter;
    AMRestMsg    m_response;
    AMRestEnvVar m_env;
    std::map<std::string, std::string> m_data;
};

#endif /* ORYX_CGI_INCLUDE_AM_REST_API_UTILS_H_ */
