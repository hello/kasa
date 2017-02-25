##
## oryx/services/image_service/make.inc
##
## History:
##    2014/12/31 - [Bin Wang] Created file
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


ifeq ($(BUILD_AMBARELLA_ORYX_IMAGE_QUALITY), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

IQ_VER_FILE = $(ORYX_DIR)/include/image_quality/version.h
IQ_VER_PREFIX = IQ
IQ_CONFIG_FILE := $(ORYX_DIR)/image_quality/etc/iq_config.acs
IQ_TARGET := libamiq.so
LOCAL_TARGET = $(IQ_TARGET)

LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS :=  -I$(ORYX_DIR)/include               \
                 -I$(ORYX_DIR)/include/configure     \
                 -I$(ORYX_DIR)/include/utility       \
                 -I$(ORYX_DIR)/include/ipc           \
                 -I$(ORYX_DIR)/include/image_quality \
                 -I$(ORYX_DIR)/image_quality/include \
                 -I$(LOCAL_PATH)                     \
                 -I$(AMB_TOPDIR)/include             \
                 -std=c++11 -Werror

LOCAL_LDFLAGS := -Wl,-rpath-link=$(FAKEROOT_DIR)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/lua/usr/lib \
                  -lpthread -lrt
LOCAL_LIBS    := libamutil.so libamipc.so libamconfig.so libamp.so libamdev.so \
                 libamosal.so
ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
ifeq ($(CONFIG_IMGPROC_PACKAGE_DYNAMIC_LIB), y)
ifeq ($(CONFIG_ARCH_S2L), y)
LOCAL_LIBS	+= libimg_algo_s2l.so
endif
ifeq ($(CONFIG_ARCH_S2), y)
LOCAL_LIBS	+= libimg_algo_s2.so
endif
ifeq ($(CONFIG_ARCH_S2E), y)
LOCAL_LIBS	+= libimg_algo_s2.so
endif
ifeq ($(CONFIG_ARCH_S3L), y)
LOCAL_LIBS	+= libimg_algo_s3l.so
endif
else
ifeq ($(CONFIG_ARCH_S2L), y)
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif
ifeq ($(CONFIG_ARCH_S2), y)
LOCAL_LIBS	+= libimg_algo_s2.a
endif
ifeq ($(CONFIG_ARCH_S2E), y)
LOCAL_LIBS	+= libimg_algo_s2.a
endif
ifeq ($(CONFIG_ARCH_S3L), y)
LOCAL_LIBS	+= libimg_algo_s3l.a
endif
endif
else
ifeq ($(CONFIG_ARCH_S2L), y)
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif
ifeq ($(CONFIG_ARCH_S2), y)
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2
endif
ifeq ($(CONFIG_ARCH_S2E), y)
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2
endif
ifeq ($(CONFIG_ARCH_S3L), y)
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s3l
endif
endif

IQ_SO_VER_MAJOR  := $(shell awk '/define\ $(IQ_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(IQ_VER_FILE))
IQ_SO_VER_MINOR  := $(shell awk '/define\ $(IQ_VER_PREFIX)_LIB_MINOR/{print $$3}' $(IQ_VER_FILE))
IQ_SO_VER_PATCH  := $(shell awk '/define\ $(IQ_VER_PREFIX)_LIB_PATCH/{print $$3}' $(IQ_VER_FILE))
IQ_SO_VER_STRING := $(IQ_SO_VER_MAJOR).$(IQ_SO_VER_MINOR).$(IQ_SO_VER_PATCH)

IQ_SO_NAME   = $(patsubst %.so,%.so.$(IQ_SO_VER_MAJOR), $(IQ_TARGET))
IQ_SO_TARGET = $(patsubst %.so,%.so.$(IQ_SO_VER_STRING), $(IQ_TARGET))

LOCAL_SO_NAME := $(IQ_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(ORYX_LIB_DIR)/
	@cp -dpRf $< $(ORYX_LIB_DIR)/$(IQ_SO_TARGET)
	@ln -sf $(IQ_SO_TARGET) $(ORYX_LIB_DIR)/$(IQ_SO_NAME)
	@echo "Build $@ Done."
	@mkdir -p $(FAKEROOT_DIR)/$(BUILD_AMBARELLA_ORYX_CONF_DIR)/image
	@cp -dpRf $(IQ_CONFIG_FILE) $(FAKEROOT_DIR)/$(BUILD_AMBARELLA_ORYX_CONF_DIR)/image
	@echo "Copy config script Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

