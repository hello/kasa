function EncParam()
{
	this.mem = new Array(
		['encodetype', 0],
		['fps', 0],
		['resolution', 0],
		['fliprotate', 0],
		['h264_n', 0],
		['idr_interval', 0],
		['profile', 0],
		['cbr_avg_bps', 0],
		['mjpeg_quality', 0]
	);
	this.count = this.mem.length;
}
function DwpParam()
{
	this.mem = new Array(
		['mount', 0],
		['vendor', 0],
		['maxfov', 0],
		['radium', 0],
		['type', 0],
		['layout', 0]
	);
	this.count = this.mem.length;
}
function PMParam()
{
	this.mem = new Array(
		['pm_left', 0],
		['pm_top', 0],
		['pm_w', 0],
		['pm_h', 0]
	);
	this.count = this.mem.length;
}
function OSDParam()
{
	this.mem = new Array(
		['osd_stream', 0],
		['time_enable', 0],
		['bmp_enable', 0],
		['text_enable', 0],
		['text', 0],
		['font_size', 0],
		['font_color', 0],
		['outline', 0],
		['bold', 0],
		['italic', 0],
		['offset_x', 0],
		['offset_y', 0],
		['box_w', 0],
		['box_h', 0]
	);
	this.count = this.mem.length;
}
function IQParam()
{
	this.mem = new Array(
		['dnmode', 0],
		['filter', 0],
		['exposure', 0],
		['compensation', 0],
		['exptargetfac', 0],
		['irismode', 0],
		['antiflicker', 0],
		['maxgain', 0],
		['saturation', 0],
		['brightness', 0],
		['contrast', 0],
		['sharpness', 0],
		['shuttermin', 0],
		['shuttermax', 0],
		['wb', 0]
	);
	this.count = this.mem.length;
}
var g_EncParam = new EncParam();
var g_DwpParam = new DwpParam();
var g_PMParam = new PMParam();
var g_OSDParam = new OSDParam();
var g_IQParam = new IQParam();
var zoom_count = 0;
var zoom_count_fish = 15;
var wbval;
var streamid = location.href.substr(location.href.indexOf("stream")+7,1);
var version = new Array();
function getData(pageName, detail)
{
	switch (pageName) {
		case 'enc':
			get_page_data(g_EncParam);
			break;
		case 'dwp':
			get_page_data(g_DwpParam);
			break;
		case 'pm':
			get_page_data(g_PMParam);
			break;
		case 'osd':
			get_page_data(g_OSDParam);
			break;
		case 'iq':
			get_page_data(g_IQParam,12);
			g_IQParam.mem[12][1] = parseInt($("#shuttermin span").text().slice(2));
			g_IQParam.mem[13][1] = parseInt($("#shuttermax span").text().slice(2));
			g_IQParam.mem[14][1] = wbval;
			break;
		default:
			break;
	}
}
function setData(pageName, detail)
{
	switch (pageName) {
		case 'enc':
			set_page_data(g_EncParam);
			break;
		case 'dwp':
			set_page_data(g_DwpParam);
			var layout = g_DwpParam.mem[5][1];
			$("input[name='special']").get(layout).checked = true;
			break;
		case 'osd':
			set_page_data(g_OSDParam);
			break;
		case 'iq':
			set_page_data(g_OSDParam, 12);
			$("#shuttermin span").text(g_IQParam.mem[12][1]);
			$("#shuttermax span").text(g_IQParam.mem[13][1]);
			$("#wb span").text(g_IQParam.mem[14][1]);
			break;
		default:
			break;
	}
}
function get_page_data(Param, detail)
{
	var count = detail?detail:Param.count;
	for(var i = 0; i < count; i++) {
		var ParamName = Param.mem[i][0];
		var obj = $("input[id^='" + ParamName + "']");
		if (obj != undefined) {
			$.each(obj, function() {
				switch(this.type) {
				case "radio":
					if(this.checked){
						Param.mem[i][1] = this.value;
					}
					break;
				case "spinner":
					Param.mem[i][1] = this.value;
					break;
				case "text":
					Param.mem[i][1] = this.value;
					break;
				case "checkbox":
					if(this.checked){
						Param.mem[i][1] = this.value;
					}
					break;
				default:
					alert("Unknow type is " + this.type + ".\n");
					break;
				}
			})
		}
	}
}
function set_page_data(Param, detail)
{
	var count = detail?detail:Param.count;
	for(var i = 0; i < count; i++) {
		var ParamName = Param.mem[i][0];
		var obj = $("input[id^='" + ParamName + "']");
		if (obj != undefined) {
			$.each(obj, function() {
				switch(this.type) {
				case "radio":
					if(Param.mem[i][1] == this.value){
						$("#"+this.id).attr("checked",'checked');
						$("#"+this.id).next("label").addClass( "ui-state-active");
					} else {
						$("#"+this.id).attr("checked",'');
						$("#"+this.id).next("label").removeClass( "ui-state-active");
					}
					break;
				case "spinner":
					this.value = Param.mem[i][1];
					break;
				case "text":
					this.value = Param.mem[i][1];
					break;
				case "checkbox":
					if(Param.mem[i][1] == 1){
						$("#"+this.id).attr("checked",'checked');
					}
					break;
				default:
					alert("Unknow type is " + this.type + ".\n");
					break;
				}
			})
		}
	}
}

