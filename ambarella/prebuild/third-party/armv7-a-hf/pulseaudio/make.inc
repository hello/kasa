##
## prebuild/third-party/armv7-a-hf/pulseaudio/make.inc
##
## History:
##    2012/05/30 - [Cao Rongrong] Created file
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

ifeq ($(CONFIG_AMBARELLA_PULSEAUDIO_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

PA_MAJOR = $(shell awk '/define\ PA_MAJOR/{print $$3}' $(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include/pulse/version.h)
PA_MINOR = $(shell awk '/define\ PA_MINOR/{print $$3}' $(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include/pulse/version.h)
PULSE_VERSION = $(PA_MAJOR).$(PA_MINOR)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_pulseaudio
LOCAL_SRCS	:= etc usr
.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE) prebuild_alsa
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_UTILS_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/bin/pacat
	@rm -rf $(FAKEROOT_DIR)/usr/bin/pacmd
	@rm -rf $(FAKEROOT_DIR)/usr/bin/pactl
	@rm -rf $(FAKEROOT_DIR)/usr/bin/pamon
	@rm -rf $(FAKEROOT_DIR)/usr/bin/paplay
	@rm -rf $(FAKEROOT_DIR)/usr/bin/parec
	@rm -rf $(FAKEROOT_DIR)/usr/bin/parecord
	@rm -rf $(FAKEROOT_DIR)/usr/bin/pasuspender
endif
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_SIMPLE_LIBRARY), y)
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libpulse-simple.so.0.1.0
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libpulse-simple.so.0
	@rm -rf $(FAKEROOT_DIR)/usr/lib/libpulse-simple.so
endif
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_AEC_SUPPORT), y)
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-echo-cancel.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/libwebrtc-util.so
endif
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_SIMPLE_PROTOCOL), y)
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-simple-protocol-*
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/libprotocol-simple.so
endif
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_CLI_PROTOCOL), y)
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/libprotocol-cli.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-cli.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-cli-protocol-*
endif
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_HTTP_PROTOCOL), y)
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-http-protocol-*
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/libprotocol-http.so
endif
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_RTP_PROTOCOL), y)
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-rtp-*
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/librtp.so
endif
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_TUNNEL_SUPPORT), y)
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-tunnel-*
endif
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_COMBINE_SUPPORT), y)
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-combine-*
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-combine.so
endif
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_REMAP_SUPPORT), y)
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-remap-*
endif
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_VOLUME_CONTROL_SUPPORT), y)
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-role-*
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-mmkbd-evdev.so
endif
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_MISC_MODULES), y)
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-sine.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-sine-*
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-match.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-remap-*
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-ladspa-*
endif
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_TEST_MODULES), y)
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-null-*
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-pipe-*
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-switch-on-*
endif
	@mkdir -p $(FAKEROOT_DIR)/usr/libexec/pulse
	@mkdir -p $(FAKEROOT_DIR)/var/lib/pulse
	@mkdir -p $(FAKEROOT_DIR)/var/run/pulse
	@echo "# Make PulseAudio the default device" >> $(FAKEROOT_DIR)/etc/asound.conf
	@echo "pcm.!default { type pulse }" >> $(FAKEROOT_DIR)/etc/asound.conf
	@echo "ctl.!default { type pulse }" >> $(FAKEROOT_DIR)/etc/asound.conf
	@[ -e $(FAKEROOT_DIR)/usr/share/pulseaudio ] && find $(FAKEROOT_DIR)/usr/share/pulseaudio -name "*.conf" | xargs chmod 644
	@[ -e $(FAKEROOT_DIR)/etc/pulse ] && chmod 644 $(FAKEROOT_DIR)/etc/pulse/*
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

