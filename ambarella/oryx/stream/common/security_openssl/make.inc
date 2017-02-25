##
## /S2l/oryx/stream/common/security_openssl/make.inc
##
## History:
##   2016-09-26 - [ccjing] created file
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

LOCAL_PATH := $(call my-dir)

### libamsecurity-openssl.so
include $(CLEAR_VARS)

SECURITY_OPENSSL_TARGET:=libamsecurity-openssl.so
SECURITY_OPENSSL_VER_FILE=$(ORYX_DIR)/stream/include/common/security_openssl/version.h
SECURITY_OPENSSL_VER_PREFIX=SECURITY_OPENSSL

LOCAL_TARGET := $(SECURITY_OPENSSL_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)

LOCAL_CFLAGS := -I$(LOCAL_PATH)                                   \
                -I$(ORYX_DIR)/include                             \
                -I$(ORYX_DIR)/include/utility                     \
                -I$(PREBUILD_3RD_PARTY_DIR)/openssl/include  \
                -I$(ORYX_DIR)/stream/include/common/security_openssl \
                -std=c++11 -Werror

SECURITY_OPENSSL_SO_VER_MAJ := $(shell awk '/define\ $(SECURITY_OPENSSL_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(SECURITY_OPENSSL_VER_FILE))
SECURITY_OPENSSL_SO_VER_MIN := $(shell awk '/define\ $(SECURITY_OPENSSL_VER_PREFIX)_LIB_MINOR/{print $$3}' $(SECURITY_OPENSSL_VER_FILE))
SECURITY_OPENSSL_SO_VER_PAT := $(shell awk '/define\ $(SECURITY_OPENSSL_VER_PREFIX)_LIB_PATCH/{print $$3}' $(SECURITY_OPENSSL_VER_FILE))
SECURITY_OPENSSL_SO_VER_STR := $(SECURITY_OPENSSL_SO_VER_MAJ).$(SECURITY_OPENSSL_SO_VER_MIN).$(SECURITY_OPENSSL_SO_VER_PAT)

SECURITY_OPENSSL_SO_NAME   := $(patsubst %.so,%.so.$(SECURITY_OPENSSL_SO_VER_MAJ), $(SECURITY_OPENSSL_TARGET))
SECURITY_OPENSSL_SO_TARGET := $(patsubst %.so,%.so.$(SECURITY_OPENSSL_SO_VER_STR), $(SECURITY_OPENSSL_TARGET))

LOCAL_LDFLAGS  := -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -lssl -lcrypto \
                 -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib

LOCAL_LIBS := libamutil.so \
              libamfile-sink.so


LOCAL_SO_NAME := $(SECURITY_OPENSSL_SO_NAME)

include $(BUILD_APP)
.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(SECURITY_OPENSSL_SO_TARGET)
	@ln -sf $(SECURITY_OPENSSL_SO_TARGET) $(ORYX_LIB_DIR)/$(SECURITY_OPENSSL_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