function runEffect() {
	$( "#alert" ).show( "blind", 500, callback);
	function callback() {
		setTimeout(function() {
		$( "#alert:visible" ).fadeOut();
		}, 2000 );
	};
};
function setEnc() {
	try {
		var oXmlDom = document.implementation.createDocument("","doc",null);
        var oRoot = oXmlDom.documentElement;
		var pvnode = document.createElement("encoders");
        oRoot = pvnode;
		var xs = new XMLSerializer();

		var new_EncParam = new EncParam();
		get_page_data(new_EncParam);
		var changed = 0;
		for (i = 0; i < 2; ++i) {
			if (new_EncParam.mem[i][1] != g_EncParam.mem[i][1]) {
				var vnode = document.createElement(new_EncParam.mem[i][0]);
				vnode.setAttribute("ul", new_EncParam.mem[i][1]);
				oRoot.appendChild(vnode);
				changed = changed+1;
			}
		}
		if ((new_EncParam.mem[2][1] != g_EncParam.mem[2][1]) ||
		(new_EncParam.mem[3][1] != g_EncParam.mem[3][1])) {
			var vnode = document.createElement(new_EncParam.mem[2][0]);
			vnode.setAttribute("ul", new_EncParam.mem[2][1]);
			oRoot.appendChild(vnode);
			changed = changed+1;
		}
		for (i = 3; i < g_EncParam.count; ++i) {
			if (new_EncParam.mem[i][1] != g_EncParam.mem[i][1]) {
				var vnode = document.createElement(new_EncParam.mem[i][0]);
				vnode.setAttribute("ul", new_EncParam.mem[i][1]);
				oRoot.appendChild(vnode);
				changed = changed+1;
			}
		}
		var xml = xs.serializeToString( oRoot );
		xml = "<?xml version='1.0'?>" + xml;
		//alert(xml);
		if(changed == 0) {
			runEffect();
		} else {
			$.ajax({
				url: '/cgi-bin/router.cgi?page=enc&action=update&stream='+ streamid,
				datatype: 'xml',
				type: 'put',
				data: xml,
				beforeSend: function() {
					$( "#processing" ).show( "blind", 500);
				},
				success: function(xml) {
					var res = $(xml).find('res').attr("ul");
					if(res == "0") {
						$( "#processing:visible" ).fadeOut();
						$( "#success" ).dialog( "open" );
						g_EncParam = new_EncParam;
					} else {
						$( "#processing:visible" ).fadeOut();
						$( "#fail" ).dialog( "open" );
					}
				},
				error: function() {
					$( "#processing:visible" ).fadeOut();
					$( "#fail" ).dialog( "open" );
				},
			});
		}
	} catch(error) {
		alert(error);
	}
}
function setDwp() {
	try {
		if (CheckVal() < 0) {
			return null;
		}
		var oXmlDom = document.implementation.createDocument("","doc",null);
        var oRoot = oXmlDom.documentElement;
		var pvnode = document.createElement("dewarp");
        oRoot = pvnode;
		var xs = new XMLSerializer();

		var new_DwpParam = new DwpParam();
		get_page_data(new_DwpParam);
		if(new_DwpParam.mem[1][1] == 0) {
			new_DwpParam.mem[4][1] = 0;
		} else if (new_DwpParam.mem[1][1] == 1) {
			new_DwpParam.mem[4][1] = 0;
		} else if (new_DwpParam.mem[1][1] == 2) {
			new_DwpParam.mem[4][1] = 0;
		}
		var changed = 0;
		for (i = 0; i < g_DwpParam.count; ++i) {
			var vnode = document.createElement(new_DwpParam.mem[i][0]);
			vnode.setAttribute("ul", new_DwpParam.mem[i][1]);
			oRoot.appendChild(vnode);
			if (new_DwpParam.mem[i][1] != g_DwpParam.mem[i][1]) {
				changed = changed+1;
			}
		}
		var xml = xs.serializeToString( oRoot );
		xml = "<?xml version='1.0'?>" + xml;
		//alert(xml);
		if(changed == 0) {
			runEffect();
		} else{
			$.ajax({
				url: '/cgi-bin/router.cgi?page=dwp&action=update',
				datatype: 'xml',
				type: 'put',
				data: xml,
				beforeSend: function() {
					$( "#processing" ).show( "blind", 500);
				},
				success: function(xml) {
					var res = $(xml).find('res').attr("ul");
					if(res == "0") {
						$( "#processing:visible" ).fadeOut();
						$( "#success" ).dialog( "open" );
						g_DwpParam = new_DwpParam;
					} else {
						$( "#processing:visible" ).fadeOut();
						$( "#fail" ).dialog( "open" );
					}
				},
				error: function() {
					$( "#processing:visible" ).fadeOut();
					$( "#fail" ).dialog( "open" );
				},
			});
		}
	} catch(error) {
		alert(error);
	}
	function CheckVal() {
		if ($("#vendor4").is(':checked')) {
			var maxfov = $( "#maxfov" ).spinner();
			var radium = $( "#radium" ).spinner();
			if(maxfov.spinner( "value" ) == null || radium.spinner( "value" ) == null) {
				$( "#check" ).show( "blind", 500, callback);
				return -1;
			}
			if(!$("#type1").next("label").hasClass( "ui-state-active") && !$("#type2").next("label").hasClass( "ui-state-active")) {
				$( "#check" ).show( "blind", 500, callback);
				return -1;
			}
		}
		if ($("#mount1").is(':checked')) {
			if($("#layout1").is(':checked') == false && $("#layout2").is(':checked') == false && $("#layout3").is(':checked') == false) {
				$( "#check2" ).show( "blind", 500, callback2);
				return -1;
			}
		}
		function callback() {
			setTimeout(function() {
			$( "#check:visible" ).fadeOut();
			}, 1500 );
		};
		function callback2() {
			setTimeout(function() {
			$( "#check2:visible" ).fadeOut();
			}, 1500 );
		};
		return 0;
	};
}

function setPM(id,action) {
	var ret=false;
	try {
		var oXmlDom = document.implementation.createDocument("","doc",null);
        var oRoot = oXmlDom.documentElement;
		var pvnode = document.createElement("privacymask");
        oRoot = pvnode;
		var xs = new XMLSerializer();

		var vnode = document.createElement("pm_action");
		vnode.setAttribute("ul", action);
		oRoot.appendChild(vnode);

		var vnode = document.createElement("pm_id");
		vnode.setAttribute("ul", id);
		oRoot.appendChild(vnode)

		var new_PMParam = new PMParam();
		get_page_data(new_PMParam);
		for (i = 0; i < g_PMParam.count; ++i) {
			var vnode = document.createElement(new_PMParam.mem[i][0]);
			vnode.setAttribute("ul", new_PMParam.mem[i][1]);
			oRoot.appendChild(vnode);
		}
		var xml = xs.serializeToString( oRoot );
		xml = "<?xml version='1.0'?>" + xml;
		//alert(xml);
		$.ajax({
			url: '/cgi-bin/router.cgi?page=pm&action=update',
			datatype: 'xml',
			type: 'put',
			async: false,
			data: xml,
			beforeSend: function() {
				$( "#processing" ).show( "blind", 500);
			},
			success: function(xml) {
				var res = $(xml).find('res').attr("ul");
				if(res == "0") {
					$( "#processing:visible" ).fadeOut();
					$( "#success" ).dialog( "open" );
					g_PMParam = new_PMParam;
					ret = true;
				} else {
					$( "#processing:visible" ).fadeOut();
					$( "#fail" ).dialog( "open" );
				}
			},
			error: function() {
				$( "#processing:visible" ).fadeOut();
				$( "#fail" ).dialog( "open" );
			},
		});
	} catch(error) {
		alert(error);
	}
	return ret;
}
function setOSD() {
	try {
		var oXmlDom = document.implementation.createDocument("","doc",null);
        var oRoot = oXmlDom.documentElement;
		var pvnode = document.createElement("osd");
        oRoot = pvnode;
		var xs = new XMLSerializer();
		var validtext = 0;

		var new_OSDParam = new OSDParam();
		get_page_data(new_OSDParam);
		var changed = 0;
		for (i = 0; i < 4; ++i) {
			var vnode = document.createElement(new_OSDParam.mem[i][0]);
			vnode.setAttribute("ul", new_OSDParam.mem[i][1]);
			oRoot.appendChild(vnode);
			if (new_OSDParam.mem[i][1] != g_OSDParam.mem[i][1]) {
				changed = changed+1;
			}
		}
		if(new_OSDParam.mem[3][1] == 1) {
			if((new_OSDParam.mem[4][1])  && (new_OSDParam.mem[12][1] != 0) && (new_OSDParam.mem[13][1] != 0)) {
				validtext = 1;
				for (i = 4; i < g_OSDParam.count; ++i) {
				var vnode = document.createElement(new_OSDParam.mem[i][0]);
				vnode.setAttribute("ul", new_OSDParam.mem[i][1]);
				oRoot.appendChild(vnode);
				if (new_OSDParam.mem[i][1] != g_OSDParam.mem[i][1]) {
					changed = changed+1;
				}
				}
			}
		} else if (new_OSDParam.mem[3][1] == 0) {
			validtext = 1;
		}
		var xml = xs.serializeToString( oRoot );
		xml = "<?xml version='1.0'?>" + xml;
		//alert(xml);
		if(changed == 0) {
			runEffect();
		} else {
			if(validtext == 1) {
			$.ajax({
				url: '/cgi-bin/router.cgi?page=osd&action=update',
				datatype: 'xml',
				type: 'put',
				data: xml,
				beforeSend: function() {
					$( "#processing" ).show( "blind", 500);
				},
				success: function(xml) {
					var res = $(xml).find('res').attr("ul");
					if(res == "0") {
						$( "#processing:visible" ).fadeOut();
						$( "#success" ).dialog( "open" );
						g_OSDParam = new_OSDParam;
					} else {
						$( "#processing:visible" ).fadeOut();
						$( "#fail" ).dialog( "open" );
					}
				},
				error: function() {
					$( "#processing:visible" ).fadeOut();
					$( "#fail" ).dialog( "open" );
				},
			});
			}
		}
	} catch(error) {
		alert(error);
	}
}
function setIQ() {
	try {
		var new_IQParam = new IQParam();
		if(CheckValid()) {
			get_page_data(new_IQParam,12);
			new_IQParam.mem[12][1] = parseInt($("#shuttermin span").text().slice(2));;
			new_IQParam.mem[13][1] = parseInt($("#shuttermax span").text().slice(2));;
			new_IQParam.mem[14][1] = wbval;
			PackSend();
		} else {
			$( "#processing" ).find("em").text("Contain invalid parameters!").end().show( "blind", 500, setTimeout(function() {
			$( "#processing:visible" ).fadeOut();
			}, 1500 )
			);
		}
		function CheckValid() {
			var checkmem = ['filter','saturation','brightness','contrast','sharpness', 'exptargetfac'],
			i = checkmem.length-1,
			range = [[0,11],[0,255],[-255,255],[0,128],[0,10],[25,400]],
			tmp;
			do{
				tmp = $("input[id^='" + checkmem[i] + "']").val();
				if( tmp >= range[i][0] && tmp <= range[i][1]) {
					i--;
					continue;
				} else {
					break;
				}
			} while(i >= 0);
			if(i < 0) {
				return true;
			} else {
				return false;
			}
		}
		function PackSend() {
			var i = new_IQParam.count-1,
			//senddata = "{",
			length,
			oXmlDom = document.implementation.createDocument("","doc",null),
			oRoot,
			pvnode,
			xs;
			oRoot = oXmlDom.documentElement;
			pvnode = document.createElement("imagequality");
			oRoot = pvnode;
			xs = new XMLSerializer();
			/*do{
				senddata = senddata +'"'+new_IQParam.mem[i][0]+'":'+new_IQParam.mem[i][1]+',';
				i--;
			} while(i > 0);
			senddata = senddata + '"' + new_IQParam.mem[i][0] + '":' + new_IQParam.mem[i][1] + '}';*/
			while(i >= 0) {
				var vnode = document.createElement(new_IQParam.mem[i][0]);
				vnode.setAttribute("ul", new_IQParam.mem[i][1]);
				oRoot.appendChild(vnode);
				i--;
			}
			var xml = xs.serializeToString( oRoot );
			xml = "<?xml version='1.0'?>" + xml;
			//alert(xml);
			$.ajax({
				url: '/cgi-bin/router.cgi?page=iq&action=update',
				datatype: 'json',
				type: 'put',
				data: xml,
				beforeSend: function() {
					$( "#processing" ).show( "blind", 500);
				},
				success: function(json) {
					if(json.res == "0") {
						$( "#processing:visible" ).fadeOut();
						$( "#success" ).dialog( "open" );
						g_IQParam = new_IQParam;
					} else {
						$( "#processing:visible" ).fadeOut();
						$( "#fail" ).dialog( "open" );
					}
				},
				error: function() {
					$( "#processing:visible" ).fadeOut();
					$( "#fail" ).dialog( "open" );
				},
			});
		}
	} catch(error) {
		alert(error);
	}
}
function LivestreamRst() {
	try {
		zoom_count = 0;
		var xml = "<?xml version='1.0'?><liveview><dptz><action z='0' x='0' y='0'/></dptz></liveview>";
		//alert(xml);
		$.ajax({
			url: '/cgi-bin/router.cgi?page=live&action=update&stream='+ streamid,
			datatype: 'xml',
			type: 'put',
			data: xml,
		});
	} catch(error) {
		alert(error);
	}
}
function LivestreamZin(offset_x, offset_y) {
	try {
		zoom_count = zoom_count + 1;
		var xml = "<?xml version='1.0'?><liveview><dptz><action z='" + zoom_count +
		"' x='0'" +
		" y='0'" +
		" /></dptz></liveview>";
		//alert(xml);
		$.ajax({
			url: '/cgi-bin/router.cgi?page=live&action=update&stream='+ streamid,
			datatype: 'xml',
			type: 'put',
			async: false,
			data: xml,
		});
	} catch(error) {
		alert(error);
	}
}
function LivestreamZout(offset_x, offset_y) {
	try {
		if(zoom_count > 0) {
			zoom_count = zoom_count -1;
			var xml = "<?xml version='1.0'?><liveview><dptz><action z='" + zoom_count +
			"' x='0'" +
			" y='0'" +
			"/></dptz></liveview>";
			//alert(xml);
			$.ajax({
				url: '/cgi-bin/router.cgi?page=live&action=update&stream='+ streamid,
				datatype: 'xml',
				type: 'put',
				async: false,
				data: xml,
			});
		}
	} catch(error) {
		alert(error);
	}
}
function LivestreamTrans(offset_x, offset_y) {
	try {
		if(zoom_count >= 0) {
		var xml = "<?xml version='1.0'?><liveview><dptz><action z='" + zoom_count +
		"' x='" + offset_x +
		"' y='" + offset_y +
		"'/></dptz></liveview>";
		//alert(xml);
		$.ajax({
			url: '/cgi-bin/router.cgi?page=live&action=update&stream='+ streamid,
			datatype: 'xml',
			type: 'put',
			async: false,
			data: xml,
		});
		}
	} catch(error) {
		alert(error);
	}
}
function FishStreamTrans(pan, tilt) {
	try {
		if(zoom_count_fish > 9) {
		var xml = "<?xml version='1.0'?><fishcam><dptz><action z='" + zoom_count_fish +
		"' p='" + pan +
		"' t='" + tilt +
		"'/></dptz></fishcam>";
		//alert(xml);
		$.ajax({
			url: '/cgi-bin/router.cgi?page=live&action=update&stream='+ streamid,
			datatype: 'xml',
			type: 'put',
			async: false,
			data: xml,
		});
		}
	} catch(error) {
		alert(error);
	}
}
function FishStreamZin(pan, tilt) {
	try {
		if(zoom_count_fish < 21) {
			zoom_count_fish = zoom_count_fish + 1;
			var xml = "<?xml version='1.0'?><fishcam><dptz><action z='" + zoom_count_fish +
			"' p='" + pan +
			"' t='" + tilt +
			" '/></dptz></fishcam>";
			//alert(xml);
			$.ajax({
				url: '/cgi-bin/router.cgi?page=live&action=update&stream='+ streamid,
				datatype: 'xml',
				type: 'put',
				async: false,
				data: xml,
			});
		}
	} catch(error) {
		alert(error);
	}
}
function FishStreamZout(pan, tilt) {
	try {
		if(zoom_count_fish > 9) {
			zoom_count_fish = zoom_count_fish -1;
			var xml = "<?xml version='1.0'?><fishcam><dptz><action z='" + zoom_count_fish +
			"' p='" + pan +
			"' t='" + tilt +
			"'/></dptz></fishcam>";
			//alert(xml);
			$.ajax({
				url: '/cgi-bin/router.cgi?page=live&action=update&stream='+ streamid,
				datatype: 'xml',
				type: 'put',
				async: false,
				data: xml,
			});
		}
	} catch(error) {
		alert(error);
	}
}
function FishStreamRst() {
	try {
		zoom_count_fish = 15;
		var xml = "<?xml version='1.0'?><fishcam><dptz><action z='15' p='0' t='0'/></dptz></fishcam>";
		//alert(xml);
		$.ajax({
			url: '/cgi-bin/router.cgi?page=live&action=update&stream='+ streamid,
			datatype: 'xml',
			type: 'put',
			async: false,
			data: xml,
		});
	} catch(error) {
		alert(error);
	}
}
function getVlcVersion() {
	if (navigator.plugins && navigator.plugins.length) {
		for (var x = 0; x < navigator.plugins.length; x++) {
			if (navigator.plugins[x].name.indexOf("VLC")!= -1) {
				for (var ver = highestVer;ver > -1; ver--){
				if (navigator.plugins[x].description.indexOf(" "+ver+".") != -1) {
					var i = navigator.plugins[x].description.indexOf(ver+".");
					if (navigator.plugins[x].description.split("")[i] == ver)
						version[0] = ver;
					if (navigator.plugins[x].description.split("")[i+1] == ".")
						version[1] = parseFloat(navigator.plugins[x].description.split("")[i+2]);
					if (navigator.plugins[x].description.split("")[i+3] == ".")
						version[2] = parseFloat(navigator.plugins[x].description.split("")[i+4]);
					if (navigator.plugins[x].description.split("")[i+5] == ".")
						version[3] = parseFloat(navigator.plugins[x].description.split("")[i+6]);
					else version[3] = 0;
					break;
				 }
				}
				return true;
			}
		}
	}
	return false;
}
function judgeVlc() {
	if(getVlcVersion()){
		if (version[0] < 2 )
			if(confirm("Vlc version is too low to play video stablely, you might want to update to latest version. Note:with Mozilla plugin.")){
				 window.open("http://www.videolan.org/vlc/","","");
			}
	}
	else{
		if(confirm("Please install vlc(with Mozilla plugin) first.")){
			window.open("http://www.videolan.org/vlc/","","");
		}
	}
}