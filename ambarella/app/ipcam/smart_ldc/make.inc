##
##
## History:
##    2012/06/25 - [Jian Tang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##
ifeq ($(BUILD_AMBARELLA_APP_IPCAM_SMART_LDC), y)

LOCAL_PATH	:= $(call my-dir)
WEB_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= smart_ldc
LOCAL_SRCS	:= $(LOCAL_PATH)/smart_ldc.c $(LOCAL_PATH)/chrome_convert.S
LOCAL_CFLAGS	:= -I$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/include
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/usr/lib -lpthread -lm -ljpeg

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@mkdir -p $(FAKEROOT_DIR)/webSvr/web/
	@cp -dpRf $(WEB_PATH)/web/css $(WEB_PATH)/web/js $(WEB_PATH)/web/index.html $(FAKEROOT_DIR)/webSvr/web
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= ldc.cgi
LOCAL_SRCS	:= $(LOCAL_PATH)/web/cgi-bin/ldc.c
LOCAL_CFLAGS	:= -I$(PREBUILD_3RD_PARTY_DIR)/clearsilver/include/ClearSilver
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/clearsilver/usr/lib -lneo_cgi -lneo_cs -lneo_utl -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/webSvr/web/cgi-bin/
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= get_vin.cgi
LOCAL_SRCS	:= $(LOCAL_PATH)/web/cgi-bin/get_vin.c
LOCAL_CFLAGS	:= -I$(PREBUILD_3RD_PARTY_DIR)/clearsilver/include/ClearSilver
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/clearsilver/usr/lib -lneo_cgi -lneo_cs -lneo_utl -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/webSvr/web/cgi-bin/
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
endif

