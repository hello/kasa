##
## unit_test/private/calibration/make.inc
##
## History:
##    2014/06/05 - [HuaiShun Hu] Created file
##
## Copyright (C) 2015 Ambarella, Inc.
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

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= cali_bad_pixel
LOCAL_SRCS	:= $(LOCAL_PATH)/cali_bad_pixel.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
ifeq ($(CONFIG_IMGPROC_PACKAGE_DYNAMIC_LIB), y)
LOCAL_LIBS	+= libimg_algo_s2l.so
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): CALIB_SCRIPTS := bad_pixel_calibration.sh calib_utils.sh
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp $(CALIB_SCRIPTS:%=$(PRIVATE_PATH)/%) $(UNIT_TEST_PATH)/
	@chmod +x $(CALIB_SCRIPTS:%=$(UNIT_TEST_PATH)/%)
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= bitmap_merger
LOCAL_SRCS	:= $(LOCAL_PATH)/bitmap_merger.c
LOCAL_LDFLAGS	:=
LOCAL_LIBS	:=

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= cali_lens_shading
LOCAL_SRCS	:= $(LOCAL_PATH)/cali_lens_shading.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:= libamp.so libamdev.so
ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
ifeq ($(CONFIG_IMGPROC_PACKAGE_DYNAMIC_LIB), y)
LOCAL_LIBS	+= libimg_algo_s2l.so
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): PRIVATE_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): CALIB_SCRIPTS := lens_shading_calibration.sh calib_utils.sh
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp $(CALIB_SCRIPTS:%=$(PRIVATE_PATH)/%) $(UNIT_TEST_PATH)/
	@chmod +x $(CALIB_SCRIPTS:%=$(UNIT_TEST_PATH)/%)
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET  := cali_fisheye_center
LOCAL_SRCS  := $(LOCAL_PATH)/cali_fisheye_center.c $(LOCAL_PATH)/dlist.c
LOCAL_CFLAGS  :=
LOCAL_LIBS  :=
LOCAL_LDFLAGS := -lm

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): PRIVATE_PATH := $(LOCAL_PATH)
$(LOCAL_TARGET): CALIB_SCRIPTS := fish_center_calibration.sh calib_utils.sh
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp $(CALIB_SCRIPTS:%=$(PRIVATE_PATH)/%) $(UNIT_TEST_PATH)/
	@chmod +x $(CALIB_SCRIPTS:%=$(UNIT_TEST_PATH)/%)
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
##TODO: pending for piris driver ready
#include $(CLEAR_VARS)
#
#SCRIPT_PIRIS	:= $(LOCAL_PATH)/piris_calibration.sh
#LOCAL_TARGET	:= cali_piris
#LOCAL_SRCS	:= $(LOCAL_PATH)/cali_piris.c
#LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/utils
#LOCAL_LDFLAGS := -lrt -lm -lpthread
#LOCAL_LIBS	:= libutils.so
#LOCAL_LIBS 	+= libamp.so
#ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
#ifeq ($(CONFIG_IMGPROC_PACKAGE_DYNAMIC_LIB), y)
#LOCAL_LIBS	+= libimg_algo_s2l.so
#else
#LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
#endif
#else
#LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
#endif
#
#include $(BUILD_APP)
#
#.PHONY: $(LOCAL_TARGET)
#
#$(LOCAL_TARGET): $(LOCAL_MODULE)
#	@mkdir -p $(UNIT_TEST_PATH)/
#	@cp $(SCRIPT_PIRIS) $(UNIT_TEST_PATH)/
#	@cp -dpRf $< $(UNIT_TEST_PATH)/
#	@echo "Build $@ Done."
#
#$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= cali_lens_focus
LOCAL_SRCS	:= $(LOCAL_PATH)/cali_lens_focus.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
ifeq ($(CONFIG_IMGPROC_PACKAGE_DYNAMIC_LIB), y)
LOCAL_LIBS	+= libimg_algo_s2l.so
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif

LOCAL_SRCS	+= $(AMB_TOPDIR)/unit_test/private/mw_test/arch_$(AMBARELLA_ARCH)/fb_image.c
LOCAL_CFLAGS += -I$(AMB_TOPDIR)/unit_test/private/mw_test/arch_$(AMBARELLA_ARCH)/


ifeq ($(BUILD_AMBARELLA_TEXTINSERT_PACKAGE), y)
LOCAL_CFLAGS += -I$(AMB_TOPDIR)/packages/textinsert
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib -lfreetype \
                 -L$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz            \
                 -L$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib -lbz2
LOCAL_LIBS += libtextinsert.a
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET    := cali_awb
LOCAL_SRCS      := $(LOCAL_PATH)/cali_awb.c \
	$(AMB_TOPDIR)/unit_test/private/idsp_test/arch_$(AMBARELLA_ARCH)/idsp_netlink.c

LOCAL_CFLAGS    := \
	-I$(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/adj_params \
	-I$(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/aeb_params \
	-I$(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/lens_params \
	-I$(AMB_TOPDIR)/unit_test/private/idsp_test/arch_$(AMBARELLA_ARCH)

LOCAL_LDFLAGS   := -lpthread -lm
LOCAL_LIBS      :=
ifeq ($(BUILD_AMBARELLA_PACKAGES_PROPRIETARY_SOURCES), y)
ifeq ($(CONFIG_IMGPROC_PACKAGE_DYNAMIC_LIB), y)
LOCAL_LIBS	+= libimg_algo_s2l.so
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif
else
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2l
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH  := $(LOCAL_PATH)
$(LOCAL_TARGET): CALIB_SCRIPTS := awb_calibration.sh calib_utils.sh
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp $(CALIB_SCRIPTS:%=$(PRIVATE_PATH)/%) $(UNIT_TEST_PATH)/
	@chmod +x $(CALIB_SCRIPTS:%=$(UNIT_TEST_PATH)/%)
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
