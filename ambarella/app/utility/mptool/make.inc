##
## app/utility/mptool/make.inc
##
## History:
##   2015-3-18 - [longli] created file
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

ifeq ($(BUILD_AMBARELLA_ORYX_MPTOOL), y)
LOCAL_PATH	:= $(call my-dir)

###

include $(CLEAR_VARS)

LOCAL_BOARD	:= $(shell grep CONFIG_BSP_BOARD $(DOT_CONFIG) | \
        grep -v '^\# CONFIG_BSP_BOARD' | sed -e s/^CONFIG_BSP_BOARD_// | \
        sed -e s/=y//)
LOCAL_BSP_DIR	:= $(AMB_TOPDIR)/boards/$(shell echo $(LOCAL_BOARD) | \
        tr [:upper:] [:lower:])/bsp

LOCAL_TARGET    :=  mptool
LOCAL_SRCS      :=  $(wildcard $(LOCAL_PATH)/src/*.c)
LOCAL_CFLAGS :=  -I$(AMB_TOPDIR)/include \
                 -I$(AMB_TOPDIR)/include/arch_$(AMBARELLA_ARCH) \
                 -I$(LOCAL_PATH)/include \
                 -I$(LOCAL_BSP_DIR) \
                 -Werror

LOCAL_LDFLAGS   := -lpthread -lm
LOCAL_LIBS  := libamp.so libamdev.so
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

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/local/bin
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/local/bin
	@cp -dpRf $(PRIVATE_PATH)/data/* $(FAKEROOT_DIR)/usr/local/bin
	@echo "Build $@ Done."


$(call add-target-into-build, $(LOCAL_TARGET))

endif
