##
## prebuild/third-party/armv7-a-hf/alsa-utils/make.inc
##
## History:
##    2012/10/17 - [Yupeng Chang] Created file
##
## Copyright (c) 2015 Ambarella, Inc.
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

ifeq ($(CONFIG_AMBARELLA_ALSA_PLUGINS_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_alsa-plugins
LOCAL_SRCS	:= usr

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_ALSA_PLUGINS_PULSEAUDIO_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/alsa-lib/libasound_module_conf_pulse.so
	@rm -rf $(FAKEROOT_DIR)/usr/lib/alsa-lib/libasound_module_ctl_pulse.so
	@rm -rf $(FAKEROOT_DIR)/usr/lib/alsa-lib/libasound_module_pcm_pulse.so
	@rm -rf $(FAKEROOT_DIR)/usr/share/alsa/alsa.conf.d/50-pulseaudio.conf
	@rm -rf $(FAKEROOT_DIR)/usr/share/alsa/alsa.conf.d/99-pulseaudio-default.conf
endif
ifneq ($(CONFIG_AMBARELLA_ALSA_PLUGINS_LIBSAMPLERATE_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/alsa-lib/libasound_module_rate_samplerate_best.so
	@rm -rf $(FAKEROOT_DIR)/usr/lib/alsa-lib/libasound_module_rate_samplerate_linear.so
	@rm -rf $(FAKEROOT_DIR)/usr/lib/alsa-lib/libasound_module_rate_samplerate_medium.so
	@rm -rf $(FAKEROOT_DIR)/usr/lib/alsa-lib/libasound_module_rate_samplerate_order.so
	@rm -rf $(FAKEROOT_DIR)/usr/lib/alsa-lib/libasound_module_rate_samplerate.so
	@rm -rf $(FAKEROOT_DIR)/usr/share/alsa/alsa.conf.d/10-samplerate.conf
endif
ifneq ($(CONFIG_AMBARELLA_ALSA_PLUGINS_SPEEX_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/alsa-lib/libasound_module_pcm_speex.so
	@rm -rf $(FAKEROOT_DIR)/usr/lib/alsa-lib/libasound_module_rate_speexrate_best.so
	@rm -rf $(FAKEROOT_DIR)/usr/lib/alsa-lib/libasound_module_rate_speexrate_medium.so
	@rm -rf $(FAKEROOT_DIR)/usr/lib/alsa-lib/libasound_module_rate_speexrate.so
	@rm -rf $(FAKEROOT_DIR)/usr/share/alsa/alsa.conf.d/10-speex.conf
endif
	@[ -e $(FAKEROOT_DIR)/usr/share/alsa/alsa.conf.d ] && chmod 644 $(FAKEROOT_DIR)/usr/share/alsa/alsa.conf.d/*
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

