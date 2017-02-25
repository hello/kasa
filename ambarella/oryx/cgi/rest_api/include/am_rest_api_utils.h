/**
 * am_rest_api_base.h
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

using std::string;
using std::map;

//response msg code for client
enum AM_REST_MSG_CODE
{
  AM_REST_OK                 = 0,   //cgi program run success
  AM_REST_SERVER_UNAVAILABLE_ERR,   //server resource tmp unavailable
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
  AM_REST_DEWARP_WARP_MODE_ERR, //invalid dewarp ldc_mode parameter
  AM_REST_DEWARP_LDC_STRENGTH_ERR, //invalid dewarp ldc_strenght parameter
  AM_REST_DEWARP_MAX_RADIUS_ERR, //invalid dewarp max_radius parameter
  AM_REST_DEWARP_PANO_HFOV_ERR, //invalid dewarp pano_hfov parameter
  AM_REST_DEWARP_YAW_PITCH_HFOV_ERR, //invalid dewarp yaw or pitch parameter
  AM_REST_DEWARP_ZOOM_ERR, //invalid dewarp zoom parameter
  //video encode control
  AM_REST_ENC_CTRL_OK   = 10400,
  AM_REST_ENC_CTRL_HANDLE_ERR,     //server control encode failed
  AM_REST_ENC_CTRL_OPTION_ERR,     //invalid enc_ctrl option
  AM_REST_ENC_CTRL_ACTION_ERR,       //invalid enc_ctrl action type
  AM_REST_ENC_CTRL_STREAM_ERR,       //invalid stream id
  //video configure
  AM_REST_VIDEO_CFG_OK         = 10400,
  AM_REST_VIDEO_CFG_HANDLE_ERR,  //handle video configure file

  //audio msg code
  AM_REST_AUDIO_OK           = 20000,

  //media msg code
  AM_REST_MEDIA_OK           = 30000,
  //recording
  AM_REST_RECORDING_OK       = 30100,
  AM_REST_RECORDING_HANDLE_ERR,       //server recording handle failed
  AM_REST_RECORDING_ACTION_ERR,       //invalid recording action type
  AM_REST_RECORDING_TYPE_ERR,       //invalid recording type
  AM_REST_RECORDING_ATTR_ERR,       //invalid recording attribute type
  AM_REST_RECORDING_EVENT_ID_ERR,   //invalid recording event id type
  //playback
  AM_REST_PLAYBACK_OK        = 30200,
  AM_REST_PLAYBACK_HANDLE_ERR,       //server playback handle failed
  AM_REST_PLAYBACK_ACTION_ERR,       //invalid playback action type
  AM_REST_PLAYBACK_TYPE_ERR,       //invalid playback type
  AM_REST_PLAYBACK_FILE_ERR,       //invalid playback file

  //image msg code
  AM_REST_IMAGE_OK           = 40000,
  //image style
  AM_REST_STYLE_OK           = 40100,
  AM_REST_STYLE_HANDLE_ERR,           //server handle image style failed
  AM_REST_STYLE_ACTION_ERR,           //invalid image style action
  AM_REST_STYLE_TYPE_ERR,             //invalid image style type
  AM_REST_STYLE_HUE_ERR,              //invalid hue value
  AM_REST_STYLE_SATURATION_ERR,       //invalid saturation value
  AM_REST_STYLE_SHARPNESS_ERR,        //invalid sharpness value
  AM_REST_STYLE_BRIGHTNESS_ERR,       //invalid brightness value
  AM_REST_STYLE_CONTRAST_ERR,         //invalid contrast value
  //denoise
  AM_REST_DENOISE_OK           = 40200,
  AM_REST_DENOISE_HANDLE_ERR,         //server handle denoise failed
  AM_REST_DENOISE_ACTION_ERR,         //invalid image denoise action
  AM_REST_DENOISE_MCTF_STRENGTH_ERR,  //invalid image denoise mctf_strength value
  //ae
  AM_REST_AE_OK           = 40300,
  AM_REST_AE_HANDLE_ERR,              //server handle ae failed
  AM_REST_AE_ACTION_ERR,              //invalid image ae action
  AM_REST_AE_METERING_MODE_ERR,       //invalid ae metering_mode value
  AM_REST_AE_DAY_NIGHT_MODE_ERR,      //invalid ae day_night_mode value
  AM_REST_AE_SLOW_SHUTTER_MODE_ERR,   //invalid ae slow_shutter_mode value
  AM_REST_AE_ANTI_FLICKER_MODE_ERR,   //invalid ae anti_flicker_mode value
  AM_REST_AE_TARGET_RATIO_ERR,        //invalid ae ae_target_ratio value
  AM_REST_AE_BACK_LIGHT_COMP_ERR,     //invalid ae back_light_comp value
  AM_REST_AE_LOCAL_EXPOSURE_ERR,      //invalid ae local_exposure value
  AM_REST_AE_DC_IRIS_ENABLE_ERR,      //invalid ae dc_iris_enable value
  AM_REST_AE_IR_LED_MODE_ERR,         //invalid ae ir_led_mode value
  AM_REST_AE_SENSOR_GAIN_MAX_ERR,     //invalid ae sensor_gain_max value
  AM_REST_AE_SENSOR_SHUTTER_MIN_ERR,  //invalid ae sensor_shutter_min value
  AM_REST_AE_SENSOR_SHUTTER_MAX_ERR,  //invalid ae sensor_shutter_max value
  AM_REST_AE_ENABLE_ERR,              //invalid ae ae_enable value
  AM_REST_AE_SENSOR_GAIN_ERR,         //invalid ae sensor_gain_manual value
  AM_REST_AE_SENSOR_SHUTTER_ERR,      //invalid ae sensor_shutter_manual value
  //awb
  AM_REST_AWB_OK           = 40400,
  AM_REST_AWB_HANDLE_ERR,             //server handle awb failed
  AM_REST_AWB_ACTION_ERR,             //invalid image awb action
  AM_REST_AWB_MODE_ERR,               //invalid image awb mode value
  //af
  AM_REST_AF_OK           = 40500,
  AM_REST_AF_HANDLE_ERR,              //server handle af failed
  AM_REST_AF_ACTION_ERR,              //invalid image af action
  AM_REST_AF_MODE_ERR,                //invalid image af mode

  //event msg code
  AM_REST_EVENT_OK           = 50000,


  //system msg code
  AM_REST_SYSTEM_OK          = 60000,
  //sdcard
  AM_REST_SD_OK       = 60100,
  AM_REST_SD_HANDLE_ERR,       //server handle sdcard failed
  AM_REST_SD_ACTION_ERR,       //invalid sdcard action type
  AM_REST_SD_OP_TYPE_ERR,      //invalid sdcard operate type
  AM_REST_SD_INFO_TYPE_ERR,    //invalid sdcard info operate type
  AM_REST_SD_MODE_OPTION_ERR,  //invalid sdcard mode option

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
  AM_REST_RESULT_ERR_PERM        = -6,     //apps_launcher is not launch,
                                           //operation is not permitted
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
    json_object  *data; //specify data client wants
    json_object  *array_data; //specify data client wants
    int32_t   status_code;  //represent http status code
    AM_REST_MSG_CODE  msg_code; //represent particular status code
    string   msg;  //description for msg code
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
    AM_REST_RESULT  get_value(const string &key, string &value);

    //set response msg info
    void  set_response_msg(const AM_REST_MSG_CODE &msg_code, const string &msg);

    //set response data section, value is json object
    void  set_response_data(const string &key, const int32_t &value);
    void  set_response_data(const string &key, const int64_t &value);
    void  set_response_data(const string &key, const double &value);
    void  set_response_data(const string &key, const bool &value);
    void  set_response_data(const string &key, const string &value);

    //set response data section, value is json array
    void  set_response_data(const int32_t &value);
    void  set_response_data(const int64_t &value);
    void  set_response_data(const double &value);
    void  set_response_data(const bool &value);
    void  set_response_data(const string &value);

    //below apis used for json object and json array nest use as response
    /* add data to a tmp json object when need
      put json object to other json object/array*/
    void  create_tmp_obj_data(const string &key, const int32_t &value);
    void  create_tmp_obj_data(const string &key, const int64_t &value);
    void  create_tmp_obj_data(const string &key, const double &value);
    void  create_tmp_obj_data(const string &key, const bool &value);
    void  create_tmp_obj_data(const string &key, const string &value);

    /* add data to a tmp json array when need
      put json array to other json object/array*/
    void  create_tmp_arr_data(const int32_t &value);
    void  create_tmp_arr_data(const int64_t &value);
    void  create_tmp_arr_data(const double &value);
    void  create_tmp_arr_data(const bool &value);
    void  create_tmp_arr_data(const string &value);

    //move tmp json object/array to local json array/object
    void  move_tmp_obj_to_tmp_arr();
    void  move_tmp_arr_to_tmp_obj(const string &key);

    //move tmp json object/array data to a local json object/array
    void  save_tmp_obj_to_local_obj(const string &key);
    void  save_tmp_obj_to_local_arr();
    void  save_tmp_arr_to_local_obj(const string &key);
    void  save_tmp_arr_to_local_arr();

    //move local json object/array to local json array/object
    void  move_local_obj_to_local_arr();
    void  move_local_arr_to_local_obj(const string &key);

    //set local object/array string to response
    void  set_local_obj_to_response(const string &key);
    void  set_local_obj_to_response();
    void  set_local_arr_to_response(const string &key);
    void  set_local_arr_to_response();

  private:
    AMRestAPIUtils();
    ~AMRestAPIUtils();
    AM_REST_RESULT json_string_parse(const char *str);
    void json_string_construct();
    void release();
    void inc_ref();
    static AMRestAPIUtils *m_instance;
    json_object  *m_obj_data; //tmp data used for array_data
    json_object  *m_arr_data; //tmp data used for array_data
    std::atomic_int m_ref_counter;
    AMRestMsg    m_response;
    AMRestEnvVar m_env;
    map<string, string> m_data;
    string       m_obj_str;
    string       m_arr_str;
};

#endif /* ORYX_CGI_INCLUDE_AM_REST_API_UTILS_H_ */
