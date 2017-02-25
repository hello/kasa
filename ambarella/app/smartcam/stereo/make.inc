##
##
## History:
##    2015/07/21 - [Zhenwu Xue] Create
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

ifeq ($(BUILD_AMBARELLA_SMARTCAM_APP_STEREO), y)
LOCAL_PATH	:= $(call my-dir)
UTILS_PATH	:= $(LOCAL_PATH)/../utils
OPENCV_PATH	:= $(PREBUILD_3RD_PARTY_DIR)/opencv

###
include $(CLEAR_VARS)

LOCAL_TARGET    :=  test_stereo
LOCAL_SRCS      :=  $(LOCAL_PATH)/test_stereo.cpp $(UTILS_PATH)/arch_$(AMBARELLA_ARCH)/iav.c $(UTILS_PATH)/arch_$(AMBARELLA_ARCH)/fb.c
LOCAL_LIBS      :=  libstereo.a
LOCAL_CFLAGS	+= -I$(AMB_TOPDIR)/packages/smartcam/stereo_lib -I$(OPENCV_PATH)/include -I$(OPENCV_PATH)/include/opencv -I$(UTILS_PATH)
LOCAL_LDFLAGS	:= -L$(OPENCV_PATH)/usr/lib -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio \
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
	@cp -dpRf $(PRIVATE_PATH)/calib.dat $(APP_INSTALL_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET    :=  stereo_calib
LOCAL_SRCS      :=  $(LOCAL_PATH)/stereo_calib.cpp $(UTILS_PATH)/arch_$(AMBARELLA_ARCH)/iav.c
LOCAL_CFLAGS	+= -I$(OPENCV_PATH)/include -I$(OPENCV_PATH)/include/opencv -I$(UTILS_PATH)
LOCAL_LDFLAGS	:= -L$(OPENCV_PATH)/usr/lib -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio \
		 -lopencv_calib3d \
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


