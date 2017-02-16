function adjust_log_size(obj) {
  $(obj).height($("#header").height()*0.9);
}

function construct_user_data(data) {
  var txt=" ";
  var key;
  for (key in data) {
    txt = txt + key + ": " + data[key] + "</br>";
  }
  $("#success_dialog").html(txt).dialog("open");
}

var gEncode_stream_state = 0;
function set_stream_control_init_state() {
  $.get("/oryx/video/enc_ctrl/stream/?action=get", datatype="json",  function(result) {
    if (result.msg_code == 0) {
      var data = result.data;
      var key;
      for (key in data) {
        if (data[key] == "encoding") {
          $("#stream_state_control").button( "option", "label", "Stop" );
          gEncode_stream_state = 1;
          break;
        }
      }
    }
  });
}

function stream_state_control() {
  if (gEncode_stream_state == 1) {
    $.get("/oryx/video/enc_ctrl/stream/?action=disable", datatype="json",  function(result) {
      if (result.msg_code != 0) {
        $("#err_dialog").text(result.msg).dialog("open");
      } else {
        $("#stream_state_control").button( "option", "label", "Start" );
        gEncode_stream_state = 0;
      }
    });
  } else {
    $.get("/oryx/video/enc_ctrl/stream/?action=enable", datatype="json",  function(result) {
      if (result.msg_code != 0) {
        $("#err_dialog").text(result.msg).dialog("open");
      } else {
        $("#stream_state_control").button( "option", "label", "Stop" );
        gEncode_stream_state = 1;
      }
    });
  }
}

function get_stream_status() {
  $.get("/oryx/video/enc_ctrl/stream/?action=get", datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
    } else {
      construct_user_data(result.data);
    }
  });
}

function force_idr() {
  $.get("/oryx/video/enc_ctrl/force_idr/stream"+$("#forceidr_stream_id").val(),  datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
    }
  });
}

var gOverlay_type = 2;
var gOverlay_available_fonts = new Array;
var gOverlay_available_bmps = new Array;
function overlay_rotate() {
  if ($("#overlay_rotate").val() == 1) {
    $("#overlay_rotate").val(0);
    $("#overlay_rotate").button( "option", "label", "false" );
  } else {
    $("#overlay_rotate").val(1);
    $("#overlay_rotate").button( "option", "label", "true" );
  }
}

function overlay_state() {
  if ($("#overlay_state").val() == "enable") {
    $("#overlay_state").val("disable");
    $("#overlay_state").button( "option", "label", "disable" );
  } else {
    $("#overlay_state").val("enable");
    $("#overlay_state").button( "option", "label", "enable" );
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
}

function show_overlay_time() {
  gOverlay_type = 1;
  show_overlay_text();
  $("#overlay_string").hide();
}

function show_overlay_picture() {
  gOverlay_type = 2;
  $("#overlay_text").hide();
  $("#overlay_area_size").hide();
  $("#overlay_picture").show();
}

function show_overlay_test() {
  gOverlay_type = 3;
  $("#overlay_picture").hide();
  $("#overlay_text").hide();
  $("#overlay_area_size").show();
}

function hexFromRGB(r, g, b) {
  var hex = [
    r.toString( 16 ),
    g.toString( 16 ),
    b.toString( 16 )
  ];
  $.each( hex, function( nr, val ) {
    if ( val.length === 1 ) {
      hex[ nr ] = "0" + val;
    }
  });
  return hex.join( "" ).toUpperCase();
}

function refresh_font_color_swatch() {
  var red = $( "#overlay_font_r" ).slider( "value" );
  var green = $( "#overlay_font_g" ).slider( "value" );
  var blue = $( "#overlay_font_b" ).slider( "value" );
  var hex = hexFromRGB( red, green, blue );
  $( "#font_color_swatch" ).css( "background-color", "#" + hex );
}

function overlay_set_autocomplete_fonts_source() {
  $("#overlay_font_type").autocomplete({source: function( request, response ) {
    var matcher = new RegExp( $.ui.autocomplete.escapeRegex( request.term )+"*", "i" );
    response( $.grep( gOverlay_available_fonts, function( item ){
      return matcher.test( item );
    }) );},
  });
}

function get_overlay_available_fonts_from_remote() {
  $.get("/oryx/video/overlay/font?action=get",  datatype="json",  function(result) {
    if (result.msg_code == 0) {
      gOverlay_available_fonts = result.data;
      overlay_set_autocomplete_fonts_source();
    }
  });
};

function overlay_set_autocomplete_bmps_source() {
  $("#overlay_picture_file").autocomplete({source: function( request, response ) {
    var matcher = new RegExp( $.ui.autocomplete.escapeRegex( request.term )+"*", "i" );
    response( $.grep( gOverlay_available_bmps, function( item ){
      return matcher.test( item );
    }) );},
  });
}

function get_overlay_available_bmps_from_remote() {
  $.get("/oryx/video/overlay/bmp?action=get",  datatype="json",  function(result) {
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

function get_overlay_Json_data() {
  var json = {
    "state": $("#overlay_state").val(),
    "rotate": $("#overlay_rotate").val()==1?true:false,
    "type": gOverlay_type,
    "layout": $("#overlay_layout_select").val(),
    "position_x": $("#overlay_area_startx").val(),
    "position_y": $("#overlay_area_starty").val(),
  };
  var yuv = new Array;
  var rgb = new Array;
  if ( gOverlay_type == 2) {
    rgb[0] = $("#overlay_bmp_color_r").val();
    rgb[1] = $("#overlay_bmp_color_g").val();
    rgb[2] = $("#overlay_bmp_color_b").val();
    var a = $("#overlay_bmp_color_a").val();
    color_space_transform(yuv, rgb);
    json.color_key = yuv[2]<<24|yuv[1]<<16|yuv[0]<<8|a;
    json.color_range = $("#overlay_bmp_color_range").val();
    if ($("#overlay_picture_file").val() != "") {
      json.bmp_file = $("#overlay_picture_file").val();
    }
  } else {
    json.width = $("#overlay_area_width").val();
    json.height = $("#overlay_area_height").val();
    if (gOverlay_type != 3) {
      rgb[0] = $( "#overlay_font_r" ).slider( "value" );
      rgb[1] = $( "#overlay_font_g" ).slider( "value" );
      rgb[2] = $( "#overlay_font_b" ).slider( "value" );
      color_space_transform(yuv, rgb);
      json.font_color = yuv[2]<<24|yuv[1]<<16|yuv[0]<<8|255;
      if ($("#overlay_font_type").val() != "") {
        json.font_type = $("#overlay_font_type").val();
      }
      json.font_size = $("#overlay_font_size").val();
      json.font_outwidth = $("#overlay_font_outwidth").val();
      json.font_horbold = $("#overlay_font_horbold").val();
      json.font_verbold = $("#overlay_font_verbold").val();
      json.font_italic = $("#overlay_font_italic").val();
      if ((gOverlay_type == 0) && ($("#overlay_string_text").val() != "")) {
        json.text_str = $("#overlay_string_text").val();
      }
    }
  }
  return json;
}

function add_overlay() {
  var stream_id = $("#overlay_add_stream_id").val();
  $.ajax({url:"/oryx/video/overlay/stream"+stream_id+"?action=set",
    type:"POST",  contentType:"application/json",
    datatype:"json", data:JSON.stringify(get_overlay_Json_data()),
    success:function(data, textStatus) {
      if (data.msg_code != 0) {
        $("#err_dialog").text(data.msg).dialog("open");
      }
    },
  });
}

function overlay_manipulate() {
  var stream_id = $("#overlay_stream_id").val();
  var area_id = $("#overlay_area_id").val();
  var action = $("#overlay_manipulate_select").val().toLowerCase();
  $.get("/oryx/video/overlay/stream"+stream_id+"/area"+area_id+"?action="+action,
      datatype="json",  function(result) {
        if (result.msg_code != 0) {
          $("#err_dialog").text(result.msg).dialog("open");
        } else if (action == "get"){
          if (result.data["layout"] == 0) {
            result.data["layout"] = "left top";
          } else if (result.data["layout"] == 1) {
            result.data["layout"] = "right top";
          } else if (result.data["layout"] == 2) {
            result.data["layout"] = "left bottom";
          } else if (result.data["layout"] == 3) {
            result.data["layout"] = "right bottom";
          } else if (result.data["layout"] == 4) {
            result.data["layout"] = "custom";
          }
          if (result.data["type"] == 0) {
            result.data["type"] = "string";
          } else if (result.data["type"] == 1) {
            result.data["type"] = "time";
          } else if (result.data["type"] == 2) {
            result.data["type"] = "picture";
          } else if (result.data["type"] == 3) {
            result.data["type"] = "test mode";
          }
          construct_user_data(result.data);
        }
      });
}

function destroy_overlay() {
  $.get("/oryx/video/overlay/stream4?action=delete",  datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
    }
  });
}

function save_overlay_config() {
  $.get("/oryx/video/overlay?action=save",  datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
    }
  });
}

var gZoom_val = new Array();
gZoom_val = [1,1,1,1];
var gPan_val = new Array();
gPan_val = [0,0,0,0];
var gTilt_val = new Array();
gTilt_val = [0,0,0,0];
var gBuffer_id = 0;
var gLdc_strength = 0;
function set_dptz_buffer() {
  gBuffer_id = $("#dptz_buffer_id").val();
  $("#zoom").val(gZoom_val[gBuffer_id]);
  $( "#zoom_slider" ).slider( "value", gZoom_val[gBuffer_id]);
  $("#pan").val(gPan_val[gBuffer_id]);
  $( "#pan_slider" ).slider( "value", gPan_val[gBuffer_id]);
  $("#tilt").val(gTilt_val[gBuffer_id]);
  $( "#tilt_slider" ).slider( "value", gTilt_val[gBuffer_id]);
}

function set_dptz_zoom(value) {
  $.get("/oryx/video/dptz/buffer"+gBuffer_id+"?action=set&zoom_ratio="+value,  datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
      $("#zoom").val(gZoom_val[gBuffer_id]);
      $( "#zoom_slider" ).slider( "value", gZoom_val[gBuffer_id]);
    } else {
      gZoom_val[gBuffer_id] = value;
    }
  });
}

function set_dptz_pan(value) {
  $.get("/oryx/video/dptz/buffer"+gBuffer_id+"?action=set&pan_ratio="+value,  datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
      $("#pan").val(gPan_val[gBuffer_id]);
      $( "#pan_slider" ).slider( "value", gPan_val[gBuffer_id]);
    } else {
      gPan_val[gBuffer_id] = value;
    }
  });
}

function set_dptz_tilt(value) {
  $.get("/oryx/video/dptz/buffer"+gBuffer_id+"?action=set&tilt_ratio="+value,  datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
      $("#tilt").val(gTilt_val[gBuffer_id]);
      $( "#tilt_slider" ).slider( "value", gTilt_val[gBuffer_id]);
    } else {
      gTilt_val[gBuffer_id] = value;
    }
  });
}

function set_dewarp(value) {
  $.get("/oryx/video/dewarp?action=set&ldc_strength="+value,  datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
      $("#strength").val(gLdc_strength);
      $( "#strength_slider" ).slider( "value", gLdc_strength);
    } else {
      gLdc_strength = value;
    }
  });
}

function media_start_recording() {
  $.get("/oryx/media/recording?action=start",  datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
    }
  });
}

function media_stop_recording() {
  $.get("/oryx/media/recording?action=stop",  datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
    }
  });
}

function media_file_recording_start() {
  $.get("/oryx/media/recording/file?action=start",  datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
    }
  });
}

function media_file_recording_stop() {
  $.get("/oryx/media/recording/file?action=stop",  datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
    }
  });
}

function media_event_recording() {
  $.get("/oryx/media/recording/event?action=start",  datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
    }
  });
}

var gPlayback_audio_add_times=0;
var gPlayback_audio_state = 0;
function media_playback_add_audio_file() {
  ++gPlayback_audio_add_times;
  var content = '<div style="margin-bottom:5px;">Path&nbsp<input id="media_playback_audio_file'+gPlayback_audio_add_times+  '"  type="text"  class=ui-corner-all  style="float:center;height:30px;width:350px" value="" />';
  content += '&nbsp<button  onclick="media_playback_del_audio_file(this)" class=ui-corner-all style="height:35px;width:35px">-</button></div>';
  $("#media_playback_audio_file").append(content);
}

function media_playback_del_audio_file(obj) {
  --gPlayback_audio_add_times;
  $(obj).parent().remove();
}

function media_palyback_audio_add() {
  $.ajax({url:"/oryx/media/playback/audio?action=set",
    type:"POST",  contentType:"application/json",
    datatype:"json", data:JSON.stringify(get_audio_playback_Json_data()),
    success:function(data, textStatus) {
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
  for (var i=0; i<=gPlayback_audio_add_times; ++i) {
    var file_id = "media_playback_audio_file"+i;
    if ($("#"+file_id).length > 0) {
      if ($("#"+file_id).val() != "") {
        json["file"+m] = $("#"+file_id).val();
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
    $.get("/oryx/media/playback/audio?action=start",  datatype="json",  function(result) {
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
    $.get("/oryx/media/playback/audio?action=pause",  datatype="json",  function(result) {
      if (result.msg_code != 0) {
        $("#err_dialog").text(result.msg).dialog("open");
      } else {
        gPlayback_audio_state = 1;
      }
    });
  }
}

function media_palyback_audio_stop() {
  $.get("/oryx/media/playback/audio?action=stop",  datatype="json",  function(result) {
    if (result.msg_code != 0) {
      $("#err_dialog").text(result.msg).dialog("open");
    } else {
      gPlayback_audio_state = 0;
    }
  });
}
