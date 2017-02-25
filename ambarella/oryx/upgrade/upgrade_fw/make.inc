##
## /S2L/oryx/upgrade/upgrade_fw/make.inc
##
## History:
##   2015-1-8 - [longli] created file
##
## Copyright (c) 2016 Ambarella, Inc.
##
## This file and its contents ("Software") are protected by intellectual
## property rights including, without limitation, U.S. and/or foreign
## copyrights. This Software is also the confidential and proprietary
## information of Ambarella, Inc. and its licensors. You may not use, reproduce,
## disclose, distribute, modify, or otherwise prepare derivative works of this
## Software or any portion thereof except pursuant to a signed license agreement
## or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
## In the absence of such an agreement, you agree to promptly notify and return
## this Software to Ambarella, Inc.
##
## THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
## INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
## MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
## IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
## INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
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
LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/am_upgrade.cpp)

LOCAL_CFLAGS := -I$(ORYX_DIR)/include \
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include/utility/libarchive \
                -I$(ORYX_DIR)/include/configure \
                -I$(ORYX_DIR)/include/upgrade \
                -I$(ORYX_DIR)/upgrade/include \
                -std=c++11 -pthread -Werror

LOCAL_LDFLAGS:= -lpthread

LOCAL_LIBS := libamutil.so \
              libamdownload.so \
              libamconfig.so \
              libamarchive.so

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
LOCAL_SRCS  := $(wildcard $(LOCAL_PATH)/pba_upgrade.cpp)
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include \
                 -I$(ORYX_DIR)/include/configure \
                 -I$(ORYX_DIR)/include/utility   \
                 -I$(ORYX_DIR)/include/utility/libarchive \
                 -I$(AMB_TOPDIR)/packages/security/digital_signature \
                 -I$(AMB_TOPDIR)/packages/security/cryptochip_library \
                 -I$(AMB_TOPDIR)/packages/security/lw_cryptography/include \
                 -std=c++11 -Werror

LOCAL_LDFLAGS:=  -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/lua/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libarchive/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/xz/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib

LOCAL_LIBS := libamconfig.so \
              libamutil.so \
              libamarchive.so \
              libdigisign.a

ifeq ($(CONFIG_DIGITAL_SIGNATURE_LIBRARY_NATIVE), y)
LOCAL_LIBS	+= liblwcryptography.a
endif
ifeq ($(CONFIG_DIGITAL_SIGNATURE_LIBRARY_OPENSSL), y)
LOCAL_LDFLAGS	+= -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -lcrypto -lssl
endif

ifeq ($(SECURE_BOOT), y)
ifeq ($(BUILD_AMBARELLA_CRYPTOCHIP_LIBRARY_FROM_SOURCE), y)
LOCAL_LIBS	:= libcryptochip.a
else
LOCAL_LDFLAGS += -L$(AMB_TOPDIR)/packages/security/cryptochip_library/arch_$(AMBARELLA_ARCH)
endif
LOCAL_LDFLAGS	+= -lcryptochip
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