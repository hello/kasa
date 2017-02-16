##
## oryx/services/apps_launcher/make.inc
##
## History:
##    2014/09/09 - [Louis Sun] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##


ifeq ($(BUILD_AMBARELLA_ORYX_SERVICE_APPS_LAUNCHER), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

###############################################################################
LOCAL_TARGET := apps_launcher
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include       \
                 -I$(ORYX_DIR)/include/utility \
                 -I$(ORYX_DIR)/include/ipc    \
                 -I$(ORYX_DIR)/include/services  \
                 -I$(ORYX_DIR)/include/watchdog \
                 -I$(ORYX_DIR)/include/configure  \
                 -I$(ORYX_DIR)/services/include \
                 -I$(LOCAL_PATH)             \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS    := libamutil.so \
                 libamipc.so  \
                 libamapxy.so \
                 libamservice-manager.so \
                 libamconfig.so\
                 libamwatchdog.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)




$(LOCAL_TARGET): $(LOCAL_MODULE) COPY_CONFIG_SCRIPTS
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."
	@mkdir -p $(FAKEROOT_DIR)/etc/oryx/apps
	@cp -dpRf $(ORYX_DIR)/services/apps_launcher/etc/*.acs  $(FAKEROOT_DIR)/etc/oryx/apps
	@echo "Copy Config scripts Done."

$(call add-target-into-build, $(LOCAL_TARGET))



endif
