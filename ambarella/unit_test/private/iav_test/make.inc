##
## unit_test/private/iav_test/make.inc
##
## History:
##    2012/06/05 - [Cao Rongrong] Created file
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

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IAV), y)

LOCAL_PATH	:= $(call my-dir)

ifeq ($(BUILD_AMBARELLA_IAV_AUTO_TEST), y)
include $(call all-subdir-makefiles)
endif

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= guard_sd_space
LOCAL_SRCS	:= $(LOCAL_PATH)/guard_sd_space.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_privacymask
LOCAL_SRCS	:= $(LOCAL_PATH)/test_privacymask.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_encode
LOCAL_SRCS	:= $(LOCAL_PATH)/test_encode.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_frame_sync
LOCAL_SRCS	:= $(LOCAL_PATH)/test_frame_sync.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_efm
LOCAL_SRCS	:= $(LOCAL_PATH)/test_efm.c
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_decode
LOCAL_SRCS	:= $(LOCAL_PATH)/test_decode.c $(LOCAL_PATH)/codec_parser.c $(LOCAL_PATH)/playback_helper.c
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
ifeq ($(BUILD_AMBARELLA_DATATX_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_stream
LOCAL_SRCS	:= $(LOCAL_PATH)/test_stream.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/data_transfer
LOCAL_LDFLAGS	:= -lpthread
LOCAL_LIBS	:= libdatatx.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

include $(CLEAR_VARS)

LOCAL_TARGET    := test_mjpeg_filo
LOCAL_SRCS      := $(LOCAL_PATH)/test_mjpeg_filo.c
LOCAL_CFLAGS    := -I$(AMB_TOPDIR)/packages/data_transfer
LOCAL_LIBS      := libdatatx.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

###
ifeq ($(BUILD_AMBARELLA_BSREADER_PACKAGE), y)
ifeq ($(BUILD_AMBARELLA_DATATX_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_bsreader
LOCAL_SRCS	:= $(LOCAL_PATH)/test_bsreader.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/bsreader \
	-I$(AMB_TOPDIR)/packages/data_transfer
LOCAL_LDFLAGS	:= -lpthread
LOCAL_LIBS	:= libbsreader.a libdatatx.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
endif

###
ifeq ($(BUILD_AMBARELLA_MAINPREPROC_PACKAGE), y)
ifeq ($(BUILD_AMBARELLA_UTILITIES_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_mainpp
LOCAL_SRCS	:= $(LOCAL_PATH)/test_mainpp.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/main_preproc \
                   -I$(AMB_TOPDIR)/packages/utils
LOCAL_LDFLAGS	:= -lrt -lpthread -lm
LOCAL_LIBS	:= libmainpp.so libutils.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
endif


###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_statistics
LOCAL_SRCS	:= $(LOCAL_PATH)/test_statistics.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_dptz
LOCAL_SRCS	:= $(LOCAL_PATH)/test_dptz.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_overlay
LOCAL_SRCS	:= $(LOCAL_PATH)/test_overlay.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_fastosd
LOCAL_SRCS	:= $(LOCAL_PATH)/test_fastosd.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/data/font_index.bin $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/data/font_map.bin $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/data/clut.bin $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_loadbmp
LOCAL_SRCS	:= $(LOCAL_PATH)/test_loadbmp.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/data/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/data/Ambarella-256x128-8bit.bmp $(UNIT_TEST_PATH)/data/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_qproi
LOCAL_SRCS	:= $(LOCAL_PATH)/test_qproi.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))


###
ifeq ($(BUILD_AMBARELLA_DATATX_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_yuvcap
LOCAL_SRCS	:= $(LOCAL_PATH)/test_yuvcap.c  $(LOCAL_PATH)/chrome_convert.S
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/data_transfer
LOCAL_LIBS	:= libdatatx.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###
ifeq ($(CONFIG_AMBARELLA_X265_SUPPORT), y)
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_sw_encoding
LOCAL_SRCS	:= $(LOCAL_PATH)/test_sw_encoding.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/prebuild/third-party/$(AMBARELLA_CPU_ARCH)/x265/include
LOCAL_LDFLAGS	:= -L$(AMB_TOPDIR)/prebuild/third-party/$(AMBARELLA_CPU_ARCH)/x265/usr/lib -lx265 -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_warp
LOCAL_SRCS	:= $(LOCAL_PATH)/test_warp.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= guard_vsync_loss
LOCAL_SRCS	:= $(LOCAL_PATH)/guard_vsync_loss.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_memcpy
LOCAL_SRCS	:= $(LOCAL_PATH)/test_memcpy.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_network_delay
LOCAL_SRCS	:= $(LOCAL_PATH)/test_ndelay/test_network_delay.cpp $(LOCAL_PATH)/test_ndelay/simple_log.cpp $(LOCAL_PATH)/test_ndelay/network_utils.cpp $(LOCAL_PATH)/test_ndelay/simple_queue.cpp
LOCAL_CFLAGS	:= -I$(LOCAL_PATH)/
LOCAL_LDFLAGS	:= -lm -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###

endif
