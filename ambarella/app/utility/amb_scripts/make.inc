##
## app/utility/amb_scripts/make.inc
##
## History:
##    2014/06/04 - [Cao Rongrong] Create
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

ifeq ($(BUILD_AMBAREALLA_UTILITY_SCRIPTS), y)

LOCAL_PATH	:= $(call my-dir)

include $(CLEAR_VARS)

.PHONY: add_amb_scripts

LOCAL_SRCS	:=

ifeq ($(BUILD_AMBAREALLA_UTILITY_SCRIPTS_ALL), y)
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/*.sh)
LOCAL_SRCS	+= $(LOCAL_PATH)/dsp $(LOCAL_PATH)/pll
else

ifeq ($(BUILD_AMBAREALLA_UTILITY_SCRIPTS_BCSTINIT), y)
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/bc_init.sh)
endif
ifeq ($(BUILD_AMBAREALLA_UTILITY_SCRIPTS_INITIAV), y)
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/init.sh)
endif
ifeq ($(BUILD_AMBAREALLA_UTILITY_SCRIPTS_SHMOO), y)
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/shmoo.sh)
endif
ifeq ($(BUILD_AMBAREALLA_UTILITY_SCRIPTS_AOSS), y)
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/aoss.sh)
endif

endif
PRIVATE_SRCS	:= $(LOCAL_SRCS)
add_amb_scripts: $(LOCAL_SRCS)
	@mkdir -p $(FAKEROOT_DIR)/usr/local/bin/
	@cp -dpRf $(PRIVATE_SRCS) $(FAKEROOT_DIR)/usr/local/bin/
	@chmod +x $(FAKEROOT_DIR)/usr/local/bin/*.sh

$(call add-target-into-build, add_amb_scripts)

endif

