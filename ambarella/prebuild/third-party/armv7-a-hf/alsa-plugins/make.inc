##
## prebuild/third-party/armv7-a-hf/alsa-utils/make.inc
##
## History:
##    2012/10/17 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
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

