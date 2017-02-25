/*******************************************************************************
 * oryx.js
 *
 * History:
 *   2015/9/21 - [Huaiqing Wang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
//0:studying; 1:using
var g_user_state = 1;
function switch_service_option(obj_id) {
  if ( obj_id === "getting_started") {
    $("#oryx_using").hide();
    $("#getting_started").show();
    g_user_state = 0;
  } else {
    $("#getting_started").hide();
    $("#oryx_using").show();
    $("#" + obj_id).show().siblings().hide();
    if (g_user_state === 0) {
      vlc_play_start(gVlc_current_video_id,
        gVlc_current_audio_type, gVlc_current_delay);
      g_user_state = 1;
    }
  }
}

function construct_user_data(data) {
  var txt = " ";
  var key;
  for (key in data) {
    txt = txt + key + ": " + data[key] + "</br>";
  }
  $("#success_dialog").html(txt).dialog("option", "title", "Ok");
  $("#success_dialog").html(txt).dialog("open");
}

function get_parameter(obj) {
  var id_name = $(obj).attr("id");
  if (id_name === "AF") {
    get_AF_init_state();
  } else if (id_name === "AWB") {
    get_AWB_init_state();
  }
}

function set_parameter(obj) {
  var id_name = $(obj).attr("id");
  var value = $(obj).val();
  if (id_name === "AWB") {
    set_AWB_mode(value);
  } else if (id_name === "ae_metering_mode" || id_name === "ir_led_mode" || id_name === "day_night_mode" ||
      id_name === "anti_flicker_mode" || id_name === "back_light_comp" || id_name === "local_exposure" ||
      id_name === "dc_iris_enable" || id_name === "ae_enable" || id_name === "ae_target_ratio" ||
      id_name === "slow_shutter_mode" || id_name === "shutter_time_manual" || id_name === "sensor_shutter_min" ||
      id_name === "sensor_shutter_max" || id_name === "sensor_gain_max" || id_name === "sensor_gain_manual") {
    var name = $(obj).attr("name");
    set_AE_parameter(name, value);
  } else if (id_name === "enc_ctrl_stream_type" || id_name === "enc_ctrl_stream_resolution" ||
		  id_name === "enc_ctrl_stream_framerate" || id_name === "enc_ctrl_stream_bitrate") {
	  enc_ctrl_stream_control(obj);
  }
}

function select_to_set_parameter(obj) {
  var id_name = $(obj).attr("id");
  var value = $(obj).val();
  if (id_name === "overlay_manipulate_select") {
    if (value === "init") {
      $("#overlay_init").show();
    } else {
      $("#overlay_init").hide();
      overlay_manipulate(value);
    }
  } else if (id_name === "overlay_data_manipulate_select") {
    if (value === "set") {
      $("#overlay_data_rect").show();
      $("#overlay_data_content").show();
    } else if (value === "update"){
      $("#overlay_data_rect").hide();
      $("#overlay_data_content").show();
    } else if (value === "delete") {
      $("#overlay_data_rect").hide();
      $("#overlay_data_content").hide();
      manipulate_overlay_data(value);
    }
  }
}

var gEncode_state = 0;
var gStream_max = 4;
var gStream_state = new Array;
function set_encode_control_init_state() {
  $.get("/oryx/video/enc_ctrl/encode/?action=get", datatype = "json",
    function(result) {
      if (result.msg_code == 0) {
        var data = result.data;
        var key;
        gStream_max = data.length;
        for (key in data) {
          $("#enc_ctrl_stream_id").append("<option value=" + key + ">" + key + "</option>");
          if (data[key]) {
            $("#encode_state_control").button("option",
              "label", "Stop");
            gEncode_state = 1;
          }
          gStream_state[key] = data[key] ? 1 : 0;
        }
      }
  });
}

function encode_state_control() {
  vlc_play_stop();
  if (gEncode_state == 1) {
    $.get("/oryx/video/enc_ctrl/encode?action=disable", datatype = "json",
      function(result) {
        if (result.msg_code != 0) {
          $("#err_dialog").text(result.msg).dialog("open");
        } else {
          $("#encode_state_control").button("option", "label",  "Start");
          gEncode_state = 0;
        }
    });
  } else {
    $.get("/oryx/video/enc_ctrl/encode?action=enable", datatype = "json",
      function(result) {
        if (result.msg_code != 0) {
          $("#err_dialog").text(result.msg).dialog("open");
        } else {
          $("#encode_state_control").button("option", "label", "Stop");
          gEncode_state = 1;
          vlc_play_start(gVlc_current_video_id,
            gVlc_current_audio_type, gVlc_current_delay);
        }
    });
  }
}

function get_encode_status() {
  $.get("/oryx/video/enc_ctrl/encode?action=get", datatype = "json",
    function(result) {
      if (result.msg_code != 0) {
        $("#err_dialog").text(result.msg).dialog("open");
      } else {
        var data = new Array;
        for (key in result.data) {
          if (result.data[key]) {
            data["stream"+key] = "encoding";
          } else {
            data["stream"+key] = "not encoding";
          }
        }
        construct_user_data(data);
      }
  });
}

function stream_state_control(id) {
  val = $("#enc_ctrl_stream_state").val();
  if (gStream_state[id] != val) {
    $("#enc_ctrl_stream_state").button("option","disabled",true);
  } else {
    if (val == 1) {
      $("#enc_ctrl_stream_state").button("option", "label", "Disable");
      $("#enc_ctrl_stream_state").val(0);
    } else {
      $("#enc_ctrl_stream_state").button("option", "label", "Enable");
      $("#enc_ctrl_stream_state").val(1);
    }
  }

  $.get("/oryx/video/enc_ctrl/stream" + id + "?action=" +
      (val == 0 ? "disable" : "enable"),
    datatype = "json", function(result) {
      if (result.msg_code != 0) {
        $("#err_dialog").text(result.msg).dialog("open");
        if (id_name === "enc_ctrl_stream_state") {
          $("#enc_ctrl_stream_state").button("option","disabled",false);
        }
      } else {
        gStream_state[id] = $("#enc_ctrl_stream_state").val();
        if ($("#enc_ctrl_stream_state").val() == 1) {
          $("#enc_ctrl_stream_state").button("option", "label", "Disable");
          $("#enc_ctrl_stream_state").val(0);
          if (gVlc_current_video_id === ("video" + id)) {
            vlc_play_start(gVlc_current_video_id,
              gVlc_current_audio_type, gVlc_current_delay);
          }
        } else {
          $("#enc_ctrl_stream_state").button("option", "label", "Enable");
          $("#enc_ctrl_stream_state").val(1);
          if (gVlc_current_video_id === ("video"+id)) {
            vlc_play_stop();
          }
        }
        setTimeout(function(){
          $("#enc_ctrl_stream_state").button("option","disabled",false);
        }, 1000);
      }
  });
}

function get_stream_info(id, callback) {
  $.get("/oryx/video/enc_ctrl/stream"+id+"?action=get", datatype = "json",
    function(result) {
      if (result.msg_code != 0) {
        $("#err_dialog").text(result.msg).dialog("open");
      } else {
        if (callback !== undefined) {
          callback(result.data);
        }
      }
  });
}

function enc_ctrl_stream_control(obj) {
  var id = $("#enc_ctrl_stream_id").val();
  if (id >= gStream_max || id < 0) {
    $("#err_dialog").text(result.msg).dialog("Invalid stream id:" + id);
    return;
  }

  var id_name = $(obj).attr("id");
  var val = 0;
  var apply_flag = false;
  var url = "/oryx/video/enc_ctrl/stream" + id + "?action=set&";
  if (id_name === "enc_ctrl_stream_state") {
    stream_state_control(id);
  } else if (id_name === "enc_ctrl_stream_type") {
    val = $("#enc_ctrl_stream_type").val();
    url += ("type=" + val);
    vlc_play_stop();
    apply_flag = true;
  } else if (id_name === "enc_ctrl_stream_resolution") {
    val = $("#enc_ctrl_stream_resolution").val();
    var w = 0;
    var h = 0;
    var fov = 0;
    if (val == 0) {
      w = 1920;
      h = 1080;
    } else if (val == 1) {
      w = 1280;
      h = 960;
      fov = 1;
    } else if (val == 2) {
      w = 1280;
      h = 960;
      fov = 0;
    } else if (val == 3) {
      w = 1280;
      h = 720;
    } else if (val == 4) {
      w = 720;
      h = 480;
      fov = 1;
    } else if (val == 5) {
      w = 720;
      h = 480;
      fov = 0;
    } else if (val == 6) {
      w = 640;
      h = 480;
      fov = 1;
    } else if (val == 7) {
      w = 640;
      h = 480;
      fov = 0;
    } else if (val == 8) {
      w = 640;
      h = 360;
    }
    url += ("width=" + w + "&height=" + h + "&keep_full_fov=" + fov);
    apply_flag = true;
  } else if (id_name === "enc_ctrl_stream_framerate") {
    val = $("#enc_ctrl_stream_framerate").val();
    url += ("framerate=" + val);
    apply_flag = true;
  } else if (id_name === "enc_ctrl_stream_bitrate") {
    val = $("#enc_ctrl_stream_bitrate").val();
    url += ("bitrate=" + val * 1024);
    apply_flag = true;
  } else if (id_name === "enc_ctrl_stream_mjpeg_quality") {
    val = $("#enc_ctrl_stream_mjpeg_quality").val();
    url += ("quality=" + val);
    apply_flag = true;
  } else if (id_name === "enc_ctrl_stream_gop_n") {
    val = $("#enc_ctrl_stream_gop_n").val();
    url += ("gop_n=" + val);
    apply_flag = true;
  } else if (id_name === "enc_ctrl_stream_gop_idr") {
    val = $("#enc_ctrl_stream_gop_idr").val();
    url += ("gop_idr=" + val);
    apply_flag = true;
  } else if (id_name === "enc_ctrl_stream_force_idr") {
    force_idr();
  }

  if (apply_flag) {
    $.get(url, datatype = "json", function(result) {
      if (result.msg_code != 0) {
        $("#err_dialog").text(result.msg).dialog("open");
      } else {
        if (((id_name === "enc_ctrl_stream_type") ||
        (id_name === "enc_ctrl_stream_resolution")) &&
      (gVlc_current_video_id === ("video" + id))) {
      vlc_play_start(gVlc_current_video_id,
        gVlc_current_audio_type, gVlc_current_delay);
      }
      get_playing_video_info();
      }
    });
  }
}

function force_idr() {
  $.get("/oryx/video/enc_ctrl/stream" + $("#enc_ctrl_stream_id").val() + "/force_idr",
  datatype = "json", function(
    result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
    }
  });
}

var gOverlay_type = 1;
var gOverlay_available_fonts = new Array;
var gOverlay_available_bmps = new Array;
function overlay_area_rotate() {
  if ($("#overlay_area_rotate").val() == 1) {
    $("#overlay_area_rotate").val(0);
    $("#overlay_area_rotate").button("option", "label", "false");
  } else {
    $("#overlay_area_rotate").val(1);
    $("#overlay_area_rotate").button("option", "label", "true");
  }
}

function overlay_data_msec() {
  if ($("#overlay_data_msec").val() == 1) {
    $("#overlay_data_msec").val(0);
    $("#overlay_data_msec").button("option", "label", "disabled");
  } else {
    $("#overlay_data_msec").val(1);
    $("#overlay_data_msec").button("option", "label", "enabled");
  }
}

function overlay_layout() {
  var layout = $("#overlay_layout_select").val();
  if (layout == 4) {
    $("#overlay_area_offset").show();
  } else {
    $("#overlay_area_offset").hide();
  }
}

function show_overlay_text() {
  $("#overlay_picture").hide();
  $("#overlay_text").show();
  $("#overlay_area_size").show();
}

function show_overlay_string() {
  gOverlay_type = 0;
  show_overlay_text();
  $("#overlay_string").show();
  $("#overlay_spacing").show();
  $("#overlay_msec").hide();
  $("#overlay_time_string").hide();
}

function show_overlay_time() {
  gOverlay_type = 2;
  show_overlay_text();
  $("#overlay_string").hide();
  $("#overlay_spacing").show();
  $("#overlay_msec").show();
  $("#overlay_time_string").show();
}

function show_overlay_bmp() {
  $("#overlay_text").hide();
  $("#overlay_area_size").hide();
  $("#overlay_picture").show();
}

function show_overlay_picture() {
  show_overlay_bmp();
  gOverlay_type = 1;
  $("#overlay_bmp_num").hide();
  $("#overlay_bmp_interval").hide();
}

function show_overlay_animation() {
  show_overlay_bmp();
  gOverlay_type = 3;
  $("#overlay_bmp_num").show();
  $("#overlay_bmp_interval").show();
}

function hexFromRGB(r, g, b) {
  var hex = [ r.toString(16), g.toString(16), b.toString(16) ];
  $.each(hex, function(nr, val) {
    if (val.length === 1) {
      hex[nr] = "0" + val;
    }
  });
  return hex.join("").toUpperCase();
}

function refresh_color_swatch(obj) {
  var ele_id = $(obj).attr("id");
  var red = 0;
  var green = 0;
  var blue = 0;
  var fresh_obj = null;
  if (ele_id == "overlay_font_r" || ele_id == "overlay_font_g"
	  ||ele_id == "overlay_font_b"  ) {
    red = $("#overlay_font_r").slider("value");
    green = $("#overlay_font_g").slider("value");
    blue = $("#overlay_font_b").slider("value");
    fresh_obj = "font_color_swatch";
  } else if  (ele_id == "overlay_area_r" || ele_id == "overlay_area_g"
	  ||ele_id == "overlay_area_b"  ) {
    red = $("#overlay_area_r").slider("value");
    green = $("#overlay_area_g").slider("value");
    blue = $("#overlay_area_b").slider("value");
    fresh_obj = "overlay_area_swatch";
}
  var hex = hexFromRGB(red, green, blue);
  $("#"+fresh_obj).css("background-color", "#" + hex);
}

function overlay_set_autocomplete_fonts_source() {
  $("#overlay_font_type").autocomplete(
      {
        source : function(request, response) {
          var matcher = new RegExp($.ui.autocomplete
              .escapeRegex(request.term)
              + "*", "i");
          response($.grep(gOverlay_available_fonts, function(item) {
            return matcher.test(item);
          }));
        },
      });
}

function get_overlay_available_fonts_from_remote() {
  $.get("/oryx/video/overlay/font?action=get", datatype = "json", function(
        result) {
    if (result.msg_code == 0) {
      gOverlay_available_fonts = result.data;
      overlay_set_autocomplete_fonts_source();
    }
  });
};

function overlay_set_autocomplete_bmps_source() {
  $("#overlay_picture_file").autocomplete(
      {
        source : function(request, response) {
          var matcher = new RegExp($.ui.autocomplete
              .escapeRegex(request.term)
              + "*", "i");
          response($.grep(gOverlay_available_bmps, function(item) {
            return matcher.test(item);
          }));
        },
      });
}

function get_overlay_available_bmps_from_remote() {
  $.get("/oryx/video/overlay/bmp?action=get", datatype = "json", function(
        result) {
    if (result.msg_code == 0) {
      gOverlay_available_bmps = result.data;
      overlay_set_autocomplete_bmps_source();
    }
  });
}

function color_space_transform(yuv, rgb) {
  yuv[0] = 0.257 * rgb[0] + 0.504 * rgb[1] + 0.098 * rgb[2] + 16;
  yuv[1] = 0.439 * rgb[2] - 0.291 * rgb[1] - 0.148 * rgb[0] + 128;
  yuv[2] = 0.439 * rgb[0] - 0.368 * rgb[1] - 0.071 * rgb[2] + 128;
}

function get_overlay_init_data() {
  var json = {
    "rotate" : $("#overlay_area_rotate").val() ,
    "buf_num" : $("#overlay_area_buf_num").val() ,
    "width" : $("#overlay_area_width").val(),
    "height" : $("#overlay_area_height").val(),
    "position_x" : $("#overlay_area_startx").val(),
    "position_y" : $("#overlay_area_starty").val(),
  };
  var yuv = new Array;
  var rgb = new Array;
  rgb[0] = $("#overlay_area_r").slider("value");
  rgb[1] = $("#overlay_area_g").slider("value");
  rgb[2] = $("#overlay_area_b").slider("value");
  var a = $("#overlay_area_a").val();
  color_space_transform(yuv, rgb);
  json.bg_color = yuv[2] << 24 | yuv[1] << 16 | yuv[0] << 8 | a;

  return json;
}

function init_overlay() {
  var stream_id = $("#overlay_stream_id").val();
  $.ajax({
    url : "/oryx/video/overlay/stream" + stream_id + "?action=set",
    type : "POST",
    contentType : "application/json",
    datatype : "json",
    data : JSON.stringify(get_overlay_init_data()),
    success : function(data, textStatus) {
      if (data.msg_code != 0) {
        $("#err_dialog").text(data.msg).dialog("open");
      }
    },
  });
}

function get_overlay_Json_data() {
  var json = {
    "type" : gOverlay_type,
    "width" : $("#overlay_data_width").val(),
    "height" : $("#overlay_data_height").val(),
    "position_x" : $("#overlay_data_startx").val(),
    "position_y" : $("#overlay_data_starty").val(),
  };
  var yuv = new Array;
  var rgb = new Array;
  if (gOverlay_type == 1 || gOverlay_type == 3) {
    rgb[0] = $("#overlay_bmp_color_r").val();
    rgb[1] = $("#overlay_bmp_color_g").val();
    rgb[2] = $("#overlay_bmp_color_b").val();
    var a = $("#overlay_bmp_color_a").val();
    color_space_transform(yuv, rgb);
    json.color_key = yuv[2] << 24 | yuv[1] << 16 | yuv[0] << 8 | a;
    json.color_range = $("#overlay_bmp_color_range").val();
    if ($("#overlay_picture_file").val() != "") {
      json.bmp_file = $("#overlay_picture_file").val();
    }
    if (gOverlay_type == 3) {
      json.bmp_num = $("#overlay_data_bmp_num").val();
      json.interval = $("#overlay_data_bmp_interval").val();
    }
  } else {
    rgb[0] = $("#overlay_font_r").slider("value");
    rgb[1] = $("#overlay_font_g").slider("value");
    rgb[2] = $("#overlay_font_b").slider("value");
    color_space_transform(yuv, rgb);
    json.font_color = yuv[2] << 24 | yuv[1] << 16 | yuv[0] << 8 | 255;
    if ($("#overlay_font_type").val() != "") {
      json.font_type = $("#overlay_font_type").val();
    }
    json.spacing = $("#overlay_string_spacing").val();
    json.en_msec = $("#overlay_data_msec").val();
    json.font_size = $("#overlay_font_size").val();
    json.font_outwidth = $("#overlay_font_outwidth").val();
    json.font_horbold = $("#overlay_font_horbold").val();
    json.font_verbold = $("#overlay_font_verbold").val();
    json.font_italic = $("#overlay_font_italic").val();
    if ((gOverlay_type == 0) && ($("#overlay_string_text").val() != "")) {
      json.text_str = $("#overlay_string_text").val();
    }
    if ((gOverlay_type == 2) && ($("#overlay_time_string_prefix").val() != "")) {
      json.pre_str = $("#overlay_time_string_prefix").val();
    }
    if ((gOverlay_type == 2) && ($("#overlay_time_string_suffix").val() != "")) {
      json.suf_str = $("#overlay_time_string_suffix").val();
    }
  }
  return json;
}

function manipulate_overlay_data(action)
{
  var stream_id = $("#overlay_data_stream_id").val();
  var area_id = $("#overlay_data_area_id").val();
  if (action === undefined) {
    action = $("#overlay_data_manipulate_select").val();
  }
  var url = "/oryx/video/overlay/stream" + stream_id + "/area" + area_id;
  if (action === "set") {
    url += "?action=" + action;
  } else {
    if (action === "update") {
      action = "set";
    }
    var data_id = $("#overlay_data_id").val();
    url += "/block" + data_id + "?action=" + action;
  }
  $.ajax({
      url ,
      type : "POST",
      contentType : "application/json",
      datatype : "json",
      data : JSON.stringify(get_overlay_Json_data()),
      success : function(data, textStatus) {
        if (data.msg_code != 0) {
          $("#err_dialog").text(data.msg).dialog("open");
        }
      },
  });
}

var gOverlay_area_info = null;
function overlay_manipulate(action) {
  var url = "/oryx/video/overlay/";
  if (action === "destory" || action === "save") {
	  url += "?action=" + action;
  } else {
      var stream_id = $("#overlay_stream_id").val();
      var area_id = $("#overlay_area_id").val();
	  url += "stream" + stream_id + "/area" + area_id + "?action=" + action;
  }
  $.get(url, datatype = "json", function(result) {
        if (result.msg_code != 0) {
		  gOverlay_area_info = null;
          $("#err_dialog").text(result.msg).dialog("open");
        } else if (action == "get") {
		  gOverlay_area_info = result.data;
		  overlay_area_info(result.data);
        }
      });
}

function overlay_area_info(data) {
  var txt = " ";
  var key;
  for (key in data) {
	if (key != "data") {
      txt += key + ": " + data[key] + "</br>";
	}
  }
  txt += "</br>"
  var n = data["data_block_num"];
  for (var i = 0; i < n; ++i) {
	if (data["data"][i]["type"] == 404) {
		++n;
		continue;
	}
    var data_id = "overlay_data_block" + i;
    var dyn_button = '<button id=' + data_id + ' onclick="overlay_data_info(this)" class="ui-corner-all"  ' +
      'style="background-color:rgb(223, 239, 252); marg-left:10px;height:35px;width:45px">More</button></br>';

    txt += "data block" + i + ": " +  dyn_button;
  }
  $("#success_dialog").html(txt).dialog("option", "title", "Overlay Parameters");
  $("#success_dialog").dialog("option", "buttons",
    [ {
        text: "Ok",
        click: function() {$( this ).dialog( "close" ); }
    } ]
  );
  $("#success_dialog").dialog("open");
}

function overlay_data_info(obj) {
  var ele_id = $(obj).attr("id");
  var index;
  var txt = " ";
  var key;
  for ( index in gOverlay_area_info["data"]) {
    if (ele_id != ("overlay_data_block" + index)) {
      continue;
    }
    var data = gOverlay_area_info["data"][index];
    for (key in data) {
      txt += key + ": " + data[key] + "</br>";
    }
    $("#success_dialog").html(txt).dialog("option", "buttons",
      [{
          text: "Back",
          click: function() {overlay_area_info(gOverlay_area_info);}
    }]	);
    break;
  }
}

var gZoom_val = new Array();
gZoom_val = [ 1, 1, 1, 1 ];
var gPan_val = new Array();
gPan_val = [ 0.0, 0.0, 0.0, 0.0 ];
var gTilt_val = new Array();
gTilt_val = [ 0.0, 0.0, 0.0, 0.0 ];
var gBuffer_id = 0;
function set_dptz_buffer() {
  gBuffer_id = $("#dptz_buffer_id").val();
  $("#zoom").val(gZoom_val[gBuffer_id]);
  $("#zoom_slider").slider("value", gZoom_val[gBuffer_id]);
  $("#pan").val(gPan_val[gBuffer_id]);
  $("#tilt").val(gTilt_val[gBuffer_id]);
}

function switch_dptz_option(value) {
  if (value === "ratio") {
    $("#dptz_ratio_option").show();
    $("#dptz_size_option").hide();
  } else if (value === "size") {
    $("#dptz_ratio_option").hide();
    $("#dptz_size_option").show();
  }
}

function set_dptz_size() {
  var x =   $("#dptz_x").val();
  var y =   $("#dptz_y").val();
  var w =   $("#dptz_w").val();
  var h =   $("#dptz_h").val();
  $.get("/oryx/video/dptz/buffer" + gBuffer_id + "?action=set&dptz_x=" + x + "&dptz_y=" +
      y + "&dptz_w=" + w + "&dptz_h=" + h, datatype = "json", function(result) {
        if (result.msg_code != 0) {
          $("#err_dialog").text(result.msg).dialog("open");
        }
      });
}

function set_dptz_zoom(value) {
  $.get("/oryx/video/dptz/buffer" + gBuffer_id + "?action=set&zoom_ratio="
      + value, datatype = "json", function(result) {
        if (result.msg_code != 0) {
          $("#err_dialog").text(result.msg).dialog("open");
          $("#zoom").val(gZoom_val[gBuffer_id]);
          $("#zoom_slider").slider("value", gZoom_val[gBuffer_id]);
        } else {
          gZoom_val[gBuffer_id] = value;
        }
      });
}

function set_dptz_pan(value) {
  gPan_val[gBuffer_id] += value;
  if (gPan_val[gBuffer_id].toFixed(2) > 1.00) {
    gPan_val[gBuffer_id] = 1.00;
    return;
  }
  if (gPan_val[gBuffer_id].toFixed(2) < -1.00) {
    gPan_val[gBuffer_id] = -1.00;
    return;
  }
  $.get("/oryx/video/dptz/buffer" + gBuffer_id + "?action=set&pan_ratio="
    + gPan_val[gBuffer_id].toFixed(2), datatype = "json", function(result) {
      if (result.msg_code != 0) {
        $("#err_dialog").text(result.msg).dialog("open");
        gPan_val[gBuffer_id] -= value;
      } else {
        $("#pan").val(gPan_val[gBuffer_id].toFixed(2));
      }
  });
}

function set_dptz_tilt(value) {
  gTilt_val[gBuffer_id] += value;
  if (gTilt_val[gBuffer_id].toFixed(2) > 1.0) {
    gTilt_val[gBuffer_id] = 1.0;
    return;
  }
  if (gTilt_val[gBuffer_id].toFixed(2) < -1.0) {
    gTilt_val[gBuffer_id] = -1.0;
    return;
  }
  $.get("/oryx/video/dptz/buffer" + gBuffer_id + "?action=set&tilt_ratio="
    + gTilt_val[gBuffer_id].toFixed(2), datatype = "json", function(result) {
      if (result.msg_code != 0) {
        $("#err_dialog").text(result.msg).dialog("open");
        gTilt_val[gBuffer_id] -= value;
      } else {
        $("#tilt").val(gTilt_val[gBuffer_id].toFixed(2));
      }
  });
}

var gWarp_mode = 1;
var gLdc_strength = 0;
var gWarp_hor_zoom = 1;
var gWarp_ver_zoom = 1;
var gWarp_region_yaw = 0;
var gWarp_region_pitch = 0;
var gPano_hfov = 1;
function switch_warp_mode(value) {
  if (value === "rectangle") {
    $("#rectangle_mode_option").show();
    $("#panorama_mode_option").hide();
    gWarp_mode = 1;
  } else if (value === "panorama") {
    $("#rectangle_mode_option").hide();
    $("#panorama_mode_option").show();
    gWarp_mode = 2;
  }
 }

function set_warp_region_pitch(value) {
  gWarp_region_pitch += value;
  if (gWarp_region_pitch > 90) {
    gWarp_region_pitch = 90;
    return;
  }
  if (gWarp_region_pitch < -90) {
    gWarp_region_pitch = -90;
    return;
  }
  set_dewarp_parameter();
  $("#pitch").val(gWarp_region_pitch);
}

function set_warp_region_yaw(value) {
  gWarp_region_yaw += value;
  if (gWarp_region_yaw > 90) {
    gWarp_region_yaw = 90;
    return;
  }
  if (gWarp_region_yaw < -90) {
    gWarp_region_yaw = -90;
    return;
  }
  set_dewarp_parameter();
  $("#yaw").val(gWarp_region_yaw);
}

function set_dewarp_parameter() {
  var hor_zoom_num = $("#warp_hor_zoom").val() * 100;
  var ver_zoom_num = $("#warp_ver_zoom").val() * 100;
  var zoom_den = 100;
  $.get("/oryx/video/dewarp?action=set&ldc_mode=" + gWarp_mode
    + "&ldc_strength=" + $("#strength").val()
    + "&max_radius=" + $("#len_max_radius").val()
    + "&pano_hfov_degree=" + $("#pano_hfov").val()
    + "&warp_region_yaw=" + gWarp_region_yaw
    + "&warp_region_pitch=" + gWarp_region_pitch
    + "&warp_hor_zoom=" + hor_zoom_num.toFixed(0) + "/" + zoom_den
    + "&warp_ver_zoom=" + ver_zoom_num.toFixed(0) + "/" + zoom_den,
  datatype = "json", function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
      $("#strength").val(gLdc_strength.toFixed(2));
      $("#strength_slider").slider("value", gLdc_strength.toFixed(2));
      $("#pano_hfov").val(gPano_hfov.toFixed(1));
      $("#pano_hfov_slider").slider("value", gPano_hfov.toFixed(1));
      $("#warp_hor_zoom").val(gWarp_hor_zoom.toFixed(2));
      $("#warp_hor_zoom_slider").slider("value", gWarp_hor_zoom.toFixed(2));
      $("#warp_ver_zoom").val(gWarp_ver_zoom).toFixed(2);
      $("#warp_ver_zoom_slider").slider("value", gWarp_ver_zoom.toFixed(2));
    } else {
      gLdc_strength = $("#strength").val();
      gPano_hfov = $("#pano_hfov").val();
      gWarp_hor_zoom = $("#warp_hor_zoom").val();
      gWarp_ver_zoom = $("#warp_ver_zoom").val();
    }
  });
}

var gMedia_state = 0;
function media_start_recording() {
  $("#media_stop_recording").button("option","disabled",true);
  $("#media_start_recording").button("option","disabled",true);
  $.get("/oryx/media/recording?action=start", datatype = "json", function(
        result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
      $("#media_start_recording").button("option","disabled",false);
    } else {
      gMedia_state = 1;
      setTimeout(function(){
        if (gMedia_state === 1) {
          if (vlc_is_playing() === false) {
            vlc_play_start(gVlc_current_video_id,
                gVlc_current_audio_type, gVlc_current_delay);
          }
          $("#media_stop_recording").button("option","disabled",false);
        }
      }, 1000);
    }
  });
}

function media_stop_recording() {
  if (vlc_is_playing() === true) {
    vlc_play_stop();
  }
  $("#media_start_recording").button("option","disabled",true);
  $("#media_stop_recording").button("option","disabled",true);
  $.get("/oryx/media/recording?action=stop", datatype = "json", function(
        result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
      $("#media_stop_recording").button("option","disabled",false);
    }  else {
      gMedia_state = 0;
      setTimeout(function(){
        $("#media_start_recording").button("option","disabled",false);
      }, 1000);
    }
  });
}

function get_media_file_muxer() {
  var muxer = 0;
  if($("#media_file_mp4").is(":checked")==true) {
    muxer += 1;
  }
  if($("#media_file_ts").is(":checked")==true) {
    muxer += 8;
  }
  if($("#media_file_jpeg").is(":checked")==true) {
    muxer += 128;
  }
  return muxer;
}

function media_file_recording_start() {
  var val = get_media_file_muxer();
  $.get("/oryx/media/recording/file?action=start&muxer_id="+val, datatype = "json",
      function(result) {
        if (result.msg_code != 0) {
          $("#err_dialog").text(result.msg).dialog("open");
        }
      });
}

function media_file_recording_stop() {
  var val = get_media_file_muxer();
  $.get("/oryx/media/recording/file?action=stop&muxer_id="+val, datatype = "json",
      function(result) {
        if (result.msg_code != 0) {
          $("#err_dialog").text(result.msg).dialog("open");
        }
      });
}

var g_media_event_type = "h26x";
function switch_media_event_option(value) {
  if (value === "h26x") {
    $("#media_event_id_label").text("Event ID");
    $("#event_mjpeg_block").hide();
    g_media_event_type = "h26x";
  } else if (value === "mjpeg") {
    $("#media_event_id_label").text("Stream ID");
    $("#event_mjpeg_block").show();
    g_media_event_type = "mjpeg";
  }
}

function media_event_recording() {
  var event_data = {
    "type" : g_media_event_type,
    "event_id" : $("#media_recording_event_id").val() ,
    "pre_num" : $("#media_recording_history_dur").val(),
    "post_num" : $("#media_recording_future_dur").val(),
    "closest_num" : $("#media_recording_closest_dur").val(),
  }
  $.ajax({
    url : "/oryx/media/recording/event",
    type : "POST",
    contentType : "application/json",
    datatype : "json",
    data : JSON.stringify(event_data),
  });
}

var gPlayback_audio_add_times = 0;
var gPlayback_audio_state = 0;
function media_playback_add_audio_file() {
  ++gPlayback_audio_add_times;
  var content = '<div style="margin-bottom:5px;">Path&nbsp<input id="media_playback_audio_file'
    + gPlayback_audio_add_times
    + '"  type="text"  class=ui-corner-all  style="float:center;height:30px;width:350px" value="" />';
  content += '&nbsp<button  onclick="media_playback_del_audio_file(this)" class=ui-corner-all style="height:35px;width:35px;background-color:rgb(223, 239, 252)">-</button></div>';
  $("#media_playback_audio_file").append(content);
}

function media_playback_del_audio_file(obj) {
  --gPlayback_audio_add_times;
  $(obj).parent().remove();
}

function media_palyback_audio_add() {
  $.ajax({
    url : "/oryx/media/playback/audio?action=set",
    type : "POST",
    contentType : "application/json",
    datatype : "json",
    data : JSON.stringify(get_audio_playback_Json_data()),
    success : function(data, textStatus) {
      if (data.msg_code != 0) {
        $("#err_dialog").text(data.msg).dialog("open");
      } else {
        gPlayback_audio_state = 1;
      }
    },
  });
}

function get_audio_playback_Json_data() {
  var m = 0;
  var file_name;
  var json = {};
  for (var i = 0; i <= gPlayback_audio_add_times; ++i) {
    var file_id = "media_playback_audio_file" + i;
    if ($("#" + file_id).length > 0) {
      if ($("#" + file_id).val() != "") {
        json["file" + m] = $("#" + file_id).val();
        ++m;
      }
    }
  }
  return json;
}

function media_palyback_audio_play() {
  if (gPlayback_audio_state == 0) {
    $("#err_dialog").text("Please add audio file firstly").dialog("open");
  } else if (gPlayback_audio_state != 2) {
    $.get("/oryx/media/playback/audio?action=start", datatype = "json",
        function(result) {
          if (result.msg_code != 0) {
            $("#err_dialog").text(result.msg).dialog("open");
          } else {
            gPlayback_audio_state = 2;
          }
        });
  }
}

function media_palyback_audio_pause() {
  if (gPlayback_audio_state == 2) {
    $.get("/oryx/media/playback/audio?action=pause", datatype = "json",
        function(result) {
          if (result.msg_code != 0) {
            $("#err_dialog").text(result.msg).dialog("open");
          } else {
            gPlayback_audio_state = 1;
          }
        });
  }
}

function media_palyback_audio_stop() {
  $.get("/oryx/media/playback/audio?action=stop", datatype = "json",
      function(result) {
        if (result.msg_code != 0) {
          $("#err_dialog").text(result.msg).dialog("open");
        } else {
          gPlayback_audio_state = 0;
        }
      });
}

function get_AE_init_state() {
  $.get("/oryx/image/ae?action=get", datatype = "json", function(result) {
    if (result.msg_code == 0) {
      $("#ir_led_mode").val(result.data["ir_led_mode"]);
      $("#day_night_mode").val(result.data["day_night_mode"]);
      change_day_night_state();
      $("#anti_flicker_mode").val(result.data["anti_flicker_mode"]);
      change_anti_flicker_state();
      $("#back_light_comp").val(result.data["back_light_comp"]);
      change_backlight_comp_state();
      $("#local_exposure").val(result.data["local_exposure"]);
      $("#ae_metering_mode").val(result.data["ae_metering_mode"]);
      $("#dc_iris_enable").val(result.data["dc_iris_enable"]);
      change_dc_iris_state();
      $("#ae_enable").val(result.data["ae_enable"]);
      change_ae_state();
      $("#ae_target_ratio").spinner( "value", result.data["ae_target_ratio"] );
      $("#slow_shutter_mode").val(result.data["slow_shutter_mode"]);
      change_slow_shutter_state();
      $("#shutter_time_manual").val(result.data["shutter_time_manual"]);
      $("#sensor_shutter_min").val(result.data["sensor_shutter_min"]);
      $("#sensor_shutter_max").val(result.data["sensor_shutter_max"]);
      $("#sensor_gain_manual").spinner( "value", result.data["sensor_gain_manual"] );
      $("#sensor_gain_max").spinner( "value", result.data["sensor_gain_max"] );
    }
  });
}

function set_AE_parameter(name, value) {
  $.get("/oryx/image/ae?action=set&" + name + "=" + value, datatype = "json",
      function(result) {
        if (result.msg_code != 0) {
          $("#err_dialog").text(result.msg).dialog("open");
        } else {
          if (name === "day_night_mode") {
            change_day_night_state();
          } else if (name === "anti_flicker_mode") {
            change_anti_flicker_state();
          } else if (name === "back_light_comp") {
            change_backlight_comp_state();
          } else if (name === "dc_iris_enable") {
            change_dc_iris_state();
          } else if (name === "ae_enable") {
            change_ae_state();
          } else if (name === "slow_shutter_mode") {
            change_slow_shutter_state();
          }
        }
      });
}

function change_day_night_state() {
  if ($("#day_night_mode").val() == 0) {
    $("#day_night_mode").val(1);
    $("#day_night_mode").button("option", "label", "Night");
  } else {
    $("#day_night_mode").val(0);
    $("#day_night_mode").button("option", "label", "Day");
  }
}

function change_anti_flicker_state() {
  if ($("#anti_flicker_mode").val() == 0) {
    $("#anti_flicker_mode").val(1);
    $("#anti_flicker_mode").button("option", "label", "60Hz");
  } else {
    $("#anti_flicker_mode").val(0);
    $("#anti_flicker_mode").button("option", "label", "50Hz");
  }
}

function change_backlight_comp_state() {
  if ($("#back_light_comp").val() == 0) {
    $("#back_light_comp").val(1);
    $("#back_light_comp").button("option", "label", "Open");
  } else {
    $("#back_light_comp").val(0);
    $("#back_light_comp").button("option", "label", "Close");
  }
}

function change_dc_iris_state() {
  if ($("#dc_iris_enable").val() == 0) {
    $("#dc_iris_enable").val(1);
    $("#dc_iris_enable").button("option", "label", "Open");
  } else {
    $("#dc_iris_enable").val(0);
    $("#dc_iris_enable").button("option", "label", "Close");
  }
}

function change_ae_state() {
  if ($("#ae_enable").val() == 0) {
    $("#ae_enable").val(1);
    $("#ae_enable").button("option", "label", "Open");
    $("#sensor_shutter_max").selectmenu("option", "disabled", true);
    $("#sensor_shutter_min").selectmenu("option", "disabled", true);
    $("#shutter_time_manual").selectmenu("option", "disabled", false);
    $("#sensor_gain_max").spinner("option", "disabled", true);
    $("#sensor_gain_manual").spinner("option", "disabled", false);
  } else {
    $("#ae_enable").val(0);
    $("#ae_enable").button("option", "label", "Close");
    $("#sensor_shutter_max").selectmenu("option", "disabled", false);
    $("#sensor_shutter_min").selectmenu("option", "disabled", false);
    $("#shutter_time_manual").selectmenu("option", "disabled", true);
    $("#sensor_gain_max").spinner("option", "disabled", false);
    $("#sensor_gain_manual").spinner("option", "disabled", true);
  }
}

function change_slow_shutter_state() {
  if ($("#slow_shutter_mode").val() == 0) {
    $("#slow_shutter_mode").val(1);
    $("#slow_shutter_mode").button("option", "label", "Open");
  } else {
    $("#slow_shutter_mode").val(0);
    $("#slow_shutter_mode").button("option", "label", "Close");
  }
}

function get_AWB_init_state() {
  $.get("/oryx/image/awb?action=get", datatype = "json", function(result) {
    if (result.msg_code == 0) {
      $("#AWB").val(result.data["wb_mode"]);
    }
  });
}

function set_AWB_mode(value) {
  $.get("/oryx/image/awb?action=set&wb_mode=" + value, datatype = "json",
      function(result) {
        if (result.msg_code != 0) {
          $("#err_dialog").text(result.msg).dialog("open");
        }
      });
}

function get_AF_init_state() {
  $("#AF > option").attr("disabled", "disabled");
}

function get_image_style_init_state() {
  $.get("/oryx/image/style?action=get", datatype = "json", function(result) {
    if (result.msg_code == 0) {
      var data = result.data;
      var key;
      for (key in data) {
        var val = data[key];
        $("#" + key + "_slider").slider("value", val);
        $("#style_" + key).val(val);
      }
    }
  });
}

function set_image_style() {
  var hue_val = $("#hue_slider").slider("value");
  $("#style_hue").val(hue_val);
  var saturation_val = $("#saturation_slider").slider("value");
  $("#style_saturation").val(saturation_val);
  var sharpness_val = $("#sharpness_slider").slider("value");
  $("#style_sharpness").val(sharpness_val);
  var brightness_val = $("#brightness_slider").slider("value");
  $("#style_brightness").val(brightness_val);
  var contrast_val = $("#contrast_slider").slider("value");
  $("#style_contrast").val(contrast_val);
  $.get("/oryx/image/style?action=set&hue=" + hue_val + "&saturation="
      + saturation_val + "&sharpness=" + sharpness_val + "&brightness="
      + brightness_val + "&contrast=" + contrast_val, datatype = "json",
      function(result) {
        if (result.msg_code != 0) {
          $("#err_dialog").text(result.msg).dialog("open");
        }
      });
}

function get_denoise_init_state() {
  $.get("/oryx/image/denoise?action=get", datatype = "json",
      function(result) {
        if (result.msg_code == 0) {
          var data = result.data;
          var key;
          for (key in data) {
            var val = data[key];
            $("#mctf_strength_slider").slider("value", val);
            $("#mctf_strength").val(val);
          }
        }
      });
}

function set_denoise() {
  var val = $("#mctf_strength_slider").slider("value");
  $("#mctf_strength").val(val);
  $.get("/oryx/image/denoise?action=set&mctf_strength=" + val,
      datatype = "json", function(result) {
        if (result.msg_code != 0) {
          $("#err_dialog").text(result.msg).dialog("open");
        }
      });
}

var gSD_life_details = {"spare_block_rate": 0,
										"actual_bytes_written": 0,
										"total_bytes_writable": 0};
function card_life_update() {
  $.get("/oryx/system/sdcard/info/life",
    datatype = "json", function(result) {
      if (result.msg_code != 0) {
        $("#card_life_status").val(result.msg);
        gSD_life_details.spare_block_rate = 0;
        gSD_life_details.actual_bytes_written = 0;
        gSD_life_details.total_bytes_writable = 0;
        $("#card_life_consumed").text("");
        $("#card_life_left").val("");
        $("#card_life_consumed_bar").progressbar({
            value: 0
        });
      } else {
        $("#card_life_status").val(result.data["state"]);
        $("#card_life_consumed").text(result.data["life_percent"]+"%");
        $("#card_life_left").val(result.data["usable_working_days"]);
        $("#card_life_consumed_bar").progressbar({
            value: result.data["life_percent"]
        });
        if (result.data["life_percent"] == 100) {
          $("#card_life_consumed_bar .ui-progressbar-value").css("background","#ff0000");
        } else if (result.data["life_percent"] >= 90) {
          $("#card_life_consumed_bar .ui-progressbar-value").css("background","#ffff00");
        } else {
          $("#card_life_consumed_bar .ui-progressbar-value").css("background","#00ff00");
        }
        gSD_life_details.spare_block_rate = result.data["spare_block_rate"];
        gSD_life_details.actual_bytes_written = result.data["life_information_num"] *
        result.data["data_size_per_unit"] * 512;
        gSD_life_details.total_bytes_writable = result.data["life_information_den"] *
        result.data["data_size_per_unit"] * 512;
      }
    });
}

function card_life_details() {
  var txt = "Space Blocks Used: " + gSD_life_details["spare_block_rate"] + "</br>" +
    "Actual Bytes Written: " + gSD_life_details["actual_bytes_written"] + " Bytes</br>" +
    "Total Bytes Writable: " + gSD_life_details["total_bytes_writable"] + " Bytes";
  $("#success_dialog").html(txt).dialog("option", "title", "SD Card Use Details");
  $("#success_dialog").dialog("option", "buttons",
    [ {
        text: "Ok",
        click: function() {$( this ).dialog( "close" ); }
    } ]
  );
  $("#success_dialog").dialog("open");
}

var gSD_err_details=null;
function card_err_check() {
  $.get("/oryx/system/sdcard/info/err",
    datatype = "json", function(result) {
      if (result.msg_code != 0) {
        $("#err_dialog").text(result.msg).dialog("open");
        if (gSD_err_details =! null) {
          gSD_err_details = null;
          $("#card_err_num").val("");
        }
      } else {
        $("#card_err_num").val(result.data["error_count"]);
        gSD_err_details = result.data["err"];
      }
  });
}

function card_err_details() {
  var txt = " ";
  if (gSD_err_details == null || gSD_err_details.length == 0) {
    txt = "No error";
  } else {
    var index;
    var err_type;
    var block_type;
    for (index in gSD_err_details) {
      if (gSD_err_details[index]["error_type"] == 0x01) {
        err_type = "Write page error";
      } else if (gSD_err_details[index]["error_type"] == 0x02) {
        err_type = "Read page error";
      } else if (gSD_err_details[index]["error_type"] == 0x03) {
        err_type = "Erase block error";
      } else if (gSD_err_details[index]["error_type"] == 0x04) {
        err_type = "Timeout error";
      } else if (gSD_err_details[index]["error_type"] == 0x05) {
        err_type = "CRC error";
      } else {
        err_type = "Unknown error";
      }
      if (gSD_err_details[index]["block_type"] == 0x01) {
        block_type = "SLC mode";
      } else if (gSD_err_details[index]["block_type"] == 0x02) {
        block_type = "MLC mode";
      } else if (gSD_err_details[index]["block_type"] == 0x03) {
        block_type = "TLC mode";
      } else {
        block_type = "Unknown mode";
      }

      var err_id = "card_err_details_more" + index;
      var dyn_button = '<button id=' + err_id + ' onclick="card_err_more(this)" class="ui-corner-all"  ' +
        'style="background-color:rgb(223, 239, 252); marg-left:10px;height:35px;width:45px">More</button></br>';

      txt += gSD_err_details[index]["error_no"]  + ".  " + err_type +  "," + block_type + " block..." + dyn_button;
    }
  }
  $("#success_dialog").html(txt).dialog("option", "title", "SD Card Err Details");
  $("#success_dialog").dialog("option", "buttons",
    [ {
        text: "Ok",
        click: function() {$( this ).dialog( "close" ); }
    } ]
  );
  $("#success_dialog").dialog("open");
}

function card_err_more(obj) {
  var ele_id = $(obj).attr("id");
  var index;
  for ( index in gSD_err_details) {
    if (ele_id != ("card_err_details_more" + index)) {
      continue;
    }
    var txt = " ";
    var key;
    for (key in gSD_err_details[index]) {
      if (key == "error_type" || key == "block_type") {
        continue;
      }
      if (key == "history") {
        var data = gSD_err_details[index]["history"];
        for (i in data) {
          txt += "History" + i + " cmd: 0x" + data[i].cmd.toString(16) + ", sector addr: 0x" +
            data[i].sector_addr.toString(16) + ", resp: 0x" + data[i].resp.toString(16) + "</br>";
        }
      } else if (key == "error_no") {
        txt += "ERR No: "  + gSD_err_details[index][key] + "</br>";
      } else if (key == "logical_CE") {
        txt += "Logical CE: 0x"  + gSD_err_details[index][key].toString(16) + "</br>";
      } else if (key == "physical_plane") {
        txt += "Physical plane: 0x"  + gSD_err_details[index][key].toString(16) + "</br>";
      } else if (key == "physical_block") {
        txt += "Physical block: 0x"  + gSD_err_details[index][key].toString(16) + "</br>";
      } else if (key == "physical_page") {
        txt += "Physical page: 0x"  + gSD_err_details[index][key].toString(16) + "</br>";
      } else if (key == "error_type_of_timeout") {
        txt += "Error type of timeout: 0x" + gSD_err_details[index][key].toString(16) + "</br>";
      } else if (key == "erase_block_count") {
        txt += "Erase block count: "  + gSD_err_details[index][key] + "</br>";
      } else if (key == "write_page_error_count") {
        txt += "Write page error count: "  + gSD_err_details[index][key] + "</br>";
      } else if (key == "read_page_error_count") {
        txt += "Read page error count: "  + gSD_err_details[index][key] + "</br>";
      } else if (key == "erase_block_error_count") {
        txt += "Erase block error count: "  + gSD_err_details[index][key] + "</br>";
      } else if (key == "crc_error_count") {
        txt += "CRC error count: "  + gSD_err_details[index][key] + "</br>";
      } else if (key == "timeout_count") {
        txt += "Timeout count: "  + gSD_err_details[index][key] + "</br>";
      }
    }
    $("#success_dialog").html(txt).dialog("option", "buttons",
      [{
          text: "Back",
          click: function() {card_err_details();}
    }]	);
    break;
  }
}

function is_vlc_installed() {
  if (navigator.plugins && navigator.plugins.length) {
    for (var x = 0; x < navigator.plugins.length; x++) {
      if (navigator.plugins[x].name.indexOf("VLC") > -1
          || navigator.plugins[x].name.indexOf("VideoLAN") > -1) {
        return true;
      }
    }
  }
  if (confirm("Please install vlc(with Mozilla plugin) first.")) {
    window.open("http://www.videolan.org/vlc/", "", "");
  }
  return false;
}

var gVlc_current_video_id = "video0";
var gVlc_current_audio_type = "aac";
var gVlc_current_delay = 300;

function set_playing_video_info(data) {
  var text = "Encode Type: " + ((data["encode_type"] == 1) ? "H264" :
    ((data["encode_type"] == 2) ? "H265" : ((data["encode_type"] == 3) ? "MJPEG" : "None")));
  if ((data["encode_type"] > 0) && (data["encode_type"] < 4)) {
    text += (" &nbsp &nbsp Resolution: " + data["width"] + "x" + data["height"] +
        " &nbsp &nbsp Framerate: " + data["framerate"] + " &nbsp &nbsp ");
    if (data["encode_type"] == 3) {
      text += ("Quality: " + data["quality"]);
    } else {
      text += ("Bitrate: " + data["bitrate"]/1000 + " kbit/s");
    }
  }
  $("#playing_video_info").html(text);
}

function get_playing_video_info() {
  var strs = gVlc_current_video_id.split("video");
  get_stream_info(strs[1], set_playing_video_info);
}

function vlc_init() {
  if (is_vlc_installed()) {
    var vlc = document.getElementById('vlc');
    if (vlc.VersionInfo[0] < 3) {
      $("#audio_opus").attr("disabled", "disabled");
    }
    vlc_play_start(gVlc_current_video_id, gVlc_current_audio_type,
        gVlc_current_delay);
  }
}

function vlc_play_start(video_id, audio_type, delay) {
  var options = new Array("network-caching=" + delay);
  var strs = window.location.href.split("://");
  strs = strs[1].split("/");
  var targetURL;
  if (audio_type != "none") {
    targetURL = 'rtsp://' + strs[0] + '/video=' + video_id + ',audio='
      + audio_type;
  } else {
    targetURL = 'rtsp://' + strs[0] + '/video=' + video_id;
  }
  var vlc = document.getElementById('vlc');
  if (vlc) {
    var itemId = vlc.playlist.add(targetURL, "ambarella-options", options);
    if (itemId != -1) {
      vlc.playlist.playItem(itemId);
	  get_playing_video_info();
    } else {
      alert("cannot play at the this moment!");
    }
  }
}

function vlc_play_stop() {
  var vlc = document.getElementById('vlc');
  if (vlc) {
    vlc.playlist.stop();
    var data = {"encode_type" : 0};
    set_playing_video_info(data);
  }
}

function vlc_is_playing() {
	var vlc = document.getElementById('vlc');
	return vlc.playlist.isPlaying;
}

function vlc_control() {
  var video_id = $("#video_channel").val();
  var audio_type = $("#audio_type").val();
  var delay = $("#vlc_delay").val();
  if (gVlc_current_video_id != video_id
      || gVlc_current_audio_type != audio_type
      || gVlc_current_delay != delay) {
    vlc_play_stop();
    gVlc_current_video_id = video_id;
    gVlc_current_audio_type = audio_type;
    gVlc_current_delay = delay;
    vlc_play_start(video_id, audio_type, delay);
  }
}
