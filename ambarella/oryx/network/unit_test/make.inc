##
## camera/test/network/make.inc
##
## History:
##    2015-1-12 - [Tao Wu] created file
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

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_TARGET := test_network_manager
LOCAL_SRCS   := $(LOCAL_PATH)/test_network_manager.cpp
LOCAL_LIBS   := libamutil.so libamnetwork.so libamnetwork-dev.so
LOCAL_CFLAGS := -I$(ORYX_DIR)/include\
                -I$(ORYX_DIR)/include/utility \
                -I$(ORYX_DIR)/include/network \
                -std=c++11 -Werror

LOCAL_LDFLAGS:= -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/NetworkManager/usr/lib \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/expat/usr/lib        \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/dbus/usr/lib         \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib         \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/ncurses/usr/lib      \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/glib2/usr/lib        \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libffi/usr/lib       \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/util-linux/usr/lib   \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/gnutls/usr/lib       \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libgcrypt/usr/lib    \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libgpg-error/usr/lib \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/nettle3/usr/lib      \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libidn/usr/lib       \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/gmp/usr/lib          \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libtasn1/usr/lib     \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/udev/usr/lib         \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/elfutils/usr/lib     \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib        \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libselinux/lib       \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libsepol/lib         \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/xz/usr/lib           \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/pam/usr/lib          \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libcap/usr/lib       \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libattr/usr/lib      \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/pcre/usr/lib         \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/readline/usr/lib     \
                -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/dbus-glib/usr/lib

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_BIN_DIR)/
	@cp -dpRf $< $(ORYX_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

