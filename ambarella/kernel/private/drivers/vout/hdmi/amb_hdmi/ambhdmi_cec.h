/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/ambhdmi_cec.h
 *
 * History:
 *    2009/06/05 - [Zhenwu Xue] Initial revision
 *
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
 */



/* ========================================================================== */

typedef enum {
	CEC_DEV_TV			= 0,
	CEC_DEV_RECORDING1		= 1,
	CEC_DEV_RECORDING2		= 2,
	CEC_DEV_STB1			= 3,
	CEC_DEV_DVD1			= 4,
	CEC_DEV_AUDIO_SYS		= 5,
	CEC_DEV_STB2			= 6,
	CEC_DEV_STB3			= 7,
	CEC_DEV_DVD2			= 8,
	CEC_DEV_RECORDING3		= 9,
	CEC_DEV_RESERVED10		= 10,
	CEC_DEV_RESERVED11		= 11,
	CEC_DEV_RESERVED12		= 12,
	CEC_DEV_RESERVED13		= 13,
	CEC_DEV_FREE_USE		= 14,
	CEC_DEV_UNREGISTERED		= 15,
	CEC_DEV_BROADCAST		= 15
} amba_hdmi_cec_logical_address_t;

typedef enum {
	CEC_ACTIVE_SOURCE    		= 0x82,
	CEC_IMAGE_VIEW_ON		= 0x04,
	CEC_TEXT_VIEW_ON       		= 0x0d,
	CEC_REQUET_ACTIVE_SOURCE       	= 0x85,
	CEC_ROUTING_CHANGE       	= 0x80,
	CEC_ROUTING_INFORMATION      	= 0x81,
	CEC_SET_STREAM_PATH      	= 0x86,
	CEC_STANDBY                     = 0x36,
	CEC_RECORD_OFF                  = 0x0b,
	CEC_RECORD_ON                  	= 0x09,
	CEC_RECORD_STATUS              	= 0x0a,
 	CEC_RECORD_TV_SCREEN           	= 0x0f,
	CEC_GIVE_PHYSICAL_ADDR         	= 0x83,
	CEC_GET_MENU_LANGUAGE         	= 0x91,
	CEC_REPORT_PHYSICAL_ADDR        = 0x84,
	CEC_SET_MENU_LANGUAGE		= 0x32,
	CEC_DECK_CONTROL                = 0x42,
	CEC_DECK_STATUS                 = 0x1b,
	CEC_GIVE_DECK_STATUS            = 0x1a,
	CEC_PLAY            		= 0x41,
	CEC_GIVE_TUNER_DEVICE_STATUS	= 0x08,
	CEC_SELECT_DIGITAL_SERVICE	= 0x93,
	CEC_TUNER_DEVICE_STATUS		= 0x07,
	CEC_TUNER_STEP_DECREMENT	= 0x06,
	CEC_TUNER_STEP_INCREMENT	= 0x05,
	CEC_DEVICE_VENDOR_ID            = 0x87,
	CEC_GIVE_DEVICE_VENDOR_ID       = 0x8c,
	CEC_VENDOR_COMMAND              = 0x89,
	CEC_VENDOR_REMOTE_BUTTON_DOWN   = 0x8a,
	CEC_VENDOR_REMOTE_BUTTON_UP   	= 0x8b,
	CEC_SET_OSD_STRING              = 0x64,
	CEC_GIVE_OSD_NAME               = 0x46,
	CEC_SET_OSD_NAME                = 0x47,
	CEC_MENU_REQUEST	        = 0x8d,
	CEC_MENU_STATUS                 = 0x8e,
	CEC_USER_CONTROL_PRESSED        = 0x44,
	CEC_USER_CONTROL_RELEASED       = 0x45,
	CEC_GIVE_DEVICE_POWER_STATUS    = 0x8f,
	CEC_REPORT_POWER_STATUS         = 0x90,
	CEC_FEATURE_ABORT               = 0x00,
	CEC_ABORT                       = 0xff
} amba_hdmi_cec_opcode_t;

typedef struct {
        amba_hdmi_cec_opcode_t	opcode;
	u8 			para_size;
	char			name[25];
	u8			addr_mode;
#define DIRECT_ADDR_MODE	0x01
#define BOARDCAST		0x02
	u8			need_respond;
#define NO			0x00
#define YES			0x01
#define NA			0x02
	u8			respond_opcode;	/* it is for rx_messae use, if needed */
} amba_hdmi_cec_opcode_table_t;

const amba_hdmi_cec_opcode_table_t amba_hdmi_cec_opcode_table[] = {
	{CEC_ACTIVE_SOURCE,		0x02,   "Active Soure",
	(BOARDCAST),			NO,	0			},
	{CEC_IMAGE_VIEW_ON,		0x00,   "Image View On",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_TEXT_VIEW_ON,       	0x00,   "Text View On",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_REQUET_ACTIVE_SOURCE,	0x00,   "Request Active Source",
	(BOARDCAST),			YES,	CEC_ACTIVE_SOURCE	},
	{CEC_ROUTING_CHANGE,       	0x04,   "Routing Channge",
	(BOARDCAST),			NA,	0			},
	{CEC_ROUTING_INFORMATION,      	0x02,   "Rounting Information",
	(BOARDCAST),			NA,	0			},
	{CEC_SET_STREAM_PATH,      	0x02,   "Set Stream Path",
	(BOARDCAST),			NA,	0			},
	{CEC_STANDBY,			0x00,   "Standby",
	(DIRECT_ADDR_MODE | BOARDCAST),	NO,	0			},
	{CEC_RECORD_OFF,		0x00,   "Record Off",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_RECORD_ON,                 0x01,   "Record On",
	(DIRECT_ADDR_MODE),		YES,	CEC_RECORD_STATUS	},	/* 0x01 or 0x08 */
	{CEC_RECORD_STATUS,		0x01,   "Record Status",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_RECORD_TV_SCREEN,		0x00,   "Record TV Screen",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_GIVE_PHYSICAL_ADDR,	0x00,   "Give Physical Addr",
	(DIRECT_ADDR_MODE),		YES,	CEC_REPORT_PHYSICAL_ADDR},
	{CEC_GET_MENU_LANGUAGE,        	0x00,   "Get Menu Language",
	(DIRECT_ADDR_MODE),		YES,	CEC_SET_MENU_LANGUAGE	},
	{CEC_REPORT_PHYSICAL_ADDR,	0x03,   "Report Physical Addr",
	(BOARDCAST),			NO,	0			},
	{CEC_SET_MENU_LANGUAGE,		0x03,   "Set Menu Language",
	(BOARDCAST),			NO,	0			},
	{CEC_DECK_CONTROL,		0x01,   "Deck Control",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_DECK_STATUS,		0x01,   "Deck Status",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_GIVE_DECK_STATUS,		0x01,   "Give Deck Status",
	(DIRECT_ADDR_MODE),		YES,	CEC_DECK_STATUS		},
	{CEC_PLAY,			0x01,   "Play",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_GIVE_TUNER_DEVICE_STATUS,	0x01,   "Give Tuner Device Status",
	(DIRECT_ADDR_MODE),		YES,	CEC_TUNER_DEVICE_STATUS	},
	{CEC_SELECT_DIGITAL_SERVICE,	0x07,   "Select Digital Service",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_TUNER_DEVICE_STATUS,	0x08,   "Tuner Device Status",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_TUNER_STEP_DECREMENT,	0x00,   "Tuner Step Decrement",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_TUNER_STEP_INCREMENT,	0x00,   "Tuner Step Increment",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_DEVICE_VENDOR_ID,		0x03,   "Device Vendor ID",
	(BOARDCAST),			NO,	0			},
	{CEC_GIVE_DEVICE_VENDOR_ID,	0x00,   "Give Device Vendor ID",
	(DIRECT_ADDR_MODE),		YES,	CEC_DEVICE_VENDOR_ID	},
	{CEC_VENDOR_COMMAND,		0x00,   "Vendor Command",
	(DIRECT_ADDR_MODE),		NA,	0			},	/* shall not exceed 14 ??? */
	{CEC_VENDOR_REMOTE_BUTTON_DOWN,	0x00,   "Vendor Remote Btn Down",
	(DIRECT_ADDR_MODE),		NA,	0			},	/* ??? */
	{CEC_VENDOR_REMOTE_BUTTON_UP,	0x00,   "Vendor Remote Btn Up",
	(DIRECT_ADDR_MODE),		NA,	0			},
	{CEC_SET_OSD_STRING,		0x0c,   "Set OSD String",
	(DIRECT_ADDR_MODE),		NA,	0			},	/* 1 to 13 */
	{CEC_GIVE_OSD_NAME,		0x00,   "Give OSD name",
	(DIRECT_ADDR_MODE),		YES,	CEC_SET_OSD_NAME	},
	{CEC_SET_OSD_NAME,		0x08,   "Set OSD name",
	(DIRECT_ADDR_MODE),		NO,	0			},	/* 1 to 8 */
	{CEC_MENU_REQUEST,		0x01,   "Menu Request",
	(DIRECT_ADDR_MODE),		YES,	CEC_MENU_STATUS		},
	{CEC_MENU_STATUS,		0x01,   "Menu Status",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_USER_CONTROL_PRESSED,	0x01,   "User Control Pressed",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_USER_CONTROL_RELEASED,	0x00,   "User Control Released",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_GIVE_DEVICE_POWER_STATUS,	0x00,   "Give Device Power Status",
	(DIRECT_ADDR_MODE),		YES,	CEC_REPORT_POWER_STATUS	},
	{CEC_REPORT_POWER_STATUS,	0x01,   "Report Power Status",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_FEATURE_ABORT,		0x02,   "Feature Abort",
	(DIRECT_ADDR_MODE),		NO,	0			},
	{CEC_ABORT,    			0x00,   "Abort",
	(DIRECT_ADDR_MODE),		YES,	CEC_FEATURE_ABORT	}
};

amba_hdmi_cec_logical_address_t
	ambhdmi_cec_allocate_logical_address(struct ambhdmi_sink *phdmi_sink);
int ambhdmi_cec_report_physical_address(struct ambhdmi_sink *phdmi_sink);
void ambhdmi_cec_receive_message(struct ambhdmi_sink *phdmi_sink);

