##
## /S2l/prebuild/third-party/noarch/locale/make.inc
##
## History:
##   2015年8月24日 - [ypchang] created file
##
## Copyright (C) 2008-2015, Ambarella Co, Ltd.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella.
##

ifeq ($(CONFIG_AMBARELLA_LOCALE_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_locale

.PHONY: $(LOCAL_TARGET)
LOCALES :=

ifeq ($(CONFIG_AMBARELLA_LOCALE_ZH_CN_SUPPORT), y)
LOCALES += zh_CN
endif
ifeq ($(CONFIG_AMBARELLA_LOCALE_ZH_TW_SUPPORT), y)
LOCALES += zh_TW
endif
ifeq ($(CONFIG_AMBARELLA_LOCALE_ZH_HK_SUPPORT), y)
LOCALES += zh_HK
endif
ifeq ($(CONFIG_AMBARELLA_LOCALE_KO_KR_SUPPORT), y)
LOCALES += ko_KR
endif
ifeq ($(CONFIG_AMBARELLA_LOCALE_JA_JP_SUPPORT), y)
LOCALES += ja_JP
endif

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/locale
	@for locale in $(LOCALES); do \
		echo "Generating $${locale}.utf8..."; \
		localedef --prefix=$(FAKEROOT_DIR) --no-archive -c --little-endian -i $${locale} -f UTF-8 $${locale}.utf8; \
	done;
	@echo "Generating locales Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif