##
## app/ipcam/fastboot_app/wowl/make.inc
##
## History:
##    2015/04/03 - [Tao Wu] Created file
##
## Copyright (C) 2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_APP_WIFI_UTILITY), y)

WOWL_PATH   := $(call my-dir)

WOWL_APP_DIR = $(FAKEROOT_DIR)/usr/bin
WOWL_LIB_DIR = $(FAKEROOT_DIR)/usr/lib

#################################

ifeq ($(BUILD_AMBARELLA_APP_WIFI_LIBRARY_BRCM), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= libbrcmioc.so
LOCAL_SRCS	:= $(WOWL_PATH)/brcm/brcm_ioc.c
LOCAL_CFLAGS 	:= -I$(WOWL_PATH)/inc \
		-I$(WOWL_PATH)/brcm/inc

LOCAL_SO_NAME := $(LOCAL_TARGET)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(WOWL_LIB_DIR)/
	@cp -dpRf $< $(WOWL_LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif ## BUILD_AMBARELLA_APP_WIFI_LIBRARY_BRCM

#################################

ifeq ($(BUILD_AMBARELLA_APP_WIFI_LIBRARY_MRVL), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= libmrvlioc.so
LOCAL_SRCS	:= $(WOWL_PATH)/mrvl/mrvl_ioc.c
LOCAL_CFLAGS 	:= -I$(WOWL_PATH)/inc \
		-I$(WOWL_PATH)/mrvl/inc

LOCAL_SO_NAME := $(LOCAL_TARGET)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(WOWL_LIB_DIR)/
	@cp -dpRf $< $(WOWL_LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif ## BUILD_AMBARELLA_APP_WIFI_LIBRARY_MRVL

#################################

ifeq ($(BUILD_AMBARELLA_APP_WIFI_WOWL), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= wowl
LOCAL_CFLAGS 	:= -I$(WOWL_PATH)/inc \
		-I$(PREBUILD_3RD_PARTY_DIR)/libpcap/include

ifeq ($(BUILD_AMBARELLA_APP_WIFI_WOWL_BRCM), y)
LOCAL_SRCS	:= $(WOWL_PATH)/wowl.c
LOCAL_LIBS   	:= libbrcmioc.so
LOCAL_CFLAGS 	+= -I$(WOWL_PATH)/brcm/inc
else ifeq ($(BUILD_AMBARELLA_APP_WIFI_WOWL_MRVL), y)
LOCAL_SRCS	:= $(WOWL_PATH)/wowl_mrvl.c
LOCAL_LIBS   	:= libmrvlioc.so
LOCAL_CFLAGS 	+= -I$(WOWL_PATH)/mrvl/inc
endif

LOCAL_LDFLAGS 	:= -L$(PREBUILD_3RD_PARTY_DIR)/libpcap/usr/lib -lpcap \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libnl/usr/lib \
		-Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/dbus/usr/lib \
		-lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(WOWL_APP_DIR)/
	@cp -dpRf $< $(WOWL_APP_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif ## BUILD_AMBARELLA_APP_WIFI_WOWL

endif ## BUILD_AMBARELLA_APP_WIFI_UTILITY
