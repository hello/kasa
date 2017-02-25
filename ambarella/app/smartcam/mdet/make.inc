##
## unit_test/common.mk
##
## History:
##    2014/03/28 - [Zhenwu Xue] Create
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

ifeq ($(BUILD_AMBARELLA_SMARTCAM_APP_MDET), y)
LOCAL_PATH	:= $(call my-dir)
OPENCV_PATH	:= $(PREBUILD_3RD_PARTY_DIR)/opencv
UTILS_PATH	:= $(LOCAL_PATH)/../utils

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_mdet
LOCAL_SRCS	:= $(LOCAL_PATH)/test_mdet.c $(UTILS_PATH)/arch_$(AMBARELLA_ARCH)/iav.c $(UTILS_PATH)/arch_$(AMBARELLA_ARCH)/fb.c
LOCAL_LDFLAGS	:= -lm
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/ambarella/library/smartcam/mdet/lib/arch_$(AMBARELLA_ARCH) -lmdet
LOCAL_CFLAGS	+= -I$(AMB_TOPDIR)/prebuild/ambarella/library/smartcam/mdet/include/ -I$(UTILS_PATH)
LOCAL_LDFLAGS	+= -Wl,-rpath-link=$(OPENCV_PATH)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib
LOCAL_LDFLAGS	+= -lstdc++

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(APP_INSTALL_PATH)/
	@cp -dpRf $< $(APP_INSTALL_PATH)/
	@chmod +x $(APP_INSTALL_PATH)/$@
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_cdet
LOCAL_SRCS	:= $(LOCAL_PATH)/test_cdet.cpp
LOCAL_LDFLAGS	:= -lm
ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
LOCAL_LIBS	:= libmdet.so
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/ambarella/library/smartcam/mdet/lib/arch_$(AMBARELLA_ARCH) -lmdet
endif
LOCAL_CFLAGS	+= -I$(AMB_TOPDIR)/prebuild/ambarella/library/smartcam/mdet/include/ -I$(UTILS_PATH)
LOCAL_CFLAGS	+= -I$(OPENCV_PATH)/include -I$(OPENCV_PATH)/include/opencv
LOCAL_LDFLAGS	+= -L$(OPENCV_PATH)/usr/lib -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio \
                 -Wl,-rpath-link=$(OPENCV_PATH)/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libjpeg-turbo/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/libpng/usr/lib \
                 -Wl,-rpath-link=$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(APP_INSTALL_PATH)/
	@cp -dpRf $< $(APP_INSTALL_PATH)/
	@chmod +x $(APP_INSTALL_PATH)/$@
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif


