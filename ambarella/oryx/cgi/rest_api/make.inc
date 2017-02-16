##
## oryx/cgi/rest_api/make.inc
##
## History:
##    2015/08/10 - [Huaiqing Wang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)
ORYX_WEB_PATH	:= $(call my-dir)
ORYX_WEB_APPS_PATH  := $(call my-dir)/../web_apps/
include $(CLEAR_VARS)

ifeq ($(BUILD_AMBARELLA_ORYX_CGI), y)
###############################################################################
LOCAL_TARGET := oryx.cgi
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/src/*.cpp)
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include            \
                 -I$(ORYX_DIR)/include/ipc        \
                 -I$(ORYX_DIR)/include/video      \
                 -I$(ORYX_DIR)/include/utility    \
                 -I$(ORYX_DIR)/include/services   \
                 -I$(ORYX_DIR)/services/include   \
                 -I$(ORYX_DIR)/cgi/rest_api/include   \
                 -I$(LOCAL_PATH)                  \
                 -I$(PREBUILD_3RD_PARTY_DIR)/json-c/include/json      \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib  \
                 -L$(PREBUILD_3RD_PARTY_DIR)/json-c/usr/lib -ljson-c
LOCAL_LIBS    := libamutil.so libamipc.so libamapi.so libammedia-data-struct.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/webSvr/web/cgi-bin
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/
	@echo "Build $@ Done."
ifeq ($(BUILD_AMBARELLA_ORYX_WEB_APPS), y)
	@cp -dpRf $(ORYX_WEB_APPS_PATH)/web_page/* $(FAKEROOT_DIR)/webSvr/web
	@echo "Copy $@ Done."
endif

$(call add-target-into-build, $(LOCAL_TARGET))

###############################################################################
endif
