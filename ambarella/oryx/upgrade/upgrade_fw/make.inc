##
## /S2L/oryx/upgrade/upgrade_fw/make.inc
##
## History:
##   2015-1-8 - [longli] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(BUILD_AMBARELLA_ORYX_UPGRADE), y)
LOCAL_PATH   := $(call my-dir)

#####
include $(CLEAR_VARS)

AMDOWNLOAD_TARGET     = libamdownload.so
AMDOWNLOAD_VER_FILE   = $(ORYX_DIR)/include/upgrade/version.h
AMDOWNLOAD_VER_PREFIX = AMDOWNLOAD

LOCAL_TARGET  := $(AMDOWNLOAD_TARGET)
LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/am_download.cpp)

LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/libcurl/include \
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/upgrade/include \
                -std=c++11 -Werror

LOCAL_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/libcurl/usr/lib -lcurl \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libssh2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libidn/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib

LOCAL_LIBS := libamutil.so

ifeq ($(BUILD_AMBARELLA_ORYX_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

AMDOWNLOAD_SO_VER_MAJOR  := $(shell awk '/define\ $(AMDOWNLOAD_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(AMDOWNLOAD_VER_FILE))
AMDOWNLOAD_SO_VER_MINOR  := $(shell awk '/define\ $(AMDOWNLOAD_VER_PREFIX)_LIB_MINOR/{print $$3}' $(AMDOWNLOAD_VER_FILE))
AMDOWNLOAD_SO_VER_PATCH  := $(shell awk '/define\ $(AMDOWNLOAD_VER_PREFIX)_LIB_PATCH/{print $$3}' $(AMDOWNLOAD_VER_FILE))
AMDOWNLOAD_SO_VER_STRING := $(AMDOWNLOAD_SO_VER_MAJOR).$(AMDOWNLOAD_SO_VER_MINOR).$(AMDOWNLOAD_SO_VER_PATCH)

AMDOWNLOAD_SO_NAME   = $(patsubst %.so,%.so.$(AMDOWNLOAD_SO_VER_MAJOR), $(AMDOWNLOAD_TARGET))
AMDOWNLOAD_SO_TARGET = $(patsubst %.so,%.so.$(AMDOWNLOAD_SO_VER_STRING), $(AMDOWNLOAD_TARGET))

LOCAL_SO_NAME := $(AMDOWNLOAD_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(AMDOWNLOAD_SO_TARGET)
	@ln -sf $(AMDOWNLOAD_SO_TARGET) $(ORYX_LIB_DIR)/$(AMDOWNLOAD_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

#####
include $(CLEAR_VARS)

UPGRADE_TARGET     = libamupgrade.so
UPGRADE_VER_FILE   = $(ORYX_DIR)/include/upgrade/version.h
UPGRADE_VER_PREFIX = UPGRADE

LOCAL_TARGET  := $(UPGRADE_TARGET)
LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/am_upgrade.cpp) \
                 $(wildcard $(LOCAL_PATH)/am_dec7z.cpp)
LOCAL_CFLAGS := -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/configure \
                -I$(ORYX_DIR)/include/upgrade \
                -I$(ORYX_DIR)/upgrade/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/7zdec/include \
                -std=c++11 -pthread -Werror

LOCAL_LDFLAGS:= -L$(PREBUILD_3RD_PARTY_DIR)/7zdec/lib -l7zDec

LOCAL_LIBS := libamutil.so \
              libamdownload.so \
              libamconfig.so

ifeq ($(BUILD_AMBARELLA_ORYX_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

UPGRADE_SO_VER_MAJOR  := $(shell awk '/define\ $(UPGRADE_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(UPGRADE_VER_FILE))
UPGRADE_SO_VER_MINOR  := $(shell awk '/define\ $(UPGRADE_VER_PREFIX)_LIB_MINOR/{print $$3}' $(UPGRADE_VER_FILE))
UPGRADE_SO_VER_PATCH  := $(shell awk '/define\ $(UPGRADE_VER_PREFIX)_LIB_PATCH/{print $$3}' $(UPGRADE_VER_FILE))
UPGRADE_SO_VER_STRING := $(UPGRADE_SO_VER_MAJOR).$(UPGRADE_SO_VER_MINOR).$(UPGRADE_SO_VER_PATCH)

UPGRADE_SO_NAME   = $(patsubst %.so,%.so.$(UPGRADE_SO_VER_MAJOR), $(UPGRADE_TARGET))
UPGRADE_SO_TARGET = $(patsubst %.so,%.so.$(UPGRADE_SO_VER_STRING), $(UPGRADE_TARGET))

LOCAL_SO_NAME := $(UPGRADE_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(UPGRADE_SO_TARGET)
	@ln -sf $(UPGRADE_SO_TARGET) $(ORYX_LIB_DIR)/$(UPGRADE_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

#####
ifeq ($(CONFIG_BACKUP_CPIO), y)
include $(CLEAR_VARS)

LOCAL_TARGET  := pba_upgrade
LOCAL_SRCS  := $(wildcard $(LOCAL_PATH)/pba_upgrade.cpp) \
               $(wildcard $(LOCAL_PATH)/am_dec7z.cpp)
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include \
                 -I$(ORYX_DIR)/include/configure \
                 -I$(ORYX_DIR)/include/utility   \
                 -I$(AMB_TOPDIR)/packages/security/digital_signature \
                 -I$(PREBUILD_3RD_PARTY_DIR)/7zdec/include \
                 -std=c++11 -Werror

LOCAL_LDFLAGS:=  -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/lua/usr/lib \
                 -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -ldigisign -ldl \
                 -L$(PREBUILD_3RD_PARTY_DIR)/7zdec/lib -l7zDec

LOCAL_LIBS := libamconfig.so \
              libamutil.so \
              libdigisign.a

ifeq ($(CONFIG_DIGITAL_SIGNATURE_LIBRARY_NATIVE), y)
LOCAL_LIBS	+= liblwcryptography.a
LOCAL_LDFLAGS	+= -llwcryptography
endif
ifeq ($(CONFIG_DIGITAL_SIGNATURE_LIBRARY_OPENSSL), y)
LOCAL_LDFLAGS	+= -lcrypto -lssl
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

endif