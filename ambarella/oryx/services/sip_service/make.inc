##
## oryx/services/sip_service/make.inc
##
## History:
##    2015/01/29 - [Shiming Dong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##


ifeq ($(BUILD_AMBARELLA_ORYX_SIP_SERVICE), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

SIP_SVC_NAME = sip_svc
###############################################################################
LOCAL_TARGET := $(SIP_SVC_NAME)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/am_sip_ua_server_main.cpp  \
                $(LOCAL_PATH)/am_sip_ua_server_msg_action.cpp)
LOCAL_CFLAGS :=  -I$(LOCAL_PATH)                         \
                 -I$(ORYX_DIR)/include                   \
                 -I$(ORYX_DIR)/include/ipc               \
                 -I$(ORYX_DIR)/include/utility           \
                 -I$(ORYX_DIR)/include/protocol          \
                 -I$(ORYX_DIR)/include/services          \
                 -I$(ORYX_DIR)/include/services/commands \
                 -I$(ORYX_DIR)/services/include          \
                 -std=c++11 -Werror

LOCAL_LIBS    := libamutil.so          \
                 libamipc.so           \
                 libamservice-frame.so \
                 libamprotocol-sip.so

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libeXosip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libosip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@mkdir -p $(ORYX_CONF_DIR)/apps/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

### libamsip-data-struct.so
include $(CLEAR_VARS)

SIP_DATA_STRUCT = libamsip-data-struct.so

LOCAL_TARGET := $(SIP_DATA_STRUCT)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/am_sip_ua_server_data_structure.cpp)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include                 \
                -I$(ORYX_DIR)/include/protocol        \
                -I$(ORYX_DIR)/include/utility         \
                -I$(ORYX_DIR)/include/services        \
                -I$(ORYX_DIR)/include/service/commands\
                -I$(ORYX_DIR)/include/ipc             \
                -std=c++11 -Werror

LOCAL_LIBS := libamprotocol-sip.so     \
              libamutil.so

SIP_DATA_STRUCT_NAME   = $(SIP_DATA_STRUCT)
SIP_DATA_STRUCT_TARGET = $(SIP_DATA_STRUCT)
LOCAL_SO_NAME := $(SIP_DATA_STRUCT)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(SIP_DATA_STRUCT_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
